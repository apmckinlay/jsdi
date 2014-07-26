/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_JNI_EXCEPTION_H__
#define __INCLUDED_JNI_EXCEPTION_H__

/**
 * \file jni_exception.h
 * \author Victor Schappert
 * \since 20130624
 * \brief C++ code useful for handling JNI exceptions
 */

#include "util.h"

#include <sstream>
#include <stdexcept>

#include <jni.h>

namespace jsdi {

//==============================================================================
//                                  MACROS
//==============================================================================

/**
 * \brief Starts a "JNI exception safe" block that C++ exceptions can't bubble
 *        out of.
 * \author Victor Schappert
 * \since 20130624
 * \see JNI_EXCEPTION_SAFE_END(env)
 * \see JNI_EXCEPTION_CHECK(env)
 * \see jsdi::jni_exception
 */
#define JNI_EXCEPTION_SAFE_BEGIN            \
    try {

/**
 * \brief Ends a "JNI exception safe" block that C++ exceptions can't bubble
 *        out of.
 * \author Victor Schappert
 * \since 20130624
 * \see JNI_EXCEPTION_SAFE_BEGIN
 * \see JNI_EXCEPTION_CHECK(env)
 * \see jsdi::jni_exception
 *
 * <p>
 * The purpose of "JNI exception safe" blocks is to ensure that whenever a C++
 * exception is raised \em or a Java/JNI exception is detected, JSDI unwinds
 * the stack to "JNI exception safe" block (which should be the last
 * <code>native</code> frame before control returns to the JVM). The
 * <code>JNI_EXCEPTION_SAFE_END</code> block then takes the appropriate action:
 * </p>
 * <ul>
 * <li>
 * If it's a jsdi::jni_exception, a JNI exception is raised \em only if one is
 * not already pending.
 * </li>
 * <li>
 * Otherwise, if it's any other kind of C++ exception, a JNI exception is
 * raised.
 * </li>
 * </ul>
 */
#define JNI_EXCEPTION_SAFE_END(env)                                     \
    }                                                                   \
    catch (const jsdi::jni_exception& e)                                \
    {                                                                   \
        e.throw_jni(env);                                               \
    }                                                                   \
    catch (const std::exception& e)                                     \
    {                                                                   \
        jsdi::jni_exception(e.what(), false).throw_jni(env);            \
    }                                                                   \
    catch (...)                                                         \
    {                                                                   \
        jsdi::jni_exception("unknown exception", false).throw_jni(env); \
    }

/**
 * \brief Throws a jsdi::jni_exception if there is a pending JNI exception.
 * \author Victor Schappert
 * \since 20130628
 * \see JNI_EXCEPTION_SAFE_BEGIN
 * \see JNI_EXCEPTION_SAFE_END(env)
 * \see jsdi::jni_exception
 *
 * <p>
 * The purpose of this macro is to immediately transfer control to the outermost
 * <code>native</code> stack frame (which must contain a
 * \link JNI_EXCEPTION_SAFE_BEGIN\endlink/\link JNI_EXCEPTION_SAFE_END\endlink
 * block) as soon as a Java exception is raised. The practical effect is to
 * respond properly to the exception condition by stopping whatever is being
 * done and letting the Java-land code handle Java exceptions. This is because
 * the \link JNI_EXCEPTION_SAFE_END\endlink block will catch the
 * jsdi::jni_exception and immediately allow control to exit the
 * <code>native</code> code and return to the last JVM stack frame.
 * </p>
 * <p>
 * A JNI exception check must be done \em everywhere a JNI exception could be
 * raised (\em eg after every call of a JNI function which indicates it may
 * raise an exception).
 * </p>
 */
#define JNI_EXCEPTION_CHECK(env)                                    \
    {                                                               \
    if (env->ExceptionCheck())                                      \
        throw jsdi::jni_exception(std::string("pending"), true);    \
    }

//==============================================================================
//                           class jni_exception
//==============================================================================

/**
 * \brief Exception raised when a JNI exception \em either is pending \em or 
 *        needs to be raised due to an error encountered in the JSDI layer.
 * \author Victor Schappert
 * \since 20130628
 *
 * <p>
 * There are two use cases for this exception:
 * </p>
 * <ol>
 * <li>
 * a pending JNI exception is detected (\em ie a Java exception that will be
 * raised as soon as control returns to the JVM);
 * </li>
 * <li>
 * an error situation is encountered in the JSDI layer where a Java exception
 * should be raised an communicated back to Java-land.
 * </li>
 * </ol>
 * <p>
 * In both of these cases, this exception should be thrown in order to unwind
 * the stack to the last <code>native</code> frame (\em ie the JNI entry-point),
 * where it will be caught and dealt with appropriately by the
 * \link JNI_EXCEPTION_SAFE_BEGIN\endlink/\link JNI_EXCEPTION_SAFE_END\endlink
 * block protecting that frame.
 * </p>
 */
class jni_exception: public std::runtime_error
{
        //
        // DATA
        //

        bool d_jni_except_pending;

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Constructor requiring explicit indication of whether there
         *        is a JNI exception pending.
         * \param what_arg Explanatory string
         * \param jni_except_pending Whether there is a JNI exception pending
         * \see jni_exception(const std::string&, JNIEnv *)
         * \see jni_except_pending() const
         *
         * \attention It is imperative that the value given to the
         * <code>jni_except_pending</code> parameter accurately indicates
         * whether there is a JNI exception pending in the current thread. If
         * the value is inaccurate, the control flow assumptions made by this
         * library break down.
         */
        jni_exception(const std::string& what_arg, bool jni_except_pending);

        /**
         * \brief Constructor that tests for a pending JNI exception.
         * \param what_arg Explanatory string
         * \param env Non-<code>null</code> pointer to the current thread's JNI
         *        environment
         * \see jni_exception(const std::string&, bool)
         * \see jni_except_pending() const
         *
         * <p>
         * Sets the JNI pending exception state of this exception by testing the
         * JNI environment.
         * </p>
         *
         * <p>
         * This is the appropriate constructor to use in situations where a
         * C++ exception must be raised but it is not known whether a JNI
         * exception is already pending.
         * </p>
         *
         * \todo
         * FIXME: Is this constructor really a good idea? The program should
         * never be in a state where it needs to raise a C++ exception and it
         * doesn't know whether a JNI exception is pending!!! If you are, it
         * means you ignored a JNI exception and blithely kept on processing
         * until you had to raise a C++ exception and you are in the position
         * where you can communicate one, but not both, of these exceptions back
         * to Java-land.
         */
        jni_exception(const std::string& what_arg, JNIEnv * env);

        //
        // ACCESSORS
        //

    public:

        /**
         * \brief Indicates whether a JNI exception was pending when this C++
         *        exception was instantiated.
         * \return Whether a JNI exception is pending
         */
        bool jni_except_pending() const;

        /**
         * \brief Call in the last <code>native</code> frame before return to
         *        Java to raise a JNI exception if one is not pending.
         * \see #jni_except_pending() const
         *
         * If #jni_except_pending() const, then nothing is done because there is
         * already a pending JNI exception that the JVM will raise on exit from
         * the <code>native</code> frame and re-entrance into the Java frame.
         * Otherwise, raises an appropriate JNI exception.
         */
        void throw_jni(JNIEnv * env) const;

};

//==============================================================================
//                           class jni_bad_alloc
//==============================================================================

/**
 * \brief Custom JNI exception for JNI out-of-memory situations.
 * \author Victor Schappert
 * \since 20130815
 *
 * Throw this exception when a JNI function gives a return value that indicates
 * it failed because it was out of memory.
 *
 * \attention
 * Do not throw this exception if there is already JNI exception pending. If the
 * JNI function raises an exception for out-of-memory scenarios, you should use
 * \link JNI_EXCEPTION_CHECK(env)\endlink instead.
 */
class jni_bad_alloc : public jni_exception
{
        //
        // INTERNALS
        //

        template<typename StrType1, typename StrType2>
        static std::string make_what(const StrType1&, const StrType2&);

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Constructs a JNI out-of-memory exception.
         * \param jni_function_name Name of the JNI function that failed for
         *                          lack of memory
         * \param throwing_function Name of the C++ function that called the JNI
         *                          function
         */
        template<typename StrType1, typename StrType2>
        jni_bad_alloc(const StrType1& jni_function_name,
                      const StrType2& throwing_function);
};

template<typename StrType1, typename StrType2>
std::string jni_bad_alloc::make_what(const StrType1& jni_function,
                                     const StrType2& throwing_function)
{
    std::ostringstream o;
    o << "JNI function " << jni_function << " returned NULL in "
      << throwing_function << std::endl;
    return o.str(); // OK to return value because of C++11 RValue references
}

template<typename StrType1, typename StrType2>
inline jni_bad_alloc::jni_bad_alloc(const StrType1& jni_function_name,
                                    const StrType2& throwing_function)
    : jni_exception(make_what(jni_function_name, throwing_function), false)
{ }

} // namespace jsdi

#endif // __INCLUDED_JNI_EXCEPTION_H__
