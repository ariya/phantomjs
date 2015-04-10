

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Thu Mar 08 14:53:53 2012
 */
/* Compiler settings for AccessibleEditableText.idl:
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

#ifndef __AccessibleEditableText_h__
#define __AccessibleEditableText_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAccessibleEditableText_FWD_DEFINED__
#define __IAccessibleEditableText_FWD_DEFINED__
typedef interface IAccessibleEditableText IAccessibleEditableText;
#endif 	/* __IAccessibleEditableText_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oaidl.h"
#include "oleacc.h"
#include "IA2CommonTypes.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IAccessibleEditableText_INTERFACE_DEFINED__
#define __IAccessibleEditableText_INTERFACE_DEFINED__

/* interface IAccessibleEditableText */
/* [uuid][object] */ 


EXTERN_C const IID IID_IAccessibleEditableText;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A59AA09A-7011-4b65-939D-32B1FB5547E3")
    IAccessibleEditableText : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE copyText( 
            /* [in] */ long startOffset,
            /* [in] */ long endOffset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE deleteText( 
            /* [in] */ long startOffset,
            /* [in] */ long endOffset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE insertText( 
            /* [in] */ long offset,
            /* [in] */ BSTR *text) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE cutText( 
            /* [in] */ long startOffset,
            /* [in] */ long endOffset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE pasteText( 
            /* [in] */ long offset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE replaceText( 
            /* [in] */ long startOffset,
            /* [in] */ long endOffset,
            /* [in] */ BSTR *text) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE setAttributes( 
            /* [in] */ long startOffset,
            /* [in] */ long endOffset,
            /* [in] */ BSTR *attributes) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAccessibleEditableTextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAccessibleEditableText * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAccessibleEditableText * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAccessibleEditableText * This);
        
        HRESULT ( STDMETHODCALLTYPE *copyText )( 
            IAccessibleEditableText * This,
            /* [in] */ long startOffset,
            /* [in] */ long endOffset);
        
        HRESULT ( STDMETHODCALLTYPE *deleteText )( 
            IAccessibleEditableText * This,
            /* [in] */ long startOffset,
            /* [in] */ long endOffset);
        
        HRESULT ( STDMETHODCALLTYPE *insertText )( 
            IAccessibleEditableText * This,
            /* [in] */ long offset,
            /* [in] */ BSTR *text);
        
        HRESULT ( STDMETHODCALLTYPE *cutText )( 
            IAccessibleEditableText * This,
            /* [in] */ long startOffset,
            /* [in] */ long endOffset);
        
        HRESULT ( STDMETHODCALLTYPE *pasteText )( 
            IAccessibleEditableText * This,
            /* [in] */ long offset);
        
        HRESULT ( STDMETHODCALLTYPE *replaceText )( 
            IAccessibleEditableText * This,
            /* [in] */ long startOffset,
            /* [in] */ long endOffset,
            /* [in] */ BSTR *text);
        
        HRESULT ( STDMETHODCALLTYPE *setAttributes )( 
            IAccessibleEditableText * This,
            /* [in] */ long startOffset,
            /* [in] */ long endOffset,
            /* [in] */ BSTR *attributes);
        
        END_INTERFACE
    } IAccessibleEditableTextVtbl;

    interface IAccessibleEditableText
    {
        CONST_VTBL struct IAccessibleEditableTextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAccessibleEditableText_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IAccessibleEditableText_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IAccessibleEditableText_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IAccessibleEditableText_copyText(This,startOffset,endOffset)	\
    ( (This)->lpVtbl -> copyText(This,startOffset,endOffset) ) 

#define IAccessibleEditableText_deleteText(This,startOffset,endOffset)	\
    ( (This)->lpVtbl -> deleteText(This,startOffset,endOffset) ) 

#define IAccessibleEditableText_insertText(This,offset,text)	\
    ( (This)->lpVtbl -> insertText(This,offset,text) ) 

#define IAccessibleEditableText_cutText(This,startOffset,endOffset)	\
    ( (This)->lpVtbl -> cutText(This,startOffset,endOffset) ) 

#define IAccessibleEditableText_pasteText(This,offset)	\
    ( (This)->lpVtbl -> pasteText(This,offset) ) 

#define IAccessibleEditableText_replaceText(This,startOffset,endOffset,text)	\
    ( (This)->lpVtbl -> replaceText(This,startOffset,endOffset,text) ) 

#define IAccessibleEditableText_setAttributes(This,startOffset,endOffset,attributes)	\
    ( (This)->lpVtbl -> setAttributes(This,startOffset,endOffset,attributes) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IAccessibleEditableText_INTERFACE_DEFINED__ */


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


