//==============================================================================
// file: jsdi_jni.cpp
// auth: Victor Schappert
// date: 20130618
// desc: JVM's interface, via JNI, into the JSDI DLL
//==============================================================================

#include "jsdi_windows.h"

#include "global_refs.h"
#include "java_enum.h"
#include "jni_exception.h"
#include "jni_util.h"
#include "jsdi_callback.h"
#include "stdcall_invoke.h"
#include "stdcall_thunk.h"

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
    jbyteArray          d_global; // only if the corresponding jobjectArray
                                  // element was replaced
    jboolean            d_is_copy;
};

constexpr jbyte_array_tuple NULL_TUPLE = { 0, 0, 0, JNI_FALSE };

/**
 * Purpose of the free list (frustrating!). We need to clean up all of the
 * variable indirect byte arrays, but we can't do it until everything has been
 * copied out of them. Here's an example use case:
 *
 *     __stdcall const char * returnSameString(const char * x) { return x; }
 *     dll string Library:returnSameString(string x)
 *
 * In the above example, the dll function returns a pointer to the same string
 * that we passed into it. We thus can't destroy the argument string 'x' until
 * the return value is done being copied out, or the return value may contain
 * garbage. Here's another example use case:
 *
 *     struct X { const char * x1, const char * x2 };
 *     __stdcall void internalPoint(X * x) { x->x1 = x->x2; }
 *    dll void Library:internalPoint(X * x)
 *
 * Again, until we have copied anything that can be pointed to by 'x', we
 * can't destroy any of the variable indirect arrays. If we do, x->x1 may point
 * to garbage.
 *
 * The other issues which cause it to have the form it does are:
 *     - To clean up the old variable indirect byte arrays, we need references
 *       to the jarrays. But we are replacing the jarray references in the
 *       viArray as we go, so we can't depend on the viArray to provide the
 *       jarray references.
 *     - So we have to keep around a separate viArray reference.
 *     - But JNI only guarantees a very small number of local references,
 *       while the variable indirect array may be an arbitrary length.
 *     - So we need to create add global references to the jarrays to the free
 *       list so we can delete them later.
 */

/**
 * Used by the variable indirect code.
 */
struct jbyte_array_container : private non_copyable
{
    typedef std::vector<jbyte_array_tuple> vector_type;
    vector_type    d_arrays;
    JNIEnv      *  d_env;
    jobjectArray   d_object_array;
    jbyte_array_container(size_t size, JNIEnv * env, jobjectArray object_array)
        : d_arrays(size, NULL_TUPLE)
        , d_env(env)
        , d_object_array(object_array)
    { }
    ~jbyte_array_container()
    {
        try
        {
            const size_t N(d_arrays.size());
            for (size_t k = 0; k < N; ++k)
            {
                jbyte_array_tuple& tuple(d_arrays[k]);
                if (! tuple.d_elems) continue;
                if (tuple.d_global)
                {
                    d_env->ReleaseByteArrayElements(tuple.d_global,
                                                    tuple.d_elems, 0);
                    d_env->DeleteGlobalRef(tuple.d_global);
                }
                else
                {
                    jni_auto_local<jobject> object(
                        d_env, d_env->GetObjectArrayElement(d_object_array, k));
                    assert(d_env->IsInstanceOf(object,
                                               GLOBAL_REFS->byte_ARRAY()));
                    d_env->ReleaseByteArrayElements(
                        static_cast<jbyteArray>(static_cast<jobject>(object)),
                        tuple.d_elems, 0);
                }
            }
        }
        catch (...)
        { }
    }
    void put_not_null(size_t pos, JNIEnv * env, jbyteArray array,
                      jbyte ** pp_array)
    { // NORMAL USE (arguments)
        jbyte_array_tuple& tuple = d_arrays[pos];
        assert(! tuple.d_elems || !"duplicate variable indirect pointer");
        tuple.d_elems = env->GetByteArrayElements(array, &tuple.d_is_copy);
        tuple.d_pp_arr = pp_array;
        *pp_array = tuple.d_elems;
    }
    void put_return_value(size_t pos, jbyte * str)
    { // FOR USE BY RETURN VALUE
        jbyte_array_tuple& tuple = d_arrays[pos];
        *tuple.d_pp_arr = str;
    }
    void put_null(size_t pos, jbyte ** pp_array)
    {
        jbyte_array_tuple& tuple = d_arrays[pos];
        assert(! tuple.d_elems || !"duplicate variable indirect pointer");
        tuple.d_pp_arr = pp_array;
    }
    void replace_byte_array(size_t pos, jobject new_object /* may be null */)
    {
        jbyte_array_tuple& tuple = d_arrays[pos];
        // If an existing byte array is already fetched at this location, we
        // need to store a global reference to the array so we can release the
        // elements on destruction.
        if (tuple.d_elems)
        {
            assert(! tuple.d_global || !"element already replaced once");
            jni_auto_local<jobject> prev_object(
                d_env, d_env->GetObjectArrayElement(d_object_array, pos));
            assert(d_env->IsInstanceOf(prev_object, GLOBAL_REFS->byte_ARRAY()));
            tuple.d_global = static_cast<jbyteArray>(
                d_env->NewGlobalRef(prev_object));
        }
        d_env->SetObjectArrayElement(d_object_array, pos, new_object);
    }
};

jstring make_jstring(JNIEnv * env, jbyte * str_bytes)
{
    const std::vector<jchar> jchars(
        widen(reinterpret_cast<const char *>(str_bytes)));
    return env->NewString(jchars.data(), jchars.size());
}

enum { UNKNOWN_LOCATION = -1 };

inline jlong invoke_stdcall_basic(int args_size_bytes, const jbyte * args_ptr,
                                  jlong func_ptr)
{
    return stdcall_invoke::basic(args_size_bytes,
                                 reinterpret_cast<const char *>(args_ptr),
                                 reinterpret_cast<void *>(func_ptr));
}

inline jlong invoke_stdcall_return_double(int args_size_bytes,
                                          const jbyte * args_ptr, jlong func_ptr)
{
    union {
        volatile double d;
        volatile jlong  l;
    };
    d = stdcall_invoke::return_double(
        args_size_bytes, reinterpret_cast<const char *>(args_ptr),
        reinterpret_cast<void *>(func_ptr));
    return l;
}

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
                    vi_array_cpp.replace_byte_array(k, 0);
                }
                else
                {
                    jni_auto_local<jstring> str(
                        env, make_jstring(env, *tuple.d_pp_arr));
                    vi_array_cpp.replace_byte_array(k, str);
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
                    vi_array_cpp.replace_byte_array(k, int_resource);
                }
                else
                {
                    jni_auto_local<jstring> str(
                        env, make_jstring(env, *tuple.d_pp_arr));
                    vi_array_cpp.replace_byte_array(k, str);
                }
                break;
            default:
                assert(!"control should never pass here");
                break;
        }
    }
}

template<typename InvokeFunc>
inline jlong call_direct(JNIEnv * env, jlong funcPtr, jint sizeDirect,
                         jbyteArray args, InvokeFunc invokeFunc)
{
    jlong result;
    JNI_EXCEPTION_SAFE_BEGIN
    // TODO: tracing
    // NOTE: I had earlier noted that you could write a critical array version
    //       of jni_array (GetPrimitiveArrayCritical,
    //       ReleasePrimitiveArrayCritical). However, this isn't actually
    //       possible in the general case since a limitation of these functions
    //       is that you can't make other JNI calls while you have a critical
    //       array pinned. But in general it is possible for 'call_direct' to
    //       invoke a DLL function which invokes a callback which calls back
    //       into Java code. If this doesn't break the restriction by itself,
    //       the callback could easily call a second DLL function itself, and
    //       we're right back here. Bottom line, you can't use the critical
    //       versions *unless* we introduce a further optimization by separating
    //       invocations that might invoke a callback from those which are
    //       guaranteed not to.
    jni_array_region<jbyte> args_(env, args, sizeDirect);
    result = invokeFunc(sizeDirect, args_.data(), funcPtr);
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

template<typename InvokeFunc>
inline jlong call_indirect(JNIEnv * env, jlong funcPtr, jint sizeDirect,
                           jbyteArray args, jintArray ptrArray,
                           InvokeFunc invokeFunc)
{
    jlong result;
    JNI_EXCEPTION_SAFE_BEGIN
    jni_array<jbyte> args_(env, args);
    jni_array_region<jint> ptr_array(env, ptrArray);
    ptrs_init(args_.data(), ptr_array.data(), ptr_array.size());
    result = invoke_stdcall_basic(sizeDirect, args_.data(), funcPtr);
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

template <typename InvokeFunc>
inline jlong call_vi(JNIEnv * env, jlong funcPtr, jint sizeDirect,
                     jbyteArray args, jintArray ptrArray, jobjectArray viArray,
                     jintArray viInstArray, InvokeFunc invokeFunc)
{
    jlong result;
    JNI_EXCEPTION_SAFE_BEGIN
    jni_array<jbyte> args_(env, args);
    jni_array_region<jint> ptr_array(env, ptrArray);
    jbyte_array_container vi_array_cpp(env->GetArrayLength(viArray), env,
                                       viArray);
    ptrs_init_vi(args_.data(), args_.size(), ptr_array.data(), ptr_array.size(),
                 env, viArray, vi_array_cpp);
    result = invoke_stdcall_basic(sizeDirect, args_.data(), funcPtr);
    jni_array_region<jint> vi_inst_array(env, viInstArray);
    ptrs_finish_vi(env, viArray, vi_array_cpp, vi_inst_array);
    JNI_EXCEPTION_SAFE_END(env);
    return result;
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
 * Method:    callReturnInt64
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_dll_NativeCall_callReturnInt64(
    JNIEnv *, jclass, jlong funcPtr)
{
    // TODO: tracing
    return reinterpret_cast<__stdcall jlong (*)()>(funcPtr)();
}

/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callLLReturnInt64
 * Signature: (JII)J
 */
JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_dll_NativeCall_callLReturnInt64(
    JNIEnv *, jclass, jlong funcPtr, jint arg0)
{
    // TODO: tracing
    return reinterpret_cast<__stdcall jlong (*)(long)>(funcPtr)(arg0);
}

/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callLLReturnInt64
 * Signature: (JII)J
 */
JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_dll_NativeCall_callLLReturnInt64(
    JNIEnv *, jclass, jlong funcPtr, jint arg0, jint arg1)
{
    // TODO: tracing
    return reinterpret_cast<__stdcall jlong (*)(long, long)>(funcPtr)(arg0, arg1);
}

/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callLLLReturnInt64
 * Signature: (JIII)J
 */
JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_dll_NativeCall_callLLLReturnInt64(
    JNIEnv *, jclass, jlong funcPtr, jint arg0, jint arg1, jint arg2)
{
    // TODO: tracing
    return reinterpret_cast<__stdcall long (*)(long, long, long)>(funcPtr)(arg0,
                                                                           arg1,
                                                                           arg2);
}

/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callDirectReturnInt64
 * Signature: (JI[B)J
 */
JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_dll_NativeCall_callDirectReturnInt64(
    JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jbyteArray args)
{ return call_direct(env, funcPtr, sizeDirect, args, invoke_stdcall_basic); }

/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callIndirectReturnInt64
 * Signature: (JI[B[I)J
 */
JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_dll_NativeCall_callIndirectReturnInt64(
    JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jbyteArray args,
    jintArray ptrArray)
{
    return call_indirect(env, funcPtr, sizeDirect, args, ptrArray,
                         invoke_stdcall_basic);
}


/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callVariableIndirectReturnInt64
 * Signature: (JI[B[I[Ljava/lang/Object;[I)J
 */
JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_dll_NativeCall_callVariableIndirectReturnInt64(
    JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jbyteArray args,
    jintArray ptrArray, jobjectArray viArray, jintArray viInstArray)
{
    return call_vi(env, funcPtr, sizeDirect, args, ptrArray, viArray,
                   viInstArray, invoke_stdcall_basic);
}

/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callDirectReturnDouble
 * Signature: (JI[B)J
 */
JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_dll_NativeCall_callDirectReturnDouble(
    JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jbyteArray args)
{
    return call_direct(env, funcPtr, sizeDirect, args,
                       invoke_stdcall_return_double);
}

/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callIndirectReturnDouble
 * Signature: (JI[B[I)J
 */
JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_dll_NativeCall_callIndirectReturnDouble(
    JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jbyteArray args,
    jintArray ptrArray)
{
    return call_indirect(env, funcPtr, sizeDirect, args, ptrArray,
                         invoke_stdcall_return_double);
}

/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callVariableIndirectReturnDouble
 * Signature: (JI[B[I[Ljava/lang/Object;[I)J
 */
JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_dll_NativeCall_callVariableIndirectReturnDouble(
    JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jbyteArray args,
    jintArray ptrArray, jobjectArray viArray, jintArray viInstArray)
{
    return call_vi(env, funcPtr, sizeDirect, args, ptrArray, viArray,
                   viInstArray, invoke_stdcall_return_double);
}

/*
 * Class:     suneido_language_jsdi_dll_NativeCall
 * Method:    callVariableIndirectReturnVariableIndirect
 * Signature: (JI[B[I[Ljava/lang/Object;[I)V
 */
JNIEXPORT void JNICALL Java_suneido_language_jsdi_dll_NativeCall_callVariableIndirectReturnVariableIndirect(
    JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jbyteArray args,
    jintArray ptrArray, jobjectArray viArray, jintArray viInstArray)
{
    JNI_EXCEPTION_SAFE_BEGIN
    jni_array<jbyte> args_(env, args);
    jni_array_region<jint> ptr_array(env, ptrArray);
    jbyte_array_container vi_array_cpp(env->GetArrayLength(viArray), env, viArray);
    ptrs_init_vi(args_.data(), args_.size(), ptr_array.data(), ptr_array.size(),
                 env, viArray, vi_array_cpp);
    jbyte * returned_str = reinterpret_cast<jbyte *>(invoke_stdcall_basic(
        sizeDirect, args_.data(), funcPtr));
    // Store a pointer to the return value in the last element of the
    // variable indirect array, so that it will be properly propagated back
    // to the Java side...
    assert(0 < vi_array_cpp.d_arrays.size());
    vi_array_cpp.put_return_value(vi_array_cpp.d_arrays.size() - 1,
                                  returned_str);
    jni_array_region<jint> vi_inst_array(env, viInstArray);
    ptrs_finish_vi(env, viArray, vi_array_cpp, vi_inst_array);
    JNI_EXCEPTION_SAFE_END(env);
}

//==============================================================================
//              JAVA CLASS: suneido.language.jsdi.ThunkManager
//==============================================================================

/*
 * Class:     suneido_language_jsdi_ThunkManager
 * Method:    newThunk
 * Signature: (Lsuneido/language/jsdi/type/Callback;Lsuneido/SuValue;II[II[I)V
 */
JNIEXPORT void JNICALL Java_suneido_language_jsdi_ThunkManager_newThunk(
    JNIEnv * env, jclass thunkManager, jobject callback, jobject boundValue,
    jint sizeDirect, jint sizeIndirect, jintArray ptrArray,
    jint variableIndirectCount, jintArray outThunkAddrs)
{
    JNI_EXCEPTION_SAFE_BEGIN
    jni_array_region<jint> ptr_array(env, ptrArray);
    jni_array<jint> out_thunk_addrs(env, outThunkAddrs);
    std::shared_ptr<jsdi::callback> callback_ptr;
    if (variableIndirectCount < 1)
    {
        callback_ptr.reset(
            new jsdi_callback_basic(env, callback, boundValue, sizeDirect,
                                    sizeIndirect,
                                    reinterpret_cast<int *>(ptr_array.data()),
                                    ptr_array.size()));
    }
    else
    {
        callback_ptr.reset(
            new jsdi_callback_vi(env, callback, boundValue, sizeDirect,
                                    sizeIndirect,
                                    reinterpret_cast<int *>(ptr_array.data()),
                                    ptr_array.size(), variableIndirectCount));
    }
    stdcall_thunk * thunk(new stdcall_thunk(callback_ptr));
    void * func_addr(thunk->func_addr());
    static_assert(sizeof(stdcall_thunk *) <= sizeof(jint), "fatal data loss");
    static_assert(sizeof(void *) <= sizeof(jint), "fatal data loss");
    out_thunk_addrs[env->GetStaticIntField(
        thunkManager,
        GLOBAL_REFS
            ->suneido_language_jsdi_ThunkManager__f_THUNK_OBJECT_ADDR_INDEX())] =
        reinterpret_cast<jint>(thunk);
    out_thunk_addrs[env->GetStaticIntField(
        thunkManager,
        GLOBAL_REFS
            ->suneido_language_jsdi_ThunkManager__f_THUNK_FUNC_ADDR_INDEX())] =
        reinterpret_cast<jint>(func_addr);
    JNI_EXCEPTION_SAFE_END(env);
}

/*
 * Class:     suneido_language_jsdi_ThunkManager
 * Method:    deleteThunk
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_suneido_language_jsdi_ThunkManager_deleteThunk
  (JNIEnv * env, jclass, jint thunkObjectAddr)
{
    static_assert(sizeof(stdcall_thunk *) <= sizeof(jint), "fatal data loss");
    auto thunk(reinterpret_cast<stdcall_thunk *>(thunkObjectAddr));
    delete thunk;
}

} // extern "C"

//==============================================================================
//                                  TESTS
//==============================================================================

#ifndef __NOTEST__

#include "test.h"
#include "test_exports.h"
#include "util.h"

#include <algorithm>

namespace {

template<typename FuncPtr>
inline jlong invoke_stdcall_(FuncPtr f, int nlongs, long * args)
{
    return invoke_stdcall_basic(
        nlongs * sizeof(long),
        reinterpret_cast<const jbyte *>(args),
        reinterpret_cast<jlong>(f)
    );
}

template<typename FuncPtr>
inline jlong invoke_stdcall_(FuncPtr f, int nbytes, jbyte * args)
{ return invoke_stdcall_basic(nbytes, args, reinterpret_cast<jlong>(f)); }

} // anonymous namespace

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

#endif // __NOTEST__
