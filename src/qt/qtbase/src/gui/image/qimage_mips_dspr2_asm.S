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

#include "../painting/qt_mips_asm_dsp_p.h"

LEAF_MIPS_DSPR2(premultiply_argb_inplace_mips_asm)

    SAVE_REGS_ON_STACK  0, s0, s1, s2, s3

3:  srl    v1, a2, 3    /* t1 = linelen / 8 */
    addiu  a1, a1, -1   /* numlines-- */
    beqz   v1, 1f       /* if (!(linelen / 8)): tail */
     andi  v0, a2, 0x7  /* v0 = linelen % 8 */
    pref    5,  0 (a0)  /* cache-hint: store-streamed */

    /* unrolled loop, handles (v1 = len / 8) batches of 8 pixels */
2:  addiu  v1, v1, -1
    pref    5, 0(a0)
    pref    5, 32(a0)

    lw     t0, 0(a0)
    lw     t1, 4(a0)
    lw     t2, 8(a0)
    lw     t3, 12(a0)
    srl    t4, t0, 24  /*  00|00|00|A1 */
    replv.ph t5, t4    /*  00|A1|00|A1 */
    srl    t6, t1, 24  /*  00|00|00|A2 */
    replv.ph t7, t6    /*  00|A2|00|A2 */
    muleu_s.ph.qbl t8, t0, t5 /* A1*A1|A1*R1 */
    muleu_s.ph.qbr t0, t0, t5 /* A1*G1|A1*B1 */
    muleu_s.ph.qbl t9, t1, t7 /* A2*A2|A2*R2 */
    muleu_s.ph.qbr t1, t1, t7 /* A2*G2|A2*B2 */
    srl    t5, t2, 24  /*  00|00|00|A3 */
    replv.ph s0, t5    /*  00|A3|00|A3 */
    srl    t7, t3, 24  /*  00|00|00|A4 */
    replv.ph s1, t7    /*  00|A4|00|A4 */
    muleu_s.ph.qbl s2, t2, s0 /* A3*A3|A3*R3 */
    muleu_s.ph.qbr t2, t2, s0 /* A3*G3|A3*B3 */
    muleu_s.ph.qbl s0, t3, s1 /* A4*A4|A4*R4 */
    muleu_s.ph.qbr t3, t3, s1 /* A4*G4|A4*B4 */
    preceu.ph.qbla s1, t8
    preceu.ph.qbla s3, t0
    addu.ph t8, t8, s1
    addu.ph t0, t0, s3
    preceu.ph.qbla s1, t9
    preceu.ph.qbla s3, t1
    addu.ph t9, t9, s1
    addu.ph t1, t1, s3
    preceu.ph.qbla s1, s2
    preceu.ph.qbla s3, t2
    addu.ph s2, s2, s1
    addu.ph t2, t2, s3
    preceu.ph.qbla s1, s0
    preceu.ph.qbla s3, t3
    addu.ph s0, s0, s1
    addu.ph t3, t3, s3
    shra_r.ph t8, t8, 8 /*  xxAA1|xxRR1 */
    shra_r.ph t0, t0, 8 /*  xxBB1|xxGG1 */
    shra_r.ph t9, t9, 8
    shra_r.ph t1, t1, 8
    shra_r.ph s2, s2, 8
    shra_r.ph t2, t2, 8
    shra_r.ph s0, s0, 8
    shra_r.ph t3, t3, 8
    precr.qb.ph t0, t8, t0
    precr.qb.ph t1, t9, t1
    precr.qb.ph t2, s2, t2
    precr.qb.ph t3, s0, t3
    append t4, t0, 24
    append t6, t1, 24
    append t5, t2, 24
    append t7, t3, 24
    sw    t4, 0(a0)
    sw    t6, 4(a0)
    sw    t5, 8(a0)
    sw    t7, 12(a0)

    lw     t0, 16(a0)
    lw     t1, 20(a0)
    lw     t2, 24(a0)
    lw     t3, 28(a0)
    srl    t4, t0, 24  /*  00|00|00|A1 */
    replv.ph t5, t4    /*  00|A1|00|A1 */
    srl    t6, t1, 24  /*  00|00|00|A2 */
    replv.ph t7, t6    /*  00|A2|00|A2 */
    muleu_s.ph.qbl t8, t0, t5 /* A1*A1|A1*R1 */
    muleu_s.ph.qbr t0, t0, t5 /* A1*G1|A1*B1 */
    muleu_s.ph.qbl t9, t1, t7 /* A2*A2|A2*R2 */
    muleu_s.ph.qbr t1, t1, t7 /* A2*G2|A2*B2 */
    srl    t5, t2, 24  /*  00|00|00|A3 */
    replv.ph s0, t5    /*  00|A3|00|A3 */
    srl    t7, t3, 24  /*  00|00|00|A4 */
    replv.ph s1, t7    /*  00|A4|00|A4 */
    muleu_s.ph.qbl s2, t2, s0 /* A3*A3|A3*R3 */
    muleu_s.ph.qbr t2, t2, s0 /* A3*G3|A3*B3 */
    muleu_s.ph.qbl s0, t3, s1 /* A4*A4|A4*R4 */
    muleu_s.ph.qbr t3, t3, s1 /* A4*G4|A4*B4 */
    preceu.ph.qbla s1, t8
    preceu.ph.qbla s3, t0
    addu.ph t8, t8, s1
    addu.ph t0, t0, s3
    preceu.ph.qbla s1, t9
    preceu.ph.qbla s3, t1
    addu.ph t9, t9, s1
    addu.ph t1, t1, s3
    preceu.ph.qbla s1, s2
    preceu.ph.qbla s3, t2
    addu.ph s2, s2, s1
    addu.ph t2, t2, s3
    preceu.ph.qbla s1, s0
    preceu.ph.qbla s3, t3
    addu.ph s0, s0, s1
    addu.ph t3, t3, s3
    shra_r.ph t8, t8, 8 /*  xxAA1|xxRR1 */
    shra_r.ph t0, t0, 8 /*  xxBB1|xxGG1 */
    shra_r.ph t9, t9, 8
    shra_r.ph t1, t1, 8
    shra_r.ph s2, s2, 8
    shra_r.ph t2, t2, 8
    shra_r.ph s0, s0, 8
    shra_r.ph t3, t3, 8
    precr.qb.ph t0, t8, t0
    precr.qb.ph t1, t9, t1
    precr.qb.ph t2, s2, t2
    precr.qb.ph t3, s0, t3
    append t4, t0, 24
    append t6, t1, 24
    append t5, t2, 24
    append t7, t3, 24
    sw    t4, 16(a0)
    sw    t6, 20(a0)
    sw    t5, 24(a0)
    sw    t7, 28(a0)
    bgtz  v1, 2b       /* if (t1): unrolled loop */
     addiu a0, a0, 32  /* data += 8 */

    beqz   v0, 4f      /* if (!v0): skip tail loop */
     nop

    /* tail loop, handles (len < 8), one pixel at a time */
1:  lw     t1, 0 (a0)
    addiu  v0, v0, -1  /* len-- */
    srl    t2, t1, 24  /* t2 = alpha */
    replv.ph t3, t2
    muleu_s.ph.qbl t4, t1, t3
    muleu_s.ph.qbr t1, t1, t3
    preceu.ph.qbla t3, t4
    preceu.ph.qbla t5, t1
    addu.ph   t4, t4, t3
    addu.ph   t1, t1, t5
    shra_r.ph t4, t4, 8
    shra_r.ph t1, t1, 8
    precr.qb.ph t1, t4, t1
    append   t2, t1, 24
    sw     t2, 0(a0)
    bgtz   v0, 1b
     addiu a0, a0,  4  /* src++ */

4:  bnez   a1, 3b      /* if (numlines): loop */
     addu  a0, a0, a3  /* src += srclineskip */

0:  /* return */
    RESTORE_REGS_FROM_STACK  0, s0, s1, s2, s3

    jr ra
     nop

END(premultiply_argb_inplace_mips_asm)


LEAF_MIPS_DSPR2(qt_convert_rgb888_to_rgb32_mips_dspr2_asm)
/*
 * Parameters:
 *   a0 - dst *a8r8g8b8
 *   a1 - src *r8g8b8
 *   a2 - len
 *
 * R G B r  g b R G  B r g b  R G B r  g b . . .  -- input
 * -------  -------  -------  -------  -------
 * _ R G B  _ r g b  _ R G B  _ r g b  _ R G . .  -- output
 *
 * Register usage:
 *   a2 - tail (len % 4) == (len & 0x3)
 *   t0 - batches (len / 4) == (len >> 2)
 *   t1-t7, s1-s3 - temporary
 */

    srl   t0, a2, 2     /* batches = len / 4 */
    andi  a2, a2, 0x3   /* tail = len % 4 */

    beqz  t0, 5f        /* if !batches: tail */
     lui  t7, 0xff00    /* [FF 00 00 00] */
    SAVE_REGS_ON_STACK 8, s1, s2, s3, s0, v0, v1

1:  pref    4, 0 (a1)   /* hint: read-streamed */
    pref    5, 0 (a0)   /* hint: prepare-write */
    addiu  t0, t0, -1   /* batches-- */

    lbu    t1,  0 (a1)  /* [__ __ __ R1] */
    lbu    t2,  1 (a1)  /* [__ __ __ G1] */
    lbu    t3,  2 (a1)  /* [__ __ __ B1] */

    lbu    t4,  3 (a1)  /* [__ __ __ r2] */
    lbu    t5,  4 (a1)  /* [__ __ __ g2] */
    lbu    t6,  5 (a1)  /* [__ __ __ b2] */

    lbu    s1,  6 (a1)  /* [__ __ __ R3] */
    lbu    s2,  7 (a1)  /* [__ __ __ G3] */
    lbu    s3,  8 (a1)  /* [__ __ __ B3] */

    lbu    s0,  9 (a1)  /* [__ __ __ r4] */
    lbu    v0, 10 (a1)  /* [__ __ __ g4] */
    lbu    v1, 11 (a1)  /* [__ __ __ b4] */

    append t1, t2, 8    /* [__ __ R1 G1] */
    append t4, t5, 8    /* [__ __ r2 g2] */
    append s1, s2, 8    /* [__ __ R3 G3] */
    append s0, v0, 8    /* [__ __ r4 g4] */
    append t1, t3, 8    /* [__ R1 G1 B1] */
    append t4, t6, 8    /* [__ r2 g2 b2] */
    append s1, s3, 8    /* [__ R3 G4 B3] */
    append s0, v1, 8    /* [__ r4 g4 b4] */
    or     t1, t1, t7   /* [FF R1 G1 B1] */
    or     t4, t4, t7   /* [FF r2 g2 b2] */
    or     s1, s1, t7   /* [FF R3 G3 B3] */
    or     s0, s0, t7   /* [FF r4 g4 b4] */

    sw     t1,  0 (a0)
    sw     t4,  4 (a0)
    sw     s1,  8 (a0)
    sw     s0, 12 (a0)

    addiu  a1, a1, 12   /* src += 4*3 */
    bnez   t0, 1b       /* if batches: loop */
     addiu a0, a0, 16   /* dst += 4 */

    RESTORE_REGS_FROM_STACK 8, s1, s2, s3, s0, v0, v1

    /* handle remaining "tail" (a2) items */
5:  beqz   a2, 0f
     lui   t0, 0xff00   /* [FF __ __ __] */

1:  lbu    t1, 0 (a1)   /* [__ __ __ RR] */
    lbu    t2, 1 (a1)   /* [__ __ __ GG] */
    lbu    t3, 2 (a1)   /* [__ __ __ BB] */
    sll    t1, t1, 16   /* [__ RR __ __] */
    sll    t2, t2,  8   /* [__ __ GG __] */
    or     t0, t0, t1   /* [FF RR __ __] */
    or     t2, t2, t3   /* [__ __ GG BB] */
    addi   a2, a2, -1   /* len--         */
    or     t0, t0, t2   /* [FF RR GG BB] */
    addiu  a1, a1,  3   /* src += 3      */
    sw     t0, 0 (a0)
    addiu  a0, a0,  4   /* dst++         */
    bnez   a2, 1b       /* if tail: loop */
     lui   t0, 0xff00   /* [FF __ __ __] */

0:  jr ra
     nop

END(qt_convert_rgb888_to_rgb32_mips_dspr2_asm)
