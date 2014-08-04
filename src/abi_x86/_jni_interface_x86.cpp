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

#include <cassert>
#include <cstring>

using namespace jsdi;
using namespace jsdi::abi_x86;

//==============================================================================
//                                INTERNALS
//==============================================================================

/* TODO: do we need to be able to handle Win32 exceptions? If so, we'll want
 *       to wrap things in SEH code *at some level*. But do we want that
 *       overhead around every DLL call, regardless of whether it is expected
 *       to throw an exception? [See NOTES A-D in callback_x86.cpp]
 */

namespace {

inline jlong invoke_stdcall_basic(JNIEnv * env, int args_size_bytes,
                                  jlong * args_ptr, jlong func_ptr)
{
    jlong result = stdcall_invoke::basic(args_size_bytes, args_ptr,
                                         reinterpret_cast<void *>(func_ptr));
    JNI_EXCEPTION_CHECK(env); // In case callback triggered exception...
    return result;
}

inline jlong invoke_stdcall_return_double(JNIEnv * env, int args_size_bytes,
                                          jlong * args_ptr,
                                          jlong func_ptr)
{
    union {
        volatile double d;
        volatile jlong  l;
    };
    d = stdcall_invoke::return_double(args_size_bytes, args_ptr,
                                      reinterpret_cast<void *>(func_ptr));
    JNI_EXCEPTION_CHECK(env); // In case callback triggered exception...
    return l;
}

template<typename InvokeFunc>
inline jlong call_direct(JNIEnv * env, jlong funcPtr, jint sizeDirect,
                         jlongArray args, InvokeFunc invokeFunc)
{
    jlong result(0);
    JNI_EXCEPTION_SAFE_BEGIN
    LOG_TRACE("funcPtr => "    << reinterpret_cast<void *>(funcPtr) << ", " <<
              "sizeDirect => " << sizeDirect << ", args => " << args);
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
#pragma warning(push) // TODO: remove after http://goo.gl/SvVcbg fixed
#pragma warning(disable:4592)
    jni_array_region<jlong> args_(env, args, min_whole_words(sizeDirect));
#pragma warning(pop)
    result = invokeFunc(env, sizeDirect, args_.data(), funcPtr);
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

template<typename InvokeFunc>
inline jlong call_indirect(JNIEnv * env, jlong funcPtr, jint sizeDirect,
                           jlongArray args, jintArray ptrArray,
                           InvokeFunc invokeFunc)
{
    jlong result(0);
    JNI_EXCEPTION_SAFE_BEGIN
    LOG_TRACE("funcPtr => "    << reinterpret_cast<void *>(funcPtr) << ", " <<
              "sizeDirect => " << sizeDirect);
    jni_array<jlong> args_(env, args);
    jni_array_region<jint> ptr_array(env, ptrArray);
    marshalling_roundtrip::ptrs_init(args_.data(), ptr_array.data(),
                                     ptr_array.size());
    result = invokeFunc(env, sizeDirect, args_.data(), funcPtr);
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

template <typename InvokeFunc>
inline jlong call_vi(JNIEnv * env, jlong funcPtr, jint sizeDirect,
                     jlongArray args, jintArray ptrArray, jobjectArray viArray,
                     jintArray viInstArray, InvokeFunc invokeFunc)
{
    jlong result(0);
    JNI_EXCEPTION_SAFE_BEGIN
    LOG_TRACE("funcPtr => "    << reinterpret_cast<void *>(funcPtr) << ", " <<
              "sizeDirect => " << sizeDirect);
    jni_array<jlong> args_(env, args);
    jni_array_region<jint> ptr_array(env, ptrArray);
    marshalling_vi_container vi_array_cpp(env->GetArrayLength(viArray), env,
                                          viArray);
    marshalling_roundtrip::ptrs_init_vi(args_.data(), args_.size(),
                                        ptr_array.data(), ptr_array.size(),
                                        env, viArray, vi_array_cpp);
    result = invokeFunc(env, sizeDirect, args_.data(), funcPtr);
    jni_array_region<jint> vi_inst_array(env, viInstArray);
    marshalling_roundtrip::ptrs_finish_vi(viArray, vi_array_cpp, vi_inst_array);
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

// FIXME: The thunks are being allocated on a heap that has static storage
//        duration, so this can't well have it too or there's likely to be some
//        subtle static initialization trouble happening.
thunk_clearing_list clearing_list;

} // anonymous namespace

//==============================================================================
//              JAVA CLASS: suneido.jsdi.abi.x86.NativeCallX86
//==============================================================================

#include "gen/suneido_jsdi_abi_x86_NativeCallX86.h"

/*
 * Class:     suneido_jsdi_abi_x86_NativeCallX86
 * Method:    callDirectReturnInt64
 * Signature: (JI[J)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_x86_NativeCallX86_callDirectReturnInt64(
    JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jlongArray args)
{ return call_direct(env, funcPtr, sizeDirect, args, invoke_stdcall_basic); }

/*
 * Class:     suneido_jsdi_abi_x86_NativeCallX86
 * Method:    callIndirectReturnInt64
 * Signature: (JI[J[I)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_x86_NativeCallX86_callIndirectReturnInt64(
    JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jlongArray args,
    jintArray ptrArray)
{
    return call_indirect(env, funcPtr, sizeDirect, args, ptrArray,
                         invoke_stdcall_basic);
}

/*
 * Class:     suneido_jsdi_abi_x86_NativeCallX86
 * Method:    callVariableIndirectReturnInt64
 * Signature: (JI[J[I[Ljava/lang/Object;[I)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_x86_NativeCallX86_callVariableIndirectReturnInt64(
    JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jlongArray args,
    jintArray ptrArray, jobjectArray viArray, jintArray viInstArray)
{
    return call_vi(env, funcPtr, sizeDirect, args, ptrArray, viArray,
                   viInstArray, invoke_stdcall_basic);
}

/*
 * Class:     suneido_jsdi_abi_x86_NativeCallX86
 * Method:    callDirectReturnDouble
 * Signature: (JI[J)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_x86_NativeCallX86_callDirectReturnDouble(
    JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jlongArray args)
{
    return call_direct(env, funcPtr, sizeDirect, args,
                       invoke_stdcall_return_double);
}

/*
 * Class:     suneido_jsdi_abi_x86_NativeCallX86
 * Method:    callIndirectReturnDouble
 * Signature: (JI[J[I)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_x86_NativeCallX86_callIndirectReturnDouble(
    JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jlongArray args,
    jintArray ptrArray)
{
    return call_indirect(env, funcPtr, sizeDirect, args, ptrArray,
                         invoke_stdcall_return_double);
}

/*
 * Class:     suneido_jsdi_abi_x86_NativeCallX86
 * Method:    callVariableIndirectReturnDouble
 * Signature: (JI[J[I[Ljava/lang/Object;[I)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_x86_NativeCallX86_callVariableIndirectReturnDouble(
    JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jlongArray args,
    jintArray ptrArray, jobjectArray viArray, jintArray viInstArray)
{
    return call_vi(env, funcPtr, sizeDirect, args, ptrArray, viArray,
                   viInstArray, invoke_stdcall_return_double);
}

/*
 * Class:     suneido_jsdi_abi_x86_NativeCallX86
 * Method:    callVariableIndirectReturnVariableIndirect
 * Signature: (JI[J[I[Ljava/lang/Object;[I)V
 */
JNIEXPORT void JNICALL Java_suneido_jsdi_abi_x86_NativeCallX86_callVariableIndirectReturnVariableIndirect(
    JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jlongArray args,
    jintArray ptrArray, jobjectArray viArray, jintArray viInstArray)
{
    JNI_EXCEPTION_SAFE_BEGIN
    LOG_TRACE("funcPtr => "    << reinterpret_cast<void *>(funcPtr) << ", " <<
              "sizeDirect => " << sizeDirect);
    jni_array<jlong> args_(env, args);
    jni_array_region<jint> ptr_array(env, ptrArray);
    marshalling_vi_container vi_array_cpp(env->GetArrayLength(viArray), env, viArray);
    marshalling_roundtrip::ptrs_init_vi(args_.data(), args_.size(),
                                        ptr_array.data(), ptr_array.size(),
                                        env, viArray, vi_array_cpp);
    jbyte * returned_str = reinterpret_cast<jbyte *>(invoke_stdcall_basic(
        env, sizeDirect, args_.data(), funcPtr));
    // Store a pointer to the return value in the last element of the variable
    // indirect array, so that it will be propagated back to the Java side...
    vi_array_cpp.put_return_value(returned_str);
    jni_array_region<jint> vi_inst_array(env, viInstArray);
    marshalling_roundtrip::ptrs_finish_vi(viArray, vi_array_cpp, vi_inst_array);
    JNI_EXCEPTION_SAFE_END(env);
}

//==============================================================================
//             JAVA CLASS: suneido.jsdi.abi.x86.ThunkManagerX86
//==============================================================================

#include "gen/suneido_jsdi_abi_x86_ThunkManagerX86.h"

/*
 * Class:     suneido_jsdi_abi_x86_ThunkManagerX86
 * Method:    newThunkX86
 * Signature: (Lsuneido/jsdi/type/Callback;Lsuneido/SuValue;II[II[J)V
 */
JNIEXPORT void JNICALL Java_suneido_jsdi_abi_x86_ThunkManagerX86_newThunkX86(
    JNIEnv * env, jclass, jobject callback, jobject boundValue, jint sizeDirect,
    jint sizeTotal, jintArray ptrArray, jint variableIndirectCount,
    jlongArray outThunkAddrs)
{
    JNI_EXCEPTION_SAFE_BEGIN
    jni_array_region<jint> ptr_array(env, ptrArray);
    jni_array<jlong> out_thunk_addrs(env, outThunkAddrs);
    std::shared_ptr<jsdi::callback> callback_ptr;
    if (0 == ptr_array.size() && variableIndirectCount < 1)
    {
        callback_ptr.reset(
            new jsdi_callback_direct(env, callback, boundValue, sizeDirect,
                                     sizeTotal));
    }
    else if (variableIndirectCount < 1)
    {
        callback_ptr.reset(
            new jsdi_callback_indirect(env, callback, boundValue, sizeDirect,
                                       sizeTotal, ptr_array.begin(),
                                       ptr_array.size()));

    }
    else
    {
        callback_ptr.reset(
            new jsdi_callback_vi(env, callback, boundValue, sizeDirect,
                                 sizeTotal, ptr_array.begin(), ptr_array.size(),
                                 variableIndirectCount));
    }
    stdcall_thunk * thunk(new stdcall_thunk(callback_ptr));
    void * func_addr(thunk->func_addr());
    static_assert(sizeof(stdcall_thunk *) <= sizeof(jlong), "fatal data loss");
    static_assert(sizeof(void *) <= sizeof(jlong), "fatal data loss");
    out_thunk_addrs[suneido_jsdi_abi_x86_ThunkManagerX86_THUNK_OBJECT_ADDR_INDEX
    ] = reinterpret_cast<jlong>(thunk);
    out_thunk_addrs[suneido_jsdi_abi_x86_ThunkManagerX86_THUNK_FUNC_ADDR_INDEX
    ] = reinterpret_cast<jlong>(func_addr);
    JNI_EXCEPTION_SAFE_END(env);
}

/*
 * Class:     suneido_jsdi_abi_x86_ThunkManagerX86
 * Method:    deleteThunkX86
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_suneido_jsdi_abi_x86_ThunkManagerX86_deleteThunkX86
  (JNIEnv * env, jclass, jlong thunkObjectAddr)
{
    JNI_EXCEPTION_SAFE_BEGIN
    static_assert(sizeof(stdcall_thunk *) <= sizeof(jlong), "fatal data loss");
    auto thunk(reinterpret_cast<stdcall_thunk *>(thunkObjectAddr));
    clearing_list.clear_thunk(thunk);
    JNI_EXCEPTION_SAFE_END(env);
}
