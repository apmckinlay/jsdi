/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: jni_exception.h
// auth: Victor Schappert
// date: 20130624
// desc: Facility for throwing exceptions into JNI (interface)
//==============================================================================

#ifndef __INCLUDED_JNI_EXCEPTION_H__
#define __INCLUDED_JNI_EXCEPTION_H__

#include "util.h"

#include <sstream>
#include <stdexcept>

#include <jni.h>

namespace jsdi {

//==============================================================================
//                                  MACROS
//==============================================================================

#define JNI_EXCEPTION_SAFE_BEGIN            \
    try {

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

#define JNI_EXCEPTION_CHECK(env)                                    \
    {                                                               \
    if (env->ExceptionCheck())                                      \
        throw jsdi::jni_exception(std::string("pending"), true);    \
    }

//==============================================================================
//                           class jni_exception
//==============================================================================

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

        jni_exception(const std::string&, bool jni_except_pending);

        jni_exception(const std::string&, JNIEnv * env);

        //
        // ACCESSORS
        //

    public:

        bool jni_except_pending() const;

        void throw_jni(JNIEnv * env) const;

};

//==============================================================================
//                           class jni_bad_alloc
//==============================================================================

// TODO: docs -- since 20130815
class jni_bad_alloc : public jni_exception
{
        //
        // INTERNALS
        //

        static std::string make_what(const char *, const char *);

        //
        // CONSTRUCTORS
        //

    public:

        jni_bad_alloc(const char * jni_function_name,
                      const char * throwing_function);
};

inline jni_bad_alloc::jni_bad_alloc(const char * jni_function_name,
                                    const char * throwing_function)
    : jni_exception(make_what(jni_function_name, throwing_function), false)
{ }


} // namespace jsdi

#endif // __INCLUDED_JNI_EXCEPTION_H__
