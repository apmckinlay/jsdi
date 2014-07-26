/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: callback_x86.cpp
// auth: Victor Schappert
// date: 20130806
// desc: Implementation of callback which is able to call back into JNI
//==============================================================================

#include "callback_x86.h"

#include "log.h"
#include "marshalling.h"

#include <stdexcept>

namespace jsdi {
namespace abi_x86 {

//==============================================================================
//                         class callback_x86_basic
//==============================================================================

namespace {

jobject globalize(JNIEnv * env, jobject value, const char * name)
{
    assert(env || !"environment cannot be NULL");
    assert(value || !"can't globalize a null reference");
    jobject const result(env->NewGlobalRef(value));
    if (result) return result;
    JNI_EXCEPTION_CHECK(env);
    std::ostringstream() << "NewGlobalRef(env => " << env << ", " << name
                         << " => " << value << ')'
                         << throw_cpp<jni_bad_alloc, const char *>(__FUNCTION__);
}

} // anonymous namespace

callback_x86_basic::callback_x86_basic(JNIEnv * env, jobject suneido_callback,
                                       jobject suneido_sucallable,
                                       int size_direct, int size_total,
                                       const int * ptr_array,
                                       int ptr_array_size, int vi_count)
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

callback_x86_basic::callback_x86_basic(JNIEnv * env, jobject suneido_callback,
                                       jobject suneido_bound_value,
                                       int size_direct, int size_total,
                                       const int * ptr_array,
                                       int ptr_array_size)
    : callback_x86_basic(env, suneido_callback, suneido_bound_value,
                         size_direct, size_total, ptr_array, ptr_array_size, 0)
{ }

callback_x86_basic::~callback_x86_basic()
{
    JNIEnv * env(fetch_env());
    if (env)
    {
        env->DeleteGlobalRef(d_suneido_callback_global_ref);
        env->DeleteGlobalRef(d_suneido_bound_value_global_ref);
    }
}

uint64_t callback_x86_basic::call(const marshall_word_t * args)
{
    uint64_t result(0);
    LOG_TRACE("callback_x86_basic::call( this => " << this << ", args => "
                                                   << args << " )");
    JNIEnv * const env(fetch_env());
    if (! env) return 0;
    JNI_EXCEPTION_SAFE_BEGIN
    /* NOTE A: The reason for the exception check immediately below is to ensure
     *         that this callback doesn't execute at all if a JNI exception was
     *         raised before this callback is called.
     *
     *         How could that possibly happen?????
     *
     *         At the moment, it can happen if the same invocation of a 'dll'
     *         function calls multiple callbacks. Recall that a 'dll' function
     *         is a black box, usually a Win32 API function written in plain
     *         C and/or assembly language. The 'dll' has no idea whether or not
     *         a callback raised a JNI exception or not. So imagine a 'dll'
     *         function implemented like this:
     *
     *             __stdcall void dll_func(callback1_ptr f, calblack2_ptr g)
     *             {
     *                 ...  // do some stuff
     *                 f(); // call first callback
     *                 ...  // do some more stuff
     *                 g(); // call second callback
     *             }
     *
     *        Now if f() causes a Java/JNI exception to be raised, JNI will be
     *        in an exception state when g() is called. this exception check
     *        at least ensure that g() will not run.
     *
     *        THIS IS A STOPGAP SOLUTION! The appropriate solution is to:
     *            1) Wrap the callback invocation in a Win32 SEH __try/__except
     *               block.
     *            2) Ensure that any callback which raises a JNI exception
     *                   (i)  cleans up after itself; and then
     *                   (ii) raises an SEH exception which should cause SEH to
     *                        immediately unwind out to the next SEH __except
     *                        block.
     *            3) Wrap stdcall_invoke calls in SEH as well.
     *
     *        However, because Microsoft's cl.exe is the only compiler package
     *        which supports SEH, we will need to build exclusively in VS to
     *        take that step.
     *
     *        See also NOTEs (B), (C), and (D) below.
     */
    JNI_EXCEPTION_CHECK(env);
    //
    // SET UP
    //
    static_assert(sizeof(marshall_word_t) == sizeof(jint), "word size issue");
    const int num_words = d_size_total / sizeof(marshall_word_t);
    jni_auto_local<jobject> out_jarray(env, env->NewIntArray(num_words));
    JNI_EXCEPTION_CHECK(env);
    if (! out_jarray) throw jni_bad_alloc("NewIntArray", __FUNCTION__);
    jvalue out_args[2];
    out_args[0].l = d_suneido_bound_value_global_ref;
    out_args[1].l = out_jarray;
    //
    // UNMARSHALL
    //
    {
        // The reason this code is in a sub-block is because we want the
        // destructor of the jni_critical_array<jint> to run before we invoke
        // the callback. This is because the destructor releases the critical
        // array and the changes to the array aren't guaranteed to propagate
        // back to the Java side until that moment.
        static_assert(sizeof(marshall_word_t) == sizeof(jint),
                      "word size mismatch");
        jni_critical_array<jint> out(
            env, static_cast<jintArray>(static_cast<jobject>(out_jarray)),
            num_words);
        unmarshaller_indirect u(d_size_direct, d_size_total, d_ptr_array.data(),
                                d_ptr_array.data() + d_ptr_array.size());
        u.unmarshall_indirect(args,
                              reinterpret_cast<marshall_word_t *>(out.data()));
    }
    //
    // CALL
    //
    result = env->CallLongMethodA(
        d_suneido_callback_global_ref,
        GLOBAL_REFS->suneido_jsdi_type_Callback__m_invoke(), out_args);
    /* NOTE B: The above JNI call need not be followed by a JNI_EXCEPTION_CHECK
     *         as long as it is the the last non-exception-handling code in the
     *         function. However, as explained in NOTE A, the JNI call should
     *         really be surrounded by a SEH __try/__catch pair in case a
     *         nested callback throws.
     */
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

JNIEnv * callback_x86_basic::fetch_env() const
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
//                           class callback_x86_vi
//==============================================================================

uint64_t callback_x86_vi::call(const marshall_word_t * args)
{
    uint64_t result(0);
    LOG_TRACE("callback_x86_vi::call( this => " << this << ", args => "
                                                << args << " )");
    JNIEnv * const env(fetch_env());
    if (!env) return 0;
    JNI_EXCEPTION_SAFE_BEGIN
    /** NOTE C: See NOTE A, above. The same argument applies verbatim. */
    JNI_EXCEPTION_CHECK(env);
    //
    // SET UP
    //
    static_assert(sizeof(marshall_word_t) == sizeof(jint), "word size issue");
    const int num_words = d_size_total / sizeof(marshall_word_t);
    jni_auto_local<jobject> out_data_jarray(env, env->NewIntArray(num_words));
    JNI_EXCEPTION_CHECK(env);
    if (! out_data_jarray) throw jni_bad_alloc("NewIntArray", __FUNCTION__);
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
        jni_array<jint> out(
            env, static_cast<jintArray>(static_cast<jobject>(out_data_jarray)),
            num_words);
        unmarshaller_vi u(d_size_direct, d_size_total, d_ptr_array.data(),
                          d_ptr_array.data() + d_ptr_array.size(), d_vi_count);
        u.unmarshall_vi(
            args,  reinterpret_cast<marshall_word_t *>(out.data()), env,
            static_cast<jobjectArray>(static_cast<jobject>(out_vi_jarray)),
            d_vi_inst_array.data());
    }
    //
    // CALL
    //
    result = env->CallLongMethodA(
        d_suneido_callback_global_ref,
        GLOBAL_REFS->suneido_jsdi_type_Callback__m_invokeVariableIndirect(),
        out_args
    );
    /** NOTE D: See NOTE B, above. The same argument applies verbatim. */
    JNI_EXCEPTION_SAFE_END(env);
    return result;
}

} // namespace abi_x86
} // namespace jni
