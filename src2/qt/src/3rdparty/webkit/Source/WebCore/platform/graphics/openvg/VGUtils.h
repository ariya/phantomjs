/*
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
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

#ifndef VGUtils_h
#define VGUtils_h

#include <openvg.h>
#include <wtf/Assertions.h>

static inline const char* toVGErrorConstant(VGErrorCode error)
{
    switch (error) {
    case VG_BAD_HANDLE_ERROR:
        return "VG_BAD_HANDLE_ERROR";
    case VG_ILLEGAL_ARGUMENT_ERROR:
        return "VG_ILLEGAL_ARGUMENT_ERROR";
    case VG_OUT_OF_MEMORY_ERROR:
        return "VG_OUT_OF_MEMORY_ERROR";
    case VG_PATH_CAPABILITY_ERROR:
        return "VG_PATH_CAPABILITY_ERROR";
    case VG_UNSUPPORTED_IMAGE_FORMAT_ERROR:
        return "VG_UNSUPPORTED_IMAGE_FORMAT_ERROR";
    case VG_UNSUPPORTED_PATH_FORMAT_ERROR:
        return "VG_UNSUPPORTED_PATH_FORMAT_ERROR";
    case VG_IMAGE_IN_USE_ERROR:
        return "VG_IMAGE_IN_USE_ERROR";
    case VG_NO_CONTEXT_ERROR:
        return "VG_NO_CONTEXT_ERROR";
    default:
        return "UNKNOWN_ERROR";
    }
}

#if ASSERT_DISABLED
#define ASSERT_VG_NO_ERROR() ((void)0)
#else
#define ASSERT_VG_NO_ERROR() do { \
    VGErrorCode vgErrorCode = vgGetError(); \
    ASSERT_WITH_MESSAGE(vgErrorCode == VG_NO_ERROR, "Found %s", toVGErrorConstant(vgErrorCode)); \
} while (0)
#endif


namespace WebCore {

class AffineTransform;
class FloatRect;
class TransformationMatrix;

class VGMatrix {
public:
    VGMatrix(const VGfloat data[9]);
    VGMatrix(const AffineTransform&);
    VGMatrix(const TransformationMatrix&);
    const VGfloat* toVGfloat() const { return m_data; }
    operator AffineTransform() const;
    operator TransformationMatrix() const;
private:
    VGfloat m_data[9];
};

class VGRect {
public:
    VGRect(const VGfloat data[4]);
    VGRect(const FloatRect&);
    const VGfloat* toVGfloat() const { return m_data; }
    operator FloatRect() const;
private:
    VGfloat m_data[4];
};

class VGUtils {
public:
    static int bytesForImage(VGImageFormat, VGint width, VGint height);
    static int bytesForImageScanline(VGImageFormat, VGint width);
    static int imageFormatBitsPerPixel(VGImageFormat);

    /**
     * Return a flipped VGImageFormat if the platform is little endian
     * (e.g. VG_ABGR_8888 for a given VG_RGBA_8888), or return the image format
     * as is if the platform is big endian.
     *
     * OpenVG itself is indifferent to endianness, it will always work on a
     * single machine word with the bytes going from left to right as specified
     * in the image format, no matter which one of the bytes is most or least
     * significant.
     *
     * However, if you interface with vgImageSubData()/vgGetImageSubData()
     * using a byte array then you want to make sure the byte order is
     * appropriate for the given platform (otherwise the byte indexes need
     * to be swapped depending on endianness). So, use this function when
     * interfacing with byte arrays, and don't use it otherwise.
     */
    static VGImageFormat endianAwareImageFormat(VGImageFormat bigEndianFormat);
};

}

#endif
