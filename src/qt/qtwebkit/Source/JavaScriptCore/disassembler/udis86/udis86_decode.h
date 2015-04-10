/* udis86 - libudis86/decode.h
 *
 * Copyright (c) 2002-2009 Vivek Thampi
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 *     * Redistributions of source code must retain the above copyright notice, 
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, 
 *       this list of conditions and the following disclaimer in the documentation 
 *       and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef UD_DECODE_H
#define UD_DECODE_H

#include "udis86_types.h"
#include "udis86_itab.h"

#define MAX_INSN_LENGTH 15

/* register classes */
#define T_NONE  0
#define T_GPR   1
#define T_MMX   2
#define T_CRG   3
#define T_DBG   4
#define T_SEG   5
#define T_XMM   6

/* itab prefix bits */
#define P_none          ( 0 )
#define P_cast          ( 1 << 0 )
#define P_CAST(n)       ( ( n >> 0 ) & 1 )
#define P_c1            ( 1 << 0 )
#define P_C1(n)         ( ( n >> 0 ) & 1 )
#define P_rexb          ( 1 << 1 )
#define P_REXB(n)       ( ( n >> 1 ) & 1 )
#define P_depM          ( 1 << 2 )
#define P_DEPM(n)       ( ( n >> 2 ) & 1 )
#define P_c3            ( 1 << 3 )
#define P_C3(n)         ( ( n >> 3 ) & 1 )
#define P_inv64         ( 1 << 4 )
#define P_INV64(n)      ( ( n >> 4 ) & 1 )
#define P_rexw          ( 1 << 5 )
#define P_REXW(n)       ( ( n >> 5 ) & 1 )
#define P_c2            ( 1 << 6 )
#define P_C2(n)         ( ( n >> 6 ) & 1 )
#define P_def64         ( 1 << 7 )
#define P_DEF64(n)      ( ( n >> 7 ) & 1 )
#define P_rexr          ( 1 << 8 )
#define P_REXR(n)       ( ( n >> 8 ) & 1 )
#define P_oso           ( 1 << 9 )
#define P_OSO(n)        ( ( n >> 9 ) & 1 )
#define P_aso           ( 1 << 10 )
#define P_ASO(n)        ( ( n >> 10 ) & 1 )
#define P_rexx          ( 1 << 11 )
#define P_REXX(n)       ( ( n >> 11 ) & 1 )
#define P_ImpAddr       ( 1 << 12 )
#define P_IMPADDR(n)    ( ( n >> 12 ) & 1 )
#define P_seg           ( 1 << 13 )
#define P_SEG(n)        ( ( n >> 13 ) & 1 )
#define P_sext          ( 1 << 14 )
#define P_SEXT(n)       ( ( n >> 14 ) & 1 )

/* rex prefix bits */
#define REX_W(r)        ( ( 0xF & ( r ) )  >> 3 )
#define REX_R(r)        ( ( 0x7 & ( r ) )  >> 2 )
#define REX_X(r)        ( ( 0x3 & ( r ) )  >> 1 )
#define REX_B(r)        ( ( 0x1 & ( r ) )  >> 0 )
#define REX_PFX_MASK(n) ( ( P_REXW(n) << 3 ) | \
                          ( P_REXR(n) << 2 ) | \
                          ( P_REXX(n) << 1 ) | \
                          ( P_REXB(n) << 0 ) )

/* scable-index-base bits */
#define SIB_S(b)        ( ( b ) >> 6 )
#define SIB_I(b)        ( ( ( b ) >> 3 ) & 7 )
#define SIB_B(b)        ( ( b ) & 7 )

/* modrm bits */
#define MODRM_REG(b)    ( ( ( b ) >> 3 ) & 7 )
#define MODRM_NNN(b)    ( ( ( b ) >> 3 ) & 7 )
#define MODRM_MOD(b)    ( ( ( b ) >> 6 ) & 3 )
#define MODRM_RM(b)     ( ( b ) & 7 )

/* operand type constants -- order is important! */

enum ud_operand_code {
    OP_NONE,

    OP_A,      OP_E,      OP_M,       OP_G,       
    OP_I,

    OP_AL,     OP_CL,     OP_DL,      OP_BL,
    OP_AH,     OP_CH,     OP_DH,      OP_BH,

    OP_ALr8b,  OP_CLr9b,  OP_DLr10b,  OP_BLr11b,
    OP_AHr12b, OP_CHr13b, OP_DHr14b,  OP_BHr15b,

    OP_AX,     OP_CX,     OP_DX,      OP_BX,
    OP_SI,     OP_DI,     OP_SP,      OP_BP,

    OP_rAX,    OP_rCX,    OP_rDX,     OP_rBX,  
    OP_rSP,    OP_rBP,    OP_rSI,     OP_rDI,

    OP_rAXr8,  OP_rCXr9,  OP_rDXr10,  OP_rBXr11,  
    OP_rSPr12, OP_rBPr13, OP_rSIr14,  OP_rDIr15,

    OP_eAX,    OP_eCX,    OP_eDX,     OP_eBX,
    OP_eSP,    OP_eBP,    OP_eSI,     OP_eDI,

    OP_ES,     OP_CS,     OP_SS,      OP_DS,  
    OP_FS,     OP_GS,

    OP_ST0,    OP_ST1,    OP_ST2,     OP_ST3,
    OP_ST4,    OP_ST5,    OP_ST6,     OP_ST7,

    OP_J,      OP_S,      OP_O,          
    OP_I1,     OP_I3, 

    OP_V,      OP_W,      OP_Q,       OP_P, 

    OP_R,      OP_C,  OP_D,       OP_VR,  OP_PR,

    OP_MR
} UD_ATTR_PACKED;


/* operand size constants */

enum ud_operand_size {
    SZ_NA  = 0,
    SZ_Z   = 1,
    SZ_V   = 2,
    SZ_P   = 3,
    SZ_WP  = 4,
    SZ_DP  = 5,
    SZ_MDQ = 6,
    SZ_RDQ = 7,

    /* the following values are used as is,
     * and thus hard-coded. changing them 
     * will break internals 
     */
    SZ_B   = 8,
    SZ_W   = 16,
    SZ_D   = 32,
    SZ_Q   = 64,
    SZ_T   = 80,
    SZ_O   = 128,

    SZ_WV  = 17,
    SZ_BV  = 18,
    SZ_DY  = 19

} UD_ATTR_PACKED;


/* A single operand of an entry in the instruction table. 
 * (internal use only)
 */
struct ud_itab_entry_operand 
{
  enum ud_operand_code type;
  enum ud_operand_size size;
};


/* A single entry in an instruction table. 
 *(internal use only)
 */
struct ud_itab_entry 
{
  enum ud_mnemonic_code         mnemonic;
  struct ud_itab_entry_operand  operand1;
  struct ud_itab_entry_operand  operand2;
  struct ud_itab_entry_operand  operand3;
  uint32_t                      prefix;
};

struct ud_lookup_table_list_entry {
    const uint16_t *table;
    enum ud_table_type type;
    const char *meta;
};
     

static inline unsigned int sse_pfx_idx( const unsigned int pfx ) 
{
    /* 00 = 0
     * f2 = 1
     * f3 = 2
     * 66 = 3
     */
    return ( ( pfx & 0xf ) + 1 ) / 2;
}

static inline unsigned int mode_idx( const unsigned int mode ) 
{
    /* 16 = 0
     * 32 = 1
     * 64 = 2
     */
    return ( mode / 32 );
}

static inline unsigned int modrm_mod_idx( const unsigned int mod )
{
    /* !11 = 0
     *  11 = 1
     */
    return ( mod + 1 ) / 4;
}

static inline unsigned int vendor_idx( const unsigned int vendor )
{
    switch ( vendor ) {
        case UD_VENDOR_AMD: return 0;
        case UD_VENDOR_INTEL: return 1;
        case UD_VENDOR_ANY: return 2; 
        default: return 2;
    }
}

static inline unsigned int is_group_ptr( uint16_t ptr )
{
    return ( 0x8000 & ptr );
}

static inline unsigned int group_idx( uint16_t ptr )
{
    return ( ~0x8000 & ptr );
}


extern struct ud_itab_entry ud_itab[];
extern struct ud_lookup_table_list_entry ud_lookup_table_list[];

#endif /* UD_DECODE_H */

/* vim:cindent
 * vim:expandtab
 * vim:ts=4
 * vim:sw=4
 */
