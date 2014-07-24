

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Thu Mar 08 14:54:00 2012
 */
/* Compiler settings for AccessibleTableCell.idl:
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

#ifndef __AccessibleTableCell_h__
#define __AccessibleTableCell_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAccessibleTableCell_FWD_DEFINED__
#define __IAccessibleTableCell_FWD_DEFINED__
typedef interface IAccessibleTableCell IAccessibleTableCell;
#endif 	/* __IAccessibleTableCell_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oaidl.h"
#include "oleacc.h"
#include "Accessible2.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IAccessibleTableCell_INTERFACE_DEFINED__
#define __IAccessibleTableCell_INTERFACE_DEFINED__

/* interface IAccessibleTableCell */
/* [uuid][object] */ 


EXTERN_C const IID IID_IAccessibleTableCell;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("594116B1-C99F-4847-AD06-0A7A86ECE645")
    IAccessibleTableCell : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_columnExtent( 
            /* [retval][out] */ long *nColumnsSpanned) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_columnHeaderCells( 
            /* [size_is][size_is][size_is][out] */ IUnknown ***cellAccessibles,
            /* [retval][out] */ long *nColumnHeaderCells) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_columnIndex( 
            /* [retval][out] */ long *columnIndex) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_rowExtent( 
            /* [retval][out] */ long *nRowsSpanned) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_rowHeaderCells( 
            /* [size_is][size_is][size_is][out] */ IUnknown ***cellAccessibles,
            /* [retval][out] */ long *nRowHeaderCells) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_rowIndex( 
            /* [retval][out] */ long *rowIndex) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_isSelected( 
            /* [retval][out] */ boolean *isSelected) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_rowColumnExtents( 
            /* [out] */ long *row,
            /* [out] */ long *column,
            /* [out] */ long *rowExtents,
            /* [out] */ long *columnExtents,
            /* [retval][out] */ boolean *isSelected) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_table( 
            /* [retval][out] */ IUnknown **table) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAccessibleTableCellVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAccessibleTableCell * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAccessibleTableCell * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAccessibleTableCell * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_columnExtent )( 
            IAccessibleTableCell * This,
            /* [retval][out] */ long *nColumnsSpanned);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_columnHeaderCells )( 
            IAccessibleTableCell * This,
            /* [size_is][size_is][size_is][out] */ IUnknown ***cellAccessibles,
            /* [retval][out] */ long *nColumnHeaderCells);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_columnIndex )( 
            IAccessibleTableCell * This,
            /* [retval][out] */ long *columnIndex);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_rowExtent )( 
            IAccessibleTableCell * This,
            /* [retval][out] */ long *nRowsSpanned);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_rowHeaderCells )( 
            IAccessibleTableCell * This,
            /* [size_is][size_is][size_is][out] */ IUnknown ***cellAccessibles,
            /* [retval][out] */ long *nRowHeaderCells);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_rowIndex )( 
            IAccessibleTableCell * This,
            /* [retval][out] */ long *rowIndex);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_isSelected )( 
            IAccessibleTableCell * This,
            /* [retval][out] */ boolean *isSelected);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_rowColumnExtents )( 
            IAccessibleTableCell * This,
            /* [out] */ long *row,
            /* [out] */ long *column,
            /* [out] */ long *rowExtents,
            /* [out] */ long *columnExtents,
            /* [retval][out] */ boolean *isSelected);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_table )( 
            IAccessibleTableCell * This,
            /* [retval][out] */ IUnknown **table);
        
        END_INTERFACE
    } IAccessibleTableCellVtbl;

    interface IAccessibleTableCell
    {
        CONST_VTBL struct IAccessibleTableCellVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAccessibleTableCell_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IAccessibleTableCell_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IAccessibleTableCell_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IAccessibleTableCell_get_columnExtent(This,nColumnsSpanned)	\
    ( (This)->lpVtbl -> get_columnExtent(This,nColumnsSpanned) ) 

#define IAccessibleTableCell_get_columnHeaderCells(This,cellAccessibles,nColumnHeaderCells)	\
    ( (This)->lpVtbl -> get_columnHeaderCells(This,cellAccessibles,nColumnHeaderCells) ) 

#define IAccessibleTableCell_get_columnIndex(This,columnIndex)	\
    ( (This)->lpVtbl -> get_columnIndex(This,columnIndex) ) 

#define IAccessibleTableCell_get_rowExtent(This,nRowsSpanned)	\
    ( (This)->lpVtbl -> get_rowExtent(This,nRowsSpanned) ) 

#define IAccessibleTableCell_get_rowHeaderCells(This,cellAccessibles,nRowHeaderCells)	\
    ( (This)->lpVtbl -> get_rowHeaderCells(This,cellAccessibles,nRowHeaderCells) ) 

#define IAccessibleTableCell_get_rowIndex(This,rowIndex)	\
    ( (This)->lpVtbl -> get_rowIndex(This,rowIndex) ) 

#define IAccessibleTableCell_get_isSelected(This,isSelected)	\
    ( (This)->lpVtbl -> get_isSelected(This,isSelected) ) 

#define IAccessibleTableCell_get_rowColumnExtents(This,row,column,rowExtents,columnExtents,isSelected)	\
    ( (This)->lpVtbl -> get_rowColumnExtents(This,row,column,rowExtents,columnExtents,isSelected) ) 

#define IAccessibleTableCell_get_table(This,table)	\
    ( (This)->lpVtbl -> get_table(This,table) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IAccessibleTableCell_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


