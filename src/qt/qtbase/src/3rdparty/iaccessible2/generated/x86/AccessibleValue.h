

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Thu Mar 08 14:53:55 2012
 */
/* Compiler settings for AccessibleValue.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 7.00.0555 
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

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __AccessibleValue_h__
#define __AccessibleValue_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAccessibleValue_FWD_DEFINED__
#define __IAccessibleValue_FWD_DEFINED__
typedef interface IAccessibleValue IAccessibleValue;
#endif 	/* __IAccessibleValue_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oaidl.h"
#include "oleacc.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IAccessibleValue_INTERFACE_DEFINED__
#define __IAccessibleValue_INTERFACE_DEFINED__

/* interface IAccessibleValue */
/* [uuid][object] */ 


EXTERN_C const IID IID_IAccessibleValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("35855B5B-C566-4fd0-A7B1-E65465600394")
    IAccessibleValue : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_currentValue( 
            /* [retval][out] */ VARIANT *currentValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE setCurrentValue( 
            /* [in] */ VARIANT value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_maximumValue( 
            /* [retval][out] */ VARIANT *maximumValue) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_minimumValue( 
            /* [retval][out] */ VARIANT *minimumValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAccessibleValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAccessibleValue * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAccessibleValue * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAccessibleValue * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_currentValue )( 
            IAccessibleValue * This,
            /* [retval][out] */ VARIANT *currentValue);
        
        HRESULT ( STDMETHODCALLTYPE *setCurrentValue )( 
            IAccessibleValue * This,
            /* [in] */ VARIANT value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_maximumValue )( 
            IAccessibleValue * This,
            /* [retval][out] */ VARIANT *maximumValue);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_minimumValue )( 
            IAccessibleValue * This,
            /* [retval][out] */ VARIANT *minimumValue);
        
        END_INTERFACE
    } IAccessibleValueVtbl;

    interface IAccessibleValue
    {
        CONST_VTBL struct IAccessibleValueVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAccessibleValue_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IAccessibleValue_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IAccessibleValue_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IAccessibleValue_get_currentValue(This,currentValue)	\
    ( (This)->lpVtbl -> get_currentValue(This,currentValue) ) 

#define IAccessibleValue_setCurrentValue(This,value)	\
    ( (This)->lpVtbl -> setCurrentValue(This,value) ) 

#define IAccessibleValue_get_maximumValue(This,maximumValue)	\
    ( (This)->lpVtbl -> get_maximumValue(This,maximumValue) ) 

#define IAccessibleValue_get_minimumValue(This,minimumValue)	\
    ( (This)->lpVtbl -> get_minimumValue(This,minimumValue) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IAccessibleValue_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


