

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Thu Mar 08 14:53:54 2012
 */
/* Compiler settings for AccessibleRelation.idl:
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

#ifndef __AccessibleRelation_h__
#define __AccessibleRelation_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAccessibleRelation_FWD_DEFINED__
#define __IAccessibleRelation_FWD_DEFINED__
typedef interface IAccessibleRelation IAccessibleRelation;
#endif 	/* __IAccessibleRelation_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oaidl.h"
#include "oleacc.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_AccessibleRelation_0000_0000 */
/* [local] */ 

#define	IA2_RELATION_CONTROLLED_BY	( L"controlledBy" )

#define	IA2_RELATION_CONTROLLER_FOR	( L"controllerFor" )

#define	IA2_RELATION_DESCRIBED_BY	( L"describedBy" )

#define	IA2_RELATION_DESCRIPTION_FOR	( L"descriptionFor" )

#define	IA2_RELATION_EMBEDDED_BY	( L"embeddedBy" )

#define	IA2_RELATION_EMBEDS	( L"embeds" )

#define	IA2_RELATION_FLOWS_FROM	( L"flowsFrom" )

#define	IA2_RELATION_FLOWS_TO	( L"flowsTo" )

#define	IA2_RELATION_LABEL_FOR	( L"labelFor" )

#define	IA2_RELATION_LABELED_BY	( L"labelledBy" )

#define	IA2_RELATION_LABELLED_BY	( L"labelledBy" )

#define	IA2_RELATION_MEMBER_OF	( L"memberOf" )

#define	IA2_RELATION_NODE_CHILD_OF	( L"nodeChildOf" )

#define	IA2_RELATION_PARENT_WINDOW_OF	( L"parentWindowOf" )

#define	IA2_RELATION_POPUP_FOR	( L"popupFor" )

#define	IA2_RELATION_SUBWINDOW_OF	( L"subwindowOf" )



extern RPC_IF_HANDLE __MIDL_itf_AccessibleRelation_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_AccessibleRelation_0000_0000_v0_0_s_ifspec;

#ifndef __IAccessibleRelation_INTERFACE_DEFINED__
#define __IAccessibleRelation_INTERFACE_DEFINED__

/* interface IAccessibleRelation */
/* [uuid][object] */ 


EXTERN_C const IID IID_IAccessibleRelation;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7CDF86EE-C3DA-496a-BDA4-281B336E1FDC")
    IAccessibleRelation : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_relationType( 
            /* [retval][out] */ BSTR *relationType) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_localizedRelationType( 
            /* [retval][out] */ BSTR *localizedRelationType) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_nTargets( 
            /* [retval][out] */ long *nTargets) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_target( 
            /* [in] */ long targetIndex,
            /* [retval][out] */ IUnknown **target) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_targets( 
            /* [in] */ long maxTargets,
            /* [length_is][size_is][out] */ IUnknown **targets,
            /* [retval][out] */ long *nTargets) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAccessibleRelationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAccessibleRelation * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAccessibleRelation * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAccessibleRelation * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_relationType )( 
            IAccessibleRelation * This,
            /* [retval][out] */ BSTR *relationType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_localizedRelationType )( 
            IAccessibleRelation * This,
            /* [retval][out] */ BSTR *localizedRelationType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_nTargets )( 
            IAccessibleRelation * This,
            /* [retval][out] */ long *nTargets);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_target )( 
            IAccessibleRelation * This,
            /* [in] */ long targetIndex,
            /* [retval][out] */ IUnknown **target);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_targets )( 
            IAccessibleRelation * This,
            /* [in] */ long maxTargets,
            /* [length_is][size_is][out] */ IUnknown **targets,
            /* [retval][out] */ long *nTargets);
        
        END_INTERFACE
    } IAccessibleRelationVtbl;

    interface IAccessibleRelation
    {
        CONST_VTBL struct IAccessibleRelationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAccessibleRelation_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IAccessibleRelation_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IAccessibleRelation_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IAccessibleRelation_get_relationType(This,relationType)	\
    ( (This)->lpVtbl -> get_relationType(This,relationType) ) 

#define IAccessibleRelation_get_localizedRelationType(This,localizedRelationType)	\
    ( (This)->lpVtbl -> get_localizedRelationType(This,localizedRelationType) ) 

#define IAccessibleRelation_get_nTargets(This,nTargets)	\
    ( (This)->lpVtbl -> get_nTargets(This,nTargets) ) 

#define IAccessibleRelation_get_target(This,targetIndex,target)	\
    ( (This)->lpVtbl -> get_target(This,targetIndex,target) ) 

#define IAccessibleRelation_get_targets(This,maxTargets,targets,nTargets)	\
    ( (This)->lpVtbl -> get_targets(This,maxTargets,targets,nTargets) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IAccessibleRelation_INTERFACE_DEFINED__ */


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


