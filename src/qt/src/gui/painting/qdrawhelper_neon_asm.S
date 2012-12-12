/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

/* Prevent the stack from becoming executable for no reason... */
#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif

.text
.fpu neon
.arch armv7a
.altmacro

/* void blend_8_pixels_argb32_on_rgb16_neon(quint16 *dst, const quint32 *src, int const_alpha) */

    .func blend_8_pixels_argb32_on_rgb16_neon
    .global blend_8_pixels_argb32_on_rgb16_neon
    /* For ELF format also set function visibility to hidden */
#ifdef __ELF__
    .hidden blend_8_pixels_argb32_on_rgb16_neon
    .type blend_8_pixels_argb32_on_rgb16_neon, %function
#endif
blend_8_pixels_argb32_on_rgb16_neon:
    vld4.8      { d0, d1, d2, d3 }, [r1]
    vld1.16     { d4, d5 }, [r0]

    cmp         r2, #256
    beq         .blend_32_inner

    vdup.8      d6, r2

    /* multiply by const_alpha */
    vmull.u8    q8,   d6, d0
    vmull.u8    q9,   d6, d1
    vmull.u8    q10,  d6, d2
    vmull.u8    q11,  d6, d3

    vshrn.u16   d0,  q8, #8
    vshrn.u16   d1,  q9, #8
    vshrn.u16   d2, q10, #8
    vshrn.u16   d3, q11, #8

.blend_32_inner:
    /* convert 8 r5g6b5 pixel data from {d4, d5} to planar 8-bit format
       and put data into d6 - red, d7 - green, d30 - blue */
    vshrn.u16   d6, q2, #8
    vshrn.u16   d7, q2, #3
    vsli.u16    q2, q2, #5
    vsri.u8     d6, d6, #5
    vmvn.8      d3, d3
    vsri.u8     d7, d7, #6
    vshrn.u16   d30, q2, #2

    pld [r0, #128]

    /* now do alpha blending, storing results in 8-bit planar format
       into d16 - red, d19 - green, d18 - blue */
    vmull.u8    q10, d3, d6
    vmull.u8    q11, d3, d7
    vmull.u8    q12, d3, d30
    vrshr.u16   q13, q10, #8
    vrshr.u16   q3,  q11, #8
    vrshr.u16   q15, q12, #8
    vraddhn.u16 d20, q10, q13
    vraddhn.u16 d23, q11, q3
    vraddhn.u16 d22, q12, q15
    vqadd.u8    d16, d2, d20
    vqadd.u8    q9, q0, q11
    /* convert the result to r5g6b5 and store it into {d28, d29} */
    vshll.u8    q14, d16, #8
    vshll.u8    q8, d19, #8
    vshll.u8    q9, d18, #8
    vsri.u16    q14, q8, #5
    vsri.u16    q14, q9, #11

    vst1.16     { d28, d29 }, [r0]

    bx          lr

    .endfunc

/* void blend_8_pixels_rgb16_on_rgb16_neon(quint16 *dst, const quint16 *src, int const_alpha) */

    .func blend_8_pixels_rgb16_on_rgb16_neon
    .global blend_8_pixels_rgb16_on_rgb16_neon
    /* For ELF format also set function visibility to hidden */
#ifdef __ELF__
    .hidden blend_8_pixels_rgb16_on_rgb16_neon
    .type blend_8_pixels_rgb16_on_rgb16_neon, %function
#endif
blend_8_pixels_rgb16_on_rgb16_neon:
    vld1.16     { d0, d1 }, [r0]
    vld1.16     { d2, d3 }, [r1]

    rsb         r3, r2, #256
    vdup.8      d4, r2
    vdup.8      d5, r3

    /* convert 8 r5g6b5 pixel data from {d0, d1} to planar 8-bit format
       and put data into d6 - red, d7 - green, d30 - blue */
    vshrn.u16   d6,  q0,  #8
    vshrn.u16   d7,  q0,  #3
    vsli.u16    q0,  q0,  #5
    vsri.u8     d6,  d6,  #5
    vsri.u8     d7,  d7,  #6
    vshrn.u16   d30, q0,  #2

    /* same from {d2, d3} into {d26, d27, d28} */
    vshrn.u16   d26, q1,  #8
    vshrn.u16   d27, q1,  #3
    vsli.u16    q1,  q1,  #5
    vsri.u8     d26, d26, #5
    vsri.u8     d27, d27, #6
    vshrn.u16   d28, q1,  #2

    /* multiply dst by inv const_alpha */
    vmull.u8    q10, d5,  d6
    vmull.u8    q11, d5,  d7
    vmull.u8    q12, d5,  d30

    vshrn.u16   d6,  q10, #8
    vshrn.u16   d7,  q11, #8
    vshrn.u16   d30, q12, #8

    /* multiply src by const_alpha */
    vmull.u8    q10,  d4, d26
    vmull.u8    q11,  d4, d27
    vmull.u8    q12,  d4, d28

    vshrn.u16   d26, q10, #8
    vshrn.u16   d27, q11, #8
    vshrn.u16   d28, q12, #8

    /* preload dst + 128 */
    pld [r0, #128]

    /* add components, storing results in 8-bit planar format
       into d16 - red, d19 - green, d18 - blue */
    vadd.u8     d16, d26, d6
    vadd.u8     d19, d27, d7
    vadd.u8     d18, d28, d30

    /* convert the result to r5g6b5 and store it into {d28, d29} */
    vshll.u8    q14, d16, #8
    vshll.u8    q8,  d19, #8
    vshll.u8    q9,  d18, #8
    vsri.u16    q14,  q8, #5
    vsri.u16    q14,  q9, #11

    vst1.16     { d28, d29 }, [r0]

    bx          lr

    .endfunc

/* void qt_rotate90_16_neon(quint16 *dst, const quint16 *src, int sstride, int dstride, int count) */
    .func qt_rotate90_16_neon
    .global qt_rotate90_16_neon
    /* For ELF format also set function visibility to hidden */
#ifdef __ELF__
    .hidden qt_rotate90_16_neon
    .type qt_rotate90_16_neon, %function
#endif
qt_rotate90_16_neon:
    push { r4-r11, lr }
    ldr r5, [sp, #(9*4)]

    /* The preloads are the key to getting good performance */
    pld [r1]

    mov r4, r5, asr #2
    add r6, r0, r3
    add r7, r6, r3

    add r8, r7, r3
    add r9, r8, r3

    pld [r1, r2]

    add r10, r9, r3
    add r11, r10, r3

    add r3, r3, r11
    and r5, r5, #3

    pld [r1, r2, lsl #1]

    cmp r4, #0
    beq .rotate90_16_tail

.rotate90_16_loop:
    vld1.16 { q8  }, [r1], r2

    pld [r1, r2, lsl #1]

    vld1.16 { q9  }, [r1], r2
    vld1.16 { q10 }, [r1], r2
    vld1.16 { q11 }, [r1], r2

    pld [r1]

    /* Could have used four quad-word zips instead,
       but those take three cycles as opposed to one. */
    vzip.16 d16, d20
    vzip.16 d17, d21

    vzip.16 d18, d22

    pld [r1, r2]

    vzip.16 d19, d23

    vzip.16 d16, d18
    vzip.16 d17, d19

    pld [r1, r2, lsl #1]

    vzip.16 d20, d22
    vzip.16 d21, d23

    vst1.16 { d23 }, [r0]!
    vst1.16 { d21 }, [r6]!
    vst1.16 { d19 }, [r7]!
    vst1.16 { d17 }, [r8]!
    vst1.16 { d22 }, [r9]!
    vst1.16 { d20 }, [r10]!
    vst1.16 { d18 }, [r11]!
    vst1.16 { d16 }, [r3]!

    sub r4, r4, #1
    cmp r4, #0
    bne .rotate90_16_loop
    b .rotate90_16_tail

.rotate90_16_tail_loop:
    sub r5, r5, #2

    vld1.16 { q8 }, [r1], r2
    vld1.16 { q9 }, [r1], r2

    vzip.16 d16, d18
    vzip.16 d17, d19

    vst1.32 { d19[1] }, [r0]!
    vst1.32 { d19[0] }, [r6]!
    vst1.32 { d17[1] }, [r7]!
    vst1.32 { d17[0] }, [r8]!
    vst1.32 { d18[1] }, [r9]!
    vst1.32 { d18[0] }, [r10]!
    vst1.32 { d16[1] }, [r11]!
    vst1.32 { d16[0] }, [r3]!

.rotate90_16_tail:
    cmp r5, #0
    bgt .rotate90_16_tail_loop

    pop { r4-r11, pc }

    .endfunc
