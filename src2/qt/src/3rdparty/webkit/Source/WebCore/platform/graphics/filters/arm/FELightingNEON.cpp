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
#include "FELightingNEON.h"

#if CPU(ARM_NEON) && COMPILER(GCC)

#include <wtf/Alignment.h>

namespace WebCore {

// These constants are copied to the following SIMD registers:
//   ALPHAX_Q ALPHAY_Q REMAPX_D REMAPY_D

static WTF_ALIGNED(short, s_FELightingConstantsForNeon[], 16) = {
    // Alpha coefficients.
    -2, 1, 0, -1, 2, 1, 0, -1,
    0, -1, -2, -1, 0, 1, 2, 1,
    // Remapping indicies.
    0x0f0e, 0x0302, 0x0504, 0x0706,
    0x0b0a, 0x1312, 0x1514, 0x1716,
};

short* feLightingConstantsForNeon()
{
    return s_FELightingConstantsForNeon;
}

#define ASSTRING(str) #str
#define TOSTRING(value) ASSTRING(value)

#define PIXELS_OFFSET TOSTRING(0)
#define WIDTH_OFFSET TOSTRING(4)
#define HEIGHT_OFFSET TOSTRING(8)
#define FLAGS_OFFSET TOSTRING(12)
#define SPECULAR_EXPONENT_OFFSET TOSTRING(16)
#define CONE_EXPONENT_OFFSET TOSTRING(20)
#define FLOAT_ARGUMENTS_OFFSET TOSTRING(24)
#define PAINTING_CONSTANTS_OFFSET TOSTRING(28)
#define NL "\n"

// Register allocation
#define PAINTING_DATA_R       "r11"
#define RESET_WIDTH_R         PAINTING_DATA_R
#define PIXELS_R              "r4"
#define WIDTH_R               "r5"
#define HEIGHT_R              "r6"
#define FLAGS_R               "r7"
#define SPECULAR_EXPONENT_R   "r8"
#define CONE_EXPONENT_R       "r10"
#define SCANLINE_R            "r12"

#define TMP1_Q                "q0"
#define TMP1_D0               "d0"
#define TMP1_S0               "s0"
#define TMP1_S1               "s1"
#define TMP1_D1               "d1"
#define TMP1_S2               "s2"
#define TMP1_S3               "s3"
#define TMP2_Q                "q1"
#define TMP2_D0               "d2"
#define TMP2_S0               "s4"
#define TMP2_S1               "s5"
#define TMP2_D1               "d3"
#define TMP2_S2               "s6"
#define TMP2_S3               "s7"
#define TMP3_Q                "q2"
#define TMP3_D0               "d4"
#define TMP3_S0               "s8"
#define TMP3_S1               "s9"
#define TMP3_D1               "d5"
#define TMP3_S2               "s10"
#define TMP3_S3               "s11"

#define COSINE_OF_ANGLE       "s12"
#define POWF_INT_S            "s13"
#define POWF_FRAC_S           "s14"
#define SPOT_COLOR_Q          "q4"

// Because of VMIN and VMAX CONST_ZERO_S and CONST_ONE_S
// must be placed on the same side of the double vector

// Current pixel position
#define POSITION_Q            "q5"
#define POSITION_X_S          "s20"
#define POSITION_Y_S          "s21"
#define POSITION_Z_S          "s22"
#define CONST_ZERO_HI_D       "d11"
#define CONST_ZERO_S          "s23"

// -------------------------------
//     Variable arguments
// Misc arguments
#define READ1_RANGE           "d12-d15"
#define READ2_RANGE           "d16-d19"
#define READ3_RANGE           "d20-d21"

#define SCALE_S               "s24"
#define SCALE_DIV4_S          "s25"
#define DIFFUSE_CONST_S       "s26"

// Light source position
#define CONE_CUT_OFF_S        "s28"
#define CONE_FULL_LIGHT_S     "s29"
#define CONE_CUT_OFF_RANGE_S  "s30"
#define CONST_ONE_HI_D        "d15"
#define CONST_ONE_S           "s31"

#define LIGHT_Q               "q8"
#define DIRECTION_Q           "q9"
#define COLOR_Q               "q10"
// -------------------------------
//    Constant coefficients
#define READ4_RANGE           "d22-d25"
#define READ5_RANGE           "d26-d27"

#define ALPHAX_Q              "q11"
#define ALPHAY_Q              "q12"
#define REMAPX_D              "d26"
#define REMAPY_D              "d27"
// -------------------------------

#define ALL_ROWS_D            "{d28,d29,d30}"
#define TOP_ROW_D             "d28"
#define MIDDLE_ROW_D          "d29"
#define BOTTOM_ROW_D          "d30"

#define GET_LENGTH(source, temp) \
    "vmul.f32 " temp##_Q ", " source##_Q ", " source##_Q NL \
    "vadd.f32 " source##_S3 ", " temp##_S0 ", " temp##_S1 NL \
    "vadd.f32 " source##_S3 ", " source##_S3 ", " temp##_S2 NL \
    "vsqrt.f32 " source##_S3 ", " source##_S3 NL

// destination##_S3 can contain the multiply of length.
#define DOT_PRODUCT(destination, source1, source2) \
    "vmul.f32 " destination##_Q ", " source1##_Q ", " source2##_Q NL \
    "vadd.f32 " destination##_S0 ", " destination##_S0 ", " destination##_S1 NL \
    "vadd.f32 " destination##_S0 ", " destination##_S0 ", " destination##_S2 NL

#define MULTIPLY_BY_DIFFUSE_CONST(normalVectorLength, dotProductLength) \
    "tst " FLAGS_R ", #" TOSTRING(FLAG_DIFFUSE_CONST_IS_1) NL \
    "vmuleq.f32 " TMP2_S1 ", " DIFFUSE_CONST_S ", " normalVectorLength NL \
    "vdiveq.f32 " TMP2_S1 ", " TMP2_S1 ", " dotProductLength NL \
    "vdivne.f32 " TMP2_S1 ", " normalVectorLength ", " dotProductLength NL

#define POWF_SQR(value, exponent, current, remaining) \
    "tst " exponent ", #" ASSTRING(current) NL \
    "vmulne.f32 " value ", " value ", " POWF_INT_S NL \
    "tst " exponent ", #" ASSTRING(remaining) NL \
    "vmulne.f32 " POWF_INT_S ", " POWF_INT_S ", " POWF_INT_S NL

#define POWF_SQRT(value, exponent, current, remaining) \
    "tst " exponent ", #" ASSTRING(remaining) NL \
    "vsqrtne.f32 " POWF_FRAC_S ", " POWF_FRAC_S NL \
    "tst " exponent ", #" ASSTRING(current) NL \
    "vmulne.f32 " value ", " value ", " POWF_FRAC_S NL

// This simplified powf function is sufficiently accurate.
#define POWF(value, exponent) \
    "tst " exponent ", #0xfc0" NL \
    "vmovne.f32 " POWF_INT_S ", " value NL \
    "tst " exponent ", #0x03f" NL \
    "vmovne.f32 " POWF_FRAC_S ", " value NL \
    "vmov.f32 " value ", " CONST_ONE_S NL \
    \
    POWF_SQR(value, exponent, 0x040, 0xf80) \
    POWF_SQR(value, exponent, 0x080, 0xf00) \
    POWF_SQR(value, exponent, 0x100, 0xe00) \
    POWF_SQR(value, exponent, 0x200, 0xc00) \
    POWF_SQR(value, exponent, 0x400, 0x800) \
    "tst " exponent ", #0x800" NL \
    "vmulne.f32 " value ", " value ", " POWF_INT_S NL \
    \
    POWF_SQRT(value, exponent, 0x20, 0x3f) \
    POWF_SQRT(value, exponent, 0x10, 0x1f) \
    POWF_SQRT(value, exponent, 0x08, 0x0f) \
    POWF_SQRT(value, exponent, 0x04, 0x07) \
    POWF_SQRT(value, exponent, 0x02, 0x03) \
    POWF_SQRT(value, exponent, 0x01, 0x01)

// The following algorithm is an ARM-NEON optimized version of
// the main loop found in FELighting.cpp. Since the whole code
// is redesigned to be as effective as possible (ARM specific
// thinking), it is four times faster than its C++ counterpart.

asm ( // NOLINT
".globl " TOSTRING(neonDrawLighting) NL
TOSTRING(neonDrawLighting) ":" NL
    // Because of the clever register allocation, nothing is stored on the stack
    // except the saved registers.
    // Stack must be aligned to 8 bytes.
    "stmdb sp!, {r4-r8, r10, r11, lr}" NL
    "vstmdb sp!, {d8-d15}" NL
    "mov " PAINTING_DATA_R ", r0" NL

    // The following two arguments are loaded to SIMD registers.
    "ldr r0, [" PAINTING_DATA_R ", #" FLOAT_ARGUMENTS_OFFSET "]" NL
    "ldr r1, [" PAINTING_DATA_R ", #" PAINTING_CONSTANTS_OFFSET "]" NL
    "ldr " PIXELS_R ", [" PAINTING_DATA_R ", #" PIXELS_OFFSET "]" NL
    "ldr " WIDTH_R ", [" PAINTING_DATA_R ", #" WIDTH_OFFSET "]" NL
    "ldr " HEIGHT_R ", [" PAINTING_DATA_R ", #" HEIGHT_OFFSET "]" NL
    "ldr " FLAGS_R ", [" PAINTING_DATA_R ", #" FLAGS_OFFSET "]" NL
    "ldr " SPECULAR_EXPONENT_R ", [" PAINTING_DATA_R ", #" SPECULAR_EXPONENT_OFFSET "]" NL
    "ldr " CONE_EXPONENT_R ", [" PAINTING_DATA_R ", #" CONE_EXPONENT_OFFSET "]" NL

    // Load all data to the SIMD registers with the least number of instructions.
    "vld1.f32 { " READ1_RANGE " }, [r0]!" NL
    "vld1.f32 { " READ2_RANGE " }, [r0]!" NL
    "vld1.f32 { " READ3_RANGE " }, [r0]!" NL
    "vld1.s16 {" READ4_RANGE "}, [r1]!" NL
    "vld1.s16 {" READ5_RANGE "}, [r1]!" NL

    // Initializing local variables.
    "mov " SCANLINE_R ", " WIDTH_R ", lsl #2" NL
    "add " SCANLINE_R ", " SCANLINE_R ", #8" NL
    "add " PIXELS_R ", " PIXELS_R ", " SCANLINE_R NL
    "add " PIXELS_R ", " PIXELS_R ", #3" NL
    "mov r0, #0" NL
    "vmov.f32 " CONST_ZERO_S ", r0" NL
    "vmov.f32 " POSITION_Y_S ", " CONST_ONE_S NL
    "tst " FLAGS_R ", #" TOSTRING(FLAG_SPOT_LIGHT) NL
    "vmov.f32 " SPOT_COLOR_Q ", " COLOR_Q NL
    "mov " RESET_WIDTH_R ", " WIDTH_R NL

".mainLoop:" NL
    "mov r3, #3" NL
    "vmov.f32 " POSITION_X_S ", " CONST_ONE_S NL

".scanline:" NL
    // The ROW registers are storing the alpha channel of the last three pixels.
    // The alpha channel is stored as signed short (sint16) values. The fourth value
    // is garbage. The following instructions are shifting out the unnecessary alpha
    // values and load the next ones.
    "ldrb r0, [" PIXELS_R ", -" SCANLINE_R "]" NL
    "ldrb r1, [" PIXELS_R ", +" SCANLINE_R "]" NL
    "ldrb r2, [" PIXELS_R "], #4" NL
    "vext.s16 " TOP_ROW_D ", " TOP_ROW_D ", " TOP_ROW_D ", #3" NL
    "vext.s16 " MIDDLE_ROW_D ", " MIDDLE_ROW_D ", " MIDDLE_ROW_D ", #3" NL
    "vext.s16 " BOTTOM_ROW_D ", " BOTTOM_ROW_D ", " BOTTOM_ROW_D ", #3" NL
    "vmov.s16 " TOP_ROW_D "[1], r0" NL
    "vmov.s16 " MIDDLE_ROW_D "[1], r2" NL
    "vmov.s16 " BOTTOM_ROW_D "[1], r1" NL

    // The two border pixels (rightmost and leftmost) are skipped when
    // the next scanline is reached. It also jumps, when the algorithm
    // is started, and the first free alpha values are loaded to each row.
    "subs r3, r3, #1" NL
    "bne .scanline" NL

    // The light vector goes to TMP1_Q. It is constant in case of distant light.
    // The fourth value contains the length of the light vector.
    "tst " FLAGS_R ", #" TOSTRING(FLAG_POINT_LIGHT | FLAG_SPOT_LIGHT) NL
    "beq .distantLight" NL

    "vmov.s16 r3, " MIDDLE_ROW_D "[2]" NL
    "vmov.f32 " POSITION_Z_S ", r3" NL
    "vcvt.f32.s32 " POSITION_Z_S ", " POSITION_Z_S NL
    "vmul.f32 " POSITION_Z_S ", " POSITION_Z_S ", " SCALE_S NL

    "vsub.f32 " TMP1_Q ", " LIGHT_Q ", " POSITION_Q NL
    GET_LENGTH(TMP1, TMP2)

    "tst " FLAGS_R ", #" TOSTRING(FLAG_SPOT_LIGHT) NL
    "bne .cosineOfAngle" NL
".visiblePixel:" NL

    //     | -1  0  1 |      | -1 -2 -1 |
    // X = | -2  0  2 |  Y = |  0  0  0 |
    //     | -1  0  1 |      |  1  2  1 |

    // Multiply the alpha values by the X and Y matrices.

    // Moving the 8 alpha value to TMP3.
    "vtbl.8 " TMP3_D0 ", " ALL_ROWS_D ", " REMAPX_D NL
    "vtbl.8 " TMP3_D1 ", " ALL_ROWS_D ", " REMAPY_D NL

    "vmul.s16 " TMP2_Q ", " TMP3_Q ", " ALPHAX_Q NL
    "vpadd.s16 " TMP2_D0 ", " TMP2_D0 ", " TMP2_D1 NL
    "vpadd.s16 " TMP2_D0 ", " TMP2_D0 ", " TMP2_D0 NL
    "vpadd.s16 " TMP2_D0 ", " TMP2_D0 ", " TMP2_D0 NL
    "vmov.s16 r0, " TMP2_D0 "[0]" NL

    "vmul.s16 " TMP2_Q ", " TMP3_Q ", " ALPHAY_Q NL
    "vpadd.s16 " TMP2_D0 ", " TMP2_D0 ", " TMP2_D1 NL
    "vpadd.s16 " TMP2_D0 ", " TMP2_D0 ", " TMP2_D0 NL
    "vpadd.s16 " TMP2_D0 ", " TMP2_D0 ", " TMP2_D0 NL
    "vmov.s16 r1, " TMP2_D0 "[0]" NL

    // r0 and r1 contains the X and Y coordinates of the
    // normal vector, respectively.

    // Calculating the spot light strength.
    "tst " FLAGS_R ", #" TOSTRING(FLAG_SPOT_LIGHT) NL
    "beq .endLight" NL

    "vneg.f32 " TMP3_S1 ", " COSINE_OF_ANGLE NL
    "tst " FLAGS_R ", #" TOSTRING(FLAG_CONE_EXPONENT_IS_1) NL
    "beq .coneExpPowf" NL
".coneExpPowfFinished:" NL

    // Smoothing the cone edge if necessary.
    "vcmp.f32 " COSINE_OF_ANGLE ", " CONE_FULL_LIGHT_S NL
    "fmstat" NL
    "bhi .cutOff" NL
".cutOffFinished:" NL

    "vmin.f32 " TMP3_D0 ", " TMP3_D0 ", " CONST_ONE_HI_D NL
    "vmul.f32 " COLOR_Q ", " SPOT_COLOR_Q ", " TMP3_D0 "[1]" NL

".endLight:" NL
    // Summarize:
    // r0 and r1 contains the normalVector.
    // TMP1_Q contains the light vector and its length.
    // COLOR_Q contains the color of the light vector.

    // Test whether both r0 and r1 are zero (Normal vector is (0, 0, 1)).
    "orrs r2, r0, r1" NL
    "bne .normalVectorIsNonZero" NL

    "tst " FLAGS_R ", #" TOSTRING(FLAG_SPECULAR_LIGHT) NL
    "bne .specularLight1" NL

    // Calculate diffuse light strength.
    MULTIPLY_BY_DIFFUSE_CONST(TMP1_S2, TMP1_S3)
    "b .lightStrengthCalculated" NL

".specularLight1:" NL
    // Calculating specular light strength.
    "vadd.f32 " TMP1_S2 ", " TMP1_S2 ", " TMP1_S3 NL
    GET_LENGTH(TMP1, TMP2)

    // When the exponent is 1, we don't need to call an expensive powf function.
    "tst " FLAGS_R ", #" TOSTRING(FLAG_SPECULAR_EXPONENT_IS_1) NL
    "vdiveq.f32 " TMP2_S1 ", " TMP1_S2 ", " TMP1_S3 NL
    "beq .specularExpPowf" NL

    MULTIPLY_BY_DIFFUSE_CONST(TMP1_S2, TMP1_S3)
    "b .lightStrengthCalculated" NL

".normalVectorIsNonZero:" NL
    // Normal vector goes to TMP2, and its length is calculated as well.
    "vmov.s32 " TMP2_S0 ", r0" NL
    "vcvt.f32.s32 " TMP2_S0 ", " TMP2_S0 NL
    "vmul.f32 " TMP2_S0 ", " TMP2_S0 ", " SCALE_DIV4_S NL
    "vmov.s32 " TMP2_S1 ", r1" NL
    "vcvt.f32.s32 " TMP2_S1 ", " TMP2_S1 NL
    "vmul.f32 " TMP2_S1 ", " TMP2_S1 ", " SCALE_DIV4_S NL
    "vmov.f32 " TMP2_S2 ", " CONST_ONE_S NL
    GET_LENGTH(TMP2, TMP3)

    "tst " FLAGS_R ", #" TOSTRING(FLAG_SPECULAR_LIGHT) NL
    "bne .specularLight2" NL

    // Calculating diffuse light strength.
    DOT_PRODUCT(TMP3, TMP2, TMP1)
    MULTIPLY_BY_DIFFUSE_CONST(TMP3_S0, TMP3_S3)
    "b .lightStrengthCalculated" NL

".specularLight2:" NL
    // Calculating specular light strength.
    "vadd.f32 " TMP1_S2 ", " TMP1_S2 ", " TMP1_S3 NL
    GET_LENGTH(TMP1, TMP3)
    DOT_PRODUCT(TMP3, TMP2, TMP1)

    // When the exponent is 1, we don't need to call an expensive powf function.
    "tst " FLAGS_R ", #" TOSTRING(FLAG_SPECULAR_EXPONENT_IS_1) NL
    "vdiveq.f32 " TMP2_S1 ", " TMP3_S0 ", " TMP3_S3 NL
    "beq .specularExpPowf" NL
    MULTIPLY_BY_DIFFUSE_CONST(TMP3_S0, TMP3_S3)

".lightStrengthCalculated:" NL
    // TMP2_S1 contains the light strength. Clamp it to [0, 1]
    "vmax.f32 " TMP2_D0 ", " TMP2_D0 ", " CONST_ZERO_HI_D NL
    "vmin.f32 " TMP2_D0 ", " TMP2_D0 ", " CONST_ONE_HI_D NL
    "vmul.f32 " TMP3_Q ", " COLOR_Q ", " TMP2_D0 "[1]" NL
    "vcvt.u32.f32 " TMP3_Q ", " TMP3_Q NL
    "vmov.u32 r2, r3, " TMP3_S0 ", " TMP3_S1 NL
    // The color values are stored in-place.
    "strb r2, [" PIXELS_R ", #-11]" NL
    "strb r3, [" PIXELS_R ", #-10]" NL
    "vmov.u32 r2, " TMP3_S2 NL
    "strb r2, [" PIXELS_R ", #-9]" NL

    // Continue to the next pixel.
".blackPixel:" NL
    "vadd.f32 " POSITION_X_S ", " CONST_ONE_S NL
    "mov r3, #1" NL
    "subs " WIDTH_R ", " WIDTH_R ", #1" NL
    "bne .scanline" NL

    // If the end of the scanline is reached, we continue
    // to the next scanline.
    "vadd.f32 " POSITION_Y_S ", " CONST_ONE_S NL
    "mov " WIDTH_R ", " RESET_WIDTH_R NL
    "subs " HEIGHT_R ", " HEIGHT_R ", #1" NL
    "bne .mainLoop" NL

    // Return.
    "vldmia sp!, {d8-d15}" NL
    "ldmia sp!, {r4-r8, r10, r11, pc}" NL

".distantLight:" NL
    // In case of distant light, the light vector is constant,
    // we simply copy it.
    "vmov.f32 " TMP1_Q ", " LIGHT_Q NL
    "b .visiblePixel" NL

".cosineOfAngle:" NL
    // If the pixel is outside of the cone angle, it is simply a black pixel.
    DOT_PRODUCT(TMP3, TMP1, DIRECTION)
    "vdiv.f32 " COSINE_OF_ANGLE ", " TMP3_S0 ", " TMP1_S3 NL
    "vcmp.f32 " COSINE_OF_ANGLE ", " CONE_CUT_OFF_S NL
    "fmstat" NL
    "bls .visiblePixel" NL
    "mov r0, #0" NL
    "strh r0, [" PIXELS_R ", #-11]" NL
    "strb r0, [" PIXELS_R ", #-9]" NL
    "b .blackPixel" NL

".cutOff:" NL
    // Smoothing the light strength on the cone edge.
    "vsub.f32 " TMP3_S0 ", " CONE_CUT_OFF_S ", " COSINE_OF_ANGLE NL
    "vdiv.f32 " TMP3_S0 ", " TMP3_S0 ", " CONE_CUT_OFF_RANGE_S NL
    "vmul.f32 " TMP3_S1 ", " TMP3_S1 ", " TMP3_S0 NL
    "b .cutOffFinished" NL

".coneExpPowf:" NL
    POWF(TMP3_S1, CONE_EXPONENT_R)
    "b .coneExpPowfFinished" NL

".specularExpPowf:" NL
    POWF(TMP2_S1, SPECULAR_EXPONENT_R)
    "tst " FLAGS_R ", #" TOSTRING(FLAG_DIFFUSE_CONST_IS_1) NL
    "vmuleq.f32 " TMP2_S1 ", " TMP2_S1 ", " DIFFUSE_CONST_S NL
    "b .lightStrengthCalculated" NL
); // NOLINT

int FELighting::getPowerCoefficients(float exponent)
{
    // Calling a powf function from the assembly code would require to save
    // and reload a lot of NEON registers. Since the base is in range [0..1]
    // and only 8 bit precision is required, we use our own powf function.
    // This is probably not the best, but it uses only a few registers and
    // gives us enough precision (modifying the exponent field directly would
    // also be possible).

    // First, we limit the exponent to maximum of 64, which gives us enough
    // precision. We split the exponent to an integer and fraction part,
    // since a^x = (a^y)*(a^z) where x = y+z. The integer exponent of the
    // power is estimated by square, and the fraction exponent of the power
    // is estimated by square root assembly instructions.
    int i, result;

    if (exponent < 0)
        exponent = 1 / (-exponent);

    if (exponent > 63.99)
        exponent = 63.99;

    exponent /= 64;
    result = 0;
    for (i = 11; i >= 0; --i) {
        exponent *= 2;
        if (exponent >= 1) {
            result |= 1 << i;
            exponent -= 1;
        }
    }
    return result;
}

} // namespace WebCore

#endif // CPU(ARM_NEON) && COMPILER(GCC)
