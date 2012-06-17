/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdrawhelper_arm_simd_p.h"

#include <private/qpaintengine_raster_p.h>
#include <private/qblendfunctions_p.h>

#ifdef QT_HAVE_ARM_SIMD

#if defined(Q_OS_SYMBIAN)
#if !defined(__SWITCH_TO_ARM)
#ifdef __MARM_THUMB__
#ifndef __ARMCC__
#define __SWITCH_TO_ARM      asm("push {r0} ");\
                             asm("add r0, pc, #4 ");\
                             asm("bx r0 ");\
                             asm("nop ");\
                             asm(".align 2 ");\
                             asm(".code 32 ");\
                             asm("ldr r0, [sp], #4 ")
#define __END_ARM            asm(".code 16 ")
#else
#define __SWITCH_TO_ARM      asm(".code 32 ");
#define __END_ARM
#endif // __ARMCC__
#else
#define __SWITCH_TO_ARM
#define __END_ARM
#endif //__MARM_THUMB__
#endif
#endif

#if defined(Q_OS_SYMBIAN) && defined(Q_CC_RVCT)
__asm void qt_blend_argb32_on_argb32_arm_simd(uchar *destPixels, int dbpl,
                                        const uchar *srcPixels, int sbpl,
                                        int w, int h,
                                        int const_alpha)
{
#ifndef __ARMCC__
    __SWITCH_TO_ARM;
#else
    CODE32
#endif // __ARMCC__

    stmfd   sp!, {r4-r12, r14}

    // read arguments off the stack
    add     r8, sp, #10 * 4
    ldmia   r8, {r4-r6}

    // adjust dbpl and sbpl
    mov     r14, #4
    mul     r14, r4, r14
    sub     r1, r1, r14
    sub     r3, r3, r14

    // load 0xFF00FF00 to r12
    mov     r12, #0xFF000000
    add     r12, r12, #0xFF00

    // load 0x800080 to r14
    mov     r14, #0x800000
    add     r14, r14, #0x80

    /*
      Registers:
       r0 dst
       r1 dbpl
       r2 src
       r3 sbpl
       r4 w
       r5 h
       r6 const_alpha
       r12 0xFF0000
       r14 0x800080
    */

    cmp     r6, #256 //test if we have fully opaque constant alpha value
    bne     argb32constalpha // branch if not

argb32_next_row

    mov     r7, r4

argb32_next_pixel

    ldr     r8, [r2], #4 // load src pixel

    // Negate r8 and extract src alpha
    mvn     r11, r8 // bitwise not
    uxtb    r11, r11, ror #24

    cmp     r11, #0 // test for full src opacity (negated)
    beq     argb32_no_blend

    cmp     r11, #255 // test for full src transparency (negated)
    addeq   r0, #4
    beq     argb32_nop

    ldr     r9, [r0] // load dst pixel

    // blend
    uxtb16  r10, r9
    uxtb16  r6, r9, ror #8
    mla     r10, r11, r10, r14
    mla     r9, r6, r11, r14
    uxtab16 r10, r10, r10, ror #8
    uxtab16 r9, r9, r9, ror #8
    and     r9, r9, r12
    uxtab16 r10, r9, r10, ror #8

    uqadd8  r8, r10, r8

argb32_no_blend

    str     r8, [r0], #4

argb32_nop

    subs    r7, r7, #1
    bgt     argb32_next_pixel

    add     r0, r0, r1 // dest = dest + dbpl
    add     r2, r2, r3 // src = src + sbpl

    subs    r5, r5, #1
    bgt     argb32_next_row

    b       argb32_blend_exit

argb32constalpha

    cmp     r6, #0
    beq     argb32_blend_exit

    ; const_alpha = (const_alpha * 255) >> 8;
    mov     r11, #255
    mul     r6, r6, r11
    mov     r11, r6, lsr #8

argb32constalpha_next_row

    mov     r7, r4

argb32constalpha_next_pixel

    ldr     r9, [r2], #4 // load src pixel

    // blend
    uxtb16  r10, r9
    uxtb16  r6, r9, ror #8
    mla     r10, r11, r10, r14
    mla     r9, r6, r11, r14
    uxtab16 r10, r10, r10, ror #8
    uxtab16 r9, r9, r9, ror #8
    and     r9, r9, r12
    uxtab16 r8, r9, r10, ror #8

    ldr     r9, [r0] // load dst pixel

    // blend
    uxtb16  r10, r9
    uxtb16  r6, r9, ror #8

    // Negate r8 and extract src alpha
    mvn     r9, r8 // bitwise not
    uxtb    r9, r9, ror #24

    mla     r10, r9, r10, r14
    mla     r9, r6, r9, r14
    uxtab16 r10, r10, r10, ror #8
    uxtab16 r9, r9, r9, ror #8
    and     r9, r9, r12
    uxtab16 r10, r9, r10, ror #8

    uqadd8  r8, r10, r8

    str     r8, [r0], #4

    subs    r7, r7, #1
    bgt     argb32constalpha_next_pixel

    add     r0, r0, r1 // dest = dest + dbpl
    add     r2, r2, r3 // src = src + sbpl

    subs    r5, r5, #1
    bgt     argb32constalpha_next_row

argb32_blend_exit

    // Restore registers
    ldmfd   sp!, {r4-r12, lr}
    bx      lr

    __END_ARM
}

void qt_blend_rgb32_on_rgb32_arm_simd(uchar *destPixels, int dbpl,
                             const uchar *srcPixels, int sbpl,
                             int w, int h,
                             int const_alpha)
{
    if (const_alpha != 256) {
        qt_blend_argb32_on_argb32_arm_simd(destPixels, dbpl, srcPixels, sbpl, w, h, const_alpha);
        return;
    }

    const uint *src = (const uint *) srcPixels;
    uint *dst = (uint *) destPixels;
    if (w <= 64) {
        for (int y=0; y<h; ++y) {
            qt_memconvert(dst, src, w);
            dst = (quint32 *)(((uchar *) dst) + dbpl);
            src = (const quint32 *)(((const uchar *) src) + sbpl);
        }
    } else {
        int len = w * 4;
        for (int y=0; y<h; ++y) {
            memcpy(dst, src, len);
            dst = (quint32 *)(((uchar *) dst) + dbpl);
            src = (const quint32 *)(((const uchar *) src) + sbpl);
        }
    }
}

#else // defined(Q_OS_SYMBIAN) && defined(Q_CC_RVCT)

// TODO: add GNU assembler instructions and support for other platforms.
//       Default to C code for now

void qt_blend_argb32_on_argb32_arm_simd(uchar *destPixels, int dbpl,
                                        const uchar *srcPixels, int sbpl,
                                        int w, int h,
                                        int const_alpha)
{
    const uint *src = (const uint *) srcPixels;
    uint *dst = (uint *) destPixels;
    if (const_alpha == 256) {
        for (int y=0; y<h; ++y) {
            for (int x=0; x<w; ++x) {
                uint s = src[x];
                if (s >= 0xff000000)
                    dst[x] = s;
                else if (s != 0)
                    dst[x] = s + BYTE_MUL(dst[x], qAlpha(~s));
            }
            dst = (quint32 *)(((uchar *) dst) + dbpl);
            src = (const quint32 *)(((const uchar *) src) + sbpl);
        }
    } else if (const_alpha != 0) {
        const_alpha = (const_alpha * 255) >> 8;
        for (int y=0; y<h; ++y) {
            for (int x=0; x<w; ++x) {
                uint s = BYTE_MUL(src[x], const_alpha);
                dst[x] = s + BYTE_MUL(dst[x], qAlpha(~s));
            }
            dst = (quint32 *)(((uchar *) dst) + dbpl);
            src = (const quint32 *)(((const uchar *) src) + sbpl);
        }
    }
}

void qt_blend_rgb32_on_rgb32_arm_simd(uchar *destPixels, int dbpl,
                             const uchar *srcPixels, int sbpl,
                             int w, int h,
                             int const_alpha)
{
    if (const_alpha != 256) {
        qt_blend_argb32_on_argb32_arm_simd(destPixels, dbpl, srcPixels, sbpl, w, h, const_alpha);
        return;
    }

    const uint *src = (const uint *) srcPixels;
    uint *dst = (uint *) destPixels;
    if (w <= 64) {
        for (int y=0; y<h; ++y) {
            qt_memconvert(dst, src, w);
            dst = (quint32 *)(((uchar *) dst) + dbpl);
            src = (const quint32 *)(((const uchar *) src) + sbpl);
        }
    } else {
        int len = w * 4;
        for (int y=0; y<h; ++y) {
            memcpy(dst, src, len);
            dst = (quint32 *)(((uchar *) dst) + dbpl);
            src = (const quint32 *)(((const uchar *) src) + sbpl);
        }
    }
}

#endif

#endif // QT_HAVE_ARMV_SIMD
