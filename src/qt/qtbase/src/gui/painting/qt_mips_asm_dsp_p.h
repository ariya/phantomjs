/****************************************************************************
**
** Copyright (C) 2013 Imagination Technologies Limited, www.imgtec.com
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QT_MIPS_ASM_DSP_H
#define QT_MIPS_ASM_DSP_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#if 0
#pragma qt_sync_stop_processing
#endif

#define zero $0
#define AT   $1
#define v0   $2
#define v1   $3
#define a0   $4
#define a1   $5
#define a2   $6
#define a3   $7
#define t0   $8
#define t1   $9
#define t2   $10
#define t3   $11
#define t4   $12
#define t5   $13
#define t6   $14
#define t7   $15
#define s0   $16
#define s1   $17
#define s2   $18
#define s3   $19
#define s4   $20
#define s5   $21
#define s6   $22
#define s7   $23
#define t8   $24
#define t9   $25
#define k0   $26
#define k1   $27
#define gp   $28
#define sp   $29
#define fp   $30
#define s8   $30
#define ra   $31

/*
 * LEAF_MIPS32R2 - declare leaf_mips32r2 routine
 */
#define LEAF_MIPS32R2(symbol)                           \
                .globl  symbol;                         \
                .align  2;                              \
                .type   symbol,@function;               \
                .ent    symbol,0;                       \
symbol:         .frame  sp, 0, ra;                      \
                .set    arch=mips32r2;                  \
                .set    noreorder;

/*
 * LEAF_MIPS_DSP - declare leaf_mips_dsp routine
 */
#define LEAF_MIPS_DSP(symbol)                           \
LEAF_MIPS32R2(symbol)                                   \
                .set    dsp;

/*
 * LEAF_MIPS_DSPR2 - declare leaf_mips_dspr2 routine
 */
#define LEAF_MIPS_DSPR2(symbol)                         \
LEAF_MIPS32R2(symbol)                                   \
                .set   dspr2;

/*
 * END - mark end of function
 */
#define END(function)                                   \
                .set    reorder;                        \
                .end    function;                       \
                .size   function,.-function

/*
 * BYTE_MUL operation on two pixels (in_1 and in_2) with two
 * multiplicator bytes, repl_a1 and repl_a2, which should be
 * prepered with:
 *   replv.ph   repl_a1, a1
 *   replv.ph   repl_a2, a2
 * to became such as:
 *   repl_a1 = | 00 | a1 | 00 | a1 |
 *   repl_a2 = | 00 | a2 | 00 | a2 |
 *
 * rounding_factor must have following value:
 *   li    rounding_factor, 0x00800080
 *
 * scratch(n) - temporary registers
 *
 * in_const: 1 -> (default) causes that in_1, in_2
 *           registers will remain unchanged after usage
 *           0 -> (or anything different then 1) causes
 *           that registers repl_a1, repl_a2 remain
 *           unchanged after usage
 */
.macro BYTE_MUL_x2 in_1, in_2, out_1, out_2                 \
                   repl_a1, repl_a2, rounding_factor,       \
                   scratch1, scratch2, scratch3, scratch4,  \
                   in_const = 1
    muleu_s.ph.qbl    \scratch1, \in_1,     \repl_a1
    muleu_s.ph.qbr    \scratch2, \in_1,     \repl_a1
    muleu_s.ph.qbl    \scratch3, \in_2,     \repl_a2
    muleu_s.ph.qbr    \scratch4, \in_2,     \repl_a2

.if \in_const == 1
    preceu.ph.qbla    \repl_a1,  \scratch1
    preceu.ph.qbla    \repl_a2,  \scratch2
    preceu.ph.qbla    \out_1,    \scratch3
    preceu.ph.qbla    \out_2,    \scratch4

    addu              \scratch1,  \repl_a1, \scratch1
    addu              \scratch2,  \repl_a2, \scratch2
.else
    preceu.ph.qbla    \in_1,      \scratch1
    preceu.ph.qbla    \in_2,      \scratch2
    preceu.ph.qbla    \out_1,     \scratch3
    preceu.ph.qbla    \out_2,     \scratch4

    addu              \scratch1,  \in_1,    \scratch1
    addu              \scratch2,  \in_2,    \scratch2
.endif

    addu              \out_1,     \out_1,   \scratch3
    addu              \out_2,     \out_2,   \scratch4

    addu              \scratch1,  \scratch1, \rounding_factor
    addu              \scratch2,  \scratch2, \rounding_factor
    addu              \scratch3,  \out_1,    \rounding_factor
    addu              \scratch4,  \out_2,    \rounding_factor

    precrq.qb.ph      \out_1,     \scratch1, \scratch2
    precrq.qb.ph      \out_2,     \scratch3, \scratch4

.endm

/*
 * BYTE_MUL operation on one pixel (in_1) with
 * multiplicator byte, repl_a1, which should be
 * prepered with:
 *   replv.ph   repl_a1, a1
 * to became such as:
 *   repl_a1 = | 00 | a1 | 00 | a1 |
 *
 * rounding_factor must have following value:
 *   li    rounding_factor, 0x00800080
 *
 * scratch(n) - temporary registers
 */
.macro BYTE_MUL in_1, out_1,                            \
                repl_a1, rounding_factor,               \
                scratch1, scratch2, scratch3, scratch4
    muleu_s.ph.qbl    \scratch1, \in_1,     \repl_a1
    muleu_s.ph.qbr    \scratch2, \in_1,     \repl_a1

    preceu.ph.qbla    \scratch3, \scratch1
    preceu.ph.qbla    \scratch4, \scratch2

    addu              \scratch1, \scratch1, \scratch3
    addu              \scratch1, \scratch1, \rounding_factor

    addu              \scratch2, \scratch2, \scratch4
    addu              \scratch2, \scratch2, \rounding_factor

    precrq.qb.ph      \out_1,    \scratch1, \scratch2

.endm

/*
 * macro for INTERPOLATE_PIXEL_255 operation
 * in_1 - First value to multiply
 * mul_1 - Multiplicator byte for first value
 * in_2 - Second value to multiply
 * mul_2 - Multiplicator byte for second value
 * rounding_factor and andi_factor should be prepared
 * as:
 *     li     rounding_factor, 0x00800080
 *     li     andi_factor,     0xff00ff00
 * scratch(n) - temporary registers
 */
.macro INTERPOLATE_PIXEL_255 in_1, mul_1,                            \
                             in_2, mul_2,                            \
                             out_1,                                  \
                             rounding_factor, andi_factor            \
                             scratch1, scratch2, scratch3, scratch4
# x part
    preceu.ph.qbra    \scratch1, \in_1
    preceu.ph.qbra    \scratch2, \in_2
    mul               \scratch1, \scratch1, \mul_1
    mul               \scratch2, \scratch2, \mul_2
# x>>8 part
    preceu.ph.qbla    \scratch3, \in_1
    preceu.ph.qbla    \scratch4, \in_2
    mul               \scratch3, \scratch3, \mul_1
    mul               \scratch4, \scratch4, \mul_2
# x part
    addu              \scratch1, \scratch1, \scratch2
    preceu.ph.qbla    \scratch2, \scratch1
    addu              \scratch1, \scratch1, \scratch2
    addu              \scratch1, \scratch1, \rounding_factor
    preceu.ph.qbla    \scratch1, \scratch1
# x>>8 part
    addu              \scratch3, \scratch3, \scratch4
    preceu.ph.qbla    \scratch4, \scratch3
    addu              \scratch3, \scratch3, \scratch4
    addu              \scratch3, \scratch3, \rounding_factor
    and               \scratch3, \scratch3, \andi_factor

    or                \out_1,    \scratch1, \scratch3
.endm

/*
 * Checks if stack offset is big enough for storing/restoring regs_num
 * number of register to/from stack. Stack offset must be greater than
 * or equal to the number of bytes needed for storing registers (regs_num*4).
 * Since MIPS ABI allows usage of first 16 bytes of stack frame (this is
 * preserved for input arguments of the functions, already stored in a0-a3),
 * stack size can be further optimized by utilizing this space.
 */
.macro CHECK_STACK_OFFSET regs_num, stack_offset
.if \stack_offset < \regs_num * 4 - 16
.error "Stack offset too small."
.endif
.endm

/*
 * Saves set of registers on stack. Maximum number of registers that
 * can be saved on stack is limitted to 14 (a0-a3, v0-v1 and s0-s7).
 * Stack offset is number of bytes that are added to stack pointer (sp)
 * before registers are pushed in order to provide enough space on stack
 * (offset must be multiple of 4, and must be big enough, as described by
 * CHECK_STACK_OFFSET macro). This macro is intended to be used in
 * combination with RESTORE_REGS_FROM_STACK macro. Example:
 *  SAVE_REGS_ON_STACK      4, v0, v1, s0, s1
 *  RESTORE_REGS_FROM_STACK 4, v0, v1, s0, s1
 */
.macro SAVE_REGS_ON_STACK stack_offset = 0, r1, \
                          r2  = 0, r3  = 0, r4  = 0, \
                          r5  = 0, r6  = 0, r7  = 0, \
                          r8  = 0, r9  = 0, r10 = 0, \
                          r11 = 0, r12 = 0, r13 = 0, \
                          r14 = 0
    .if (\stack_offset < 0) || (\stack_offset - (\stack_offset / 4) * 4)
    .error "Stack offset must be positive and multiple of 4."
    .endif
    .if \stack_offset != 0
    addiu           sp, sp, -\stack_offset
    .endif
    sw              \r1, 0(sp)
    .if \r2 != 0
    sw              \r2, 4(sp)
    .endif
    .if \r3 != 0
    sw              \r3, 8(sp)
    .endif
    .if \r4 != 0
    sw              \r4, 12(sp)
    .endif
    .if \r5 != 0
    CHECK_STACK_OFFSET 5, \stack_offset
    sw              \r5, 16(sp)
    .endif
    .if \r6 != 0
    CHECK_STACK_OFFSET 6, \stack_offset
    sw              \r6, 20(sp)
    .endif
    .if \r7 != 0
    CHECK_STACK_OFFSET 7, \stack_offset
    sw              \r7, 24(sp)
    .endif
    .if \r8 != 0
    CHECK_STACK_OFFSET 8, \stack_offset
    sw              \r8, 28(sp)
    .endif
    .if \r9 != 0
    CHECK_STACK_OFFSET 9, \stack_offset
    sw              \r9, 32(sp)
    .endif
    .if \r10 != 0
    CHECK_STACK_OFFSET 10, \stack_offset
    sw              \r10, 36(sp)
    .endif
    .if \r11 != 0
    CHECK_STACK_OFFSET 11, \stack_offset
    sw              \r11, 40(sp)
    .endif
    .if \r12 != 0
    CHECK_STACK_OFFSET 12, \stack_offset
    sw              \r12, 44(sp)
    .endif
    .if \r13 != 0
    CHECK_STACK_OFFSET 13, \stack_offset
    sw              \r13, 48(sp)
    .endif
    .if \r14 != 0
    CHECK_STACK_OFFSET 14, \stack_offset
    sw              \r14, 52(sp)
    .endif
.endm

/*
 * Restores set of registers from stack. Maximum number of registers that
 * can be restored from stack is limitted to 14 (a0-a3, v0-v1 and s0-s7).
 * Stack offset is number of bytes that are added to stack pointer (sp)
 * after registers are restored (offset must be multiple of 4, and must
 * be big enough, as described by CHECK_STACK_OFFSET macro). This macro is
 * intended to be used in combination with RESTORE_REGS_FROM_STACK macro.
 * Example:
 *  SAVE_REGS_ON_STACK      4, v0, v1, s0, s1
 *  RESTORE_REGS_FROM_STACK 4, v0, v1, s0, s1
 */
.macro RESTORE_REGS_FROM_STACK stack_offset = 0, r1, \
                               r2  = 0, r3  = 0, r4  = 0, \
                               r5  = 0, r6  = 0, r7  = 0, \
                               r8  = 0, r9  = 0, r10 = 0, \
                               r11 = 0, r12 = 0, r13 = 0, \
                               r14 = 0
    .if (\stack_offset < 0) || (\stack_offset - (\stack_offset/4)*4)
    .error "Stack offset must be pozitive and multiple of 4."
    .endif
    lw              \r1, 0(sp)
    .if \r2 != 0
    lw              \r2, 4(sp)
    .endif
    .if \r3 != 0
    lw              \r3, 8(sp)
    .endif
    .if \r4 != 0
    lw              \r4, 12(sp)
    .endif
    .if \r5 != 0
    CHECK_STACK_OFFSET 5, \stack_offset
    lw              \r5, 16(sp)
    .endif
    .if \r6 != 0
    CHECK_STACK_OFFSET 6, \stack_offset
    lw              \r6, 20(sp)
    .endif
    .if \r7 != 0
    CHECK_STACK_OFFSET 7, \stack_offset
    lw              \r7, 24(sp)
    .endif
    .if \r8 != 0
    CHECK_STACK_OFFSET 8, \stack_offset
    lw              \r8, 28(sp)
    .endif
    .if \r9 != 0
    CHECK_STACK_OFFSET 9, \stack_offset
    lw              \r9, 32(sp)
    .endif
    .if \r10 != 0
    CHECK_STACK_OFFSET 10, \stack_offset
    lw              \r10, 36(sp)
    .endif
    .if \r11 != 0
    CHECK_STACK_OFFSET 11, \stack_offset
    lw              \r11, 40(sp)
    .endif
    .if \r12 != 0
    CHECK_STACK_OFFSET 12, \stack_offset
    lw              \r12, 44(sp)
    .endif
    .if \r13 != 0
    CHECK_STACK_OFFSET 13, \stack_offset
    lw              \r13, 48(sp)
    .endif
    .if \r14 != 0
    CHECK_STACK_OFFSET 14, \stack_offset
    lw              \r14, 52(sp)
    .endif
    .if \stack_offset != 0
    addiu           sp, sp, \stack_offset
    .endif
.endm

#endif // QT_MIPS_ASM_DSP_H
