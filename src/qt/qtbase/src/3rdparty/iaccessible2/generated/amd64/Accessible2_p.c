

/* this ALWAYS GENERATED file contains the proxy stub code */


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


#include "Accessible2.h"

#define TYPE_FORMAT_STRING_SIZE   189                               
#define PROC_FORMAT_STRING_SIZE   751                               
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   2            

typedef struct _Accessible2_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } Accessible2_MIDL_TYPE_FORMAT_STRING;

typedef struct _Accessible2_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } Accessible2_MIDL_PROC_FORMAT_STRING;

typedef struct _Accessible2_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } Accessible2_MIDL_EXPR_FORMAT_STRING;


static const RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};


extern const Accessible2_MIDL_TYPE_FORMAT_STRING Accessible2__MIDL_TypeFormatString;
extern const Accessible2_MIDL_PROC_FORMAT_STRING Accessible2__MIDL_ProcFormatString;
extern const Accessible2_MIDL_EXPR_FORMAT_STRING Accessible2__MIDL_ExprFormatString;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IAccessible2_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IAccessible2_ProxyInfo;


extern const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ];

#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const Accessible2_MIDL_PROC_FORMAT_STRING Accessible2__MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure get_nRelations */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x1c ),	/* 28 */
/*  8 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 10 */	NdrFcShort( 0x0 ),	/* 0 */
/* 12 */	NdrFcShort( 0x24 ),	/* 36 */
/* 14 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 16 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 18 */	NdrFcShort( 0x0 ),	/* 0 */
/* 20 */	NdrFcShort( 0x0 ),	/* 0 */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter nRelations */

/* 26 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 28 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 30 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 32 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 34 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 36 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_relation */

/* 38 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 40 */	NdrFcLong( 0x0 ),	/* 0 */
/* 44 */	NdrFcShort( 0x1d ),	/* 29 */
/* 46 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 48 */	NdrFcShort( 0x8 ),	/* 8 */
/* 50 */	NdrFcShort( 0x8 ),	/* 8 */
/* 52 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x3,		/* 3 */
/* 54 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 56 */	NdrFcShort( 0x0 ),	/* 0 */
/* 58 */	NdrFcShort( 0x0 ),	/* 0 */
/* 60 */	NdrFcShort( 0x0 ),	/* 0 */
/* 62 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter relationIndex */

/* 64 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 66 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 68 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter relation */

/* 70 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 72 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 74 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 76 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 78 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 80 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_relations */

/* 82 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 84 */	NdrFcLong( 0x0 ),	/* 0 */
/* 88 */	NdrFcShort( 0x1e ),	/* 30 */
/* 90 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 92 */	NdrFcShort( 0x8 ),	/* 8 */
/* 94 */	NdrFcShort( 0x24 ),	/* 36 */
/* 96 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x4,		/* 4 */
/* 98 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 100 */	NdrFcShort( 0x1 ),	/* 1 */
/* 102 */	NdrFcShort( 0x0 ),	/* 0 */
/* 104 */	NdrFcShort( 0x0 ),	/* 0 */
/* 106 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter maxRelations */

/* 108 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 110 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 112 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter relations */

/* 114 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 116 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 118 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Parameter nRelations */

/* 120 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 122 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 124 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 126 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 128 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 130 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure role */

/* 132 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 134 */	NdrFcLong( 0x0 ),	/* 0 */
/* 138 */	NdrFcShort( 0x1f ),	/* 31 */
/* 140 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 142 */	NdrFcShort( 0x0 ),	/* 0 */
/* 144 */	NdrFcShort( 0x24 ),	/* 36 */
/* 146 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 148 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 150 */	NdrFcShort( 0x0 ),	/* 0 */
/* 152 */	NdrFcShort( 0x0 ),	/* 0 */
/* 154 */	NdrFcShort( 0x0 ),	/* 0 */
/* 156 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter role */

/* 158 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 160 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 162 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 164 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 166 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 168 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure scrollTo */

/* 170 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 172 */	NdrFcLong( 0x0 ),	/* 0 */
/* 176 */	NdrFcShort( 0x20 ),	/* 32 */
/* 178 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 180 */	NdrFcShort( 0x6 ),	/* 6 */
/* 182 */	NdrFcShort( 0x8 ),	/* 8 */
/* 184 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 186 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 188 */	NdrFcShort( 0x0 ),	/* 0 */
/* 190 */	NdrFcShort( 0x0 ),	/* 0 */
/* 192 */	NdrFcShort( 0x0 ),	/* 0 */
/* 194 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter scrollType */

/* 196 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 198 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 200 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Return value */

/* 202 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 204 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 206 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure scrollToPoint */

/* 208 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 210 */	NdrFcLong( 0x0 ),	/* 0 */
/* 214 */	NdrFcShort( 0x21 ),	/* 33 */
/* 216 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 218 */	NdrFcShort( 0x16 ),	/* 22 */
/* 220 */	NdrFcShort( 0x8 ),	/* 8 */
/* 222 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x4,		/* 4 */
/* 224 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 226 */	NdrFcShort( 0x0 ),	/* 0 */
/* 228 */	NdrFcShort( 0x0 ),	/* 0 */
/* 230 */	NdrFcShort( 0x0 ),	/* 0 */
/* 232 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter coordinateType */

/* 234 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 236 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 238 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Parameter x */

/* 240 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 242 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 244 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter y */

/* 246 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 248 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 250 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 252 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 254 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 256 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_groupPosition */

/* 258 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 260 */	NdrFcLong( 0x0 ),	/* 0 */
/* 264 */	NdrFcShort( 0x22 ),	/* 34 */
/* 266 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 268 */	NdrFcShort( 0x0 ),	/* 0 */
/* 270 */	NdrFcShort( 0x5c ),	/* 92 */
/* 272 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x4,		/* 4 */
/* 274 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 276 */	NdrFcShort( 0x0 ),	/* 0 */
/* 278 */	NdrFcShort( 0x0 ),	/* 0 */
/* 280 */	NdrFcShort( 0x0 ),	/* 0 */
/* 282 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter groupLevel */

/* 284 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 286 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 288 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter similarItemsInGroup */

/* 290 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 292 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 294 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter positionInGroup */

/* 296 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 298 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 300 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 302 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 304 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 306 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_states */

/* 308 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 310 */	NdrFcLong( 0x0 ),	/* 0 */
/* 314 */	NdrFcShort( 0x23 ),	/* 35 */
/* 316 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 318 */	NdrFcShort( 0x0 ),	/* 0 */
/* 320 */	NdrFcShort( 0x24 ),	/* 36 */
/* 322 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 324 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 326 */	NdrFcShort( 0x0 ),	/* 0 */
/* 328 */	NdrFcShort( 0x0 ),	/* 0 */
/* 330 */	NdrFcShort( 0x0 ),	/* 0 */
/* 332 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter states */

/* 334 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 336 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 338 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 340 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 342 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 344 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_extendedRole */

/* 346 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 348 */	NdrFcLong( 0x0 ),	/* 0 */
/* 352 */	NdrFcShort( 0x24 ),	/* 36 */
/* 354 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 356 */	NdrFcShort( 0x0 ),	/* 0 */
/* 358 */	NdrFcShort( 0x8 ),	/* 8 */
/* 360 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 362 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 364 */	NdrFcShort( 0x1 ),	/* 1 */
/* 366 */	NdrFcShort( 0x0 ),	/* 0 */
/* 368 */	NdrFcShort( 0x0 ),	/* 0 */
/* 370 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter extendedRole */

/* 372 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 374 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 376 */	NdrFcShort( 0x54 ),	/* Type Offset=84 */

	/* Return value */

/* 378 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 380 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 382 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_localizedExtendedRole */

/* 384 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 386 */	NdrFcLong( 0x0 ),	/* 0 */
/* 390 */	NdrFcShort( 0x25 ),	/* 37 */
/* 392 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 394 */	NdrFcShort( 0x0 ),	/* 0 */
/* 396 */	NdrFcShort( 0x8 ),	/* 8 */
/* 398 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 400 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 402 */	NdrFcShort( 0x1 ),	/* 1 */
/* 404 */	NdrFcShort( 0x0 ),	/* 0 */
/* 406 */	NdrFcShort( 0x0 ),	/* 0 */
/* 408 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter localizedExtendedRole */

/* 410 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 412 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 414 */	NdrFcShort( 0x54 ),	/* Type Offset=84 */

	/* Return value */

/* 416 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 418 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 420 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_nExtendedStates */

/* 422 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 424 */	NdrFcLong( 0x0 ),	/* 0 */
/* 428 */	NdrFcShort( 0x26 ),	/* 38 */
/* 430 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 432 */	NdrFcShort( 0x0 ),	/* 0 */
/* 434 */	NdrFcShort( 0x24 ),	/* 36 */
/* 436 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 438 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 440 */	NdrFcShort( 0x0 ),	/* 0 */
/* 442 */	NdrFcShort( 0x0 ),	/* 0 */
/* 444 */	NdrFcShort( 0x0 ),	/* 0 */
/* 446 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter nExtendedStates */

/* 448 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 450 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 452 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 454 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 456 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 458 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_extendedStates */

/* 460 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 462 */	NdrFcLong( 0x0 ),	/* 0 */
/* 466 */	NdrFcShort( 0x27 ),	/* 39 */
/* 468 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 470 */	NdrFcShort( 0x8 ),	/* 8 */
/* 472 */	NdrFcShort( 0x24 ),	/* 36 */
/* 474 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x4,		/* 4 */
/* 476 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 478 */	NdrFcShort( 0x1 ),	/* 1 */
/* 480 */	NdrFcShort( 0x0 ),	/* 0 */
/* 482 */	NdrFcShort( 0x0 ),	/* 0 */
/* 484 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter maxExtendedStates */

/* 486 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 488 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 490 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter extendedStates */

/* 492 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 494 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 496 */	NdrFcShort( 0x5e ),	/* Type Offset=94 */

	/* Parameter nExtendedStates */

/* 498 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 500 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 502 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 504 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 506 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 508 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_localizedExtendedStates */

/* 510 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 512 */	NdrFcLong( 0x0 ),	/* 0 */
/* 516 */	NdrFcShort( 0x28 ),	/* 40 */
/* 518 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 520 */	NdrFcShort( 0x8 ),	/* 8 */
/* 522 */	NdrFcShort( 0x24 ),	/* 36 */
/* 524 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x4,		/* 4 */
/* 526 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 528 */	NdrFcShort( 0x1 ),	/* 1 */
/* 530 */	NdrFcShort( 0x0 ),	/* 0 */
/* 532 */	NdrFcShort( 0x0 ),	/* 0 */
/* 534 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter maxLocalizedExtendedStates */

/* 536 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 538 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 540 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter localizedExtendedStates */

/* 542 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 544 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 546 */	NdrFcShort( 0x5e ),	/* Type Offset=94 */

	/* Parameter nLocalizedExtendedStates */

/* 548 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 550 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 552 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 554 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 556 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 558 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_uniqueID */

/* 560 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 562 */	NdrFcLong( 0x0 ),	/* 0 */
/* 566 */	NdrFcShort( 0x29 ),	/* 41 */
/* 568 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 570 */	NdrFcShort( 0x0 ),	/* 0 */
/* 572 */	NdrFcShort( 0x24 ),	/* 36 */
/* 574 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 576 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 578 */	NdrFcShort( 0x0 ),	/* 0 */
/* 580 */	NdrFcShort( 0x0 ),	/* 0 */
/* 582 */	NdrFcShort( 0x0 ),	/* 0 */
/* 584 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter uniqueID */

/* 586 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 588 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 590 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 592 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 594 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 596 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_windowHandle */

/* 598 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 600 */	NdrFcLong( 0x0 ),	/* 0 */
/* 604 */	NdrFcShort( 0x2a ),	/* 42 */
/* 606 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 608 */	NdrFcShort( 0x0 ),	/* 0 */
/* 610 */	NdrFcShort( 0x8 ),	/* 8 */
/* 612 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 614 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 616 */	NdrFcShort( 0x1 ),	/* 1 */
/* 618 */	NdrFcShort( 0x0 ),	/* 0 */
/* 620 */	NdrFcShort( 0x0 ),	/* 0 */
/* 622 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter windowHandle */

/* 624 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 626 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 628 */	NdrFcShort( 0x98 ),	/* Type Offset=152 */

	/* Return value */

/* 630 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 632 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 634 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_indexInParent */

/* 636 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 638 */	NdrFcLong( 0x0 ),	/* 0 */
/* 642 */	NdrFcShort( 0x2b ),	/* 43 */
/* 644 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 646 */	NdrFcShort( 0x0 ),	/* 0 */
/* 648 */	NdrFcShort( 0x24 ),	/* 36 */
/* 650 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 652 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 654 */	NdrFcShort( 0x0 ),	/* 0 */
/* 656 */	NdrFcShort( 0x0 ),	/* 0 */
/* 658 */	NdrFcShort( 0x0 ),	/* 0 */
/* 660 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter indexInParent */

/* 662 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 664 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 666 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 668 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 670 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 672 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_locale */

/* 674 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 676 */	NdrFcLong( 0x0 ),	/* 0 */
/* 680 */	NdrFcShort( 0x2c ),	/* 44 */
/* 682 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 684 */	NdrFcShort( 0x0 ),	/* 0 */
/* 686 */	NdrFcShort( 0x8 ),	/* 8 */
/* 688 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 690 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 692 */	NdrFcShort( 0x1 ),	/* 1 */
/* 694 */	NdrFcShort( 0x0 ),	/* 0 */
/* 696 */	NdrFcShort( 0x0 ),	/* 0 */
/* 698 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter locale */

/* 700 */	NdrFcShort( 0x6113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=24 */
/* 702 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 704 */	NdrFcShort( 0xa6 ),	/* Type Offset=166 */

	/* Return value */

/* 706 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 708 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 710 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_attributes */

/* 712 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 714 */	NdrFcLong( 0x0 ),	/* 0 */
/* 718 */	NdrFcShort( 0x2d ),	/* 45 */
/* 720 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 722 */	NdrFcShort( 0x0 ),	/* 0 */
/* 724 */	NdrFcShort( 0x8 ),	/* 8 */
/* 726 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 728 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 730 */	NdrFcShort( 0x1 ),	/* 1 */
/* 732 */	NdrFcShort( 0x0 ),	/* 0 */
/* 734 */	NdrFcShort( 0x0 ),	/* 0 */
/* 736 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter attributes */

/* 738 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 740 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 742 */	NdrFcShort( 0x54 ),	/* Type Offset=84 */

	/* Return value */

/* 744 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 746 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 748 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

			0x0
        }
    };

static const Accessible2_MIDL_TYPE_FORMAT_STRING Accessible2__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/*  4 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/*  6 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/*  8 */	NdrFcShort( 0x2 ),	/* Offset= 2 (10) */
/* 10 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 12 */	NdrFcLong( 0x7cdf86ee ),	/* 2095023854 */
/* 16 */	NdrFcShort( 0xc3da ),	/* -15398 */
/* 18 */	NdrFcShort( 0x496a ),	/* 18794 */
/* 20 */	0xbd,		/* 189 */
			0xa4,		/* 164 */
/* 22 */	0x28,		/* 40 */
			0x1b,		/* 27 */
/* 24 */	0x33,		/* 51 */
			0x6e,		/* 110 */
/* 26 */	0x1f,		/* 31 */
			0xdc,		/* 220 */
/* 28 */	
			0x11, 0x0,	/* FC_RP */
/* 30 */	NdrFcShort( 0x2 ),	/* Offset= 2 (32) */
/* 32 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 34 */	NdrFcShort( 0x0 ),	/* 0 */
/* 36 */	0x28,		/* Corr desc:  parameter, FC_LONG */
			0x0,		/*  */
/* 38 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 40 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 42 */	0x28,		/* Corr desc:  parameter, FC_LONG */
			0x54,		/* FC_DEREFERENCE */
/* 44 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 46 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 48 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 50 */	NdrFcShort( 0xffd8 ),	/* Offset= -40 (10) */
/* 52 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 54 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 56 */	NdrFcShort( 0x1c ),	/* Offset= 28 (84) */
/* 58 */	
			0x13, 0x0,	/* FC_OP */
/* 60 */	NdrFcShort( 0xe ),	/* Offset= 14 (74) */
/* 62 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 64 */	NdrFcShort( 0x2 ),	/* 2 */
/* 66 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 68 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 70 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 72 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 74 */	
			0x17,		/* FC_CSTRUCT */
			0x3,		/* 3 */
/* 76 */	NdrFcShort( 0x8 ),	/* 8 */
/* 78 */	NdrFcShort( 0xfff0 ),	/* Offset= -16 (62) */
/* 80 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 82 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 84 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 86 */	NdrFcShort( 0x0 ),	/* 0 */
/* 88 */	NdrFcShort( 0x8 ),	/* 8 */
/* 90 */	NdrFcShort( 0x0 ),	/* 0 */
/* 92 */	NdrFcShort( 0xffde ),	/* Offset= -34 (58) */
/* 94 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 96 */	NdrFcShort( 0x2 ),	/* Offset= 2 (98) */
/* 98 */	
			0x13, 0x0,	/* FC_OP */
/* 100 */	NdrFcShort( 0x2 ),	/* Offset= 2 (102) */
/* 102 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 104 */	NdrFcShort( 0x0 ),	/* 0 */
/* 106 */	0x28,		/* Corr desc:  parameter, FC_LONG */
			0x0,		/*  */
/* 108 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 110 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 112 */	0x28,		/* Corr desc:  parameter, FC_LONG */
			0x54,		/* FC_DEREFERENCE */
/* 114 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 116 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 118 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 120 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (84) */
/* 122 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 124 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 126 */	NdrFcShort( 0x1a ),	/* Offset= 26 (152) */
/* 128 */	
			0x13, 0x0,	/* FC_OP */
/* 130 */	NdrFcShort( 0x2 ),	/* Offset= 2 (132) */
/* 132 */	
			0x2a,		/* FC_ENCAPSULATED_UNION */
			0x48,		/* 72 */
/* 134 */	NdrFcShort( 0x4 ),	/* 4 */
/* 136 */	NdrFcShort( 0x2 ),	/* 2 */
/* 138 */	NdrFcLong( 0x48746457 ),	/* 1215587415 */
/* 142 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 144 */	NdrFcLong( 0x52746457 ),	/* 1383359575 */
/* 148 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 150 */	NdrFcShort( 0xffff ),	/* Offset= -1 (149) */
/* 152 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 154 */	NdrFcShort( 0x1 ),	/* 1 */
/* 156 */	NdrFcShort( 0x8 ),	/* 8 */
/* 158 */	NdrFcShort( 0x0 ),	/* 0 */
/* 160 */	NdrFcShort( 0xffe0 ),	/* Offset= -32 (128) */
/* 162 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 164 */	NdrFcShort( 0x2 ),	/* Offset= 2 (166) */
/* 166 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 168 */	NdrFcShort( 0x18 ),	/* 24 */
/* 170 */	NdrFcShort( 0x0 ),	/* 0 */
/* 172 */	NdrFcShort( 0x0 ),	/* Offset= 0 (172) */
/* 174 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 176 */	NdrFcShort( 0xffa4 ),	/* Offset= -92 (84) */
/* 178 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 180 */	NdrFcShort( 0xffa0 ),	/* Offset= -96 (84) */
/* 182 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 184 */	NdrFcShort( 0xff9c ),	/* Offset= -100 (84) */
/* 186 */	0x5c,		/* FC_PAD */
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
            },
            {
            HWND_UserSize
            ,HWND_UserMarshal
            ,HWND_UserUnmarshal
            ,HWND_UserFree
            }

        };



/* Standard interface: __MIDL_itf_Accessible2_0000_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IDispatch, ver. 0.0,
   GUID={0x00020400,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IAccessible, ver. 0.0,
   GUID={0x618736e0,0x3c3d,0x11cf,{0x81,0x0c,0x00,0xaa,0x00,0x38,0x9b,0x71}} */


/* Object interface: IAccessible2, ver. 0.0,
   GUID={0xE89F726E,0xC4F4,0x4c19,{0xBB,0x19,0xB6,0x47,0xD7,0xFA,0x84,0x78}} */

#pragma code_seg(".orpc")
static const unsigned short IAccessible2_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    0,
    38,
    82,
    132,
    170,
    208,
    258,
    308,
    346,
    384,
    422,
    460,
    510,
    560,
    598,
    636,
    674,
    712
    };

static const MIDL_STUBLESS_PROXY_INFO IAccessible2_ProxyInfo =
    {
    &Object_StubDesc,
    Accessible2__MIDL_ProcFormatString.Format,
    &IAccessible2_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IAccessible2_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    Accessible2__MIDL_ProcFormatString.Format,
    &IAccessible2_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(46) _IAccessible2ProxyVtbl = 
{
    &IAccessible2_ProxyInfo,
    &IID_IAccessible2,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    0 /* IAccessible::get_accParent */ ,
    0 /* IAccessible::get_accChildCount */ ,
    0 /* IAccessible::get_accChild */ ,
    0 /* IAccessible::get_accName */ ,
    0 /* IAccessible::get_accValue */ ,
    0 /* IAccessible::get_accDescription */ ,
    0 /* IAccessible::get_accRole */ ,
    0 /* IAccessible::get_accState */ ,
    0 /* IAccessible::get_accHelp */ ,
    0 /* IAccessible::get_accHelpTopic */ ,
    0 /* IAccessible::get_accKeyboardShortcut */ ,
    0 /* IAccessible::get_accFocus */ ,
    0 /* IAccessible::get_accSelection */ ,
    0 /* IAccessible::get_accDefaultAction */ ,
    0 /* IAccessible::accSelect */ ,
    0 /* IAccessible::accLocation */ ,
    0 /* IAccessible::accNavigate */ ,
    0 /* IAccessible::accHitTest */ ,
    0 /* IAccessible::accDoDefaultAction */ ,
    0 /* IAccessible::put_accName */ ,
    0 /* IAccessible::put_accValue */ ,
    (void *) (INT_PTR) -1 /* IAccessible2::get_nRelations */ ,
    (void *) (INT_PTR) -1 /* IAccessible2::get_relation */ ,
    (void *) (INT_PTR) -1 /* IAccessible2::get_relations */ ,
    (void *) (INT_PTR) -1 /* IAccessible2::role */ ,
    (void *) (INT_PTR) -1 /* IAccessible2::scrollTo */ ,
    (void *) (INT_PTR) -1 /* IAccessible2::scrollToPoint */ ,
    (void *) (INT_PTR) -1 /* IAccessible2::get_groupPosition */ ,
    (void *) (INT_PTR) -1 /* IAccessible2::get_states */ ,
    (void *) (INT_PTR) -1 /* IAccessible2::get_extendedRole */ ,
    (void *) (INT_PTR) -1 /* IAccessible2::get_localizedExtendedRole */ ,
    (void *) (INT_PTR) -1 /* IAccessible2::get_nExtendedStates */ ,
    (void *) (INT_PTR) -1 /* IAccessible2::get_extendedStates */ ,
    (void *) (INT_PTR) -1 /* IAccessible2::get_localizedExtendedStates */ ,
    (void *) (INT_PTR) -1 /* IAccessible2::get_uniqueID */ ,
    (void *) (INT_PTR) -1 /* IAccessible2::get_windowHandle */ ,
    (void *) (INT_PTR) -1 /* IAccessible2::get_indexInParent */ ,
    (void *) (INT_PTR) -1 /* IAccessible2::get_locale */ ,
    (void *) (INT_PTR) -1 /* IAccessible2::get_attributes */
};


static const PRPC_STUB_FUNCTION IAccessible2_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IAccessible2StubVtbl =
{
    &IID_IAccessible2,
    &IAccessible2_ServerInfo,
    46,
    &IAccessible2_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
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
    Accessible2__MIDL_TypeFormatString.Format,
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

const CInterfaceProxyVtbl * const _Accessible2_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_IAccessible2ProxyVtbl,
    0
};

const CInterfaceStubVtbl * const _Accessible2_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_IAccessible2StubVtbl,
    0
};

PCInterfaceName const _Accessible2_InterfaceNamesList[] = 
{
    "IAccessible2",
    0
};

const IID *  const _Accessible2_BaseIIDList[] = 
{
    &IID_IAccessible,
    0
};


#define _Accessible2_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _Accessible2, pIID, n)

int __stdcall _Accessible2_IID_Lookup( const IID * pIID, int * pIndex )
{
    
    if(!_Accessible2_CHECK_IID(0))
        {
        *pIndex = 0;
        return 1;
        }

    return 0;
}

const ExtendedProxyFileInfo Accessible2_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _Accessible2_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _Accessible2_StubVtblList,
    (const PCInterfaceName * ) & _Accessible2_InterfaceNamesList,
    (const IID ** ) & _Accessible2_BaseIIDList,
    & _Accessible2_IID_Lookup, 
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

