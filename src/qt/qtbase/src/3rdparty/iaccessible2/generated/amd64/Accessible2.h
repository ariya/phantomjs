

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Thu Mar 08 14:53:56 2012
 */
/* Compiler settings for Accessible2.idl:
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

#ifndef __Accessible2_h__
#define __Accessible2_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAccessible2_FWD_DEFINED__
#define __IAccessible2_FWD_DEFINED__
typedef interface IAccessible2 IAccessible2;
#endif 	/* __IAccessible2_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oaidl.h"
#include "oleacc.h"
#include "AccessibleRelation.h"
#include "AccessibleStates.h"
#include "IA2CommonTypes.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_Accessible2_0000_0000 */
/* [local] */ 

typedef struct IA2Locale
    {
    BSTR language;
    BSTR country;
    BSTR variant;
    } 	IA2Locale;



extern RPC_IF_HANDLE __MIDL_itf_Accessible2_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_Accessible2_0000_0000_v0_0_s_ifspec;

#ifndef __IAccessible2_INTERFACE_DEFINED__
#define __IAccessible2_INTERFACE_DEFINED__

/* interface IAccessible2 */
/* [uuid][object] */ 


EXTERN_C const IID IID_IAccessible2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E89F726E-C4F4-4c19-BB19-B647D7FA8478")
    IAccessible2 : public IAccessible
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_nRelations( 
            /* [retval][out] */ long *nRelations) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_relation( 
            /* [in] */ long relationIndex,
            /* [retval][out] */ IAccessibleRelation **relation) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_relations( 
            /* [in] */ long maxRelations,
            /* [length_is][size_is][out] */ IAccessibleRelation **relations,
            /* [retval][out] */ long *nRelations) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE role( 
            /* [retval][out] */ long *role) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE scrollTo( 
            /* [in] */ enum IA2ScrollType scrollType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE scrollToPoint( 
            /* [in] */ enum IA2CoordinateType coordinateType,
            /* [in] */ long x,
            /* [in] */ long y) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_groupPosition( 
            /* [out] */ long *groupLevel,
            /* [out] */ long *similarItemsInGroup,
            /* [retval][out] */ long *positionInGroup) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_states( 
            /* [retval][out] */ AccessibleStates *states) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_extendedRole( 
            /* [retval][out] */ BSTR *extendedRole) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_localizedExtendedRole( 
            /* [retval][out] */ BSTR *localizedExtendedRole) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_nExtendedStates( 
            /* [retval][out] */ long *nExtendedStates) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_extendedStates( 
            /* [in] */ long maxExtendedStates,
            /* [length_is][length_is][size_is][size_is][out] */ BSTR **extendedStates,
            /* [retval][out] */ long *nExtendedStates) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_localizedExtendedStates( 
            /* [in] */ long maxLocalizedExtendedStates,
            /* [length_is][length_is][size_is][size_is][out] */ BSTR **localizedExtendedStates,
            /* [retval][out] */ long *nLocalizedExtendedStates) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_uniqueID( 
            /* [retval][out] */ long *uniqueID) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_windowHandle( 
            /* [retval][out] */ HWND *windowHandle) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_indexInParent( 
            /* [retval][out] */ long *indexInParent) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_locale( 
            /* [retval][out] */ IA2Locale *locale) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_attributes( 
            /* [retval][out] */ BSTR *attributes) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAccessible2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAccessible2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAccessible2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAccessible2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAccessible2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAccessible2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAccessible2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAccessible2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_accParent )( 
            IAccessible2 * This,
            /* [retval][out] */ IDispatch **ppdispParent);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_accChildCount )( 
            IAccessible2 * This,
            /* [retval][out] */ long *pcountChildren);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_accChild )( 
            IAccessible2 * This,
            /* [in] */ VARIANT varChild,
            /* [retval][out] */ IDispatch **ppdispChild);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_accName )( 
            IAccessible2 * This,
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ BSTR *pszName);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_accValue )( 
            IAccessible2 * This,
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ BSTR *pszValue);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_accDescription )( 
            IAccessible2 * This,
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ BSTR *pszDescription);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_accRole )( 
            IAccessible2 * This,
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ VARIANT *pvarRole);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_accState )( 
            IAccessible2 * This,
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ VARIANT *pvarState);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_accHelp )( 
            IAccessible2 * This,
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ BSTR *pszHelp);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_accHelpTopic )( 
            IAccessible2 * This,
            /* [out] */ BSTR *pszHelpFile,
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ long *pidTopic);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_accKeyboardShortcut )( 
            IAccessible2 * This,
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ BSTR *pszKeyboardShortcut);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_accFocus )( 
            IAccessible2 * This,
            /* [retval][out] */ VARIANT *pvarChild);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_accSelection )( 
            IAccessible2 * This,
            /* [retval][out] */ VARIANT *pvarChildren);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE *get_accDefaultAction )( 
            IAccessible2 * This,
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ BSTR *pszDefaultAction);
        
        /* [id][hidden] */ HRESULT ( STDMETHODCALLTYPE *accSelect )( 
            IAccessible2 * This,
            /* [in] */ long flagsSelect,
            /* [optional][in] */ VARIANT varChild);
        
        /* [id][hidden] */ HRESULT ( STDMETHODCALLTYPE *accLocation )( 
            IAccessible2 * This,
            /* [out] */ long *pxLeft,
            /* [out] */ long *pyTop,
            /* [out] */ long *pcxWidth,
            /* [out] */ long *pcyHeight,
            /* [optional][in] */ VARIANT varChild);
        
        /* [id][hidden] */ HRESULT ( STDMETHODCALLTYPE *accNavigate )( 
            IAccessible2 * This,
            /* [in] */ long navDir,
            /* [optional][in] */ VARIANT varStart,
            /* [retval][out] */ VARIANT *pvarEndUpAt);
        
        /* [id][hidden] */ HRESULT ( STDMETHODCALLTYPE *accHitTest )( 
            IAccessible2 * This,
            /* [in] */ long xLeft,
            /* [in] */ long yTop,
            /* [retval][out] */ VARIANT *pvarChild);
        
        /* [id][hidden] */ HRESULT ( STDMETHODCALLTYPE *accDoDefaultAction )( 
            IAccessible2 * This,
            /* [optional][in] */ VARIANT varChild);
        
        /* [id][propput][hidden] */ HRESULT ( STDMETHODCALLTYPE *put_accName )( 
            IAccessible2 * This,
            /* [optional][in] */ VARIANT varChild,
            /* [in] */ BSTR szName);
        
        /* [id][propput][hidden] */ HRESULT ( STDMETHODCALLTYPE *put_accValue )( 
            IAccessible2 * This,
            /* [optional][in] */ VARIANT varChild,
            /* [in] */ BSTR szValue);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_nRelations )( 
            IAccessible2 * This,
            /* [retval][out] */ long *nRelations);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_relation )( 
            IAccessible2 * This,
            /* [in] */ long relationIndex,
            /* [retval][out] */ IAccessibleRelation **relation);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_relations )( 
            IAccessible2 * This,
            /* [in] */ long maxRelations,
            /* [length_is][size_is][out] */ IAccessibleRelation **relations,
            /* [retval][out] */ long *nRelations);
        
        HRESULT ( STDMETHODCALLTYPE *role )( 
            IAccessible2 * This,
            /* [retval][out] */ long *role);
        
        HRESULT ( STDMETHODCALLTYPE *scrollTo )( 
            IAccessible2 * This,
            /* [in] */ enum IA2ScrollType scrollType);
        
        HRESULT ( STDMETHODCALLTYPE *scrollToPoint )( 
            IAccessible2 * This,
            /* [in] */ enum IA2CoordinateType coordinateType,
            /* [in] */ long x,
            /* [in] */ long y);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_groupPosition )( 
            IAccessible2 * This,
            /* [out] */ long *groupLevel,
            /* [out] */ long *similarItemsInGroup,
            /* [retval][out] */ long *positionInGroup);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_states )( 
            IAccessible2 * This,
            /* [retval][out] */ AccessibleStates *states);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_extendedRole )( 
            IAccessible2 * This,
            /* [retval][out] */ BSTR *extendedRole);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_localizedExtendedRole )( 
            IAccessible2 * This,
            /* [retval][out] */ BSTR *localizedExtendedRole);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_nExtendedStates )( 
            IAccessible2 * This,
            /* [retval][out] */ long *nExtendedStates);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_extendedStates )( 
            IAccessible2 * This,
            /* [in] */ long maxExtendedStates,
            /* [length_is][length_is][size_is][size_is][out] */ BSTR **extendedStates,
            /* [retval][out] */ long *nExtendedStates);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_localizedExtendedStates )( 
            IAccessible2 * This,
            /* [in] */ long maxLocalizedExtendedStates,
            /* [length_is][length_is][size_is][size_is][out] */ BSTR **localizedExtendedStates,
            /* [retval][out] */ long *nLocalizedExtendedStates);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_uniqueID )( 
            IAccessible2 * This,
            /* [retval][out] */ long *uniqueID);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_windowHandle )( 
            IAccessible2 * This,
            /* [retval][out] */ HWND *windowHandle);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_indexInParent )( 
            IAccessible2 * This,
            /* [retval][out] */ long *indexInParent);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_locale )( 
            IAccessible2 * This,
            /* [retval][out] */ IA2Locale *locale);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_attributes )( 
            IAccessible2 * This,
            /* [retval][out] */ BSTR *attributes);
        
        END_INTERFACE
    } IAccessible2Vtbl;

    interface IAccessible2
    {
        CONST_VTBL struct IAccessible2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAccessible2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IAccessible2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IAccessible2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IAccessible2_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IAccessible2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IAccessible2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IAccessible2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IAccessible2_get_accParent(This,ppdispParent)	\
    ( (This)->lpVtbl -> get_accParent(This,ppdispParent) ) 

#define IAccessible2_get_accChildCount(This,pcountChildren)	\
    ( (This)->lpVtbl -> get_accChildCount(This,pcountChildren) ) 

#define IAccessible2_get_accChild(This,varChild,ppdispChild)	\
    ( (This)->lpVtbl -> get_accChild(This,varChild,ppdispChild) ) 

#define IAccessible2_get_accName(This,varChild,pszName)	\
    ( (This)->lpVtbl -> get_accName(This,varChild,pszName) ) 

#define IAccessible2_get_accValue(This,varChild,pszValue)	\
    ( (This)->lpVtbl -> get_accValue(This,varChild,pszValue) ) 

#define IAccessible2_get_accDescription(This,varChild,pszDescription)	\
    ( (This)->lpVtbl -> get_accDescription(This,varChild,pszDescription) ) 

#define IAccessible2_get_accRole(This,varChild,pvarRole)	\
    ( (This)->lpVtbl -> get_accRole(This,varChild,pvarRole) ) 

#define IAccessible2_get_accState(This,varChild,pvarState)	\
    ( (This)->lpVtbl -> get_accState(This,varChild,pvarState) ) 

#define IAccessible2_get_accHelp(This,varChild,pszHelp)	\
    ( (This)->lpVtbl -> get_accHelp(This,varChild,pszHelp) ) 

#define IAccessible2_get_accHelpTopic(This,pszHelpFile,varChild,pidTopic)	\
    ( (This)->lpVtbl -> get_accHelpTopic(This,pszHelpFile,varChild,pidTopic) ) 

#define IAccessible2_get_accKeyboardShortcut(This,varChild,pszKeyboardShortcut)	\
    ( (This)->lpVtbl -> get_accKeyboardShortcut(This,varChild,pszKeyboardShortcut) ) 

#define IAccessible2_get_accFocus(This,pvarChild)	\
    ( (This)->lpVtbl -> get_accFocus(This,pvarChild) ) 

#define IAccessible2_get_accSelection(This,pvarChildren)	\
    ( (This)->lpVtbl -> get_accSelection(This,pvarChildren) ) 

#define IAccessible2_get_accDefaultAction(This,varChild,pszDefaultAction)	\
    ( (This)->lpVtbl -> get_accDefaultAction(This,varChild,pszDefaultAction) ) 

#define IAccessible2_accSelect(This,flagsSelect,varChild)	\
    ( (This)->lpVtbl -> accSelect(This,flagsSelect,varChild) ) 

#define IAccessible2_accLocation(This,pxLeft,pyTop,pcxWidth,pcyHeight,varChild)	\
    ( (This)->lpVtbl -> accLocation(This,pxLeft,pyTop,pcxWidth,pcyHeight,varChild) ) 

#define IAccessible2_accNavigate(This,navDir,varStart,pvarEndUpAt)	\
    ( (This)->lpVtbl -> accNavigate(This,navDir,varStart,pvarEndUpAt) ) 

#define IAccessible2_accHitTest(This,xLeft,yTop,pvarChild)	\
    ( (This)->lpVtbl -> accHitTest(This,xLeft,yTop,pvarChild) ) 

#define IAccessible2_accDoDefaultAction(This,varChild)	\
    ( (This)->lpVtbl -> accDoDefaultAction(This,varChild) ) 

#define IAccessible2_put_accName(This,varChild,szName)	\
    ( (This)->lpVtbl -> put_accName(This,varChild,szName) ) 

#define IAccessible2_put_accValue(This,varChild,szValue)	\
    ( (This)->lpVtbl -> put_accValue(This,varChild,szValue) ) 


#define IAccessible2_get_nRelations(This,nRelations)	\
    ( (This)->lpVtbl -> get_nRelations(This,nRelations) ) 

#define IAccessible2_get_relation(This,relationIndex,relation)	\
    ( (This)->lpVtbl -> get_relation(This,relationIndex,relation) ) 

#define IAccessible2_get_relations(This,maxRelations,relations,nRelations)	\
    ( (This)->lpVtbl -> get_relations(This,maxRelations,relations,nRelations) ) 

#define IAccessible2_role(This,role)	\
    ( (This)->lpVtbl -> role(This,role) ) 

#define IAccessible2_scrollTo(This,scrollType)	\
    ( (This)->lpVtbl -> scrollTo(This,scrollType) ) 

#define IAccessible2_scrollToPoint(This,coordinateType,x,y)	\
    ( (This)->lpVtbl -> scrollToPoint(This,coordinateType,x,y) ) 

#define IAccessible2_get_groupPosition(This,groupLevel,similarItemsInGroup,positionInGroup)	\
    ( (This)->lpVtbl -> get_groupPosition(This,groupLevel,similarItemsInGroup,positionInGroup) ) 

#define IAccessible2_get_states(This,states)	\
    ( (This)->lpVtbl -> get_states(This,states) ) 

#define IAccessible2_get_extendedRole(This,extendedRole)	\
    ( (This)->lpVtbl -> get_extendedRole(This,extendedRole) ) 

#define IAccessible2_get_localizedExtendedRole(This,localizedExtendedRole)	\
    ( (This)->lpVtbl -> get_localizedExtendedRole(This,localizedExtendedRole) ) 

#define IAccessible2_get_nExtendedStates(This,nExtendedStates)	\
    ( (This)->lpVtbl -> get_nExtendedStates(This,nExtendedStates) ) 

#define IAccessible2_get_extendedStates(This,maxExtendedStates,extendedStates,nExtendedStates)	\
    ( (This)->lpVtbl -> get_extendedStates(This,maxExtendedStates,extendedStates,nExtendedStates) ) 

#define IAccessible2_get_localizedExtendedStates(This,maxLocalizedExtendedStates,localizedExtendedStates,nLocalizedExtendedStates)	\
    ( (This)->lpVtbl -> get_localizedExtendedStates(This,maxLocalizedExtendedStates,localizedExtendedStates,nLocalizedExtendedStates) ) 

#define IAccessible2_get_uniqueID(This,uniqueID)	\
    ( (This)->lpVtbl -> get_uniqueID(This,uniqueID) ) 

#define IAccessible2_get_windowHandle(This,windowHandle)	\
    ( (This)->lpVtbl -> get_windowHandle(This,windowHandle) ) 

#define IAccessible2_get_indexInParent(This,indexInParent)	\
    ( (This)->lpVtbl -> get_indexInParent(This,indexInParent) ) 

#define IAccessible2_get_locale(This,locale)	\
    ( (This)->lpVtbl -> get_locale(This,locale) ) 

#define IAccessible2_get_attributes(This,attributes)	\
    ( (This)->lpVtbl -> get_attributes(This,attributes) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IAccessible2_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long *, unsigned long            , HWND * ); 
unsigned char * __RPC_USER  HWND_UserMarshal(  unsigned long *, unsigned char *, HWND * ); 
unsigned char * __RPC_USER  HWND_UserUnmarshal(unsigned long *, unsigned char *, HWND * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long *, HWND * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


