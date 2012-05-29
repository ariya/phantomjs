/*
 * Copyright (C) 2011 University of Szeged
 * Copyright (C) 2011 Zoltan Herczeg
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY UNIVERSITY OF SZEGED ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL UNIVERSITY OF SZEGED OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "FEGaussianBlurNEON.h"

#if CPU(ARM_NEON) && COMPILER(GCC)

#include <wtf/Alignment.h>

namespace WebCore {

static WTF_ALIGNED(unsigned char, s_FEGaussianBlurConstantsForNeon[], 16) = {
    // Mapping from ARM to NEON registers.
    0, 16, 16, 16, 1,  16, 16, 16, 2,  16, 16, 16, 3,  16, 16, 16,
    // Mapping from NEON to ARM registers.
    0, 4,  8,  12, 16, 16, 16, 16
};

unsigned char* feGaussianBlurConstantsForNeon()
{
    return s_FEGaussianBlurConstantsForNeon;
}

#define ASSTRING(str) #str
#define TOSTRING(value) ASSTRING(value)

#define STRIDE_OFFSET TOSTRING(0)
#define STRIDE_WIDTH_OFFSET TOSTRING(4)
#define STRIDE_LINE_OFFSET TOSTRING(8)
#define STRIDE_LINE_WIDTH_OFFSET TOSTRING(12)
#define REMAINING_STRIDES_OFFSET TOSTRING(16)
#define DISTANCE_LEFT_OFFSET TOSTRING(20)
#define DISTANCE_RIGHT_OFFSET TOSTRING(24)
#define INVERTED_KERNEL_SIZE_OFFSET TOSTRING(28)
#define PAINTING_CONSTANTS_OFFSET TOSTRING(32)
#define NL "\n"

// Register allocation.
#define SOURCE_R                "r0"
#define DESTINATION_R           "r1"
#define LEFT_R                  "r2"
#define RIGHT_R                 "r3"
#define SOURCE_END_R            "r4"
#define DESTINATION_END_R       "r5"
#define STRIDE_R                "r6"
#define STRIDE_WIDTH_R          "r7"
#define STRIDE_LINE_R           "r8"
#define SOURCE_LINE_END_R       "r10"
#define DISTANCE_LEFT_R         "r11"
#define DISTANCE_RIGHT_R        "r12"
#define MAX_KERNEL_SIZE_R       "lr"

// Alternate names.
#define INIT_INVERTED_KERNEL_SIZE_R SOURCE_END_R
#define INIT_PAINTING_CONSTANTS_R DESTINATION_END_R
#define INIT_SUM_R LEFT_R
#define REMAINING_STRIDES_R SOURCE_LINE_END_R

#define INVERTED_KERNEL_SIZE_Q  "q0"
#define SUM_Q                   "q1"
#define PIXEL_Q                 "q2"
#define PIXEL_D0                "d4"
#define PIXEL_D1                "d5"
#define PIXEL_D00               "d4[0]"
#define PIXEL_D01               "d4[1]"
#define PIXEL_S1                "s9"
#define PIXEL_D10               "d5[0]"
#define PIXEL_S2                "s10"
#define PIXEL_D11               "d5[1]"
#define REMAINING_STRIDES_S0    "s12"

#define READ_RANGE              "d16-d18"
#define REMAP_ARM_NEON1_Q       "d16"
#define REMAP_ARM_NEON2_Q       "d17"
#define REMAP_NEON_ARM_Q        "d18"

asm ( // NOLINT
".globl " TOSTRING(neonDrawAllChannelGaussianBlur) NL
TOSTRING(neonDrawAllChannelGaussianBlur) ":" NL
    "stmdb sp!, {r4-r8, r10, r11, lr}" NL
    "ldr " STRIDE_R ", [r2, #" STRIDE_OFFSET "]" NL
    "ldr " STRIDE_WIDTH_R ", [r2, #" STRIDE_WIDTH_OFFSET "]" NL
    "ldr " DISTANCE_LEFT_R ", [r2, #" DISTANCE_LEFT_OFFSET "]" NL
    "ldr " DISTANCE_RIGHT_R ", [r2, #" DISTANCE_RIGHT_OFFSET "]" NL
    "ldr " STRIDE_LINE_R ", [r2, #" STRIDE_LINE_OFFSET "]" NL
    "ldr " SOURCE_LINE_END_R ", [r2, #" STRIDE_LINE_WIDTH_OFFSET "]" NL
    "ldr " INIT_INVERTED_KERNEL_SIZE_R ", [r2, #" INVERTED_KERNEL_SIZE_OFFSET "]" NL
    "ldr " INIT_PAINTING_CONSTANTS_R ", [r2, #" PAINTING_CONSTANTS_OFFSET "]" NL

    // Initialize locals.
    "mul " DISTANCE_LEFT_R ", " DISTANCE_LEFT_R ", " STRIDE_R NL
    "mul " DISTANCE_RIGHT_R ", " DISTANCE_RIGHT_R ", " STRIDE_R NL
    "mov " MAX_KERNEL_SIZE_R ", " DISTANCE_RIGHT_R NL
    "cmp " MAX_KERNEL_SIZE_R ", " STRIDE_WIDTH_R NL
    "movcs " MAX_KERNEL_SIZE_R ", " STRIDE_WIDTH_R NL
    "add " SOURCE_LINE_END_R ", " SOURCE_LINE_END_R ", " SOURCE_R NL
    "vdup.f32 " INVERTED_KERNEL_SIZE_Q ", " INIT_INVERTED_KERNEL_SIZE_R NL
    "vld1.f32 { " READ_RANGE " }, [" INIT_PAINTING_CONSTANTS_R "]!" NL

".allChannelMainLoop:" NL

    // Initialize the sum variable.
    "vmov.u32 " SUM_Q ", #0" NL
    "mov " INIT_SUM_R ", " SOURCE_R NL
    "add " SOURCE_END_R ", " SOURCE_R ", " MAX_KERNEL_SIZE_R NL
    "cmp " INIT_SUM_R ", " SOURCE_END_R NL
    "bcs .allChannelInitSumDone" NL
".allChannelInitSum:" NL
    "vld1.u32 " PIXEL_D00 ", [" INIT_SUM_R "], " STRIDE_R NL
    "vtbl.8 " PIXEL_D1 ", {" PIXEL_D0 "}, " REMAP_ARM_NEON2_Q NL
    "vtbl.8 " PIXEL_D0 ", {" PIXEL_D0 "}, " REMAP_ARM_NEON1_Q NL
    "vadd.u32 " SUM_Q ", " SUM_Q ", " PIXEL_Q NL
    "cmp " INIT_SUM_R ", " SOURCE_END_R NL
    "bcc .allChannelInitSum" NL
".allChannelInitSumDone:" NL

    // Blurring.
    "add " SOURCE_END_R ", " SOURCE_R ", " STRIDE_WIDTH_R NL
    "add " DESTINATION_END_R ", " DESTINATION_R ", " STRIDE_WIDTH_R NL
    "sub " LEFT_R ", " SOURCE_R ", " DISTANCE_LEFT_R NL
    "add " RIGHT_R ", " SOURCE_R ", " DISTANCE_RIGHT_R NL

".allChannelBlur:" NL
    "vcvt.f32.u32 " PIXEL_Q ", " SUM_Q NL
    "vmul.f32 " PIXEL_Q ", " PIXEL_Q ", " INVERTED_KERNEL_SIZE_Q NL
    "vcvt.u32.f32 " PIXEL_Q ", " PIXEL_Q NL
    "vtbl.8 " PIXEL_D0 ", {" PIXEL_D0 "-" PIXEL_D1 "}, " REMAP_NEON_ARM_Q NL
    "vst1.u32 " PIXEL_D00 ", [" DESTINATION_R "], " STRIDE_R NL

    "cmp " LEFT_R ", " SOURCE_R NL
    "bcc .allChannelSkipLeft" NL
    "vld1.u32 " PIXEL_D00 ", [" LEFT_R "]" NL
    "vtbl.8 " PIXEL_D1 ", {" PIXEL_D0 "}, " REMAP_ARM_NEON2_Q NL
    "vtbl.8 " PIXEL_D0 ", {" PIXEL_D0 "}, " REMAP_ARM_NEON1_Q NL
    "vsub.u32 " SUM_Q ", " SUM_Q ", " PIXEL_Q NL
".allChannelSkipLeft: " NL

    "cmp " RIGHT_R ", " SOURCE_END_R NL
    "bcs .allChannelSkipRight" NL
    "vld1.u32 " PIXEL_D00 ", [" RIGHT_R "]" NL
    "vtbl.8 " PIXEL_D1 ", {" PIXEL_D0 "}, " REMAP_ARM_NEON2_Q NL
    "vtbl.8 " PIXEL_D0 ", {" PIXEL_D0 "}, " REMAP_ARM_NEON1_Q NL
    "vadd.u32 " SUM_Q ", " SUM_Q ", " PIXEL_Q NL
".allChannelSkipRight: " NL

    "add " LEFT_R ", " LEFT_R ", " STRIDE_R NL
    "add " RIGHT_R ", " RIGHT_R ", " STRIDE_R NL
    "cmp " DESTINATION_R ", " DESTINATION_END_R NL
    "bcc .allChannelBlur" NL
    "sub " DESTINATION_R ", " DESTINATION_R ", " STRIDE_WIDTH_R NL

    "add " SOURCE_R ", " SOURCE_R ", " STRIDE_LINE_R NL
    "add " DESTINATION_R ", " DESTINATION_R ", " STRIDE_LINE_R NL
    "cmp " SOURCE_R ", " SOURCE_LINE_END_R NL
    "bcc .allChannelMainLoop" NL

    "ldmia sp!, {r4-r8, r10, r11, pc}" NL
); // NOLINT

#define DATA_TRANSFER4(command, base) \
    command " " PIXEL_D00 ", [" base "], " STRIDE_LINE_R NL \
    command " " PIXEL_D01 ", [" base "], " STRIDE_LINE_R NL \
    command " " PIXEL_D10 ", [" base "], " STRIDE_LINE_R NL \
    command " " PIXEL_D11 ", [" base "], " STRIDE_LINE_R NL \
    "sub " base ", " base ", " STRIDE_LINE_R ", lsl #2" NL

// The number of reads depend on REMAINING_STRIDES_R, but it is always >= 1 and <= 3
#define CONDITIONAL_DATA_TRANSFER4(command1, command2, base) \
    command1 " " PIXEL_D00 ", [" base "], " STRIDE_LINE_R NL \
    "cmp " REMAINING_STRIDES_R ", #2" NL \
    command2 "cs " PIXEL_S1 ", [" base "]" NL \
    "add " base ", " base ", " STRIDE_LINE_R NL \
    "cmp " REMAINING_STRIDES_R ", #3" NL \
    command2 "cs " PIXEL_S2 ", [" base "]" NL \
    "sub " base ", " base ", " STRIDE_LINE_R ", lsl #1" NL

asm ( // NOLINT
".globl " TOSTRING(neonDrawAlphaChannelGaussianBlur) NL
TOSTRING(neonDrawAlphaChannelGaussianBlur) ":" NL
    "stmdb sp!, {r4-r8, r10, r11, lr}" NL
    "ldr " STRIDE_R ", [r2, #" STRIDE_OFFSET "]" NL
    "ldr " STRIDE_WIDTH_R ", [r2, #" STRIDE_WIDTH_OFFSET "]" NL
    "ldr " DISTANCE_LEFT_R ", [r2, #" DISTANCE_LEFT_OFFSET "]" NL
    "ldr " DISTANCE_RIGHT_R ", [r2, #" DISTANCE_RIGHT_OFFSET "]" NL
    "ldr " STRIDE_LINE_R ", [r2, #" STRIDE_LINE_OFFSET "]" NL
    "ldr " SOURCE_LINE_END_R ", [r2, #" STRIDE_LINE_WIDTH_OFFSET "]" NL
    "ldr " INIT_INVERTED_KERNEL_SIZE_R ", [r2, #" INVERTED_KERNEL_SIZE_OFFSET "]" NL
    "vldr.u32 " REMAINING_STRIDES_S0 ", [r2, #" REMAINING_STRIDES_OFFSET "]" NL

    // Initialize locals.
    "mul " DISTANCE_LEFT_R ", " DISTANCE_LEFT_R ", " STRIDE_R NL
    "mul " DISTANCE_RIGHT_R ", " DISTANCE_RIGHT_R ", " STRIDE_R NL
    "mov " MAX_KERNEL_SIZE_R ", " DISTANCE_RIGHT_R NL
    "cmp " MAX_KERNEL_SIZE_R ", " STRIDE_WIDTH_R NL
    "movcs " MAX_KERNEL_SIZE_R ", " STRIDE_WIDTH_R NL
    "add " SOURCE_LINE_END_R ", " SOURCE_LINE_END_R ", " SOURCE_R NL
    "vdup.f32 " INVERTED_KERNEL_SIZE_Q ", " INIT_INVERTED_KERNEL_SIZE_R NL
    "cmp " SOURCE_LINE_END_R ", " SOURCE_R NL
    "beq .alphaChannelEarlyLeave" NL

    // Processing 4 strides parallelly.

".alphaChannelMainLoop:" NL

    // Initialize the sum variable.
    "vmov.u32 " SUM_Q ", #0" NL
    "mov " INIT_SUM_R ", " SOURCE_R NL
    "add " SOURCE_END_R ", " SOURCE_R ", " MAX_KERNEL_SIZE_R NL
    "cmp " INIT_SUM_R ", " SOURCE_END_R NL
    "bcs .alphaChannelInitSumDone" NL
".alphaChannelInitSum:" NL
    DATA_TRANSFER4("vld1.u32", INIT_SUM_R)
    "vshr.u32 " PIXEL_Q ", " PIXEL_Q ", #24" NL
    "vadd.u32 " SUM_Q ", " SUM_Q ", " PIXEL_Q NL
    "add " INIT_SUM_R ", " INIT_SUM_R ", " STRIDE_R NL
    "cmp " INIT_SUM_R ", " SOURCE_END_R NL
    "bcc .alphaChannelInitSum" NL
".alphaChannelInitSumDone:" NL

    // Blurring.
    "add " SOURCE_END_R ", " SOURCE_R ", " STRIDE_WIDTH_R NL
    "add " DESTINATION_END_R ", " DESTINATION_R ", " STRIDE_WIDTH_R NL
    "sub " LEFT_R ", " SOURCE_R ", " DISTANCE_LEFT_R NL
    "add " RIGHT_R ", " SOURCE_R ", " DISTANCE_RIGHT_R NL

".alphaChannelBlur:" NL
    "vcvt.f32.u32 " PIXEL_Q ", " SUM_Q NL
    "vmul.f32 " PIXEL_Q ", " PIXEL_Q ", " INVERTED_KERNEL_SIZE_Q NL
    "vcvt.u32.f32 " PIXEL_Q ", " PIXEL_Q NL
    "vshl.u32 " PIXEL_Q ", " PIXEL_Q ", #24" NL
    DATA_TRANSFER4("vst1.u32", DESTINATION_R)

    "cmp " LEFT_R ", " SOURCE_R NL
    "bcc .alphaChannelSkipLeft" NL
    DATA_TRANSFER4("vld1.u32", LEFT_R)
    "vshr.u32 " PIXEL_Q ", " PIXEL_Q ", #24" NL
    "vsub.u32 " SUM_Q ", " SUM_Q ", " PIXEL_Q NL
".alphaChannelSkipLeft: " NL

    "cmp " RIGHT_R ", " SOURCE_END_R NL
    "bcs .alphaChannelSkipRight" NL
    DATA_TRANSFER4("vld1.u32", RIGHT_R)
    "vshr.u32 " PIXEL_Q ", " PIXEL_Q ", #24" NL
    "vadd.u32 " SUM_Q ", " SUM_Q ", " PIXEL_Q NL
".alphaChannelSkipRight: " NL

    "add " DESTINATION_R ", " DESTINATION_R ", " STRIDE_R NL
    "add " LEFT_R ", " LEFT_R ", " STRIDE_R NL
    "add " RIGHT_R ", " RIGHT_R ", " STRIDE_R NL
    "cmp " DESTINATION_R ", " DESTINATION_END_R NL
    "bcc .alphaChannelBlur" NL
    "sub " DESTINATION_R ", " DESTINATION_R ", " STRIDE_WIDTH_R NL

    "add " SOURCE_R ", " SOURCE_R ", " STRIDE_LINE_R ", lsl #2" NL
    "add " DESTINATION_R ", " DESTINATION_R ", " STRIDE_LINE_R ", lsl #2" NL
    "cmp " SOURCE_R ", " SOURCE_LINE_END_R NL
    "bcc .alphaChannelMainLoop" NL

    // Processing the remaining strides (0 - 3).
".alphaChannelEarlyLeave:" NL
    "vmov.u32 " REMAINING_STRIDES_R ", " REMAINING_STRIDES_S0 NL
    // Early return for 0 strides.
    "cmp " REMAINING_STRIDES_R ", #0" NL
    "ldmeqia sp!, {r4-r8, r10, r11, pc}" NL

    // Initialize the sum variable.
    "vmov.u32 " SUM_Q ", #0" NL
    "mov " INIT_SUM_R ", " SOURCE_R NL
    "add " SOURCE_END_R ", " SOURCE_R ", " MAX_KERNEL_SIZE_R NL
    "cmp " INIT_SUM_R ", " SOURCE_END_R NL
    "bcs .alphaChannelSecondInitSumDone" NL
".alphaChannelSecondInitSum:" NL
    CONDITIONAL_DATA_TRANSFER4("vld1.u32", "vldr", INIT_SUM_R)
    "vshr.u32 " PIXEL_Q ", " PIXEL_Q ", #24" NL
    "vadd.u32 " SUM_Q ", " SUM_Q ", " PIXEL_Q NL
    "add " INIT_SUM_R ", " INIT_SUM_R ", " STRIDE_R NL
    "cmp " INIT_SUM_R ", " SOURCE_END_R NL
    "bcc .alphaChannelSecondInitSum" NL
".alphaChannelSecondInitSumDone:" NL

    // Blurring.
    "add " SOURCE_END_R ", " SOURCE_R ", " STRIDE_WIDTH_R NL
    "add " DESTINATION_END_R ", " DESTINATION_R ", " STRIDE_WIDTH_R NL
    "sub " LEFT_R ", " SOURCE_R ", " DISTANCE_LEFT_R NL
    "add " RIGHT_R ", " SOURCE_R ", " DISTANCE_RIGHT_R NL

".alphaChannelSecondBlur:" NL
    "vcvt.f32.u32 " PIXEL_Q ", " SUM_Q NL
    "vmul.f32 " PIXEL_Q ", " PIXEL_Q ", " INVERTED_KERNEL_SIZE_Q NL
    "vcvt.u32.f32 " PIXEL_Q ", " PIXEL_Q NL
    "vshl.u32 " PIXEL_Q ", " PIXEL_Q ", #24" NL
    CONDITIONAL_DATA_TRANSFER4("vst1.u32", "vstr", DESTINATION_R)

    "cmp " LEFT_R ", " SOURCE_R NL
    "bcc .alphaChannelSecondSkipLeft" NL
    CONDITIONAL_DATA_TRANSFER4("vld1.u32", "vldr", LEFT_R)
    "vshr.u32 " PIXEL_Q ", " PIXEL_Q ", #24" NL
    "vsub.u32 " SUM_Q ", " SUM_Q ", " PIXEL_Q NL
".alphaChannelSecondSkipLeft: " NL

    "cmp " RIGHT_R ", " SOURCE_END_R NL
    "bcs .alphaChannelSecondSkipRight" NL
    CONDITIONAL_DATA_TRANSFER4("vld1.u32", "vldr", RIGHT_R)
    "vshr.u32 " PIXEL_Q ", " PIXEL_Q ", #24" NL
    "vadd.u32 " SUM_Q ", " SUM_Q ", " PIXEL_Q NL
".alphaChannelSecondSkipRight: " NL

    "add " DESTINATION_R ", " DESTINATION_R ", " STRIDE_R NL
    "add " LEFT_R ", " LEFT_R ", " STRIDE_R NL
    "add " RIGHT_R ", " RIGHT_R ", " STRIDE_R NL
    "cmp " DESTINATION_R ", " DESTINATION_END_R NL
    "bcc .alphaChannelSecondBlur" NL

    "ldmia sp!, {r4-r8, r10, r11, pc}" NL
); // NOLINT

} // namespace WebCore

#endif // CPU(ARM_NEON) && COMPILER(GCC)
