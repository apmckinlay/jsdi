#ifndef __INCLUDED_GLOBAL_REFS_H__
#define __INCLUDED_GLOBAL_REFS_H__

/**
 * \file global_refs.h
 * \author Victor Schappert
 * \since 20130624
 * \brief Global references to Java classes, objects, and members which remain
 *        valid between JNI invocations.
 * \see java_enum.h
 *
 * This file contains some automatically-generated code.
 */

#include "jni_exception.h"

#include <jni.h>

namespace jsdi {

/**
 * \brief Contains gobal references to Java classes. <em>Must be properly
 *        initialized</em>!
 * \author Victor Schappert
 * \since 20130624
 *
 * \attention
 * Must be properly initialized &mdash;<em> once, and once only</em> &mdash; by
 * calling #init(JNIEnv *). Use only via the #ptr pointer.
 *
 * \internal
 * \par
 * This class is set up using private members to \em contain the global
 * references and public inline functions to \em return the global references.
 * The reason for inserting the inline functions getter functions between the
 * private members and clients of this class is to facilitate static analysis
 * and determining whether a particular global reference is unused and should be
 * removed.
 *
 * \par
 * The idea is that all of the member variables are assigned to in the
 * init(JNIEnv *) member function so it will appear to a naive static analysis
 * tool that the members are being assigned to and it may not complain if the
 * member is never used anywhere else in the program. But the only way a client
 * can read the member variable value is by calling a public member function
 * and none of the public member functions are referred to in this translation
 * unit. Thus a static analysis tool should be able to determine whether any
 * public member functions are unused. You can then use that information to
 * eliminate unused global references.
 *
 * \par
 * \em eg <dfn>cppcheck --enable=unusedFunction</dfn>
 * \endinternal
 */
struct global_refs
{
    //
    // GLOBAL REFERENCES
    //

    // [BEGIN:GENERATED CODE last updated Tue Aug 06 08:51:49 PDT 2013]
    private:
        jclass java_lang_Object_;
    public:
        jclass java_lang_Object() const
        { return java_lang_Object_; }
        /**<
         * \brief Returns a global reference to the class <dfn>java.lang.Object</dfn>.
         * \return <dfn>java.lang.Object</dfn>
         *
         * Auto-generated by <dfn>suneido.language.jsdi.tools.GenerateGlobalReferences</dfn>.
         */
    private:
        jclass java_lang_Boolean_;
    public:
        jclass java_lang_Boolean() const
        { return java_lang_Boolean_; }
        /**<
         * \brief Returns a global reference to the class <dfn>java.lang.Boolean</dfn>.
         * \return <dfn>java.lang.Boolean</dfn>
         *
         * Auto-generated by <dfn>suneido.language.jsdi.tools.GenerateGlobalReferences</dfn>.
         */
    private:
        jfieldID java_lang_Boolean__f_TRUE_;
    public:
        jfieldID java_lang_Boolean__f_TRUE() const
        { return java_lang_Boolean__f_TRUE_; }
        /**<
         * \brief Returns a global reference to the static field <dfn>public static final java.lang.Boolean java.lang.Boolean.TRUE</dfn>.
         * \return <dfn>public static final java.lang.Boolean java.lang.Boolean.TRUE</dfn>
         * \see jclass java_lang_Boolean() const
         *
         * Auto-generated by <dfn>suneido.language.jsdi.tools.GenerateGlobalReferences</dfn>.
         */
    private:
        jfieldID java_lang_Boolean__f_FALSE_;
    public:
        jfieldID java_lang_Boolean__f_FALSE() const
        { return java_lang_Boolean__f_FALSE_; }
        /**<
         * \brief Returns a global reference to the static field <dfn>public static final java.lang.Boolean java.lang.Boolean.FALSE</dfn>.
         * \return <dfn>public static final java.lang.Boolean java.lang.Boolean.FALSE</dfn>
         * \see jclass java_lang_Boolean() const
         *
         * Auto-generated by <dfn>suneido.language.jsdi.tools.GenerateGlobalReferences</dfn>.
         */
    private:
        jclass java_lang_Integer_;
    public:
        jclass java_lang_Integer() const
        { return java_lang_Integer_; }
        /**<
         * \brief Returns a global reference to the class <dfn>java.lang.Integer</dfn>.
         * \return <dfn>java.lang.Integer</dfn>
         *
         * Auto-generated by <dfn>suneido.language.jsdi.tools.GenerateGlobalReferences</dfn>.
         */
    private:
        jmethodID java_lang_Integer__init_;
    public:
        jmethodID java_lang_Integer__init() const
        { return java_lang_Integer__init_; }
        /**<
         * \brief Returns a global reference to the constructor <dfn>public java.lang.Integer(int)</dfn>.
         * \return <dfn>public java.lang.Integer(int)</dfn>
         * \see jclass java_lang_Integer() const
         *
         * Auto-generated by <dfn>suneido.language.jsdi.tools.GenerateGlobalReferences</dfn>.
         */
    private:
        jmethodID java_lang_Integer__m_intValue_;
    public:
        jmethodID java_lang_Integer__m_intValue() const
        { return java_lang_Integer__m_intValue_; }
        /**<
         * \brief Returns a global reference to the instance method <dfn>public int java.lang.Integer.intValue()</dfn>.
         * \return <dfn>public int java.lang.Integer.intValue()</dfn>
         * \see jclass java_lang_Integer() const
         *
         * Auto-generated by <dfn>suneido.language.jsdi.tools.GenerateGlobalReferences</dfn>.
         */
    private:
        jclass java_lang_Enum_;
    public:
        jclass java_lang_Enum() const
        { return java_lang_Enum_; }
        /**<
         * \brief Returns a global reference to the class <dfn>java.lang.Enum</dfn>.
         * \return <dfn>java.lang.Enum</dfn>
         *
         * Auto-generated by <dfn>suneido.language.jsdi.tools.GenerateGlobalReferences</dfn>.
         */
    private:
        jmethodID java_lang_Enum__m_ordinal_;
    public:
        jmethodID java_lang_Enum__m_ordinal() const
        { return java_lang_Enum__m_ordinal_; }
        /**<
         * \brief Returns a global reference to the instance method <dfn>public final int java.lang.Enum.ordinal()</dfn>.
         * \return <dfn>public final int java.lang.Enum.ordinal()</dfn>
         * \see jclass java_lang_Enum() const
         *
         * Auto-generated by <dfn>suneido.language.jsdi.tools.GenerateGlobalReferences</dfn>.
         */
    private:
        jclass byte_ARRAY_;
    public:
        jclass byte_ARRAY() const
        { return byte_ARRAY_; }
        /**<
         * \brief Returns a global reference to the class <dfn>[B</dfn>.
         * \return <dfn>[B</dfn>
         *
         * Auto-generated by <dfn>suneido.language.jsdi.tools.GenerateGlobalReferences</dfn>.
         */
    private:
        jclass suneido_language_jsdi_type_Callback_;
    public:
        jclass suneido_language_jsdi_type_Callback() const
        { return suneido_language_jsdi_type_Callback_; }
        /**<
         * \brief Returns a global reference to the class <dfn>suneido.language.jsdi.type.Callback</dfn>.
         * \return <dfn>suneido.language.jsdi.type.Callback</dfn>
         *
         * Auto-generated by <dfn>suneido.language.jsdi.tools.GenerateGlobalReferences</dfn>.
         */
    // [END:GENERATED CODE]

    //
    // NON-GENERATED GLOBAL REFERENCES
    //

    private:
        jobject TRUE_object_;
        jobject FALSE_object_;
        jobject ZERO_object_;
    public:
        /**
         * \brief Returns a global reference corresponding to
         *        <dfn>java.lang.Boolean.TRUE</dfn>.
         * \return <dfn>java.lang.Boolean.TRUE</dfn>
         * \see #FALSE_object() const
         * \see #ZERO_object() const
         */
        jobject TRUE_object() const
        { return TRUE_object_; }
        /**
         * \brief Returns a global reference corresponding to
         *        <dfn>java.lang.Boolean.FALSE</dfn>.
         * \return <dfn>java.lang.Boolean.FALSE</dfn>
         * \see #TRUE_object() const
         * \see #ZERO_object() const
         */
        jobject FALSE_object() const
        { return FALSE_object_; }
        /**
         * \brief Returns a global reference to a Java <dfn>Integer</dfn>
         *        which contains the value 0.
         * \return A <dfn>java.lang.Integer</dfn> equal to zero
         * \see #TRUE_object() const
         * \see #FALSE_object() const
         */
        jobject ZERO_object() const
        { return ZERO_object_; }

    //
    // INITIALIZATION
    //

    /**
     * \brief Initializes #GLOBAL_REFS.
     * \param env Valid pointer to the JNI environment
     * \throws jni_exception If a JNI error occurs initializing any global
     *         reference.
     *
     * Call this function once, and once only, before the first use of the
     * global references. After initialization, the #GLOBAL_REFS pointer may
     * validly be used to access global references.
     */
    static void init(JNIEnv * env) throw(jni_exception);
};


/**
 * \brief Global pointer to the global global_refs structure.
 *
 * Do not use until global_refs::init(JNIEnv *) has been called!
 */
extern global_refs const * const GLOBAL_REFS;

} // namespace jsdi

#endif // __INCLUDED_GLOBAL_REFS_H__
