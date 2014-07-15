/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

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
#include "java_enum.h"
#include "jni_util.h"

namespace jsdi {
namespace abi_x86 {

//==============================================================================
//                         class jsdi_callback_basic
//==============================================================================

class jsdi_callback_basic : public callback<uint32_t>, private non_copyable
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
        // MUTATORS
        //

    protected:

        virtual uint32_t call(const uint32_t * args);

        //
        // STATICS
        //

    protected:

        JNIEnv * fetch_env() const;
};

//==============================================================================
//                          class jsdi_callback_vi
//==============================================================================

class jsdi_callback_vi : public jsdi_callback_basic
{
        //
        // DATA
        //

        std::vector<int> d_vi_inst_array;

        //
        // CONSTRUCTORS
        //

    public:

        jsdi_callback_vi(JNIEnv * env, jobject suneido_callback,
                         jobject suneido_bound_value, int size_direct,
                         int size_indirect, const int * ptr_array,
                         int ptr_array_size, int vi_count);

        //
        // MUTATORS
        //

    protected:

        virtual uint32_t call(const uint32_t * args);
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
    , d_vi_inst_array(
          vi_count,
          static_cast<int>(
              suneido_language_jsdi_VariableIndirectInstruction::RETURN_JAVA_STRING)
      )
{ }

} // namespace abi_x86
} // namespace jsdi

#endif // __INCLUDED_JSDI_CALLBACK_H___
