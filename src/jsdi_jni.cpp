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
#include "test.h"

#include <iostream>
#include <cassert>
#include <limits>

//==============================================================================
//                                INTERNALS
//==============================================================================

namespace {

jlong invoke_stdcall(int args_size_bytes, const jbyte * args_ptr,
                     void * func_ptr)
{
    unsigned long hi32, lo32;
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
        : "r" (args_size_bytes), "r" (args_ptr), "r" (func_ptr)
        :
    );
#else
#pragma error "Replacement for inline assembler required"
#endif
    uint64_t unsigned_result(static_cast<uint64_t>(hi32) << 32 | lo32);
    return static_cast<jlong>(unsigned_result);
}

inline jlong invoke_stdcall(int arg_size_bytes, const jbyte * args_ptr, jlong func_ptr)
{ return invoke_stdcall(arg_size_bytes, args_ptr, reinterpret_cast<void *>(func_ptr)); }

} // anonymous namespace

extern "C" {

using namespace jsdi;

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

static void todo_deleteme_Throw(const char * func_name)
{
    throw jni_exception(func_name, false);
}

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
 * Signature: (JI[I[B)J
 */
JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_dll_NativeCall_callIndirect
  (JNIEnv * env, jclass, jlong, jint, jintArray, jbyteArray)
{
    jlong result;
    JNI_EXCEPTION_SAFE_BEGIN
    todo_deleteme_Throw(__FUNCTION__);
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callVariableIndirect
 * Signature: (JI[I[B[Ljava/lang/Object;[Z)J
 */
JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_dll_NativeCall_callVariableIndirect
  (JNIEnv * env, jclass, jlong, jint, jintArray, jbyteArray, jobjectArray, jbooleanArray)
{
    jlong result;
    JNI_EXCEPTION_SAFE_BEGIN
    todo_deleteme_Throw(__FUNCTION__);
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

} // extern "C"

//==============================================================================
//                                  TESTS
//==============================================================================

#ifndef __TEST_H_NO_TESTS__

#include "test_exports.h"

#include <cstring>

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

} // anonymous namespace

TEST(assembly,
    long a[4];
    invoke_stdcall_(TestVoid, 0, a);
    a[0] = static_cast<long>('a');
    assert_equals('a', static_cast<char>(invoke_stdcall_(TestChar, 1, a)));
    a[0] = 0xf1;
    assert_equals(0xf1, static_cast<short>(invoke_stdcall_(TestShort, 1, a)));
    a[0] = 0x20130725;
    assert_equals(0x20130725, static_cast<long>(invoke_stdcall_(TestLong, 1, a)));
    *reinterpret_cast<int64_t *>(a) = std::numeric_limits<int64_t>::min();
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
    a[0] = -1;
    a[1] =  1;
    a[2] =  3;
    a[3] =  5;
    assert_equals(8, static_cast<long>(invoke_stdcall_(TestSumFourLongs, 4, a)));
    *reinterpret_cast<int64_t *>(a + 1) = std::numeric_limits<int64_t>::max() - 2;
    assert_equals(
        std::numeric_limits<int64_t>::max() - 3,
        invoke_stdcall_(TestSumCharPlusInt64, 3, a)
    );
    Packed_CharCharShortLong * p_ccs =
        reinterpret_cast<Packed_CharCharShortLong *>(a);
    p_ccs->a = -1;
    p_ccs->b = -3;
    p_ccs->c = -129;
    p_ccs->d = -70000;
    assert_equals(
        -70133,
        static_cast<long>(invoke_stdcall_(
            TestSumPackedCharCharShortLong,
            (sizeof(*p_ccs) + sizeof(long) - 1) / sizeof(long),
            a
        ))
    );
    a[0] = reinterpret_cast<long>(
        "From hence ye beauties, undeceived,     \n"
        "Know, one false step is ne'er retrieved,\n"
        "    And be with caution bold.           \n"
        "Not all that tempts your wandering eyes \n"
        "And heedless hearts is lawful prize;    \n"
        "    Nor all that glitters, gold.         "
    );
    assert_equals(41*6, static_cast<long>(invoke_stdcall_(TestStrLen, 1, a)));
    assert_equals(
        std::string("hello world"),
        reinterpret_cast<const char *>(invoke_stdcall_(TestHelloWorldReturn, 0, a))
    );
    const char * str(0);
    reinterpret_cast<char const **&>(a) = &str;
    invoke_stdcall_(TestHelloWorldOutParam, 1, a);
    assert_equals(std::string("hello world"), str);
    invoke_stdcall_(TestNullPtrOutParam, 1, a);
    assert_equals(0, str);
);

#endif // __TEST_H_NO_TESTS__
