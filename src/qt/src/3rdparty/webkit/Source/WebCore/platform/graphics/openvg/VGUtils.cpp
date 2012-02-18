/*
 * Copyright (C) Research In Motion Limited 2009. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "VGUtils.h"

#include "AffineTransform.h"
#include "FloatRect.h"
#include "TransformationMatrix.h"

namespace WebCore {

VGMatrix::VGMatrix(const VGfloat data[9])
{
    m_data[0] = data[0];
    m_data[1] = data[1];
    m_data[2] = data[2];
    m_data[3] = data[3];
    m_data[4] = data[4];
    m_data[5] = data[5];
    m_data[6] = data[6];
    m_data[7] = data[7];
    m_data[8] = data[8];
}

VGMatrix::VGMatrix(const AffineTransform& transformation)
{
    m_data[0] = transformation.a();
    m_data[1] = transformation.b();
    m_data[2] = 0;
    m_data[3] = transformation.c();
    m_data[4] = transformation.d();
    m_data[5] = 0;
    m_data[6] = transformation.e();
    m_data[7] = transformation.f();
    m_data[8] = 1;
}

VGMatrix::VGMatrix(const TransformationMatrix& matrix)
{
    m_data[0] = matrix.m11();
    m_data[1] = matrix.m12();
    m_data[2] = matrix.m14();
    m_data[3] = matrix.m21();
    m_data[4] = matrix.m22();
    m_data[5] = matrix.m24();
    m_data[6] = matrix.m41();
    m_data[7] = matrix.m42();
    m_data[8] = matrix.m44();
}

VGMatrix::operator AffineTransform() const
{
    AffineTransform transformation(
        m_data[0], m_data[1],
        m_data[3], m_data[4],
        m_data[6], m_data[7]);
    return transformation;
}

VGMatrix::operator TransformationMatrix() const
{
    TransformationMatrix matrix(
        m_data[0], m_data[1], 0, m_data[2],
        m_data[3], m_data[4], 0, m_data[5],
        0,         0,         1, 0,
        m_data[6], m_data[7], 0, m_data[8]);
    return matrix;
}

AffineTransform::operator VGMatrix() const
{
    return VGMatrix(*this);
}

TransformationMatrix::operator VGMatrix() const
{
    return VGMatrix(*this);
}

VGRect::VGRect(const VGfloat data[4])
{
    m_data[0] = data[0];
    m_data[1] = data[1];
    m_data[2] = data[2];
    m_data[3] = data[3];
}

VGRect::VGRect(const FloatRect& rect)
{
    m_data[0] = rect.x();
    m_data[1] = rect.y();
    m_data[2] = rect.width();
    m_data[3] = rect.height();
}

VGRect::operator FloatRect() const
{
    return FloatRect(m_data[0], m_data[1], m_data[2], m_data[3]);
}

FloatRect::operator VGRect() const
{
    return VGRect(*this);
}

int VGUtils::bytesForImage(VGImageFormat format, VGint width, VGint height)
{
    return width * height * imageFormatBitsPerPixel(format) / 8;
}

int VGUtils::bytesForImageScanline(VGImageFormat format, VGint width)
{
    int bits = width * imageFormatBitsPerPixel(format);
    if (bits % 8 > 1) // If unaligned, round up to the next byte.
        bits += 8 - (bits % 8);

    return bits / 8;
}

int VGUtils::imageFormatBitsPerPixel(VGImageFormat format)
{
    switch (format) {
    case VG_sRGBX_8888:
    case VG_sRGBA_8888:
    case VG_sRGBA_8888_PRE:
    case VG_lRGBX_8888:
    case VG_lRGBA_8888:
    case VG_lRGBA_8888_PRE:
    case VG_sXRGB_8888:
    case VG_sARGB_8888:
    case VG_sARGB_8888_PRE:
    case VG_lXRGB_8888:
    case VG_lARGB_8888:
    case VG_lARGB_8888_PRE:
    case VG_sBGRX_8888:
    case VG_sBGRA_8888:
    case VG_sBGRA_8888_PRE:
    case VG_lBGRX_8888:
    case VG_lBGRA_8888:
    case VG_lBGRA_8888_PRE:
    case VG_sXBGR_8888:
    case VG_sABGR_8888:
    case VG_sABGR_8888_PRE:
    case VG_lXBGR_8888:
    case VG_lABGR_8888:
    case VG_lABGR_8888_PRE:
        return 32;

    case VG_sRGB_565:
    case VG_sRGBA_5551:
    case VG_sRGBA_4444:
    case VG_sARGB_1555:
    case VG_sARGB_4444:
    case VG_sBGR_565:
    case VG_sBGRA_5551:
    case VG_sBGRA_4444:
    case VG_sABGR_1555:
    case VG_sABGR_4444:
        return 16;

    case VG_sL_8:
    case VG_lL_8:
    case VG_A_8:
        return 8;

    case VG_A_4:
        return 4;

    case VG_BW_1:
    case VG_A_1:
        return 1;

    default: // Will only happen when OpenVG extends the enum and we don't.
        ASSERT(false);
        return 0;
    }
}

#ifndef WTF_PLATFORM_BIG_ENDIAN
VGImageFormat VGUtils::endianAwareImageFormat(VGImageFormat bigEndianFormat)
{
    switch (bigEndianFormat) {
    case VG_sRGBX_8888:     return VG_sXBGR_8888;
    case VG_sRGBA_8888:     return VG_sABGR_8888;
    case VG_sRGBA_8888_PRE: return VG_sABGR_8888_PRE;
    case VG_lRGBX_8888:     return VG_lXBGR_8888;
    case VG_lRGBA_8888:     return VG_lABGR_8888;
    case VG_lRGBA_8888_PRE: return VG_lABGR_8888_PRE;
    case VG_sXRGB_8888:     return VG_sBGRX_8888;
    case VG_sARGB_8888:     return VG_sBGRA_8888;
    case VG_sARGB_8888_PRE: return VG_sBGRA_8888_PRE;
    case VG_lXRGB_8888:     return VG_lBGRX_8888;
    case VG_lARGB_8888:     return VG_lBGRA_8888;
    case VG_lARGB_8888_PRE: return VG_lBGRA_8888_PRE;
    case VG_sBGRX_8888:     return VG_sXRGB_8888;
    case VG_sBGRA_8888:     return VG_sARGB_8888;
    case VG_sBGRA_8888_PRE: return VG_sARGB_8888_PRE;
    case VG_lBGRX_8888:     return VG_lXRGB_8888;
    case VG_lBGRA_8888:     return VG_lARGB_8888;
    case VG_lBGRA_8888_PRE: return VG_lARGB_8888_PRE;
    case VG_sXBGR_8888:     return VG_sRGBX_8888;
    case VG_sABGR_8888:     return VG_sRGBA_8888;
    case VG_sABGR_8888_PRE: return VG_sRGBA_8888_PRE;
    case VG_lXBGR_8888:     return VG_lRGBX_8888;
    case VG_lABGR_8888:     return VG_lRGBA_8888;
    case VG_lABGR_8888_PRE: return VG_lRGBA_8888_PRE;
    default:                ASSERT(false);
                            return (VGImageFormat) 0;
    }
}
#else
VGImageFormat VGUtils::endianAwareImageFormat(VGImageFormat bigEndianFormat)
{
    return bigEndianFormat;
}
#endif

}
