/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
* Licensed under GPLv2.
*/

//==============================================================================
// file: _jni_interface_64.cpp
// auth: Victor Schappert
// date: 20140731
// desc: JVM's interface for functionality specific to the amd64 ABI
//==============================================================================

#include "global_refs.h"
#include "jni_exception.h"
#include "jsdi_callback.h"
#include "log.h"
#include "marshalling.h"
#include "seh.h"

#include "invoke64.h"
#include "thunk64.h"

#include <algorithm>
#include <ostream>
#include <cassert>

using namespace jsdi;
using namespace jsdi::abi_amd64;

//==============================================================================
//                                INTERNALS
//==============================================================================

namespace {

#if LOG_LEVEL_TRACE <= STATIC_LOG_THRESHOLD
template<typename ... ArgTypes>
std::initializer_list<jlong> to_list(ArgTypes ... args)
{ return { args... }; }

std::ostream& operator<<(std::ostream& o,
                         const std::initializer_list<jlong>& args)
{
    o << "[ ";
    std::copy(args.begin(), args.end(), std::ostream_iterator<jlong>(o, " "));
    return o << ']';
}
#endif // STATIC_LOG_THRESHOLD < LOG_LEVEL_TRACE

template<typename ... ArgTypes>
jlong call_fast(JNIEnv * env, jlong funcPtr, ArgTypes ... args)
{
    typedef jlong (* func_t)(ArgTypes ...);
    auto f = reinterpret_cast<func_t>(funcPtr);
    jlong r(0);
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    LOG_TRACE("funcPtr => " << f << ", args => " << to_list(args...));
    r = seh::convert_to_cpp(f, args...);
    JNI_EXCEPTION_SAFE_CPP_END(env);
    return r;
}

template<typename T>
jlong coerce_to_jlong(T value)
{ return *reinterpret_cast<jlong const *>(&value); }

template<>
jlong coerce_to_jlong<float>(float value)
{ return coerce_to_jlong(static_cast<double>(value)); }

jlong call_direct_nofp(JNIEnv * env, jlong funcPtr, jint sizeDirect,
                       jlongArray args)
{
    uint64_t result(0);
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    LOG_TRACE("funcPtr => "    << reinterpret_cast<void *>(funcPtr) << ", " <<
              "sizeDirect => " << sizeDirect << ", args => " << args);
#pragma warning(push) // TODO: remove after http://goo.gl/SvVcbg fixed
#pragma warning(disable:4592)
    jni_array_region<jlong> args_(env, args, min_whole_words(sizeDirect));
#pragma warning(pop)
    result = invoke64::basic(sizeDirect, args_.data(),
                             reinterpret_cast<void *>(funcPtr));
    JNI_EXCEPTION_SAFE_CPP_END(env);
    return static_cast<jlong>(result);
}

template<
    typename ReturnType,
    ReturnType (*InvokeFunc)(size_t, const void *, void *, param_register_types)
>
jlong call_direct_fp(JNIEnv * env, jlong funcPtr, jint sizeDirect,
                     jlongArray args, jint registers)
{
    jlong result(0);
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    LOG_TRACE("funcPtr => "    << reinterpret_cast<void *>(funcPtr) << ", " <<
              "sizeDirect => " << sizeDirect << ", registers " << registers
                               << ", args => " << args);
#pragma warning(push) // TODO: remove after http://goo.gl/SvVcbg fixed
#pragma warning(disable:4592)
    jni_array_region<jlong> args_(env, args, min_whole_words(sizeDirect));
#pragma warning(pop)
    param_register_types const registers_(static_cast<uint32_t>(registers));
    ReturnType return_value = InvokeFunc(sizeDirect, args_.data(),
                                         reinterpret_cast<void *>(funcPtr),
                                         registers);
    result = coerce_to_jlong(return_value);
    JNI_EXCEPTION_SAFE_CPP_END(env);
    return result;
}

jlong call_indirect_nofp(JNIEnv * env, jlong funcPtr, jint sizeDirect,
                         jlongArray args, jintArray ptrArray)
{
    uint64_t result(0);
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    LOG_TRACE("funcPtr => "    << reinterpret_cast<void *>(funcPtr) << ", " <<
              "sizeDirect => " << sizeDirect << ", args => " << args);
    jni_array<jlong> args_(env, args);
    jni_array_region<jint> ptr_array(env, ptrArray);
    marshalling_roundtrip::ptrs_init(args_.data(), ptr_array.data(),
                                     ptr_array.size());
    result = invoke64::basic(sizeDirect, args_.data(),
                             reinterpret_cast<void *>(funcPtr));
    JNI_EXCEPTION_SAFE_CPP_END(env);
    return static_cast<jlong>(result);
}

template<
    typename ReturnType,
    ReturnType (*InvokeFunc)(size_t, const void *, void *, param_register_types)
>
jlong call_indirect_fp(JNIEnv * env, jlong funcPtr, jint sizeDirect,
                       jlongArray args, jint registers, jintArray ptrArray)
{
    jlong result(0);
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    LOG_TRACE("funcPtr => "    << reinterpret_cast<void *>(funcPtr) << ", " <<
              "sizeDirect => " << sizeDirect << ", registers " << registers
                               << ", args => " << args);
    jni_array<jlong> args_(env, args);
    jni_array_region<jint> ptr_array(env, ptrArray);
    param_register_types const registers_(static_cast<uint32_t>(registers));
    marshalling_roundtrip::ptrs_init(args_.data(), ptr_array.data(),
                                     ptr_array.size());
    ReturnType return_value = InvokeFunc(sizeDirect, args_.data(),
                                         reinterpret_cast<void *>(funcPtr),
                                         registers);
    result = coerce_to_jlong(return_value);
    JNI_EXCEPTION_SAFE_CPP_END(env);
    return result;
}

template<typename Value, typename CoercedValue>
void call_vi_coerce(Value const& x, CoercedValue& y, marshalling_vi_container&)
{ *reinterpret_cast<Value *>(&y) = x; }

template<>
void call_vi_coerce(const uint64_t& x, jbyte *& y, marshalling_vi_container& z)
{
    jbyte * str = reinterpret_cast<jbyte *>(x);
    y = str;
    z.put_return_value(str);
}

template<
    typename ReturnType,
    ReturnType (*InvokeFunc)(size_t, const void *, void *, param_register_types),
    typename CoerceReturnType = jlong
>
CoerceReturnType
    call_vi_fp(JNIEnv * env, jlong funcPtr, jint sizeDirect, jlongArray args,
               jint registers, jintArray ptrArray, jobjectArray viArray,
               jintArray viInstArray)
{
    CoerceReturnType result(0);
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    LOG_TRACE("funcPtr => "    << reinterpret_cast<void *>(funcPtr) << ", " <<
              "sizeDirect => " << sizeDirect << ", registers " << registers
                               << ", args => " << args);
    jni_array<jlong> args_(env, args);
    jni_array_region<jint> ptr_array(env, ptrArray);
    marshalling_vi_container vi_array_cpp(env->GetArrayLength(viArray), env,
                                          viArray);
    marshalling_roundtrip::ptrs_init_vi(args_.data(), args_.size(),
                                        ptr_array.data(), ptr_array.size(),
                                        env, viArray, vi_array_cpp);
    param_register_types const registers_(static_cast<uint32_t>(registers));
    ReturnType return_value = InvokeFunc(sizeDirect, args_.data(),
                                         reinterpret_cast<void *>(funcPtr),
                                         registers);
    JNI_EXCEPTION_CHECK(env);
    call_vi_coerce<ReturnType, CoerceReturnType>(return_value, result,
                                                 vi_array_cpp);
    jni_array_region<jint> vi_inst_array(env, viInstArray);
    marshalling_roundtrip::ptrs_finish_vi(viArray, vi_array_cpp, vi_inst_array);
    JNI_EXCEPTION_SAFE_CPP_END(env);
    return result;
}


// FIXME: The thunks are being allocated on a heap that has static storage
//        duration, so this can't well have it too or there's likely to be some
//        subtle static initialization trouble happening.
thunk_clearing_list clearing_list;

} // anonymous namespaces

//==============================================================================
//              JAVA CLASS: suneido.jsdi.abi.amd64.NativeCall64
//==============================================================================

#include "gen/suneido_jsdi_abi_amd64_NativeCall64.h"

/*
 * Class:     suneido_jsdi_abi_amd64_NativeCall64
 * Method:    callJ0
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_amd64_NativeCall64_callJ0
  (JNIEnv * env, jclass, jlong funcPtr)
{ return call_fast(env, funcPtr); }

/*
 * Class:     suneido_jsdi_abi_amd64_NativeCall64
 * Method:    callJ1
 * Signature: (JJ)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_amd64_NativeCall64_callJ1
  (JNIEnv * env, jclass, jlong funcPtr, jlong a)
{ return call_fast(env, funcPtr, a); }

/*
 * Class:     suneido_jsdi_abi_amd64_NativeCall64
 * Method:    callJ2
 * Signature: (JJJ)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_amd64_NativeCall64_callJ2
  (JNIEnv * env, jclass, jlong funcPtr, jlong a, jlong b)
{ return call_fast(env, funcPtr, a, b); }

/*
 * Class:     suneido_jsdi_abi_amd64_NativeCall64
 * Method:    callJ3
 * Signature: (JJJJ)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_amd64_NativeCall64_callJ3
  (JNIEnv * env, jclass, jlong funcPtr, jlong a, jlong b, jlong c)
{ return call_fast(env, funcPtr, a, b, c); }

/*
 * Class:     suneido_jsdi_abi_amd64_NativeCall64
 * Method:    callJ4
 * Signature: (JJJJJ)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_amd64_NativeCall64_callJ4
  (JNIEnv * env, jclass, jlong funcPtr, jlong a, jlong b, jlong c, jlong d)
{ return call_fast(env, funcPtr, a, b, c, d); }

/*
 * Class:     suneido_jsdi_abi_amd64_NativeCall64
 * Method:    callDirectNoFpReturnInt64
 * Signature: (JI[J)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_amd64_NativeCall64_callDirectNoFpReturnInt64
  (JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jlongArray args)
{ return call_direct_nofp(env, funcPtr, sizeDirect, args); }

/*
 * Class:     suneido_jsdi_abi_amd64_NativeCall64
 * Method:    callDirectReturnInt64
 * Signature: (JI[JI)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_amd64_NativeCall64_callDirectReturnInt64(
    JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jlongArray args, jint registers)
{
    return call_direct_fp<uint64_t, invoke64::fp>(env, funcPtr, sizeDirect,
                                                  args, registers);
}

/*
 * Class:     suneido_jsdi_abi_amd64_NativeCall64
 * Method:    callDirectReturnFloat
 * Signature: (JI[JI)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_amd64_NativeCall64_callDirectReturnFloat
  (JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jlongArray args, jint registers)
{
    return call_direct_fp<float, invoke64::return_float>(env, funcPtr,
                                                         sizeDirect, args,
                                                         registers);
}
  
/*
 * Class:     suneido_jsdi_abi_amd64_NativeCall64
 * Method:    callDirectReturnDouble
 * Signature: (JI[JI)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_amd64_NativeCall64_callDirectReturnDouble
  (JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jlongArray args, jint registers)
{
    return call_direct_fp<double, invoke64::return_double>(
        env, funcPtr, sizeDirect, args, registers);
}

/*
 * Class:     suneido_jsdi_abi_amd64_NativeCall64
 * Method:    callIndirectNoFpReturnInt64
 * Signature: (JI[J[I)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_amd64_NativeCall64_callIndirectNoFpReturnInt64
  (JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jlongArray args, jintArray ptrArray)
{ return call_indirect_nofp(env, funcPtr, sizeDirect, args, ptrArray); }

/*
 * Class:     suneido_jsdi_abi_amd64_NativeCall64
 * Method:    callIndirectReturnInt64
 * Signature: (JI[JI[I)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_amd64_NativeCall64_callIndirectReturnInt64
  (JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jlongArray args, jint registers, jintArray ptrArray)
{
    return call_indirect_fp<uint64_t, invoke64::fp>(env, funcPtr, sizeDirect,
                                                    args, registers, ptrArray);
}

/*
 * Class:     suneido_jsdi_abi_amd64_NativeCall64
 * Method:    callIndirectReturnFloat
 * Signature: (JI[JI[I)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_amd64_NativeCall64_callIndirectReturnFloat
  (JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jlongArray args, jint registers, jintArray ptrArray)
{
    return call_indirect_fp<float, invoke64::return_float>(
        env, funcPtr, sizeDirect, args, registers, ptrArray);
}

/*
 * Class:     suneido_jsdi_abi_amd64_NativeCall64
 * Method:    callIndirectReturnDouble
 * Signature: (JI[JI[I)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_amd64_NativeCall64_callIndirectReturnDouble
  (JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jlongArray args, jint registers, jintArray ptrArray)
{
    return call_indirect_fp<double, invoke64::return_double>(
        env, funcPtr, sizeDirect, args, registers, ptrArray);
}

/*
 * Class:     suneido_jsdi_abi_amd64_NativeCall64
 * Method:    callVariableIndirectReturnInt64
 * Signature: (JI[JI[I[Ljava/lang/Object;[I)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_amd64_NativeCall64_callVariableIndirectReturnInt64
  (JNIEnv *  env, jclass, jlong funcPtr, jint sizeDirect, jlongArray args, jint registers, jintArray ptrArray, jobjectArray viArray, jintArray viInstArray)
{
    return call_vi_fp<uint64_t, invoke64::fp>(
        env, funcPtr, sizeDirect, args, registers, ptrArray,
        viArray, viInstArray);
}

/*
 * Class:     suneido_jsdi_abi_amd64_NativeCall64
 * Method:    callVariableIndirectReturnFloat
 * Signature: (JI[JI[I[Ljava/lang/Object;[I)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_amd64_NativeCall64_callVariableIndirectReturnFloat
  (JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jlongArray args, jint registers, jintArray ptrArray, jobjectArray viArray, jintArray viInstArray)
 {
    return call_vi_fp<float, invoke64::return_float>(
        env, funcPtr, sizeDirect, args, registers, ptrArray,
        viArray, viInstArray);
 }

/*
 * Class:     suneido_jsdi_abi_amd64_NativeCall64
 * Method:    callVariableIndirectReturnDouble
 * Signature: (JI[JI[I[Ljava/lang/Object;[I)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_abi_amd64_NativeCall64_callVariableIndirectReturnDouble
  (JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jlongArray args, jint registers, jintArray ptrArray, jobjectArray viArray, jintArray viInstArray)
{
    return call_vi_fp<double, invoke64::return_double>(
        env, funcPtr, sizeDirect, args, registers, ptrArray,
        viArray, viInstArray);
}

/*
 * Class:     suneido_jsdi_abi_amd64_NativeCall64
 * Method:    callVariableIndirectReturnVariableIndirect
 * Signature: (JI[JI[I[Ljava/lang/Object;[I)V
 */
JNIEXPORT void JNICALL Java_suneido_jsdi_abi_amd64_NativeCall64_callVariableIndirectReturnVariableIndirect
  (JNIEnv * env, jclass, jlong funcPtr, jint sizeDirect, jlongArray args, jint registers, jintArray ptrArray, jobjectArray viArray, jintArray viInstArray)
{
    call_vi_fp<uint64_t, invoke64::fp, jbyte *>(
        env, funcPtr, sizeDirect, args, registers, ptrArray,
        viArray, viInstArray);
}

//==============================================================================
//             JAVA CLASS: suneido.jsdi.abi.amd64.ThunkManager64
//==============================================================================

#include "gen/suneido_jsdi_abi_amd64_ThunkManager64.h"

/*
 * Class:     suneido_jsdi_abi_amd64_ThunkManager64
 * Method:    newThunk64
 * Signature: (Lsuneido/jsdi/type/Callback;Lsuneido/SuValue;III[II[J)V
 */
JNIEXPORT void JNICALL Java_suneido_jsdi_abi_amd64_ThunkManager64_newThunk64(
  JNIEnv * env, jclass, jobject callback, jobject boundValue, jint sizeDirect,
  jint sizeTotal, jintArray ptrArray, jint variableIndirectCount,
  jint registerUsage, jint numParams, jboolean makeFastCall,
  jlongArray outThunkAddrs)
{
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    LOG_DEBUG("newThunk64( sizeDirect => " << sizeDirect << ", sizeTotal => "
              << sizeTotal << ", viCount => " << variableIndirectCount
              << ", registerUsage => " << registerUsage << ", numParams => "
              << numParams << ", makeFastCall => " << makeFastCall << " )");
    std::shared_ptr<jsdi::callback> callback_ptr;
    if (makeFastCall)
    {
        switch (numParams)
        {
            case 0:
                callback_ptr.reset(new jsdi_callback_fast0(env, callback,
                                   boundValue, sizeDirect));
                break;
            case 1:
                callback_ptr.reset(new jsdi_callback_fast1(env, callback,
                                   boundValue, sizeDirect));
                break;
            case 2:
                callback_ptr.reset(new jsdi_callback_fast2(env, callback,
                                   boundValue, sizeDirect));
                break;
            case 3:
                callback_ptr.reset(new jsdi_callback_fast3(env, callback,
                                   boundValue, sizeDirect));
                break;
            case 4:
                callback_ptr.reset(new jsdi_callback_fast4(env, callback,
                                   boundValue, sizeDirect));
                break;
            default:
                LOG_WARN("can't make a fastcall for " << numParams
                                                      << " parameters");
        }
    }
    if (! callback_ptr)
    {
        jni_array_region<jint> ptr_array(env, ptrArray);
        if (0 == ptr_array.size() && variableIndirectCount < 1)
        {
            callback_ptr.reset(
                new jsdi_callback_direct(env, callback, boundValue, sizeDirect,
                                         sizeTotal));
        }
        else if (variableIndirectCount < 1)
        {
            callback_ptr.reset(
                new jsdi_callback_indirect(env, callback, boundValue,
                                           sizeDirect, sizeTotal,
                                           ptr_array.begin(),
                                           ptr_array.size()));
        }
        else
        {
            callback_ptr.reset(
                new jsdi_callback_vi(env, callback, boundValue, sizeDirect,
                                     sizeTotal, ptr_array.begin(),
                                     ptr_array.size(), variableIndirectCount));
        }
    }
    assert(callback_ptr);
    param_register_types const registers(static_cast<uint32_t>(registerUsage));
    size_t const num_register_params = std::min(static_cast<size_t>(numParams),
                                                NUM_PARAM_REGISTERS);
    jni_array<jlong> out_thunk_addrs(env, outThunkAddrs);
    thunk64 * thunk(new thunk64(callback_ptr, num_register_params, registers));
    void * func_addr(thunk->func_addr());
    out_thunk_addrs[suneido_jsdi_abi_amd64_ThunkManager64_THUNK_OBJECT_ADDR_INDEX
    ] = reinterpret_cast<jlong>(thunk);
    out_thunk_addrs[suneido_jsdi_abi_amd64_ThunkManager64_THUNK_FUNC_ADDR_INDEX
    ] = reinterpret_cast<jlong>(func_addr);
    JNI_EXCEPTION_SAFE_CPP_END(env);
}

/*
 * Class:     suneido_jsdi_abi_amd64_ThunkManager64
 * Method:    deleteThunk64
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_suneido_jsdi_abi_amd64_ThunkManager64_deleteThunk64
  (JNIEnv * env, jclass, jlong thunkObjectAddr)
{
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    static_assert(sizeof(thunk64 *) <= sizeof(jlong), "fatal data loss");
    auto thunk(reinterpret_cast<thunk64 *>(thunkObjectAddr));
    std::function<void()> clear_func(
        std::bind(
            std::mem_fn(&thunk_clearing_list::clear_thunk),
            clearing_list, thunk));
    seh::convert_to_cpp(clear_func);
    JNI_EXCEPTION_SAFE_CPP_END(env);
}
