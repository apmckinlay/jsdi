/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: _jni_interface_x86.cpp
// auth: Victor Schappert
// date: 20140707
// desc: JVM's interface for functionality specific to the x86 __stdcall ABI.
//==============================================================================

#include "global_refs.h"
#include "jni_exception.h"
#include "jsdi_callback.h"
#include "log.h"
#include "marshalling.h"
#include "stdcall_invoke.h"
#include "stdcall_thunk.h"

#include <deque>
#include <mutex>
#include <cassert>
#include <cstring>

using namespace jsdi;

//==============================================================================
//                                INTERNALS
//==============================================================================

/* TODO: do we need to be able to handle Win32 exceptions? If so, we'll want
 *       to wrap things in SEH code *at some level*. But do we want that
 *       overhead around every DLL call, regardless of whether it is expected
 *       to throw an exception? [See NOTES A-D in jsdi_callback.cpp]
 */

namespace {

inline jlong invoke_stdcall_basic(JNIEnv * env, int args_size_bytes,
                                  jbyte * args_ptr, jlong func_ptr)
{
    jlong result = stdcall_invoke::basic(
        args_size_bytes, reinterpret_cast<char *>(args_ptr),
        reinterpret_cast<void *>(func_ptr));
    JNI_EXCEPTION_CHECK(env); // In case callback triggered exception...
    return result;
}

inline jlong invoke_stdcall_return_double(JNIEnv * env, int args_size_bytes,
                                          jbyte * args_ptr,
                                          jlong func_ptr)
{
    union {
        volatile double d;
        volatile jlong  l;
    };
    d = stdcall_invoke::return_double(
        args_size_bytes, reinterpret_cast<char *>(args_ptr),
        reinterpret_cast<void *>(func_ptr));
    JNI_EXCEPTION_CHECK(env); // In case callback triggered exception...
    return l;
}

template<typename InvokeFunc>
inline jlong call_direct(JNIEnv * env, jlong funcPtr, jint sizeDirect,
                         jbyteArray args, InvokeFunc invokeFunc)
{
    jlong result(0);
    JNI_EXCEPTION_SAFE_BEGIN
    LOG_TRACE("funcPtr => "    << reinterpret_cast<void *>(funcPtr) << ", " <<
              "sizeDirect => " << sizeDirect);
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
    // NOTE: The *unless* clause above is WRONG. You can never guarantee a
    //       native call won't trigger a callback because (unless we force the
    //       user to decorate the 'dll' with metadata) because, for example, the
    //       native code could store a pointer to the callback in its own state
    //       and the immediate native call might therefore not take a callback
    //       parameter ... because to invoke the callback it just needs to look
    //       at state previously stored.
    jni_array_region<jbyte> args_(env, args, sizeDirect);
    result = invokeFunc(env, sizeDirect, args_.data(), funcPtr);
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

template<typename InvokeFunc>
inline jlong call_indirect(JNIEnv * env, jlong funcPtr, jint sizeDirect,
                           jbyteArray args, jintArray ptrArray,
                           InvokeFunc invokeFunc)
{
    jlong result(0);
    JNI_EXCEPTION_SAFE_BEGIN
    LOG_TRACE("funcPtr => "    << reinterpret_cast<void *>(funcPtr) << ", " <<
              "sizeDirect => " << sizeDirect);
    jni_array<jbyte> args_(env, args);
    jni_array_region<jint> ptr_array(env, ptrArray);
    marshalling_roundtrip::ptrs_init(args_.data(), ptr_array.data(), ptr_array.size());
    result = invokeFunc(env, sizeDirect, args_.data(), funcPtr);
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

template <typename InvokeFunc>
inline jlong call_vi(JNIEnv * env, jlong funcPtr, jint sizeDirect,
                     jbyteArray args, jintArray ptrArray, jobjectArray viArray,
                     jintArray viInstArray, InvokeFunc invokeFunc)
{
    jlong result(0);
    JNI_EXCEPTION_SAFE_BEGIN
    LOG_TRACE("funcPtr => "    << reinterpret_cast<void *>(funcPtr) << ", " <<
              "sizeDirect => " << sizeDirect);
    jni_array<jbyte> args_(env, args);
    jni_array_region<jint> ptr_array(env, ptrArray);
    marshalling_vi_container vi_array_cpp(env->GetArrayLength(viArray), env,
                                       viArray);
    marshalling_roundtrip::ptrs_init_vi(args_.data(), args_.size(),
                                        ptr_array.data(), ptr_array.size(),
                                        env, viArray, vi_array_cpp);
    result = invokeFunc(env, sizeDirect, args_.data(), funcPtr);
    jni_array_region<jint> vi_inst_array(env, viInstArray);
    marshalling_roundtrip::ptrs_finish_vi(env, viArray, vi_array_cpp,
                                          vi_inst_array);
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

inline const char * get_struct_ptr(jlong struct_addr, jint size_direct)
{
    assert(struct_addr || !"can't copy out a NULL pointer");
    assert(0 < size_direct || !"structure must have positive size");
    return reinterpret_cast<const char *>(struct_addr);
}

constexpr size_t CLEARED_LIST_DELETE_THRESHOLD = 10;
std::mutex callback_list_lock;
std::deque<stdcall_thunk *> callback_clearing_list;
std::deque<stdcall_thunk *> callback_cleared_list;

void clear_callback(stdcall_thunk * thunk)
{
    std::lock_guard<std::mutex> lock(callback_list_lock);
    switch (thunk->clear())
    {
        case stdcall_thunk_state::CLEARED:
            callback_cleared_list.push_back(thunk);
            break;
        case stdcall_thunk_state::CLEARING:
            callback_clearing_list.push_back(thunk);
            break;
        default:
            assert(!"invalid callback state");
            break;
    }
    // Don't let the cleared list grow indefinitely
    if (CLEARED_LIST_DELETE_THRESHOLD < callback_cleared_list.size())
    {
        assert(thunk != callback_cleared_list.front());
        delete callback_cleared_list.front();
        callback_cleared_list.pop_front();
    }
    // Don't let the clearing list grow indefinitely
    if (1 < callback_clearing_list.size() &&
        stdcall_thunk_state::CLEARED == callback_clearing_list.front()->state())
    {
        callback_cleared_list.push_back(callback_clearing_list.front());
        callback_clearing_list.pop_front();
    }
}

} // anonymous namespace

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
    return reinterpret_cast<jlong(__stdcall *)()>(funcPtr)();
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
    return reinterpret_cast<jlong(__stdcall *)(long)>(funcPtr)(arg0);
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
    return reinterpret_cast<jlong(__stdcall *)(long, long)>(funcPtr)(arg0, arg1);
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
    return reinterpret_cast<long(__stdcall *)(long, long, long)>(funcPtr)(arg0,
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
    LOG_TRACE("funcPtr => "    << reinterpret_cast<void *>(funcPtr) << ", " <<
              "sizeDirect => " << sizeDirect);
    jni_array<jbyte> args_(env, args);
    jni_array_region<jint> ptr_array(env, ptrArray);
    marshalling_vi_container vi_array_cpp(env->GetArrayLength(viArray), env, viArray);
    marshalling_roundtrip::ptrs_init_vi(args_.data(), args_.size(),
                                        ptr_array.data(), ptr_array.size(),
                                        env, viArray, vi_array_cpp);
    jbyte * returned_str = reinterpret_cast<jbyte *>(invoke_stdcall_basic(
        env, sizeDirect, args_.data(), funcPtr));
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
 * Signature: (Lsuneido/language/jsdi/type/Callback;Lsuneido/SuValue;II[II[J)V
 */
JNIEXPORT void JNICALL Java_suneido_language_jsdi_ThunkManager_newThunk(
    JNIEnv * env, jclass thunkManager, jobject callback, jobject boundValue,
    jint sizeDirect, jint sizeIndirect, jintArray ptrArray,
    jint variableIndirectCount, jlongArray outThunkAddrs)
{
    JNI_EXCEPTION_SAFE_BEGIN
    jni_array_region<jint> ptr_array(env, ptrArray);
    jni_array<jlong> out_thunk_addrs(env, outThunkAddrs);
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
    static_assert(sizeof(stdcall_thunk *) <= sizeof(jlong), "fatal data loss");
    static_assert(sizeof(void *) <= sizeof(jlong), "fatal data loss");
    out_thunk_addrs[env->GetStaticIntField(
        thunkManager,
        GLOBAL_REFS
            ->suneido_language_jsdi_ThunkManager__f_THUNK_OBJECT_ADDR_INDEX())] =
        reinterpret_cast<jlong>(thunk);
    out_thunk_addrs[env->GetStaticIntField(
        thunkManager,
        GLOBAL_REFS
            ->suneido_language_jsdi_ThunkManager__f_THUNK_FUNC_ADDR_INDEX())] =
        reinterpret_cast<jlong>(func_addr);
    JNI_EXCEPTION_SAFE_END(env);
}

/*
 * Class:     suneido_language_jsdi_ThunkManager
 * Method:    deleteThunk
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_suneido_language_jsdi_ThunkManager_deleteThunk
  (JNIEnv * env, jclass, jlong thunkObjectAddr)
{
    static_assert(sizeof(stdcall_thunk *) <= sizeof(jlong), "fatal data loss");
    auto thunk(reinterpret_cast<stdcall_thunk *>(thunkObjectAddr));
    clear_callback(thunk);
}

//==============================================================================
//             JAVA CLASS: suneido.language.jsdi.type.Structure
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
 * Signature: (J[BI)V
 */
JNIEXPORT void JNICALL Java_suneido_language_jsdi_type_Structure_copyOutDirect(
    JNIEnv * env, jclass, jlong structAddr, jbyteArray data, jint sizeDirect)
{
    JNI_EXCEPTION_SAFE_BEGIN
    LOG_TRACE("structAddr => "   << reinterpret_cast<void *>(structAddr) <<
              ", sizeDirect => " << sizeDirect);
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
 * Signature: (J[BI[I)V
 */
JNIEXPORT void JNICALL Java_suneido_language_jsdi_type_Structure_copyOutIndirect(
    JNIEnv * env, jclass, jlong structAddr, jbyteArray data, jint sizeDirect,
    jintArray ptrArray)
{
    JNI_EXCEPTION_SAFE_BEGIN
    LOG_TRACE("structAddr => "   << reinterpret_cast<void *>(structAddr) <<
              ", sizeDirect => " << sizeDirect);
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
 * Signature: (J[BI[I[Ljava/lang/Object;[I)V
 */
JNIEXPORT void JNICALL Java_suneido_language_jsdi_type_Structure_copyOutVariableIndirect(
    JNIEnv * env, jclass, jlong structAddr, jbyteArray data, jint sizeDirect,
    jintArray ptrArray, jobjectArray viArray, jintArray viInstArray)
{
    JNI_EXCEPTION_SAFE_BEGIN
    LOG_TRACE("structAddr => "   << reinterpret_cast<void *>(structAddr) <<
              ", sizeDirect => " << sizeDirect);
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

