//==============================================================================
// file: jsdi_jni.cpp
// auth: Victor Schappert
// date: 20130618
// desc: JVM's interface, via JNI, into the JSDI DLL
//==============================================================================

#include "jsdi_windows.h"
#include "jni_exception.h"
#include "java_enum.h"
#include "global_refs.h"
#include "basic_type.h"

#include <iostream>
#include <iomanip>

extern "C" {

using namespace jsdi;

//==============================================================================
//                  JAVA CLASS: suneido.language.jsdi.JSDI
//==============================================================================

#include "gen/suneido_language_jsdi_JSDI.h"
    // This #include isn't strictly necessary -- the only caller of these
    // functions is the JVM. However, it is useful to have the generated code
    // around.

JNIEXPORT void JNICALL Java_suneido_language_jsdi_JSDI_init
  (JNIEnv * env, jclass)
{
    global_refs::init(env);
}

JNIEXPORT jstring JNICALL Java_suneido_language_jsdi_JSDI_when
  (JNIEnv *, jclass)
{
    // TODO
}

//==============================================================================
//            JAVA CLASS: suneido.language.jsdi.type.TypeFactory
//==============================================================================

#include "gen/suneido_language_jsdi_type_TypeFactory.h"
    // This #include isn't strictly necessary -- the only caller of these
    // functions is the JVM. However, it is useful to have the generated code
    // around.

JNIEXPORT void JNICALL Java_suneido_language_jsdi_type_TypeFactory_releaseHandle
  (JNIEnv *, jclass, jlong);

JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_type_TypeFactory_getBasicValueHandle
  (JNIEnv * env, jclass, jobject basic_type)
{
    const suneido_language_jsdi_type_BasicType basic_type_(
        jni_enum_to_cpp<suneido_language_jsdi_type_BasicType>(
            env,
            global_refs::ptr->suneido_language_jsdi_type_BasicType(),
            basic_type)
    );
    const basic_value& result(basic_value::instance(basic_type_));
    // The value returned is a pointer to a persistent type. It is never freed.
    return reinterpret_cast<jlong>(&result);
}

JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_type_TypeFactory_getBasicPointerHandle
  (JNIEnv * env, jclass, jobject basic_type)
{
    const suneido_language_jsdi_type_BasicType basic_type_(
        jni_enum_to_cpp<suneido_language_jsdi_type_BasicType>(
            env,
            global_refs::ptr->suneido_language_jsdi_type_BasicType(),
            basic_type)
    );
    const basic_pointer * result(basic_value::instance(basic_type_).pointer_type());
    // The value returned is a pointer to a persistent type. It is never freed.
    return reinterpret_cast<jlong>(result);

}

JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_type_TypeFactory_makeBasicArrayHandle
  (JNIEnv * env, jclass, jobject basic_type, jint num_elems)
{
    const suneido_language_jsdi_type_BasicType basic_type_(
        jni_enum_to_cpp<suneido_language_jsdi_type_BasicType>(
            env,
            global_refs::ptr->suneido_language_jsdi_type_BasicType(),
            basic_type)
    );
    const basic_value& underlying(basic_value::instance(basic_type_));
    // This pointer is now owned by the Java-side. It is the responsibility of
    // the owning Java object to free it when the Java object becomes garbage.
    return reinterpret_cast<jlong>(new basic_array(&underlying, num_elems));
}

} // extern "C"
