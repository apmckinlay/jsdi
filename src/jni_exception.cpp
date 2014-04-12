/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: jni_exception.cpp
// auth: Victor Schappert
// date: 20130624
// desc: Facility for throwing exceptions into JNI (implementation)
//==============================================================================

#include "jni_exception.h"

#include <cassert>
#include <cstdlib>
#include <iostream>

namespace jsdi {

//==============================================================================
//                           class jni_exception
//==============================================================================

jni_exception::jni_exception(const std::string& str, bool jni_except_pending)
    : std::runtime_error(str)
    , d_jni_except_pending(jni_except_pending)
{ }

jni_exception::jni_exception(const std::string& str, JNIEnv * env)
    : std::runtime_error(str)
    , d_jni_except_pending(env->ExceptionCheck() ? true : false)
{ }

void jni_exception::throw_jni(JNIEnv * env) const
{
    //
    // If JNI already has an exception marked pending, we don't need to do
    // anything -- just let it happen.
    //
    if (d_jni_except_pending)
    {
        assert(env->ExceptionCheck());
    }
    //
    // Otherwise, we need to construct and throw a Java exception. We don't
    // rely on global_refs because at this point they might not have been
    // successfully created.
    //
    else
    {
        jclass clazz(0);
        // Try to get access to the JSDI exception class;
        clazz = env->FindClass("suneido/language/jsdi/JSDIException");
        if (! clazz)
        {
            clazz = env->FindClass("java/lang/RuntimeException");
        }
        assert(clazz || !"Failed to locate any JNI exception to throw");
        if (0 != env->ThrowNew(clazz, what()))
        {
            std::cerr << "Failed to throw a Java exception from JNI in "
                      << __FILE__ << " at line " << __LINE__ << std::endl;
            env->FatalError("Failed to throw a Java exception from JNI");
            std::abort();
        }
    }
}

//==============================================================================
//                           class jni_bad_alloc
//==============================================================================

std::string jni_bad_alloc::make_what(const char * jni_function,
                                     const char * throwing_function)
{
    std::ostringstream o;
    o << "JNI function " << jni_function << " returned NULL in "
      << throwing_function << std::endl;
    return o.str(); // OK to return value because of C++11 RValue references
}

} // namespace jsdi
