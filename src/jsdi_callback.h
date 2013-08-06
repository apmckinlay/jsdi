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
        jni_array<jbyte>  d_data;

        //
        // CONSTRUCTORS
        //

    public:

        jsdi_callback_args_basic(JNIEnv * env, int data_size);

        //
        // ACCESSORS
        //

    public:

        JNIEnv * env();

        jni_array<jbyte>& data_as_jni_array();

        //
        // MUTATORS
        //

    public:

        virtual void vi_string_ptr(const char * str, int vi_index);
};

inline jsdi_callback_args_basic::jsdi_callback_args_basic(JNIEnv * env,
                                                          int data_size)
    : d_env(env)
    , d_data(env, data_size)
{ set_data(reinterpret_cast<char *>(d_data.data())); }

inline JNIEnv * jsdi_callback_args_basic::env()
{ return d_env; }

inline jni_array<jbyte>& jsdi_callback_args_basic::data_as_jni_array()
{ return d_data; }

//==============================================================================
//                         class jsdi_callback_basic
//==============================================================================

class jsdi_callback_basic : public callback, private non_copyable
{
        //
        // DATA
        //

        jobject   d_suneido_callback_global_ref;
        jobject   d_suneido_sucallable_global_ref;
        JavaVM  * d_jni_jvm;

        //
        // INTERNALS
        //

    private:

        // GCC doesn't support C++11 delegating constructors yet so need init()
        void init(JNIEnv * env, jobject suneido_callback,
                  jobject suneido_sucallable);

        //
        // CONSTRUCTORS
        //

    protected:

        jsdi_callback_basic(JNIEnv * env,
                            jobject suneido_callback,
                            jobject suneido_sucallable, int size_direct,
                            int size_indirect, const int * ptr_array,
                            int ptr_array_size, int vi_count);

    public:

        jsdi_callback_basic(JNIEnv * env,
                            jobject suneido_callback,
                            jobject suneido_sucallable, int size_direct,
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

        virtual long call(callback_args& args);

        //
        // STATICS
        //

    protected:

        JNIEnv * fetch_env() const;
};

//==============================================================================
//                        class jsdi_callback_args_vi
//==============================================================================

class jsdi_callback_args_vi : public jsdi_callback_args_basic
{
        //
        // DATA
        //

        jobjectArray d_vi_array;

        //
        // CONSTRUCTORS
        //

    public:

        jsdi_callback_args_vi(JNIEnv * env, int data_size, int vi_count);

        //
        // ACCESSORS
        //

    public:

        jobjectArray vi_array();

        //
        // MUTATORS
        //

    public:

        virtual void vi_string_ptr(const char * str, int vi_index);
};

inline jsdi_callback_args_vi::jsdi_callback_args_vi(JNIEnv * env, int data_size,
                                                    int vi_count)
    : jsdi_callback_args_basic(env, data_size)
    , d_vi_array(
          env->NewObjectArray(vi_count, GLOBAL_REFS->java_lang_Object(), 0))
{ assert(d_vi_array || !"failed to allocate variable indirect array"); }

inline jobjectArray jsdi_callback_args_vi::vi_array()
{ return d_vi_array; }

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
                         jobject suneido_sucallable, int size_direct,
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

        virtual long call(callback_args& args);
};

inline jsdi_callback_vi::jsdi_callback_vi(JNIEnv * env,
                                          jobject suneido_callback,
                                          jobject suneido_sucallable,
                                          int size_direct, int size_indirect,
                                          const int * ptr_array,
                                          int ptr_array_size, int vi_count)
    : jsdi_callback_basic(env, suneido_callback, suneido_sucallable,
                          size_direct, size_indirect, ptr_array,
                          ptr_array_size, vi_count)
{ }

} // namespace jsdi

#endif // __INCLUDED_JSDI_CALLBACK_H___
