

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Thu Mar 08 14:53:56 2012
 */
/* Compiler settings for IA2CommonTypes.idl:
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


#ifndef __IA2CommonTypes_h__
#define __IA2CommonTypes_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_IA2CommonTypes_0000_0000 */
/* [local] */ 


enum IA2ScrollType
    {	IA2_SCROLL_TYPE_TOP_LEFT	= 0,
	IA2_SCROLL_TYPE_BOTTOM_RIGHT	= ( IA2_SCROLL_TYPE_TOP_LEFT + 1 ) ,
	IA2_SCROLL_TYPE_TOP_EDGE	= ( IA2_SCROLL_TYPE_BOTTOM_RIGHT + 1 ) ,
	IA2_SCROLL_TYPE_BOTTOM_EDGE	= ( IA2_SCROLL_TYPE_TOP_EDGE + 1 ) ,
	IA2_SCROLL_TYPE_LEFT_EDGE	= ( IA2_SCROLL_TYPE_BOTTOM_EDGE + 1 ) ,
	IA2_SCROLL_TYPE_RIGHT_EDGE	= ( IA2_SCROLL_TYPE_LEFT_EDGE + 1 ) ,
	IA2_SCROLL_TYPE_ANYWHERE	= ( IA2_SCROLL_TYPE_RIGHT_EDGE + 1 ) 
    } ;

enum IA2CoordinateType
    {	IA2_COORDTYPE_SCREEN_RELATIVE	= 0,
	IA2_COORDTYPE_PARENT_RELATIVE	= ( IA2_COORDTYPE_SCREEN_RELATIVE + 1 ) 
    } ;

enum IA2TextSpecialOffsets
    {	IA2_TEXT_OFFSET_LENGTH	= -1,
	IA2_TEXT_OFFSET_CARET	= -2
    } ;

enum IA2TableModelChangeType
    {	IA2_TABLE_MODEL_CHANGE_INSERT	= 0,
	IA2_TABLE_MODEL_CHANGE_DELETE	= ( IA2_TABLE_MODEL_CHANGE_INSERT + 1 ) ,
	IA2_TABLE_MODEL_CHANGE_UPDATE	= ( IA2_TABLE_MODEL_CHANGE_DELETE + 1 ) 
    } ;
typedef struct IA2TableModelChange
    {
    enum IA2TableModelChangeType type;
    long firstRow;
    long lastRow;
    long firstColumn;
    long lastColumn;
    } 	IA2TableModelChange;



extern RPC_IF_HANDLE __MIDL_itf_IA2CommonTypes_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_IA2CommonTypes_0000_0000_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


