/* udis86 - libudis86/syn-intel.c
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

#include "udis86_types.h"
#include "udis86_extern.h"
#include "udis86_decode.h"
#include "udis86_itab.h"
#include "udis86_syn.h"

/* -----------------------------------------------------------------------------
 * opr_cast() - Prints an operand cast.
 * -----------------------------------------------------------------------------
 */
static void 
opr_cast(struct ud* u, struct ud_operand* op)
{
  switch(op->size) {
	case  8: mkasm(u, "byte " ); break;
	case 16: mkasm(u, "word " ); break;
	case 32: mkasm(u, "dword "); break;
	case 64: mkasm(u, "qword "); break;
	case 80: mkasm(u, "tword "); break;
	default: break;
  }
  if (u->br_far)
	mkasm(u, "far "); 
}

/* -----------------------------------------------------------------------------
 * gen_operand() - Generates assembly output for each operand.
 * -----------------------------------------------------------------------------
 */
static void gen_operand(struct ud* u, struct ud_operand* op, int syn_cast)
{
  switch(op->type) {
	case UD_OP_REG:
		mkasm(u, "%s", ud_reg_tab[op->base - UD_R_AL]);
		break;

	case UD_OP_MEM: {

		int op_f = 0;

		if (syn_cast) 
			opr_cast(u, op);

		mkasm(u, "[");

		if (u->pfx_seg)
			mkasm(u, "%s:", ud_reg_tab[u->pfx_seg - UD_R_AL]);

		if (op->base) {
			mkasm(u, "%s", ud_reg_tab[op->base - UD_R_AL]);
			op_f = 1;
		}

		if (op->index) {
			if (op_f)
				mkasm(u, "+");
			mkasm(u, "%s", ud_reg_tab[op->index - UD_R_AL]);
			op_f = 1;
		}

		if (op->scale)
			mkasm(u, "*%d", op->scale);

		if (op->offset == 8) {
			if (op->lval.sbyte < 0)
				mkasm(u, "-0x%x", -op->lval.sbyte);
			else	mkasm(u, "%s0x%x", (op_f) ? "+" : "", op->lval.sbyte);
		}
		else if (op->offset == 16)
			mkasm(u, "%s0x%x", (op_f) ? "+" : "", op->lval.uword);
		else if (op->offset == 32) {
			if (u->adr_mode == 64) {
				if (op->lval.sdword < 0)
					mkasm(u, "-0x%x", -op->lval.sdword);
				else	mkasm(u, "%s0x%x", (op_f) ? "+" : "", op->lval.sdword);
			} 
			else	mkasm(u, "%s0x%lx", (op_f) ? "+" : "", (unsigned long)op->lval.udword);
		}
		else if (op->offset == 64) 
			mkasm(u, "%s0x" FMT64 "x", (op_f) ? "+" : "", op->lval.uqword);

		mkasm(u, "]");
		break;
	}
			
	case UD_OP_IMM: {
        int64_t  imm = 0;
        uint64_t sext_mask = 0xffffffffffffffffull;
        unsigned sext_size = op->size;

		if (syn_cast) 
            opr_cast(u, op);
        switch (op->size) {
            case  8: imm = op->lval.sbyte; break;
            case 16: imm = op->lval.sword; break;
            case 32: imm = op->lval.sdword; break;
            case 64: imm = op->lval.sqword; break;
        }
        if ( P_SEXT( u->itab_entry->prefix ) ) {
            sext_size = u->operand[ 0 ].size; 
            if ( u->mnemonic == UD_Ipush )
                /* push sign-extends to operand size */
                sext_size = u->opr_mode; 
        }
        if ( sext_size < 64 )
            sext_mask = ( 1ull << sext_size ) - 1;
        mkasm( u, "0x" FMT64 "x", imm & sext_mask ); 

		break;
    }


	case UD_OP_JIMM:
		if (syn_cast) opr_cast(u, op);
		switch (op->size) {
			case  8:
				mkasm(u, "0x" FMT64 "x", u->pc + op->lval.sbyte); 
				break;
			case 16:
				mkasm(u, "0x" FMT64 "x", ( u->pc + op->lval.sword ) & 0xffff );
				break;
			case 32:
				mkasm(u, "0x" FMT64 "x", ( u->pc + op->lval.sdword ) & 0xfffffffful );
				break;
			default:break;
		}
		break;

	case UD_OP_PTR:
		switch (op->size) {
			case 32:
				mkasm(u, "word 0x%x:0x%x", op->lval.ptr.seg, 
					op->lval.ptr.off & 0xFFFF);
				break;
			case 48:
				mkasm(u, "dword 0x%x:0x%lx", op->lval.ptr.seg, 
					(unsigned long)op->lval.ptr.off);
				break;
		}
		break;

	case UD_OP_CONST:
		if (syn_cast) opr_cast(u, op);
		mkasm(u, "%d", op->lval.udword);
		break;

	default: return;
  }
}

/* =============================================================================
 * translates to intel syntax 
 * =============================================================================
 */
extern void ud_translate_intel(struct ud* u)
{
  /* -- prefixes -- */

  /* check if P_OSO prefix is used */
  if (! P_OSO(u->itab_entry->prefix) && u->pfx_opr) {
	switch (u->dis_mode) {
		case 16: 
			mkasm(u, "o32 ");
			break;
		case 32:
		case 64:
 			mkasm(u, "o16 ");
			break;
	}
  }

  /* check if P_ASO prefix was used */
  if (! P_ASO(u->itab_entry->prefix) && u->pfx_adr) {
	switch (u->dis_mode) {
		case 16: 
			mkasm(u, "a32 ");
			break;
		case 32:
 			mkasm(u, "a16 ");
			break;
		case 64:
 			mkasm(u, "a32 ");
			break;
	}
  }

  if ( u->pfx_seg &&
        u->operand[0].type != UD_OP_MEM &&
        u->operand[1].type != UD_OP_MEM ) {
	   mkasm(u, "%s ", ud_reg_tab[u->pfx_seg - UD_R_AL]);
    }
  if (u->pfx_lock)
	mkasm(u, "lock ");
  if (u->pfx_rep)
	mkasm(u, "rep ");
  if (u->pfx_repne)
	mkasm(u, "repne ");

  /* print the instruction mnemonic */
  mkasm(u, "%s ", ud_lookup_mnemonic(u->mnemonic));

  /* operand 1 */
  if (u->operand[0].type != UD_NONE) {
    int cast = 0;
    if ( u->operand[0].type == UD_OP_IMM &&
         u->operand[1].type == UD_NONE )
        cast = u->c1;
    if ( u->operand[0].type == UD_OP_MEM ) {
        cast = u->c1;
        if ( u->operand[1].type == UD_OP_IMM ||
             u->operand[1].type == UD_OP_CONST ) 
            cast = 1;
        if ( u->operand[1].type == UD_NONE )
            cast = 1;
        if ( ( u->operand[0].size != u->operand[1].size ) && u->operand[1].size )
            cast = 1;
    } else if ( u->operand[ 0 ].type == UD_OP_JIMM ) {
        if ( u->operand[ 0 ].size > 8 ) cast = 1;
    }
	gen_operand(u, &u->operand[0], cast);
  }
  /* operand 2 */
  if (u->operand[1].type != UD_NONE) {
    int cast = 0;
	mkasm(u, ", ");
    if ( u->operand[1].type == UD_OP_MEM ) {
        cast = u->c1;
                
         if ( u->operand[0].type != UD_OP_REG )  
            cast = 1;
         if ( u->operand[0].size != u->operand[1].size && u->operand[1].size )
            cast = 1;
         if ( u->operand[0].type == UD_OP_REG &&
                u->operand[0].base >= UD_R_ES &&
                u->operand[0].base <= UD_R_GS )
            cast = 0;
    }
	gen_operand(u, &u->operand[1], cast );
  }

  /* operand 3 */
  if (u->operand[2].type != UD_NONE) {
	mkasm(u, ", ");
	gen_operand(u, &u->operand[2], u->c3);
  }
}

#endif // USE(UDIS86)

