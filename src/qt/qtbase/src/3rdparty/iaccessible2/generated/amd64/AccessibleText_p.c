

/* this ALWAYS GENERATED file contains the proxy stub code */


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

#if defined(_M_AMD64)


#pragma warning( disable: 4049 )  /* more than 64k source lines */
#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning( disable: 4211 )  /* redefine extern to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/
#pragma warning( disable: 4024 )  /* array to pointer mapping*/
#pragma warning( disable: 4152 )  /* function/data pointer conversion in expression */

#define USE_STUBLESS_PROXY


/* verify that the <rpcproxy.h> version is high enough to compile this file*/
#ifndef __REDQ_RPCPROXY_H_VERSION__
#define __REQUIRED_RPCPROXY_H_VERSION__ 475
#endif


#include "rpcproxy.h"
#ifndef __RPCPROXY_H_VERSION__
#error this stub requires an updated version of <rpcproxy.h>
#endif /* __RPCPROXY_H_VERSION__ */


#include "AccessibleText.h"

#define TYPE_FORMAT_STRING_SIZE   67                                
#define PROC_FORMAT_STRING_SIZE   939                               
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   1            

typedef struct _AccessibleText_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } AccessibleText_MIDL_TYPE_FORMAT_STRING;

typedef struct _AccessibleText_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } AccessibleText_MIDL_PROC_FORMAT_STRING;

typedef struct _AccessibleText_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } AccessibleText_MIDL_EXPR_FORMAT_STRING;


static const RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};


extern const AccessibleText_MIDL_TYPE_FORMAT_STRING AccessibleText__MIDL_TypeFormatString;
extern const AccessibleText_MIDL_PROC_FORMAT_STRING AccessibleText__MIDL_ProcFormatString;
extern const AccessibleText_MIDL_EXPR_FORMAT_STRING AccessibleText__MIDL_ExprFormatString;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IAccessibleText_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IAccessibleText_ProxyInfo;


extern const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ];

#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const AccessibleText_MIDL_PROC_FORMAT_STRING AccessibleText__MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure addSelection */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 10 */	NdrFcShort( 0x10 ),	/* 16 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x3,		/* 3 */
/* 16 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 18 */	NdrFcShort( 0x0 ),	/* 0 */
/* 20 */	NdrFcShort( 0x0 ),	/* 0 */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter startOffset */

/* 26 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 28 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 30 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter endOffset */

/* 32 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 34 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 36 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 38 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 40 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 42 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_attributes */

/* 44 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 46 */	NdrFcLong( 0x0 ),	/* 0 */
/* 50 */	NdrFcShort( 0x4 ),	/* 4 */
/* 52 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 54 */	NdrFcShort( 0x8 ),	/* 8 */
/* 56 */	NdrFcShort( 0x40 ),	/* 64 */
/* 58 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x5,		/* 5 */
/* 60 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 62 */	NdrFcShort( 0x1 ),	/* 1 */
/* 64 */	NdrFcShort( 0x0 ),	/* 0 */
/* 66 */	NdrFcShort( 0x0 ),	/* 0 */
/* 68 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter offset */

/* 70 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 72 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 74 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter startOffset */

/* 76 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 78 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 80 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter endOffset */

/* 82 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 84 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 86 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter textAttributes */

/* 88 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 90 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 92 */	NdrFcShort( 0x24 ),	/* Type Offset=36 */

	/* Return value */

/* 94 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 96 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 98 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_caretOffset */

/* 100 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 102 */	NdrFcLong( 0x0 ),	/* 0 */
/* 106 */	NdrFcShort( 0x5 ),	/* 5 */
/* 108 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 110 */	NdrFcShort( 0x0 ),	/* 0 */
/* 112 */	NdrFcShort( 0x24 ),	/* 36 */
/* 114 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 116 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 118 */	NdrFcShort( 0x0 ),	/* 0 */
/* 120 */	NdrFcShort( 0x0 ),	/* 0 */
/* 122 */	NdrFcShort( 0x0 ),	/* 0 */
/* 124 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter offset */

/* 126 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 128 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 130 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 132 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 134 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 136 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_characterExtents */

/* 138 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 140 */	NdrFcLong( 0x0 ),	/* 0 */
/* 144 */	NdrFcShort( 0x6 ),	/* 6 */
/* 146 */	NdrFcShort( 0x40 ),	/* X64 Stack size/offset = 64 */
/* 148 */	NdrFcShort( 0xe ),	/* 14 */
/* 150 */	NdrFcShort( 0x78 ),	/* 120 */
/* 152 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x7,		/* 7 */
/* 154 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 156 */	NdrFcShort( 0x0 ),	/* 0 */
/* 158 */	NdrFcShort( 0x0 ),	/* 0 */
/* 160 */	NdrFcShort( 0x0 ),	/* 0 */
/* 162 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter offset */

/* 164 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 166 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 168 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter coordType */

/* 170 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 172 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 174 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Parameter x */

/* 176 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 178 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 180 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter y */

/* 182 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 184 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 186 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter width */

/* 188 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 190 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 192 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter height */

/* 194 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 196 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 198 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 200 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 202 */	NdrFcShort( 0x38 ),	/* X64 Stack size/offset = 56 */
/* 204 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_nSelections */

/* 206 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 208 */	NdrFcLong( 0x0 ),	/* 0 */
/* 212 */	NdrFcShort( 0x7 ),	/* 7 */
/* 214 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 216 */	NdrFcShort( 0x0 ),	/* 0 */
/* 218 */	NdrFcShort( 0x24 ),	/* 36 */
/* 220 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 222 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 224 */	NdrFcShort( 0x0 ),	/* 0 */
/* 226 */	NdrFcShort( 0x0 ),	/* 0 */
/* 228 */	NdrFcShort( 0x0 ),	/* 0 */
/* 230 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter nSelections */

/* 232 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 234 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 236 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 238 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 240 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 242 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_offsetAtPoint */

/* 244 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 246 */	NdrFcLong( 0x0 ),	/* 0 */
/* 250 */	NdrFcShort( 0x8 ),	/* 8 */
/* 252 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 254 */	NdrFcShort( 0x16 ),	/* 22 */
/* 256 */	NdrFcShort( 0x24 ),	/* 36 */
/* 258 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x5,		/* 5 */
/* 260 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 262 */	NdrFcShort( 0x0 ),	/* 0 */
/* 264 */	NdrFcShort( 0x0 ),	/* 0 */
/* 266 */	NdrFcShort( 0x0 ),	/* 0 */
/* 268 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter x */

/* 270 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 272 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 274 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter y */

/* 276 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 278 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 280 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter coordType */

/* 282 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 284 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 286 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Parameter offset */

/* 288 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 290 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 292 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 294 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 296 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 298 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_selection */

/* 300 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 302 */	NdrFcLong( 0x0 ),	/* 0 */
/* 306 */	NdrFcShort( 0x9 ),	/* 9 */
/* 308 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 310 */	NdrFcShort( 0x8 ),	/* 8 */
/* 312 */	NdrFcShort( 0x40 ),	/* 64 */
/* 314 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x4,		/* 4 */
/* 316 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 318 */	NdrFcShort( 0x0 ),	/* 0 */
/* 320 */	NdrFcShort( 0x0 ),	/* 0 */
/* 322 */	NdrFcShort( 0x0 ),	/* 0 */
/* 324 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter selectionIndex */

/* 326 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 328 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 330 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter startOffset */

/* 332 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 334 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 336 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter endOffset */

/* 338 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 340 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 342 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 344 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 346 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 348 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_text */

/* 350 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 352 */	NdrFcLong( 0x0 ),	/* 0 */
/* 356 */	NdrFcShort( 0xa ),	/* 10 */
/* 358 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 360 */	NdrFcShort( 0x10 ),	/* 16 */
/* 362 */	NdrFcShort( 0x8 ),	/* 8 */
/* 364 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x4,		/* 4 */
/* 366 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 368 */	NdrFcShort( 0x1 ),	/* 1 */
/* 370 */	NdrFcShort( 0x0 ),	/* 0 */
/* 372 */	NdrFcShort( 0x0 ),	/* 0 */
/* 374 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter startOffset */

/* 376 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 378 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 380 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter endOffset */

/* 382 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 384 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 386 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter text */

/* 388 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 390 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 392 */	NdrFcShort( 0x24 ),	/* Type Offset=36 */

	/* Return value */

/* 394 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 396 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 398 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_textBeforeOffset */

/* 400 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 402 */	NdrFcLong( 0x0 ),	/* 0 */
/* 406 */	NdrFcShort( 0xb ),	/* 11 */
/* 408 */	NdrFcShort( 0x38 ),	/* X64 Stack size/offset = 56 */
/* 410 */	NdrFcShort( 0xe ),	/* 14 */
/* 412 */	NdrFcShort( 0x40 ),	/* 64 */
/* 414 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x6,		/* 6 */
/* 416 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 418 */	NdrFcShort( 0x1 ),	/* 1 */
/* 420 */	NdrFcShort( 0x0 ),	/* 0 */
/* 422 */	NdrFcShort( 0x0 ),	/* 0 */
/* 424 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter offset */

/* 426 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 428 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 430 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter boundaryType */

/* 432 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 434 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 436 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Parameter startOffset */

/* 438 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 440 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 442 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter endOffset */

/* 444 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 446 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 448 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter text */

/* 450 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 452 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 454 */	NdrFcShort( 0x24 ),	/* Type Offset=36 */

	/* Return value */

/* 456 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 458 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 460 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_textAfterOffset */

/* 462 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 464 */	NdrFcLong( 0x0 ),	/* 0 */
/* 468 */	NdrFcShort( 0xc ),	/* 12 */
/* 470 */	NdrFcShort( 0x38 ),	/* X64 Stack size/offset = 56 */
/* 472 */	NdrFcShort( 0xe ),	/* 14 */
/* 474 */	NdrFcShort( 0x40 ),	/* 64 */
/* 476 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x6,		/* 6 */
/* 478 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 480 */	NdrFcShort( 0x1 ),	/* 1 */
/* 482 */	NdrFcShort( 0x0 ),	/* 0 */
/* 484 */	NdrFcShort( 0x0 ),	/* 0 */
/* 486 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter offset */

/* 488 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 490 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 492 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter boundaryType */

/* 494 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 496 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 498 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Parameter startOffset */

/* 500 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 502 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 504 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter endOffset */

/* 506 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 508 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 510 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter text */

/* 512 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 514 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 516 */	NdrFcShort( 0x24 ),	/* Type Offset=36 */

	/* Return value */

/* 518 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 520 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 522 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_textAtOffset */

/* 524 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 526 */	NdrFcLong( 0x0 ),	/* 0 */
/* 530 */	NdrFcShort( 0xd ),	/* 13 */
/* 532 */	NdrFcShort( 0x38 ),	/* X64 Stack size/offset = 56 */
/* 534 */	NdrFcShort( 0xe ),	/* 14 */
/* 536 */	NdrFcShort( 0x40 ),	/* 64 */
/* 538 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x6,		/* 6 */
/* 540 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 542 */	NdrFcShort( 0x1 ),	/* 1 */
/* 544 */	NdrFcShort( 0x0 ),	/* 0 */
/* 546 */	NdrFcShort( 0x0 ),	/* 0 */
/* 548 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter offset */

/* 550 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 552 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 554 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter boundaryType */

/* 556 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 558 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 560 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Parameter startOffset */

/* 562 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 564 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 566 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter endOffset */

/* 568 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 570 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 572 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter text */

/* 574 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 576 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 578 */	NdrFcShort( 0x24 ),	/* Type Offset=36 */

	/* Return value */

/* 580 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 582 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 584 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure removeSelection */

/* 586 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 588 */	NdrFcLong( 0x0 ),	/* 0 */
/* 592 */	NdrFcShort( 0xe ),	/* 14 */
/* 594 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 596 */	NdrFcShort( 0x8 ),	/* 8 */
/* 598 */	NdrFcShort( 0x8 ),	/* 8 */
/* 600 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 602 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 604 */	NdrFcShort( 0x0 ),	/* 0 */
/* 606 */	NdrFcShort( 0x0 ),	/* 0 */
/* 608 */	NdrFcShort( 0x0 ),	/* 0 */
/* 610 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter selectionIndex */

/* 612 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 614 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 616 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 618 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 620 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 622 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure setCaretOffset */

/* 624 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 626 */	NdrFcLong( 0x0 ),	/* 0 */
/* 630 */	NdrFcShort( 0xf ),	/* 15 */
/* 632 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 634 */	NdrFcShort( 0x8 ),	/* 8 */
/* 636 */	NdrFcShort( 0x8 ),	/* 8 */
/* 638 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 640 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 642 */	NdrFcShort( 0x0 ),	/* 0 */
/* 644 */	NdrFcShort( 0x0 ),	/* 0 */
/* 646 */	NdrFcShort( 0x0 ),	/* 0 */
/* 648 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter offset */

/* 650 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 652 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 654 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 656 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 658 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 660 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure setSelection */

/* 662 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 664 */	NdrFcLong( 0x0 ),	/* 0 */
/* 668 */	NdrFcShort( 0x10 ),	/* 16 */
/* 670 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 672 */	NdrFcShort( 0x18 ),	/* 24 */
/* 674 */	NdrFcShort( 0x8 ),	/* 8 */
/* 676 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x4,		/* 4 */
/* 678 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 680 */	NdrFcShort( 0x0 ),	/* 0 */
/* 682 */	NdrFcShort( 0x0 ),	/* 0 */
/* 684 */	NdrFcShort( 0x0 ),	/* 0 */
/* 686 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter selectionIndex */

/* 688 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 690 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 692 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter startOffset */

/* 694 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 696 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 698 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter endOffset */

/* 700 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 702 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 704 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 706 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 708 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 710 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_nCharacters */

/* 712 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 714 */	NdrFcLong( 0x0 ),	/* 0 */
/* 718 */	NdrFcShort( 0x11 ),	/* 17 */
/* 720 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 722 */	NdrFcShort( 0x0 ),	/* 0 */
/* 724 */	NdrFcShort( 0x24 ),	/* 36 */
/* 726 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 728 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 730 */	NdrFcShort( 0x0 ),	/* 0 */
/* 732 */	NdrFcShort( 0x0 ),	/* 0 */
/* 734 */	NdrFcShort( 0x0 ),	/* 0 */
/* 736 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter nCharacters */

/* 738 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 740 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 742 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 744 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 746 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 748 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure scrollSubstringTo */

/* 750 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 752 */	NdrFcLong( 0x0 ),	/* 0 */
/* 756 */	NdrFcShort( 0x12 ),	/* 18 */
/* 758 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 760 */	NdrFcShort( 0x16 ),	/* 22 */
/* 762 */	NdrFcShort( 0x8 ),	/* 8 */
/* 764 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x4,		/* 4 */
/* 766 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 768 */	NdrFcShort( 0x0 ),	/* 0 */
/* 770 */	NdrFcShort( 0x0 ),	/* 0 */
/* 772 */	NdrFcShort( 0x0 ),	/* 0 */
/* 774 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter startIndex */

/* 776 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 778 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 780 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter endIndex */

/* 782 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 784 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 786 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter scrollType */

/* 788 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 790 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 792 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Return value */

/* 794 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 796 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 798 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure scrollSubstringToPoint */

/* 800 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 802 */	NdrFcLong( 0x0 ),	/* 0 */
/* 806 */	NdrFcShort( 0x13 ),	/* 19 */
/* 808 */	NdrFcShort( 0x38 ),	/* X64 Stack size/offset = 56 */
/* 810 */	NdrFcShort( 0x26 ),	/* 38 */
/* 812 */	NdrFcShort( 0x8 ),	/* 8 */
/* 814 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x6,		/* 6 */
/* 816 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 818 */	NdrFcShort( 0x0 ),	/* 0 */
/* 820 */	NdrFcShort( 0x0 ),	/* 0 */
/* 822 */	NdrFcShort( 0x0 ),	/* 0 */
/* 824 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter startIndex */

/* 826 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 828 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 830 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter endIndex */

/* 832 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 834 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 836 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter coordinateType */

/* 838 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 840 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 842 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Parameter x */

/* 844 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 846 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 848 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter y */

/* 850 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 852 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 854 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 856 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 858 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 860 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_newText */

/* 862 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 864 */	NdrFcLong( 0x0 ),	/* 0 */
/* 868 */	NdrFcShort( 0x14 ),	/* 20 */
/* 870 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 872 */	NdrFcShort( 0x0 ),	/* 0 */
/* 874 */	NdrFcShort( 0x8 ),	/* 8 */
/* 876 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 878 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 880 */	NdrFcShort( 0x1 ),	/* 1 */
/* 882 */	NdrFcShort( 0x0 ),	/* 0 */
/* 884 */	NdrFcShort( 0x0 ),	/* 0 */
/* 886 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter newText */

/* 888 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
/* 890 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 892 */	NdrFcShort( 0x32 ),	/* Type Offset=50 */

	/* Return value */

/* 894 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 896 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 898 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_oldText */

/* 900 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 902 */	NdrFcLong( 0x0 ),	/* 0 */
/* 906 */	NdrFcShort( 0x15 ),	/* 21 */
/* 908 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 910 */	NdrFcShort( 0x0 ),	/* 0 */
/* 912 */	NdrFcShort( 0x8 ),	/* 8 */
/* 914 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 916 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 918 */	NdrFcShort( 0x1 ),	/* 1 */
/* 920 */	NdrFcShort( 0x0 ),	/* 0 */
/* 922 */	NdrFcShort( 0x0 ),	/* 0 */
/* 924 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter oldText */

/* 926 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
/* 928 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 930 */	NdrFcShort( 0x32 ),	/* Type Offset=50 */

	/* Return value */

/* 932 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 934 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 936 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

			0x0
        }
    };

static const AccessibleText_MIDL_TYPE_FORMAT_STRING AccessibleText__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/*  4 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/*  6 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/*  8 */	NdrFcShort( 0x1c ),	/* Offset= 28 (36) */
/* 10 */	
			0x13, 0x0,	/* FC_OP */
/* 12 */	NdrFcShort( 0xe ),	/* Offset= 14 (26) */
/* 14 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 16 */	NdrFcShort( 0x2 ),	/* 2 */
/* 18 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 20 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 22 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 24 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 26 */	
			0x17,		/* FC_CSTRUCT */
			0x3,		/* 3 */
/* 28 */	NdrFcShort( 0x8 ),	/* 8 */
/* 30 */	NdrFcShort( 0xfff0 ),	/* Offset= -16 (14) */
/* 32 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 34 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 36 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 38 */	NdrFcShort( 0x0 ),	/* 0 */
/* 40 */	NdrFcShort( 0x8 ),	/* 8 */
/* 42 */	NdrFcShort( 0x0 ),	/* 0 */
/* 44 */	NdrFcShort( 0xffde ),	/* Offset= -34 (10) */
/* 46 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 48 */	NdrFcShort( 0x2 ),	/* Offset= 2 (50) */
/* 50 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 52 */	NdrFcShort( 0x10 ),	/* 16 */
/* 54 */	NdrFcShort( 0x0 ),	/* 0 */
/* 56 */	NdrFcShort( 0x0 ),	/* Offset= 0 (56) */
/* 58 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 60 */	NdrFcShort( 0xffe8 ),	/* Offset= -24 (36) */
/* 62 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 64 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */

			0x0
        }
    };

static const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ] = 
        {
            
            {
            BSTR_UserSize
            ,BSTR_UserMarshal
            ,BSTR_UserUnmarshal
            ,BSTR_UserFree
            }

        };



/* Standard interface: __MIDL_itf_AccessibleText_0000_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IAccessibleText, ver. 0.0,
   GUID={0x24FD2FFB,0x3AAD,0x4a08,{0x83,0x35,0xA3,0xAD,0x89,0xC0,0xFB,0x4B}} */

#pragma code_seg(".orpc")
static const unsigned short IAccessibleText_FormatStringOffsetTable[] =
    {
    0,
    44,
    100,
    138,
    206,
    244,
    300,
    350,
    400,
    462,
    524,
    586,
    624,
    662,
    712,
    750,
    800,
    862,
    900
    };

static const MIDL_STUBLESS_PROXY_INFO IAccessibleText_ProxyInfo =
    {
    &Object_StubDesc,
    AccessibleText__MIDL_ProcFormatString.Format,
    &IAccessibleText_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IAccessibleText_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    AccessibleText__MIDL_ProcFormatString.Format,
    &IAccessibleText_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(22) _IAccessibleTextProxyVtbl = 
{
    &IAccessibleText_ProxyInfo,
    &IID_IAccessibleText,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IAccessibleText::addSelection */ ,
    (void *) (INT_PTR) -1 /* IAccessibleText::get_attributes */ ,
    (void *) (INT_PTR) -1 /* IAccessibleText::get_caretOffset */ ,
    (void *) (INT_PTR) -1 /* IAccessibleText::get_characterExtents */ ,
    (void *) (INT_PTR) -1 /* IAccessibleText::get_nSelections */ ,
    (void *) (INT_PTR) -1 /* IAccessibleText::get_offsetAtPoint */ ,
    (void *) (INT_PTR) -1 /* IAccessibleText::get_selection */ ,
    (void *) (INT_PTR) -1 /* IAccessibleText::get_text */ ,
    (void *) (INT_PTR) -1 /* IAccessibleText::get_textBeforeOffset */ ,
    (void *) (INT_PTR) -1 /* IAccessibleText::get_textAfterOffset */ ,
    (void *) (INT_PTR) -1 /* IAccessibleText::get_textAtOffset */ ,
    (void *) (INT_PTR) -1 /* IAccessibleText::removeSelection */ ,
    (void *) (INT_PTR) -1 /* IAccessibleText::setCaretOffset */ ,
    (void *) (INT_PTR) -1 /* IAccessibleText::setSelection */ ,
    (void *) (INT_PTR) -1 /* IAccessibleText::get_nCharacters */ ,
    (void *) (INT_PTR) -1 /* IAccessibleText::scrollSubstringTo */ ,
    (void *) (INT_PTR) -1 /* IAccessibleText::scrollSubstringToPoint */ ,
    (void *) (INT_PTR) -1 /* IAccessibleText::get_newText */ ,
    (void *) (INT_PTR) -1 /* IAccessibleText::get_oldText */
};

const CInterfaceStubVtbl _IAccessibleTextStubVtbl =
{
    &IID_IAccessibleText,
    &IAccessibleText_ServerInfo,
    22,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};

static const MIDL_STUB_DESC Object_StubDesc = 
    {
    0,
    NdrOleAllocate,
    NdrOleFree,
    0,
    0,
    0,
    0,
    0,
    AccessibleText__MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x700022b, /* MIDL Version 7.0.555 */
    0,
    UserMarshalRoutines,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0
    };

const CInterfaceProxyVtbl * const _AccessibleText_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_IAccessibleTextProxyVtbl,
    0
};

const CInterfaceStubVtbl * const _AccessibleText_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_IAccessibleTextStubVtbl,
    0
};

PCInterfaceName const _AccessibleText_InterfaceNamesList[] = 
{
    "IAccessibleText",
    0
};


#define _AccessibleText_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _AccessibleText, pIID, n)

int __stdcall _AccessibleText_IID_Lookup( const IID * pIID, int * pIndex )
{
    
    if(!_AccessibleText_CHECK_IID(0))
        {
        *pIndex = 0;
        return 1;
        }

    return 0;
}

const ExtendedProxyFileInfo AccessibleText_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _AccessibleText_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _AccessibleText_StubVtblList,
    (const PCInterfaceName * ) & _AccessibleText_InterfaceNamesList,
    0, /* no delegation */
    & _AccessibleText_IID_Lookup, 
    1,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* defined(_M_AMD64)*/

