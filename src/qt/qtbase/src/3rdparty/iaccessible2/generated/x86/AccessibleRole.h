

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Thu Mar 08 14:53:54 2012
 */
/* Compiler settings for AccessibleRole.idl:
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


#ifndef __AccessibleRole_h__
#define __AccessibleRole_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

/* header files for imported files */
#include "objidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_AccessibleRole_0000_0000 */
/* [local] */ 


enum IA2Role
    {	IA2_ROLE_UNKNOWN	= 0,
	IA2_ROLE_CANVAS	= 0x401,
	IA2_ROLE_CAPTION	= ( IA2_ROLE_CANVAS + 1 ) ,
	IA2_ROLE_CHECK_MENU_ITEM	= ( IA2_ROLE_CAPTION + 1 ) ,
	IA2_ROLE_COLOR_CHOOSER	= ( IA2_ROLE_CHECK_MENU_ITEM + 1 ) ,
	IA2_ROLE_DATE_EDITOR	= ( IA2_ROLE_COLOR_CHOOSER + 1 ) ,
	IA2_ROLE_DESKTOP_ICON	= ( IA2_ROLE_DATE_EDITOR + 1 ) ,
	IA2_ROLE_DESKTOP_PANE	= ( IA2_ROLE_DESKTOP_ICON + 1 ) ,
	IA2_ROLE_DIRECTORY_PANE	= ( IA2_ROLE_DESKTOP_PANE + 1 ) ,
	IA2_ROLE_EDITBAR	= ( IA2_ROLE_DIRECTORY_PANE + 1 ) ,
	IA2_ROLE_EMBEDDED_OBJECT	= ( IA2_ROLE_EDITBAR + 1 ) ,
	IA2_ROLE_ENDNOTE	= ( IA2_ROLE_EMBEDDED_OBJECT + 1 ) ,
	IA2_ROLE_FILE_CHOOSER	= ( IA2_ROLE_ENDNOTE + 1 ) ,
	IA2_ROLE_FONT_CHOOSER	= ( IA2_ROLE_FILE_CHOOSER + 1 ) ,
	IA2_ROLE_FOOTER	= ( IA2_ROLE_FONT_CHOOSER + 1 ) ,
	IA2_ROLE_FOOTNOTE	= ( IA2_ROLE_FOOTER + 1 ) ,
	IA2_ROLE_FORM	= ( IA2_ROLE_FOOTNOTE + 1 ) ,
	IA2_ROLE_FRAME	= ( IA2_ROLE_FORM + 1 ) ,
	IA2_ROLE_GLASS_PANE	= ( IA2_ROLE_FRAME + 1 ) ,
	IA2_ROLE_HEADER	= ( IA2_ROLE_GLASS_PANE + 1 ) ,
	IA2_ROLE_HEADING	= ( IA2_ROLE_HEADER + 1 ) ,
	IA2_ROLE_ICON	= ( IA2_ROLE_HEADING + 1 ) ,
	IA2_ROLE_IMAGE_MAP	= ( IA2_ROLE_ICON + 1 ) ,
	IA2_ROLE_INPUT_METHOD_WINDOW	= ( IA2_ROLE_IMAGE_MAP + 1 ) ,
	IA2_ROLE_INTERNAL_FRAME	= ( IA2_ROLE_INPUT_METHOD_WINDOW + 1 ) ,
	IA2_ROLE_LABEL	= ( IA2_ROLE_INTERNAL_FRAME + 1 ) ,
	IA2_ROLE_LAYERED_PANE	= ( IA2_ROLE_LABEL + 1 ) ,
	IA2_ROLE_NOTE	= ( IA2_ROLE_LAYERED_PANE + 1 ) ,
	IA2_ROLE_OPTION_PANE	= ( IA2_ROLE_NOTE + 1 ) ,
	IA2_ROLE_PAGE	= ( IA2_ROLE_OPTION_PANE + 1 ) ,
	IA2_ROLE_PARAGRAPH	= ( IA2_ROLE_PAGE + 1 ) ,
	IA2_ROLE_RADIO_MENU_ITEM	= ( IA2_ROLE_PARAGRAPH + 1 ) ,
	IA2_ROLE_REDUNDANT_OBJECT	= ( IA2_ROLE_RADIO_MENU_ITEM + 1 ) ,
	IA2_ROLE_ROOT_PANE	= ( IA2_ROLE_REDUNDANT_OBJECT + 1 ) ,
	IA2_ROLE_RULER	= ( IA2_ROLE_ROOT_PANE + 1 ) ,
	IA2_ROLE_SCROLL_PANE	= ( IA2_ROLE_RULER + 1 ) ,
	IA2_ROLE_SECTION	= ( IA2_ROLE_SCROLL_PANE + 1 ) ,
	IA2_ROLE_SHAPE	= ( IA2_ROLE_SECTION + 1 ) ,
	IA2_ROLE_SPLIT_PANE	= ( IA2_ROLE_SHAPE + 1 ) ,
	IA2_ROLE_TEAR_OFF_MENU	= ( IA2_ROLE_SPLIT_PANE + 1 ) ,
	IA2_ROLE_TERMINAL	= ( IA2_ROLE_TEAR_OFF_MENU + 1 ) ,
	IA2_ROLE_TEXT_FRAME	= ( IA2_ROLE_TERMINAL + 1 ) ,
	IA2_ROLE_TOGGLE_BUTTON	= ( IA2_ROLE_TEXT_FRAME + 1 ) ,
	IA2_ROLE_VIEW_PORT	= ( IA2_ROLE_TOGGLE_BUTTON + 1 ) ,
	IA2_ROLE_COMPLEMENTARY_CONTENT	= ( IA2_ROLE_VIEW_PORT + 1 ) 
    } ;


extern RPC_IF_HANDLE __MIDL_itf_AccessibleRole_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_AccessibleRole_0000_0000_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


