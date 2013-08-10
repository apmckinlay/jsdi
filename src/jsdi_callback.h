#ifndef __INCLUDED_JSDI_CALLBACK_H___
#define __INCLUDED_JSDI_CALLBACK_H___

/**
 * \file jsdi_callback.h
 * \author Victor Schappert
 * \since 20130804
 * \brief Implementation of \link jsdi::callback \endlink which is able to
 *        call back to JSDI classes in Java via JNI
 */

#include "callback.h"
#include "global_refs.h"
#include "jni_util.h"

namespace jsdi {

//==============================================================================
//                      class jsdi_callback_args_basic
//==============================================================================

class jsdi_callback_args_basic : public callback_args, private non_copyable
{
        //
        // DATA
        //

        JNIEnv          * d_env;
        jbyteArray        d_array;
        jboolean          d_is_copy;

        //
        // CONSTRUCTORS
        //

    public:

        jsdi_callback_args_basic(JNIEnv * env, int data_size);

        ~jsdi_callback_args_basic();

        //
        // ACCESSORS
        //

    public:

        JNIEnv * env();

        //
        // MUTATORS
        //

    public:

        jbyteArray release_data();

        virtual void vi_string_ptr(const char * str, int vi_index);
};

inline jsdi_callback_args_basic::jsdi_callback_args_basic(JNIEnv * env,
                                                          int data_size)
    : d_env(env)
    , d_array(env->NewByteArray(data_size))
{
    // NOTE: GetPrimitiveArrayCritical only has defined behaviour if you promise
    //       not to call any other JNI function, or any other function that
    //       causes you to block on another Java thread until you call
    //       ReleasePrimitiveArrayCritical (done by release_array()).
    // FIXME: Eliminate GetPrimitiveArrayCritical() calls. You can't guarantee
    //        in the general case that you won't break their restrictions
    //        because the callback could very well call another 'dll' function.
    // TODO: Make a test with this call chain: Suneido:func() -> dll(..., callback)
    //       -> Suneido:func() -> dll ...
    if (! d_array) throw std::bad_alloc();
    void * ptr = env->GetPrimitiveArrayCritical(d_array, &d_is_copy);
    if (! ptr)
    {
        env->DeleteLocalRef(d_array);
        throw std::bad_alloc();
    }
    set_data(reinterpret_cast<char *>(ptr));
}

inline JNIEnv * jsdi_callback_args_basic::env()
{ return d_env; }

inline jbyteArray jsdi_callback_args_basic::release_data()
{
    assert(data() || !"data() not set or has been released");
    d_env->ReleasePrimitiveArrayCritical(d_array, data(), 0);
    set_data(0);
    return d_array;
}

//==============================================================================
//                         class jsdi_callback_basic
//==============================================================================

class jsdi_callback_basic : public callback, private non_copyable
{
        //
        // DATA
        //

    protected:

        jobject   d_suneido_callback_global_ref;
        jobject   d_suneido_bound_value_global_ref;
        JavaVM  * d_jni_jvm;

        //
        // INTERNALS
        //

    private:

        // GCC doesn't support C++11 delegating constructors yet so need init()
        void init(JNIEnv * env, jobject suneido_callback,
                  jobject suneido_bound_value);

        //
        // CONSTRUCTORS
        //

    protected:

        jsdi_callback_basic(JNIEnv * env,
                            jobject suneido_callback,
                            jobject suneido_bound_value, int size_direct,
                            int size_indirect, const int * ptr_array,
                            int ptr_array_size, int vi_count);

    public:

        jsdi_callback_basic(JNIEnv * env,
                            jobject suneido_callback,
                            jobject suneido_bound_value, int size_direct,
                            int size_indirect, const int * ptr_array,
                            int ptr_array_size);

        ~jsdi_callback_basic();

        //
        // ACCESSORS
        //

    protected:

        virtual callback_args * alloc_args() const;

        //
        // MUTATORS
        //

    protected:

        virtual long call(std::unique_ptr<callback_args> args);

        //
        // STATICS
        //

    protected:

        JNIEnv * fetch_env() const;
};

//==============================================================================
//                        class jsdi_callback_args_vi
//==============================================================================

class jsdi_callback_args_vi : public callback_args, private non_copyable
{
        //
        // DATA
        //

        JNIEnv     * d_env;
        jbyteArray   d_array;
        jobjectArray d_vi_array;
        jboolean     d_is_copy[2];

        //
        // CONSTRUCTORS
        //

    public:

        jsdi_callback_args_vi(JNIEnv * env, int data_size, int vi_count);

        ~jsdi_callback_args_vi();

        //
        // ACCESSORS
        //

    public:

        JNIEnv * env();

        jobjectArray vi_array();

        //
        // MUTATORS
        //

    public:

        jbyteArray release_data();

        virtual void vi_string_ptr(const char * str, int vi_index);
};

inline jsdi_callback_args_vi::jsdi_callback_args_vi(JNIEnv * env, int data_size,
                                                    int vi_count)
    : d_env(env)
    , d_array(0)
    , d_vi_array(0)
{
    d_array = env->NewByteArray(data_size);
    if (! d_array) goto cleanup;
    d_vi_array = env->NewObjectArray(vi_count, GLOBAL_REFS->java_lang_Object(),
                                     0);
    if (! d_vi_array) goto cleanup;
    set_data(
        reinterpret_cast<char *>(env->GetByteArrayElements(d_array,
                                                           &d_is_copy[0])));
    if (! data()) goto cleanup;
    return;
cleanup:
    if (d_vi_array) env->DeleteLocalRef(d_vi_array);
    if (d_array) env->DeleteLocalRef(d_array);
    throw std::bad_alloc();
}

inline JNIEnv * jsdi_callback_args_vi::env()
{ return d_env; }

inline jobjectArray jsdi_callback_args_vi::vi_array()
{ return d_vi_array; }

inline jbyteArray jsdi_callback_args_vi::release_data()
{
    assert(data() || !"data() not set or has been released");
    d_env->ReleaseByteArrayElements(d_array, reinterpret_cast<jbyte *>(data()),
                                    0);
    set_data(0);
    return d_array;
}

//==============================================================================
//                          class jsdi_callback_vi
//==============================================================================

class jsdi_callback_vi : public jsdi_callback_basic
{
        //
        // CONSTRUCTORS
        //

    public:

        jsdi_callback_vi(JNIEnv * env, jobject suneido_callback,
                         jobject suneido_bound_value, int size_direct,
                         int size_indirect, const int * ptr_array,
                         int ptr_array_size, int vi_count);

        //
        // ACCESSORS
        //

    protected:

        virtual callback_args * alloc_args() const;

        //
        // MUTATORS
        //

    protected:

        virtual long call(std::unique_ptr<callback_args> args);
};

inline jsdi_callback_vi::jsdi_callback_vi(JNIEnv * env,
                                          jobject suneido_callback,
                                          jobject suneido_bound_value,
                                          int size_direct, int size_indirect,
                                          const int * ptr_array,
                                          int ptr_array_size, int vi_count)
    : jsdi_callback_basic(env, suneido_callback, suneido_bound_value,
                          size_direct, size_indirect, ptr_array,
                          ptr_array_size, vi_count)
{ }

} // namespace jsdi

#endif // __INCLUDED_JSDI_CALLBACK_H___
