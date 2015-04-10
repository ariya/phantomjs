

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Thu Mar 08 14:54:00 2012
 */
/* Compiler settings for AccessibleText.idl:
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

#ifndef __AccessibleText_h__
#define __AccessibleText_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAccessibleText_FWD_DEFINED__
#define __IAccessibleText_FWD_DEFINED__
typedef interface IAccessibleText IAccessibleText;
#endif 	/* __IAccessibleText_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oaidl.h"
#include "oleacc.h"
#include "IA2CommonTypes.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_AccessibleText_0000_0000 */
/* [local] */ 

typedef struct IA2TextSegment
    {
    BSTR text;
    long start;
    long end;
    } 	IA2TextSegment;


enum IA2TextBoundaryType
    {	IA2_TEXT_BOUNDARY_CHAR	= 0,
	IA2_TEXT_BOUNDARY_WORD	= ( IA2_TEXT_BOUNDARY_CHAR + 1 ) ,
	IA2_TEXT_BOUNDARY_SENTENCE	= ( IA2_TEXT_BOUNDARY_WORD + 1 ) ,
	IA2_TEXT_BOUNDARY_PARAGRAPH	= ( IA2_TEXT_BOUNDARY_SENTENCE + 1 ) ,
	IA2_TEXT_BOUNDARY_LINE	= ( IA2_TEXT_BOUNDARY_PARAGRAPH + 1 ) ,
	IA2_TEXT_BOUNDARY_ALL	= ( IA2_TEXT_BOUNDARY_LINE + 1 ) 
    } ;


extern RPC_IF_HANDLE __MIDL_itf_AccessibleText_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_AccessibleText_0000_0000_v0_0_s_ifspec;

#ifndef __IAccessibleText_INTERFACE_DEFINED__
#define __IAccessibleText_INTERFACE_DEFINED__

/* interface IAccessibleText */
/* [uuid][object] */ 


EXTERN_C const IID IID_IAccessibleText;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("24FD2FFB-3AAD-4a08-8335-A3AD89C0FB4B")
    IAccessibleText : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE addSelection( 
            /* [in] */ long startOffset,
            /* [in] */ long endOffset) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_attributes( 
            /* [in] */ long offset,
            /* [out] */ long *startOffset,
            /* [out] */ long *endOffset,
            /* [retval][out] */ BSTR *textAttributes) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_caretOffset( 
            /* [retval][out] */ long *offset) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_characterExtents( 
            /* [in] */ long offset,
            /* [in] */ enum IA2CoordinateType coordType,
            /* [out] */ long *x,
            /* [out] */ long *y,
            /* [out] */ long *width,
            /* [retval][out] */ long *height) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_nSelections( 
            /* [retval][out] */ long *nSelections) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_offsetAtPoint( 
            /* [in] */ long x,
            /* [in] */ long y,
            /* [in] */ enum IA2CoordinateType coordType,
            /* [retval][out] */ long *offset) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_selection( 
            /* [in] */ long selectionIndex,
            /* [out] */ long *startOffset,
            /* [retval][out] */ long *endOffset) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_text( 
            /* [in] */ long startOffset,
            /* [in] */ long endOffset,
            /* [retval][out] */ BSTR *text) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_textBeforeOffset( 
            /* [in] */ long offset,
            /* [in] */ enum IA2TextBoundaryType boundaryType,
            /* [out] */ long *startOffset,
            /* [out] */ long *endOffset,
            /* [retval][out] */ BSTR *text) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_textAfterOffset( 
            /* [in] */ long offset,
            /* [in] */ enum IA2TextBoundaryType boundaryType,
            /* [out] */ long *startOffset,
            /* [out] */ long *endOffset,
            /* [retval][out] */ BSTR *text) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_textAtOffset( 
            /* [in] */ long offset,
            /* [in] */ enum IA2TextBoundaryType boundaryType,
            /* [out] */ long *startOffset,
            /* [out] */ long *endOffset,
            /* [retval][out] */ BSTR *text) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE removeSelection( 
            /* [in] */ long selectionIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE setCaretOffset( 
            /* [in] */ long offset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE setSelection( 
            /* [in] */ long selectionIndex,
            /* [in] */ long startOffset,
            /* [in] */ long endOffset) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_nCharacters( 
            /* [retval][out] */ long *nCharacters) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE scrollSubstringTo( 
            /* [in] */ long startIndex,
            /* [in] */ long endIndex,
            /* [in] */ enum IA2ScrollType scrollType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE scrollSubstringToPoint( 
            /* [in] */ long startIndex,
            /* [in] */ long endIndex,
            /* [in] */ enum IA2CoordinateType coordinateType,
            /* [in] */ long x,
            /* [in] */ long y) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_newText( 
            /* [retval][out] */ IA2TextSegment *newText) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_oldText( 
            /* [retval][out] */ IA2TextSegment *oldText) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAccessibleTextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAccessibleText * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAccessibleText * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAccessibleText * This);
        
        HRESULT ( STDMETHODCALLTYPE *addSelection )( 
            IAccessibleText * This,
            /* [in] */ long startOffset,
            /* [in] */ long endOffset);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_attributes )( 
            IAccessibleText * This,
            /* [in] */ long offset,
            /* [out] */ long *startOffset,
            /* [out] */ long *endOffset,
            /* [retval][out] */ BSTR *textAttributes);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_caretOffset )( 
            IAccessibleText * This,
            /* [retval][out] */ long *offset);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_characterExtents )( 
            IAccessibleText * This,
            /* [in] */ long offset,
            /* [in] */ enum IA2CoordinateType coordType,
            /* [out] */ long *x,
            /* [out] */ long *y,
            /* [out] */ long *width,
            /* [retval][out] */ long *height);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_nSelections )( 
            IAccessibleText * This,
            /* [retval][out] */ long *nSelections);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_offsetAtPoint )( 
            IAccessibleText * This,
            /* [in] */ long x,
            /* [in] */ long y,
            /* [in] */ enum IA2CoordinateType coordType,
            /* [retval][out] */ long *offset);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_selection )( 
            IAccessibleText * This,
            /* [in] */ long selectionIndex,
            /* [out] */ long *startOffset,
            /* [retval][out] */ long *endOffset);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_text )( 
            IAccessibleText * This,
            /* [in] */ long startOffset,
            /* [in] */ long endOffset,
            /* [retval][out] */ BSTR *text);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_textBeforeOffset )( 
            IAccessibleText * This,
            /* [in] */ long offset,
            /* [in] */ enum IA2TextBoundaryType boundaryType,
            /* [out] */ long *startOffset,
            /* [out] */ long *endOffset,
            /* [retval][out] */ BSTR *text);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_textAfterOffset )( 
            IAccessibleText * This,
            /* [in] */ long offset,
            /* [in] */ enum IA2TextBoundaryType boundaryType,
            /* [out] */ long *startOffset,
            /* [out] */ long *endOffset,
            /* [retval][out] */ BSTR *text);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_textAtOffset )( 
            IAccessibleText * This,
            /* [in] */ long offset,
            /* [in] */ enum IA2TextBoundaryType boundaryType,
            /* [out] */ long *startOffset,
            /* [out] */ long *endOffset,
            /* [retval][out] */ BSTR *text);
        
        HRESULT ( STDMETHODCALLTYPE *removeSelection )( 
            IAccessibleText * This,
            /* [in] */ long selectionIndex);
        
        HRESULT ( STDMETHODCALLTYPE *setCaretOffset )( 
            IAccessibleText * This,
            /* [in] */ long offset);
        
        HRESULT ( STDMETHODCALLTYPE *setSelection )( 
            IAccessibleText * This,
            /* [in] */ long selectionIndex,
            /* [in] */ long startOffset,
            /* [in] */ long endOffset);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_nCharacters )( 
            IAccessibleText * This,
            /* [retval][out] */ long *nCharacters);
        
        HRESULT ( STDMETHODCALLTYPE *scrollSubstringTo )( 
            IAccessibleText * This,
            /* [in] */ long startIndex,
            /* [in] */ long endIndex,
            /* [in] */ enum IA2ScrollType scrollType);
        
        HRESULT ( STDMETHODCALLTYPE *scrollSubstringToPoint )( 
            IAccessibleText * This,
            /* [in] */ long startIndex,
            /* [in] */ long endIndex,
            /* [in] */ enum IA2CoordinateType coordinateType,
            /* [in] */ long x,
            /* [in] */ long y);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_newText )( 
            IAccessibleText * This,
            /* [retval][out] */ IA2TextSegment *newText);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_oldText )( 
            IAccessibleText * This,
            /* [retval][out] */ IA2TextSegment *oldText);
        
        END_INTERFACE
    } IAccessibleTextVtbl;

    interface IAccessibleText
    {
        CONST_VTBL struct IAccessibleTextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAccessibleText_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IAccessibleText_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IAccessibleText_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IAccessibleText_addSelection(This,startOffset,endOffset)	\
    ( (This)->lpVtbl -> addSelection(This,startOffset,endOffset) ) 

#define IAccessibleText_get_attributes(This,offset,startOffset,endOffset,textAttributes)	\
    ( (This)->lpVtbl -> get_attributes(This,offset,startOffset,endOffset,textAttributes) ) 

#define IAccessibleText_get_caretOffset(This,offset)	\
    ( (This)->lpVtbl -> get_caretOffset(This,offset) ) 

#define IAccessibleText_get_characterExtents(This,offset,coordType,x,y,width,height)	\
    ( (This)->lpVtbl -> get_characterExtents(This,offset,coordType,x,y,width,height) ) 

#define IAccessibleText_get_nSelections(This,nSelections)	\
    ( (This)->lpVtbl -> get_nSelections(This,nSelections) ) 

#define IAccessibleText_get_offsetAtPoint(This,x,y,coordType,offset)	\
    ( (This)->lpVtbl -> get_offsetAtPoint(This,x,y,coordType,offset) ) 

#define IAccessibleText_get_selection(This,selectionIndex,startOffset,endOffset)	\
    ( (This)->lpVtbl -> get_selection(This,selectionIndex,startOffset,endOffset) ) 

#define IAccessibleText_get_text(This,startOffset,endOffset,text)	\
    ( (This)->lpVtbl -> get_text(This,startOffset,endOffset,text) ) 

#define IAccessibleText_get_textBeforeOffset(This,offset,boundaryType,startOffset,endOffset,text)	\
    ( (This)->lpVtbl -> get_textBeforeOffset(This,offset,boundaryType,startOffset,endOffset,text) ) 

#define IAccessibleText_get_textAfterOffset(This,offset,boundaryType,startOffset,endOffset,text)	\
    ( (This)->lpVtbl -> get_textAfterOffset(This,offset,boundaryType,startOffset,endOffset,text) ) 

#define IAccessibleText_get_textAtOffset(This,offset,boundaryType,startOffset,endOffset,text)	\
    ( (This)->lpVtbl -> get_textAtOffset(This,offset,boundaryType,startOffset,endOffset,text) ) 

#define IAccessibleText_removeSelection(This,selectionIndex)	\
    ( (This)->lpVtbl -> removeSelection(This,selectionIndex) ) 

#define IAccessibleText_setCaretOffset(This,offset)	\
    ( (This)->lpVtbl -> setCaretOffset(This,offset) ) 

#define IAccessibleText_setSelection(This,selectionIndex,startOffset,endOffset)	\
    ( (This)->lpVtbl -> setSelection(This,selectionIndex,startOffset,endOffset) ) 

#define IAccessibleText_get_nCharacters(This,nCharacters)	\
    ( (This)->lpVtbl -> get_nCharacters(This,nCharacters) ) 

#define IAccessibleText_scrollSubstringTo(This,startIndex,endIndex,scrollType)	\
    ( (This)->lpVtbl -> scrollSubstringTo(This,startIndex,endIndex,scrollType) ) 

#define IAccessibleText_scrollSubstringToPoint(This,startIndex,endIndex,coordinateType,x,y)	\
    ( (This)->lpVtbl -> scrollSubstringToPoint(This,startIndex,endIndex,coordinateType,x,y) ) 

#define IAccessibleText_get_newText(This,newText)	\
    ( (This)->lpVtbl -> get_newText(This,newText) ) 

#define IAccessibleText_get_oldText(This,oldText)	\
    ( (This)->lpVtbl -> get_oldText(This,oldText) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IAccessibleText_INTERFACE_DEFINED__ */


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


