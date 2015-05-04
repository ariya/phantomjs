//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// imageformats.h: Defines image format types with functions for mip generation
// and copying.

#ifndef LIBGLESV2_RENDERER_IMAGEFORMATS_H_
#define LIBGLESV2_RENDERER_IMAGEFORMATS_H_

#include "common/mathutil.h"

namespace rx
{

// Several structures share functionality for reading, writing or mipmapping but the layout
// must match the texture format which the structure represents. If collapsing or typedefing
// structs in this header, make sure the functionality and memory layout is exactly the same.

struct L8
{
    unsigned char L;

    static void readColor(gl::ColorF *dst, const L8 *src)
    {
        const float lum = gl::normalizedToFloat(src->L);
        dst->red   = lum;
        dst->green = lum;
        dst->blue  = lum;
        dst->alpha = 1.0f;
    }

    static void writeColor(L8 *dst, const gl::ColorF *src)
    {
        dst->L = gl::floatToNormalized<unsigned char>((src->red + src->green + src->blue) / 3.0f);
    }

    static void average(L8 *dst, const L8 *src1, const L8 *src2)
    {
        dst->L = gl::average(src1->L, src2->L);
    }
};

struct R8
{
    unsigned char R;

    static void readColor(gl::ColorF *dst, const R8 *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = 0.0f;
        dst->blue  = 0.0f;
        dst->alpha = 1.0f;
    }

    static void readColor(gl::ColorUI *dst, const R8 *src)
    {
        dst->red   = src->R;
        dst->green = 0;
        dst->blue  = 0;
        dst->alpha = 1;
    }

    static void writeColor(R8 *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<unsigned char>(src->red);
    }

    static void writeColor(R8 *dst, const gl::ColorUI *src)
    {
        dst->R = static_cast<unsigned char>(src->red);
    }

    static void average(R8 *dst, const R8 *src1, const R8 *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
    }
};

struct A8
{
    unsigned char A;

    static void readColor(gl::ColorF *dst, const A8 *src)
    {
        dst->red   = 0.0f;
        dst->green = 0.0f;
        dst->blue  = 0.0f;
        dst->alpha = gl::normalizedToFloat(src->A);
    }

    static void writeColor(A8 *dst, const gl::ColorF *src)
    {
        dst->A = gl::floatToNormalized<unsigned char>(src->alpha);
    }

    static void average(A8 *dst, const A8 *src1, const A8 *src2)
    {
        dst->A = gl::average(src1->A, src2->A);
    }
};

struct L8A8
{
    unsigned char L;
    unsigned char A;

    static void readColor(gl::ColorF *dst, const L8A8 *src)
    {
        const float lum = gl::normalizedToFloat(src->L);
        dst->red   = lum;
        dst->green = lum;
        dst->blue  = lum;
        dst->alpha = gl::normalizedToFloat(src->A);
    }

    static void writeColor(L8A8 *dst, const gl::ColorF *src)
    {
        dst->L = gl::floatToNormalized<unsigned char>((src->red + src->green + src->blue) / 3.0f);
        dst->A = gl::floatToNormalized<unsigned char>(src->alpha);
    }

    static void average(L8A8 *dst, const L8A8 *src1, const L8A8 *src2)
    {
        *(unsigned short*)dst = (((*(unsigned short*)src1 ^ *(unsigned short*)src2) & 0xFEFE) >> 1) + (*(unsigned short*)src1 & *(unsigned short*)src2);
    }
};

struct A8L8
{
    unsigned char A;
    unsigned char L;

    static void readColor(gl::ColorF *dst, const A8L8 *src)
    {
        const float lum = gl::normalizedToFloat(src->L);
        dst->red   = lum;
        dst->green = lum;
        dst->blue  = lum;
        dst->alpha = gl::normalizedToFloat(src->A);
    }

    static void writeColor(A8L8 *dst, const gl::ColorF *src)
    {
        dst->L = gl::floatToNormalized<unsigned char>((src->red + src->green + src->blue) / 3.0f);
        dst->A = gl::floatToNormalized<unsigned char>(src->alpha);
    }

    static void average(A8L8 *dst, const A8L8 *src1, const A8L8 *src2)
    {
        *(unsigned short*)dst = (((*(unsigned short*)src1 ^ *(unsigned short*)src2) & 0xFEFE) >> 1) + (*(unsigned short*)src1 & *(unsigned short*)src2);
    }
};

struct R8G8
{
    unsigned char R;
    unsigned char G;

    static void readColor(gl::ColorF *dst, const R8G8 *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue  = 0.0f;
        dst->alpha = 1.0f;
    }

    static void readColor(gl::ColorUI *dst, const R8G8 *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = 0;
        dst->alpha = 1;
    }

    static void writeColor(R8G8 *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<unsigned char>(src->red);
        dst->G = gl::floatToNormalized<unsigned char>(src->green);
    }

    static void writeColor(R8G8 *dst, const gl::ColorUI *src)
    {
        dst->R = static_cast<unsigned char>(src->red);
        dst->G = static_cast<unsigned char>(src->green);
    }

    static void average(R8G8 *dst, const R8G8 *src1, const R8G8 *src2)
    {
        *(unsigned short*)dst = (((*(unsigned short*)src1 ^ *(unsigned short*)src2) & 0xFEFE) >> 1) + (*(unsigned short*)src1 & *(unsigned short*)src2);
    }
};

struct R8G8B8
{
    unsigned char R;
    unsigned char G;
    unsigned char B;

    static void readColor(gl::ColorF *dst, const R8G8B8 *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue  = gl::normalizedToFloat(src->B);
        dst->alpha = 1.0f;
    }

    static void readColor(gl::ColorUI *dst, const R8G8B8 *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = src->G;
        dst->alpha = 1;
    }

    static void writeColor(R8G8B8 *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<unsigned char>(src->red);
        dst->G = gl::floatToNormalized<unsigned char>(src->green);
        dst->B = gl::floatToNormalized<unsigned char>(src->blue);
    }

    static void writeColor(R8G8B8 *dst, const gl::ColorUI *src)
    {
        dst->R = static_cast<unsigned char>(src->red);
        dst->G = static_cast<unsigned char>(src->green);
        dst->B = static_cast<unsigned char>(src->blue);
    }

    static void average(R8G8B8 *dst, const R8G8B8 *src1, const R8G8B8 *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
        dst->B = gl::average(src1->B, src2->B);
    }
};

struct B8G8R8
{
    unsigned char B;
    unsigned char G;
    unsigned char R;

    static void readColor(gl::ColorF *dst, const B8G8R8 *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue  = gl::normalizedToFloat(src->B);
        dst->alpha = 1.0f;
    }

    static void readColor(gl::ColorUI *dst, const B8G8R8 *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = src->G;
        dst->alpha = 1;
    }

    static void writeColor(B8G8R8 *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<unsigned char>(src->red);
        dst->G = gl::floatToNormalized<unsigned char>(src->green);
        dst->B = gl::floatToNormalized<unsigned char>(src->blue);
    }

    static void writeColor(B8G8R8 *dst, const gl::ColorUI *src)
    {
        dst->R = static_cast<unsigned char>(src->red);
        dst->G = static_cast<unsigned char>(src->green);
        dst->B = static_cast<unsigned char>(src->blue);
    }

    static void average(B8G8R8 *dst, const B8G8R8 *src1, const B8G8R8 *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
        dst->B = gl::average(src1->B, src2->B);
    }
};

struct R5G6B5
{
    unsigned short RGB;

    static void readColor(gl::ColorF *dst, const R5G6B5 *src)
    {
        dst->red   = gl::normalizedToFloat<5>(gl::getShiftedData<5, 11>(src->RGB));
        dst->green = gl::normalizedToFloat<6>(gl::getShiftedData<6,  5>(src->RGB));
        dst->blue  = gl::normalizedToFloat<5>(gl::getShiftedData<5,  0>(src->RGB));
        dst->alpha = 1.0f;
    }

    static void writeColor(R5G6B5 *dst, const gl::ColorF *src)
    {
        dst->RGB = gl::shiftData<5, 11>(gl::floatToNormalized<5, unsigned short>(src->red))   |
                   gl::shiftData<6,  5>(gl::floatToNormalized<6, unsigned short>(src->green)) |
                   gl::shiftData<5,  0>(gl::floatToNormalized<5, unsigned short>(src->blue));
    }

    static void average(R5G6B5 *dst, const R5G6B5 *src1, const R5G6B5 *src2)
    {
        dst->RGB = gl::shiftData<5, 11>(gl::average(gl::getShiftedData<5, 11>(src1->RGB), gl::getShiftedData<5, 11>(src2->RGB))) |
                   gl::shiftData<6,  5>(gl::average(gl::getShiftedData<6,  5>(src1->RGB), gl::getShiftedData<6,  5>(src2->RGB))) |
                   gl::shiftData<5,  0>(gl::average(gl::getShiftedData<5,  0>(src1->RGB), gl::getShiftedData<5,  0>(src2->RGB)));
    }
};

struct A8R8G8B8
{
    unsigned char A;
    unsigned char R;
    unsigned char G;
    unsigned char B;

    static void readColor(gl::ColorF *dst, const A8R8G8B8 *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue  = gl::normalizedToFloat(src->B);
        dst->alpha = gl::normalizedToFloat(src->A);
    }

    static void readColor(gl::ColorUI *dst, const A8R8G8B8 *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = src->B;
        dst->alpha = src->A;
    }

    static void writeColor(A8R8G8B8 *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<unsigned char>(src->red);
        dst->G = gl::floatToNormalized<unsigned char>(src->green);
        dst->B = gl::floatToNormalized<unsigned char>(src->blue);
        dst->A = gl::floatToNormalized<unsigned char>(src->alpha);
    }

    static void writeColor(A8R8G8B8 *dst, const gl::ColorUI *src)
    {
        dst->R = static_cast<unsigned char>(src->red);
        dst->G = static_cast<unsigned char>(src->green);
        dst->B = static_cast<unsigned char>(src->blue);
        dst->A = static_cast<unsigned char>(src->alpha);
    }

    static void average(A8R8G8B8 *dst, const A8R8G8B8 *src1, const A8R8G8B8 *src2)
    {
        *(unsigned int*)dst = (((*(unsigned int*)src1 ^ *(unsigned int*)src2) & 0xFEFEFEFE) >> 1) + (*(unsigned int*)src1 & *(unsigned int*)src2);
    }
};

struct R8G8B8A8
{
    unsigned char R;
    unsigned char G;
    unsigned char B;
    unsigned char A;

    static void readColor(gl::ColorF *dst, const R8G8B8A8 *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue  = gl::normalizedToFloat(src->B);
        dst->alpha = gl::normalizedToFloat(src->A);
    }

    static void readColor(gl::ColorUI *dst, const R8G8B8A8 *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = src->B;
        dst->alpha = src->A;
    }

    static void writeColor(R8G8B8A8 *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<unsigned char>(src->red);
        dst->G = gl::floatToNormalized<unsigned char>(src->green);
        dst->B = gl::floatToNormalized<unsigned char>(src->blue);
        dst->A = gl::floatToNormalized<unsigned char>(src->alpha);
    }

    static void writeColor(R8G8B8A8 *dst, const gl::ColorUI *src)
    {
        dst->R = static_cast<unsigned char>(src->red);
        dst->G = static_cast<unsigned char>(src->green);
        dst->B = static_cast<unsigned char>(src->blue);
        dst->A = static_cast<unsigned char>(src->alpha);
    }

    static void average(R8G8B8A8 *dst, const R8G8B8A8 *src1, const R8G8B8A8 *src2)
    {
        *(unsigned int*)dst = (((*(unsigned int*)src1 ^ *(unsigned int*)src2) & 0xFEFEFEFE) >> 1) + (*(unsigned int*)src1 & *(unsigned int*)src2);
    }
};

struct B8G8R8A8
{
    unsigned char B;
    unsigned char G;
    unsigned char R;
    unsigned char A;

    static void readColor(gl::ColorF *dst, const B8G8R8A8 *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue  = gl::normalizedToFloat(src->B);
        dst->alpha = gl::normalizedToFloat(src->A);
    }

    static void readColor(gl::ColorUI *dst, const B8G8R8A8 *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = src->B;
        dst->alpha = src->A;
    }

    static void writeColor(B8G8R8A8 *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<unsigned char>(src->red);
        dst->G = gl::floatToNormalized<unsigned char>(src->green);
        dst->B = gl::floatToNormalized<unsigned char>(src->blue);
        dst->A = gl::floatToNormalized<unsigned char>(src->alpha);
    }

    static void writeColor(B8G8R8A8 *dst, const gl::ColorUI *src)
    {
        dst->R = static_cast<unsigned char>(src->red);
        dst->G = static_cast<unsigned char>(src->green);
        dst->B = static_cast<unsigned char>(src->blue);
        dst->A = static_cast<unsigned char>(src->alpha);
    }

    static void average(B8G8R8A8 *dst, const B8G8R8A8 *src1, const B8G8R8A8 *src2)
    {
        *(unsigned int*)dst = (((*(unsigned int*)src1 ^ *(unsigned int*)src2) & 0xFEFEFEFE) >> 1) + (*(unsigned int*)src1 & *(unsigned int*)src2);
    }
};

struct B8G8R8X8
{
    unsigned char B;
    unsigned char G;
    unsigned char R;
    unsigned char X;

    static void readColor(gl::ColorF *dst, const B8G8R8X8 *src)
    {
        dst->red = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue = gl::normalizedToFloat(src->B);
        dst->alpha = 1.0f;
    }

    static void readColor(gl::ColorUI *dst, const B8G8R8X8 *src)
    {
        dst->red = src->R;
        dst->green = src->G;
        dst->blue = src->B;
        dst->alpha = 1;
    }

    static void writeColor(B8G8R8X8 *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<unsigned char>(src->red);
        dst->G = gl::floatToNormalized<unsigned char>(src->green);
        dst->B = gl::floatToNormalized<unsigned char>(src->blue);
        dst->X = 255;
    }

    static void writeColor(B8G8R8X8 *dst, const gl::ColorUI *src)
    {
        dst->R = static_cast<unsigned char>(src->red);
        dst->G = static_cast<unsigned char>(src->green);
        dst->B = static_cast<unsigned char>(src->blue);
        dst->X = 255;
    }

    static void average(B8G8R8X8 *dst, const B8G8R8X8 *src1, const B8G8R8X8 *src2)
    {
        *(unsigned int*)dst = (((*(unsigned int*)src1 ^ *(unsigned int*)src2) & 0xFEFEFEFE) >> 1) + (*(unsigned int*)src1 & *(unsigned int*)src2);
        dst->X = 255;
    }
};

struct B5G5R5A1
{
    unsigned short BGRA;

    static void readColor(gl::ColorF *dst, const B5G5R5A1 *src)
    {
        dst->alpha = gl::normalizedToFloat<1>(gl::getShiftedData<1, 15>(src->BGRA));
        dst->red   = gl::normalizedToFloat<5>(gl::getShiftedData<5, 10>(src->BGRA));
        dst->green = gl::normalizedToFloat<5>(gl::getShiftedData<5,  5>(src->BGRA));
        dst->blue  = gl::normalizedToFloat<5>(gl::getShiftedData<5,  0>(src->BGRA));
    }

    static void writeColor(B5G5R5A1 *dst, const gl::ColorF *src)
    {
        dst->BGRA = gl::shiftData<1, 15>(gl::floatToNormalized<1, unsigned short>(src->alpha)) |
                    gl::shiftData<5, 10>(gl::floatToNormalized<5, unsigned short>(src->red))   |
                    gl::shiftData<5,  5>(gl::floatToNormalized<5, unsigned short>(src->green)) |
                    gl::shiftData<5,  0>(gl::floatToNormalized<5, unsigned short>(src->blue));
    }

    static void average(B5G5R5A1 *dst, const B5G5R5A1 *src1, const B5G5R5A1 *src2)
    {
        dst->BGRA = gl::shiftData<1, 15>(gl::average(gl::getShiftedData<1, 15>(src1->BGRA), gl::getShiftedData<1, 15>(src2->BGRA))) |
                    gl::shiftData<5, 10>(gl::average(gl::getShiftedData<5, 10>(src1->BGRA), gl::getShiftedData<5, 10>(src2->BGRA))) |
                    gl::shiftData<5,  5>(gl::average(gl::getShiftedData<5,  5>(src1->BGRA), gl::getShiftedData<5,  5>(src2->BGRA))) |
                    gl::shiftData<5,  0>(gl::average(gl::getShiftedData<5,  0>(src1->BGRA), gl::getShiftedData<5,  0>(src2->BGRA)));
    }
};

struct R5G5B5A1
{
    unsigned short RGBA;

    static void readColor(gl::ColorF *dst, const R5G5B5A1 *src)
    {
        dst->alpha = gl::normalizedToFloat<1>(gl::getShiftedData<1, 15>(src->RGBA));
        dst->blue  = gl::normalizedToFloat<5>(gl::getShiftedData<5, 10>(src->RGBA));
        dst->green = gl::normalizedToFloat<5>(gl::getShiftedData<5,  5>(src->RGBA));
        dst->red   = gl::normalizedToFloat<5>(gl::getShiftedData<5,  0>(src->RGBA));
    }

    static void writeColor(R5G5B5A1 *dst, const gl::ColorF *src)
    {
        dst->RGBA = gl::shiftData<1, 15>(gl::floatToNormalized<1, unsigned short>(src->alpha)) |
                    gl::shiftData<5, 10>(gl::floatToNormalized<5, unsigned short>(src->blue))  |
                    gl::shiftData<5,  5>(gl::floatToNormalized<5, unsigned short>(src->green)) |
                    gl::shiftData<5,  0>(gl::floatToNormalized<5, unsigned short>(src->red));
    }

    static void average(R5G5B5A1 *dst, const R5G5B5A1 *src1, const R5G5B5A1 *src2)
    {
        dst->RGBA = gl::shiftData<1, 15>(gl::average(gl::getShiftedData<1, 15>(src1->RGBA), gl::getShiftedData<1, 15>(src2->RGBA))) |
                    gl::shiftData<5, 10>(gl::average(gl::getShiftedData<5, 10>(src1->RGBA), gl::getShiftedData<5, 10>(src2->RGBA))) |
                    gl::shiftData<5,  5>(gl::average(gl::getShiftedData<5,  5>(src1->RGBA), gl::getShiftedData<5,  5>(src2->RGBA))) |
                    gl::shiftData<5,  0>(gl::average(gl::getShiftedData<5,  0>(src1->RGBA), gl::getShiftedData<5,  0>(src2->RGBA)));
    }
};

struct R4G4B4A4
{
    unsigned char R : 4;
    unsigned char G : 4;
    unsigned char B : 4;
    unsigned char A : 4;

    static void readColor(gl::ColorF *dst, const R4G4B4A4 *src)
    {
        dst->red   = gl::normalizedToFloat<4>(src->R);
        dst->green = gl::normalizedToFloat<4>(src->G);
        dst->blue  = gl::normalizedToFloat<4>(src->B);
        dst->alpha = gl::normalizedToFloat<4>(src->A);
    }

    static void writeColor(R4G4B4A4 *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<4, unsigned char>(src->red);
        dst->G = gl::floatToNormalized<4, unsigned char>(src->green);
        dst->B = gl::floatToNormalized<4, unsigned char>(src->blue);
        dst->A = gl::floatToNormalized<4, unsigned char>(src->alpha);
    }

    static void average(R4G4B4A4 *dst, const R4G4B4A4 *src1, const R4G4B4A4 *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
        dst->B = gl::average(src1->B, src2->B);
        dst->A = gl::average(src1->A, src2->A);
    }
};

struct A4R4G4B4
{
    unsigned char A : 4;
    unsigned char R : 4;
    unsigned char G : 4;
    unsigned char B : 4;

    static void readColor(gl::ColorF *dst, const A4R4G4B4 *src)
    {
        dst->red   = gl::normalizedToFloat<4>(src->R);
        dst->green = gl::normalizedToFloat<4>(src->G);
        dst->blue  = gl::normalizedToFloat<4>(src->B);
        dst->alpha = gl::normalizedToFloat<4>(src->A);
    }

    static void writeColor(A4R4G4B4 *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<4, unsigned char>(src->red);
        dst->G = gl::floatToNormalized<4, unsigned char>(src->green);
        dst->B = gl::floatToNormalized<4, unsigned char>(src->blue);
        dst->A = gl::floatToNormalized<4, unsigned char>(src->alpha);
    }

    static void average(A4R4G4B4 *dst, const A4R4G4B4 *src1, const A4R4G4B4 *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
        dst->B = gl::average(src1->B, src2->B);
        dst->A = gl::average(src1->A, src2->A);
    }
};

struct B4G4R4A4
{
    unsigned char B : 4;
    unsigned char G : 4;
    unsigned char R : 4;
    unsigned char A : 4;

    static void readColor(gl::ColorF *dst, const B4G4R4A4 *src)
    {
        dst->red   = gl::normalizedToFloat<4>(src->R);
        dst->green = gl::normalizedToFloat<4>(src->G);
        dst->blue  = gl::normalizedToFloat<4>(src->B);
        dst->alpha = gl::normalizedToFloat<4>(src->A);
    }

    static void writeColor(B4G4R4A4 *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<4, unsigned char>(src->red);
        dst->G = gl::floatToNormalized<4, unsigned char>(src->green);
        dst->B = gl::floatToNormalized<4, unsigned char>(src->blue);
        dst->A = gl::floatToNormalized<4, unsigned char>(src->alpha);
    }

    static void average(B4G4R4A4 *dst, const B4G4R4A4 *src1, const B4G4R4A4 *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
        dst->B = gl::average(src1->B, src2->B);
        dst->A = gl::average(src1->A, src2->A);
    }
};

struct R16
{
    unsigned short R;

    static void readColor(gl::ColorF *dst, const R16 *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = 0.0f;
        dst->blue  = 0.0f;
        dst->alpha = 1.0f;
    }

    static void readColor(gl::ColorUI *dst, const R16 *src)
    {
        dst->red   = src->R;
        dst->green = 0;
        dst->blue  = 0;
        dst->alpha = 1;
    }

    static void writeColor(R16 *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<unsigned short>(src->red);
    }

    static void writeColor(R16 *dst, const gl::ColorUI *src)
    {
        dst->R = static_cast<unsigned short>(src->red);
    }

    static void average(R16 *dst, const R16 *src1, const R16 *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
    }
};

struct R16G16
{
    unsigned short R;
    unsigned short G;

    static void readColor(gl::ColorF *dst, const R16G16 *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue  = 0.0f;
        dst->alpha = 1.0f;
    }

    static void readColor(gl::ColorUI *dst, const R16G16 *src)
    {
        dst->red = src->R;
        dst->green = src->G;
        dst->blue  = 0;
        dst->alpha = 1;
    }

    static void writeColor(R16G16 *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<unsigned short>(src->red);
        dst->G = gl::floatToNormalized<unsigned short>(src->green);
    }

    static void writeColor(R16G16 *dst, const gl::ColorUI *src)
    {
        dst->R = static_cast<unsigned short>(src->red);
        dst->G = static_cast<unsigned short>(src->green);
    }

    static void average(R16G16 *dst, const R16G16 *src1, const R16G16 *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
    }
};

struct R16G16B16
{
    unsigned short R;
    unsigned short G;
    unsigned short B;

    static void readColor(gl::ColorF *dst, const R16G16B16 *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue  = gl::normalizedToFloat(src->B);
        dst->alpha = 1.0f;
    }

    static void readColor(gl::ColorUI *dst, const R16G16B16 *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = src->B;
        dst->alpha = 1;
    }

    static void writeColor(R16G16B16 *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<unsigned short>(src->red);
        dst->G = gl::floatToNormalized<unsigned short>(src->green);
        dst->B = gl::floatToNormalized<unsigned short>(src->blue);
    }

    static void writeColor(R16G16B16 *dst, const gl::ColorUI *src)
    {
        dst->R = static_cast<unsigned short>(src->red);
        dst->G = static_cast<unsigned short>(src->green);
        dst->B = static_cast<unsigned short>(src->blue);
    }

    static void average(R16G16B16 *dst, const R16G16B16 *src1, const R16G16B16 *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
        dst->B = gl::average(src1->B, src2->B);
    }
};

struct R16G16B16A16
{
    unsigned short R;
    unsigned short G;
    unsigned short B;
    unsigned short A;

    static void readColor(gl::ColorF *dst, const R16G16B16A16 *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue  = gl::normalizedToFloat(src->B);
        dst->alpha = gl::normalizedToFloat(src->A);
    }

    static void readColor(gl::ColorUI *dst, const R16G16B16A16 *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = src->B;
        dst->alpha = src->A;
    }

    static void writeColor(R16G16B16A16 *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<unsigned short>(src->red);
        dst->G = gl::floatToNormalized<unsigned short>(src->green);
        dst->B = gl::floatToNormalized<unsigned short>(src->blue);
        dst->A = gl::floatToNormalized<unsigned short>(src->alpha);
    }

    static void writeColor(R16G16B16A16 *dst, const gl::ColorUI *src)
    {
        dst->R = static_cast<unsigned short>(src->red);
        dst->G = static_cast<unsigned short>(src->green);
        dst->B = static_cast<unsigned short>(src->blue);
        dst->A = static_cast<unsigned short>(src->alpha);
    }

    static void average(R16G16B16A16 *dst, const R16G16B16A16 *src1, const R16G16B16A16 *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
        dst->B = gl::average(src1->B, src2->B);
        dst->A = gl::average(src1->A, src2->A);
    }
};

struct R32
{
    unsigned int R;

    static void readColor(gl::ColorF *dst, const R32 *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = 0.0f;
        dst->blue  = 0.0f;
        dst->alpha = 1.0f;
    }

    static void readColor(gl::ColorUI *dst, const R32 *src)
    {
        dst->red   = src->R;
        dst->green = 0;
        dst->blue  = 0;
        dst->alpha = 1;
    }

    static void writeColor(R32 *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<unsigned int>(src->red);
    }

    static void writeColor(R32 *dst, const gl::ColorUI *src)
    {
        dst->R = static_cast<unsigned int>(src->red);
    }

    static void average(R32 *dst, const R32 *src1, const R32 *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
    }
};

struct R32G32
{
    unsigned int R;
    unsigned int G;

    static void readColor(gl::ColorF *dst, const R32G32 *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue  = 0.0f;
        dst->alpha = 1.0f;
    }

    static void readColor(gl::ColorUI *dst, const R32G32 *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = 0;
        dst->alpha = 1;
    }

    static void writeColor(R32G32 *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<unsigned int>(src->red);
        dst->G = gl::floatToNormalized<unsigned int>(src->green);
    }

    static void writeColor(R32G32 *dst, const gl::ColorUI *src)
    {
        dst->R = static_cast<unsigned int>(src->red);
        dst->G = static_cast<unsigned int>(src->green);
    }

    static void average(R32G32 *dst, const R32G32 *src1, const R32G32 *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
    }
};

struct R32G32B32
{
    unsigned int R;
    unsigned int G;
    unsigned int B;

    static void readColor(gl::ColorF *dst, const R32G32B32 *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue  = gl::normalizedToFloat(src->B);
        dst->alpha = 1.0f;
    }

    static void readColor(gl::ColorUI *dst, const R32G32B32 *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = src->B;
        dst->alpha = 1;
    }

    static void writeColor(R32G32B32 *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<unsigned int>(src->red);
        dst->G = gl::floatToNormalized<unsigned int>(src->green);
        dst->B = gl::floatToNormalized<unsigned int>(src->blue);
    }

    static void writeColor(R32G32B32 *dst, const gl::ColorUI *src)
    {
        dst->R = static_cast<unsigned int>(src->red);
        dst->G = static_cast<unsigned int>(src->green);
        dst->B = static_cast<unsigned int>(src->blue);
    }

    static void average(R32G32B32 *dst, const R32G32B32 *src1, const R32G32B32 *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
        dst->B = gl::average(src1->B, src2->B);
    }
};

struct R32G32B32A32
{
    unsigned int R;
    unsigned int G;
    unsigned int B;
    unsigned int A;

    static void readColor(gl::ColorF *dst, const R32G32B32A32 *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue  = gl::normalizedToFloat(src->B);
        dst->alpha = gl::normalizedToFloat(src->A);
    }

    static void readColor(gl::ColorUI *dst, const R32G32B32A32 *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = src->B;
        dst->alpha = src->A;
    }

    static void writeColor(R32G32B32A32 *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<unsigned int>(src->red);
        dst->G = gl::floatToNormalized<unsigned int>(src->green);
        dst->B = gl::floatToNormalized<unsigned int>(src->blue);
        dst->A = gl::floatToNormalized<unsigned int>(src->alpha);
    }

    static void writeColor(R32G32B32A32 *dst, const gl::ColorUI *src)
    {
        dst->R = static_cast<unsigned int>(src->red);
        dst->G = static_cast<unsigned int>(src->green);
        dst->B = static_cast<unsigned int>(src->blue);
        dst->A = static_cast<unsigned int>(src->alpha);
    }

    static void average(R32G32B32A32 *dst, const R32G32B32A32 *src1, const R32G32B32A32 *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
        dst->B = gl::average(src1->B, src2->B);
        dst->A = gl::average(src1->A, src2->A);
    }
};

struct R8S
{
    char R;

    static void readColor(gl::ColorF *dst, const R8S *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = 0.0f;
        dst->blue  = 0.0f;
        dst->alpha = 1.0f;
    }

    static void readColor(gl::ColorI *dst, const R8S *src)
    {
        dst->red   = src->R;
        dst->green = 0;
        dst->blue  = 0;
        dst->alpha = 1;
    }

    static void writeColor(R8S *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<char>(src->red);
    }

    static void writeColor(R8S *dst, const gl::ColorI *src)
    {
        dst->R = static_cast<char>(src->red);
    }

    static void average(R8S *dst, const R8S *src1, const R8S *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
    }
};

struct R8G8S
{
    char R;
    char G;

    static void readColor(gl::ColorF *dst, const R8G8S *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue  = 0.0f;
        dst->alpha = 1.0f;
    }

    static void readColor(gl::ColorI *dst, const R8G8S *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = 0;
        dst->alpha = 1;
    }

    static void writeColor(R8G8S *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<char>(src->red);
        dst->G = gl::floatToNormalized<char>(src->green);
    }

    static void writeColor(R8G8S *dst, const gl::ColorI *src)
    {
        dst->R = static_cast<char>(src->red);
        dst->G = static_cast<char>(src->green);
    }

    static void average(R8G8S *dst, const R8G8S *src1, const R8G8S *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
    }
};

struct R8G8B8S
{
    char R;
    char G;
    char B;

    static void readColor(gl::ColorF *dst, const R8G8B8S *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue  = gl::normalizedToFloat(src->B);
        dst->alpha = 1.0f;
    }

    static void readColor(gl::ColorI *dst, const R8G8B8S *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = src->B;
        dst->alpha = 1;
    }

    static void writeColor(R8G8B8S *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<char>(src->red);
        dst->G = gl::floatToNormalized<char>(src->green);
        dst->B = gl::floatToNormalized<char>(src->blue);
    }

    static void writeColor(R8G8B8S *dst, const gl::ColorI *src)
    {
        dst->R = static_cast<char>(src->red);
        dst->G = static_cast<char>(src->green);
        dst->B = static_cast<char>(src->blue);
    }

    static void average(R8G8B8S *dst, const R8G8B8S *src1, const R8G8B8S *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
        dst->B = gl::average(src1->B, src2->B);
    }
};

struct R8G8B8A8S
{
    char R;
    char G;
    char B;
    char A;

    static void readColor(gl::ColorF *dst, const R8G8B8A8S *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue  = gl::normalizedToFloat(src->B);
        dst->alpha = gl::normalizedToFloat(src->A);
    }

    static void readColor(gl::ColorI *dst, const R8G8B8A8S *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = src->B;
        dst->alpha = src->A;
    }

    static void writeColor(R8G8B8A8S *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<char>(src->red);
        dst->G = gl::floatToNormalized<char>(src->green);
        dst->B = gl::floatToNormalized<char>(src->blue);
        dst->A = gl::floatToNormalized<char>(src->alpha);
    }

    static void writeColor(R8G8B8A8S *dst, const gl::ColorI *src)
    {
        dst->R = static_cast<char>(src->red);
        dst->G = static_cast<char>(src->green);
        dst->B = static_cast<char>(src->blue);
        dst->A = static_cast<char>(src->alpha);
    }

    static void average(R8G8B8A8S *dst, const R8G8B8A8S *src1, const R8G8B8A8S *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
        dst->B = gl::average(src1->B, src2->B);
        dst->A = gl::average(src1->A, src2->A);
    }
};

struct R16S
{
    short R;

    static void readColor(gl::ColorF *dst, const R16S *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = 0.0f;
        dst->blue  = 0.0f;
        dst->alpha = 1.0f;
    }

    static void readColor(gl::ColorI *dst, const R16S *src)
    {
        dst->red   = src->R;
        dst->green = 0;
        dst->blue  = 0;
        dst->alpha = 1;
    }

    static void writeColor(R16S *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<short>(src->red);
    }

    static void writeColor(R16S *dst, const gl::ColorI *src)
    {
        dst->R = static_cast<short>(src->red);
    }

    static void average(R16S *dst, const R16S *src1, const R16S *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
    }
};

struct R16G16S
{
    short R;
    short G;

    static void readColor(gl::ColorF *dst, const R16G16S *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue  = 0.0f;
        dst->alpha = 1.0f;
    }

    static void readColor(gl::ColorI *dst, const R16G16S *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = 0;
        dst->alpha = 1;
    }

    static void writeColor(R16G16S *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<short>(src->red);
        dst->G = gl::floatToNormalized<short>(src->green);
    }

    static void writeColor(R16G16S *dst, const gl::ColorI *src)
    {
        dst->R = static_cast<short>(src->red);
        dst->G = static_cast<short>(src->green);
    }

    static void average(R16G16S *dst, const R16G16S *src1, const R16G16S *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
    }
};

struct R16G16B16S
{
    short R;
    short G;
    short B;

    static void readColor(gl::ColorF *dst, const R16G16B16S *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue  = gl::normalizedToFloat(src->B);
        dst->alpha = 1.0f;
    }

    static void readColor(gl::ColorI *dst, const R16G16B16S *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = src->B;
        dst->alpha = 1;
    }

    static void writeColor(R16G16B16S *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<short>(src->red);
        dst->G = gl::floatToNormalized<short>(src->green);
        dst->B = gl::floatToNormalized<short>(src->blue);
    }

    static void writeColor(R16G16B16S *dst, const gl::ColorI *src)
    {
        dst->R = static_cast<short>(src->red);
        dst->G = static_cast<short>(src->green);
        dst->B = static_cast<short>(src->blue);
    }

    static void average(R16G16B16S *dst, const R16G16B16S *src1, const R16G16B16S *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
        dst->B = gl::average(src1->B, src2->B);
    }
};

struct R16G16B16A16S
{
    short R;
    short G;
    short B;
    short A;

    static void readColor(gl::ColorF *dst, const R16G16B16A16S *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue  = gl::normalizedToFloat(src->B);
        dst->alpha = gl::normalizedToFloat(src->A);
    }

    static void readColor(gl::ColorI *dst, const R16G16B16A16S *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = src->B;
        dst->alpha = src->A;
    }

    static void writeColor(R16G16B16A16S *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<short>(src->red);
        dst->G = gl::floatToNormalized<short>(src->green);
        dst->B = gl::floatToNormalized<short>(src->blue);
        dst->A = gl::floatToNormalized<short>(src->alpha);
    }

    static void writeColor(R16G16B16A16S *dst, const gl::ColorI *src)
    {
        dst->R = static_cast<short>(src->red);
        dst->G = static_cast<short>(src->green);
        dst->B = static_cast<short>(src->blue);
        dst->A = static_cast<short>(src->alpha);
    }

    static void average(R16G16B16A16S *dst, const R16G16B16A16S *src1, const R16G16B16A16S *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
        dst->B = gl::average(src1->B, src2->B);
        dst->A = gl::average(src1->A, src2->A);
    }
};

struct R32S
{
    int R;

    static void readColor(gl::ColorF *dst, const R32S *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = 0.0f;
        dst->blue  = 0.0f;
        dst->alpha = 1.0f;
    }

    static void readColor(gl::ColorI *dst, const R32S *src)
    {
        dst->red   = src->R;
        dst->green = 0;
        dst->blue  = 0;
        dst->alpha = 1;
    }

    static void writeColor(R32S *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<int>(src->red);
    }

    static void writeColor(R32S *dst, const gl::ColorI *src)
    {
        dst->R = static_cast<int>(src->red);
    }

    static void average(R32S *dst, const R32S *src1, const R32S *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
    }
};

struct R32G32S
{
    int R;
    int G;

    static void readColor(gl::ColorF *dst, const R32G32S *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue  = 0.0f;
        dst->alpha = 1.0f;
    }

    static void readColor(gl::ColorI *dst, const R32G32S *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = 0;
        dst->alpha = 1;
    }

    static void writeColor(R32G32S *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<int>(src->red);
        dst->G = gl::floatToNormalized<int>(src->green);
    }

    static void writeColor(R32G32S *dst, const gl::ColorI *src)
    {
        dst->R = static_cast<int>(src->red);
        dst->G = static_cast<int>(src->green);
    }

    static void average(R32G32S *dst, const R32G32S *src1, const R32G32S *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
    }
};

struct R32G32B32S
{
    int R;
    int G;
    int B;

    static void readColor(gl::ColorF *dst, const R32G32B32S *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue  = gl::normalizedToFloat(src->B);
        dst->alpha = 1.0f;
    }

    static void readColor(gl::ColorI *dst, const R32G32B32S *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = src->B;
        dst->alpha = 1;
    }

    static void writeColor(R32G32B32S *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<int>(src->red);
        dst->G = gl::floatToNormalized<int>(src->green);
        dst->B = gl::floatToNormalized<int>(src->blue);
    }

    static void writeColor(R32G32B32S *dst, const gl::ColorI *src)
    {
        dst->R = static_cast<int>(src->red);
        dst->G = static_cast<int>(src->green);
        dst->B = static_cast<int>(src->blue);
    }

    static void average(R32G32B32S *dst, const R32G32B32S *src1, const R32G32B32S *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
        dst->B = gl::average(src1->B, src2->B);
    }
};

struct R32G32B32A32S
{
    int R;
    int G;
    int B;
    int A;

    static void readColor(gl::ColorF *dst, const R32G32B32A32S *src)
    {
        dst->red   = gl::normalizedToFloat(src->R);
        dst->green = gl::normalizedToFloat(src->G);
        dst->blue  = gl::normalizedToFloat(src->B);
        dst->alpha = gl::normalizedToFloat(src->A);
    }

    static void readColor(gl::ColorI *dst, const R32G32B32A32S *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = src->B;
        dst->alpha = src->A;
    }

    static void writeColor(R32G32B32A32S *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<int>(src->red);
        dst->G = gl::floatToNormalized<int>(src->green);
        dst->B = gl::floatToNormalized<int>(src->blue);
        dst->A = gl::floatToNormalized<int>(src->alpha);
    }

    static void writeColor(R32G32B32A32S *dst, const gl::ColorI *src)
    {
        dst->R = static_cast<int>(src->red);
        dst->G = static_cast<int>(src->green);
        dst->B = static_cast<int>(src->blue);
        dst->A = static_cast<int>(src->alpha);
    }

    static void average(R32G32B32A32S *dst, const R32G32B32A32S *src1, const R32G32B32A32S *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
        dst->B = gl::average(src1->B, src2->B);
        dst->A = gl::average(src1->A, src2->A);
    }
};

struct A16B16G16R16F
{
    unsigned short A;
    unsigned short R;
    unsigned short G;
    unsigned short B;

    static void readColor(gl::ColorF *dst, const A16B16G16R16F *src)
    {
        dst->red   = gl::float16ToFloat32(src->R);
        dst->green = gl::float16ToFloat32(src->G);
        dst->blue  = gl::float16ToFloat32(src->B);
        dst->alpha = gl::float16ToFloat32(src->A);
    }

    static void writeColor(A16B16G16R16F *dst, const gl::ColorF *src)
    {
        dst->R = gl::float32ToFloat16(src->red);
        dst->G = gl::float32ToFloat16(src->green);
        dst->B = gl::float32ToFloat16(src->blue);
        dst->A = gl::float32ToFloat16(src->alpha);
    }

    static void average(A16B16G16R16F *dst, const A16B16G16R16F *src1, const A16B16G16R16F *src2)
    {
        dst->R = gl::averageHalfFloat(src1->R, src2->R);
        dst->G = gl::averageHalfFloat(src1->G, src2->G);
        dst->B = gl::averageHalfFloat(src1->B, src2->B);
        dst->A = gl::averageHalfFloat(src1->A, src2->A);
    }
};

struct R16G16B16A16F
{
    unsigned short R;
    unsigned short G;
    unsigned short B;
    unsigned short A;

    static void readColor(gl::ColorF *dst, const R16G16B16A16F *src)
    {
        dst->red   = gl::float16ToFloat32(src->R);
        dst->green = gl::float16ToFloat32(src->G);
        dst->blue  = gl::float16ToFloat32(src->B);
        dst->alpha = gl::float16ToFloat32(src->A);
    }

    static void writeColor(R16G16B16A16F *dst, const gl::ColorF *src)
    {
        dst->R = gl::float32ToFloat16(src->red);
        dst->G = gl::float32ToFloat16(src->green);
        dst->B = gl::float32ToFloat16(src->blue);
        dst->A = gl::float32ToFloat16(src->alpha);
    }

    static void average(R16G16B16A16F *dst, const R16G16B16A16F *src1, const R16G16B16A16F *src2)
    {
        dst->R = gl::averageHalfFloat(src1->R, src2->R);
        dst->G = gl::averageHalfFloat(src1->G, src2->G);
        dst->B = gl::averageHalfFloat(src1->B, src2->B);
        dst->A = gl::averageHalfFloat(src1->A, src2->A);
    }
};

struct R16F
{
    unsigned short R;

    static void readColor(gl::ColorF *dst, const R16F *src)
    {
        dst->red   = gl::float16ToFloat32(src->R);
        dst->green = 0.0f;
        dst->blue  = 0.0f;
        dst->alpha = 1.0f;
    }

    static void writeColor(R16F *dst, const gl::ColorF *src)
    {
        dst->R = gl::float32ToFloat16(src->red);
    }
    
    static void average(R16F *dst, const R16F *src1, const R16F *src2)
    {
        dst->R = gl::averageHalfFloat(src1->R, src2->R);
    }
};

struct A16F
{
    unsigned short A;

    static void readColor(gl::ColorF *dst, const A16F *src)
    {
        dst->red   = 0.0f;
        dst->green = 0.0f;
        dst->blue  = 0.0f;
        dst->alpha = gl::float16ToFloat32(src->A);
    }

    static void writeColor(A16F *dst, const gl::ColorF *src)
    {
        dst->A = gl::float32ToFloat16(src->alpha);
    }

    static void average(A16F *dst, const A16F *src1, const A16F *src2)
    {
        dst->A = gl::averageHalfFloat(src1->A, src2->A);
    }
};

struct L16F
{
    unsigned short L;

    static void readColor(gl::ColorF *dst, const L16F *src)
    {
        float lum = gl::float16ToFloat32(src->L);
        dst->red   = lum;
        dst->green = lum;
        dst->blue  = lum;
        dst->alpha = 1.0f;
    }

    static void writeColor(L16F *dst, const gl::ColorF *src)
    {
        dst->L = gl::float32ToFloat16((src->red + src->green + src->blue) / 3.0f);
    }

    static void average(L16F *dst, const L16F *src1, const L16F *src2)
    {
        dst->L = gl::averageHalfFloat(src1->L, src2->L);
    }
};

struct L16A16F
{
    unsigned short L;
    unsigned short A;

    static void readColor(gl::ColorF *dst, const L16A16F *src)
    {
        float lum = gl::float16ToFloat32(src->L);
        dst->red   = lum;
        dst->green = lum;
        dst->blue  = lum;
        dst->alpha = gl::float16ToFloat32(src->A);
    }

    static void writeColor(L16A16F *dst, const gl::ColorF *src)
    {
        dst->L = gl::float32ToFloat16((src->red + src->green + src->blue) / 3.0f);
        dst->A = gl::float32ToFloat16(src->alpha);
    }

    static void average(L16A16F *dst, const L16A16F *src1, const L16A16F *src2)
    {
        dst->L = gl::averageHalfFloat(src1->L, src2->L);
        dst->A = gl::averageHalfFloat(src1->A, src2->A);
    }
};

struct R16G16F
{
    unsigned short R;
    unsigned short G;

    static void readColor(gl::ColorF *dst, const R16G16F *src)
    {
        dst->red   = gl::float16ToFloat32(src->R);
        dst->green = gl::float16ToFloat32(src->G);
        dst->blue  = 0.0f;
        dst->alpha = 1.0f;
    }

    static void writeColor(R16G16F *dst, const gl::ColorF *src)
    {
        dst->R = gl::float32ToFloat16(src->red);
        dst->G = gl::float32ToFloat16(src->green);
    }

    static void average(R16G16F *dst, const R16G16F *src1, const R16G16F *src2)
    {
        dst->R = gl::averageHalfFloat(src1->R, src2->R);
        dst->G = gl::averageHalfFloat(src1->G, src2->G);
    }
};

struct R16G16B16F
{
    unsigned short R;
    unsigned short G;
    unsigned short B;

    static void readColor(gl::ColorF *dst, const R16G16B16F *src)
    {
        dst->red   = gl::float16ToFloat32(src->R);
        dst->green = gl::float16ToFloat32(src->G);
        dst->blue  = gl::float16ToFloat32(src->B);
        dst->alpha = 1.0f;
    }

    static void writeColor(R16G16B16F *dst, const gl::ColorF *src)
    {
        dst->R = gl::float32ToFloat16(src->red);
        dst->G = gl::float32ToFloat16(src->green);
        dst->B = gl::float32ToFloat16(src->blue);
    }

    static void average(R16G16B16F *dst, const R16G16B16F *src1, const R16G16B16F *src2)
    {
        dst->R = gl::averageHalfFloat(src1->R, src2->R);
        dst->G = gl::averageHalfFloat(src1->G, src2->G);
        dst->B = gl::averageHalfFloat(src1->B, src2->B);
    }
};

struct A32B32G32R32F
{
    float A;
    float R;
    float G;
    float B;

    static void readColor(gl::ColorF *dst, const A32B32G32R32F *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = src->B;
        dst->alpha = src->A;
    }

    static void writeColor(A32B32G32R32F *dst, const gl::ColorF *src)
    {
        dst->R = src->red;
        dst->G = src->green;
        dst->B = src->blue;
        dst->A = src->alpha;
    }

    static void average(A32B32G32R32F *dst, const A32B32G32R32F *src1, const A32B32G32R32F *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
        dst->B = gl::average(src1->B, src2->B);
        dst->A = gl::average(src1->A, src2->A);
    }
};

struct R32G32B32A32F
{
    float R;
    float G;
    float B;
    float A;

    static void readColor(gl::ColorF *dst, const R32G32B32A32F *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = src->B;
        dst->alpha = src->A;
    }

    static void writeColor(R32G32B32A32F *dst, const gl::ColorF *src)
    {
        dst->R = src->red;
        dst->G = src->green;
        dst->B = src->blue;
        dst->A = src->alpha;
    }

    static void average(R32G32B32A32F *dst, const R32G32B32A32F *src1, const R32G32B32A32F *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
        dst->B = gl::average(src1->B, src2->B);
        dst->A = gl::average(src1->A, src2->A);
    }
};

struct R32F
{
    float R;

    static void readColor(gl::ColorF *dst, const R32F *src)
    {
        dst->red   = src->R;
        dst->green = 0.0f;
        dst->blue  = 0.0f;
        dst->alpha = 1.0f;
    }

    static void writeColor(R32F *dst, const gl::ColorF *src)
    {
        dst->R = src->red;
    }

    static void average(R32F *dst, const R32F *src1, const R32F *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
    }
};

struct A32F
{
    float A;

    static void readColor(gl::ColorF *dst, const A32F *src)
    {
        dst->red   = 0.0f;
        dst->green = 0.0f;
        dst->blue  = 0.0f;
        dst->alpha = src->A;
    }

    static void writeColor(A32F *dst, const gl::ColorF *src)
    {
        dst->A = src->alpha;
    }

    static void average(A32F *dst, const A32F *src1, const A32F *src2)
    {
        dst->A = gl::average(src1->A, src2->A);
    }
};

struct L32F
{
    float L;

    static void readColor(gl::ColorF *dst, const L32F *src)
    {
        dst->red   = src->L;
        dst->green = src->L;
        dst->blue  = src->L;
        dst->alpha = 1.0f;
    }

    static void writeColor(L32F *dst, const gl::ColorF *src)
    {
        dst->L = (src->red + src->green + src->blue) / 3.0f;
    }

    static void average(L32F *dst, const L32F *src1, const L32F *src2)
    {
        dst->L = gl::average(src1->L, src2->L);
    }
};

struct L32A32F
{
    float L;
    float A;

    static void readColor(gl::ColorF *dst, const L32A32F *src)
    {
        dst->red   = src->L;
        dst->green = src->L;
        dst->blue  = src->L;
        dst->alpha = src->A;
    }

    static void writeColor(L32A32F *dst, const gl::ColorF *src)
    {
        dst->L = (src->red + src->green + src->blue) / 3.0f;
        dst->A = src->alpha;
    }

    static void average(L32A32F *dst, const L32A32F *src1, const L32A32F *src2)
    {
        dst->L = gl::average(src1->L, src2->L);
        dst->A = gl::average(src1->A, src2->A);
    }
};

struct R32G32F
{
    float R;
    float G;

    static void readColor(gl::ColorF *dst, const R32G32F *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = 0.0f;
        dst->alpha = 1.0f;
    }

    static void writeColor(R32G32F *dst, const gl::ColorF *src)
    {
        dst->R = src->red;
        dst->G = src->green;
    }

    static void average(R32G32F *dst, const R32G32F *src1, const R32G32F *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
    }
};

struct R32G32B32F
{
    float R;
    float G;
    float B;

    static void readColor(gl::ColorF *dst, const R32G32B32F *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = src->B;
        dst->alpha = 1.0f;
    }

    static void writeColor(R32G32B32F *dst, const gl::ColorF *src)
    {
        dst->R = src->red;
        dst->G = src->green;
        dst->B = src->blue;
    }

    static void average(R32G32B32F *dst, const R32G32B32F *src1, const R32G32B32F *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
        dst->B = gl::average(src1->B, src2->B);
    }
};

struct R10G10B10A2
{
    unsigned int R : 10;
    unsigned int G : 10;
    unsigned int B : 10;
    unsigned int A : 2;

    static void readColor(gl::ColorF *dst, const R10G10B10A2 *src)
    {
        dst->red   = gl::normalizedToFloat<10>(src->R);
        dst->green = gl::normalizedToFloat<10>(src->G);
        dst->blue  = gl::normalizedToFloat<10>(src->B);
        dst->alpha = gl::normalizedToFloat< 2>(src->A);
    }

    static void readColor(gl::ColorUI *dst, const R10G10B10A2 *src)
    {
        dst->red   = src->R;
        dst->green = src->G;
        dst->blue  = src->B;
        dst->alpha = src->A;
    }

    static void writeColor(R10G10B10A2 *dst, const gl::ColorF *src)
    {
        dst->R = gl::floatToNormalized<10, unsigned int>(src->red);
        dst->G = gl::floatToNormalized<10, unsigned int>(src->green);
        dst->B = gl::floatToNormalized<10, unsigned int>(src->blue);
        dst->A = gl::floatToNormalized< 2, unsigned int>(src->alpha);
    }

    static void writeColor(R10G10B10A2 *dst, const gl::ColorUI *src)
    {
        dst->R = static_cast<unsigned int>(src->red);
        dst->G = static_cast<unsigned int>(src->green);
        dst->B = static_cast<unsigned int>(src->blue);
        dst->A = static_cast<unsigned int>(src->alpha);
    }

    static void average(R10G10B10A2 *dst, const R10G10B10A2 *src1, const R10G10B10A2 *src2)
    {
        dst->R = gl::average(src1->R, src2->R);
        dst->G = gl::average(src1->G, src2->G);
        dst->B = gl::average(src1->B, src2->B);
        dst->A = gl::average(src1->A, src2->A);
    }
};

struct R9G9B9E5
{
    unsigned int R : 9;
    unsigned int G : 9;
    unsigned int B : 9;
    unsigned int E : 5;

    static void readColor(gl::ColorF *dst, const R9G9B9E5 *src)
    {
        gl::convert999E5toRGBFloats(gl::bitCast<unsigned int>(*src), &dst->red, &dst->green, &dst->blue);
        dst->alpha = 1.0f;
    }

    static void writeColor(R9G9B9E5 *dst, const gl::ColorF *src)
    {
        *reinterpret_cast<unsigned int*>(dst) = gl::convertRGBFloatsTo999E5(src->red,
                                                                            src->green,
                                                                            src->blue);
    }

    static void average(R9G9B9E5 *dst, const R9G9B9E5 *src1, const R9G9B9E5 *src2)
    {
        float r1, g1, b1;
        gl::convert999E5toRGBFloats(*reinterpret_cast<const unsigned int*>(src1), &r1, &g1, &b1);

        float r2, g2, b2;
        gl::convert999E5toRGBFloats(*reinterpret_cast<const unsigned int*>(src2), &r2, &g2, &b2);

        *reinterpret_cast<unsigned int*>(dst) = gl::convertRGBFloatsTo999E5(gl::average(r1, r2),
                                                                            gl::average(g1, g2),
                                                                            gl::average(b1, b2));
    }
};

struct R11G11B10F
{
    unsigned int R : 11;
    unsigned int G : 11;
    unsigned int B : 10;

    static void readColor(gl::ColorF *dst, const R11G11B10F *src)
    {
        dst->red   = gl::float11ToFloat32(src->R);
        dst->green = gl::float11ToFloat32(src->G);
        dst->blue  = gl::float10ToFloat32(src->B);
        dst->alpha = 1.0f;
    }

    static void writeColor(R11G11B10F *dst, const gl::ColorF *src)
    {
        dst->R = gl::float32ToFloat11(src->red);
        dst->G = gl::float32ToFloat11(src->green);
        dst->B = gl::float32ToFloat10(src->blue);
    }

    static void average(R11G11B10F *dst, const R11G11B10F *src1, const R11G11B10F *src2)
    {
        dst->R = gl::averageFloat11(src1->R, src2->R);
        dst->G = gl::averageFloat11(src1->G, src2->G);
        dst->B = gl::averageFloat10(src1->B, src2->B);
    }
};

}

#endif // LIBGLESV2_RENDERER_IMAGEFORMATS_H_
