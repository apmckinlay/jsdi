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

/**
 * \brief Callback for Windows x86 platform that can unmarshall direct and
 *        indirect data, but not variable indirect data
 * \author Victor Schappert
 * \since 20130806
 * \see callback_x86_vi
 */
class callback_x86_basic : public callback, private non_copyable
{
        //
        // DATA
        //

    protected:

        /** \cond internal */
        jobject   d_suneido_callback_global_ref;
        jobject   d_suneido_bound_value_global_ref;
        JavaVM  * d_jni_jvm;
        /** \endcond internal */

        //
        // INTERNALS
        //

    private:

        //
        // CONSTRUCTORS
        //

    protected:

        /** \cond internal */
        callback_x86_basic(JNIEnv * env, jobject suneido_callback,
                           jobject suneido_bound_value, jint size_direct,
                           jint size_total, jint const * ptr_array,
                           jint ptr_array_size, jint vi_count);
        /** \endcond internal */

    public:

        /**
         * \brief Constructs a basic x86 callback
         * \param env Non-NULL pointer to the JNI environment for the current
         *        thread
         * \param suneido_callback Non-NULL reference to a Java
         *        <code>Object</code> of type
         *        <code>suneido.jsdi.type.Callback</code> (the Java entry-point
         *        to be invoked when this C++ callback is called)
         * \param suneido_bound_value Non-NULL reference to a Java
         *        <code>Object</code> of type
         *        <code>suneido.language.SuCallable</code> representing the
         *        Suneido language callable value that is bound to the callback
         *        (this is the code that is ultimately desired to be called by
         *        the Suneido programmer)
         * \param size_direct Size of the on-stack arguments:
         *        <code>0 &le size_direct &le size_total</code>
         * \param size_total Total size of the unmarshalled storage
         * \param ptr_array Array of tuples indicating which positions in the
         *        direct and indirect storage are pointers, and which positions
         *        they point to
         * \param ptr_array_size Size of <code>ptr_array</code>
         */
        callback_x86_basic(JNIEnv * env, jobject suneido_callback,
                           jobject suneido_bound_value, jint size_direct,
                           jint size_total, jint const * ptr_array,
                           jint ptr_array_size);

        ~callback_x86_basic();

        //
        // MUTATORS
        //

    protected:

        virtual uint64_t call(marshall_word_t const * args);

        //
        // ACCESSORS
        //

    protected:

        /** \cond internal */
        JNIEnv * fetch_env() const;
        /** \endcond internal */
};

//==============================================================================
//                          class callback_x86_vi
//==============================================================================

/**
 * \brief Callback for Windows x86 platform that can unmarshall variable
 *        indirect data, as well as direct and indirect data
 * \author Victor Schappert
 * \since 20130806
 * \see callback_x86_basic
 */
class callback_x86_vi : public callback_x86_basic
{
        //
        // DATA
        //

        std::vector<jint> d_vi_inst_array;

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Constructs an x86 callback with variable indirect capability
         * \param env Non-NULL pointer to the JNI environment for the current
         *        thread
         * \param suneido_callback Non-NULL reference to a Java
         *        <code>Object</code> of type
         *        <code>suneido.jsdi.type.Callback</code> (the Java entry-point
         *        to be invoked when this C++ callback is called)
         * \param suneido_bound_value Non-NULL reference to a Java
         *        <code>Object</code> of type
         *        <code>suneido.language.SuCallable</code> representing the
         *        Suneido language callable value that is bound to the callback
         *        (this is the code that is ultimately desired to be called by
         *        the Suneido programmer)
         * \param size_direct Size of the on-stack arguments:
         *        <code>0 &le size_direct &le size_total</code>
         * \param size_total Total size of the unmarshalled storage
         * \param ptr_array Array of tuples indicating which positions in the
         *        direct and indirect storage are pointers, and which positions
         *        they point to
         * \param ptr_array_size Size of <code>ptr_array</code>
         * \param vi_count Number of variable indirect pointers: <code>0 &le
         *        vi_count</code>
         */
        callback_x86_vi(JNIEnv * env, jobject suneido_callback,
                         jobject suneido_bound_value, jint size_direct,
                         jint size_total, jint const * ptr_array,
                         jint ptr_array_size, jint vi_count);

        //
        // MUTATORS
        //

    protected:

        virtual uint64_t call(marshall_word_t const * args);
};

inline callback_x86_vi::callback_x86_vi(JNIEnv * env, jobject suneido_callback,
                                        jobject suneido_bound_value,
                                        jint size_direct, jint size_total,
                                        jint const * ptr_array,
                                        jint ptr_array_size, jint vi_count)
    : callback_x86_basic(env, suneido_callback, suneido_bound_value,
                         size_direct, size_total, ptr_array, ptr_array_size,
                         vi_count)
    , d_vi_inst_array(
          vi_count,
          static_cast<jint>(
              suneido_jsdi_marshall_VariableIndirectInstruction::RETURN_JAVA_STRING)
      )
{ }

} // namespace abi_x86
} // namespace jsdi

#endif // __INCLUDED_CALLBACK_X86_H___
