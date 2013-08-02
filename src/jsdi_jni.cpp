//==============================================================================
// file: jsdi_jni.cpp
// auth: Victor Schappert
// date: 20130618
// desc: JVM's interface, via JNI, into the JSDI DLL
//==============================================================================

#include "jsdi_windows.h"
#include "jni_exception.h"
#include "jni_util.h"
#include "java_enum.h"
#include "global_refs.h"

#include <cassert>

using namespace jsdi;

//==============================================================================
//                                INTERNALS
//==============================================================================

/* TODO: do we need to be able to handle Win32 exceptions? If so, we'll want
 *       to wrap things in SEH code *at some level*. But do we want that
 *       overhead around every DLL call, regardless of whether it is expected
 *       to throw an exception?
 */

namespace {

/**
 * Used by the variable indirect code. This is deliberately a POD type.
 *
 * The reason we are not using pointers to jni_array<jbyte> within the structure
 * is that we don't know how many arrays we will have to fetch simultaneously
 * in order to set up for calling the __stdcall function. There could be an
 * arbitrarily large number of arrays at once. If we keep the jbyteArray
 * local reference for each such array around (as jni_array does), we run the
 * risk of running over the JNI local reference limit.
 */
struct jbyte_array_tuple
{
    jbyte            *  d_elems;  // pointer to the byte array elements, OWNED
    jbyte            ** d_pp_arr; // points to the address in the marshalled
                                  // data array which contains the pointer to
                                  // this byte array
    jboolean            d_is_copy;
};

/**
 * Used by the variable indirect code.
 */
struct jbyte_array_container : private non_copyable
{
    typedef std::vector<jbyte_array_tuple> vector_type;
    vector_type   d_arrays;
    JNIEnv      * d_env;
    jobjectArray  d_object_array;
    jbyte_array_container(size_t size, JNIEnv * env, jobjectArray object_array)
        : d_arrays(size, { 0, 0, JNI_FALSE })
        , d_env(env)
        , d_object_array(object_array)
    { }
    ~jbyte_array_container()
    {
        const jsize N(d_arrays.size());
        try
        { for (jsize k = 0; k < N; ++k) free_byte_array(k); }
        catch (...)
        { }
    }
    void put_not_null(size_t pos, JNIEnv * env, jbyteArray array,
                      jbyte ** pp_array)
    {
        jbyte_array_tuple& tuple = d_arrays[pos];
        assert(! tuple.d_elems || !"duplicate variable indirect pointer");
        tuple.d_elems = env->GetByteArrayElements(array, &tuple.d_is_copy);
        tuple.d_pp_arr = pp_array;
        *pp_array = tuple.d_elems;
    }
    void put_null(size_t pos, jbyte ** pp_array)
    {
        jbyte_array_tuple& tuple = d_arrays[pos];
        assert(! tuple.d_elems || !"duplicate variable indirect pointer");
        tuple.d_pp_arr = pp_array;
    }
    void free_byte_array(size_t pos)
    {
        jbyte_array_tuple& tuple = d_arrays[pos];
        if (tuple.d_elems)
        {
            jni_auto_local<jobject> object(
                d_env, d_env->GetObjectArrayElement(d_object_array, pos));
            assert(d_env->IsInstanceOf(object, GLOBAL_REFS->byte_ARRAY()));
            d_env->ReleaseByteArrayElements(
                static_cast<jbyteArray>(static_cast<jobject>(object)),
                tuple.d_elems, 0);
            tuple = { 0, 0, JNI_FALSE };
        }
    }
};

jstring make_jstring(JNIEnv * env, jbyte * str_bytes)
{
    const std::vector<jchar> jchars(
        widen(reinterpret_cast<const char *>(str_bytes)));
    return env->NewString(jchars.data(), jchars.size());
}

enum { UNKNOWN_LOCATION = -1 };

/**
 * This function is very limited in capability. In particular, it cannot:
 *     - Call stdcall functions which return floating-point values. This is
 *       because such values are returned in ST(0), the top register of the
 *       floating-point stack, not in the EAX/EDX pair, under stdcall.
 *     - Call stdcall functions which return aggregates of 5-7, or 9+ bytes in
 *       size, because these functions require the caller to pass the address
 *       of the return value as a silent parameter in EAX.
 *
 * See: http://pic.dhe.ibm.com/infocenter/ratdevz/v7r5/index.jsp?topic=
 *     %2Fcom.ibm.etools.pl1.win.doc%2Ftopics%2Fxf6700.htm
 * See also: http://stackoverflow.com/q/17912828/1911388
 */
jlong invoke_stdcall(int args_size_bytes, const jbyte * args_ptr,
                     void * func_ptr)
{
    uint32_t hi32, lo32;
    assert(0 == args_size_bytes % 4 || !"Argument size must be a multiple of 4 bytes");
#if defined(__GNUC__)
    asm
    (
        /* OUTPUT PARAMS: %0 gets the high-order 32 bits of the return value */
        /*                %1 gets the low-order 32 bits of the return value */
        /* INPUT PARAMS:  %2 is the number of BYTES to push onto the stack, */
        /*                   and during the copy loop it is the address of */
        /*                   the next word to push */
        /*                %3 is the base address of the array */
        /*                %4 is the address of the function to call */
            "testl %2, %2    # If zero argument bytes given, skip \n\t"
            "je    2f        # right to the function call.        \n\t"
            "addl  %3, %2\n"
        "1:\n\t"
            "subl  $4, %2    # Push arguments onto the stack in   \n\t"
            "pushl (%2)      # reverse order. Keep looping while  \n\t"
            "cmp   %3, %2    # addr to push (%2) > base addr (%3) \n\t"
            "jg    1b        # Callee cleans up b/c __stdcall.    \n"
        "2:\n\t"
            "call  * %4"
        : "=d" (hi32), "=a" (lo32)
        : "0" (args_size_bytes), "1" (args_ptr), "mr" (func_ptr)
        : "%ecx" /* eax, ecx, edx are caller-save */, "cc"
    );
#else
#pragma error "Replacement for inline assembler required"
#endif
    uint64_t unsigned_result(static_cast<uint64_t>(hi32) << 32 | lo32);
    return static_cast<jlong>(unsigned_result);
}

inline jlong invoke_stdcall(int args_size_bytes, const jbyte * args_ptr, jlong func_ptr)
{ return invoke_stdcall(args_size_bytes, args_ptr, reinterpret_cast<void *>(func_ptr)); }

void ptrs_init(jbyte * args, const jint * ptr_array, jsize ptr_array_size)
{
    assert(0 == ptr_array_size % 2 || !"pointer array must have even size");
    jint const * i(ptr_array), * e(ptr_array + ptr_array_size);
    while (i < e)
    {
        jint ptr_pos = *i++;
        jint ptd_to_pos = *i++;
        // If the Java-side marshaller put UNKNOWN_LOCATION as the location to
        // point to, just skip this pointer -- we will trust that the Java side
        // put a NULL value into the data array. Otherwise, set the pointer in
        // the data array to point to the appropriate location.
        if (UNKNOWN_LOCATION != ptd_to_pos)
        {
            jbyte ** ptr_addr = reinterpret_cast<jbyte **>(&args[ptr_pos]);
            jbyte *  ptd_to_addr = &args[ptd_to_pos];
            // Possible alignment issue if the Java-side marshaller didn't set
            // things up so that the pointers are word-aligned. This is the
            // Java side's job, however, and we trust it was done properly.
            *ptr_addr = ptd_to_addr;
        }
    }
}

void ptrs_init_vi(jbyte * args, jsize args_size, const jint * ptr_array,
                  jsize ptr_array_size, JNIEnv * env, jobjectArray vi_array_in,
                  jbyte_array_container& vi_array_out)
{
    assert(0 == ptr_array_size % 2 || !"pointer array must have even size");
    jint const * i(ptr_array), * e(ptr_array + ptr_array_size);
    while (i < e)
    {
        jint ptr_pos = *i++;
        jint ptd_to_pos = *i++;
        if (UNKNOWN_LOCATION == ptd_to_pos) continue; // Leave a null pointer
        assert(0 <= ptd_to_pos || !"pointer position must be a non-negative index");
        jbyte ** ptr_addr = reinterpret_cast<jbyte **>(&args[ptr_pos]);
        if (ptd_to_pos < args_size)
        {
            // Normal pointer: points back into a location within args
            jbyte * ptd_to_addr = &args[ptd_to_pos];
            *ptr_addr = ptd_to_addr;
        }
        else
        {
            // Variable indirect pointer: points to the start of a byte[] which
            // is passed in from Java in vi_array_in, and marshalled into the
            // C++ environment in vi_array_out.
            ptd_to_pos -= args_size; // Convert to an index into vi_array
            assert(
                static_cast<size_t>(ptd_to_pos) < vi_array_out.d_arrays.size()
                || !"pointer points outside of variable indirect array"
            );
            jni_auto_local<jobject> object(
                env, env->GetObjectArrayElement(vi_array_in, ptd_to_pos));
            if (! object)
            {   // Note if this is a 'resource', the value at *ptr_addr is
                // actually a 16-bit integer which may not be NULL
                vi_array_out.put_null(ptd_to_pos, ptr_addr);
            }
            else
            {
                assert(env->IsInstanceOf(object, GLOBAL_REFS->byte_ARRAY()));
                vi_array_out.put_not_null(
                    ptd_to_pos, env,
                    static_cast<jbyteArray>(static_cast<jobject>(object)),
                    ptr_addr
                );
            }
        }
    }
}

void ptrs_finish_vi(JNIEnv * env, jobjectArray vi_array_java,
                    jbyte_array_container& vi_array_cpp,
                    const jni_array_region<jint>& vi_inst_array)
{
    const jsize N(vi_array_cpp.d_arrays.size());
    assert(
        N == vi_inst_array.size() || !"variable indirect array size mismatch");
    for (jsize k = 0; k < N; ++k)
    {
        const jbyte_array_tuple& tuple(vi_array_cpp.d_arrays[k]);
        switch (ordinal_enum_to_cpp<
            suneido_language_jsdi_VariableIndirectInstruction>(vi_inst_array[k])
        )
        {
            case NO_ACTION:
                break;
            case RETURN_JAVA_STRING:
                if (! *tuple.d_pp_arr)
                {   // null pointer, so return a null String ref
                    vi_array_cpp.free_byte_array(k);
                    env->SetObjectArrayElement(vi_array_java, k, 0);
                }
                else
                {
                    jni_auto_local<jstring> str(
                        env, make_jstring(env, *tuple.d_pp_arr));
                    vi_array_cpp.free_byte_array(k);
                    env->SetObjectArrayElement(vi_array_java, k, str);
                }
                break;
            case RETURN_RESOURCE:
                if (IS_INTRESOURCE(*tuple.d_pp_arr))
                {   // it's an INT resource, not a string, so return an Integer
                    jni_auto_local<jobject> int_resource(
                        env,
                        env->NewObject(
                            GLOBAL_REFS->java_lang_Integer(),
                            GLOBAL_REFS->java_lang_Integer__init(),
                            reinterpret_cast<int>(*tuple.d_pp_arr)
                        )
                    );
                    vi_array_cpp.free_byte_array(k);
                    env->SetObjectArrayElement(vi_array_java, k, int_resource);
                }
                else
                {
                    jni_auto_local<jstring> str(
                        env, make_jstring(env, *tuple.d_pp_arr));
                    vi_array_cpp.free_byte_array(k);
                    env->SetObjectArrayElement(vi_array_java, k, str);
                }
                break;
            default:
                assert(!"control should never pass here");
                break;
        }
    }
}

} // anonymous namespace

extern "C" {

//==============================================================================
//                  JAVA CLASS: suneido.language.jsdi.JSDI
//==============================================================================

#include "gen/suneido_language_jsdi_JSDI.h"
    // This #include isn't strictly necessary -- the only caller of these
    // functions is the JVM. However, it is useful to have the generated code
    // around.

JNIEXPORT void JNICALL Java_suneido_language_jsdi_JSDI_init
  (JNIEnv * env, jclass)
{
    JNI_EXCEPTION_SAFE_BEGIN;
    global_refs::init(env);
    JNI_EXCEPTION_SAFE_END(env);
}

JNIEXPORT jstring JNICALL Java_suneido_language_jsdi_JSDI_when
  (JNIEnv * env, jclass)
{
    jstring result(0);
    JNI_EXCEPTION_SAFE_BEGIN
    jni_utf16_ostream o(env);
    o << u"todo: make when() result"; // TODO: make when() result
    result = o.jstr();
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

//==============================================================================
//             JAVA CLASS: suneido.language.jsdi.dll.DllFactory
//==============================================================================

#include "gen/suneido_language_jsdi_dll_DllFactory.h"
    // This #include isn't strictly necessary -- the only caller of these
    // functions is the JVM. However, it is useful to have the generated code
    // around.

/*
 * Class:     suneido_language_jsdi_dll_DllFactory
 * Method:    loadLibrary
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_dll_DllFactory_loadLibrary
  (JNIEnv * env, jclass, jstring libraryName)
{
    jlong result(0);
    JNI_EXCEPTION_SAFE_BEGIN
    jni_utf16_string_region libraryName_(env, libraryName);
    HMODULE hmodule = LoadLibraryW(libraryName_.wstr());
    result = reinterpret_cast<jlong>(hmodule);
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

/*
 * Class:     suneido_language_jsdi_dll_DllFactory
 * Method:    freeLibrary
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_suneido_language_jsdi_dll_DllFactory_freeLibrary
  (JNIEnv * env, jclass, jlong hModule)
{
    FreeLibrary(reinterpret_cast<HMODULE>(hModule));
}

/*
 * Class:     suneido_language_jsdi_dll_DllFactory
 * Method:    getProcAddress
 * Signature: (JLjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_dll_DllFactory_getProcAddress
  (JNIEnv * env, jclass, jlong hModule, jstring procName)
{
    jlong result(0);
    JNI_EXCEPTION_SAFE_BEGIN
    jni_utf8_string_region procName_(env, procName);
    FARPROC addr = GetProcAddress(reinterpret_cast<HMODULE>(hModule),
        procName_.str());
        // NOTE: There is no GetProcAddressW... GetProcAddress() only accepts
        //       ANSI strings.
    result = reinterpret_cast<jlong>(addr);
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

//==============================================================================
//             JAVA CLASS: suneido.language.jsdi.dll.NativeCall
//==============================================================================

#include "gen/suneido_language_jsdi_dll_NativeCall.h"
    // This #include isn't strictly necessary -- the only caller of these
    // functions is the JVM. However, it is useful to have the generated code
    // around.

/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callReturnV
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_suneido_language_jsdi_dll_NativeCall_callReturnV
  (JNIEnv *, jclass, jlong funcPtr)
{
    // TODO: tracing
    reinterpret_cast<__stdcall void (*)()>(funcPtr)();
}

/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callLReturnV
 * Signature: (JI)V
 */
JNIEXPORT void JNICALL Java_suneido_language_jsdi_dll_NativeCall_callLReturnV
  (JNIEnv *, jclass, jlong funcPtr, jint arg0)
{
    // TODO: tracing
    reinterpret_cast<__stdcall void (*)(long)>(funcPtr)(arg0);
}

/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callLLReturnV
 * Signature: (JII)V
 */
JNIEXPORT void JNICALL Java_suneido_language_jsdi_dll_NativeCall_callLLReturnV
  (JNIEnv *, jclass, jlong funcPtr, jint arg0, jint arg1)
{
    // TODO: tracing
    reinterpret_cast<__stdcall void (*)(long, long)>(funcPtr)(arg0, arg1);
}

/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callLLLReturnV
 * Signature: (JIII)V
 */
JNIEXPORT void JNICALL Java_suneido_language_jsdi_dll_NativeCall_callLLLReturnV
  (JNIEnv *, jclass, jlong funcPtr, jint arg0, jint arg1, jint arg2)
{
    // TODO: tracing
    reinterpret_cast<__stdcall void (*)(long, long, long)>(funcPtr)(arg0, arg1,
                                                                    arg2);
}

/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callReturnL
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_suneido_language_jsdi_dll_NativeCall_callReturnL
  (JNIEnv *, jclass, jlong funcPtr)
{
    // TODO: tracing
    return reinterpret_cast<__stdcall long (*)()>(funcPtr)();
}

/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callLReturnL
 * Signature: (JI)I
 */
JNIEXPORT jint JNICALL Java_suneido_language_jsdi_dll_NativeCall_callLReturnL
  (JNIEnv *, jclass, jlong funcPtr, jint arg0)
{
    // TODO: tracing
    return reinterpret_cast<__stdcall long (*)(long)>(funcPtr)(arg0);
}

/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callLLReturnL
 * Signature: (JII)I
 */
JNIEXPORT jint JNICALL Java_suneido_language_jsdi_dll_NativeCall_callLLReturnL
  (JNIEnv *, jclass, jlong funcPtr, jint arg0, jint arg1)
{
    // TODO: tracing
    return reinterpret_cast<__stdcall long (*)(long, long)>(funcPtr)(arg0, arg1);
}

/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callLLLReturnL
 * Signature: (JIII)I
 */
JNIEXPORT jint JNICALL Java_suneido_language_jsdi_dll_NativeCall_callLLLReturnL(
    JNIEnv *, jclass, jlong funcPtr, jint arg0, jint arg1, jint arg2)
{
    // TODO: tracing
    return reinterpret_cast<__stdcall long (*)(long, long, long)>(funcPtr)(arg0,
                                                                           arg1,
                                                                           arg2);
}

/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callDirectOnly
 * Signature: (JI[B)J
 */
JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_dll_NativeCall_callDirectOnly
  (JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jbyteArray args)
{
    jlong result;
    JNI_EXCEPTION_SAFE_BEGIN
    // TODO: tracing
    jni_array_region<jbyte> args_(env, args, sizeDirect);
    result = invoke_stdcall(sizeDirect, &args_[0], funcPtr);
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callIndirect
 * Signature: (JI[B[I)J
 */
JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_dll_NativeCall_callIndirect(
    JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jbyteArray args,
    jintArray ptrArray)
{
    jlong result;
    JNI_EXCEPTION_SAFE_BEGIN
    jni_array<jbyte> args_(env, args);
    jni_array_region<jint> ptr_array(env, ptrArray);
    ptrs_init(&args_[0], &ptr_array[0], ptr_array.size());
    result = invoke_stdcall(sizeDirect, &args_[0], funcPtr);
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callVariableIndirect
 * Signature: (JI[B[I[Ljava/lang/Object;[I)J
 */
JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_dll_NativeCall_callVariableIndirect(
    JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jbyteArray args,
    jintArray ptrArray, jobjectArray viArray, jintArray viInstArray)
{
    jlong result;
    JNI_EXCEPTION_SAFE_BEGIN
    jni_array<jbyte> args_(env, args);
    jni_array_region<jint> ptr_array(env, ptrArray);
    jbyte_array_container vi_array_cpp(env->GetArrayLength(viArray), env, viArray);
    ptrs_init_vi(&args_[0], args_.size(), &ptr_array[0], ptr_array.size(),
                 env, viArray, vi_array_cpp);
    result = invoke_stdcall(sizeDirect, &args_[0], funcPtr);
    jni_array_region<jint> vi_inst_array(env, viInstArray);
    ptrs_finish_vi(env, viArray, vi_array_cpp, vi_inst_array);
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

} // extern "C"

//==============================================================================
//                                  TESTS
//==============================================================================

#ifndef __TEST_H_NO_TESTS__

#include "test.h"
#include "test_exports.h"
#include "util.h"

#include <algorithm>

namespace {

template<typename FuncPtr>
inline jlong invoke_stdcall_(FuncPtr f, int nlongs, long * args)
{
    return invoke_stdcall(
        nlongs * sizeof(long),
        reinterpret_cast<const jbyte *>(args),
        reinterpret_cast<jlong>(f)
    );
}

template<typename FuncPtr>
inline jlong invoke_stdcall_(FuncPtr f, int nbytes, jbyte * args)
{ return invoke_stdcall(nbytes, args, reinterpret_cast<jlong>(f)); }

} // anonymous namespace

TEST(assembly,
    union
    {
        long a[4];
        const char * str;
        const char ** pstr;
        Packed_CharCharShortLong p_ccsl;
        int64_t int64;
    };
    invoke_stdcall_(TestVoid, 0, a);
    a[0] = static_cast<long>('a');
    assert_equals('a', static_cast<char>(invoke_stdcall_(TestChar, 1, a)));
    a[0] = 0xf1;
    assert_equals(0xf1, static_cast<short>(invoke_stdcall_(TestShort, 1, a)));
    a[0] = 0x20130725;
    assert_equals(0x20130725, static_cast<long>(invoke_stdcall_(TestLong, 1, a)));
    int64 = std::numeric_limits<int64_t>::min();
    assert_equals(
        std::numeric_limits<int64_t>::min(),
        invoke_stdcall_(TestInt64, 2, a)
    );
    a[0] = 0x80; // this is -128 as a char
    a[1] = 0x7f; // this is 127 as a char
    assert_equals(
        static_cast<char>(0xff),
        static_cast<char>(invoke_stdcall_(TestSumTwoChars, 2, a))
    );
    a[0] = 0x8000;
    a[1] = 0x7fff;
    assert_equals(
        static_cast<short>(0xffff),
        static_cast<short>(invoke_stdcall_(TestSumTwoShorts, 2, a))
    );
    a[0] = std::numeric_limits<long>::min() + 5;
    a[1] = -5;
    assert_equals(
        std::numeric_limits<long>::min(),
        static_cast<long>(invoke_stdcall_(TestSumTwoLongs, 2, a))
    );
    a[2] = std::numeric_limits<long>::max();
    assert_equals(
        std::numeric_limits<long>::max() + std::numeric_limits<long>::min(),
        static_cast<long>(invoke_stdcall_(TestSumThreeLongs, 3, a))
    );
    a[0] = -100;
    a[1] =   99;
    a[2] = -200;
    a[3] =  199;
    assert_equals(-2, static_cast<long>(invoke_stdcall_(TestSumFourLongs, 4, a)));
    a[0] = -1;
    *reinterpret_cast<int64_t *>(a + 1) = std::numeric_limits<int64_t>::max() - 2;
    assert_equals(
        std::numeric_limits<int64_t>::max() - 3,
        invoke_stdcall_(TestSumCharPlusInt64, 3, a)
    );
    p_ccsl.a = -1;
    p_ccsl.b = -3;
    p_ccsl.c = -129;
    p_ccsl.d = -70000;
    assert_equals(
        -70133,
        static_cast<long>(invoke_stdcall_(
            TestSumPackedCharCharShortLong,
            (sizeof(p_ccsl) + sizeof(long) - 1) / sizeof(long),
            a
        ))
    );
    str =
        "From hence ye beauties, undeceived,     \n"
        "Know, one false step is ne'er retrieved,\n"
        "    And be with caution bold.           \n"
        "Not all that tempts your wandering eyes \n"
        "And heedless hearts is lawful prize;    \n"
        "    Nor all that glitters, gold.         ";
    assert_equals(41*6, static_cast<long>(invoke_stdcall_(TestStrLen, 1, a)));
    a[0] = 1; // true
    assert_equals(
        std::string("hello world"),
        reinterpret_cast<const char *>(invoke_stdcall_(TestHelloWorldReturn, 1, a))
    );
    const char * tmp_str(0);
    pstr = &tmp_str;
    invoke_stdcall_(TestHelloWorldOutParam, 1, a);
    assert_equals(std::string("hello world"), tmp_str);
    invoke_stdcall_(TestNullPtrOutParam, 1, a);
    assert_equals(0, tmp_str);
);

TEST(ptrs_init,
    // single indirection
    {
        jbyte args[sizeof(long *) + sizeof(long)];
        jint ptr_array[] = { 0, sizeof(long *) };
        long *& ptr = reinterpret_cast<long *&>(args);
        long const & ptd_to = reinterpret_cast<long const &>(*(args + sizeof(long *)));
        ptrs_init(args, ptr_array, array_length(ptr_array));
        *ptr = 0x19820207;
        assert_equals(ptd_to, 0x19820207);
    }
    // double indirection
    {
        jbyte args[sizeof(long **) + sizeof(long *) + sizeof(long)];
        jint ptr_array[] = { 0, sizeof(long **), sizeof(long **), sizeof(long **) + sizeof(long *) };
        long **& ptr_ptr = reinterpret_cast<long **&>(args);
        long const & ptd_to = reinterpret_cast<long const &>(
            *(args + sizeof(long **) + sizeof(long *))
        );
        ptrs_init(args, ptr_array, array_length(ptr_array));
        **ptr_ptr = 0x19900606;
        assert_equals(0x19900606, ptd_to);
    }
    // triple indirection
    {
        union u_
        {
            struct
            {
                double *** ptr_ptr_ptr;
                double **  ptr_ptr;
                double *   ptr;
                double     value;
            } x;
            jbyte args[sizeof(x)];
        } u;
        jint ptr_array[] =
        {
            0,
                reinterpret_cast<char *>(&u.x.ptr_ptr) - reinterpret_cast<char *>(&u.args[0]),
            reinterpret_cast<char *>(&u.x.ptr_ptr) - reinterpret_cast<char *>(&u.args[0]),
                reinterpret_cast<char *>(&u.x.ptr) - reinterpret_cast<char *>(&u.args[0]),
            reinterpret_cast<char *>(&u.x.ptr) - reinterpret_cast<char *>(&u.args[0]),
                reinterpret_cast<char *>(&u.x.value) - reinterpret_cast<char *>(&u.args[0]),
        };
        ptrs_init(u.args, ptr_array, array_length(ptr_array));
        const double expect = -123456789.0 + (1.0 / 32.0);
        ***u.x.ptr_ptr_ptr = expect;
        assert_equals(expect, u.x.value);
        for (int k = 0; k < 10; ++k)
        {
            std::shared_ptr<u_> u2(new u_);
            std::fill(u2->args, u2->args + array_length(u2->args), 0);
            std::vector<jint> ptr_vector(ptr_array, ptr_array + array_length(ptr_array));
            assert(array_length(ptr_array) == ptr_vector.size());
            ptrs_init(u2->args, &ptr_vector[0], ptr_vector.size());
            const double expect2 = double(k) * double(k) * double(k);
            ***u2->x.ptr_ptr_ptr = expect2;
            union
            {
                double   as_dbl;
                uint64_t as_uint64;
            };
            as_uint64 = TestReturnPtrPtrPtrDoubleAsUInt64(u2->x.ptr_ptr_ptr);
            assert_equals(expect2, as_dbl);
            as_uint64 = static_cast<uint64_t>(
                invoke_stdcall_(TestReturnPtrPtrPtrDoubleAsUInt64, sizeof (double ***), u2->args)
            );
            assert_equals(expect2, as_dbl);
        }
    }
);

#endif // __TEST_H_NO_TESTS__
