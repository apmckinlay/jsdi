/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: suneido_protocol.cpp
// auth: Victor Schappert
// date: 20140210
// desc: Implements COM interface for handling 'suneido://' protocol.
//==============================================================================

#include "suneido_protocol.h"

#include "com_util.h"
#include "global_refs.h"
#include "jni_util.h"
#include "jsdi_windows.h"
#include "log.h"
#include "util.h"

#include <algorithm> // std::min
#include <sstream>
#include <vector>

#include <WinInet.h> // for decoding url

namespace jsdi {

namespace {

// =============================================================================
//                           struct basic_unknown
// =============================================================================

template<typename COMInterface>
struct basic_unknown : public COMInterface
{
        //
        // DATA
        //

        LONG d_ref_count;

        //
        // CONSTRUCTORS
        //

        basic_unknown() : d_ref_count(1) { }

        virtual ~basic_unknown();

        //
        // ANCESTOR CLASS [of COMInterface]: IUnknown
        //

        virtual ULONG __stdcall AddRef();

        virtual ULONG __stdcall Release();
};

template<typename COMInterface>
basic_unknown<COMInterface>::~basic_unknown() { }

template<typename COMInterface>
ULONG __stdcall basic_unknown<COMInterface>::AddRef()
{ return static_cast<ULONG>(InterlockedIncrement(&d_ref_count)); }

template<typename COMInterface>
ULONG __stdcall basic_unknown<COMInterface>::Release()
{
    LONG nrefs = InterlockedDecrement(&d_ref_count);
    assert(0 <= nrefs);
    if (! nrefs) delete this;
    return static_cast<ULONG>(nrefs);
}

//==============================================================================
//                                  INTERNALS
//==============================================================================

std::string narrow(const wchar_t * wstr, size_t len)
{   // Convert wchar_t string to char string for logging purposes
    int const wchar_len = static_cast<int>(len);
    int const utf8_len = WideCharToMultiByte(CP_UTF8, 0, wstr, wchar_len,
                                             nullptr, 0, nullptr, nullptr);
    std::string result(utf8_len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, wchar_len, &result[0], utf8_len,
                        nullptr, nullptr);
    return result;
}

// =============================================================================
//                              struct protocol
// =============================================================================

struct protocol : public basic_unknown<IInternetProtocol>
{
        //
        // DATA
        //

        std::vector<char> d_data;
        size_t            d_pos;
        JavaVM *          d_jni_jvm;

        //
        // CONSTRUCTORS
        //

        protocol(JavaVM * jni_jvm) : d_pos(0), d_jni_jvm(jni_jvm)
        { assert(jni_jvm || !"valid Java virtual machine instance required"); }

        //
        // ANCESTOR CLASS: IInternetProtocolRoot
        //

        HRESULT __stdcall Start(
            /* [in] */LPCWSTR szUrl,
            /* [in] */IInternetProtocolSink __RPC_FAR *pOIProtSink,
            /* [in] */IInternetBindInfo __RPC_FAR *pOIBindInfo,
            /* [in] */DWORD grfPI,
            /* [in] */HANDLE_PTR dwReserved);

        HRESULT __stdcall Continue(
            /* [in] */PROTOCOLDATA __RPC_FAR *pProtocolData);

        HRESULT __stdcall Abort(
            /* [in] */ HRESULT hrReason,
            /* [in] */ DWORD dwOptions);

        HRESULT __stdcall Terminate(/* [in] */DWORD dwOptions);

        HRESULT __stdcall Suspend(); // Not implemented

        HRESULT __stdcall Resume();  // Not implemented

        //
        // ANCESTOR CLASS: IInternetProtocol
        //

        HRESULT __stdcall Read(
            /* [length_is][size_is][out][in] */ void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbRead);

        HRESULT __stdcall Seek(
            /* [in] */ LARGE_INTEGER dlibMove,
            /* [in] */ DWORD dwOrigin,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition);

        HRESULT __stdcall LockRequest(/* [in] */ DWORD dwOptions);

        HRESULT __stdcall UnlockRequest();

        //
        // ANCESTOR CLASS: IUnknown
        //

        HRESULT __stdcall QueryInterface(REFIID iid, void **ppv);
};

HRESULT __stdcall protocol::Start(LPCWSTR szUrl,
                                  IInternetProtocolSink __RPC_FAR *pOIProtSink,
                                  IInternetBindInfo __RPC_FAR *pOIBindInfo,
                                  DWORD grfPI, HANDLE_PTR dwReserved)
{   
    // Decode any %XX sequences in the URL to the actual character. This should
    // never make the URL string longer, but just in case we check to see if a
    // larger buffer is required.
    const size_t orig_url_len(std::wcslen(szUrl));
    DWORD url_len(static_cast<DWORD>(orig_url_len));
    std::unique_ptr<wchar_t[]> url_dec(new wchar_t[url_len]);
    if (! InternetCanonicalizeUrlW(szUrl, url_dec.get(), &url_len,
                                   ICU_DECODE | ICU_NO_ENCODE))
    {
        if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
        {
            url_dec.reset(new wchar_t[url_len /* Updated by first call */]);
            if (InternetCanonicalizeUrlW(szUrl, url_dec.get(), &url_len,
                                         ICU_DECODE | ICU_NO_ENCODE))
                goto canonicalized_ok;
        }
        LOG_ERROR("Failed to canonicalize URL '" << narrow(szUrl, orig_url_len)
                                                 << "', GetLastError() => "
                                                 << GetLastError());
        return INET_E_INVALID_URL;
    }
canonicalized_ok:
    ;
    // Get a JNI handle to a Java string containing the decoded URL. This
    // requires attaching the running thread to the JVM because we can't be sure
    // whether it started out as a native thread or a JVM thread.
    JNIEnv * env(nullptr);
    JavaVMAttachArgs attach_args;
    attach_args.version = JNI_VERSION_1_6;
    attach_args.name    = nullptr;
    attach_args.group   = nullptr;
    if (JNI_OK != d_jni_jvm->AttachCurrentThread(reinterpret_cast<void **>(&env),
                                                 &attach_args))
    {
        LOG_ERROR("Failed to attach thread to JVM on URL '"
                  << narrow(szUrl, orig_url_len) << '\'');
        return INET_E_OBJECT_NOT_FOUND;
    }
    jni_auto_local<jstring> url_java(env,
                                     reinterpret_cast<const jchar *>(url_dec.get()),
                                     url_len);
    // Invoke Suneido with the URL
    jni_auto_local<jobject> data(env, env->CallStaticObjectMethod(
        GLOBAL_REFS->suneido_jsdi_suneido_protocol_InternetProtocol(),
        GLOBAL_REFS->suneido_jsdi_suneido_protocol_InternetProtocol__m_start(),
        static_cast<jstring>(url_java)));
    if (env->ExceptionCheck())
    {
        LOG_ERROR("A JNI exception propagated back to the COM "
                  "IInternetProtocolRoot::Start() handler for URL '"
                  << narrow(szUrl, orig_url_len) << '\'');
        return INET_E_DATA_NOT_AVAILABLE;
    }
    else if (! data)
    {
        LOG_ERROR("Unexpectedly got back a null from Suneido for URL '"
                  << narrow(szUrl, orig_url_len) << '\'');
        return INET_E_DATA_NOT_AVAILABLE;
    }
    // At this point, we got some kind of data back from the Suneido side which
    // we can pass on to the browser.
    d_pos = 0;
    jbyteArray data_array(static_cast<jbyteArray>(static_cast<jobject>(data)));
    static_assert(
        sizeof(jbyte) == sizeof(decltype(d_data)::value_type),
        "data size mismatch"
    );
    d_data.resize(env->GetArrayLength(data_array));
    env->GetByteArrayRegion(data_array, 0, static_cast<jsize>(d_data.size()),
                            reinterpret_cast<jbyte *>(d_data.data()));
    // Report to the sink that the data is available.
    pOIProtSink->ReportData(BSCF_DATAFULLYAVAILABLE | BSCF_LASTDATANOTIFICATION,
                            static_cast<ULONG>(d_data.size()),
                            static_cast<ULONG>(d_data.size()));
    LOG_DEBUG("Fetched " << d_data.size() << " bytes for URL '"
                         << narrow(szUrl, orig_url_len) << '\'');
    // Done
    return S_OK;
}

HRESULT __stdcall protocol::Continue(PROTOCOLDATA __RPC_FAR *pProtocolData)
{ return S_OK; }

HRESULT __stdcall protocol::Abort(HRESULT hrReason, DWORD dwOptions)
{ return S_OK; }

HRESULT __stdcall protocol::Terminate(DWORD dwOptions)
{ return S_OK; }

HRESULT __stdcall protocol::Suspend()
{ return E_NOTIMPL; }

HRESULT __stdcall protocol::Resume()
{ return E_NOTIMPL; }

HRESULT __stdcall protocol::Read(void __RPC_FAR *pv, ULONG cb,
                                 ULONG __RPC_FAR *pcbRead)
{
    assert(0 <= d_pos && d_pos <= d_data.size());
    const size_t len = std::min(static_cast<size_t>(cb), d_data.size() - d_pos);
    std::memcpy(pv, &d_data[d_pos], len);
    d_pos += len;
    *pcbRead = static_cast<ULONG>(len);
    return d_pos < d_data.size() ? S_OK : S_FALSE;
}

HRESULT __stdcall protocol::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin,
                                 ULARGE_INTEGER __RPC_FAR *plibNewPosition)
{ return S_OK; }

HRESULT __stdcall protocol::LockRequest(DWORD dwOptions)
{ return S_OK; }

HRESULT __stdcall protocol::UnlockRequest()
{ return S_OK; }

HRESULT __stdcall protocol::QueryInterface(REFIID riid, void **ppv)
{
    if (IID_IInternetProtocol == riid)
        *ppv = static_cast<IInternetProtocol *>(this);
    else if (IID_IInternetProtocolRoot == riid)
        *ppv = static_cast<IInternetProtocolRoot *>(this);
    else if (IID_IUnknown == riid)
        *ppv = static_cast<IUnknown *>(this);
    else
    {
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

// =============================================================================
//                          struct protocol_factory
// =============================================================================

struct protocol_factory : public basic_unknown<IClassFactory>
{
        //
        // DATA
        //

        JavaVM * d_jni_jvm;

        //
        // CONSTRUCTORS
        //

        protocol_factory() : d_jni_jvm(nullptr) { };

        //
        // ANCESTOR CLASS: IClassFactory
        //

        HRESULT __stdcall CreateInstance(IUnknown * pUnkOuter, REFIID refiid,
                                         void **ppv);

        HRESULT __stdcall LockServer(BOOL bLock);

        //
        // ANCESTOR CLASS: IUnknown
        //

        HRESULT __stdcall QueryInterface(REFIID iid, void **ppv);
};

HRESULT __stdcall protocol_factory::CreateInstance(IUnknown * pUnkOuter,
                                                   REFIID refiid,
                                                   void **ppv)
{
    if (nullptr != pUnkOuter)
        return CLASS_E_NOAGGREGATION;
    else
    {
        protocol * p(nullptr);
        try
        { p = new protocol(d_jni_jvm); }
        catch (const std::bad_alloc&)
        { return E_OUTOFMEMORY; /* An unlikely and bad state to be in! */ }
        HRESULT hresult = p->QueryInterface(refiid, ppv);
        // If the QueryInterfaces succeeds, it will increment the reference
        // count and we need to Release() to compensate. Conversely, if it
        // fails, then calling Release() will cause the class to delete itself.
        p->Release();
        return hresult;
    }
}

HRESULT __stdcall protocol_factory::LockServer(BOOL bLock)
{ return S_OK; }

HRESULT __stdcall protocol_factory::QueryInterface(REFIID riid, void **ppv)
{
    if (IID_IClassFactory == riid)
        *ppv = static_cast<IClassFactory *>(this);
    else if (IID_IUnknown == riid)
        *ppv = static_cast<IUnknown *>(this);
    else
    {
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

// =============================================================================
//                               INTERNAL DATA
// =============================================================================

static protocol_factory factory;

constexpr CLSID CLSID_SUNEIDO_PROTOCOL =
{    // This is equal to CLSID_SuneidoAPP from the cSuneido's sunapp.cpp.
     0xbfbe2090, 0x6bba, 0x11d4,
     { 0xbc, 0x13, 0x00, 0x60, 0x6e, 0x30, 0xb2, 0x58 },
};

} // anonymous namespace

// =============================================================================
//                          struct suneido_protocol
// =============================================================================

void suneido_protocol::register_handler(JavaVM * jni_jvm)
{
    HRESULT hresult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hresult))
    {
        IInternetSession * iis(nullptr);
        hresult = CoInternetGetSession(0, &iis, 0);
        if (FAILED(hresult))
        {
            std::ostringstream() << "CoInternetGetSession() failed in "
                                 << __func__ << "() with hresult " << hresult
                                 << throw_cpp<std::runtime_error>();
        }
        hresult = iis->RegisterNameSpace(&factory, CLSID_SUNEIDO_PROTOCOL,
                                         L"suneido", 0, nullptr, 0);
        if (FAILED(hresult))
        {
            std::ostringstream() << "IInternetSession::RegisterNameSpace() "
                                    "failed in " << __func__
                                 << "() with hresult " << hresult
                                 << throw_cpp<std::runtime_error>();
        }
        iis->Release();
        assert(jni_jvm || !"valid Java virtual machine instance required");
        assert(!factory.d_jni_jvm || !"handler already registered");
        factory.d_jni_jvm = jni_jvm;
    }
    else
    {
        std::ostringstream() << "CoInitializeEx() failed in " << __func__
                             << "() with hresult " << hresult
                             << throw_cpp<std::runtime_error>();
    }
}

void suneido_protocol::unregister_handler() noexcept
{ CoUninitialize(); }

} // namespace jsdi

//==============================================================================
//                                  TESTS
//==============================================================================

#ifndef __NOTEST__

#include "test.h"

TEST(register,
    jsdi::test_java_vm vm;
    jsdi::suneido_protocol::register_handler(vm.java_vm());
    jsdi::suneido_protocol::unregister_handler();
);

#endif __NOTEST__
