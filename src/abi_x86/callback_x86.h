/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_CALLBACK_X86_H___
#define __INCLUDED_CALLBACK_X86_H___

/**
 * \file callback_x86.h
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
//                         class callback_x86_basic
//==============================================================================

class callback_x86_basic : public callback, private non_copyable
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

        callback_x86_basic(JNIEnv * env, jobject suneido_callback,
                           jobject suneido_bound_value, int size_direct,
                           int size_total, const int * ptr_array,
                           int ptr_array_size, int vi_count);

    public:

        callback_x86_basic(JNIEnv * env, jobject suneido_callback,
                           jobject suneido_bound_value, int size_direct,
                           int size_total, const int * ptr_array,
                           int ptr_array_size);

        ~callback_x86_basic();

        //
        // MUTATORS
        //

    protected:

        virtual uint64_t call(const marshall_word_t * args);

        //
        // STATICS
        //

    protected:

        JNIEnv * fetch_env() const;
};

//==============================================================================
//                          class callback_x86_vi
//==============================================================================

class callback_x86_vi : public callback_x86_basic
{
        //
        // DATA
        //

        std::vector<int> d_vi_inst_array;

        //
        // CONSTRUCTORS
        //

    public:

        callback_x86_vi(JNIEnv * env, jobject suneido_callback,
                         jobject suneido_bound_value, int size_direct,
                         int size_total, const int * ptr_array,
                         int ptr_array_size, int vi_count);

        //
        // MUTATORS
        //

    protected:

        virtual uint64_t call(const marshall_word_t * args);
};

inline callback_x86_vi::callback_x86_vi(JNIEnv * env, jobject suneido_callback,
                                        jobject suneido_bound_value,
                                        int size_direct, int size_total,
                                        const int * ptr_array,
                                        int ptr_array_size, int vi_count)
    : callback_x86_basic(env, suneido_callback, suneido_bound_value,
                         size_direct, size_total, ptr_array, ptr_array_size,
                         vi_count)
    , d_vi_inst_array(
          vi_count,
          static_cast<int>(
              suneido_jsdi_VariableIndirectInstruction::RETURN_JAVA_STRING)
      )
{ }

} // namespace abi_x86
} // namespace jsdi

#endif // __INCLUDED_CALLBACK_X86_H___
