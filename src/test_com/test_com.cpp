/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: test_com.cpp
// auth: Victor Schappert
// date: 20131016
// desc: Non-generated file implementing ITestJSDICom interface as well as its
//       IDispatch support
//==============================================================================

#include "test_com.h"

#include "../concurrent.h"
#include "../util.h"

#include <stdexcept>
#include <sstream>
#include <string>
#include <cassert>
#include <cstdio>  // TODO: remove this when we have better logging

#define ERROR_STR(str) (__FILE__ ": " str)
#define THROW_ERROR(str) { throw std::runtime_error(ERROR_STR(str)); }
#define CHECK_OUTPUT_POINTER(ptr) \
    { assert(ptr || !"output pointer cannot be null"); }

using namespace jsdi;

namespace {

struct TestJSDIComImpl : public ITestJSDICom
{
        //
        // DATA
        //

        unsigned __int32           d_ref_count;
        ITypeInfo                * d_type_info;
        critical_section           d_critical_section;

        BOOL                       d_bool_value;
        signed __int32             d_int32_value;
        signed __int64             d_int64_value;
        double                     d_double_value;
        std::basic_string<OLECHAR> d_string_value;
        DATE                       d_date_value;

        //
        // CONSTRUCTORS
        //

        TestJSDIComImpl();

        virtual ~TestJSDIComImpl();

        //
        // ANCESTOR CLASS: IUnknown
        //

        HRESULT __stdcall QueryInterface(REFIID riid, void ** ppv);

        ULONG __stdcall AddRef();

        ULONG __stdcall Release();

        //
        // ANCESTOR CLASS: IDispatch
        //

        HRESULT __stdcall GetTypeInfoCount(UINT * pCountTypeInfo);

        HRESULT __stdcall GetTypeInfo(UINT iTypeInfo, LCID lcid,
                                      ITypeInfo ** ppITypeInfo);

        HRESULT __stdcall GetIDsOfNames(REFIID riid, LPOLESTR * rgszNames,
                                        UINT cNames, LCID lcid,
                                        DISPID * rgDispId);

        HRESULT __stdcall Invoke(DISPID dispIdMember, REFIID riid, LCID lcid,
                                 WORD wFlags, DISPPARAMS * pDispParams,
                                 VARIANT * pVarResult, EXCEPINFO * pExcepInfo,
                                 UINT * puArgError);

        //
        // ANCESTOR CLASS: ITestJSDICom
        //

        HRESULT __stdcall get_RefCount(unsigned __int32 * value);

        HRESULT __stdcall get_BoolValue(BOOL * value);
        HRESULT __stdcall put_BoolValue(BOOL newvalue);

        HRESULT __stdcall get_Int32Value(signed __int32 * value);
        HRESULT __stdcall put_Int32Value(signed __int32 newvalue);

        HRESULT __stdcall get_Int64Value(signed __int64 * value);
        HRESULT __stdcall put_Int64Value(signed __int64 newvalue);

        HRESULT __stdcall get_DoubleValue(double * value);
        HRESULT __stdcall put_DoubleValue(double newvalue);

        HRESULT __stdcall get_StringValue(BSTR * value);
        HRESULT __stdcall put_StringValue(BSTR newvalue);

        HRESULT __stdcall get_DateValue(DATE * value);
        HRESULT __stdcall put_DateValue(DATE newvalue);

        HRESULT __stdcall get_IUnkValue(IUnknown ** value);

        HRESULT __stdcall get_IDispValue(IDispatch ** value);

        HRESULT __stdcall Sum2Ints(signed __int32 x, signed __int32 y,
                                   signed __int64 * result);
        HRESULT __stdcall Sum2Doubles(double x, double y, double * result);
        HRESULT __stdcall SumProperties(double * result);
};

//==============================================================================
//                                Internals
//==============================================================================

HINSTANCE get_module_handle()
{
    // Get a handle to the module containing this translation unit.
    // See here: http://stackoverflow.com/a/6924332/1911388
    HINSTANCE hinst(0);
    if (! GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCTSTR>(&get_module_handle),
            &hinst))
    {
        THROW_ERROR("failed to get module handle");
    }
    return hinst;
}

ITypeLib * load_type_lib()
{
    // Get the module name
    HINSTANCE hinst(get_module_handle());
    // NOTE: - MAX_PATH already includes the terminating NUL. The +2 is so we
    //         can tack on the string "\9999", to tell LoadTypeLibEx to load the
    //         first type library resource it encounters.
    //       - And the reason we are tacking on \9999 is because the resource ID
    //         of the type library is hardcoded to 9999. So the full library
    //         path will be "X:\path\to\library.DLL\9999".
    OLECHAR module_name[MAX_PATH + 5];
    DWORD size = GetModuleFileNameW(hinst, module_name, MAX_PATH);
    if (MAX_PATH == size &&
        ERROR_INSUFFICIENT_BUFFER == GetLastError())
    {
        THROW_ERROR("not enough buffer space for module name");
    }
    // Concatenate the string "\9999". Note OLECHAR is defined as WCHAR which is
    // defined as wchar_t, so we can do the following.
    wcsncat(module_name, L"\\9999", array_length(module_name));
    // Load type library resource 1 from the module
    ITypeLib * result(0);
    HRESULT hresult = LoadTypeLibEx(module_name, REGKIND_NONE, &result);
    if (FAILED(hresult))
    {
        std::ostringstream() << ERROR_STR("failed to load type library, "
                                          "got HRESULT ")
                             << hresult << throw_cpp<std::runtime_error>();
    }
    return result;
}

ITypeInfo * load_type_info()
{
    ITypeInfo * result(0);
    ITypeLib * type_lib(load_type_lib());
    HRESULT hresult = type_lib->GetTypeInfoOfGuid(IID_ITestJSDICom, &result);
    type_lib->Release();
    if (FAILED(hresult))
    {
        std::ostringstream() << ERROR_STR("failed to load type info for "
                                          "IID_ITestJSDICom, got HRESULT ")
                             << hresult << throw_cpp<std::runtime_error>();
    }
    return result;
}

//==============================================================================
//                               Constructors
//==============================================================================

TestJSDIComImpl::TestJSDIComImpl()
    : d_ref_count(0)
    , d_type_info(0)
    , d_bool_value(FALSE)
    , d_int32_value(0)
    , d_int64_value(0)
    , d_double_value(0.0)
    , d_date_value(0.0) // 30 December 1899, midnight
{
    d_type_info = load_type_info();
}

TestJSDIComImpl::~TestJSDIComImpl()
{
    d_type_info->Release();
}

//==============================================================================
//                                 IUnknown
//==============================================================================

HRESULT __stdcall TestJSDIComImpl::QueryInterface(REFIID riid, void ** ppv)
{
    CHECK_OUTPUT_POINTER(ppv);
    if (IID_IDispatch == riid)
            *ppv = static_cast<IDispatch *>(this);
    else if (IID_IUnknown == riid)
        *ppv = static_cast<IUnknown *>(this);
    else if (IID_ITestJSDICom == riid)
        *ppv = static_cast<ITestJSDICom *>(this);
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG __stdcall TestJSDIComImpl::AddRef()
{
    lock_guard<critical_section> lock(&d_critical_section);
    return ++d_ref_count;
}

ULONG __stdcall TestJSDIComImpl::Release()
{
    unsigned __int32 nrefs(0);
    {
        lock_guard<critical_section> lock(&d_critical_section);
        nrefs = --d_ref_count;
    }
    assert(0 <= nrefs);
    if (! nrefs) delete this;
    return nrefs;
}

//==============================================================================
//                                 IDispatch
//==============================================================================

HRESULT __stdcall TestJSDIComImpl::GetTypeInfoCount(UINT * pCountTypeInfo)
{
    CHECK_OUTPUT_POINTER(pCountTypeInfo);
    *pCountTypeInfo = 1;
    return S_OK;
}

HRESULT __stdcall TestJSDIComImpl::GetTypeInfo(UINT iTypeInfo, LCID lcid,
                                               ITypeInfo ** ppITypeInfo)
{
    CHECK_OUTPUT_POINTER(ppITypeInfo);
    *ppITypeInfo = NULL;
    if (0 != iTypeInfo) return DISP_E_BADINDEX;
    lock_guard<critical_section> lock(&d_critical_section);
    d_type_info->AddRef();
    *ppITypeInfo = d_type_info;
    return S_OK;
}

HRESULT __stdcall TestJSDIComImpl::GetIDsOfNames(REFIID riid,
                                                 LPOLESTR * rgszNames,
                                                 UINT cNames, LCID lcid,
                                                 DISPID * rgDispId)
{
    if (IID_NULL != riid) return DISP_E_UNKNOWNINTERFACE;
    lock_guard<critical_section> lock(&d_critical_section);
    HRESULT result = DispGetIDsOfNames(d_type_info, rgszNames, cNames,
                                       rgDispId);
    return result;
}

HRESULT __stdcall TestJSDIComImpl::Invoke(DISPID dispIdMember, REFIID riid,
                                          LCID lcid, WORD wFlags,
                                          DISPPARAMS * pDispParams,
                                          VARIANT * pVarResult,
                                          EXCEPINFO * pExcepInfo,
                                          UINT * puArgError)
{
    if (IID_NULL != riid) return DISP_E_UNKNOWNINTERFACE;
    lock_guard<critical_section> lock(&d_critical_section);
    HRESULT result = DispInvoke(this, d_type_info, dispIdMember, wFlags,
                                pDispParams, pVarResult, pExcepInfo,
                                puArgError);
    return result;
}

//==============================================================================
//                               ITestJSDICom
//==============================================================================

#define GETTER(member)                                  \
    CHECK_OUTPUT_POINTER(value);                        \
    *value = member;                                    \
    return S_OK;

#define PUTTER(member)                                  \
    member = newvalue;                                  \
    return S_OK;

HRESULT __stdcall TestJSDIComImpl::get_RefCount(unsigned __int32 * value)
{ GETTER(d_ref_count); }

HRESULT __stdcall TestJSDIComImpl::get_BoolValue(BOOL * value)
{ GETTER(d_bool_value); }

HRESULT __stdcall TestJSDIComImpl::put_BoolValue(BOOL newvalue)
{ PUTTER(d_bool_value); }

HRESULT __stdcall TestJSDIComImpl::get_Int32Value(signed __int32 * value)
{ GETTER(d_int32_value); }

HRESULT __stdcall TestJSDIComImpl::put_Int32Value(signed __int32 newvalue)
{ PUTTER(d_int32_value); }

HRESULT __stdcall TestJSDIComImpl::get_Int64Value(signed __int64 * value)
{ GETTER(d_int64_value); }

HRESULT __stdcall TestJSDIComImpl::put_Int64Value(signed __int64 newvalue)
{ PUTTER(d_int64_value); }

HRESULT __stdcall TestJSDIComImpl::get_DoubleValue(double * value)
{ GETTER(d_double_value); }

HRESULT __stdcall TestJSDIComImpl::put_DoubleValue(double newvalue)
{ PUTTER(d_double_value); }

HRESULT __stdcall TestJSDIComImpl::get_StringValue(BSTR * value)
{
    lock_guard<critical_section> lock(&d_critical_section);
    // Contract is caller must release with SysFreeString
    // http://stackoverflow.com/a/19523652/1911388
    *value = SysAllocString(d_string_value.data());
    return S_OK;
}

HRESULT __stdcall TestJSDIComImpl::put_StringValue(BSTR newvalue)
{
    lock_guard<critical_section> lock(&d_critical_section);
    PUTTER(d_string_value);
}

HRESULT __stdcall TestJSDIComImpl::get_DateValue(DATE * value)
{ GETTER(d_date_value); }

HRESULT __stdcall TestJSDIComImpl::put_DateValue(DATE newvalue)
{ PUTTER(d_date_value); }

HRESULT __stdcall TestJSDIComImpl::get_IUnkValue(IUnknown ** value)
{ return QueryInterface(IID_IUnknown, reinterpret_cast<void **>(value)); }

HRESULT __stdcall TestJSDIComImpl::get_IDispValue(IDispatch ** value)
{ return QueryInterface(IID_IDispatch, reinterpret_cast<void **>(value)); }

HRESULT __stdcall TestJSDIComImpl::Sum2Ints(signed __int32 x, signed __int32 y,
                                            signed __int64 * result)
{
    CHECK_OUTPUT_POINTER(result);
    *result = x + y;
    return S_OK;
}

HRESULT __stdcall TestJSDIComImpl::Sum2Doubles(double x, double y,
                                               double * result)
{
    CHECK_OUTPUT_POINTER(result);
    *result = x + y;
    return S_OK;
}

HRESULT __stdcall TestJSDIComImpl::SumProperties(double * result)
{
    CHECK_OUTPUT_POINTER(result);
    *result = static_cast<double>(d_int32_value) +
              static_cast<double>(d_int64_value) +
              d_double_value;
    return S_OK;
}

} // anonymous namespace

//==============================================================================
//                     Exported public creator function
//==============================================================================

extern "C"
{

EXPORT_STDCALL ITestJSDICom * TestCreateComObject()
{
    ITestJSDICom * result(0);
    try
    {
        result = new TestJSDIComImpl;
    }
    catch (const std::runtime_error& e)
    {
        // TODO: jsdi should have some kind of unified logging system for this
        //       type of issue.
        FILE * x = std::fopen(__FILE__ "_error.log", "w");
        if (x)
        {
            std::fprintf(x, e.what());
            std::fclose(x);
        }
    }
    return result;
}

} // extern "C"
