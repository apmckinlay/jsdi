//==============================================================================
// file: jsdi_callback.cpp
// auth: Victor Schappert
// date: 20130806
// desc: Implementation of callback which is able to call back into JNI
//==============================================================================

#include "jsdi_callback.h"

#include "marshalling.h"

#include <stdexcept>

namespace jsdi {

//==============================================================================
//                         class jsdi_callback_basic
//==============================================================================

void jsdi_callback_basic::init(JNIEnv * env, jobject suneido_callback,
                               jobject suneido_sucallable)
{
    assert(suneido_callback);
    assert(suneido_sucallable);
    d_suneido_callback_global_ref = env->NewGlobalRef(suneido_callback);
    d_suneido_bound_value_global_ref = env->NewGlobalRef(suneido_sucallable);
    assert(d_suneido_callback_global_ref);
    assert(d_suneido_bound_value_global_ref);
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
                                         jobject suneido_bound_value,
                                         int size_direct, int size_indirect,
                                         const int * ptr_array,
                                         int ptr_array_size)
    : callback(size_direct, size_indirect, ptr_array, ptr_array_size, 0)
{ init(env, suneido_callback, suneido_bound_value); }

jsdi_callback_basic::~jsdi_callback_basic()
{
    JNIEnv * env(0);
    d_jni_jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    if (env)
    {
        env->DeleteGlobalRef(d_suneido_callback_global_ref);
        env->DeleteGlobalRef(d_suneido_bound_value_global_ref);
    }
}

long jsdi_callback_basic::call(const char * args)
{
    long result(0);
    JNIEnv * const env(fetch_env());
    JNI_EXCEPTION_SAFE_BEGIN
    //
    // SET UP
    //
    jni_auto_local<jobject> out_jarray(env, env->NewByteArray(d_size_total));
    JNI_EXCEPTION_CHECK(env);
    if (! out_jarray) throw jni_bad_alloc("NewByteArray", __FUNCTION__);
    jvalue out_args[2];
    out_args[0].l = d_suneido_bound_value_global_ref;
    out_args[1].l = out_jarray;
    //
    // UNMARSHALL
    //
    {
        // The reason this code is in a sub-block is because we want the
        // destructor of the jni_critical_array<jbyte> to run before we invoke
        // the callback. This is because the destructor releases the critical
        // array and the changes to the array aren't guaranteed to propagate
        // back to the Java side until that moment.
        jni_critical_array<jbyte> out(
            env, static_cast<jbyteArray>(static_cast<jobject>(out_jarray)),
            d_size_total);
        unmarshaller_indirect u(d_size_direct, d_size_total, d_ptr_array.data(),
                                d_ptr_array.data() + d_ptr_array.size());
        u.unmarshall_indirect(reinterpret_cast<char *>(out.data()), args);
    }
    //
    // CALL
    //
    result = env->CallNonvirtualIntMethodA(
        d_suneido_callback_global_ref,
        GLOBAL_REFS->suneido_language_jsdi_type_Callback(),
        GLOBAL_REFS->suneido_language_jsdi_type_Callback__m_invoke(),
        out_args
    );
    JNI_EXCEPTION_CHECK(env);
    JNI_EXCEPTION_SAFE_END(env);
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
//                            class jsdi_callback_vi
//==============================================================================

long jsdi_callback_vi::call(const char * args)
{
    long result(0);
    JNIEnv * const env(fetch_env());
    JNI_EXCEPTION_SAFE_BEGIN
    //
    // SET UP
    //
    jni_auto_local<jobject> out_data_jarray(
        env, env->NewByteArray(d_size_total));
    JNI_EXCEPTION_CHECK(env);
    if (! out_data_jarray) throw jni_bad_alloc("NewByteArray", __FUNCTION__);
    jni_auto_local<jobject> out_vi_jarray(
        env,
        env->NewObjectArray(d_vi_count, GLOBAL_REFS->java_lang_Object(), 0));
    JNI_EXCEPTION_CHECK(env);
    if (! out_vi_jarray) throw jni_bad_alloc("NewObjectArray", __FUNCTION__);
    jvalue out_args[3];
    out_args[0].l = d_suneido_bound_value_global_ref;
    out_args[1].l = out_data_jarray;
    out_args[2].l = out_vi_jarray;
    //
    // UNMARSHALL
    //
    {
        // The reason this code is in a sub-block is because we want the
        // destructor of the jni_array<jbyte> to run before we invoke the
        // callback. This is because the destructor releases the byte array
        // elements, which is necessary to ensure that the unmarshalled data is
        // propagated into the Java side before the callback runs.
        jni_array<jbyte> out(
            env, static_cast<jbyteArray>(static_cast<jobject>(out_data_jarray)),
            d_size_total);
        unmarshaller_vi u(d_size_direct, d_size_total, d_ptr_array.data(),
                          d_ptr_array.data() + d_ptr_array.size(), d_vi_count);
        u.unmarshall_vi(
            reinterpret_cast<char *>(out.data()), args, env,
            static_cast<jobjectArray>(static_cast<jobject>(out_vi_jarray)),
            d_vi_inst_array.data());
    }
    //
    // CALL
    //
    result = env->CallNonvirtualIntMethodA(
        d_suneido_callback_global_ref,
        GLOBAL_REFS->suneido_language_jsdi_type_Callback(),
        GLOBAL_REFS->suneido_language_jsdi_type_Callback__m_invokeVariableIndirect(),
        out_args
    );
    JNI_EXCEPTION_CHECK(env);
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

} // namespace jni
