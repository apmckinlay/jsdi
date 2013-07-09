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

// [BEGIN:GENERATED CODE last updated Tue Jul 09 16:15:02 PDT 2013]
template <>
suneido_language_jsdi_type_BasicType jni_enum_to_cpp(JNIEnv * env, jclass clazz, jobject e)
{
    jmethodID method_id = global_refs::ptr->java_lang_Enum__m_ordinal();
    jint ordinal = env->CallIntMethod(e, method_id);
    JNI_EXCEPTION_CHECK(env);
    assert(0 <= ordinal && ordinal < 9);
    return static_cast<suneido_language_jsdi_type_BasicType>(ordinal);
}

static const char * const suneido_language_jsdi_type_BasicType__NAME[] =
{
    "BOOL",
    "CHAR",
    "SHORT",
    "LONG",
    "INT64",
    "FLOAT",
    "DOUBLE",
    "HANDLE",
    "GDIOBJ",
};

std::ostream& operator<<(std::ostream& o, const suneido_language_jsdi_type_BasicType& e)
{
    assert(0 <= e && e < 9);
    o << suneido_language_jsdi_type_BasicType__NAME[e]
      << '<' << static_cast<int>(e) << '>';
    return o;
}

// [END:GENERATED CODE]

} // namespace jsdi
