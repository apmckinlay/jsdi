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

} // namespace jsdi

#endif // __INCLUDED_JNI_EXCEPTION_H__
