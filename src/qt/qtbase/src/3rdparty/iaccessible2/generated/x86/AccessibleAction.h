

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Thu Mar 08 14:53:51 2012
 */
/* Compiler settings for AccessibleAction.idl:
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

#ifndef __AccessibleAction_h__
#define __AccessibleAction_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAccessibleAction_FWD_DEFINED__
#define __IAccessibleAction_FWD_DEFINED__
typedef interface IAccessibleAction IAccessibleAction;
#endif 	/* __IAccessibleAction_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oaidl.h"
#include "oleacc.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IAccessibleAction_INTERFACE_DEFINED__
#define __IAccessibleAction_INTERFACE_DEFINED__

/* interface IAccessibleAction */
/* [uuid][object] */ 


EXTERN_C const IID IID_IAccessibleAction;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B70D9F59-3B5A-4dba-AB9E-22012F607DF5")
    IAccessibleAction : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE nActions( 
            /* [retval][out] */ long *nActions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE doAction( 
            /* [in] */ long actionIndex) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_description( 
            /* [in] */ long actionIndex,
            /* [retval][out] */ BSTR *description) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_keyBinding( 
            /* [in] */ long actionIndex,
            /* [in] */ long nMaxBindings,
            /* [length_is][length_is][size_is][size_is][out] */ BSTR **keyBindings,
            /* [retval][out] */ long *nBindings) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_name( 
            /* [in] */ long actionIndex,
            /* [retval][out] */ BSTR *name) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_localizedName( 
            /* [in] */ long actionIndex,
            /* [retval][out] */ BSTR *localizedName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAccessibleActionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAccessibleAction * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAccessibleAction * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAccessibleAction * This);
        
        HRESULT ( STDMETHODCALLTYPE *nActions )( 
            IAccessibleAction * This,
            /* [retval][out] */ long *nActions);
        
        HRESULT ( STDMETHODCALLTYPE *doAction )( 
            IAccessibleAction * This,
            /* [in] */ long actionIndex);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_description )( 
            IAccessibleAction * This,
            /* [in] */ long actionIndex,
            /* [retval][out] */ BSTR *description);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_keyBinding )( 
            IAccessibleAction * This,
            /* [in] */ long actionIndex,
            /* [in] */ long nMaxBindings,
            /* [length_is][length_is][size_is][size_is][out] */ BSTR **keyBindings,
            /* [retval][out] */ long *nBindings);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_name )( 
            IAccessibleAction * This,
            /* [in] */ long actionIndex,
            /* [retval][out] */ BSTR *name);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_localizedName )( 
            IAccessibleAction * This,
            /* [in] */ long actionIndex,
            /* [retval][out] */ BSTR *localizedName);
        
        END_INTERFACE
    } IAccessibleActionVtbl;

    interface IAccessibleAction
    {
        CONST_VTBL struct IAccessibleActionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAccessibleAction_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IAccessibleAction_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IAccessibleAction_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IAccessibleAction_nActions(This,nActions)	\
    ( (This)->lpVtbl -> nActions(This,nActions) ) 

#define IAccessibleAction_doAction(This,actionIndex)	\
    ( (This)->lpVtbl -> doAction(This,actionIndex) ) 

#define IAccessibleAction_get_description(This,actionIndex,description)	\
    ( (This)->lpVtbl -> get_description(This,actionIndex,description) ) 

#define IAccessibleAction_get_keyBinding(This,actionIndex,nMaxBindings,keyBindings,nBindings)	\
    ( (This)->lpVtbl -> get_keyBinding(This,actionIndex,nMaxBindings,keyBindings,nBindings) ) 

#define IAccessibleAction_get_name(This,actionIndex,name)	\
    ( (This)->lpVtbl -> get_name(This,actionIndex,name) ) 

#define IAccessibleAction_get_localizedName(This,actionIndex,localizedName)	\
    ( (This)->lpVtbl -> get_localizedName(This,actionIndex,localizedName) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IAccessibleAction_INTERFACE_DEFINED__ */


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


