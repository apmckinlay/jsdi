#ifndef __INCLUDED_JAVA_ENUM_H___
#define __INCLUDED_JAVA_ENUM_H___

/**
 * \file java_enum.h
 * \author Victor Schappert
 * \since 20130628
 * \brief C++ declarations for enumerations whose primary definition is in Java.
 * \see global_refs.h
 *
 * This file is predominantly auto-generated.
 */

#include "jni_exception.h"

#include <iosfwd>

#include <jni.h>

namespace jsdi {

/**
 * \brief Converts a JNI <dfn>jobject</dfn> reference to a Java enumerator into
 *        the corresponding C++ enumerator.
 * \tparam EnumType An enumeration which is defined in
 *         \link java_enum.h \endlink.
 * \param env Valid pointer to the JNI environment
 * \param clazz Reference to Java enum class (\em ie an instance of
 *        <dfn>Class<E extends Enum></dfn>)
 * \param e Enumerator to convert (reference to instance of <dfn>clazz</dfn>)
 * \throw jni_exception If an error occurs getting the ordinal value of
 *        <dfn>e</dfn> from the JNI environment
 * \author Victor Schappert
 */
template <typename EnumType>
EnumType jni_enum_to_cpp(JNIEnv * env, jclass clazz, jobject e)
    throw (jni_exception);

/**
 * \brief Converts an integer value to the corresponding C++ enumerator for the
 *        given enumeration type.
 * \tparam EnumType An enumeration which is defined in
 *         \link java_enum.h \endlink
 * \param e Ordinal value of the enumerator desired
 * \throw jni_exception If the ordinal <dfn>e</dfn> does not correspond to any
 *        members of the enumeration <dfn>EnumType</dfn>.
 * \author Victor Schappert
 */
template <typename EnumType>
EnumType ordinal_enum_to_cpp(int e) throw (jni_exception);

// [BEGIN:GENERATED CODE last updated Thu Aug 01 10:58:02 PDT 2013]
/**
 * \brief C++ enumeration corresponding to the Java enumeration <dfn>suneido.language.jsdi.VariableIndirectInstruction</dfn>.
 * \author GenerateSharedEnums
 *
 * Auto-generated by <dfn>suneido.language.jsdi.tools.GenerateSharedEnums</dfn>.
 */
enum suneido_language_jsdi_VariableIndirectInstruction
{
    NO_ACTION,
    RETURN_JAVA_STRING,
    RETURN_RESOURCE,
};

/** \cond internal */
template <>
suneido_language_jsdi_VariableIndirectInstruction jni_enum_to_cpp(JNIEnv *, jclass, jobject) throw(jni_exception);
/** \endcond */

/** \cond internal */
template <>
suneido_language_jsdi_VariableIndirectInstruction ordinal_enum_to_cpp(int) throw(jni_exception);
/** \endcond */

/**
 * \brief Stream insertion operator for \link suneido_language_jsdi_VariableIndirectInstruction\endlink.
 * \author GenerateSharedEnums
 * \param o Stream to insert into
 * \param e Enumerator to insert
 *
 * Auto-generated by <dfn>suneido.language.jsdi.tools.GenerateSharedEnums</dfn>.
 */
std::ostream& operator<<(std::ostream& o, const suneido_language_jsdi_VariableIndirectInstruction& e);

// [END:GENERATED CODE]

} // namespace jsdi

#endif // __INCLUDED_JAVA_ENUM_H__
