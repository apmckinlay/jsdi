/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: com.cpp
// auth: Victor Schappert
// date: 20131022
// desc: Code for interacting with COM IUnknown and IDispatch interfaces
//==============================================================================

#include "com.h"

#include "com_util.h"
#include "global_refs.h"
#include "jni_exception.h"
#include "jni_util.h"

#include <cassert>
#include <limits>
#include <stdexcept>

namespace jsdi {

//==============================================================================
//                                  INTERNALS
//==============================================================================

namespace {

#define ASSERT_IUNKNOWN(iunk)   \
    assert(iunk || !"IUnknown pointer cannot be NULL");
#define ASSERT_IDISPATCH(idisp) \
    assert(idisp || !"IDispatch pointer cannot be NULL");

static_assert(sizeof(std::remove_pointer<BSTR>::type) == sizeof(jchar),
              "character size mismatch");

constexpr int64_t _100_NANO_INTERVALS_FROM_JAN1_1601_TO_JAN1_1970 =
    116444736000000000LL; // see here: http://support.microsoft.com/kb/167296
constexpr int64_t _100_NANO_INTERVALS_PER_MILLISECOND = 10000;

std::u16string temp_convert_string(const std::string& s)
{
    // TODO: Remove this function once it is safe to remove the temp_to_string
    //       functions below.
    std::u16string t(s.size(), std::char_traits<char16_t>::eof());
    std::string::const_iterator i = s.begin(), e = s.end();
    std::u16string::iterator j = t.begin();
    for (; i != e; ++i, ++j) *j = static_cast<char16_t>(*i);
    return t;
}

template <typename NumberType>
std::u16string temp_to_string(const NumberType& n)
{
    // This is a temporary workaround to the present inability to insert
    // integers into basic_ostream's based no char16_t because the std library
    // isn't fully implemented.
    //
    // TODO: Remove this function once GCC and MinGW finally get some more
    //       support for char16_t -- for example, when they support char16_t-
    //       based locale facets -- or even when std::to_string() is actually
    //       available. We don't even have std::to_string() in GCC 4.8.1 with
    //       MinGW, because of the _GLIBCXX_HAVE_BROKEN_VSWPRINTF guard in
    //       basic_string.h.
    // SEE:  http://stackoverflow.com/q/19784588/1911388
    //       http://stackoverflow.com/q/12975341/1911388
    static_assert(std::is_arithmetic<NumberType>::value, "use only on numbers");
    std::ostringstream o;
    o << n;
    return temp_convert_string(o.str());
}

jstring bstr_to_jstr(BSTR bstr, JNIEnv * env)
{
    assert(bstr && env);
    size_t size(SysStringLen(bstr));
    jstring result = env->NewString(reinterpret_cast<const jchar *>(bstr),
                                    size);
    // NewString() returns NULL if it fails
    if (! result) throw jni_bad_alloc("NewString", __FUNCTION__);
    return result;
}

com_managed_bstr jstr_to_bstr(jstring jstr, JNIEnv * env)
{
    assert(jstr && env);
    jsize size(env->GetStringLength(jstr));
    com_managed_bstr result(SysAllocStringLen(nullptr, size));
    env->GetStringRegion(jstr, 0, size,
                         reinterpret_cast<jchar *>(result.get()));
    // GetStringRegion() raises a JNI exception if it fails
    JNI_EXCEPTION_CHECK(env);
    return result;
}

void throw_com_exception(JNIEnv * env, const char * message)
{
    assert(env);
    if (!env->ThrowNew(GLOBAL_REFS->suneido_language_jsdi_com_COMException(),
                       message))
        throw jni_exception(message, true /* pending */);
    else
        throw std::runtime_error("failed to throw COMException");
}

void throw_com_exception(JNIEnv * env, jstring message)
{
    assert(env && message);
    jni_auto_local<jthrowable> exception(
        env,
        static_cast<jthrowable>(env->NewObject(
            GLOBAL_REFS->suneido_language_jsdi_com_COMException(),
            GLOBAL_REFS->suneido_language_jsdi_com_COMException__init(),
            message)));
    JNI_EXCEPTION_CHECK(env);
    if (! exception) throw jni_bad_alloc("NewObject", __FUNCTION__);
    env->Throw(static_cast<jthrowable>(exception));
    JNI_EXCEPTION_CHECK(env); // This has to throw by definition.
}

void throw_com_exception(JNIEnv * env, const char * message, jobject object)
{
    assert(env && object);
    jni_auto_local<jstring> tostr(
        env,
        static_cast<jstring>(env->CallObjectMethod(
            object, GLOBAL_REFS->java_lang_Object__m_toString())));
    JNI_EXCEPTION_CHECK(env);
    jni_utf16_ostream o(env);
    o << message << u": " << static_cast<jstring>(tostr);
    jni_auto_local<jstring> jstr_message(env, o.jstr());
    throw_com_exception(env, static_cast<jstring>(jstr_message));
}

jobject jni_make_int64(JNIEnv * env, int64_t value)
{
    jobject result(
        env->NewObject(GLOBAL_REFS->java_lang_Long(),
                       GLOBAL_REFS->java_lang_Long__init(), value));
    if (! result) jni_bad_alloc("NewObject", __FUNCTION__);
    return result;
}

jobject jni_make_uint64(JNIEnv * env, uint64_t value)
{
    jobject result(nullptr);
    if (value <= static_cast<uint64_t>(std::numeric_limits<jlong>::max()))
    {
        result = env->NewObject(GLOBAL_REFS->java_lang_Long(),
                                GLOBAL_REFS->java_lang_Long__init(),
                                static_cast<jlong>(value));
    }
    else
    {
        jni_utf16_ostream o(env);
        o << temp_to_string(value);
        jni_auto_local<jstring> str_value(env, o.jstr());
        result = env->NewObject(GLOBAL_REFS->java_math_BigDecimal(),
                                GLOBAL_REFS->java_math_BigDecimal__init1(),
                                static_cast<jstring>(str_value),
                                GLOBAL_REFS->suneido_language_Numbers__f_MC());
    }
    JNI_EXCEPTION_CHECK(env);
    if (! result) jni_bad_alloc("NewObject", __FUNCTION__);
    return result;
}

jobject jni_make_bigdecimal(JNIEnv * env, double value)
{
    jobject result(
        env->NewObject(GLOBAL_REFS->java_math_BigDecimal(),
                       GLOBAL_REFS->java_math_BigDecimal__init(), value,
                       GLOBAL_REFS->suneido_language_Numbers__f_MC()));
    JNI_EXCEPTION_CHECK(env);
    if (! result) jni_bad_alloc("NewObject", __FUNCTION__);
    return result;
}

jlong com_date_to_millis_since_jan1_1970(double com_date)
{
    SYSTEMTIME st;
    union
    {
        FILETIME ft;
        uint64_t _100_nano_intervals_since_jan1_1601_utc;
    };
    // We have to do signed arithmetic because any value before January 1, 1970
    // is a negative number.
    if (! VariantTimeToSystemTime(com_date, &st) ||
        ! SystemTimeToFileTime(&st, &ft))
        throw std::runtime_error("date conversion error");
    assert(_100_nano_intervals_since_jan1_1601_utc <
           static_cast<uint64_t>(std::numeric_limits<int64_t>::max()));
    int64_t result =
        (static_cast<int64_t>(_100_nano_intervals_since_jan1_1601_utc)
            - _100_NANO_INTERVALS_FROM_JAN1_1601_TO_JAN1_1970)
            / _100_NANO_INTERVALS_PER_MILLISECOND;
    return static_cast<jlong>(result);
}

double millis_since_jan1_1970_to_com_date(jlong millis_since_jan1_1970)
{
    double result(0.0);
    SYSTEMTIME st;
    union
    {
        FILETIME ft;
        int64_t  _100_nano_intervals_since_jan1_1601_utc;
    };
    // We have to do signed arithmetic because any number before January 1, 1900
    // is a negative number.
    _100_nano_intervals_since_jan1_1601_utc =
        millis_since_jan1_1970 * _100_NANO_INTERVALS_PER_MILLISECOND +
        _100_NANO_INTERVALS_FROM_JAN1_1601_TO_JAN1_1970;
    if (_100_nano_intervals_since_jan1_1601_utc < 0)
        throw std::runtime_error("date conversion error: number below zero");
    if (! FileTimeToSystemTime(&ft, &st))
        throw std::runtime_error("date conversion error");
    else if (! SystemTimeToVariantTime(&st, &result))
        throw std::runtime_error("can't fit jSuneido date into COM date");
    return result;
}

jobject jni_make_date(JNIEnv * env, double com_date)
{
    // Convert to a value we can use to initialize a Java date
    jlong millis_since_jan1_1970(0);
    try
    { millis_since_jan1_1970 = com_date_to_millis_since_jan1_1970(com_date); }
    catch (const std::runtime_error& e)
    { throw_com_exception(env, e.what()); }
    // Return
    jobject result = env->NewObject(GLOBAL_REFS->java_util_Date(),
                                    GLOBAL_REFS->java_util_Date__init(),
                                    static_cast<jlong>(millis_since_jan1_1970));
    JNI_EXCEPTION_CHECK(env);
    if (! result) throw jni_bad_alloc("NewObject", __FUNCTION__);
    return result;
}

double java_date_to_com_date(JNIEnv * env, jobject java_date)
{
    double result(0.0);
    try
    {
        result = millis_since_jan1_1970_to_com_date(
            env->CallNonvirtualLongMethod(
                java_date, GLOBAL_REFS->java_util_Date(),
                GLOBAL_REFS->java_util_Date__m_getTime()));
    }
    catch (const std::runtime_error& e)
    { throw_com_exception(env, e.what()); }
    return result;
}

jobject jni_make_comobject(JNIEnv * env, IUnknown * iunk)
{
    assert(env && iunk);
    // Source of the IUnknown pointer (whoever packed it into the VARIANT
    // structure) should already have incremented the reference count for us.
    // However, because we are managing the VARIANT itself and will call
    // VariantClear() on it, which will call Release() on the IUnknown, we need
    // to increment the reference count ourselves so that the COMobject we
    // create 'owns' one reference.
    iunk->AddRef();
    com_managed_interface<IUnknown> managed_iunk(iunk);
    // Create the COMobject.
    jobject result = env->NewObject(
        GLOBAL_REFS->suneido_language_jsdi_com_COMobject(),
        GLOBAL_REFS->suneido_language_jsdi_com_COMobject__init(),
        static_cast<jstring>(nullptr), reinterpret_cast<jlong>(iunk),
        JNI_FALSE);
    JNI_EXCEPTION_CHECK(env);
    if (! result) throw jni_bad_alloc("NewObject", __FUNCTION__);
    managed_iunk.release(); // Everything worked so don't reduce ref count
    return result;
}

jobject jni_make_comobject(JNIEnv * env, IDispatch * idisp)
{
    assert(env && idisp);
    idisp->AddRef(); // See comment in the IUnknown version.
    com_managed_interface<IDispatch> managed_idisp(idisp);
    // Get the progid if it is available.
    jni_auto_local<jstring> progid(env, com::get_progid(idisp, env));
    // Create the COMobject.
    jobject result = env->NewObject(
        GLOBAL_REFS->suneido_language_jsdi_com_COMobject(),
        GLOBAL_REFS->suneido_language_jsdi_com_COMobject__init(),
        static_cast<jstring>(progid), reinterpret_cast<jlong>(idisp),
        JNI_TRUE);
    JNI_EXCEPTION_CHECK(env);
    if (! result) throw jni_bad_alloc("NewObject", __FUNCTION__);
    managed_idisp.release(); // Everything worked so don't reduce ref count
    return result;
}

VARIANT& jsuneido_to_com(JNIEnv * env, jobject in, VARIANT& out)
{
    // -------------------------------------------------------------------------
    // MARSHALLING CODE TO CONVERT FROM A jSuneido JAVA TYPE TO A COM C++ TYPE.
    // -------------------------------------------------------------------------
    // * This code is conceptually equivalent to cSuneido's su2com() function.
    // * Please keep this code in sync with the jSuneido type system in the Java
    //   code.
    // -------------------------------------------------------------------------
    if (env->IsInstanceOf(in, GLOBAL_REFS->java_lang_Number()))
    {
        // TODO: Remove Integer from the number system and only use Long and
        //       BigDecimal.
        jni_auto_local<jobject> number(
            env,
            env->CallStaticObjectMethod(
                GLOBAL_REFS->suneido_language_Numbers(),
                GLOBAL_REFS->suneido_language_Numbers__m_narrow(), in));
        if (env->IsInstanceOf(in, GLOBAL_REFS->java_lang_Integer()))
        {
            V_VT(&out) = VT_I4;
            V_I4(&out) = env->CallNonvirtualIntMethod(
                in, GLOBAL_REFS->java_lang_Integer(),
                GLOBAL_REFS->java_lang_Integer__m_intValue());
        }
        else if (env->IsInstanceOf(in, GLOBAL_REFS->java_lang_Long()))
        {
            V_VT(&out) = VT_I8;
            V_I8(&out) = env->CallNonvirtualLongMethod(
                in, GLOBAL_REFS->java_lang_Long(),
                GLOBAL_REFS->java_lang_Long__m_longValue());
        }
        else if (env->IsInstanceOf(in, GLOBAL_REFS->java_math_BigDecimal()))
        {
            V_VT(&out) = VT_R8;
            V_R8(&out) = env->CallNonvirtualDoubleMethod(
                in, GLOBAL_REFS->java_math_BigDecimal(),
                GLOBAL_REFS->java_math_BigDecimal__m_doubleValue());
        }
        else
        {
            throw_com_exception(env, "unknown number class", in);
        }
    }
    else if (env->IsInstanceOf(in, GLOBAL_REFS->java_lang_Boolean()))
    {
        V_VT(&out) = VT_BOOL;
        V_BOOL(&out) = env->CallNonvirtualBooleanMethod(
            in, GLOBAL_REFS->java_lang_Boolean(),
            GLOBAL_REFS->java_lang_Boolean__m_booleanValue());
    }
    else if (env->IsInstanceOf(in, GLOBAL_REFS->java_lang_CharSequence()))
    {
        // COM convention is callee frees the BSTR.
        jni_auto_local<jstring> str(
            env,
            static_cast<jstring>(env->CallObjectMethod(
                in, GLOBAL_REFS->java_lang_Object__m_toString())));
        com_managed_bstr bstr(jstr_to_bstr(static_cast<jstring>(str), env));
        V_VT(&out) = VT_BSTR;
        V_BSTR(&out) = bstr.release();
    }
    else if (env->IsInstanceOf(in, GLOBAL_REFS->java_util_Date()))
    {
        V_VT(&out) = VT_DATE;
        V_DATE(&out) = java_date_to_com_date(env, in);
    }
    else if (env->IsInstanceOf(
        in, GLOBAL_REFS->suneido_language_jsdi_com_COMobject()))
    {
        // We need to get a live reference to the appropriate COM interface
        // in a thread-safe manner, so we need the equivalent of a Java
        // 'synchronized' block on the COMobject instance, and we need to be
        // sure that the underlying reference hasn't already been release()'d
        // in Java because otherwise we can't tell if the pointers are valid.
        //
        // NOTE: You could potentially get a DEADLOCK if Suneido programmer has
        //       two COMobjects, x & y, and two threads, T1 and T2. If T1 does
        //       x.something(y) "simultaneously" with T2 doing y.something(x)
        //       such that T1 locks x, T2 locks y, and then T1 tries to lock
        //       y, there will be a deadlock because on the Java side all of
        //       the get/put/call methods on COMobject lock the object for the
        //       duration of the call. This is admittedly a weird usage pattern,
        //       and arguably the Suneido programmer should fix his circularly-
        //       referencing multi-threaded code rather than us changing how
        //       COMobject is implemented.
        jni_auto_monitor(env, in);
        env->CallNonvirtualVoidMethod(
            in,
            GLOBAL_REFS->suneido_language_jsdi_com_COMobject(),
            GLOBAL_REFS
                ->suneido_language_jsdi_com_COMobject__m_verifyNotReleased());
        JNI_EXCEPTION_CHECK(env); // Will throw if verifyNotReleased() fails
        jlong ptr = env->GetLongField(
            in, GLOBAL_REFS->suneido_language_jsdi_com_COMobject__f_ptr());
        jboolean is_disp = env->CallNonvirtualBooleanMethod(
            in,
            GLOBAL_REFS->suneido_language_jsdi_com_COMobject(),
            GLOBAL_REFS->suneido_language_jsdi_com_COMobject__m_isDispatch());
        assert(ptr || !"COMobject cannot contain a NULL pointer");
        if (is_disp)
        {
            V_VT(&out) = VT_DISPATCH;
            V_DISPATCH(&out) = reinterpret_cast<IDispatch *>(ptr);
            V_DISPATCH(&out)->AddRef(); // Convention is callee Release
        }
        else
        {
            V_VT(&out) = VT_UNKNOWN;
            V_UNKNOWN(&out) = reinterpret_cast<IUnknown *>(ptr);
            V_UNKNOWN(&out)->AddRef(); // Convention is callee Release
        }
    }
    else
    {
        throw_com_exception(env, "can't convert", in);
    }
    //
    // Throw if any part of the conversion process raised a Java exception.
    //
    JNI_EXCEPTION_CHECK(env);
    //
    // Done
    //
    return out;
}

jobject com_to_jsuneido(JNIEnv * env, VARIANT& in)
{
    // -------------------------------------------------------------------------
    // MARSHALLING CODE TO CONVERT FROM A COM C++ TYPE TO A jSuneido JAVA TYPE
    // -------------------------------------------------------------------------
    // * This code is conceptually equivalent to cSuneido's com2su() function.
    // * Please keep this code in sync with the jSuneido type system in the Java
    //   code.
    // -------------------------------------------------------------------------
    jobject result(nullptr);
    //
    // Dereference the variant if necessary.
    //
    VARIANT * value(&in);
    VARIANT buffer;                     // Only used if reference is provided...
    com_managed_variant managed_buffer; // Manages buffer only if reference
    if (VT_BYREF == V_VT(&in))
    {
        VariantInit(&buffer);
        VariantCopyInd(&buffer, &in);
        value = &buffer;
        managed_buffer.reset(&buffer);  // Caller's responsibility to free 'in'
    }
    //
    // Convert the variant value to a jobject recognized by the jSuneido type
    // system.
    //
    switch (V_VT(value))
    {
        case VT_NULL:
        case VT_EMPTY:
            // cSuneido returns zero when there's no value in the VARIANT.
            result = GLOBAL_REFS->ZERO_object(); // TODO: return Long, not Integer
            break;
        case VT_BOOL:
            result = V_BOOL(value)
                ? GLOBAL_REFS->TRUE_object()
                : GLOBAL_REFS->FALSE_object()
                ;
            break;
        case VT_I1:
            result = jni_make_int64(env, static_cast<int64_t>(V_I1(value)));
            break;
        case VT_I2:
            result = jni_make_int64(env, static_cast<int64_t>(V_I2(value)));
            break;
        case VT_I4:
            result = jni_make_int64(env, static_cast<int64_t>(V_I4(value)));
            break;
        case VT_I8:
            result = jni_make_int64(env, static_cast<int64_t>(V_I8(value)));
            break;
        case VT_UI1:
            result = jni_make_int64(env, static_cast<int64_t>(V_UI1(value)));
            break;
        case VT_UI2:
            result = jni_make_int64(env, static_cast<int64_t>(V_UI2(value)));
            break;
        case VT_UI4:
            result = jni_make_int64(env, static_cast<int64_t>(V_UI4(value)));
            break;
        case VT_UI8:
            result = jni_make_uint64(env, static_cast<uint64_t>(V_UI8(value)));
            break;
        case VT_R4:
            result = jni_make_bigdecimal(env, static_cast<double>(V_R4(value)));
            break;
        case VT_R8:
            result = jni_make_bigdecimal(env, static_cast<double>(V_R8(value)));
            break;
        case VT_BSTR:
            // The memory allocated for the BSTR is freed, as with any other
            // resources used by the variant, when VariantClear is called at the
            // bottom of this function.
            result = bstr_to_jstr(V_BSTR(value), env);
            break;
        case VT_DATE:
            result = jni_make_date(env, V_DATE(value));
            break;
        case VT_UNKNOWN:
            result = V_UNKNOWN(value)
                ? jni_make_comobject(env, V_UNKNOWN(value))
                : GLOBAL_REFS->FALSE_object()
                ;
            break;
        case VT_DISPATCH:
            result = V_DISPATCH(value)
                ? jni_make_comobject(env, V_DISPATCH(value))
                : GLOBAL_REFS->FALSE_object()
                ;
            break;
        default:
            throw_com_exception(env, "can't convert to jSuneido value");
            break;
    }
    //
    // Done
    //
    return result;
}

void append_excepinfo(jni_utf16_ostream& o, EXCEPINFO& excepinfo)
{
    if (excepinfo.pfnDeferredFillIn)
    {
        if (FAILED(excepinfo.pfnDeferredFillIn(&excepinfo)))
            throw std::runtime_error("failed deferred fill-in");
    }
    o << u"COM exception - ";
    if (excepinfo.bstrDescription)
    {
        o << excepinfo.bstrDescription << u", ";
        SysFreeString(excepinfo.bstrDescription);
    }
    o << u" code: " << temp_to_string(excepinfo.wCode);
    if (excepinfo.bstrSource)
    {
        o << u", source: " << excepinfo.bstrSource;
        SysFreeString(excepinfo.bstrSource);
    }
    if (excepinfo.bstrHelpFile)
    {
        SysFreeString(excepinfo.bstrHelpFile);
    }
}

void append_param(jni_utf16_ostream& o, const UINT * pu_arg_error)
{
    if (pu_arg_error)
        o << u" (at param " << temp_to_string(*pu_arg_error) << u')';
}

void throw_invoke_fail(JNIEnv * env, HRESULT hresult, EXCEPINFO& excepinfo,
                       const UINT * pu_arg_error, const char * action)
{
    assert(env && action); // pu_arg_error may be NULL
    jni_utf16_ostream o(env);
    // Convert HRESULT and/or the other exception information into readable
    // string.
    switch (hresult)
    {
        case DISP_E_BADPARAMCOUNT:
            o << u"bad param count";
            break;
        case DISP_E_BADVARTYPE:
            o << u"bad var type";
            break;
        case DISP_E_EXCEPTION:
            append_excepinfo(o, excepinfo);
            break;
        case DISP_E_MEMBERNOTFOUND:
            o << u"member not found";
            break;
        case DISP_E_NONAMEDARGS:
            o << u"no named args";
            break;
        case DISP_E_OVERFLOW:
            o << u"overflow (one of the arguments could not be coerced to the "
                  "specified type)";
            break;
        case DISP_E_PARAMNOTFOUND:
            o << u"param not found";
            append_param(o, pu_arg_error);
            break;
        case DISP_E_TYPEMISMATCH:
            o << u"type mismatch";
            append_param(o, pu_arg_error);
            break;
        default:
            assert(FAILED(hresult));
            o << u"failed with HRESULT: " << hresult;
            break;
    }
    // Throw the exception
    jni_auto_local<jstring> message(env, o.jstr());
    throw_com_exception(env, static_cast<jstring>(message));
}

} // anonymous namespace

//==============================================================================
//                                  struct com
//==============================================================================

IDispatch * com::query_for_dispatch(IUnknown * iunk)
{
    ASSERT_IUNKNOWN(iunk);
    IDispatch * idisp(nullptr);
    HRESULT hresult = iunk->QueryInterface(IID_IDispatch,
                                           reinterpret_cast<void **>(&idisp));
    return SUCCEEDED(hresult) ? idisp : nullptr;
}

jstring com::get_progid(IDispatch * idisp, JNIEnv * env)
{
    ASSERT_IDISPATCH(idisp);
    UINT count(0);
    if (SUCCEEDED(idisp->GetTypeInfoCount(&count)) && 0 < count)
    {
        ITypeInfo * type_info_unmanaged(nullptr);
        if (SUCCEEDED(idisp->GetTypeInfo(0, LOCALE_SYSTEM_DEFAULT,
                                         &type_info_unmanaged)) &&
            type_info_unmanaged)
        {
            com_managed_interface<ITypeInfo> type_info(type_info_unmanaged);
            BSTR name_unmanaged(nullptr);
            if (SUCCEEDED(type_info->GetDocumentation(MEMBERID_NIL,
                    &name_unmanaged,
                    nullptr, nullptr, nullptr)) && name_unmanaged)
            {
                static_assert(sizeof(OLECHAR) == sizeof(char16_t),
                              "character size mismatch");
                com_managed_bstr name(name_unmanaged);
                return bstr_to_jstr(name_unmanaged, env);
            }
        }
    }
    return static_cast<jstring>(nullptr);
}

bool com::create_from_progid(JNIEnv * env, jstring progid, IUnknown *& iunk,
                             IDispatch *& idisp)
{
    com_managed_bstr progid_bstr(jstr_to_bstr(progid, env));
    CLSID clsid;
    if (FAILED(CLSIDFromProgID(progid_bstr.get(), &clsid))) return false;
    // Try to get IDispatch first. Only if we don't get IDispatch will we try
    // to get IUnknown.
    HRESULT hresult = CoCreateInstance(clsid, nullptr, CLSCTX_SERVER,
                                       IID_IDispatch,
                                       reinterpret_cast<void **>(&idisp));
    if (SUCCEEDED(hresult) && idisp) return true;
    hresult = CoCreateInstance(clsid, nullptr, CLSCTX_SERVER, IID_IUnknown,
                               reinterpret_cast<void **>(&iunk));
    return SUCCEEDED(hresult) && iunk;
}

DISPID com::get_dispid_of_name(IDispatch * idisp, JNIEnv * env, jstring name)
    throw (jni_exception)
{
    ASSERT_IDISPATCH(idisp);
    com_managed_bstr name_bstr(jstr_to_bstr(name, env));
    BSTR name_arr[1] = { name_bstr.get() };
    DISPID dispid(0);
    if (FAILED(idisp->GetIDsOfNames(IID_NULL, name_arr, array_length(name_arr),
                                    LOCALE_SYSTEM_DEFAULT, &dispid)))
    {
        jni_utf16_ostream o(env);
        o << u"no member named '" << name << u'\'';
        jni_auto_local<jstring> message(env, o.jstr());
        throw_com_exception(env, static_cast<jstring>(message));
    }
    return dispid;
}

jobject com::property_get(IDispatch * idisp, DISPID dispid, JNIEnv * env)
    throw (jni_exception)
{
    ASSERT_IDISPATCH(idisp);
    DISPPARAMS args = { nullptr, nullptr, 0, 0 };
    VARIANT result;
    EXCEPINFO excepinfo;
    HRESULT hresult = idisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT,
                                    DISPATCH_PROPERTYGET, &args, &result,
                                    &excepinfo, nullptr);
    com_managed_variant managed_result(&result);
    if (FAILED(hresult))
    {
        throw_invoke_fail(env, hresult, excepinfo, nullptr, "property get");
    }
    return com_to_jsuneido(env, result);
}

void com::property_put(IDispatch * idisp, DISPID dispid, JNIEnv * env,
                       jobject value) throw (jni_exception)
{
    ASSERT_IDISPATCH(idisp);
    VARIANT input;
    DISPID put = DISPID_PROPERTYPUT;
    DISPPARAMS args = { &jsuneido_to_com(env, value, input), &put, 1, 1 };
    EXCEPINFO excepinfo;
    // The reason we don't need a managed pointer for 'input' is that it is
    // callee's responsibility to free it.
    HRESULT hresult = idisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT,
                                    DISPATCH_PROPERTYPUT, &args, nullptr,
                                    &excepinfo, nullptr);
    if (FAILED(hresult))
    {
        throw_invoke_fail(env, hresult, excepinfo, nullptr, "property put");
    }
}

jobject com::call_method(IDispatch * idisp, DISPID dispid, JNIEnv * env,
                         jobjectArray args) throw (jni_exception)
{
    ASSERT_IDISPATCH(idisp);
    const jsize num_args(env->GetArrayLength(args));
    DISPPARAMS com_args = { nullptr, nullptr, static_cast<UINT>(num_args), 0 };
    std::vector<VARIANT> var_args(com_args.cArgs);
    std::vector<VARIANT>::reverse_iterator i = var_args.rbegin(),
                                           e = var_args.rend();
    try
    {
        for (jsize arg_index = 0; i != e; ++i, ++arg_index)
        {
            jni_auto_local<jobject> arg(
                env, env->GetObjectArrayElement(args, arg_index));
            jsuneido_to_com(env, static_cast<jobject>(arg), *i);
        }
    }
    catch (...)
    {
        // If an exception gets thrown while converting the jSuneido Java types
        // to COM types, we need to clear all the variants which were
        // initialized before rethrowing.
        std::vector<VARIANT>::reverse_iterator j = var_args.rbegin();
        for (; j != i; ++j) VariantClear(&*j);
        throw;
    }
    com_args.rgvarg = var_args.data();
    VARIANT result;
    EXCEPINFO excepinfo;
    UINT arg_error(0);
    HRESULT hresult = idisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT,
                                    DISPATCH_METHOD, &com_args, &result,
                                    &excepinfo, &arg_error);
    com_managed_variant managed_result(&result);
    if (FAILED(hresult))
    {
        throw_invoke_fail(env, hresult, excepinfo, &arg_error, "call");
    }
    return com_to_jsuneido(env, result); // com_to_jsuneido() will clear result.
}

} // namespace jsdi

//==============================================================================
//                                  TESTS
//==============================================================================

#ifndef __NOTEST__

#include "test.h"

using namespace jsdi;

TEST(com_date_conversion,
    constexpr jlong feb7_1982_in_millis = 381888000000LL; // UTC
    constexpr double feb7_1982_as_double = 29989.0;
    assert_equals(feb7_1982_in_millis,
                  com_date_to_millis_since_jan1_1970(feb7_1982_as_double));
    assert_equals(feb7_1982_as_double,
                  millis_since_jan1_1970_to_com_date(feb7_1982_in_millis));
    assert_equals(com_date_to_millis_since_jan1_1970(0.0),
                  com_date_to_millis_since_jan1_1970(
                      millis_since_jan1_1970_to_com_date(
                          com_date_to_millis_since_jan1_1970(0.0))));
);

#endif // __NOTEST__

