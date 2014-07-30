/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

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
EnumType jni_enum_to_cpp(JNIEnv * env, jclass clazz, jobject e);

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
EnumType ordinal_enum_to_cpp(int e);

// [BEGIN:GENERATED CODE last updated Tue Jul 29 14:18:10 PDT 2014]
/**
 * \brief C++ enumeration corresponding to the Java enumeration <code>suneido.jsdi.marshall.VariableIndirectInstruction</code>.
 * \author GenerateSharedEnums
 *
 * Auto-generated by <code>suneido.jsdi.tools.GenerateSharedEnums</code>.
 */
enum suneido_jsdi_marshall_VariableIndirectInstruction
{
    NO_ACTION,
    RETURN_JAVA_STRING,
    RETURN_RESOURCE,
};

/** \cond internal */
template <>
suneido_jsdi_marshall_VariableIndirectInstruction jni_enum_to_cpp(JNIEnv *, jclass, jobject);
/** \endcond */

/** \cond internal */
template <>
suneido_jsdi_marshall_VariableIndirectInstruction ordinal_enum_to_cpp(int);
/** \endcond */

/**
 * \brief Stream insertion operator for \link suneido_jsdi_marshall_VariableIndirectInstruction\endlink.
 * \author GenerateSharedEnums
 * \param o Stream to insert into
 * \param e Enumerator to insert
 *
 * Auto-generated by <code>suneido.jsdi.tools.GenerateSharedEnums</code>.
 */
std::ostream& operator<<(std::ostream& o, const suneido_jsdi_marshall_VariableIndirectInstruction& e);

// [END:GENERATED CODE]

} // namespace jsdi

#endif // __INCLUDED_JAVA_ENUM_H__
