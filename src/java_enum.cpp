/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: java_enum.cpp
// auth: Victor Schappert
// date: 20130628
// desc: C++ declarations for enumerations whose primary definition is in Java.
//       This file is predominantly auto-generated.
//==============================================================================

#include "java_enum.h"

#include "global_refs.h"

#include <cassert>

namespace jsdi {

namespace {

void throw_out_of_range(const char * func_name, int ordinal,
                        const char * enum_name)
{
    std::ostringstream() << func_name << ": ordinal value " << ordinal
                         << " is outside the range of enumeration "
                         << enum_name << throw_cpp<jni_exception, bool>(false);
}

} // anonymous namespace

namespace java_enum {

// [BEGIN:GENERATED CODE last updated Thu Jul 31 19:37:28 PDT 2014]
template <>
suneido_jsdi_marshall_VariableIndirectInstruction jni_enum_to_cpp(JNIEnv * env, jobject e)
{
    jmethodID method_id = GLOBAL_REFS->java_lang_Enum__m_ordinal();
    jint ordinal = env->CallIntMethod(e, method_id);
    JNI_EXCEPTION_CHECK(env);
    return ordinal_enum_to_cpp<suneido_jsdi_marshall_VariableIndirectInstruction>(ordinal);
}

template <>
suneido_jsdi_marshall_VariableIndirectInstruction ordinal_enum_to_cpp(int e)
{
    if (! (0 <= e && e < 3))
    {
        throw_out_of_range(
            __FUNCTION__,
            e,
            "suneido.jsdi.marshall.VariableIndirectInstruction"
        );
    }
    return static_cast<suneido_jsdi_marshall_VariableIndirectInstruction>(e);
}

template <>
jobject cpp_to_jni_enum(JNIEnv * env, suneido_jsdi_LogLevel e)
{
    jmethodID method_id = GLOBAL_REFS->suneido_jsdi_LogLevel__m_values();
    jobjectArray values = static_cast<jobjectArray>(env->CallStaticObjectMethod(GLOBAL_REFS->suneido_jsdi_LogLevel(), method_id));
    JNI_EXCEPTION_CHECK(env);
    assert(values || !"got null from JNI");
    jobject result = env->GetObjectArrayElement(values, static_cast<jsize>(e));
    JNI_EXCEPTION_CHECK(env);
    assert(result || !"got null from JNI");
    env->DeleteLocalRef(values);
    return result;
}

template <>
suneido_jsdi_LogLevel jni_enum_to_cpp(JNIEnv * env, jobject e)
{
    jmethodID method_id = GLOBAL_REFS->java_lang_Enum__m_ordinal();
    jint ordinal = env->CallIntMethod(e, method_id);
    JNI_EXCEPTION_CHECK(env);
    return ordinal_enum_to_cpp<suneido_jsdi_LogLevel>(ordinal);
}

template <>
suneido_jsdi_LogLevel ordinal_enum_to_cpp(int e)
{
    if (! (0 <= e && e < 7))
    {
        throw_out_of_range(
            __FUNCTION__,
            e,
            "suneido.jsdi.LogLevel"
        );
    }
    return static_cast<suneido_jsdi_LogLevel>(e);
}

static const char * const suneido_jsdi_LogLevel__NAME[] =
{
    "NONE",
    "FATAL",
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG",
    "TRACE",
};

std::ostream& operator<<(std::ostream& o, const suneido_jsdi_LogLevel& e)
{
    if (! (0 <= e && e < 7))
    {
        throw_out_of_range(
            __FUNCTION__,
            e,
            "suneido.jsdi.LogLevel"
        );
    }
    o << suneido_jsdi_LogLevel__NAME[e]
      << '<' << static_cast<int>(e) << '>';
    return o;
}

// [END:GENERATED CODE]

} // namespace java_enum
} // namespace jsdi
