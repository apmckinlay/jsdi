//==============================================================================
// file: jni_exception.cpp
// auth: Victor Schappert
// date: 20130624
// desc: Facility for throwing exceptions into JNI (implementation)
//==============================================================================

#include "jni_exception.h"

#include <cassert>

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
    , d_jni_except_pending(env->ExceptionCheck())
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
        clazz = env->FindClass("Lsuneido/language/jsdi/JSDIException;");
        if (! clazz)
        {
            clazz = env->FindClass("Ljava/lang/RuntimeException;");
        }
        assert(clazz || !"Failed to locate any JNI exception to throw");
        if (! env->ThrowNew(clazz, what()))
        {
            // TODO: do something here
            // TODO: use env->FatalError(const char * msg)
            // TODO: should write to a log file and then blow up spectacularly
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
