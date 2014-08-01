/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: global_refs.cpp
// auth: Victor Schappert
// date: 20130624
// desc: Global references to Java classes and objects which remain valid
//       between JNI invocations (implementation file).
//==============================================================================

#include "global_refs.h"

#include "jni_util.h"

#include <cassert>

namespace jsdi {

//==============================================================================
//                               [ INTERNALS ]
//==============================================================================

static jobject globalize(JNIEnv * env, jobject object, const char * name)
{
    jobject global = env->NewGlobalRef(object);
    if (! global)
    {
        std::ostringstream() << __FUNCTION__ << "() failed to get a "
                                "global reference to '" << object
                             << "('" << name << "')"
                             << throw_cpp<jni_exception, JNIEnv *>(env);
    }
    return global;
}

static jclass globalize(JNIEnv * env, jclass clazz, const char * name)
{
    return static_cast<jclass>(
        globalize(env, static_cast<jobject>(clazz), name));
}

static jclass get_global_class_ref(JNIEnv * env, const char * class_name)
{   // NOTE: FindClass will force a load, and will return 0 if class not found
    jni_auto_local<jclass> clazz(env, class_name);
    if (!clazz)
    {
        std::ostringstream() << __FUNCTION__ << "() failed to load class name '"
                             << class_name << '\''
                             << throw_cpp<jni_exception, JNIEnv *>(env);
    }
    return globalize(env, clazz, class_name);
}

static jmethodID get_method_id(JNIEnv * env, jclass clazz,
                               const char * method_name, const char * signature)
{
    jmethodID method_id = env->GetMethodID(clazz, method_name, signature);
    if (! method_id)
    {
        std::ostringstream() << __FUNCTION__ << "() failed to get a method ID "
                                " for class " << clazz << ", method name '"
                             << method_name << "', signature=" << signature
                             << throw_cpp<jni_exception, JNIEnv *>(env);
    }
    return method_id;
}

static jmethodID get_static_method_id(JNIEnv * env, jclass clazz,
                                      const char * method_name,
                                      const char * signature)
{
    jmethodID method_id = env->GetStaticMethodID(clazz, method_name, signature);
    if (! method_id)
    {
        std::ostringstream() << __FUNCTION__ << "() failed to get a static "
                                " method ID for class " << clazz << ", method "
                                "name '" << method_name << "', signature="
                             << signature
                             << throw_cpp<jni_exception, JNIEnv *>(env);
    }
    return method_id;
}

static jfieldID get_field_id(JNIEnv * env, jclass clazz,
                             const char * field_name, const char * signature)
{
    jfieldID field_id = env->GetFieldID(clazz, field_name, signature);
    if (! field_id)
    {
        std::ostringstream() << __FUNCTION__ << "() failed to get a field ID "
                                "for class " << clazz << ", field name '"
                             << field_name << "', signature=" << signature
                             << throw_cpp<jni_exception, JNIEnv *>(env);
    }
    return field_id;
}

static jfieldID get_static_field_id(JNIEnv * env, jclass clazz,
                                    const char * field_name,
                                    const char * signature)
{
    jfieldID field_id = env->GetStaticFieldID(clazz, field_name, signature);
    if (! field_id)
    {
        std::ostringstream() << __FUNCTION__ << "() failed to get a static "
                                "field ID for class " << clazz << ", field "
                                "name '" << field_name << "', signature="
                             << signature
                             << throw_cpp<jni_exception, JNIEnv *>(env);
    }
    return field_id;
}

static jobject get_static_field_value_object(JNIEnv * env, jclass clazz,
                                             jfieldID field_id,
                                             const char * name)
{
    jni_auto_local<jobject> value(env, clazz, field_id);
    if (! value)
    {
        std::ostringstream() << __FUNCTION__ << "() got null result for class "
                             << clazz << ", field id '" << field_id
                             << throw_cpp<jni_exception, JNIEnv *>(env);
    }
    return globalize(env, value, name);
}

//==============================================================================
//                            struct global_refs
//==============================================================================

global_refs global_refs_;
global_refs const * const GLOBAL_REFS(&global_refs_);

// NOTE: This function MUST be called once, and MAY ONLY be called once, prior
//       to the use of ANY JSDI functionality by ANY Java thread via JNI. This
//       can be easily managed by ensuring that this function is triggered by
//       the static constructor of a single 'factory' class on the Java side
//       and ensuring that only that factory class is able construct other JSDI
//       types.
void global_refs::init(JNIEnv * env)
{
    //
    // Load the global references
    //

    global_refs * g = &global_refs_;

    // [BEGIN:GENERATED CODE last updated Fri Aug 01 03:45:11 PDT 2014]
    g->java_lang_Object_ = get_global_class_ref(env, "java/lang/Object");
    g->java_lang_Object__m_toString_ = get_method_id(env, g->java_lang_Object_, "toString", "()Ljava/lang/String;");
    g->java_lang_Boolean_ = get_global_class_ref(env, "java/lang/Boolean");
    g->java_lang_Boolean__m_booleanValue_ = get_method_id(env, g->java_lang_Boolean_, "booleanValue", "()Z");
    g->java_lang_Boolean__f_TRUE_ = get_static_field_id(env, g->java_lang_Boolean_, "TRUE", "Ljava/lang/Boolean;");
    g->java_lang_Boolean__f_FALSE_ = get_static_field_id(env, g->java_lang_Boolean_, "FALSE", "Ljava/lang/Boolean;");
    g->java_lang_Number_ = get_global_class_ref(env, "java/lang/Number");
    g->java_lang_Integer_ = get_global_class_ref(env, "java/lang/Integer");
    g->java_lang_Integer__init_ = get_method_id(env, g->java_lang_Integer_, "<init>", "(I)V");
    g->java_lang_Integer__m_intValue_ = get_method_id(env, g->java_lang_Integer_, "intValue", "()I");
    g->java_lang_Long_ = get_global_class_ref(env, "java/lang/Long");
    g->java_lang_Long__init_ = get_method_id(env, g->java_lang_Long_, "<init>", "(J)V");
    g->java_lang_Long__m_longValue_ = get_method_id(env, g->java_lang_Long_, "longValue", "()J");
    g->java_math_BigDecimal_ = get_global_class_ref(env, "java/math/BigDecimal");
    g->java_math_BigDecimal__init_ = get_method_id(env, g->java_math_BigDecimal_, "<init>", "(DLjava/math/MathContext;)V");
    g->java_math_BigDecimal__m_doubleValue_ = get_method_id(env, g->java_math_BigDecimal_, "doubleValue", "()D");
    g->java_lang_CharSequence_ = get_global_class_ref(env, "java/lang/CharSequence");
    g->java_lang_Enum_ = get_global_class_ref(env, "java/lang/Enum");
    g->java_lang_Enum__m_ordinal_ = get_method_id(env, g->java_lang_Enum_, "ordinal", "()I");
    g->byte_ARRAY_ = get_global_class_ref(env, "[B");
    g->java_util_Date_ = get_global_class_ref(env, "java/util/Date");
    g->java_util_Date__init_ = get_method_id(env, g->java_util_Date_, "<init>", "(J)V");
    g->java_util_Date__m_getTime_ = get_method_id(env, g->java_util_Date_, "getTime", "()J");
    g->suneido_jsdi_LogLevel_ = get_global_class_ref(env, "suneido/jsdi/LogLevel");
    g->suneido_jsdi_LogLevel__m_values_ = get_static_method_id(env, g->suneido_jsdi_LogLevel_, "values", "()[Lsuneido/jsdi/LogLevel;");
    g->suneido_jsdi_type_Callback_ = get_global_class_ref(env, "suneido/jsdi/type/Callback");
    g->suneido_jsdi_type_Callback__m_invoke_ = get_method_id(env, g->suneido_jsdi_type_Callback_, "invoke", "(Lsuneido/SuValue;[J)J");
    g->suneido_jsdi_type_Callback__m_invokeVariableIndirect_ = get_method_id(env, g->suneido_jsdi_type_Callback_, "invokeVariableIndirect", "(Lsuneido/SuValue;[J[Ljava/lang/Object;)J");
    g->suneido_jsdi_type_Callback__m_invoke0_ = get_static_method_id(env, g->suneido_jsdi_type_Callback_, "invoke0", "(Lsuneido/SuValue;)J");
    g->suneido_jsdi_type_Callback__m_invoke1_ = get_method_id(env, g->suneido_jsdi_type_Callback_, "invoke1", "(Lsuneido/SuValue;J)J");
    g->suneido_jsdi_type_Callback__m_invoke2_ = get_method_id(env, g->suneido_jsdi_type_Callback_, "invoke2", "(Lsuneido/SuValue;JJ)J");
    g->suneido_jsdi_type_Callback__m_invoke3_ = get_method_id(env, g->suneido_jsdi_type_Callback_, "invoke3", "(Lsuneido/SuValue;JJJ)J");
    g->suneido_jsdi_type_Callback__m_invoke4_ = get_method_id(env, g->suneido_jsdi_type_Callback_, "invoke4", "(Lsuneido/SuValue;JJJJ)J");
    g->suneido_jsdi_com_COMobject_ = get_global_class_ref(env, "suneido/jsdi/com/COMobject");
    g->suneido_jsdi_com_COMobject__init_ = get_method_id(env, g->suneido_jsdi_com_COMobject_, "<init>", "(Ljava/lang/String;JZ)V");
    g->suneido_jsdi_com_COMobject__m_isDispatch_ = get_method_id(env, g->suneido_jsdi_com_COMobject_, "isDispatch", "()Z");
    g->suneido_jsdi_com_COMobject__m_verifyNotReleased_ = get_method_id(env, g->suneido_jsdi_com_COMobject_, "verifyNotReleased", "()V");
    g->suneido_jsdi_com_COMobject__f_ptr_ = get_field_id(env, g->suneido_jsdi_com_COMobject_, "ptr", "J");
    g->suneido_jsdi_com_COMException_ = get_global_class_ref(env, "suneido/jsdi/com/COMException");
    g->suneido_jsdi_com_COMException__init_ = get_method_id(env, g->suneido_jsdi_com_COMException_, "<init>", "(Ljava/lang/String;)V");
    g->suneido_jsdi_suneido_protocol_InternetProtocol_ = get_global_class_ref(env, "suneido/jsdi/suneido_protocol/InternetProtocol");
    g->suneido_jsdi_suneido_protocol_InternetProtocol__m_start_ = get_static_method_id(env, g->suneido_jsdi_suneido_protocol_InternetProtocol_, "start", "(Ljava/lang/String;)[B");
    g->suneido_language_Numbers_ = get_global_class_ref(env, "suneido/language/Numbers");
    g->suneido_language_Numbers__m_narrow_ = get_static_method_id(env, g->suneido_language_Numbers_, "narrow", "(Ljava/lang/Number;)Ljava/lang/Number;");
    g->suneido_language_Numbers__f_MC_ = get_static_field_id(env, g->suneido_language_Numbers_, "MC", "Ljava/math/MathContext;");
    // [END:GENERATED CODE]

    g->TRUE_object_ = get_static_field_value_object(
        env, g->java_lang_Boolean_, g->java_lang_Boolean__f_TRUE_, "TRUE");
    g->FALSE_object_ = get_static_field_value_object(
        env, g->java_lang_Boolean_, g->java_lang_Boolean__f_FALSE_, "FALSE");
    // TODO: The zero object should be a Long and all numbers passed over JNI
    //       between Java and C++ should also be Longs/longs in order to
    //       simplify (A) the jSuneido number system and (B) the C++ code, by
    //       reducing the number of global references required -- i.e. we can
    //       get rid of java_lang_Integer.
    jni_auto_local<jobject> zero(
        env,
        env->NewObject(g->java_lang_Integer_, g->java_lang_Integer__init_, 0));
    g->ZERO_object_ = globalize(env, zero, "zero");
    const jchar empty_chars[1] = { 0 };
    jni_auto_local<jstring> empty(env, empty_chars, 0);
    g->EMPTY_STRING_object_ = static_cast<jstring>(globalize(
        env, static_cast<jstring>(empty), "empty string"));
}

} // namespace jsdi
