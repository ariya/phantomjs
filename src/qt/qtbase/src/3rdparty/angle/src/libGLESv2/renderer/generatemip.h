//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// generatemip.h: Defines the GenerateMip function, templated on the format
// type of the image for which mip levels are being generated.

#ifndef LIBGLESV2_RENDERER_GENERATEMIP_H_
#define LIBGLESV2_RENDERER_GENERATEMIP_H_

#include "libGLESv2/mathutil.h"

namespace rx
{
struct L8
{
    unsigned char L;

    static void average(L8 *dst, const L8 *src1, const L8 *src2)
    {
        dst->L = ((src1->L ^ src2->L) >> 1) + (src1->L & src2->L);
    }
};

typedef L8 R8; // R8 type is functionally equivalent for mip purposes
typedef L8 A8; // A8 type is functionally equivalent for mip purposes

struct A8L8
{
    unsigned char L;
    unsigned char A;

    static void average(A8L8 *dst, const A8L8 *src1, const A8L8 *src2)
    {
        *(unsigned short*)dst = (((*(unsigned short*)src1 ^ *(unsigned short*)src2) & 0xFEFE) >> 1) + (*(unsigned short*)src1 & *(unsigned short*)src2);
    }
};

typedef A8L8 R8G8; // R8G8 type is functionally equivalent for mip purposes

struct A8R8G8B8
{
    unsigned char B;
    unsigned char G;
    unsigned char R;
    unsigned char A;

    static void average(A8R8G8B8 *dst, const A8R8G8B8 *src1, const A8R8G8B8 *src2)
    {
        *(unsigned int*)dst = (((*(unsigned int*)src1 ^ *(unsigned int*)src2) & 0xFEFEFEFE) >> 1) + (*(unsigned int*)src1 & *(unsigned int*)src2);
    }
};

typedef A8R8G8B8 R8G8B8A8; // R8G8B8A8 type is functionally equivalent for mip purposes

struct A16B16G16R16F
{
    unsigned short R;
    unsigned short G;
    unsigned short B;
    unsigned short A;

    static void average(A16B16G16R16F *dst, const A16B16G16R16F *src1, const A16B16G16R16F *src2)
    {
        dst->R = gl::float32ToFloat16((gl::float16ToFloat32(src1->R) + gl::float16ToFloat32(src2->R)) * 0.5f);
        dst->G = gl::float32ToFloat16((gl::float16ToFloat32(src1->G) + gl::float16ToFloat32(src2->G)) * 0.5f);
        dst->B = gl::float32ToFloat16((gl::float16ToFloat32(src1->B) + gl::float16ToFloat32(src2->B)) * 0.5f);
        dst->A = gl::float32ToFloat16((gl::float16ToFloat32(src1->A) + gl::float16ToFloat32(src2->A)) * 0.5f);
    }
};

struct R16F
{
    unsigned short R;

    static void average(R16F *dst, const R16F *src1, const R16F *src2)
    {
        dst->R = gl::float32ToFloat16((gl::float16ToFloat32(src1->R) + gl::float16ToFloat32(src2->R)) * 0.5f);
    }
};

struct R16G16F
{
    unsigned short R;
    unsigned short G;

    static void average(R16G16F *dst, const R16G16F *src1, const R16G16F *src2)
    {
        dst->R = gl::float32ToFloat16((gl::float16ToFloat32(src1->R) + gl::float16ToFloat32(src2->R)) * 0.5f);
        dst->G = gl::float32ToFloat16((gl::float16ToFloat32(src1->G) + gl::float16ToFloat32(src2->G)) * 0.5f);
    }
};

struct A32B32G32R32F
{
    float R;
    float G;
    float B;
    float A;

    static void average(A32B32G32R32F *dst, const A32B32G32R32F *src1, const A32B32G32R32F *src2)
    {
        dst->R = (src1->R + src2->R) * 0.5f;
        dst->G = (src1->G + src2->G) * 0.5f;
        dst->B = (src1->B + src2->B) * 0.5f;
        dst->A = (src1->A + src2->A) * 0.5f;
    }
};

struct R32F
{
    float R;

    static void average(R32F *dst, const R32F *src1, const R32F *src2)
    {
        dst->R = (src1->R + src2->R) * 0.5f;
    }
};

struct R32G32F
{
    float R;
    float G;

    static void average(R32G32F *dst, const R32G32F *src1, const R32G32F *src2)
    {
        dst->R = (src1->R + src2->R) * 0.5f;
        dst->G = (src1->G + src2->G) * 0.5f;
    }
};

struct R32G32B32F
{
    float R;
    float G;
    float B;

    static void average(R32G32B32F *dst, const R32G32B32F *src1, const R32G32B32F *src2)
    {
        dst->R = (src1->R + src2->R) * 0.5f;
        dst->G = (src1->G + src2->G) * 0.5f;
        dst->B = (src1->B + src2->B) * 0.5f;
    }
};

template <typename T>
static void GenerateMip(unsigned int sourceWidth, unsigned int sourceHeight,
                        const unsigned char *sourceData, int sourcePitch,
                        unsigned char *destData, int destPitch)
{
    unsigned int mipWidth = std::max(1U, sourceWidth >> 1);
    unsigned int mipHeight = std::max(1U, sourceHeight >> 1);

    if (sourceHeight == 1)
    {
        ASSERT(sourceWidth != 1);

        const T *src = (const T*)sourceData;
        T *dst = (T*)destData;

        for (unsigned int x = 0; x < mipWidth; x++)
        {
            T::average(&dst[x], &src[x * 2], &src[x * 2 + 1]);
        }
    }
    else if (sourceWidth == 1)
    {
        ASSERT(sourceHeight != 1);

        for (unsigned int y = 0; y < mipHeight; y++)
        {
            const T *src0 = (const T*)(sourceData + y * 2 * sourcePitch);
            const T *src1 = (const T*)(sourceData + y * 2 * sourcePitch + sourcePitch);
            T *dst = (T*)(destData + y * destPitch);

            T::average(dst, src0, src1);
        }
    }
    else
    {
        for (unsigned int y = 0; y < mipHeight; y++)
        {
            const T *src0 = (const T*)(sourceData + y * 2 * sourcePitch);
            const T *src1 = (const T*)(sourceData + y * 2 * sourcePitch + sourcePitch);
            T *dst = (T*)(destData + y * destPitch);

            for (unsigned int x = 0; x < mipWidth; x++)
            {
                T tmp0;
                T tmp1;

                T::average(&tmp0, &src0[x * 2], &src0[x * 2 + 1]);
                T::average(&tmp1, &src1[x * 2], &src1[x * 2 + 1]);
                T::average(&dst[x], &tmp0, &tmp1);
            }
        }
    }
}
}

#endif // LIBGLESV2_RENDERER_GENERATEMIP_H_
