

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Thu Mar 08 14:53:55 2012
 */
/* Compiler settings for AccessibleTable2.idl:
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

#ifndef __AccessibleTable2_h__
#define __AccessibleTable2_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAccessibleTable2_FWD_DEFINED__
#define __IAccessibleTable2_FWD_DEFINED__
typedef interface IAccessibleTable2 IAccessibleTable2;
#endif 	/* __IAccessibleTable2_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oaidl.h"
#include "oleacc.h"
#include "Accessible2.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IAccessibleTable2_INTERFACE_DEFINED__
#define __IAccessibleTable2_INTERFACE_DEFINED__

/* interface IAccessibleTable2 */
/* [uuid][object] */ 


EXTERN_C const IID IID_IAccessibleTable2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6167f295-06f0-4cdd-a1fa-02e25153d869")
    IAccessibleTable2 : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_cellAt( 
            /* [in] */ long row,
            /* [in] */ long column,
            /* [retval][out] */ IUnknown **cell) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_caption( 
            /* [retval][out] */ IUnknown **accessible) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_columnDescription( 
            /* [in] */ long column,
            /* [retval][out] */ BSTR *description) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_nColumns( 
            /* [retval][out] */ long *columnCount) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_nRows( 
            /* [retval][out] */ long *rowCount) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_nSelectedCells( 
            /* [retval][out] */ long *cellCount) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_nSelectedColumns( 
            /* [retval][out] */ long *columnCount) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_nSelectedRows( 
            /* [retval][out] */ long *rowCount) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_rowDescription( 
            /* [in] */ long row,
            /* [retval][out] */ BSTR *description) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_selectedCells( 
            /* [size_is][size_is][size_is][out] */ IUnknown ***cells,
            /* [retval][out] */ long *nSelectedCells) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_selectedColumns( 
            /* [size_is][size_is][out] */ long **selectedColumns,
            /* [retval][out] */ long *nColumns) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_selectedRows( 
            /* [size_is][size_is][out] */ long **selectedRows,
            /* [retval][out] */ long *nRows) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_summary( 
            /* [retval][out] */ IUnknown **accessible) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_isColumnSelected( 
            /* [in] */ long column,
            /* [retval][out] */ boolean *isSelected) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_isRowSelected( 
            /* [in] */ long row,
            /* [retval][out] */ boolean *isSelected) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE selectRow( 
            /* [in] */ long row) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE selectColumn( 
            /* [in] */ long column) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE unselectRow( 
            /* [in] */ long row) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE unselectColumn( 
            /* [in] */ long column) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_modelChange( 
            /* [retval][out] */ IA2TableModelChange *modelChange) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAccessibleTable2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAccessibleTable2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAccessibleTable2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAccessibleTable2 * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_cellAt )( 
            IAccessibleTable2 * This,
            /* [in] */ long row,
            /* [in] */ long column,
            /* [retval][out] */ IUnknown **cell);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_caption )( 
            IAccessibleTable2 * This,
            /* [retval][out] */ IUnknown **accessible);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_columnDescription )( 
            IAccessibleTable2 * This,
            /* [in] */ long column,
            /* [retval][out] */ BSTR *description);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_nColumns )( 
            IAccessibleTable2 * This,
            /* [retval][out] */ long *columnCount);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_nRows )( 
            IAccessibleTable2 * This,
            /* [retval][out] */ long *rowCount);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_nSelectedCells )( 
            IAccessibleTable2 * This,
            /* [retval][out] */ long *cellCount);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_nSelectedColumns )( 
            IAccessibleTable2 * This,
            /* [retval][out] */ long *columnCount);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_nSelectedRows )( 
            IAccessibleTable2 * This,
            /* [retval][out] */ long *rowCount);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_rowDescription )( 
            IAccessibleTable2 * This,
            /* [in] */ long row,
            /* [retval][out] */ BSTR *description);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_selectedCells )( 
            IAccessibleTable2 * This,
            /* [size_is][size_is][size_is][out] */ IUnknown ***cells,
            /* [retval][out] */ long *nSelectedCells);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_selectedColumns )( 
            IAccessibleTable2 * This,
            /* [size_is][size_is][out] */ long **selectedColumns,
            /* [retval][out] */ long *nColumns);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_selectedRows )( 
            IAccessibleTable2 * This,
            /* [size_is][size_is][out] */ long **selectedRows,
            /* [retval][out] */ long *nRows);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_summary )( 
            IAccessibleTable2 * This,
            /* [retval][out] */ IUnknown **accessible);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_isColumnSelected )( 
            IAccessibleTable2 * This,
            /* [in] */ long column,
            /* [retval][out] */ boolean *isSelected);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_isRowSelected )( 
            IAccessibleTable2 * This,
            /* [in] */ long row,
            /* [retval][out] */ boolean *isSelected);
        
        HRESULT ( STDMETHODCALLTYPE *selectRow )( 
            IAccessibleTable2 * This,
            /* [in] */ long row);
        
        HRESULT ( STDMETHODCALLTYPE *selectColumn )( 
            IAccessibleTable2 * This,
            /* [in] */ long column);
        
        HRESULT ( STDMETHODCALLTYPE *unselectRow )( 
            IAccessibleTable2 * This,
            /* [in] */ long row);
        
        HRESULT ( STDMETHODCALLTYPE *unselectColumn )( 
            IAccessibleTable2 * This,
            /* [in] */ long column);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_modelChange )( 
            IAccessibleTable2 * This,
            /* [retval][out] */ IA2TableModelChange *modelChange);
        
        END_INTERFACE
    } IAccessibleTable2Vtbl;

    interface IAccessibleTable2
    {
        CONST_VTBL struct IAccessibleTable2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAccessibleTable2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IAccessibleTable2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IAccessibleTable2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IAccessibleTable2_get_cellAt(This,row,column,cell)	\
    ( (This)->lpVtbl -> get_cellAt(This,row,column,cell) ) 

#define IAccessibleTable2_get_caption(This,accessible)	\
    ( (This)->lpVtbl -> get_caption(This,accessible) ) 

#define IAccessibleTable2_get_columnDescription(This,column,description)	\
    ( (This)->lpVtbl -> get_columnDescription(This,column,description) ) 

#define IAccessibleTable2_get_nColumns(This,columnCount)	\
    ( (This)->lpVtbl -> get_nColumns(This,columnCount) ) 

#define IAccessibleTable2_get_nRows(This,rowCount)	\
    ( (This)->lpVtbl -> get_nRows(This,rowCount) ) 

#define IAccessibleTable2_get_nSelectedCells(This,cellCount)	\
    ( (This)->lpVtbl -> get_nSelectedCells(This,cellCount) ) 

#define IAccessibleTable2_get_nSelectedColumns(This,columnCount)	\
    ( (This)->lpVtbl -> get_nSelectedColumns(This,columnCount) ) 

#define IAccessibleTable2_get_nSelectedRows(This,rowCount)	\
    ( (This)->lpVtbl -> get_nSelectedRows(This,rowCount) ) 

#define IAccessibleTable2_get_rowDescription(This,row,description)	\
    ( (This)->lpVtbl -> get_rowDescription(This,row,description) ) 

#define IAccessibleTable2_get_selectedCells(This,cells,nSelectedCells)	\
    ( (This)->lpVtbl -> get_selectedCells(This,cells,nSelectedCells) ) 

#define IAccessibleTable2_get_selectedColumns(This,selectedColumns,nColumns)	\
    ( (This)->lpVtbl -> get_selectedColumns(This,selectedColumns,nColumns) ) 

#define IAccessibleTable2_get_selectedRows(This,selectedRows,nRows)	\
    ( (This)->lpVtbl -> get_selectedRows(This,selectedRows,nRows) ) 

#define IAccessibleTable2_get_summary(This,accessible)	\
    ( (This)->lpVtbl -> get_summary(This,accessible) ) 

#define IAccessibleTable2_get_isColumnSelected(This,column,isSelected)	\
    ( (This)->lpVtbl -> get_isColumnSelected(This,column,isSelected) ) 

#define IAccessibleTable2_get_isRowSelected(This,row,isSelected)	\
    ( (This)->lpVtbl -> get_isRowSelected(This,row,isSelected) ) 

#define IAccessibleTable2_selectRow(This,row)	\
    ( (This)->lpVtbl -> selectRow(This,row) ) 

#define IAccessibleTable2_selectColumn(This,column)	\
    ( (This)->lpVtbl -> selectColumn(This,column) ) 

#define IAccessibleTable2_unselectRow(This,row)	\
    ( (This)->lpVtbl -> unselectRow(This,row) ) 

#define IAccessibleTable2_unselectColumn(This,column)	\
    ( (This)->lpVtbl -> unselectColumn(This,column) ) 

#define IAccessibleTable2_get_modelChange(This,modelChange)	\
    ( (This)->lpVtbl -> get_modelChange(This,modelChange) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IAccessibleTable2_INTERFACE_DEFINED__ */


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


