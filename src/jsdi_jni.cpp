//==============================================================================
// file: jsdi_jni.cpp
// auth: Victor Schappert
// date: 20130618
// desc: JVM's interface, via JNI, into the JSDI DLL
//==============================================================================

#include "jsdi_windows.h"
#include "jni_exception.h"
#include "jni_util.h"
#include "java_enum.h"
#include "global_refs.h"
#include "basic_type.h"

#include <iostream>
#include <cassert>

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
  (JNIEnv * env, jclass)
{
    jni_utf16_ostream o(env);
    o << u"todo: make when() result"; // TODO: make when() result
    return o.jstr();
}

//==============================================================================
//                JAVA CLASS: suneido.language.jsdi.type.Type
//==============================================================================

#include "gen/suneido_language_jsdi_type_Type.h"
    // This #include isn't strictly necessary -- the only caller of these
    // functions is the JVM. However, it is useful to have the generated code
    // around.

/*
 * Class:     suneido_language_jsdi_type_Type
 * Method:    toStringNative
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_suneido_language_jsdi_type_Type_toStringNative
  (JNIEnv * env, jclass, jlong jsdiHandle)
{
    assert(jsdiHandle || !"Can't call toStringNative without a JSDI handle");
    const type_descriptor * p =
        reinterpret_cast<const type_descriptor *>(jsdiHandle);
    assert(p->is_valid());
    jni_utf16_ostream o(env);
    o << u"TODO: delete this function and cleanup ostreaming";
    return o.jstr();
}

/*
 * Class:     suneido_language_jsdi_type_Type
 * Method:    sizeOf
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_suneido_language_jsdi_type_Type_sizeOf
  (JNIEnv * env, jclass, jlong jsdiHandle)
{
    assert(jsdiHandle || !"Can't call sizeOf without a JSDI handle");
    const type_descriptor * p =
        reinterpret_cast<const type_descriptor *>(jsdiHandle);
    assert(p->is_valid());
    return static_cast<jint>(p->type_size());
}

//==============================================================================
//             JAVA CLASS: suneido.language.jsdi.dll.DllFactory
//==============================================================================

/*
 * Class:     suneido_language_jsdi_dll_DllFactory
 * Method:    loadLibrary
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_dll_DllFactory_loadLibrary
  (JNIEnv * env, jclass, jstring libraryName)
{
    jni_utf16_string_region libraryName_(env, libraryName);
    HMODULE result = LoadLibraryW(libraryName_.wstr());
    return reinterpret_cast<jlong>(result);
}

/*
 * Class:     suneido_language_jsdi_dll_DllFactory
 * Method:    freeLibrary
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_suneido_language_jsdi_dll_DllFactory_freeLibrary
  (JNIEnv * env, jclass, jlong hModule)
{
    FreeLibrary(reinterpret_cast<HMODULE>(hModule));
}

/*
 * Class:     suneido_language_jsdi_dll_DllFactory
 * Method:    getProcAddress
 * Signature: (JLjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_suneido_language_jsdi_dll_DllFactory_getProcAddress
  (JNIEnv * env, jclass, jlong hModule, jstring procName)
{
    jni_utf8_string_region procName_(env, procName);
    FARPROC result = GetProcAddress(reinterpret_cast<HMODULE>(hModule),
                                    procName_.str());
        // NOTE: There is no GetProcAddressW... GetProcAddress() only accepts
        //       ANSI strings.
    return reinterpret_cast<jlong>(result);
}

} // extern "C"
