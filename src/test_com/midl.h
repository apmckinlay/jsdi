

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0595 */
/* at Tue Oct 22 10:14:42 2013
 */
/* Compiler settings for test_com.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 8.00.0595 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __midl_h__
#define __midl_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ITestJSDICom_FWD_DEFINED__
#define __ITestJSDICom_FWD_DEFINED__
typedef interface ITestJSDICom ITestJSDICom;

#endif 	/* __ITestJSDICom_FWD_DEFINED__ */


/* header files for imported files */
#include "Oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __TestJSDICom_LIBRARY_DEFINED__
#define __TestJSDICom_LIBRARY_DEFINED__

/* library TestJSDICom */
/* [version][uuid] */ 


EXTERN_C const IID LIBID_TestJSDICom;

#ifndef __ITestJSDICom_INTERFACE_DEFINED__
#define __ITestJSDICom_INTERFACE_DEFINED__

/* interface ITestJSDICom */
/* [local][dual][uuid][object] */ 


EXTERN_C const IID IID_ITestJSDICom;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("357279B0-C03A-414A-8869-A3E515252C06")
    ITestJSDICom : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_RefCount( 
            /* [retval][out] */ unsigned __int32 *value) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_BoolValue( 
            /* [retval][out] */ BOOL *value) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_BoolValue( 
            BOOL newvalue) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Int32Value( 
            /* [retval][out] */ __int32 *value) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Int32Value( 
            __int32 newvalue) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Int64Value( 
            /* [retval][out] */ __int64 *value) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Int64Value( 
            __int64 newvalue) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_DoubleValue( 
            /* [retval][out] */ double *value) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_DoubleValue( 
            double newvalue) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_StringValue( 
            /* [retval][string][out] */ BSTR *value) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_StringValue( 
            /* [string][in] */ BSTR newvalue) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_DateValue( 
            /* [retval][out] */ DATE *value) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_DateValue( 
            DATE newvalue) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_IUnkValue( 
            /* [retval][out] */ IUnknown **value) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_IDispValue( 
            /* [retval][out] */ IDispatch **value) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Sum2Ints( 
            __int32 x,
            __int32 y,
            /* [retval][out] */ __int64 *result) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Sum2Doubles( 
            double x,
            double y,
            /* [retval][out] */ double *result) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SumProperties( 
            /* [retval][out] */ double *result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ITestJSDIComVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITestJSDICom * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITestJSDICom * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITestJSDICom * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITestJSDICom * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITestJSDICom * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITestJSDICom * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITestJSDICom * This,
            /* [annotation][in] */ 
            _In_  DISPID dispIdMember,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][in] */ 
            _In_  LCID lcid,
            /* [annotation][in] */ 
            _In_  WORD wFlags,
            /* [annotation][out][in] */ 
            _In_  DISPPARAMS *pDispParams,
            /* [annotation][out] */ 
            _Out_opt_  VARIANT *pVarResult,
            /* [annotation][out] */ 
            _Out_opt_  EXCEPINFO *pExcepInfo,
            /* [annotation][out] */ 
            _Out_opt_  UINT *puArgErr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RefCount )( 
            ITestJSDICom * This,
            /* [retval][out] */ unsigned __int32 *value);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BoolValue )( 
            ITestJSDICom * This,
            /* [retval][out] */ BOOL *value);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BoolValue )( 
            ITestJSDICom * This,
            BOOL newvalue);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Int32Value )( 
            ITestJSDICom * This,
            /* [retval][out] */ __int32 *value);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Int32Value )( 
            ITestJSDICom * This,
            __int32 newvalue);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Int64Value )( 
            ITestJSDICom * This,
            /* [retval][out] */ __int64 *value);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Int64Value )( 
            ITestJSDICom * This,
            __int64 newvalue);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DoubleValue )( 
            ITestJSDICom * This,
            /* [retval][out] */ double *value);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DoubleValue )( 
            ITestJSDICom * This,
            double newvalue);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_StringValue )( 
            ITestJSDICom * This,
            /* [retval][string][out] */ BSTR *value);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_StringValue )( 
            ITestJSDICom * This,
            /* [string][in] */ BSTR newvalue);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DateValue )( 
            ITestJSDICom * This,
            /* [retval][out] */ DATE *value);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DateValue )( 
            ITestJSDICom * This,
            DATE newvalue);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_IUnkValue )( 
            ITestJSDICom * This,
            /* [retval][out] */ IUnknown **value);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_IDispValue )( 
            ITestJSDICom * This,
            /* [retval][out] */ IDispatch **value);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Sum2Ints )( 
            ITestJSDICom * This,
            __int32 x,
            __int32 y,
            /* [retval][out] */ __int64 *result);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Sum2Doubles )( 
            ITestJSDICom * This,
            double x,
            double y,
            /* [retval][out] */ double *result);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SumProperties )( 
            ITestJSDICom * This,
            /* [retval][out] */ double *result);
        
        END_INTERFACE
    } ITestJSDIComVtbl;

    interface ITestJSDICom
    {
        CONST_VTBL struct ITestJSDIComVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITestJSDICom_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ITestJSDICom_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ITestJSDICom_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ITestJSDICom_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define ITestJSDICom_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define ITestJSDICom_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define ITestJSDICom_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define ITestJSDICom_get_RefCount(This,value)	\
    ( (This)->lpVtbl -> get_RefCount(This,value) ) 

#define ITestJSDICom_get_BoolValue(This,value)	\
    ( (This)->lpVtbl -> get_BoolValue(This,value) ) 

#define ITestJSDICom_put_BoolValue(This,newvalue)	\
    ( (This)->lpVtbl -> put_BoolValue(This,newvalue) ) 

#define ITestJSDICom_get_Int32Value(This,value)	\
    ( (This)->lpVtbl -> get_Int32Value(This,value) ) 

#define ITestJSDICom_put_Int32Value(This,newvalue)	\
    ( (This)->lpVtbl -> put_Int32Value(This,newvalue) ) 

#define ITestJSDICom_get_Int64Value(This,value)	\
    ( (This)->lpVtbl -> get_Int64Value(This,value) ) 

#define ITestJSDICom_put_Int64Value(This,newvalue)	\
    ( (This)->lpVtbl -> put_Int64Value(This,newvalue) ) 

#define ITestJSDICom_get_DoubleValue(This,value)	\
    ( (This)->lpVtbl -> get_DoubleValue(This,value) ) 

#define ITestJSDICom_put_DoubleValue(This,newvalue)	\
    ( (This)->lpVtbl -> put_DoubleValue(This,newvalue) ) 

#define ITestJSDICom_get_StringValue(This,value)	\
    ( (This)->lpVtbl -> get_StringValue(This,value) ) 

#define ITestJSDICom_put_StringValue(This,newvalue)	\
    ( (This)->lpVtbl -> put_StringValue(This,newvalue) ) 

#define ITestJSDICom_get_DateValue(This,value)	\
    ( (This)->lpVtbl -> get_DateValue(This,value) ) 

#define ITestJSDICom_put_DateValue(This,newvalue)	\
    ( (This)->lpVtbl -> put_DateValue(This,newvalue) ) 

#define ITestJSDICom_get_IUnkValue(This,value)	\
    ( (This)->lpVtbl -> get_IUnkValue(This,value) ) 

#define ITestJSDICom_get_IDispValue(This,value)	\
    ( (This)->lpVtbl -> get_IDispValue(This,value) ) 

#define ITestJSDICom_Sum2Ints(This,x,y,result)	\
    ( (This)->lpVtbl -> Sum2Ints(This,x,y,result) ) 

#define ITestJSDICom_Sum2Doubles(This,x,y,result)	\
    ( (This)->lpVtbl -> Sum2Doubles(This,x,y,result) ) 

#define ITestJSDICom_SumProperties(This,result)	\
    ( (This)->lpVtbl -> SumProperties(This,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ITestJSDICom_INTERFACE_DEFINED__ */

#endif /* __TestJSDICom_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


