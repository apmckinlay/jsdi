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

// [BEGIN:GENERATED CODE last updated Tue Jul 29 14:18:10 PDT 2014]
template <>
suneido_jsdi_marshall_VariableIndirectInstruction jni_enum_to_cpp(JNIEnv * env, jclass clazz, jobject e)
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

static const char * const suneido_jsdi_marshall_VariableIndirectInstruction__NAME[] =
{
    "NO_ACTION",
    "RETURN_JAVA_STRING",
    "RETURN_RESOURCE",
};

std::ostream& operator<<(std::ostream& o, const suneido_jsdi_marshall_VariableIndirectInstruction& e)
{
    if (! (0 <= e && e < 3))
    {
        throw_out_of_range(
            __FUNCTION__,
            e,
            "suneido.jsdi.marshall.VariableIndirectInstruction"
        );
    }
    o << suneido_jsdi_marshall_VariableIndirectInstruction__NAME[e]
      << '<' << static_cast<int>(e) << '>';
    return o;
}

// [END:GENERATED CODE]

} // namespace jsdi
