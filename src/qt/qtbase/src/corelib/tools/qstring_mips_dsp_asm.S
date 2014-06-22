/****************************************************************************
**
** Copyright (C) 2013 Imagination Technologies Limited, www.imgtec.com
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "../../gui/painting/qt_mips_asm_dsp_p.h"

.macro EXTRACT_HALVES_x2 src1, src2, hi_1, hi_2
#if defined(__MIPSEL) && __MIPSEL
    srl  \hi_1, \src1, 16
    ext  \src1, \src1, 0, 16
    srl  \hi_2, \src2, 16
    ext  \src2, \src2, 0, 16
#else
    ext  \hi_1, \src1, 0, 16
    srl  \src1, \src1, 16
    ext  \hi_2, \src2, 0, 16
    srl  \src2, \src2, 16
#endif
.endm


LEAF_MIPS_DSP(qt_ucstrncmp_mips_dsp_asm)
/*
 * Arguments:
 *   a0 - string_a (uint16_t*)
 *   a1 - string_b (uint16_t*)
 *   a2 - length   (uint32_t)
 *
 * Register usage:
 *   t0 - batches
 */

    move  v0, zero      /* result = 0 */
    andi   t0, a0, 0x3  /* t1 = string_a % 4 */
    andi   t1, a1, 0x3  /* t0 = string_b % 4 */
    or     t2, t0, t1   /* t2 = t0 | t1 */

    beqz   t2, 5f       /* both aligned */
     and   t2, t0 ,t1
    beqz   t2, 6f       /* one aligned */
     nop

    /*
     * Both strings are unaligned: read 1 halfword from each,
     * then fall-off to continue with the both-aligned case.
     */
    lhu    t0, 0 (a0)
    lhu    t1, 0 (a1)
    addiu  a2, a2, -1   /* len-- */
    sub    v0, t0, t1   /* v0 = t0-t1 */
    addiu  a0, a0, 2    /* string_a++ */
    bnez   v0, 0f       /* if (t0-t1): return */
     addiu a1, a1, 2    /* string_b++ */
    beqz   a2, 0f       /* if !len: return */
     /* next instruction (srl) fills delay branch slot */

5:  /* Both string pointers are aligned */
    srl    t0, a2, 3    /* batches = length / 8 */
    beqz   t0, 9f       /* if !batches: tail */
     andi  a2, a2, 0x7  /* length = length % 8 */

    SAVE_REGS_ON_STACK 0, s0, s1, s2, s3

1:  lw     t1,  0 (a0)  /* [a0 a1] */
    lw     t3,  4 (a0)  /* [a2 a3] */
    lw     t5,  8 (a0)  /* [a4 a5] */
    lw     t7, 12 (a0)  /* [a6 a7] */

    lw     t2,  0 (a1)  /* [b0 b1] */
    lw     t4,  4 (a1)  /* [b2 b3] */
    lw     t6,  8 (a1)  /* [b4 b5] */
    lw     t8, 12 (a1)  /* [b6 b7] */

    /*
     * Subtract elements one by one, if the result is zero
     * both halves of the registers (shorts) are equal.
     */
    subq.ph s0, t1, t2  /* [a0-b0 a1-b1] */
    subq.ph s1, t3, t4  /* [a2-b2 a3-b3] */

    bnez   s0, 1f
     subq.ph s2, t5, t6 /* [a4-b4 a5-b5] */
    bnez   s1, 2f
     subq.ph s3, t7, t8 /* [a6-b6 a7-b7] */
    bnez   s2, 3f
     addiu t0, t0, -1   /* batches-- */
    bnez   s3, 4f
     addiu a0, a0, 8*2  /* string_a += 8 */

    bnez   t0, 1b       /* if batches: loop */
     addiu a1, a1, 8*2  /* string_b += 8 */

    RESTORE_REGS_FROM_STACK 0, s0, s1, s2, s3

    bnez   a2, 9f       /* if length: tail */
     nop
    jr ra
     nop


 1: /* Check t1 [a0 a1] vs. t2 [b0 b1] */
    EXTRACT_HALVES_x2  t1, t2, t3, t4  /* a0, b0, a1, b1 */
    sub    v0, t1, t2
    RESTORE_REGS_FROM_STACK 0, s0, s1, s2, s3
    bnez   v0, 0f       /* if (a0-b0): return */
     nop
    jr     ra
     sub   v0, t3, t4   /* return a1-b1 */

 2: /* Check t3 [a2 a3] vs. t4 [b2 b3] */
    EXTRACT_HALVES_x2  t3, t4, t1, t2  /* a2, b2, a3, b3 */
    sub    v0, t3, t4
    RESTORE_REGS_FROM_STACK 0, s0, s1, s2, s3
    bnez   v0, 0f       /* if (a2-b2): return */
     nop
    jr     ra
     sub   v0, t1, t2   /* return a3-b3 */

 3: /* Check t5 [a4 a5] vs. t6 [b4 b5] */
    EXTRACT_HALVES_x2  t5, t6, t1, t2  /* a4, b4, a5, b5 */
    sub    v0, t5, t6
    RESTORE_REGS_FROM_STACK 0, s0, s1, s2, s3
    bnez   v0, 0f       /* if (a4-b4): return */
     nop
    jr     ra
     sub   v0, t1, t2   /* return a5-b5 */

 4: /* Check t7 [a6 a7] vs. t8 [b6 b7] */
    EXTRACT_HALVES_x2  t7, t8, t1, t2  /* a6, b6, a7, b7 */
    sub    v0, t7, t8
    RESTORE_REGS_FROM_STACK 0, s0, s1, s2, s3
    bnez   v0, 0f       /* if (a6-b6): return */
     nop
    jr     ra
     sub   v0, t1, t2


    /* Process remaining tail items. */
9:  lhu    t1, 0 (a0)   /* a */
    lhu    t2, 0 (a1)   /* b */
    addiu  a2, a2, -1   /* length-- */
    sub    v0, t1, t2   /* result = (a - b) */

    bnez   v0, 0f       /* if (a - b): return */
     addiu a0, a0, 2    /* string_a++ */

    bnez   a2, 9b       /* if length: loop */
     addiu a1, a1, 2    /* string_b++ */

0:  jr     ra
     nop


    /* One of the inputs is unaligned, do unrolled half-word loads */
6:  srl    t0, a2, 3    /* batches = length / 8 */
    andi  a2, a2, 0x7   /* length = length % 8 */

1:  lhu    t1,  0 (a0)
    lhu    t2,  0 (a1)
    lhu    t3,  2 (a0)
    lhu    t4,  2 (a1)
    lhu    t5,  4 (a0)
    lhu    t6,  4 (a1)
    lhu    t7,  6 (a0)
    lhu    t8,  6 (a1)

    sub    v0, t1, t2
    sub    t1, t3, t4

    bnez   v0, 0f
     sub   t2, t5, t6
    bnez   t1, 2f
     sub   t3, t7, t8
    bnez   t2, 3f
     lhu   t1,  8 (a0)
    bnez   t3, 4f
     lhu    t2,  8 (a1)

    lhu    t3, 10 (a0)
    lhu    t4, 10 (a1)
    lhu    t5, 12 (a0)
    lhu    t6, 12 (a1)
    lhu    t7, 14 (a0)
    lhu    t8, 14 (a1)

    sub    v0, t1, t2
    sub    t1, t3, t4

    bnez   v0, 0f
     sub   t2, t5, t6
    bnez   t1, 2f
     sub   t3, t7, t8
    bnez   t2, 3f
     addiu t0, t0, -1   /* batches-- */
    bnez   t3, 4f
     addiu a0, a0, 8*2  /* string_a += 8 */

    bnez   t0, 1b
     addiu a1, a1, 8*2  /* string_b += 8 */

    bnez   a2, 9b       /* if length: tail */
     nop

0:  jr     ra
     nop
2:  jr     ra
     move  v0, t1
3:  jr     ra
     move  v0, t2
4:  jr     ra
     move  v0, t3

END(qt_ucstrncmp_mips_dsp_asm)


#if defined(__mips_dspr2)
LEAF_MIPS_DSPR2(qt_fromlatin1_mips_asm_unroll8)
#else
LEAF_MIPS_DSP(qt_fromlatin1_mips_asm_unroll8)
#endif

    andi   t0, a1, 0x3
    beqz   t0, 9f       /* check that src is aligned */
     nop

1:  lbu    t1, 0 (a1)
    addiu  a1, a1,  1
    addiu  a2, a2, -1
    sh     t1, 0 (a0)
    beqz   a2, 0f
     andi  t0, a1, 0x3
    bnez   t0, 1b
     addiu a0, a0,  2

9:  /* source pointer is aligned: do batches of 8 elements */
    andi   t0, a0, 3    /* check if dst is aligned */
    bnez   t0, 6f
     srl   t0, a2, 3    /* batches = len / 8 */
    andi   a2, a2, 0x7  /* tail    = len % 8 */

    beqz   t0, 8f       /* if !batches: tail */
     nop

1:  lw     t1, 0 (a1)
    lw     t2, 4 (a1)

    addiu  a1, a1, 8*1
    addiu  t0, t0, -1

    preceu.ph.qbl t3, t1
    preceu.ph.qbr t1, t1
    preceu.ph.qbl t4, t2
    preceu.ph.qbr t2, t2

#if defined(__MIPSEL) && __MIPSEL
    sw     t1,  0 (a0)
    sw     t3,  4 (a0)
    sw     t2,  8 (a0)
    sw     t4, 12 (a0)
#else
    sw     t3,  0 (a0)
    sw     t1,  4 (a0)
    sw     t4,  8 (a0)
    sw     t2, 12 (a0)
#endif

    bnez   t0, 1b
     addiu a0, a0, 8*2

8:  /* process tail items */
    beqz   a2, 0f
     nop

1:  lbu    t1, 0 (a1)
    addiu  a2, a2, -1
    sh     t1, 0 (a0)
    addiu  a1, a1, 1
    bnez   a2, 1b
     addiu a0, a0, 2

0:  jr     ra
     nop

6:  beqz   t0, 8b
     andi  a2, a2, 7
7:  lw     t1, 0(a1)
    lw     t2, 4(a1)
    addiu  t0, t0, -1
    addiu  a1, a1, 8
    andi   t3, t1, 0xff
#if defined(__mips_dspr2)
    prepend t1, t2, 8
#else
    sll    t4, t4, 24
    srl    t1, t1, 8
    or     t1, t1, t4
#endif
    srl    t2, t2, 8
    preceu.ph.qbr t4, t1
    preceu.ph.qbl t1, t1
    preceu.ph.qbr t5, t2
    srl    t2, t2, 16
    sh     t3, 0(a0)
    sw     t4, 2(a0)
    sw     t1, 6(a0)
    sw     t5, 10(a0)
    sh     t2, 14(a0)
    bnez   t0, 7b
     addiu a0, a0, 16
    bnez   a2, 1b
     nop

    jr     ra
     nop

END(qt_fromlatin1_mips_asm_unroll8)


LEAF_MIPS_DSP(qt_fromlatin1_mips_asm_unroll4)
/*
 * Arguments:
 *   a0 - dst (uint16_t*)
 *   a1 - src (const char*)
 *   a2 - len (unsigned int)
 */

    /*
     * QString::fromLatin1_helper() already handles the len==0
     * case: assume that len is never zero.
     */
    srl    t0, a2, 2
    beqz   t0, 9f
     andi  a2, a2, 0x3

1:  lbu    t1, 0(a1)
    lbu    t2, 1(a1)
    lbu    t3, 2(a1)
    lbu    t4, 3(a1)
    sh     t1, 0(a0)
    sh     t2, 2(a0)
    sh     t3, 4(a0)
    sh     t4, 6(a0)
    addiu  t0, t0, -1
    addiu  a1, a1, 4
    bnez   t0, 1b
     addiu a0, a0, 8

8:  beqz   a2, 0f
     nop

9:  lbu    t1, 0(a1)
    addiu  a2, a2, -1
    addiu  a1, a1, 1
    sh     t1, 0(a0)
    bnez   a2, 9b
     addiu a0, a0, 2

0:  jr     ra
     nop

END(qt_fromlatin1_mips_asm_unroll4)


LEAF_MIPS_DSP(qt_toLatin1_mips_dsp_asm)
  /*
   * a0 - dst
   * a1 - src
   * a2 - length
   */

    addiu  t9, zero, 0x3f
    srl    t8, a2, 2
    beqz   t8, 2f
     andi  a2, a2, 3
1:
    lhu    t0, 0(a1)
    lhu    t1, 2(a1)
    lhu    t2, 4(a1)
    lhu    t3, 6(a1)
    srl    t4, t0, 8
    srl    t5, t1, 8
    srl    t6, t2, 8
    srl    t7, t3, 8
    movn   t0, t9, t4
    movn   t1, t9, t5
    movn   t2, t9, t6
    movn   t3, t9, t7
    addiu  a1, a1, 8
    addiu  t8, t8, -1
    sb     t0, 0(a0)
    sb     t1, 1(a0)
    sb     t2, 2(a0)
    sb     t3, 3(a0)
    bgtz   t8, 1b
     addiu a0, a0, 4
2:  beqz   a2, 4f
     nop
3:
    lhu    t0, 0(a1)
    addiu  a1, a1, 2
    addiu  a2, a2, -1
    srl    t1, t0, 8
    movn   t0, t9, t1
    sb     t0, 0(a0)
    bgtz   a2, 3b
     addiu a0, a0, 1
4:
    jr     ra
     nop

END(qt_toLatin1_mips_dsp_asm)

