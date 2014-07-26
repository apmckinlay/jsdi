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

#include "log.h"

#include <cassert>
#include <cstdlib>

namespace jsdi {

//==============================================================================
//                           class jni_exception
//==============================================================================

jni_exception::jni_exception(const std::string& what_arg,
                             bool jni_except_pending)
    : std::runtime_error(what_arg)
    , d_jni_except_pending(jni_except_pending)
{ }

// FIXME: I think this constructor may be conceptually flawed. See the \todo
//        block in the header file.
jni_exception::jni_exception(const std::string& what_arg, JNIEnv * env)
    : std::runtime_error(what_arg)
    , d_jni_except_pending(env->ExceptionCheck() ? true : false)
{ }

void jni_exception::throw_jni(JNIEnv * env) const
{
    //
    // If JNI already has an exception marked pending, we don't need to do
    // anything -- just let the JVM raise it as soon as control flows back to
    // Java-land.
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
        clazz = env->FindClass("suneido/jsdi/JSDIException");
        if (! clazz)
        {
            LOG_ERROR("Unable to find normal exception class");
            clazz = env->FindClass("java/lang/RuntimeException");
            if (! clazz)
            {
                LOG_FATAL("Failed to find ANY exception class");
                env->FatalError("Failed to find ANY exception class");
                std::abort();
            }
        }
        if (0 != env->ThrowNew(clazz, what()))
        {
            LOG_FATAL("Failed to throw a Java exception from JNI");
            env->FatalError("Failed to throw a Java exception from JNI");
            std::abort();
        }
    }
}

} // namespace jsdi
