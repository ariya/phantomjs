

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Thu Mar 08 14:53:53 2012
 */
/* Compiler settings for AccessibleImage.idl:
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

#ifndef __AccessibleImage_h__
#define __AccessibleImage_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAccessibleImage_FWD_DEFINED__
#define __IAccessibleImage_FWD_DEFINED__
typedef interface IAccessibleImage IAccessibleImage;
#endif 	/* __IAccessibleImage_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oaidl.h"
#include "oleacc.h"
#include "IA2CommonTypes.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IAccessibleImage_INTERFACE_DEFINED__
#define __IAccessibleImage_INTERFACE_DEFINED__

/* interface IAccessibleImage */
/* [uuid][object] */ 


EXTERN_C const IID IID_IAccessibleImage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FE5ABB3D-615E-4f7b-909F-5F0EDA9E8DDE")
    IAccessibleImage : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_description( 
            /* [retval][out] */ BSTR *description) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_imagePosition( 
            /* [in] */ enum IA2CoordinateType coordinateType,
            /* [out] */ long *x,
            /* [retval][out] */ long *y) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_imageSize( 
            /* [out] */ long *height,
            /* [retval][out] */ long *width) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAccessibleImageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAccessibleImage * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAccessibleImage * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAccessibleImage * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_description )( 
            IAccessibleImage * This,
            /* [retval][out] */ BSTR *description);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_imagePosition )( 
            IAccessibleImage * This,
            /* [in] */ enum IA2CoordinateType coordinateType,
            /* [out] */ long *x,
            /* [retval][out] */ long *y);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_imageSize )( 
            IAccessibleImage * This,
            /* [out] */ long *height,
            /* [retval][out] */ long *width);
        
        END_INTERFACE
    } IAccessibleImageVtbl;

    interface IAccessibleImage
    {
        CONST_VTBL struct IAccessibleImageVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAccessibleImage_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IAccessibleImage_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IAccessibleImage_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IAccessibleImage_get_description(This,description)	\
    ( (This)->lpVtbl -> get_description(This,description) ) 

#define IAccessibleImage_get_imagePosition(This,coordinateType,x,y)	\
    ( (This)->lpVtbl -> get_imagePosition(This,coordinateType,x,y) ) 

#define IAccessibleImage_get_imageSize(This,height,width)	\
    ( (This)->lpVtbl -> get_imageSize(This,height,width) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IAccessibleImage_INTERFACE_DEFINED__ */


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


