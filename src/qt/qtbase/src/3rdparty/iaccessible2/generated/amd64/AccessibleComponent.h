

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Thu Mar 08 14:53:57 2012
 */
/* Compiler settings for AccessibleComponent.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 7.00.0555 
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

#ifndef __AccessibleComponent_h__
#define __AccessibleComponent_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAccessibleComponent_FWD_DEFINED__
#define __IAccessibleComponent_FWD_DEFINED__
typedef interface IAccessibleComponent IAccessibleComponent;
#endif 	/* __IAccessibleComponent_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oaidl.h"
#include "oleacc.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_AccessibleComponent_0000_0000 */
/* [local] */ 

typedef long IA2Color;



extern RPC_IF_HANDLE __MIDL_itf_AccessibleComponent_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_AccessibleComponent_0000_0000_v0_0_s_ifspec;

#ifndef __IAccessibleComponent_INTERFACE_DEFINED__
#define __IAccessibleComponent_INTERFACE_DEFINED__

/* interface IAccessibleComponent */
/* [uuid][object] */ 


EXTERN_C const IID IID_IAccessibleComponent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1546D4B0-4C98-4bda-89AE-9A64748BDDE4")
    IAccessibleComponent : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_locationInParent( 
            /* [out] */ long *x,
            /* [retval][out] */ long *y) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_foreground( 
            /* [retval][out] */ IA2Color *foreground) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_background( 
            /* [retval][out] */ IA2Color *background) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAccessibleComponentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAccessibleComponent * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAccessibleComponent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAccessibleComponent * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_locationInParent )( 
            IAccessibleComponent * This,
            /* [out] */ long *x,
            /* [retval][out] */ long *y);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_foreground )( 
            IAccessibleComponent * This,
            /* [retval][out] */ IA2Color *foreground);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_background )( 
            IAccessibleComponent * This,
            /* [retval][out] */ IA2Color *background);
        
        END_INTERFACE
    } IAccessibleComponentVtbl;

    interface IAccessibleComponent
    {
        CONST_VTBL struct IAccessibleComponentVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAccessibleComponent_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IAccessibleComponent_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IAccessibleComponent_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IAccessibleComponent_get_locationInParent(This,x,y)	\
    ( (This)->lpVtbl -> get_locationInParent(This,x,y) ) 

#define IAccessibleComponent_get_foreground(This,foreground)	\
    ( (This)->lpVtbl -> get_foreground(This,foreground) ) 

#define IAccessibleComponent_get_background(This,background)	\
    ( (This)->lpVtbl -> get_background(This,background) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IAccessibleComponent_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


