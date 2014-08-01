/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_JSDI_CALLBACK_H__
#define __INCLUDED_JSDI_CALLBACK_H__

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

//==============================================================================
//                         class jsdi_callback_base
//==============================================================================

/**
 * \brief Ancestor class of all callbacks that are capable of calling back into
 *        the JVM
 * \author Victor Schappert
 * \since 20140801
 */
class jsdi_callback_base : public callback, private non_copyable
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
        // CONSTRUCTORS
        //

    protected:

        /** \cond internal */
        jsdi_callback_base(JNIEnv * env, jobject suneido_callback,
                           jobject suneido_bound_value, jint size_direct,
                           jint size_total, jint const * ptr_array,
                           jint ptr_array_size, jint vi_count);
        /** \endcond internal */

    public:

        ~jsdi_callback_base();

        //
        // ACCESSORS
        //

    protected:

        /** \cond internal */
        JNIEnv * fetch_env() const;
        /** \endcond internal */
};

//==============================================================================
//                        struct jsdi_callback_fast0
//==============================================================================

/**
 * \brief Zero-parameter callback
 * \author Victor Schappert
 * \since 20140801
 * \see jsdi_callback_direct
 */
struct jsdi_callback_fast0 : public jsdi_callback_base
{
        /**
         * \brief Constructs a zero-parameter callback
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
         *        <code>0 &le; size_direct &le;
         *        sizeof(</code>\link marshall_word_t\endlink<code>)</code>
         */
        jsdi_callback_fast0(JNIEnv * env, jobject suneido_callback,
                            jobject suneido_bound_value, jint size_direct);

        virtual uint64_t call(marshall_word_t const * args);
};

//==============================================================================
//                        struct jsdi_callback_fast1
//==============================================================================

/**
 * \brief One-parameter fast callback for a limited class of situations
 * \author Victor Schappert
 * \since 20140801
 * \see jsdi_callback_direct
 *
 * This callback may be used when the JSDI parameter type can be marshalled out
 * of a 64-bit integer value provided by the caller and the parameter type uses
 * no indirect storage.
 */
 struct jsdi_callback_fast1 : public jsdi_callback_base
{
        /**
         * \brief Constructs a one parameter callback
         * \param env As described in \link jsdi_callback_fast0\endlink
         * \param suneido_callback As described in
         *        \link jsdi_callback_fast0\endlink
         * \param suneido_bound_value As described in
         *        \link jsdi_callback_fast0\endlink
         * \param size_direct As described in \link jsdi_callback_fast0\endlink
         */
        jsdi_callback_fast1(JNIEnv * env, jobject suneido_callback,
                            jobject suneido_bound_value, jint size_direct);

        virtual uint64_t call(marshall_word_t const * args);
};

//==============================================================================
//                        struct jsdi_callback_fast2
//==============================================================================

/**
 * \brief Two-parameter fast callback for a limited class of situations
 * \author Victor Schappert
 * \since 20140801
 * \see jsdi_callback_direct
 *
 * This callback may be used when the JSDI parameter types can be marshalled out
 * of 64-bit integer values provided by the caller and the parameter types use
 * no indirect storage.
 */
struct jsdi_callback_fast2 : public jsdi_callback_base
{
        /**
         * \brief Constructs a one parameter callback
         * \param env As described in \link jsdi_callback_fast0\endlink
         * \param suneido_callback As described in
         *        \link jsdi_callback_fast0\endlink
         * \param suneido_bound_value As described in
         *        \link jsdi_callback_fast0\endlink
         * \param size_direct As described in \link jsdi_callback_fast0\endlink
         */
        jsdi_callback_fast2(JNIEnv * env, jobject suneido_callback,
                            jobject suneido_bound_value, jint size_direct);

        virtual uint64_t call(marshall_word_t const * args);
};

//==============================================================================
//                        struct jsdi_callback_fast3
//==============================================================================

/**
 * \brief Three-parameter fast callback for a limited class of situations
 * \author Victor Schappert
 * \since 20140801
 * \see jsdi_callback_direct
 *
 * This callback may be used when the JSDI parameter types can be marshalled out
 * of 64-bit integer values provided by the caller and the parameter types use
 * no indirect storage.
 */
struct jsdi_callback_fast3 : public jsdi_callback_base
{
        /**
         * \brief Constructs a one parameter callback
         * \param env As described in \link jsdi_callback_fast0\endlink
         * \param suneido_callback As described in
         *        \link jsdi_callback_fast0\endlink
         * \param suneido_bound_value As described in
         *        \link jsdi_callback_fast0\endlink
         * \param size_direct As described in \link jsdi_callback_fast0\endlink
         */
        jsdi_callback_fast3(JNIEnv * env, jobject suneido_callback,
                            jobject suneido_bound_value, jint size_direct);

        virtual uint64_t call(marshall_word_t const * args);
};

//==============================================================================
//                        struct jsdi_callback_fast4
//==============================================================================

/**
 * \brief Four-parameter fast callback for a limited class of situations
 * \author Victor Schappert
 * \since 20140801
 * \see jsdi_callback_direct
 *
 * This callback may be used when the JSDI parameter types can be marshalled out
 * of 64-bit integer values provided by the caller and the parameter types use
 * no indirect storage.
 */
struct jsdi_callback_fast4 : public jsdi_callback_base
{
        /**
         * \brief Constructs a one parameter callback
         * \param env As described in \link jsdi_callback_fast0\endlink
         * \param suneido_callback As described in
         *        \link jsdi_callback_fast0\endlink
         * \param suneido_bound_value As described in
         *        \link jsdi_callback_fast0\endlink
         * \param size_direct As described in \link jsdi_callback_fast0\endlink
         */
        jsdi_callback_fast4(JNIEnv * env, jobject suneido_callback,
                            jobject suneido_bound_value, jint size_direct);

        virtual uint64_t call(marshall_word_t const * args);
};

//==============================================================================
//                        struct jsdi_callback_direct
//==============================================================================

/**
 * \brief Callback receiving an arbitrary amount of direct storage parameters
 *        but no indirect or variable indirect storage
 * \author Victor Schappert
 * \since 20140801
 * \see jsdi_callback_indirect
 * \see jsdi_callback_vi
 */
struct jsdi_callback_direct : public jsdi_callback_base
{
        /**
         * \brief Constructs a direct-data-only callback
         * \param env As described in \link jsdi_callback_fast0\endlink
         * \param suneido_callback As described in
         *        \link jsdi_callback_fast0\endlink
         * \param suneido_bound_value As described in
         *        \link jsdi_callback_fast0\endlink
         * \param size_direct Size of the on-stack arguments:
         *        <code>0 &le; size_direct &le; size_total</code>
         * \param size_total Total size of the unmarshalled storage
         */
        jsdi_callback_direct(JNIEnv * env, jobject suneido_callback,
                             jobject suneido_bound_value, jint size_direct,
                             jint size_total);


        virtual uint64_t call(marshall_word_t const * args);
};

//==============================================================================
//                       struct jsdi_callback_indirect
//==============================================================================

/**
 * \brief Callback receiving an arbitrary amount of direct storage parameters
 *        with optional indirect storage, but no variable indirect storage
 * \author Victor Schappert
 * \since 20140801
 * \see jsdi_callback_direct
 * \see jsdi_callback_vi
 */
struct jsdi_callback_indirect : public jsdi_callback_base
{
        /**
         * \brief Constructs an indirect-capable callback
         * \param env As described in \link jsdi_callback_fast0\endlink
         * \param suneido_callback As described in
         *        \link jsdi_callback_fast0\endlink
         * \param suneido_bound_value As described in
         *        \link jsdi_callback_fast0\endlink
         * \param size_direct As described in \link jsdi_callback_direct\endlink
         * \param size_total As described in \link jsdi_callback_direct\endlink
         * \param ptr_array Array of tuples indicating which positions in the
         *        direct and indirect storage are pointers, and which positions
         *        they point to
         * \param ptr_array_size Size of <code>ptr_array</code>
         */
        jsdi_callback_indirect(JNIEnv * env, jobject suneido_callback,
                               jobject suneido_bound_value, jint size_direct,
                               jint size_total, jint const * ptr_array,
                               jint ptr_array_size);

        virtual uint64_t call(marshall_word_t const * args);
};

//==============================================================================
//                          class jsdi_callback_vi
//==============================================================================

/**
 * \brief Callback receiving an arbitrary amount of direct storage parameters
 *        with optional indirect storage and variable indirect storage
 * \author Victor Schappert
 * \since 20130806
 * \see jsdi_callback_base
 */
class jsdi_callback_vi : public jsdi_callback_base
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
         * \brief Constructs a callback capable of variable indirect
         *        unmarshalling
         * \param env As described in \link jsdi_callback_fast0\endlink
         * \param suneido_callback As described in
         *        \link jsdi_callback_fast0\endlink
         * \param suneido_bound_value As described in
         *        \link jsdi_callback_fast0\endlink
         * \param size_direct As described in \link jsdi_callback_direct\endlink
         * \param size_total As described in \link jsdi_callback_direct\endlink
         * \param ptr_array As described in \link jsdi_callback_indirect\endlink
         * \param ptr_array_size As described in
         *        \link jsdi_callback_indirect\endlink
         * \param vi_count Number of variable indirect pointers: <code>0 &le;
         *        vi_count</code>
         */
        jsdi_callback_vi(JNIEnv * env, jobject suneido_callback,
                         jobject suneido_bound_value, jint size_direct,
                         jint size_total, jint const * ptr_array,
                         jint ptr_array_size, jint vi_count);

        //
        // ANCESTOR CLASS: callback
        //

    public:

        virtual uint64_t call(marshall_word_t const * args);
};

} // namespace jsdi

#endif // __INCLUDED_JSDI_CALLBACK_H__
