

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Thu Mar 08 14:53:52 2012
 */
/* Compiler settings for AccessibleApplication.idl:
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

#ifndef __AccessibleApplication_h__
#define __AccessibleApplication_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAccessibleApplication_FWD_DEFINED__
#define __IAccessibleApplication_FWD_DEFINED__
typedef interface IAccessibleApplication IAccessibleApplication;
#endif 	/* __IAccessibleApplication_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oaidl.h"
#include "oleacc.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IAccessibleApplication_INTERFACE_DEFINED__
#define __IAccessibleApplication_INTERFACE_DEFINED__

/* interface IAccessibleApplication */
/* [uuid][object] */ 


EXTERN_C const IID IID_IAccessibleApplication;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D49DED83-5B25-43F4-9B95-93B44595979E")
    IAccessibleApplication : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_appName( 
            /* [retval][out] */ BSTR *name) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_appVersion( 
            /* [retval][out] */ BSTR *version) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_toolkitName( 
            /* [retval][out] */ BSTR *name) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_toolkitVersion( 
            /* [retval][out] */ BSTR *version) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAccessibleApplicationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAccessibleApplication * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAccessibleApplication * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAccessibleApplication * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_appName )( 
            IAccessibleApplication * This,
            /* [retval][out] */ BSTR *name);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_appVersion )( 
            IAccessibleApplication * This,
            /* [retval][out] */ BSTR *version);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_toolkitName )( 
            IAccessibleApplication * This,
            /* [retval][out] */ BSTR *name);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_toolkitVersion )( 
            IAccessibleApplication * This,
            /* [retval][out] */ BSTR *version);
        
        END_INTERFACE
    } IAccessibleApplicationVtbl;

    interface IAccessibleApplication
    {
        CONST_VTBL struct IAccessibleApplicationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAccessibleApplication_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IAccessibleApplication_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IAccessibleApplication_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IAccessibleApplication_get_appName(This,name)	\
    ( (This)->lpVtbl -> get_appName(This,name) ) 

#define IAccessibleApplication_get_appVersion(This,version)	\
    ( (This)->lpVtbl -> get_appVersion(This,version) ) 

#define IAccessibleApplication_get_toolkitName(This,name)	\
    ( (This)->lpVtbl -> get_toolkitName(This,name) ) 

#define IAccessibleApplication_get_toolkitVersion(This,version)	\
    ( (This)->lpVtbl -> get_toolkitVersion(This,version) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IAccessibleApplication_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


