/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: jsdi_callback.cpp
// auth: Victor Schappert
// date: 20130806
// desc: Implementation of callback which is able to call back into JNI
//==============================================================================

#include "jsdi_callback.h"

#include "log.h"
#include "marshalling.h"

#include <stdexcept>

namespace jsdi {

//==============================================================================
//                         class jsdi_callback_base
//==============================================================================

namespace {

static jint const EMPTY_PTR_ARRAY[1] = { 0 };

jobject globalize(JNIEnv * env, jobject value, char const * name)
{
    assert(env || !"environment cannot be NULL");
    assert(value || !"can't globalize a null reference");
    jobject const result(env->NewGlobalRef(value));
    if (result) return result;
    JNI_EXCEPTION_CHECK(env);
    std::ostringstream() << "NewGlobalRef(env => " << env << ", " << name
                         << " => " << value << ')'
                         << throw_cpp<jni_bad_alloc, const char *>(__FUNCTION__);
    return nullptr; // Squelch compiler warning (control never gets here)
}

} // anonymous namespace

jsdi_callback_base::jsdi_callback_base(JNIEnv * env, jobject suneido_callback,
                                       jobject suneido_sucallable,
                                       jint size_direct, jint size_total,
                                       jint const * ptr_array,
                                       jint ptr_array_size, jint vi_count)
    : callback(size_direct, size_total, ptr_array, ptr_array_size, vi_count)
    , d_suneido_callback_global_ref(
          globalize(env, suneido_callback, "callback"))
    , d_suneido_bound_value_global_ref(
          globalize(env, suneido_sucallable, "callable"))
    , d_jni_jvm(nullptr)
{
    assert(env || !"environment cannot be null");
    if (JNI_OK != env->GetJavaVM(&d_jni_jvm))
    {
        JNI_EXCEPTION_CHECK(env);
        std::ostringstream() << "failed to get JVM reference, env => " << env
                             << throw_cpp<jni_exception, JNIEnv *>(env);
    }
}

jsdi_callback_base::~jsdi_callback_base()
{
    JNIEnv * env(fetch_env());
    if (env)
    {
        env->DeleteGlobalRef(d_suneido_callback_global_ref);
        env->DeleteGlobalRef(d_suneido_bound_value_global_ref);
    }
}

JNIEnv * jsdi_callback_base::fetch_env() const
{
    JNIEnv * env(nullptr);
    JavaVMAttachArgs attach_args;
    attach_args.version = JNI_VERSION_1_6;
    attach_args.name    = nullptr;
    attach_args.group   = nullptr;
    if (JNI_OK == d_jni_jvm->AttachCurrentThread(reinterpret_cast<void **>(&env),
                                                 &attach_args))
        return env;
    else
    {
        LOG_FATAL("Failed to get JNI environment with d_jni_jvm => " <<
                  d_jni_jvm);
        return nullptr;
    }
}

//==============================================================================
//                        struct jsdi_callback_fast0
//==============================================================================

jsdi_callback_fast0::jsdi_callback_fast0(JNIEnv * env, jobject suneido_callback,
                                         jobject suneido_bound_value,
                                         jint size_direct)
    : jsdi_callback_base(env, suneido_callback, suneido_bound_value, 0, 0,
                         EMPTY_PTR_ARRAY, 0, 0)
{ assert(0 == size_direct); }

uint64_t jsdi_callback_fast0::call(marshall_word_t const * args)
{
    uint64_t result(0);
    LOG_TRACE("jsdi_callback_fast0::call( this => " << this << ", args => "
                                                    << args << " )");
    JNIEnv * const env(fetch_env());
    if (! env) return 0;
    JNI_EXCEPTION_SAFE_BEGIN
    JNI_EXCEPTION_CHECK(env); // TODO: This goes away with proper SEH support
    result = env->CallStaticLongMethod(
        GLOBAL_REFS->suneido_jsdi_type_Callback(),
        GLOBAL_REFS->suneido_jsdi_type_Callback__m_invoke0(),
        d_suneido_bound_value_global_ref);
    // TODO: Should check exception here and raise SEH if JNI exception pending
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

//==============================================================================
//                        struct jsdi_callback_fast1
//==============================================================================

jsdi_callback_fast1::jsdi_callback_fast1(JNIEnv * env, jobject suneido_callback,
                                         jobject suneido_bound_value,
                                         jint size_direct)
    : jsdi_callback_base(env, suneido_callback, suneido_bound_value,
                         size_direct,
                         1 * static_cast<jint>(sizeof(marshall_word_t)),
                         EMPTY_PTR_ARRAY, 0, 0)
{ }

uint64_t jsdi_callback_fast1::call(marshall_word_t const * args)
{
    uint64_t result(0);
    LOG_TRACE("jsdi_callback_fast1::call( this => " << this << ", args => "
                                                    << args << " )");
    JNIEnv * const env(fetch_env());
    if (! env) return 0;
    JNI_EXCEPTION_SAFE_BEGIN
    JNI_EXCEPTION_CHECK(env); // TODO: This goes away with proper SEH support
    jvalue out_args[2];
    out_args[0].l = d_suneido_bound_value_global_ref;
    out_args[1].j = args[0];
    result = env->CallNonvirtualLongMethodA(
        d_suneido_callback_global_ref,
        GLOBAL_REFS->suneido_jsdi_type_Callback(),
        GLOBAL_REFS->suneido_jsdi_type_Callback__m_invoke1(), out_args);
    // TODO: Should check exception here and raise SEH if JNI exception pending
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

//==============================================================================
//                        struct jsdi_callback_fast2
//==============================================================================

jsdi_callback_fast2::jsdi_callback_fast2(JNIEnv * env, jobject suneido_callback,
                                         jobject suneido_bound_value,
                                         jint size_direct)
    : jsdi_callback_base(env, suneido_callback, suneido_bound_value,
                         size_direct,
                         2 * static_cast<jint>(sizeof(marshall_word_t)),
                         EMPTY_PTR_ARRAY, 0, 0)
{ }

uint64_t jsdi_callback_fast2::call(marshall_word_t const * args)
{
    uint64_t result(0);
    LOG_TRACE("jsdi_callback_fast2::call( this => " << this << ", args => "
                                                    << args << " )");
    JNIEnv * const env(fetch_env());
    if (! env) return 0;
    JNI_EXCEPTION_SAFE_BEGIN
    JNI_EXCEPTION_CHECK(env); // TODO: This goes away with proper SEH support
    jvalue out_args[3];
    out_args[0].l = d_suneido_bound_value_global_ref;
    out_args[1].j = args[0];
    out_args[2].j = args[1];
    result = env->CallNonvirtualLongMethodA(
        d_suneido_callback_global_ref,
        GLOBAL_REFS->suneido_jsdi_type_Callback(),
        GLOBAL_REFS->suneido_jsdi_type_Callback__m_invoke2(), out_args);
    // TODO: Should check exception here and raise SEH if JNI exception pending
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

//==============================================================================
//                        struct jsdi_callback_fast3
//==============================================================================

jsdi_callback_fast3::jsdi_callback_fast3(JNIEnv * env, jobject suneido_callback,
                                         jobject suneido_bound_value,
                                         jint size_direct)
    : jsdi_callback_base(env, suneido_callback, suneido_bound_value,
                         size_direct,
                         3 * static_cast<jint>(sizeof(marshall_word_t)),
                         EMPTY_PTR_ARRAY, 0, 0)
{ }

uint64_t jsdi_callback_fast3::call(marshall_word_t const * args)
{
    uint64_t result(0);
    LOG_TRACE("jsdi_callback_fast3::call( this => " << this << ", args => "
                                                    << args << " )");
    JNIEnv * const env(fetch_env());
    if (! env) return 0;
    JNI_EXCEPTION_SAFE_BEGIN
    JNI_EXCEPTION_CHECK(env); // TODO: This goes away with proper SEH support
    jvalue out_args[4];
    out_args[0].l = d_suneido_bound_value_global_ref;
    out_args[1].j = args[0];
    out_args[2].j = args[1];
    out_args[3].j = args[2];
    result = env->CallNonvirtualLongMethodA(
        d_suneido_callback_global_ref,
        GLOBAL_REFS->suneido_jsdi_type_Callback(),
        GLOBAL_REFS->suneido_jsdi_type_Callback__m_invoke3(), out_args);
    // TODO: Should check exception here and raise SEH if JNI exception pending
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

//==============================================================================
//                        struct jsdi_callback_fast4
//==============================================================================

jsdi_callback_fast4::jsdi_callback_fast4(JNIEnv * env, jobject suneido_callback,
                                         jobject suneido_bound_value,
                                         jint size_direct)
    : jsdi_callback_base(env, suneido_callback, suneido_bound_value,
                         size_direct,
                         4 * static_cast<jint>(sizeof(marshall_word_t)),
                         EMPTY_PTR_ARRAY, 0, 0)
{ }

uint64_t jsdi_callback_fast4::call(marshall_word_t const * args)
{
    uint64_t result(0);
    LOG_TRACE("jsdi_callback_fast4::call( this => " << this << ", args => "
                                                    << args << " )");
    JNIEnv * const env(fetch_env());
    if (! env) return 0;
    JNI_EXCEPTION_SAFE_BEGIN
    JNI_EXCEPTION_CHECK(env); // TODO: This goes away with proper SEH support
    jvalue out_args[5];
    out_args[0].l = d_suneido_bound_value_global_ref;
    out_args[1].j = args[0];
    out_args[2].j = args[1];
    out_args[3].j = args[2];
    out_args[4].j = args[3];
    result = env->CallNonvirtualLongMethodA(
        d_suneido_callback_global_ref,
        GLOBAL_REFS->suneido_jsdi_type_Callback(),
        GLOBAL_REFS->suneido_jsdi_type_Callback__m_invoke4(), out_args);
    // TODO: Should check exception here and raise SEH if JNI exception pending
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

//==============================================================================
//                        struct jsdi_callback_direct
//==============================================================================

jsdi_callback_direct::jsdi_callback_direct(
    JNIEnv * env, jobject suneido_callback, jobject suneido_bound_value,
    jint size_direct, jint size_total)
    : jsdi_callback_base(env, suneido_callback, suneido_bound_value,
                         size_direct, size_total, EMPTY_PTR_ARRAY, 0, 0)
{ }

uint64_t jsdi_callback_direct::call(marshall_word_t const * args)
{
    uint64_t result(0);
    LOG_TRACE("jsdi_callback_direct::call( this => " << this << ", args => "
                                                     << args << " )");
    JNIEnv * const env(fetch_env());
    if (! env) return 0;
    JNI_EXCEPTION_SAFE_BEGIN
    JNI_EXCEPTION_CHECK(env); // TODO: This goes away with proper SEH support
    jni_auto_local<jobject> out_jarray(env, env->NewLongArray(
                                                d_size_total_words));
    JNI_EXCEPTION_CHECK(env);
    if (! out_jarray) throw jni_bad_alloc("NewLongArray", __FUNCTION__);
    jvalue out_args[2];
    out_args[0].l = d_suneido_bound_value_global_ref;
    out_args[1].l = out_jarray;
    {
        // Scope the critical array so it is destroyed before we do any other
        // JNI operations on this thread.
        jni_critical_array<jlong> out(
            env, static_cast<jlongArray>(static_cast<jobject>(out_jarray)),
            d_size_total_words);
        // Unmarshall
        std::memcpy(out.data(), args, d_size_direct);
    }
    result = env->CallNonvirtualLongMethodA(
        d_suneido_callback_global_ref,
        GLOBAL_REFS->suneido_jsdi_type_Callback(),
        GLOBAL_REFS->suneido_jsdi_type_Callback__m_invoke(), out_args);
    JNI_EXCEPTION_SAFE_END(env);
    // TODO: Should check exception here and raise SEH if JNI exception pending
    return result;
}

//==============================================================================
//                       struct jsdi_callback_indirect
//==============================================================================

jsdi_callback_indirect::jsdi_callback_indirect(
    JNIEnv * env, jobject suneido_callback, jobject suneido_bound_value,
    jint size_direct, jint size_total, jint const * ptr_array,
    jint ptr_array_size)
    : jsdi_callback_base(env, suneido_callback, suneido_bound_value,
                         size_direct, size_total, ptr_array, ptr_array_size, 0)
{ }

uint64_t jsdi_callback_indirect::call(marshall_word_t const * args)
{
    uint64_t result(0);
    LOG_TRACE("jsdi_callback_indirect::call( this => " << this << ", args => "
                                                       << args << " )");
    JNIEnv * const env(fetch_env());
    if (! env) return 0;
    JNI_EXCEPTION_SAFE_BEGIN
    JNI_EXCEPTION_CHECK(env); // TODO: This goes away with proper SEH support
    jni_auto_local<jobject> out_jarray(env, env->NewLongArray(
                                                d_size_total_words));
    JNI_EXCEPTION_CHECK(env);
    if (! out_jarray) throw jni_bad_alloc("NewLongArray", __FUNCTION__);
    jvalue out_args[2];
    out_args[0].l = d_suneido_bound_value_global_ref;
    out_args[1].l = out_jarray;
    {
        // Scope the critical array so it is destroyed before we do any other
        // JNI operations on this thread.
        jni_critical_array<jlong> out(
            env, static_cast<jlongArray>(static_cast<jobject>(out_jarray)),
            d_size_total_words);
        // Unmarshall
        unmarshaller_indirect u(d_size_direct, d_size_total_bytes,
                                d_ptr_array.data(),
                                d_ptr_array.data() + d_ptr_array.size());
        u.unmarshall_indirect(args,
                              reinterpret_cast<marshall_word_t *>(out.data()));
    }
    result = env->CallNonvirtualLongMethodA(
        d_suneido_callback_global_ref,
        GLOBAL_REFS->suneido_jsdi_type_Callback(),
        GLOBAL_REFS->suneido_jsdi_type_Callback__m_invoke(), out_args);
    JNI_EXCEPTION_SAFE_END(env);
    // TODO: Should check exception here and raise SEH if JNI exception pending
    return result;
}

//==============================================================================
//                           class jsdi_callback_vi
//==============================================================================

jsdi_callback_vi::jsdi_callback_vi(
    JNIEnv * env, jobject suneido_callback, jobject suneido_bound_value,
    jint size_direct, jint size_total, jint const * ptr_array,
    jint ptr_array_size, jint vi_count)
    : jsdi_callback_base(env, suneido_callback, suneido_bound_value,
                         size_direct, size_total, ptr_array, ptr_array_size,
                         vi_count)
    , d_vi_inst_array(
          vi_count,
          static_cast<jint>(
              java_enum::suneido_jsdi_marshall_VariableIndirectInstruction::RETURN_JAVA_STRING)
      )
{ }

uint64_t jsdi_callback_vi::call(marshall_word_t const * args)
{
    uint64_t result(0);
    LOG_TRACE("jsdi_callback_vi::call( this => " << this << ", args => "
                                                << args << " )");
    JNIEnv * const env(fetch_env());
    if (!env) return 0;
    JNI_EXCEPTION_SAFE_BEGIN
    JNI_EXCEPTION_CHECK(env); // TODO: This goes away with proper SEH support
    jni_auto_local<jobject> out_data_jarray(env, env->NewLongArray(
                                                     d_size_total_words));
    JNI_EXCEPTION_CHECK(env);
    if (! out_data_jarray) throw jni_bad_alloc("NewLongArray", __FUNCTION__);
    jni_auto_local<jobject> out_vi_jarray(
        env,
        env->NewObjectArray(d_vi_count, GLOBAL_REFS->java_lang_Object(),
                            nullptr));
    JNI_EXCEPTION_CHECK(env);
    if (! out_vi_jarray) throw jni_bad_alloc("NewObjectArray", __FUNCTION__);
    jvalue out_args[3];
    out_args[0].l = d_suneido_bound_value_global_ref;
    out_args[1].l = out_data_jarray;
    out_args[2].l = out_vi_jarray;
    {
        // The reason this code is in a sub-block is because we want the
        // destructor of the jni_array<jbyte> to run before we invoke the
        // callback. This is because the destructor releases the byte array
        // elements, which is necessary to ensure that the unmarshalled data is
        // propagated into the Java side before the callback runs.
        jni_array<jlong> out(
            env, static_cast<jlongArray>(static_cast<jobject>(out_data_jarray)),
            d_size_total_words);
        unmarshaller_vi u(d_size_direct, d_size_total_bytes, d_ptr_array.data(),
                          d_ptr_array.data() + d_ptr_array.size(), d_vi_count);
        u.unmarshall_vi(
            args,  reinterpret_cast<marshall_word_t *>(out.data()), env,
            static_cast<jobjectArray>(static_cast<jobject>(out_vi_jarray)),
            d_vi_inst_array.data());
    }
    result = env->CallNonvirtualLongMethodA(
        d_suneido_callback_global_ref,
        GLOBAL_REFS->suneido_jsdi_type_Callback(),
        GLOBAL_REFS->suneido_jsdi_type_Callback__m_invokeVariableIndirect(),
        out_args
    );
    JNI_EXCEPTION_SAFE_END(env);
    // TODO: Should check exception here and raise SEH if JNI exception pending
    return result;
}

} // namespace jni
