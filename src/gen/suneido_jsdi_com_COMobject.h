/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class suneido_jsdi_com_COMobject */

#ifndef _Included_suneido_jsdi_com_COMobject
#define _Included_suneido_jsdi_com_COMobject
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     suneido_jsdi_com_COMobject
 * Method:    queryIDispatchAndProgId
 * Signature: (J[Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_com_COMobject_queryIDispatchAndProgId
  (JNIEnv *, jclass, jlong, jobjectArray);

/*
 * Class:     suneido_jsdi_com_COMobject
 * Method:    coCreateFromProgId
 * Signature: (Ljava/lang/String;[J)Z
 */
JNIEXPORT jboolean JNICALL Java_suneido_jsdi_com_COMobject_coCreateFromProgId
  (JNIEnv *, jclass, jstring, jlongArray);

/*
 * Class:     suneido_jsdi_com_COMobject
 * Method:    release
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_suneido_jsdi_com_COMobject_release
  (JNIEnv *, jclass, jlong, jlong);

/*
 * Class:     suneido_jsdi_com_COMobject
 * Method:    getPropertyByName
 * Signature: (JLjava/lang/String;[I)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_suneido_jsdi_com_COMobject_getPropertyByName
  (JNIEnv *, jclass, jlong, jstring, jintArray);

/*
 * Class:     suneido_jsdi_com_COMobject
 * Method:    getPropertyByDispId
 * Signature: (JI)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_suneido_jsdi_com_COMobject_getPropertyByDispId
  (JNIEnv *, jclass, jlong, jint);

/*
 * Class:     suneido_jsdi_com_COMobject
 * Method:    putPropertyByName
 * Signature: (JLjava/lang/String;Ljava/lang/Object;)I
 */
JNIEXPORT jint JNICALL Java_suneido_jsdi_com_COMobject_putPropertyByName
  (JNIEnv *, jclass, jlong, jstring, jobject);

/*
 * Class:     suneido_jsdi_com_COMobject
 * Method:    putPropertyByDispId
 * Signature: (JILjava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_suneido_jsdi_com_COMobject_putPropertyByDispId
  (JNIEnv *, jclass, jlong, jint, jobject);

/*
 * Class:     suneido_jsdi_com_COMobject
 * Method:    callMethodByName
 * Signature: (JLjava/lang/String;[Ljava/lang/Object;[I)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_suneido_jsdi_com_COMobject_callMethodByName
  (JNIEnv *, jclass, jlong, jstring, jobjectArray, jintArray);

/*
 * Class:     suneido_jsdi_com_COMobject
 * Method:    callMethodByDispId
 * Signature: (JI[Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_suneido_jsdi_com_COMobject_callMethodByDispId
  (JNIEnv *, jclass, jlong, jint, jobjectArray);

#ifdef __cplusplus
}
#endif
#endif
