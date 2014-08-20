/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: _jni_interface.cpp
// auth: Victor Schappert
// date: 20130618
// desc: JVM's interface, via JNI, into the JSDI DLL.
//       (This translation unit contains general functions available regardless
//       of what ABI the DLL is compiled for.)
//==============================================================================

#include "com.h"
#include "global_refs.h"
#include "jni_exception.h"
#include "jni_util.h"
#include "jsdi_windows.h"
#include "log.h"
#include "marshalling.h"
#include "seh.h"
#include "suneido_protocol.h"
#include "version.h"

#include <cassert>

using namespace jsdi;

//==============================================================================
//                                INTERNALS
//==============================================================================

namespace {

log_level log_level_java_to_cpp(JNIEnv * env, jobject java_log_level)
{
    assert(java_log_level);
    auto level = java_enum::jni_enum_to_cpp<java_enum::suneido_jsdi_LogLevel>(
        env, java_log_level);
    switch (level)
    {
        case java_enum::suneido_jsdi_LogLevel::NONE:
            return log_level::NONE;
        case java_enum::suneido_jsdi_LogLevel::FATAL:
            return log_level::FATAL;
        case java_enum::suneido_jsdi_LogLevel::ERROR:
            return log_level::ERROR;
        case java_enum::suneido_jsdi_LogLevel::WARN:
            return log_level::WARN;
        case java_enum::suneido_jsdi_LogLevel::INFO:
            return log_level::INFO;
        case java_enum::suneido_jsdi_LogLevel::DEBUG:
            return log_level::DEBUG;
        case java_enum::suneido_jsdi_LogLevel::TRACE:
            return log_level::TRACE;
        default:
            std::ostringstream() << "no conversion for " << level
                                 << " from Java to C++"
                                 << throw_cpp<jni_exception, bool>(false);
            return log_level::NONE; // control never passes here
    }
}

jobject log_level_cpp_to_java(JNIEnv * env, log_level cpp_level)
{
    auto level = java_enum::suneido_jsdi_LogLevel::NONE;
    switch (cpp_level)
    {
        case log_level::NONE:
            break;
        case log_level::FATAL:
            level = java_enum::suneido_jsdi_LogLevel::FATAL;
            break;
        case log_level::ERROR:
            level = java_enum::suneido_jsdi_LogLevel::ERROR;
            break;
        case log_level::WARN:
            level = java_enum::suneido_jsdi_LogLevel::WARN;
            break;
        case log_level::INFO:
            level = java_enum::suneido_jsdi_LogLevel::INFO;
            break;
        case log_level::DEBUG:
            level = java_enum::suneido_jsdi_LogLevel::DEBUG;
            break;
        case log_level::TRACE:
            level = java_enum::suneido_jsdi_LogLevel::TRACE;
            break;
        default:
            std::ostringstream() << "no conversion for " << cpp_level
                                 << " from C++ to Java"
                                 << throw_cpp<jni_exception, bool>(false);
    }
    return java_enum::cpp_to_jni_enum(env, level);
}

void check_array_atleast(jsize size, const char * array_name, JNIEnv * env,
                         jarray array)
{
    if (env->GetArrayLength(array) < size)
    {
        std::ostringstream() << array_name << " must have length at least "
                             << size << throw_cpp<std::runtime_error>();
    }
}

void check_array_atleast_1(const char * array_name, JNIEnv * env, jarray array)
{ check_array_atleast(1, array_name, env, array); }

inline const char * struct_get_ptr(jlong struct_addr)
{
    assert(struct_addr || !"can't copy out a NULL pointer");
    return reinterpret_cast<const char *>(struct_addr);
}

inline void struct_check_size(jint size_direct)
{ assert(0 < size_direct || !"structure must have positive size"); }

void struct_unmarshall_direct_seh(void * dest, void const * src,
                                         size_t size_direct)
{
    SEH_CONVERT_TO_CPP_BEGIN
    std::memcpy(dest, src, size_direct);
    SEH_CONVERT_TO_CPP_END
}

} // anonymous namespace

extern "C" {

//==============================================================================
//                      JAVA CLASS: suneido.jsdi.JSDI
//==============================================================================

#include "gen/suneido_jsdi_JSDI.h"

JNIEXPORT void JNICALL Java_suneido_jsdi_JSDI_init
  (JNIEnv * env, jclass)
{
    JNI_EXCEPTION_SAFE_CPP_BEGIN;
    log_manager::instance().set_path(std::string("jsdi.log"));
    LOG_TRACE("Initializing JSDI library built " << version::BUILD_DATE
                                                 << " for " << version::PLATFORM);
    JavaVM * vm(nullptr);
    if (JNI_OK == env->GetJavaVM(&vm))
    {
        global_refs::init(env);
        suneido_protocol::register_handler(vm);
        // TODO: presently no-one is calling suneido_protocol::unregister_handler()
    }
    else throw std::runtime_error("Failed to obtain JavaVM in JSDI.init()");
    LOG_TRACE("JSDI library initialized OK");
    JNI_EXCEPTION_SAFE_CPP_END(env);
}

JNIEXPORT jstring JNICALL Java_suneido_jsdi_JSDI_when
  (JNIEnv * env, jclass)
{
    jstring result(0);
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    jni_utf16_ostream o(env);
    o << version::BUILD_DATE << " (" << version::PLATFORM;
    if (! version::IS_RELEASE) o << " debug";
    o << ')';
    result = o.jstr();
    JNI_EXCEPTION_SAFE_CPP_END(env);
    return result;
}

/*
 * Class:     suneido_jsdi_JSDI
 * Method:    logLevel
 * Signature: (Lsuneido/jsdi/LogLevel;)Lsuneido/jsdi/LogLevel;
 */
JNIEXPORT jobject JNICALL Java_suneido_jsdi_JSDI_logThreshold
  (JNIEnv * env, jclass, jobject threshold)
{
    jobject result(nullptr);
    JNI_EXCEPTION_SAFE_CPP_BEGIN;
    // Level can be null, which indicates just to return the value.
    if (threshold)
    {
        auto cpp_level = log_level_java_to_cpp(env, threshold);
        log_manager::instance().set_threshold(cpp_level);
        LOG_INFO("logThreshold( " << cpp_level << " ) => "
                                  << log_manager::instance().threshold());
    }
    result = log_level_cpp_to_java(env, log_manager::instance().threshold());
    JNI_EXCEPTION_SAFE_CPP_END(env);
    return result;
}

//==============================================================================
//                    JAVA CLASS: suneido.jsdi.DllFactory
//==============================================================================

#include "gen/suneido_jsdi_DllFactory.h"

/*
 * Class:     suneido_jsdi_DllFactory
 * Method:    loadLibrary
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_DllFactory_loadLibrary
  (JNIEnv * env, jclass, jstring libraryName)
{
    jlong result(0);
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    jni_utf16_string_region libraryName_(env, libraryName);
    HMODULE hmodule = LoadLibraryW(libraryName_.wstr());
    result = reinterpret_cast<jlong>(hmodule);
    LOG_INFO("LoadLibraryW('" << jni_utf8_string_region(env, libraryName)
                              << "') => " << hmodule);
    JNI_EXCEPTION_SAFE_CPP_END(env);
    return result;
}

/*
 * Class:     suneido_jsdi_DllFactory
 * Method:    freeLibrary
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_suneido_jsdi_DllFactory_freeLibrary
  (JNIEnv * env, jclass, jlong hModule)
{
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    HMODULE hmodule = reinterpret_cast<HMODULE>(hModule);
    BOOL result = FreeLibrary(hmodule);
    LOG_INFO("FreeLibrary(" << hmodule << ") => " << result);
    JNI_EXCEPTION_SAFE_CPP_END(env);
}

/*
 * Class:     suneido_jsdi_DllFactory
 * Method:    getProcAddress
 * Signature: (JLjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_DllFactory_getProcAddress
  (JNIEnv * env, jclass, jlong hModule, jstring procName)
{
    jlong result(0);
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    jni_utf8_string_region procName_(env, procName);
    FARPROC addr = GetProcAddress(reinterpret_cast<HMODULE>(hModule),
        procName_.str());
        // NOTE: There is no GetProcAddressW... GetProcAddress() only accepts
        //       ANSI strings.
    result = reinterpret_cast<jlong>(addr);
    LOG_DEBUG("GetProcAddress('" << procName_.str() << "') => " << addr);
    JNI_EXCEPTION_SAFE_CPP_END(env);
    return result;
}

//==============================================================================
//                 JAVA CLASS: suneido.jsdi.type.Structure
//==============================================================================

#include "gen/suneido_jsdi_type_Structure.h"

/*
 * Class:     suneido_jsdi_type_Structure
 * Method:    copyOutDirect
 * Signature: (J[JI)V
 */
JNIEXPORT void JNICALL Java_suneido_jsdi_type_Structure_copyOutDirect(
    JNIEnv * env, jclass, jlong structAddr, jlongArray data, jint sizeDirect)
{
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    LOG_TRACE("structAddr => "   << reinterpret_cast<void *>(structAddr) <<
              ", sizeDirect => " << sizeDirect);
    struct_check_size(sizeDirect);
    auto ptr(struct_get_ptr(structAddr));
    // NOTE: In contrast to most other situations, it is safe to use a primitive
    // critical array here because in a struct copy out, we don't call any other
    // JNI functions (nor is it possible to surreptitiously re-enter the Java
    // world via a callback).
#pragma warning(push) // TODO: remove after http://goo.gl/SvVcbg fixed
#pragma warning(disable:4592)
    jni_critical_array<jlong> data_(env, data, min_whole_words(sizeDirect));
#pragma warning(pop)
    struct_unmarshall_direct_seh(data_.data(), ptr, sizeDirect);
    JNI_EXCEPTION_SAFE_CPP_END(env);
}

/*
 * Class:     suneido_jsdi_type_Structure
 * Method:    copyOutIndirect
 * Signature: (J[JI[I)V
 */
JNIEXPORT void JNICALL Java_suneido_jsdi_type_Structure_copyOutIndirect(
    JNIEnv * env, jclass, jlong structAddr, jlongArray data, jint sizeDirect,
    jintArray ptrArray)
{
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    LOG_TRACE("structAddr => "   << reinterpret_cast<void *>(structAddr) <<
              ", sizeDirect => " << sizeDirect);
    struct_check_size(sizeDirect);
    auto ptr(struct_get_ptr(structAddr));
    // See note above: critical arrays safe here.
    const jni_array_region<jint> ptr_array(env, ptrArray);
    jni_critical_array<jlong> data_(env, data);
    unmarshaller_indirect u(sizeDirect,
                            data_.size() * sizeof(decltype(data_)::value_type),
                            ptr_array.begin(), ptr_array.end());
    u.unmarshall_indirect(ptr, // SEH-safe
                          reinterpret_cast<marshall_word_t *>(data_.data()));
    JNI_EXCEPTION_SAFE_CPP_END(env);
}

/*
 * Class:     suneido_jsdi_type_Structure
 * Method:    copyOutVariableIndirect
 * Signature: (J[JI[I[Ljava/lang/Object;[I)V
 */
JNIEXPORT void JNICALL Java_suneido_jsdi_type_Structure_copyOutVariableIndirect(
    JNIEnv * env, jclass, jlong structAddr, jlongArray data, jint sizeDirect,
    jintArray ptrArray, jobjectArray viArray, jintArray viInstArray)
{
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    LOG_TRACE("structAddr => "   << reinterpret_cast<void *>(structAddr) <<
              ", sizeDirect => " << sizeDirect);
    struct_check_size(sizeDirect);
    auto ptr(struct_get_ptr(structAddr));
    // Can't use critical arrays here because the unmarshalling process isn't
    // guaranteed to follow the JNI critical array function restrictions.
    jni_array<jlong> data_(env, data);
    const jni_array_region<jint> ptr_array(env, ptrArray);
    const jni_array_region<jint> vi_inst_array(env, viInstArray);
    unmarshaller_vi u(sizeDirect,
                      data_.size() * sizeof(decltype(data_)::value_type),
                      ptr_array.begin(), ptr_array.end(), vi_inst_array.size());
    u.unmarshall_vi(ptr, reinterpret_cast<marshall_word_t *>(data_.data()),
                    env, viArray, vi_inst_array.begin()); // SEH-safe
    JNI_EXCEPTION_SAFE_CPP_END(env);
}

//==============================================================================
//                  JAVA CLASS: suneido.jsdi.com.COMobject
//==============================================================================

#include "gen/suneido_jsdi_com_COMobject.h"

/*
 * Class:     suneido_jsdi_com_COMobject
 * Method:    queryIDispatchAndProgId
 * Signature: (J[Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_suneido_jsdi_com_COMobject_queryIDispatchAndProgId(
    JNIEnv * env, jclass, jlong ptrToIUnknown,
    jobjectArray /* String[] */ progid)
{
    IUnknown * iunk(reinterpret_cast<IUnknown *>(ptrToIUnknown));
    IDispatch * idisp(0);
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    check_array_atleast_1("progid", env, progid); // check first, as may throw
    idisp = seh::convert_to_cpp(com::query_for_dispatch, iunk);
    if (idisp)
    {
        jni_auto_local<jstring> progid_jstr(
            env, seh::convert_to_cpp(com::get_progid, idisp, env));
        env->SetObjectArrayElement(progid, 0,
                                   static_cast<jstring>(progid_jstr));
    }
    JNI_EXCEPTION_SAFE_CPP_END(env);
    return reinterpret_cast<jlong>(idisp);
}

/*
 * Class:     suneido_jsdi_com_COMobject
 * Method:    coCreateFromProgId
 * Signature: (Ljava/lang/String;[J)Z
 */
JNIEXPORT jboolean JNICALL Java_suneido_jsdi_com_COMobject_coCreateFromProgId(
    JNIEnv * env, jclass, jstring progid, jlongArray ptrPair)
{
    jboolean did_create_object(false);
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    IUnknown * iunk(0);
    IDispatch * idisp(0);
    check_array_atleast(2, "ptrPair", env, ptrPair); // check before creating
    bool created(
        seh::convert_to_cpp<bool, JNIEnv *, jstring, IUnknown *&, IDispatch *&>(
            com::create_from_progid, env, progid, iunk, idisp)
    );
    if (created)
    {
        assert(iunk || idisp);
        const jlong ptrs[2] =
        {
            reinterpret_cast<jlong>(idisp),
            reinterpret_cast<jlong>(iunk),
        };
        env->SetLongArrayRegion(ptrPair, 0, 2, ptrs);
        did_create_object = true;
    }
    JNI_EXCEPTION_SAFE_CPP_END(env);
    return did_create_object;
}

/*
 * Class:     suneido_jsdi_com_COMobject
 * Method:    release
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_suneido_jsdi_com_COMobject_release
 (JNIEnv * env, jclass, jlong ptrToIDispatch, jlong ptrToIUnknown)
{
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    if (ptrToIDispatch)
        reinterpret_cast<IDispatch *>(ptrToIDispatch)->Release();
    if (ptrToIUnknown)
        reinterpret_cast<IUnknown *>(ptrToIUnknown)->Release();
    JNI_EXCEPTION_SAFE_CPP_END(env);
}

/*
 * Class:     suneido_jsdi_com_COMobject
 * Method:    getPropertyByName
 * Signature: (JLjava/lang/String;[I)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_suneido_jsdi_com_COMobject_getPropertyByName(
    JNIEnv * env, jclass, jlong ptrToIDispatch, jstring name, jintArray dispid)
{
    jobject result(0);
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    IDispatch * idisp(reinterpret_cast<IDispatch *>(ptrToIDispatch));
    DISPID dispid_(seh::convert_to_cpp(com::get_dispid_of_name, idisp, env, name));
    // Check the dispid array before getting the property so that we don't throw
    // an exception while we have a local reference to be freed...
    check_array_atleast_1("dispid", env, dispid);
    env->SetIntArrayRegion(dispid, 0, 1,
                           reinterpret_cast<const jint *>(&dispid_));
    result = seh::convert_to_cpp(com::property_get, idisp, dispid_, env);
    JNI_EXCEPTION_SAFE_CPP_END(env);
    return result;
}

/*
 * Class:     suneido_jsdi_com_COMobject
 * Method:    getPropertyByDispId
 * Signature: (JI)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_suneido_jsdi_com_COMobject_getPropertyByDispId(
    JNIEnv * env, jclass, jlong ptrToIDispatch, jint dispid)
{
    jobject result(0);
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    IDispatch * idisp(reinterpret_cast<IDispatch *>(ptrToIDispatch));
    result = seh::convert_to_cpp(
        com::property_get, idisp, static_cast<DISPID>(dispid), env);
    JNI_EXCEPTION_SAFE_CPP_END(env);
    return result;
}

/*
 * Class:     suneido_jsdi_com_COMobject
 * Method:    putPropertyByName
 * Signature: (JLjava/lang/String;Ljava/lang/Object;)I
 */
JNIEXPORT jint JNICALL Java_suneido_jsdi_com_COMobject_putPropertyByName(
    JNIEnv * env, jclass, jlong ptrToIDispatch, jstring name, jobject value)
{
    DISPID dispid(0);
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    IDispatch * idisp(reinterpret_cast<IDispatch *>(ptrToIDispatch));
    dispid = seh::convert_to_cpp(com::get_dispid_of_name, idisp, env, name);
    seh::convert_to_cpp(com::property_put, idisp, dispid, env, value);
    JNI_EXCEPTION_SAFE_CPP_END(env);
    return dispid;
}

/*
 * Class:     suneido_jsdi_com_COMobject
 * Method:    putPropertyByDispId
 * Signature: (JILjava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_suneido_jsdi_com_COMobject_putPropertyByDispId(
    JNIEnv * env, jclass, jlong ptrToIDispatch, jint dispid, jobject value)
{
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    IDispatch * idisp(reinterpret_cast<IDispatch *>(ptrToIDispatch));
    seh::convert_to_cpp(
        com::property_put, idisp, static_cast<DISPID>(dispid), env, value);
    JNI_EXCEPTION_SAFE_CPP_END(env);
}

/*
 * Class:     suneido_jsdi_com_COMobject
 * Method:    callMethodByName
 * Signature: (JLjava/lang/String;[Ljava/lang/Object;[I)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_suneido_jsdi_com_COMobject_callMethodByName(
    JNIEnv * env, jclass, jlong ptrToIDispatch, jstring name, jobjectArray args,
    jintArray dispid)
{
    jobject result(0);
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    IDispatch * idisp(reinterpret_cast<IDispatch *>(ptrToIDispatch));
    DISPID dispid_(seh::convert_to_cpp(com::get_dispid_of_name, idisp, env, name));
    // Check the dispid array before calling the method so that we don't throw
    // an exception while we have a local reference to be freed...
    check_array_atleast_1("dispid", env, dispid);
    env->SetIntArrayRegion(dispid, 0, 1,
                           reinterpret_cast<const jint *>(&dispid_));
    result = seh::convert_to_cpp(com::call_method, idisp, dispid_, env, args);
    JNI_EXCEPTION_SAFE_CPP_END(env);
    return result;
}

/*
 * Class:     suneido_jsdi_com_COMobject
 * Method:    callMethodByDispId
 * Signature: (JI[Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_suneido_jsdi_com_COMobject_callMethodByDispId(
    JNIEnv * env, jclass, jlong ptrToIDispatch, jint dispid, jobjectArray args)
{
    jobject result(0);
    JNI_EXCEPTION_SAFE_CPP_BEGIN
    IDispatch * idisp(reinterpret_cast<IDispatch *>(ptrToIDispatch));
    result = seh::convert_to_cpp(com::call_method, idisp,
                                 static_cast<DISPID>(dispid), env, args);
    JNI_EXCEPTION_SAFE_CPP_END(env);
    return result;
}

} // extern "C"
