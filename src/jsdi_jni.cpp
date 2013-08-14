//==============================================================================
// file: jsdi_jni.cpp
// auth: Victor Schappert
// date: 20130618
// desc: JVM's interface, via JNI, into the JSDI DLL
//==============================================================================

#include "jni_exception.h"
#include "jni_util.h"
#include "jsdi_callback.h"
#include "jsdi_windows.h"
#include "marshalling.h"
#include "stdcall_invoke.h"
#include "stdcall_thunk.h"

#include <cassert>
#include <cstring>

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
    marshalling_roundtrip::ptrs_init(args_.data(), ptr_array.data(), ptr_array.size());
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
    marshalling_vi_container vi_array_cpp(env->GetArrayLength(viArray), env,
                                       viArray);
    marshalling_roundtrip::ptrs_init_vi(args_.data(), args_.size(),
                                        ptr_array.data(), ptr_array.size(),
                                        env, viArray, vi_array_cpp);
    result = invoke_stdcall_basic(sizeDirect, args_.data(), funcPtr);
    jni_array_region<jint> vi_inst_array(env, viInstArray);
    marshalling_roundtrip::ptrs_finish_vi(env, viArray, vi_array_cpp,
                                          vi_inst_array);
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

inline const char * get_struct_ptr(jint struct_addr, jint size_direct)
{
    assert(struct_addr || !"can't copy out a NULL pointer");
    assert(0 < size_direct || !"structure must have positive size");
    return reinterpret_cast<const char *>(struct_addr);
}

} // anonymous namespace

extern "C" {

//==============================================================================
//                  JAVA CLASS: suneido.language.jsdi.JSDI
//==============================================================================

#include "gen/suneido_language_jsdi_JSDI.h"
    // This #include isn't strictly necessary -- the only caller of these
    // functions is the JVM. However, it is useful to have the generated code
    // around. Also, because you can only have one extern "C" symbol with the
    // same name, including the header allows the compiler to find prototype
    // declaration/definition conflicts.

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
    // around. Also, because you can only have one extern "C" symbol with the
    // same name, including the header allows the compiler to find prototype
    // declaration/definition conflicts.

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
    // around. Also, because you can only have one extern "C" symbol with the
    // same name, including the header allows the compiler to find prototype
    // declaration/definition conflicts.

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
    marshalling_vi_container vi_array_cpp(env->GetArrayLength(viArray), env, viArray);
    marshalling_roundtrip::ptrs_init_vi(args_.data(), args_.size(),
                                        ptr_array.data(), ptr_array.size(),
                                        env, viArray, vi_array_cpp);
    jbyte * returned_str = reinterpret_cast<jbyte *>(invoke_stdcall_basic(
        sizeDirect, args_.data(), funcPtr));
    // Store a pointer to the return value in the last element of the
    // variable indirect array, so that it will be properly propagated back
    // to the Java side...
    assert(0 < vi_array_cpp.size());
    vi_array_cpp.put_return_value(vi_array_cpp.size() - 1, returned_str);
    jni_array_region<jint> vi_inst_array(env, viInstArray);
    marshalling_roundtrip::ptrs_finish_vi(env, viArray, vi_array_cpp,
                                          vi_inst_array);
    JNI_EXCEPTION_SAFE_END(env);
}

//==============================================================================
//              JAVA CLASS: suneido.language.jsdi.ThunkManager
//==============================================================================

#include "gen/suneido_language_jsdi_ThunkManager.h"
    // This #include isn't strictly necessary -- the only caller of these
    // functions is the JVM. However, it is useful to have the generated code
    // around. Also, because you can only have one extern "C" symbol with the
    // same name, including the header allows the compiler to find prototype
    // declaration/definition conflicts.

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

//==============================================================================
//                JAVA CLASS: suneido.language.jsdi.Structure
//==============================================================================

#include "gen/suneido_language_jsdi_type_Structure.h"
    // This #include isn't strictly necessary -- the only caller of these
    // functions is the JVM. However, it is useful to have the generated code
    // around. Also, because you can only have one extern "C" symbol with the
    // same name, including the header allows the compiler to find prototype
    // declaration/definition conflicts.

/*
 * Class:     suneido_language_jsdi_type_Structure
 * Method:    copyOutDirect
 * Signature: (I[BI)V
 */
JNIEXPORT void JNICALL Java_suneido_language_jsdi_type_Structure_copyOutDirect(
    JNIEnv * env, jclass, jint structAddr, jbyteArray data, jint sizeDirect)
{
    JNI_EXCEPTION_SAFE_BEGIN
    auto ptr(get_struct_ptr(structAddr, sizeDirect));
    // NOTE: In contrast to most other situations, it is safe to use a primitive
    // critical array here because in a struct copy out, we don't call any other
    // JNI functions (nor is it possible to surreptitiously re-enter the Java
    // world via a callback).
    jni_critical_array<jbyte> data_(env, data, sizeDirect);
    std::memcpy(data_.data(), ptr, sizeDirect);
    JNI_EXCEPTION_SAFE_END(env);
}

/*
 * Class:     suneido_language_jsdi_type_Structure
 * Method:    copyOutIndirect
 * Signature: (I[BI[I)V
 */
JNIEXPORT void JNICALL Java_suneido_language_jsdi_type_Structure_copyOutIndirect(
    JNIEnv * env, jclass, jint structAddr, jbyteArray data, jint sizeDirect,
    jintArray ptrArray)
{
    JNI_EXCEPTION_SAFE_BEGIN
    auto ptr(get_struct_ptr(structAddr, sizeDirect));
    // See note above: critical arrays safe here.
    const jni_array_region<jint> ptr_array(env, ptrArray);
    jni_critical_array<jbyte> data_(env, data);
    unmarshaller_indirect u(sizeDirect, data_.size(),
                            reinterpret_cast<const int *>(ptr_array.data()),
                            reinterpret_cast<const int *>(ptr_array.data() +
                                ptr_array.size()));
    u.unmarshall_indirect(reinterpret_cast<char *>(data_.data()), ptr);
    JNI_EXCEPTION_SAFE_END(env);
}

/*
 * Class:     suneido_language_jsdi_type_Structure
 * Method:    copyOutVariableIndirect
 * Signature: (I[BI[I[Ljava/lang/Object;[I)V
 */
JNIEXPORT void JNICALL Java_suneido_language_jsdi_type_Structure_copyOutVariableIndirect(
    JNIEnv * env, jclass, jint structAddr, jbyteArray data, jint sizeDirect,
    jintArray ptrArray, jobjectArray viArray, jintArray viInstArray)
{
    JNI_EXCEPTION_SAFE_BEGIN
    auto ptr(get_struct_ptr(structAddr, sizeDirect));
    // Can't use critical arrays here because the unmarshalling process isn't
    // guaranteed to follow the JNI critical array function restrictions.
    jni_array<jbyte> data_(env, data);
    const jni_array_region<jint> ptr_array(env, ptrArray);
    const jni_array_region<jint> vi_inst_array(env, viInstArray);
    unmarshaller_vi u(sizeDirect, data_.size(),
                      reinterpret_cast<const int *>(ptr_array.data()),
                      reinterpret_cast<const int *>(ptr_array.data() +
                          ptr_array.size()),
                      vi_inst_array.size());
    u.unmarshall_vi(reinterpret_cast<char *>(data_.data()), ptr, env,
                    viArray,
                    reinterpret_cast<const int *>(vi_inst_array.data()));
    JNI_EXCEPTION_SAFE_END(env);
}

} // extern "C"
