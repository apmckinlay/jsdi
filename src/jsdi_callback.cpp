//==============================================================================
// file: jsdi_callback.cpp
// auth: Victor Schappert
// date: 20130806
// desc: Implementation of callback which is able to call back into JNI
//==============================================================================

#include "jsdi_callback.h"

#include <stdexcept>

namespace jsdi {

//==============================================================================
//                      class jsdi_callback_args_basic
//==============================================================================

jsdi_callback_args_basic::~jsdi_callback_args_basic()
{
    if (data()) release_data();
    d_env->DeleteLocalRef(d_array);
}

void jsdi_callback_args_basic::vi_string_ptr(const char * str, int vi_index)
{
    throw std::logic_error(
        "jsdi_callback_args_basic does not support vi_string_ptr");
}

//==============================================================================
//                         class jsdi_callback_basic
//==============================================================================

void jsdi_callback_basic::init(JNIEnv * env, jobject suneido_callback,
                               jobject suneido_sucallable)
{
    assert(suneido_callback);
    assert(suneido_sucallable);
    d_suneido_callback_global_ref = env->NewGlobalRef(suneido_callback);
    d_suneido_sucallable_global_ref = env->NewGlobalRef(suneido_sucallable);
    assert(d_suneido_callback_global_ref);
    assert(d_suneido_sucallable_global_ref);
    if (0 != env->GetJavaVM(&d_jni_jvm))
    {
        assert(false || !"failed to get JVM reference");
    }
}

jsdi_callback_basic::jsdi_callback_basic(JNIEnv * env,
                                         jobject suneido_callback,
                                         jobject suneido_sucallable,
                                         int size_direct, int size_indirect,
                                         const int * ptr_array,
                                         int ptr_array_size, int vi_count)
    : callback(size_direct, size_indirect, ptr_array, ptr_array_size, vi_count)
{ init(env, suneido_callback, suneido_sucallable); }

jsdi_callback_basic::jsdi_callback_basic(JNIEnv * env, jobject suneido_callback,
                                         jobject suneido_sucallable,
                                         int size_direct, int size_indirect,
                                         const int * ptr_array,
                                         int ptr_array_size)
    : callback(size_direct, size_indirect, ptr_array, ptr_array_size, 0)
{ init(env, suneido_callback, suneido_sucallable); }

jsdi_callback_basic::~jsdi_callback_basic()
{
    JNIEnv * env(0);
    d_jni_jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    if (env)
    {
        env->DeleteGlobalRef(d_suneido_callback_global_ref);
        env->DeleteGlobalRef(d_suneido_sucallable_global_ref);
    }
}

callback_args * jsdi_callback_basic::alloc_args() const
{ return new jsdi_callback_args_basic(fetch_env(), d_size_total); }

long jsdi_callback_basic::call(std::unique_ptr<callback_args> args)
{
    auto& in_args(static_cast<jsdi_callback_args_basic&>(*args));
    JNIEnv * env(in_args.env());
    jvalue out_args[2];
    out_args[0].l = d_suneido_sucallable_global_ref;
    out_args[1].l = in_args.release_data();
    long result(0);
    result = env->CallNonvirtualIntMethodA(
        d_suneido_callback_global_ref,
        GLOBAL_REFS->suneido_language_jsdi_type_Callback(),
        GLOBAL_REFS->suneido_language_jsdi_type_Callback__m_invoke(),
        out_args
    );
    // TODO: check for JNI exception here??
    return result;
}

JNIEnv * jsdi_callback_basic::fetch_env() const
{
    JNIEnv * env(0);
    d_jni_jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    assert(
        env || !"unable to fetch this thread's JNI environment (not attached?");
    return env;
}

//==============================================================================
//                        class jsdi_callback_args_vi
//==============================================================================

jsdi_callback_args_vi::~jsdi_callback_args_vi()
{
    if (data()) release_data();
    d_env->DeleteLocalRef(d_array);
    d_env->DeleteLocalRef(d_vi_array);
}

void jsdi_callback_args_vi::vi_string_ptr(const char * str, int vi_index)
{
    assert(str || !"str cannot be NULL");
    std::vector<jchar> chars(widen(str));
    jni_auto_local<jstring> jstr(d_env, chars.data(), chars.size());
    d_env->SetObjectArrayElement(d_vi_array, vi_index,
                                 static_cast<jobject>(jstr));
}

//==============================================================================
//                            class jsdi_callback_vi
//==============================================================================

callback_args * jsdi_callback_vi::alloc_args() const
{ return new jsdi_callback_args_vi(fetch_env(), d_size_total, d_vi_count); }

long jsdi_callback_vi::call(std::unique_ptr<callback_args> args)
{
    auto& in_args(static_cast<jsdi_callback_args_vi&>(*args));
    JNIEnv * env(in_args.env());
    jvalue out_args[3];
    out_args[0].l = d_suneido_sucallable_global_ref;
    out_args[1].l = in_args.release_data();
    out_args[2].l = in_args.vi_array();
    long result(0);
    result = env->CallNonvirtualIntMethodA(
        d_suneido_callback_global_ref,
        GLOBAL_REFS->suneido_language_jsdi_type_Callback(),
        GLOBAL_REFS->suneido_language_jsdi_type_Callback__m_invokeVariableIndirect(),
        out_args
    );
    // TODO: check for JNI exception here??
    return result;
}

} // namespace jni
