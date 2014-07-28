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
#include "jsdi_ole2.h"

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
         * \brief Obtains the <code>IDispatch</code> interface on the
         *        underlying, if available.
         * \param iunk Non-NULL pointer to an <code>IUnknown</code> interface
         * \return Pointer to the <code>IDispatch</code> interface, or NULL if
         *         it is not available
         *
         * In the case of a non-NULL return value, it is caller's responsibility
         * to call <code>Release()</code> on the returned interface pointer.
         */
        static IDispatch * query_for_dispatch(IUnknown * iunk);

        /**
         * \brief The <code>progid</code>, if available, corresponding to an
         *        <code>IDispatch</code> interface's underlying object as a JNI
         *        string.
         * \param idisp Non-NULL pointer to an <code>IDispatch</code> interface
         * \param env Non-NULL pointer to the JNI environment for the current
         *        JNI method invocation (this is required to return a
         *        <code>jstring</code>)
         * \return A JNI string containing the <code>progid</code>, or the NULL
         *         JNI string if no <code>progid</code> is available.
         * \see #create_from_progid(JNIEnv *, jst ring, IUnknown *&,
         *                          IDispatch *&);
         */
        static jstring get_progid(IDispatch * idisp, JNIEnv * env);

        /**
         * \brief Constructs a COM object using <code>CoCreateInstance()</code>
         *        and obtains an <code>IDispatch</code> interface on it if that
         *        interface is available, or an <code>IUnknown</code> interface
         *        on it otherwise.
         * \param env Non-NULL pointer to the JNI environment for the current
         *        JNI method invocation (required to obtain the string value
         *        within <code>progid</code>)
         * \param progid JNI string containing the <code>progid</code> of the
         *        COM object to create.
         * \param iunk Receives the <code>IUnknown</code> interface pointer. The
         *        value returned is NULL if the function returns
         *        <code>false</code> or if <code>idisp</code> receives a
         *        non-NULL value, or a valid non-NULL pointer otherwise.
         * \param idisp Receives the <code>IDispatch</code> interface pointer.
         *        The value returned is NULL if the function returns
         *        <code>false</code> or if <code>iunk</code> receives a non-NULL
         *        value, or a valid non-NULL pointer otherwise.
         * \return The return value is <code>true</code> if the COM object is
         *         successfully constructed (thus one or the other, but not
         *         both, of <code>iunk</code> and <code>idisp</code> receive a
         *         non-NULL value), or <code>false</code> otherwise.
         * \see #get_progid(IDispatch *, JNIEnv *)
         *
         * As should be obvious from the documentation, the function attempts
         * to obtain the <code>IDispatch</code> interface and only obtains
         * <code>IUnknown</code> if the COM object does not support
         * <code>IDispatch</code>.
         *
         * It is the caller's responsibility to call <code>Release()</code> on
         * the returned interface pointer, if any.
         */
        static bool create_from_progid(JNIEnv * env, jstring progid,
                                       IUnknown *& iunk, IDispatch *& idisp);

        /**
         * \brief Obtains the dispatch identifier (an integer) for a given
         *        property or method name supported by an <code>IDispatch</code>
         *        interface.
         * \param idisp Non-NULL pointer to an <code>IDispatch</code> interface
         *        on which the member name should be looked up
         * \param env Non-NULL pointer to the JNI environment for the current
         *        JNI method invocation (required to obtain the string value
         *        within <code>name</code>, and so that a JNI exception can be
         *        raised if <code>name</code> is not found)
         * \param name Member name to look up
         * \return Valid dispatch identifier
         * \throw jni_exception If <code>name</code> is not found&mdash;in which
         *        case, a <code>COMException</code> is raised in the JNI
         *        environment as well
         * \see #property_get(IDispatch *, DISPID, JNIEnv *)
         * \see #property_put(IDispatch *, DISPID, JNIEnv *, jobject)
         * \see #call_method(IDispatch *, DISPID, JNIEnv *, jobjectArray)
         */
        static DISPID get_dispid_of_name(IDispatch * idisp, JNIEnv * env,
                                         jstring name);

        /**
         * \brief Fetches the value of a property from an <code>IDispatch</code>
         *        interface pointer and returns it as a jSuneido-compatible
         *        Java object.
         * \param idisp Non-NULL pointer to an <code>IDispatch</code> interface
         *        on which the property should be looked up
         * \param dispid Dispatch identifier of the property to fetch
         * \param env Non-NULL pointer to the JNI environment (required to
         *        convert the COM property value into a Java type, and to raise
         *        an exception if needed)
         * \return JNI object compatible with the jSuneido type system
         * \throw jni_exception If the property get fails, or if the property
         *        value cannot be converted from COM to jSuneido&mdash;and in
         *        in either case a <code>COMException</code> is raised in the
         *        JNI environment as well
         * \see #property_put(IDispatch *, DISPID, JNIEnv *, jobject)
         * \see #call_method(IDispatch *, DISPID, JNIEnv *, jobjectArray)
         * \see #get_dispid_of_name(IDispatch *, JNIEnv *, jstring)
         */
        static jobject property_get(IDispatch * idisp, DISPID dispid,
                                    JNIEnv * env);

        /**
         * \brief Sets a property on an <code>IDispatch</code> interface pointer
         *        to the value of a jSuneido-compatible Java object.
         * \param idisp Non-NULL pointer to an <code>IDispatch</code> interface
         *        on which the property is to be set
         * \param dispid Dispatch identifier of the property to set
         * \param env Non-NULL pointer to the JNI environment (required to
         *        convert the Java value into a COM-compatible type, and to
         *        raise an exception if needed)
         * \param value Value to assign to property
         * \throw jni_exception If the property put fails, or if the property
         *        value cannot be converted from jSuneido to COM&mdash;and in
         *        in either case a <code>COMException</code> is raised in the
         *        JNI environment as well
         * \see #property_get(IDispatch *, DISPID, JNIEnv *)
         * \see #call_method(IDispatch *, DISPID, JNIEnv *, jobjectArray)
         * \see #get_dispid_of_name(IDispatch *, JNIEnv *, jstring)
         */
        static void property_put(IDispatch * idisp, DISPID dispid, JNIEnv * env,
                                 jobject value);

        /**
         * \brief Invokes a method on an <code>IDispatch</code> interface
         *        pointer
         * \param idisp Non-NULL pointer to an <code>IDispatch</code> interface
         *        on which the method is to be invoked
         * \param dispid Dispatch identifier of the method to call
         * \param env Non-NULL pointer to the JNI environment (required to
         *        convert the Java argument values into COM-compatible values,
         *        and to raise an exception if needed)
         * \param args Array of arguments to pass to the method
         * \throw jni_exception If the method invocation fails, or if any
         *        argument cannot be converted from jSuneido to COM&mdash;and in
         *        in either case a <code>COMException</code> is raised in the
         *        JNI environment as well
         * \see #property_put(IDispatch *, DISPID, JNIEnv *, jobject)
         * \see #property_get(IDispatch *, DISPID, JNIEnv *)
         * \see #get_dispid_of_name(IDispatch *, JNIEnv *, jstring)
         */
        static jobject call_method(IDispatch * idisp, DISPID dispid,
                                   JNIEnv * env, jobjectArray args);
};

} // namespace jsdi

#endif // __INCLUDED_COM_H___
