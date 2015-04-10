/* udis86 - libudis86/decode.c
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

#include "config.h"

#if USE(UDIS86)

#include "udis86_extern.h"
#include "udis86_types.h"
#include "udis86_input.h"
#include "udis86_decode.h"
#include <wtf/Assertions.h>

#define dbg(x, n...)
/* #define dbg printf */

#ifndef __UD_STANDALONE__
# include <string.h>
#endif /* __UD_STANDALONE__ */

/* The max number of prefixes to an instruction */
#define MAX_PREFIXES    15

/* instruction aliases and special cases */
static struct ud_itab_entry s_ie__invalid = 
    { UD_Iinvalid, O_NONE, O_NONE, O_NONE, P_none };

static int
decode_ext(struct ud *u, uint16_t ptr);


static inline int
eff_opr_mode(int dis_mode, int rex_w, int pfx_opr)
{
  if (dis_mode == 64) {
    return rex_w ? 64 : (pfx_opr ? 16 : 32);
  } else if (dis_mode == 32) {
    return pfx_opr ? 16 : 32;
  } else {
    ASSERT(dis_mode == 16);
    return pfx_opr ? 32 : 16;
  }
}


static inline int
eff_adr_mode(int dis_mode, int pfx_adr)
{
  if (dis_mode == 64) {
    return pfx_adr ? 32 : 64;
  } else if (dis_mode == 32) {
    return pfx_adr ? 16 : 32;
  } else {
    ASSERT(dis_mode == 16);
    return pfx_adr ? 32 : 16;
  }
}


/* Looks up mnemonic code in the mnemonic string table
 * Returns NULL if the mnemonic code is invalid
 */
const char * ud_lookup_mnemonic( enum ud_mnemonic_code c )
{
    return ud_mnemonics_str[ c ];
}


/* 
 * decode_prefixes
 *
 *  Extracts instruction prefixes.
 */
static int 
decode_prefixes(struct ud *u)
{
    unsigned int have_pfx = 1;
    unsigned int i;
    uint8_t curr;

    /* if in error state, bail out */
    if ( u->error ) 
        return -1; 

    /* keep going as long as there are prefixes available */
    for ( i = 0; have_pfx ; ++i ) {

        /* Get next byte. */
        ud_inp_next(u); 
        if ( u->error ) 
            return -1;
        curr = ud_inp_curr( u );

        /* rex prefixes in 64bit mode */
        if ( u->dis_mode == 64 && ( curr & 0xF0 ) == 0x40 ) {
            u->pfx_rex = curr;  
        } else {
            switch ( curr )  
            {
            case 0x2E : 
                u->pfx_seg = UD_R_CS; 
                u->pfx_rex = 0;
                break;
            case 0x36 :     
                u->pfx_seg = UD_R_SS; 
                u->pfx_rex = 0;
                break;
            case 0x3E : 
                u->pfx_seg = UD_R_DS; 
                u->pfx_rex = 0;
                break;
            case 0x26 : 
                u->pfx_seg = UD_R_ES; 
                u->pfx_rex = 0;
                break;
            case 0x64 : 
                u->pfx_seg = UD_R_FS; 
                u->pfx_rex = 0;
                break;
            case 0x65 : 
                u->pfx_seg = UD_R_GS; 
                u->pfx_rex = 0;
                break;
            case 0x67 : /* adress-size override prefix */ 
                u->pfx_adr = 0x67;
                u->pfx_rex = 0;
                break;
            case 0xF0 : 
                u->pfx_lock = 0xF0;
                u->pfx_rex  = 0;
                break;
            case 0x66: 
                /* the 0x66 sse prefix is only effective if no other sse prefix
                 * has already been specified.
                 */
                if ( !u->pfx_insn ) u->pfx_insn = 0x66;
                u->pfx_opr = 0x66;           
                u->pfx_rex = 0;
                break;
            case 0xF2:
                u->pfx_insn  = 0xF2;
                u->pfx_repne = 0xF2; 
                u->pfx_rex   = 0;
                break;
            case 0xF3:
                u->pfx_insn = 0xF3;
                u->pfx_rep  = 0xF3; 
                u->pfx_repe = 0xF3; 
                u->pfx_rex  = 0;
                break;
            default : 
                /* No more prefixes */
                have_pfx = 0;
                break;
            }
        }

        /* check if we reached max instruction length */
        if ( i + 1 == MAX_INSN_LENGTH ) {
            u->error = 1;
            break;
        }
    }

    /* return status */
    if ( u->error ) 
        return -1; 

    /* rewind back one byte in stream, since the above loop 
     * stops with a non-prefix byte. 
     */
    ud_inp_back(u);
    return 0;
}


static inline unsigned int modrm( struct ud * u )
{
    if ( !u->have_modrm ) {
        u->modrm = ud_inp_next( u );
        u->have_modrm = 1;
    }
    return u->modrm;
}


static unsigned int resolve_operand_size( const struct ud * u, unsigned int s )
{
    switch ( s ) 
    {
    case SZ_V:
        return ( u->opr_mode );
    case SZ_Z:  
        return ( u->opr_mode == 16 ) ? 16 : 32;
    case SZ_P:  
        return ( u->opr_mode == 16 ) ? SZ_WP : SZ_DP;
    case SZ_MDQ:
        return ( u->opr_mode == 16 ) ? 32 : u->opr_mode;
    case SZ_RDQ:
        return ( u->dis_mode == 64 ) ? 64 : 32;
    default:
        return s;
    }
}


static int resolve_mnemonic( struct ud* u )
{
  /* far/near flags */
  u->br_far = 0;
  u->br_near = 0;
  /* readjust operand sizes for call/jmp instrcutions */
  if ( u->mnemonic == UD_Icall || u->mnemonic == UD_Ijmp ) {
    /* WP: 16:16 pointer */
    if ( u->operand[ 0 ].size == SZ_WP ) {
        u->operand[ 0 ].size = 16;
        u->br_far = 1;
        u->br_near= 0;
    /* DP: 32:32 pointer */
    } else if ( u->operand[ 0 ].size == SZ_DP ) {
        u->operand[ 0 ].size = 32;
        u->br_far = 1;
        u->br_near= 0;
    } else {
        u->br_far = 0;
        u->br_near= 1;
    }
  /* resolve 3dnow weirdness. */
  } else if ( u->mnemonic == UD_I3dnow ) {
    u->mnemonic = ud_itab[ u->le->table[ ud_inp_curr( u )  ] ].mnemonic;
  }
  /* SWAPGS is only valid in 64bits mode */
  if ( u->mnemonic == UD_Iswapgs && u->dis_mode != 64 ) {
    u->error = 1;
    return -1;
  }

  if (u->mnemonic == UD_Ixchg) {
    if ((u->operand[0].type == UD_OP_REG && u->operand[0].base == UD_R_AX  &&
         u->operand[1].type == UD_OP_REG && u->operand[1].base == UD_R_AX) ||
        (u->operand[0].type == UD_OP_REG && u->operand[0].base == UD_R_EAX &&
         u->operand[1].type == UD_OP_REG && u->operand[1].base == UD_R_EAX)) {
      u->operand[0].type = UD_NONE;
      u->operand[1].type = UD_NONE;
      u->mnemonic = UD_Inop;
    }
  }

  if (u->mnemonic == UD_Inop && u->pfx_rep) {
    u->pfx_rep = 0;
    u->mnemonic = UD_Ipause;
  }
  return 0;
}


/* -----------------------------------------------------------------------------
 * decode_a()- Decodes operands of the type seg:offset
 * -----------------------------------------------------------------------------
 */
static void 
decode_a(struct ud* u, struct ud_operand *op)
{
  if (u->opr_mode == 16) {  
    /* seg16:off16 */
    op->type = UD_OP_PTR;
    op->size = 32;
    op->lval.ptr.off = ud_inp_uint16(u);
    op->lval.ptr.seg = ud_inp_uint16(u);
  } else {
    /* seg16:off32 */
    op->type = UD_OP_PTR;
    op->size = 48;
    op->lval.ptr.off = ud_inp_uint32(u);
    op->lval.ptr.seg = ud_inp_uint16(u);
  }
}

/* -----------------------------------------------------------------------------
 * decode_gpr() - Returns decoded General Purpose Register 
 * -----------------------------------------------------------------------------
 */
static enum ud_type 
decode_gpr(register struct ud* u, unsigned int s, unsigned char rm)
{
  s = resolve_operand_size(u, s);
        
  switch (s) {
    case 64:
        return UD_R_RAX + rm;
    case SZ_DP:
    case 32:
        return UD_R_EAX + rm;
    case SZ_WP:
    case 16:
        return UD_R_AX  + rm;
    case  8:
        if (u->dis_mode == 64 && u->pfx_rex) {
            if (rm >= 4)
                return UD_R_SPL + (rm-4);
            return UD_R_AL + rm;
        } else return UD_R_AL + rm;
    default:
        return 0;
  }
}

/* -----------------------------------------------------------------------------
 * resolve_gpr64() - 64bit General Purpose Register-Selection. 
 * -----------------------------------------------------------------------------
 */
static enum ud_type 
resolve_gpr64(struct ud* u, enum ud_operand_code gpr_op, enum ud_operand_size * size)
{
  if (gpr_op >= OP_rAXr8 && gpr_op <= OP_rDIr15)
    gpr_op = (gpr_op - OP_rAXr8) | (REX_B(u->pfx_rex) << 3);          
  else  gpr_op = (gpr_op - OP_rAX);

  if (u->opr_mode == 16) {
    *size = 16;
    return gpr_op + UD_R_AX;
  }
  if (u->dis_mode == 32 || 
    (u->opr_mode == 32 && ! (REX_W(u->pfx_rex) || u->default64))) {
    *size = 32;
    return gpr_op + UD_R_EAX;
  }

  *size = 64;
  return gpr_op + UD_R_RAX;
}

/* -----------------------------------------------------------------------------
 * resolve_gpr32 () - 32bit General Purpose Register-Selection. 
 * -----------------------------------------------------------------------------
 */
static enum ud_type 
resolve_gpr32(struct ud* u, enum ud_operand_code gpr_op)
{
  gpr_op = gpr_op - OP_eAX;

  if (u->opr_mode == 16) 
    return gpr_op + UD_R_AX;

  return gpr_op +  UD_R_EAX;
}

/* -----------------------------------------------------------------------------
 * resolve_reg() - Resolves the register type 
 * -----------------------------------------------------------------------------
 */
static enum ud_type 
resolve_reg(struct ud* u, unsigned int type, unsigned char i)
{
  switch (type) {
    case T_MMX :    return UD_R_MM0  + (i & 7);
    case T_XMM :    return UD_R_XMM0 + i;
    case T_CRG :    return UD_R_CR0  + i;
    case T_DBG :    return UD_R_DR0  + i;
    case T_SEG : {
      /*
       * Only 6 segment registers, anything else is an error.
       */
      if ((i & 7) > 5) {
        u->error = 1;
      } else {
        return UD_R_ES + (i & 7);
      }
    }
    case T_NONE:
    default:    return UD_NONE;
  }
}

/* -----------------------------------------------------------------------------
 * decode_imm() - Decodes Immediate values.
 * -----------------------------------------------------------------------------
 */
static void 
decode_imm(struct ud* u, unsigned int s, struct ud_operand *op)
{
  op->size = resolve_operand_size(u, s);
  op->type = UD_OP_IMM;

  switch (op->size) {
    case  8: op->lval.sbyte = ud_inp_uint8(u);   break;
    case 16: op->lval.uword = ud_inp_uint16(u);  break;
    case 32: op->lval.udword = ud_inp_uint32(u); break;
    case 64: op->lval.uqword = ud_inp_uint64(u); break;
    default: return;
  }
}


/*
 * decode_modrm_reg
 *
 *    Decodes reg field of mod/rm byte
 * 
 */
static void
decode_modrm_reg(struct ud         *u, 
                 struct ud_operand *operand,
                 unsigned int       type,
                 unsigned int       size)
{
  uint8_t reg = (REX_R(u->pfx_rex) << 3) | MODRM_REG(modrm(u));
  operand->type = UD_OP_REG;
  operand->size = resolve_operand_size(u, size);

  if (type == T_GPR) {
    operand->base = decode_gpr(u, operand->size, reg);
  } else {
    operand->base = resolve_reg(u, type, reg);
  }
}


/*
 * decode_modrm_rm
 *
 *    Decodes rm field of mod/rm byte
 * 
 */
static void 
decode_modrm_rm(struct ud         *u, 
                struct ud_operand *op,
                unsigned char      type,
                unsigned int       size)

{
  unsigned char mod, rm, reg;

  /* get mod, r/m and reg fields */
  mod = MODRM_MOD(modrm(u));
  rm  = (REX_B(u->pfx_rex) << 3) | MODRM_RM(modrm(u));
  reg = (REX_R(u->pfx_rex) << 3) | MODRM_REG(modrm(u));

  op->size = resolve_operand_size(u, size);

  /* 
   * If mod is 11b, then the modrm.rm specifies a register.
   *
   */
  if (mod == 3) {
    op->type = UD_OP_REG;
    if (type ==  T_GPR) {
      op->base = decode_gpr(u, op->size, rm);
    } else {
      op->base = resolve_reg(u, type, (REX_B(u->pfx_rex) << 3) | (rm & 7));
    }
    return;
  } 


  /* 
   * !11 => Memory Address
   */  
  op->type = UD_OP_MEM;

  if (u->adr_mode == 64) {
    op->base = UD_R_RAX + rm;
    if (mod == 1) {
      op->offset = 8;
    } else if (mod == 2) {
      op->offset = 32;
    } else if (mod == 0 && (rm & 7) == 5) {           
      op->base = UD_R_RIP;
      op->offset = 32;
    } else {
      op->offset = 0;
    }
    /* 
     * Scale-Index-Base (SIB) 
     */
    if ((rm & 7) == 4) {
      ud_inp_next(u);
      
      op->scale = (1 << SIB_S(ud_inp_curr(u))) & ~1;
      op->index = UD_R_RAX + (SIB_I(ud_inp_curr(u)) | (REX_X(u->pfx_rex) << 3));
      op->base  = UD_R_RAX + (SIB_B(ud_inp_curr(u)) | (REX_B(u->pfx_rex) << 3));

      /* special conditions for base reference */
      if (op->index == UD_R_RSP) {
        op->index = UD_NONE;
        op->scale = UD_NONE;
      }

      if (op->base == UD_R_RBP || op->base == UD_R_R13) {
        if (mod == 0) {
          op->base = UD_NONE;
        } 
        if (mod == 1) {
          op->offset = 8;
        } else {
          op->offset = 32;
        }
      }
    }
  } else if (u->adr_mode == 32) {
    op->base = UD_R_EAX + rm;
    if (mod == 1) {
      op->offset = 8;
    } else if (mod == 2) {
      op->offset = 32;
    } else if (mod == 0 && rm == 5) {
      op->base = UD_NONE;
      op->offset = 32;
    } else {
      op->offset = 0;
    }

    /* Scale-Index-Base (SIB) */
    if ((rm & 7) == 4) {
      ud_inp_next(u);

      op->scale = (1 << SIB_S(ud_inp_curr(u))) & ~1;
      op->index = UD_R_EAX + (SIB_I(ud_inp_curr(u)) | (REX_X(u->pfx_rex) << 3));
      op->base  = UD_R_EAX + (SIB_B(ud_inp_curr(u)) | (REX_B(u->pfx_rex) << 3));

      if (op->index == UD_R_ESP) {
        op->index = UD_NONE;
        op->scale = UD_NONE;
      }

      /* special condition for base reference */
      if (op->base == UD_R_EBP) {
        if (mod == 0) {
          op->base = UD_NONE;
        } 
        if (mod == 1) {
          op->offset = 8;
        } else {
          op->offset = 32;
        }
      }
    }
  } else {
    const unsigned int bases[]   = { UD_R_BX, UD_R_BX, UD_R_BP, UD_R_BP,
                                     UD_R_SI, UD_R_DI, UD_R_BP, UD_R_BX };
    const unsigned int indices[] = { UD_R_SI, UD_R_DI, UD_R_SI, UD_R_DI,
                                     UD_NONE, UD_NONE, UD_NONE, UD_NONE };
    op->base  = bases[rm & 7];
    op->index = indices[rm & 7];
    if (mod == 0 && rm == 6) {
      op->offset= 16;
      op->base = UD_NONE;
    } else if (mod == 1) {
      op->offset = 8;
    } else if (mod == 2) { 
      op->offset = 16;
    }
  }

  /* 
   * extract offset, if any 
   */
  switch (op->offset) {
    case 8 : op->lval.ubyte  = ud_inp_uint8(u);  break;
    case 16: op->lval.uword  = ud_inp_uint16(u); break;
    case 32: op->lval.udword = ud_inp_uint32(u); break;
    case 64: op->lval.uqword = ud_inp_uint64(u); break;
    default: break;
  }
}

/* -----------------------------------------------------------------------------
 * decode_o() - Decodes offset
 * -----------------------------------------------------------------------------
 */
static void 
decode_o(struct ud* u, unsigned int s, struct ud_operand *op)
{
  switch (u->adr_mode) {
    case 64:
        op->offset = 64; 
        op->lval.uqword = ud_inp_uint64(u); 
        break;
    case 32:
        op->offset = 32; 
        op->lval.udword = ud_inp_uint32(u); 
        break;
    case 16:
        op->offset = 16; 
        op->lval.uword  = ud_inp_uint16(u); 
        break;
    default:
        return;
  }
  op->type = UD_OP_MEM;
  op->size = resolve_operand_size(u, s);
}

/* -----------------------------------------------------------------------------
 * decode_operands() - Disassembles Operands.
 * -----------------------------------------------------------------------------
 */
static int
decode_operand(struct ud           *u, 
               struct ud_operand   *operand,
               enum ud_operand_code type,
               unsigned int         size)
{
  switch (type) {
    case OP_A :
      decode_a(u, operand);
      break;
    case OP_MR:
      if (MODRM_MOD(modrm(u)) == 3) {
        decode_modrm_rm(u, operand, T_GPR, 
                        size == SZ_DY ? SZ_MDQ : SZ_V);
      } else if (size == SZ_WV) {
        decode_modrm_rm( u, operand, T_GPR, SZ_W);
      } else if (size == SZ_BV) {
        decode_modrm_rm( u, operand, T_GPR, SZ_B);
      } else if (size == SZ_DY) {
        decode_modrm_rm( u, operand, T_GPR, SZ_D);
      } else {
        ASSERT(!"unexpected size");
      }
      break;
    case OP_M:
      if (MODRM_MOD(modrm(u)) == 3) {
          u->error = 1;
      }
      /* intended fall through */
    case OP_E:
      decode_modrm_rm(u, operand, T_GPR, size);
      break;
      break;
    case OP_G:
      decode_modrm_reg(u, operand, T_GPR, size);
      break;
    case OP_I:
      decode_imm(u, size, operand);
      break;
    case OP_I1:
      operand->type = UD_OP_CONST;
      operand->lval.udword = 1;
      break;
    case OP_PR:
      if (MODRM_MOD(modrm(u)) != 3) {
          u->error = 1;
      }
      decode_modrm_rm(u, operand, T_MMX, size);
      break;
    case OP_P:
      decode_modrm_reg(u, operand, T_MMX, size);
      break;
    case OP_VR:
      if (MODRM_MOD(modrm(u)) != 3) {
          u->error = 1;
      }
      /* intended fall through */
    case OP_W:
      decode_modrm_rm(u, operand, T_XMM, size);
      break;
    case OP_V:
      decode_modrm_reg(u, operand, T_XMM, size);
      break;
    case OP_S:
      decode_modrm_reg(u, operand, T_SEG, size);
      break;
    case OP_AL:
    case OP_CL:
    case OP_DL:
    case OP_BL:
    case OP_AH:
    case OP_CH:
    case OP_DH:
    case OP_BH:
      operand->type = UD_OP_REG;
      operand->base = UD_R_AL + (type - OP_AL);
      operand->size = 8;
      break;
    case OP_DX:
      operand->type = UD_OP_REG;
      operand->base = UD_R_DX;
      operand->size = 16;
      break;
    case OP_O:
      decode_o(u, size, operand);
      break;
    case OP_rAXr8: 
    case OP_rCXr9: 
    case OP_rDXr10: 
    case OP_rBXr11:
    case OP_rSPr12: 
    case OP_rBPr13: 
    case OP_rSIr14: 
    case OP_rDIr15:
    case OP_rAX: 
    case OP_rCX: 
    case OP_rDX: 
    case OP_rBX:
    case OP_rSP: 
    case OP_rBP: 
    case OP_rSI: 
    case OP_rDI:
      operand->type = UD_OP_REG;
      operand->base = resolve_gpr64(u, type, &operand->size);
      break;
    case OP_ALr8b:
    case OP_CLr9b: 
    case OP_DLr10b: 
    case OP_BLr11b:
    case OP_AHr12b:
    case OP_CHr13b:
    case OP_DHr14b:
    case OP_BHr15b: {
      ud_type_t gpr = (type - OP_ALr8b) + UD_R_AL
                        + (REX_B(u->pfx_rex) << 3);
      if (UD_R_AH <= gpr && u->pfx_rex) {
        gpr = gpr + 4;
      }
      operand->type = UD_OP_REG;
      operand->base = gpr;
      break;
    }
    case OP_eAX: 
    case OP_eCX: 
    case OP_eDX: 
    case OP_eBX:
    case OP_eSP: 
    case OP_eBP: 
    case OP_eSI: 
    case OP_eDI:
      operand->type = UD_OP_REG;
      operand->base = resolve_gpr32(u, type);
      operand->size = u->opr_mode == 16 ? 16 : 32;
      break;
    case OP_ES: 
    case OP_CS: 
    case OP_DS:
    case OP_SS: 
    case OP_FS: 
    case OP_GS:
      /* in 64bits mode, only fs and gs are allowed */
      if (u->dis_mode == 64) {
        if (type != OP_FS && type != OP_GS) {
          u->error= 1;
        }
      }
      operand->type = UD_OP_REG;
      operand->base = (type - OP_ES) + UD_R_ES;
      operand->size = 16;
      break;
    case OP_J :
      decode_imm(u, size, operand);
      operand->type = UD_OP_JIMM;
      break ;
    case OP_Q:
      decode_modrm_rm(u, operand, T_MMX, size);
      break;
    case OP_R :
      decode_modrm_rm(u, operand, T_GPR, size);
      break;
    case OP_C:
      decode_modrm_reg(u, operand, T_CRG, size);
      break;
    case OP_D:
      decode_modrm_reg(u, operand, T_DBG, size);
      break;
    case OP_I3 :
      operand->type = UD_OP_CONST;
      operand->lval.sbyte = 3;
      break;
    case OP_ST0: 
    case OP_ST1: 
    case OP_ST2: 
    case OP_ST3:
    case OP_ST4:
    case OP_ST5: 
    case OP_ST6: 
    case OP_ST7:
      operand->type = UD_OP_REG;
      operand->base = (type - OP_ST0) + UD_R_ST0;
      operand->size = 0;
      break;
    case OP_AX:
      operand->type = UD_OP_REG;
      operand->base = UD_R_AX;
      operand->size = 16;
      break;
    default :
      operand->type = UD_NONE;
      break;
  }
  return 0;
}


/* 
 * decode_operands
 *
 *    Disassemble upto 3 operands of the current instruction being
 *    disassembled. By the end of the function, the operand fields
 *    of the ud structure will have been filled.
 */
static int
decode_operands(struct ud* u)
{
  decode_operand(u, &u->operand[0],
                    u->itab_entry->operand1.type,
                    u->itab_entry->operand1.size);
  decode_operand(u, &u->operand[1],
                    u->itab_entry->operand2.type,
                    u->itab_entry->operand2.size);
  decode_operand(u, &u->operand[2],
                    u->itab_entry->operand3.type,
                    u->itab_entry->operand3.size);
  return 0;
}
    
/* -----------------------------------------------------------------------------
 * clear_insn() - clear instruction structure
 * -----------------------------------------------------------------------------
 */
static void
clear_insn(register struct ud* u)
{
  u->error     = 0;
  u->pfx_seg   = 0;
  u->pfx_opr   = 0;
  u->pfx_adr   = 0;
  u->pfx_lock  = 0;
  u->pfx_repne = 0;
  u->pfx_rep   = 0;
  u->pfx_repe  = 0;
  u->pfx_rex   = 0;
  u->pfx_insn  = 0;
  u->mnemonic  = UD_Inone;
  u->itab_entry = NULL;
  u->have_modrm = 0;

  memset( &u->operand[ 0 ], 0, sizeof( struct ud_operand ) );
  memset( &u->operand[ 1 ], 0, sizeof( struct ud_operand ) );
  memset( &u->operand[ 2 ], 0, sizeof( struct ud_operand ) );
}

static int
resolve_mode( struct ud* u )
{
  /* if in error state, bail out */
  if ( u->error ) return -1; 

  /* propagate prefix effects */
  if ( u->dis_mode == 64 ) {  /* set 64bit-mode flags */

    /* Check validity of  instruction m64 */
    if ( P_INV64( u->itab_entry->prefix ) ) {
        u->error = 1;
        return -1;
    }

    /* effective rex prefix is the  effective mask for the 
     * instruction hard-coded in the opcode map.
     */
    u->pfx_rex = ( u->pfx_rex & 0x40 ) | 
                 ( u->pfx_rex & REX_PFX_MASK( u->itab_entry->prefix ) ); 

    /* whether this instruction has a default operand size of 
     * 64bit, also hardcoded into the opcode map.
     */
    u->default64 = P_DEF64( u->itab_entry->prefix ); 
    /* calculate effective operand size */
    if ( REX_W( u->pfx_rex ) ) {
        u->opr_mode = 64;
    } else if ( u->pfx_opr ) {
        u->opr_mode = 16;
    } else {
        /* unless the default opr size of instruction is 64,
         * the effective operand size in the absence of rex.w
         * prefix is 32.
         */
        u->opr_mode = ( u->default64 ) ? 64 : 32;
    }

    /* calculate effective address size */
    u->adr_mode = (u->pfx_adr) ? 32 : 64;
  } else if ( u->dis_mode == 32 ) { /* set 32bit-mode flags */
    u->opr_mode = ( u->pfx_opr ) ? 16 : 32;
    u->adr_mode = ( u->pfx_adr ) ? 16 : 32;
  } else if ( u->dis_mode == 16 ) { /* set 16bit-mode flags */
    u->opr_mode = ( u->pfx_opr ) ? 32 : 16;
    u->adr_mode = ( u->pfx_adr ) ? 32 : 16;
  }

  /* These flags determine which operand to apply the operand size
   * cast to.
   */
  u->c1 = ( P_C1( u->itab_entry->prefix ) ) ? 1 : 0;
  u->c2 = ( P_C2( u->itab_entry->prefix ) ) ? 1 : 0;
  u->c3 = ( P_C3( u->itab_entry->prefix ) ) ? 1 : 0;

  /* set flags for implicit addressing */
  u->implicit_addr = P_IMPADDR( u->itab_entry->prefix );

  return 0;
}

static int gen_hex( struct ud *u )
{
  unsigned int i;
  unsigned char *src_ptr = ud_inp_sess( u );
  char* src_hex;

  /* bail out if in error stat. */
  if ( u->error ) return -1; 
  /* output buffer pointe */
  src_hex = ( char* ) u->insn_hexcode;
  /* for each byte used to decode instruction */
  for ( i = 0; i < u->inp_ctr; ++i, ++src_ptr) {
    sprintf( src_hex, "%02x", *src_ptr & 0xFF );
    src_hex += 2;
  }
  return 0;
}


static inline int
decode_insn(struct ud *u, uint16_t ptr)
{
  ASSERT((ptr & 0x8000) == 0);
  u->itab_entry = &ud_itab[ ptr ];
  u->mnemonic = u->itab_entry->mnemonic;
  return (resolve_mode(u)     == 0 &&
          decode_operands(u)  == 0 &&
          resolve_mnemonic(u) == 0) ? 0 : -1;
}


/*
 * decode_3dnow()
 *
 *    Decoding 3dnow is a little tricky because of its strange opcode
 *    structure. The final opcode disambiguation depends on the last
 *    byte that comes after the operands have been decoded. Fortunately,
 *    all 3dnow instructions have the same set of operand types. So we
 *    go ahead and decode the instruction by picking an arbitrarily chosen
 *    valid entry in the table, decode the operands, and read the final
 *    byte to resolve the menmonic.
 */
static inline int
decode_3dnow(struct ud* u)
{
  uint16_t ptr;
  ASSERT(u->le->type == UD_TAB__OPC_3DNOW);
  ASSERT(u->le->table[0xc] != 0);
  decode_insn(u, u->le->table[0xc]);
  ud_inp_next(u); 
  if (u->error) {
    return -1;
  }
  ptr = u->le->table[ud_inp_curr(u)]; 
  ASSERT((ptr & 0x8000) == 0);
  u->mnemonic = ud_itab[ptr].mnemonic;
  return 0;
}


static int
decode_ssepfx(struct ud *u)
{
  uint8_t idx = ((u->pfx_insn & 0xf) + 1) / 2;
  if (u->le->table[idx] == 0) {
    idx = 0;
  }
  if (idx && u->le->table[idx] != 0) {
    /*
     * "Consume" the prefix as a part of the opcode, so it is no
     * longer exported as an instruction prefix.
     */
    switch (u->pfx_insn) {
      case 0xf2: 
        u->pfx_repne = 0;
        break;
      case 0xf3: 
        u->pfx_rep = 0;
        u->pfx_repe = 0;
        break;
      case 0x66: 
        u->pfx_opr = 0;
        break;
    }
  }
  return decode_ext(u, u->le->table[idx]);
}


/*
 * decode_ext()
 *
 *    Decode opcode extensions (if any)
 */
static int
decode_ext(struct ud *u, uint16_t ptr)
{
  uint8_t idx = 0;
  if ((ptr & 0x8000) == 0) {
    return decode_insn(u, ptr); 
  }
  u->le = &ud_lookup_table_list[(~0x8000 & ptr)];
  if (u->le->type == UD_TAB__OPC_3DNOW) {
    return decode_3dnow(u);
  }

  switch (u->le->type) {
    case UD_TAB__OPC_MOD:
      /* !11 = 0, 11 = 1 */
      idx = (MODRM_MOD(modrm(u)) + 1) / 4;
      break;
      /* disassembly mode/operand size/address size based tables.
       * 16 = 0,, 32 = 1, 64 = 2
       */
    case UD_TAB__OPC_MODE:
      idx = u->dis_mode / 32;
      break;
    case UD_TAB__OPC_OSIZE:
      idx = eff_opr_mode(u->dis_mode, REX_W(u->pfx_rex), u->pfx_opr) / 32;
      break;
    case UD_TAB__OPC_ASIZE:
      idx = eff_adr_mode(u->dis_mode, u->pfx_adr) / 32;
      break;
    case UD_TAB__OPC_X87:
      idx = modrm(u) - 0xC0;
      break;
    case UD_TAB__OPC_VENDOR:
      if (u->vendor == UD_VENDOR_ANY) {
        /* choose a valid entry */
        idx = (u->le->table[idx] != 0) ? 0 : 1;
      } else if (u->vendor == UD_VENDOR_AMD) {
        idx = 0;
      } else {
        idx = 1;
      }
      break;
    case UD_TAB__OPC_RM:
      idx = MODRM_RM(modrm(u));
      break;
    case UD_TAB__OPC_REG:
      idx = MODRM_REG(modrm(u));
      break;
    case UD_TAB__OPC_SSE:
      return decode_ssepfx(u);
    default:
      ASSERT(!"not reached");
      break;
  }

  return decode_ext(u, u->le->table[idx]);
}


static inline int
decode_opcode(struct ud *u)
{
  uint16_t ptr;
  ASSERT(u->le->type == UD_TAB__OPC_TABLE);
  ud_inp_next(u); 
  if (u->error) {
    return -1;
  }
  ptr = u->le->table[ud_inp_curr(u)];
  if (ptr & 0x8000) {
    u->le = &ud_lookup_table_list[ptr & ~0x8000];
    if (u->le->type == UD_TAB__OPC_TABLE) {
      return decode_opcode(u);
    }
  }
  return decode_ext(u, ptr);
}

 
/* =============================================================================
 * ud_decode() - Instruction decoder. Returns the number of bytes decoded.
 * =============================================================================
 */
unsigned int
ud_decode(struct ud *u)
{
  ud_inp_start(u);
  clear_insn(u);
  u->le = &ud_lookup_table_list[0];
  u->error = decode_prefixes(u) == -1 || 
             decode_opcode(u)   == -1 ||
             u->error;
  /* Handle decode error. */
  if (u->error) {
    /* clear out the decode data. */
    clear_insn(u);
    /* mark the sequence of bytes as invalid. */
    u->itab_entry = & s_ie__invalid;
    u->mnemonic = u->itab_entry->mnemonic;
  } 

    /* maybe this stray segment override byte
     * should be spewed out?
     */
    if ( !P_SEG( u->itab_entry->prefix ) && 
            u->operand[0].type != UD_OP_MEM &&
            u->operand[1].type != UD_OP_MEM )
        u->pfx_seg = 0;

  u->insn_offset = u->pc; /* set offset of instruction */
  u->insn_fill = 0;   /* set translation buffer index to 0 */
  u->pc += u->inp_ctr;    /* move program counter by bytes decoded */
  gen_hex( u );       /* generate hex code */

  /* return number of bytes disassembled. */
  return u->inp_ctr;
}

/*
vim: set ts=2 sw=2 expandtab
*/

#endif // USE(UDIS86)
