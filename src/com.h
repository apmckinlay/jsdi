/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_COM_H___
#define __INCLUDED_COM_H___

/**
 * \file com.h
 * \author Victor Schappert
 * \since 20131011
 * \brief Code for interacting with COM IUnknown and IDispatch interfaces
 */

#include "jni_exception.h"

#include <ole2.h>
#include <jni.h>

namespace jsdi {

/**
 * \brief Contains functions for implementing COMobject capabilities.
 * \author Victor Schappert
 * \since 20131022
 */
struct com
{
        /**
         * \brief Obtains the <dfn>IDispatch</dfn> interface on the underlying,
         *        if available.
         * \param iunk Non-NULL pointer to an <dfn>IUnknown</dfn> interface
         * \return Pointer to the <dfn>IDispatch</dfn> interface, or NULL if
         *         it is not available
         *
         * In the case of a non-NULL return value, it is caller's responsibility
         * to call <dfn>Release()</dfn> on the returned interface pointer.
         */
        static IDispatch * query_for_dispatch(IUnknown * iunk);

        /**
         * \brief The <dfn>progid</dfn>, if available, corresponding to an
         *        <dfn>IDispatch</dfn> interface's underlying object as a JNI
         *        string.
         * \param idisp Non-NULL pointer to an <dfn>IDispatch</dfn> interface
         * \param env Non-NULL pointer to the JNI environment for the current
         *        JNI method invocation (this is required to return a
         *        <dfn>jstring</dfn>)
         * \return A JNI string containing the <dfn>progid</dfn>, or the NULL
         *         JNI string if no <dfn>progid</dfn> is available.
         * \see #create_from_progid(JNIEnv *, jst ring, IUnknown *&,
         *                          IDispatch *&);
         */
        static jstring get_progid(IDispatch * idisp, JNIEnv * env);

        /**
         * \brief Constructs a COM object using <dfn>CoCreateInstance()</dfn>
         *        and obtains an <dfn>IDispatch</dfn> interface on it if that
         *        interface is available, or an <dfn>IUnknown</dfn> interface
         *        on it otherwise.
         * \param env Non-NULL pointer to the JNI environment for the current
         *        JNI method invocation (required to obtain the string value
         *        within <dfn>progid</dfn>)
         * \param progid JNI string containing the <dfn>progid</dfn> of the COM
         *        object to create.
         * \param iunk Receives the <dfn>IUnknown</dfn> interface pointer. The
         *        value returned is NULL if the function returns
         *        <dfn>false</dfn> or if <dfn>idisp</dfn> receives a non-NULL
         *        value, or a valid non-NULL pointer otherwise.
         * \param idisp Receives the <dfn>IDispatch</dfn> interface pointer. The
         *        value returned is NULL if the function returns
         *        <dfn>false</dfn> or if <dfn>iunk</dfn> receives a non-NULL
         *        value, or a valid non-NULL pointer otherwise.
         * \return The return value is <dfn>true</dfn> if the COM object is
         *         successfully constructed (thus one or the other, but not
         *         both, of <dfn>iunk</dfn> and <dfn>idisp</dfn> receive a
         *         non-NULL value), or <dfn>false</dfn> otherwise.
         * \see #get_progid(IDispatch *, JNIEnv *)
         *
         * As should be obvious from the documentation, the function attempts
         * to obtain the <dfn>IDispatch</dfn> interface and only obtains
         * <dfn>IUnknown</dfn> if the COM object does not support
         * <dfn>IDispatch</dfn>.
         *
         * It is the caller's responsibility to call <dfn>Release()</dfn> on the
         * returned interface pointer, if any.
         */
        static bool create_from_progid(JNIEnv * env, jstring progid,
                                       IUnknown *& iunk, IDispatch *& idisp);

        /**
         * \brief Obtains the dispatch identifier (an integer) for a given
         *        property or method name supported by an <dfn>IDispatch</dfn>
         *        interface.
         * \param idisp Non-NULL pointer to an <dfn>IDispatch</dfn> interface
         *        on which the member name should be looked up
         * \param env Non-NULL pointer to the JNI environment for the current
         *        JNI method invocation (required to obtain the string value
         *        within <dfn>name</dfn>, and so that a JNI exception can be
         *        raised if <dfn>name</dfn> is not found)
         * \param name Member name to look up
         * \return Valid dispatch identifier
         * \throw jni_exception If <dfn>name</dfn> is not found&mdash;in which
         *        case, a <dfn>COMException</dfn> is raised in the JNI
         *        environment as well
         * \see #property_get(IDispatch *, DISPID, JNIEnv *)
         *      throw(jni_exception)
         * \see #property_put(IDispatch *, DISPID, JNIEnv *, jobject)
         *      throw(jni_exception)
         * \see #call_method(IDispatch *, DISPID, JNIEnv *, jobjectArray)
         *      throw(jni_exception)
         */
        static DISPID get_dispid_of_name(IDispatch * idisp, JNIEnv * env,
                                         jstring name) throw(jni_exception);

        /**
         * \brief Fetches the value of a property from an <dfn>IDispatch</dfn>
         *        interface pointer and returns it as a jSuneido-compatible
         *        Java object.
         * \param idisp Non-NULL pointer to an <dfn>IDispatch</dfn> interface
         *        on which the property should be looked up
         * \param dispid Dispatch identifier of the property to fetch
         * \param env Non-NULL pointer to the JNI environment (required to
         *        convert the COM property value into a Java type, and to raise
         *        an exception if needed)
         * \return JNI object compatible with the jSuneido type system
         * \throw jni_exception If the property get fails, or if the property
         *        value cannot be converted from COM to jSuneido&mdash;and in
         *        in either case a <dfn>COMException</dfn> is raised in the JNI
         *        environment as well
         * \see #property_put(IDispatch *, DISPID, JNIEnv *, jobject)
         *      throw(jni_exception)
         * \see #call_method(IDispatch *, DISPID, JNIEnv *, jobjectArray)
         *      throw(jni_exception)
         * \see #get_dispid_of_name(IDispatch *, JNIEnv *, jstring)
         *      throw(jni_exception)
         */
        static jobject property_get(IDispatch * idisp, DISPID dispid,
                                    JNIEnv * env) throw(jni_exception);

        /**
         * \brief Sets a property on an <dfn>IDispatch</dfn> interface pointer
         *        to the value of a jSuneido-compatible Java object.
         * \param idisp Non-NULL pointer to an <dfn>IDispatch</dfn> interface
         *        on which the property is to be set
         * \param dispid Dispatch identifier of the property to set
         * \param env Non-NULL pointer to the JNI environment (required to
         *        convert the Java value into a COM-compatible type, and to
         *        raise an exception if needed)
         * \param value Value to assign to property
         * \throw jni_exception If the property put fails, or if the property
         *        value cannot be converted from jSuneido to COM&mdash;and in
         *        in either case a <dfn>COMException</dfn> is raised in the JNI
         *        environment as well
         * \see #property_get(IDispatch *, DISPID, JNIEnv *)
         *      throw(jni_exception)
         * \see #call_method(IDispatch *, DISPID, JNIEnv *, jobjectArray)
         *      throw(jni_exception)
         * \see #get_dispid_of_name(IDispatch *, JNIEnv *, jstring)
         *      throw(jni_exception)
         */
        static void property_put(IDispatch * idisp, DISPID dispid, JNIEnv * env,
                                 jobject value) throw(jni_exception);

        /**
         * \brief Invokes a method on an <dfn>IDispatch</dfn> interface pointer.
         * \param idisp Non-NULL pointer to an <dfn>IDispatch</dfn> interface
         *        on which the method is to be invoked
         * \param dispid Dispatch identifier of the method to call
         * \param env Non-NULL pointer to the JNI environment (required to
         *        convert the Java argument values into COM-compatible values,
         *        and to raise an exception if needed)
         * \param args Array of arguments to pass to the method
         * \throw jni_exception If the method invocation fails, or if any
         *        argument cannot be converted from jSuneido to COM&mdash;and in
         *        in either case a <dfn>COMException</dfn> is raised in the JNI
         *        environment as well
         * \see #property_put(IDispatch *, DISPID, JNIEnv *, jobject)
         *      throw(jni_exception)
         * \see #property_get(IDispatch *, DISPID, JNIEnv *)
         *      throw(jni_exception)
         * \see #get_dispid_of_name(IDispatch *, JNIEnv *, jstring)
         *      throw(jni_exception)
         */
        static jobject call_method(IDispatch * idisp, DISPID dispid,
                                   JNIEnv * env, jobjectArray args)
                                       throw (jni_exception);
};

} // namespace jsdi

#endif // __INCLUDED_COM_H___
