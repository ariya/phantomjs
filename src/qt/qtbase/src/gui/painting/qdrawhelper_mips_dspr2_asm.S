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

#include "qt_mips_asm_dsp_p.h"

LEAF_MIPS_DSPR2(qConvertRgb16To32_asm_mips_dspr2)
/*
 * a0 - dst (a8r8g8b8)
 * a1 - src (r5g6b5)
 * a2 - w
 */

    beqz              a2, 3f
     nop
    addiu             t1, a2, -1
    beqz              t1, 2f
     nop
    li                t4, 0x07e007e0
    li                t5, 0x001F001F
/* Convert two pixels at time (2 x rgb565 -> 2 x rgb8888) */
1:
    lhu               t0, 0(a1)
    lhu               t1, 2(a1)
    addiu             a1, a1, 4
    addiu             a2, a2, -2

    sll               t6, t0, 16
    or                t6, t6, t1          /* t6 = R1 G1 B1 | R2 G2 B2 */
    lui               t3, 0xff00
    ori               t3, t3, 0xff00      /* t3 = FF 00 | FF 00 (in place) */
    shrl.ph           t7, t6, 11          /* t7 = 0 R1 | 0 R2 */
    and               t8, t6, t4          /* t8 = 0 G1 0 | 0 G2 0 */
    shra.ph           t9, t7, 2           /* t9 = 0 R1 | 0 R2   (lower) */
    shll.ph           t7, t7, 3           /* t7 = 0 R1 | 0 R2   (higher) */
    shll.ph           t8, t8, 5           /* t8 = G1 0 | G2 0   (higher) */
    or                t7, t7, t9          /* t7 = 0 R1 | 0 R2   (in place) */
    shrl.qb           t9, t8, 6           /* t9 = G1 0 | G2 0   (lower) */
    or                t3, t3, t7          /* t3 = FF R1 | FF R2 (in place) */
    or                t8, t8, t9          /* t8 = G1 0 | G2 0   (in place) */
    and               t6, t6, t5          /* t6 = 0 B1 | 0 B2 */
    shll.ph           t7, t6, 3           /* t7 = 0 B1 | 0 B2   (higher) */
    shra.ph           t9, t6, 2           /* t9 = 0 B1 | 0 B2   (lower) */
    or                t7, t7, t9          /* t7 = 0 B1 | 0 B2   (in place) */
    or                t8, t7, t8          /* t8 = G1 B1 | G2 B2 (in place) */
    precrq.ph.w       t2, t3, t8          /* t2 = FF R1 G1 B1   (in place) */
    precr_sra.ph.w    t3, t8, 0           /* t3 = FF R2 G2 B2   (in place) */

    sw                t2, 0(a0)
    sw                t3, 4(a0)

    addiu             t2, a2, -1
    bgtz              t2, 1b
     addiu            a0, a0, 8
2:
    beqz              a2, 3f
     nop
    lhu               t0, 0(a1)

/* Remaining pixel conversion (rgb565 -> rgb8888) */
    lui               t1, 0xff00
    sll               t2, t0, 0x3
    andi              t3, t2, 0xff
    ext               t2, t0, 0x2, 0x3
    or                t2, t3, t2
    or                t1, t1, t2

    sll               t2, t0, 0x5
    andi              t2, t2, 0xfc00
    srl               t3, t0, 0x1
    andi              t3, t3, 0x300
    or                t3, t2, t3
    or                t1, t1, t3

    andi              t2, t0, 0xf800
    srl               t3, t2, 0x5
    andi              t3, t3, 0xff00
    or                t2, t2, t3
    sll               t2, t2, 0x8
    or                t1, t1, t2

    sw                t1, 0(a0)
3:
    j                 ra
     nop

END(qConvertRgb16To32_asm_mips_dspr2)


#if defined(__MIPSEL) && __MIPSEL
# define PACK(r, s, t)  packrl.ph r, s, t
# define LDHI(r, o, b)  lwl r, o + 1 (b)
# define LDLO(r, o, b)  lwr r, o + 2 (b)
#else
# define PACK(r, s, t)  packrl.ph r, t, s
# define LDHI(r, o, b)  lwr r, o + 1 (b)
# define LDLO(r, o, b)  lwl r, o + 2 (b)
#endif


LEAF_MIPS_DSPR2(qt_blend_rgb16_on_rgb16_mips_dspr2_asm)
/*
+ * a0 - dst (*r5g6b5)
 * a1 - src (const *r5g6b5)
 * a2 - len (unsigned int) - batch length
 * a3 - alpha (int)
 *
 * Register usage:
 *  t0-3 - Scratch registers
 *  t4   - Number of iterations to do in unrolled loops
 *  t5   - Inverse alpha
 *  t6   - Alpha >> 2
 *  t7   - Inverse alpha >> 2
 *  t8   - magic1 (0x07e007e0)
 *  t9   - magic2 (0xf81ff81f)
 *
 * NOTE:
 *   Cannot use DSP instructions for the multiplication of two
 *   16-bit values: overflow would be always rounded or saturated.
 */

    beqz  a2, 0f
     andi t0, a0, 0x3
    andi  t1, a1, 0x3
    /* Adjust alpha value, and calculate inverse alpha value */
    li    t5, 255
    or    t2, t0, t1      /* t0 = (dst & 0x3) | (src & 0x3) */
    sll   t8, a3, 8
    subu  a3, t8, a3
    li    t8, 0x07e007e0  /* magic1 */
    srl   a3, a3, 8       /* alpha >>= 8  */
    li    t9, 0xf81ff81f  /* magic2 */
    subu  t5, t5, a3      /* ialpha = 255 - alpha */
    addiu a3, a3, 1       /* alpha++ */
    addiu t5, t5, 1       /* ialpha++ */
    srl   t6, a3, 2       /* ashift = alpha >> 2 */

    beqz  t2, 4f /* both aligned */
     srl  t7, t5, 2       /* iashift = ialpha >> 2 */

    beqz  t1, 2f /* src aligned, dst unaligned */
     nop

    beqz  t0, 3f /* dst aligned, src unaligned */
     nop

    /*
     * Both src/dst are unaligned: read 1 halfword from each, then
     * fall-off to continue with word-aligned operation.
     */
    lhu t1, 0 (a1)
    lhu t0, 0 (a0)
    addiu a2, a2, -1  /* len-- */
    andi  t2, t1, 0x07e0
    andi  t1, t1, 0xf81f
    mul   t2, t2, a3
    mul   t1, t1, t6
    andi  t3, t0, 0x07e0
    andi  t0, t0, 0xf81f
    mul   t3, t3, t5
    mul   t0, t0, t7
    addiu a1, a1,  2  /* src++ */
    srl   t2, t2, 8
    srl   t1, t1, 6
    andi  t2, t2, 0x07e0
    andi  t1, t1, 0xf81f
    or    t1, t1, t2
    srl   t3, t3, 8
    srl   t0, t0, 6
    andi  t3, t3, 0x07e0
    andi  t0, t0, 0xf81f
    or    t0, t0, t3
    addu  t0, t0, t1  /* src * alpha + dst * ialpha */
    sh    t0,  0 (a0)
    addiu a0, a0,  2  /* dst++ */

    /*
     * Both src/dst pointers are word-aligned, process eight
     * items at a time in an unrolled loop.
     */
4:  beqz   a2, 0f
     srl   t4, a2, 3   /* t4 = len / 8 */
    beqz   t4, 5f
     andi  a2, a2, 0x7 /* len = len % 8 */
    SAVE_REGS_ON_STACK 12, s0, s1, s2, s3, s4, v0, v1

1:  lw  t1,  0 (a1)  /* [s0, s1] */
    lw  v1,  4 (a1)  /* [s2, s3] */
    lw  s1,  8 (a1)  /* [s4, s5] */
    lw  s3, 12 (a1)  /* [s6, s7] */

    lw  t0,  0 (a0)  /* [d0, d1] */
    lw  v0,  4 (a0)  /* [d2, d3] */
    lw  s0,  8 (a0)  /* [d4, d5] */
    lw  s2, 12 (a0)  /* [d6, d7] */

    pref 4, 16 (a1)
    pref 5, 16 (a0)

    and     t2, t1, t8
    ext     t3, t2, 0, 16
    srl     t2, t2, 16
    mul     t3, t3, a3
    mul     t2, t2, a3
    and     t1, t1, t9
    ext     s4, t1, 0, 16
    mul     s4, s4, t6
    srl     t1, t1, 16
    mul     t1, t1, t6
    srl     t3, t3, 8
    srl     t2, t2, 8
    append  t2, t3, 16
    and     t2, t2, t8
    srl     s4, s4, 6
    and     t3, v1, t8
    srl     t1, t1, 6
    append  t1, s4, 16
    and     t1, t1, t9
    or      t1, t1, t2
    ext     t2, t3, 0, 16
    srl     t3, t3, 16
    mul     t2, t2, a3
    mul     t3, t3, a3
    and     v1, v1, t9
    ext     s4, v1, 0, 16
    mul     s4, s4, t6
    srl     v1, v1, 16
    mul     v1, v1, t6
    srl     t2, t2, 8
    srl     t3, t3, 8
    append  t3, t2, 16
    and     t3, t3, t8
    srl     s4, s4, 6
    and     t2, s1, t8
    srl     v1, v1, 6
    append  v1, s4, 16
    and     v1, v1, t9
    or      v1, v1, t3
    ext     t3, t2, 0, 16
    srl     t2, t2, 16
    mul     t3, t3, a3
    mul     t2, t2, a3
    and     s1, s1, t9
    ext     s4, s1, 0, 16
    mul     s4, s4, t6
    srl     s1, s1, 16
    mul     s1, s1, t6
    srl     t3, t3, 8
    srl     t2, t2, 8
    append  t2, t3, 16
    and     t2, t2, t8
    srl     s4, s4, 6
    and     t3, s3, t8
    srl     s1, s1, 6
    append  s1, s4, 16
    and     s1, s1, t9
    or      s1, s1, t2
    ext     t2, t3, 0, 16
    srl     t3, t3, 16
    mul     t2, t2, a3
    mul     t3, t3, a3
    and     s3, s3, t9
    ext     s4, s3, 0, 16
    mul     s4, s4, t6
    srl     s3, s3, 16
    mul     s3, s3, t6
    srl     t2, t2, 8
    srl     t3, t3, 8
    append  t3, t2, 16
    and     t3, t3, t8
    srl     s4, s4, 6
    and     t2, t0, t8
    srl     s3, s3, 6
    append  s3, s4, 16
    and     s3, s3, t9
    or      s3, s3, t3
    ext     t3, t2, 0, 16
    srl     t2, t2, 16
    mul     t3, t3, t5
    mul     t2, t2, t5
    and     t0, t0, t9
    ext     s4, t0, 0, 16
    mul     s4, s4, t7
    srl     t0, t0, 16
    mul     t0, t0, t7
    srl     t3, t3, 8
    srl     t2, t2, 8
    append  t2, t3, 16
    and     t2, t2, t8
    srl     s4, s4, 6
    and     t3, v0, t8
    srl     t0, t0, 6
    append  t0, s4, 16
    and     t0, t0, t9
    or      t0, t0, t2
    ext     t2, t3, 0, 16
    srl     t3, t3, 16
    mul     t2, t2, t5
    mul     t3, t3, t5
    and     v0, v0, t9
    ext     s4, v0, 0, 16
    mul     s4, s4, t7
    srl     v0, v0, 16
    mul     v0, v0, t7
    srl     t2, t2, 8
    srl     t3, t3, 8
    append  t3, t2, 16
    and     t3, t3, t8
    srl     s4, s4, 6
    and     t2, s0, t8
    srl     v0, v0, 6
    append  v0, s4, 16
    and     v0, v0, t9
    or      v0, v0, t3
    ext     t3, t2, 0, 16
    srl     t2, t2, 16
    mul     t3, t3, t5
    mul     t2, t2, t5
    and     s0, s0, t9
    ext     s4, s0, 0, 16
    mul     s4, s4, t7
    srl     s0, s0, 16
    mul     s0, s0, t7
    srl     t3, t3, 8
    srl     t2, t2, 8
    append  t2, t3, 16
    and     t2, t2, t8
    srl     s4, s4, 6
    and     t3, s2, t8
    srl     s0, s0, 6
    append  s0, s4, 16
    and     s0, s0, t9
    or      s0, s0, t2
    ext     t2, t3, 0, 16
    srl     t3, t3, 16
    mul     t2, t2, t5
    mul     t3, t3, t5
    and     s2, s2, t9
    ext     s4, s2, 0, 16
    mul     s4, s4, t7
    srl     s2, s2, 16
    mul     s2, s2, t7
    srl     t2, t2, 8
    srl     t3, t3, 8
    append  t3, t2, 16
    and     t3, t3, t8
    srl     s4, s4, 6
    addu.ph t0, t0, t1
    srl     s2, s2, 6
    append  s2, s4, 16
    and     s2, s2, t9
    or      s2, s2, t3
    addu.ph v0, v0, v1  /* v0 = [S2 + D2, S3 + D3] */
    addu.ph s0, s0, s1  /* s0 = [S4 + D4, S5 + D5] */
    addu.ph s2, s2, s3  /* s2 = [S6 + D6, S7 + D7] */

    sw      t0,  0 (a0) /* [SS0, SS1] */
    sw      v0,  4 (a0) /* [SS2, SS3] */
    sw      s0,  8 (a0) /* [SS4, SS5] */
    sw      s2, 12 (a0) /* [SS6, SS7] */

    addiu   t4, t4, -1   /* t4-- */
    addiu   a1, a1, 16   /* src += 8 */

    bnez    t4, 1b
     addiu  a0, a0, 16   /* dst += 8 */

    RESTORE_REGS_FROM_STACK 12, s0, s1, s2, s3, s4, v0, v1
    b 5f
     nop


    /* dst unaligned: do one item and fall down to the src unaligned case */
2:  lhu t1, 0 (a1)
    lhu t0, 0 (a0)
    addiu a2, a2, -1  /* len-- */
    andi  t2, t1, 0x07e0
    andi  t1, t1, 0xf81f
    mul   t2, t2, a3
    mul   t1, t1, t6
    andi  t3, t0, 0x07e0
    andi  t0, t0, 0xf81f
    mul   t3, t3, t5
    mul   t0, t0, t7
    addiu a1, a1,  2  /* src++ */
    srl   t2, t2, 8
    srl   t1, t1, 6
    andi  t2, t2, 0x07e0
    andi  t1, t1, 0xf81f
    or    t1, t1, t2
    srl   t3, t3, 8
    srl   t0, t0, 6
    andi  t3, t3, 0x07e0
    andi  t0, t0, 0xf81f
    or    t0, t0, t3
    addu  t0, t0, t1  /* src * alpha + dst * ialpha */
    sh    t0,  0 (a0)
    addiu a0, a0,  2  /* dst++ */

    /* src unaligned */
3:  beqz   a2, 0f
     srl   t4, a2, 3   /* t4 = len / 8 */
    beqz   t4, 5f
     andi  a2, a2, 0x7 /* len = len % 8 */
    SAVE_REGS_ON_STACK 12, s0, s1, s2, s3, s4, v0, v1

1:  lw     t0,  0 (a0) /* [d0, d1] */
    lw     v0,  4 (a0) /* [d2, d3] */
    lw     s0,  8 (a0) /* [d4, d5] */
    lw     s2, 12 (a0) /* [d6, d7] */

    LDHI  (t1,  0, a1) /* [s0, __] */
    lw     v1,  2 (a1) /* [s1, s2] */
    lw     s1,  6 (a1) /* [s3, s4] */
    lw     s3, 10 (a1) /* [s5, s6] */
    LDLO  (s4, 12, a1) /* [__, s7] */

    pref    4, 14 (a1)
    pref    5, 16 (a0)

    PACK  (t1, v1, t1) /* [s0, s1] */
    PACK  (v1, s1, v1) /* [s2, s3] */
    PACK  (s1, s3, s1) /* [s4, s5] */
    PACK  (s3, s4, s3) /* [s6, s7] */

    and     t2, t1, t8
    ext     t3, t2, 0, 16
    srl     t2, t2, 16
    mul     t3, t3, a3
    mul     t2, t2, a3
    and     t1, t1, t9
    ext     s4, t1, 0, 16
    mul     s4, s4, t6
    srl     t1, t1, 16
    mul     t1, t1, t6
    srl     t3, t3, 8
    srl     t2, t2, 8
    append  t2, t3, 16
    and     t2, t2, t8
    srl     s4, s4, 6
    and     t3, v1, t8
    srl     t1, t1, 6
    append  t1, s4, 16
    and     t1, t1, t9
    or      t1, t1, t2
    ext     t2, t3, 0, 16
    srl     t3, t3, 16
    mul     t2, t2, a3
    mul     t3, t3, a3
    and     v1, v1, t9
    ext     s4, v1, 0, 16
    mul     s4, s4, t6
    srl     v1, v1, 16
    mul     v1, v1, t6
    srl     t2, t2, 8
    srl     t3, t3, 8
    append  t3, t2, 16
    and     t3, t3, t8
    srl     s4, s4, 6
    and     t2, s1, t8
    srl     v1, v1, 6
    append  v1, s4, 16
    and     v1, v1, t9
    or      v1, v1, t3
    ext     t3, t2, 0, 16
    srl     t2, t2, 16
    mul     t3, t3, a3
    mul     t2, t2, a3
    and     s1, s1, t9
    ext     s4, s1, 0, 16
    mul     s4, s4, t6
    srl     s1, s1, 16
    mul     s1, s1, t6
    srl     t3, t3, 8
    srl     t2, t2, 8
    append  t2, t3, 16
    and     t2, t2, t8
    srl     s4, s4, 6
    and     t3, s3, t8
    srl     s1, s1, 6
    append  s1, s4, 16
    and     s1, s1, t9
    or      s1, s1, t2
    ext     t2, t3, 0, 16
    srl     t3, t3, 16
    mul     t2, t2, a3
    mul     t3, t3, a3
    and     s3, s3, t9
    ext     s4, s3, 0, 16
    mul     s4, s4, t6
    srl     s3, s3, 16
    mul     s3, s3, t6
    srl     t2, t2, 8
    srl     t3, t3, 8
    append  t3, t2, 16
    and     t3, t3, t8
    srl     s4, s4, 6
    and     t2, t0, t8
    srl     s3, s3, 6
    append  s3, s4, 16
    and     s3, s3, t9
    or      s3, s3, t3
    ext     t3, t2, 0, 16
    srl     t2, t2, 16
    mul     t3, t3, t5
    mul     t2, t2, t5
    and     t0, t0, t9
    ext     s4, t0, 0, 16
    mul     s4, s4, t7
    srl     t0, t0, 16
    mul     t0, t0, t7
    srl     t3, t3, 8
    srl     t2, t2, 8
    append  t2, t3, 16
    and     t2, t2, t8
    srl     s4, s4, 6
    and     t3, v0, t8
    srl     t0, t0, 6
    append  t0, s4, 16
    and     t0, t0, t9
    or      t0, t0, t2
    ext     t2, t3, 0, 16
    srl     t3, t3, 16
    mul     t2, t2, t5
    mul     t3, t3, t5
    and     v0, v0, t9
    ext     s4, v0, 0, 16
    mul     s4, s4, t7
    srl     v0, v0, 16
    mul     v0, v0, t7
    srl     t2, t2, 8
    srl     t3, t3, 8
    append  t3, t2, 16
    and     t3, t3, t8
    srl     s4, s4, 6
    and     t2, s0, t8
    srl     v0, v0, 6
    append  v0, s4, 16
    and     v0, v0, t9
    or      v0, v0, t3
    ext     t3, t2, 0, 16
    srl     t2, t2, 16
    mul     t3, t3, t5
    mul     t2, t2, t5
    and     s0, s0, t9
    ext     s4, s0, 0, 16
    mul     s4, s4, t7
    srl     s0, s0, 16
    mul     s0, s0, t7
    srl     t3, t3, 8
    srl     t2, t2, 8
    append  t2, t3, 16
    and     t2, t2, t8
    srl     s4, s4, 6
    and     t3, s2, t8
    srl     s0, s0, 6
    append  s0, s4, 16
    and     s0, s0, t9
    or      s0, s0, t2
    ext     t2, t3, 0, 16
    srl     t3, t3, 16
    mul     t2, t2, t5
    mul     t3, t3, t5
    and     s2, s2, t9
    ext     s4, s2, 0, 16
    mul     s4, s4, t7
    srl     s2, s2, 16
    mul     s2, s2, t7
    srl     t2, t2, 8
    srl     t3, t3, 8
    append  t3, t2, 16
    and     t3, t3, t8
    srl     s4, s4, 6
    addu.ph t0, t0, t1
    srl     s2, s2, 6
    append  s2, s4, 16
    and     s2, s2, t9
    or      s2, s2, t3
    addu.ph v0, v0, v1  /* v0 = [S2 + D2, S3 + D3] */
    addu.ph s0, s0, s1  /* s0 = [S4 + D4, S5 + D5] */
    addu.ph s2, s2, s3  /* s2 = [S6 + D6, S7 + D7] */

    sw      t0,  0 (a0) /* [SS0, SS1] */
    sw      v0,  4 (a0) /* [SS2, SS3] */
    sw      s0,  8 (a0) /* [SS4, SS5] */
    sw      s2, 12 (a0) /* [SS6, SS7] */

    addiu   t4, t4, -1   /* t4-- */
    addiu   a1, a1, 16   /* src += 8 */

    bnez    t4, 1b
     addiu  a0, a0, 16   /* dst += 8 */

    RESTORE_REGS_FROM_STACK 12, s0, s1, s2, s3, s4, v0, v1

5:  /* Process remaining items (len < 8), one at a time */
    beqz  a2, 0f
     nop

1:  lhu    t1, 0 (a1)
    lhu    t0, 0 (a0)
    addiu  a1, a1, 2   /* src++ */
    andi  t2, t1, 0x07e0
    andi  t1, t1, 0xf81f
    mul   t2, t2, a3
    mul   t1, t1, t6
    andi  t3, t0, 0x07e0
    andi  t0, t0, 0xf81f
    mul   t3, t3, t5
    mul   t0, t0, t7
    addiu a2, a2,  -1   /* len-- */
    srl   t2, t2, 8
    srl   t1, t1, 6
    andi  t2, t2, 0x07e0
    andi  t1, t1, 0xf81f
    or    t1, t1, t2
    srl   t3, t3, 8
    srl   t0, t0, 6
    andi  t3, t3, 0x07e0
    andi  t0, t0, 0xf81f
    or    t0, t0, t3

    addu   t0, t0, t1  /* src*alpha + dst*ialpha */
    sh     t0, 0 (a0)
    bnez   a2, 1b
     addiu a0, a0, 2   /* dst++ */

0:  jr ra
     nop

END(qt_blend_rgb16_on_rgb16_mips_dspr2_asm)

#undef PACK
#undef LDHI
#undef LDLO
