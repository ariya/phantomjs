/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if ENABLE(WEBGL)

#include "GraphicsContext3D.h"

#include "ArrayBufferView.h"
#include "CheckedInt.h"
#include "DrawingBuffer.h"
#include "Extensions3D.h"
#include "Image.h"
#include "ImageData.h"

#include <wtf/OwnArrayPtr.h>
#include <wtf/PassOwnArrayPtr.h>

namespace WebCore {

namespace {

uint8_t convertColor16LittleTo8(uint16_t value)
{
    return value >> 8;
}

uint8_t convertColor16BigTo8(uint16_t value)
{
    return static_cast<uint8_t>(value & 0x00FF);
}

} // anonymous namespace


PassRefPtr<DrawingBuffer> GraphicsContext3D::createDrawingBuffer(const IntSize& size)
{
    return DrawingBuffer::create(this, size);
}

bool GraphicsContext3D::texImage2DResourceSafe(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height, GC3Dint border, GC3Denum format, GC3Denum type, GC3Dint unpackAlignment)
{
    ASSERT(unpackAlignment == 1 || unpackAlignment == 2 || unpackAlignment == 4 || unpackAlignment == 8);
    OwnArrayPtr<unsigned char> zero;
    if (width > 0 && height > 0) {
        unsigned int size;
        GC3Denum error = computeImageSizeInBytes(format, type, width, height, unpackAlignment, &size, 0);
        if (error != GraphicsContext3D::NO_ERROR) {
            synthesizeGLError(error);
            return false;
        }
        zero = adoptArrayPtr(new unsigned char[size]);
        if (!zero) {
            synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
            return false;
        }
        memset(zero.get(), 0, size);
    }
    return texImage2D(target, level, internalformat, width, height, border, format, type, zero.get());
}

bool GraphicsContext3D::computeFormatAndTypeParameters(GC3Denum format,
                                                       GC3Denum type,
                                                       unsigned int* componentsPerPixel,
                                                       unsigned int* bytesPerComponent)
{
    switch (format) {
    case GraphicsContext3D::ALPHA:
        *componentsPerPixel = 1;
        break;
    case GraphicsContext3D::LUMINANCE:
        *componentsPerPixel = 1;
        break;
    case GraphicsContext3D::LUMINANCE_ALPHA:
        *componentsPerPixel = 2;
        break;
    case GraphicsContext3D::RGB:
        *componentsPerPixel = 3;
        break;
    case GraphicsContext3D::RGBA:
    case Extensions3D::BGRA_EXT: // GL_EXT_texture_format_BGRA8888
        *componentsPerPixel = 4;
        break;
    default:
        return false;
    }
    switch (type) {
    case GraphicsContext3D::UNSIGNED_BYTE:
        *bytesPerComponent = sizeof(GC3Dubyte);
        break;
    case GraphicsContext3D::UNSIGNED_SHORT_5_6_5:
    case GraphicsContext3D::UNSIGNED_SHORT_4_4_4_4:
    case GraphicsContext3D::UNSIGNED_SHORT_5_5_5_1:
        *componentsPerPixel = 1;
        *bytesPerComponent = sizeof(GC3Dushort);
        break;
    case GraphicsContext3D::FLOAT: // OES_texture_float
        *bytesPerComponent = sizeof(GC3Dfloat);
        break;
    default:
        return false;
    }
    return true;
}

GC3Denum GraphicsContext3D::computeImageSizeInBytes(GC3Denum format, GC3Denum type, GC3Dsizei width, GC3Dsizei height, GC3Dint alignment,
                                                    unsigned int* imageSizeInBytes, unsigned int* paddingInBytes)
{
    ASSERT(imageSizeInBytes);
    ASSERT(alignment == 1 || alignment == 2 || alignment == 4 || alignment == 8);
    if (width < 0 || height < 0)
        return GraphicsContext3D::INVALID_VALUE;
    unsigned int bytesPerComponent, componentsPerPixel;
    if (!computeFormatAndTypeParameters(format, type, &bytesPerComponent, &componentsPerPixel))
        return GraphicsContext3D::INVALID_ENUM;
    if (!width || !height) {
        *imageSizeInBytes = 0;
        if (paddingInBytes)
            *paddingInBytes = 0;
        return GraphicsContext3D::NO_ERROR;
    }
    CheckedInt<uint32_t> checkedValue(bytesPerComponent * componentsPerPixel);
    checkedValue *=  width;
    if (!checkedValue.valid())
        return GraphicsContext3D::INVALID_VALUE;
    unsigned int validRowSize = checkedValue.value();
    unsigned int padding = 0;
    unsigned int residual = validRowSize % alignment;
    if (residual) {
        padding = alignment - residual;
        checkedValue += padding;
    }
    // Last row needs no padding.
    checkedValue *= (height - 1);
    checkedValue += validRowSize;
    if (!checkedValue.valid())
        return GraphicsContext3D::INVALID_VALUE;
    *imageSizeInBytes = checkedValue.value();
    if (paddingInBytes)
        *paddingInBytes = padding;
    return GraphicsContext3D::NO_ERROR;
}

bool GraphicsContext3D::extractImageData(Image* image,
                                         GC3Denum format,
                                         GC3Denum type,
                                         bool flipY,
                                         bool premultiplyAlpha,
                                         bool ignoreGammaAndColorProfile,
                                         Vector<uint8_t>& data)
{
    if (!image)
        return false;
    if (!getImageData(image, format, type, premultiplyAlpha, ignoreGammaAndColorProfile, data))
        return false;
    if (flipY) {
        unsigned int componentsPerPixel, bytesPerComponent;
        if (!computeFormatAndTypeParameters(format, type,
                                            &componentsPerPixel,
                                            &bytesPerComponent))
            return false;
        // The image data is tightly packed, and we upload it as such.
        unsigned int unpackAlignment = 1;
        flipVertically(data.data(), image->width(), image->height(),
                       componentsPerPixel * bytesPerComponent,
                       unpackAlignment);
    }
    return true;
}

bool GraphicsContext3D::extractImageData(ImageData* imageData,
                                         GC3Denum format,
                                         GC3Denum type,
                                         bool flipY,
                                         bool premultiplyAlpha,
                                         Vector<uint8_t>& data)
{
    if (!imageData)
        return false;
    int width = imageData->width();
    int height = imageData->height();
    int dataBytes = width * height * 4;
    data.resize(dataBytes);
    if (!packPixels(imageData->data()->data()->data(),
                    SourceFormatRGBA8,
                    width,
                    height,
                    0,
                    format,
                    type,
                    premultiplyAlpha ? AlphaDoPremultiply : AlphaDoNothing,
                    data.data()))
        return false;
    if (flipY) {
        unsigned int componentsPerPixel, bytesPerComponent;
        if (!computeFormatAndTypeParameters(format, type,
                                            &componentsPerPixel,
                                            &bytesPerComponent))
            return false;
        // The image data is tightly packed, and we upload it as such.
        unsigned int unpackAlignment = 1;
        flipVertically(data.data(), width, height,
                       componentsPerPixel * bytesPerComponent,
                       unpackAlignment);
    }
    return true;
}

bool GraphicsContext3D::extractTextureData(unsigned int width, unsigned int height,
                                           GC3Denum format, GC3Denum type,
                                           unsigned int unpackAlignment,
                                           bool flipY, bool premultiplyAlpha,
                                           const void* pixels,
                                           Vector<uint8_t>& data)
{
    // Assumes format, type, etc. have already been validated.
    SourceDataFormat sourceDataFormat = SourceFormatRGBA8;
    switch (type) {
    case UNSIGNED_BYTE:
        switch (format) {
        case RGBA:
            sourceDataFormat = SourceFormatRGBA8;
            break;
        case RGB:
            sourceDataFormat = SourceFormatRGB8;
            break;
        case ALPHA:
            sourceDataFormat = SourceFormatA8;
            break;
        case LUMINANCE:
            sourceDataFormat = SourceFormatR8;
            break;
        case LUMINANCE_ALPHA:
            sourceDataFormat = SourceFormatRA8;
            break;
        default:
            ASSERT_NOT_REACHED();
        }
        break;
    case FLOAT: // OES_texture_float
        switch (format) {
        case RGBA:
            sourceDataFormat = SourceFormatRGBA32F;
            break;
        case RGB:
            sourceDataFormat = SourceFormatRGB32F;
            break;
        case ALPHA:
            sourceDataFormat = SourceFormatA32F;
            break;
        case LUMINANCE:
            sourceDataFormat = SourceFormatR32F;
            break;
        case LUMINANCE_ALPHA:
            sourceDataFormat = SourceFormatRA32F;
            break;
        default:
            ASSERT_NOT_REACHED();
        }
        break;
    case UNSIGNED_SHORT_5_5_5_1:
        sourceDataFormat = SourceFormatRGBA5551;
        break;
    case UNSIGNED_SHORT_4_4_4_4:
        sourceDataFormat = SourceFormatRGBA4444;
        break;
    case UNSIGNED_SHORT_5_6_5:
        sourceDataFormat = SourceFormatRGB565;
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    // Resize the output buffer.
    unsigned int componentsPerPixel, bytesPerComponent;
    if (!computeFormatAndTypeParameters(format, type,
                                        &componentsPerPixel,
                                        &bytesPerComponent))
        return false;
    unsigned int bytesPerPixel = componentsPerPixel * bytesPerComponent;
    data.resize(width * height * bytesPerPixel);

    if (!packPixels(static_cast<const uint8_t*>(pixels),
                    sourceDataFormat,
                    width, height, unpackAlignment,
                    format, type,
                    (premultiplyAlpha ? AlphaDoPremultiply : AlphaDoNothing),
                    data.data()))
        return false;
    // The pixel data is now tightly packed.
    if (flipY)
        flipVertically(data.data(), width, height, bytesPerPixel, 1);
    return true;
}

void GraphicsContext3D::flipVertically(void* imageData,
                                       unsigned int width,
                                       unsigned int height,
                                       unsigned int bytesPerPixel,
                                       unsigned int unpackAlignment)
{
    if (!width || !height)
        return;
    unsigned int validRowBytes = width * bytesPerPixel;
    unsigned int totalRowBytes = validRowBytes;
    unsigned int remainder = validRowBytes % unpackAlignment;
    if (remainder)
        totalRowBytes += (unpackAlignment - remainder);
    uint8_t* tempRow = new uint8_t[validRowBytes];
    uint8_t* data = static_cast<uint8_t*>(imageData);
    for (unsigned i = 0; i < height / 2; i++) {
        uint8_t* lowRow = data + (totalRowBytes * i);
        uint8_t* highRow = data + (totalRowBytes * (height - i - 1));
        memcpy(tempRow, lowRow, validRowBytes);
        memcpy(lowRow, highRow, validRowBytes);
        memcpy(highRow, tempRow, validRowBytes);
    }
    delete[] tempRow;
}

// The following packing and unpacking routines are expressed in terms
// of line-by-line operations, and passed by function pointer rather
// than template parameter, to achieve the majority of the speedups of
// having them inlined while reducing code size.

namespace {

//----------------------------------------------------------------------
// Pixel unpacking routines.

void unpackOneRowOfRGBA16LittleToRGBA8(const uint16_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = convertColor16LittleTo8(source[0]);
        destination[1] = convertColor16LittleTo8(source[1]);
        destination[2] = convertColor16LittleTo8(source[2]);
        destination[3] = convertColor16LittleTo8(source[3]);
        source += 4;
        destination += 4;
    }
}

void unpackOneRowOfRGBA16BigToRGBA8(const uint16_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = convertColor16BigTo8(source[0]);
        destination[1] = convertColor16BigTo8(source[1]);
        destination[2] = convertColor16BigTo8(source[2]);
        destination[3] = convertColor16BigTo8(source[3]);
        source += 4;
        destination += 4;
    }
}

void unpackOneRowOfRGB8ToRGBA8(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = source[0];
        destination[1] = source[1];
        destination[2] = source[2];
        destination[3] = 0xFF;
        source += 3;
        destination += 4;
    }
}

void unpackOneRowOfRGB16LittleToRGBA8(const uint16_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = convertColor16LittleTo8(source[0]);
        destination[1] = convertColor16LittleTo8(source[1]);
        destination[2] = convertColor16LittleTo8(source[2]);
        destination[3] = 0xFF;
        source += 3;
        destination += 4;
    }
}

void unpackOneRowOfRGB16BigToRGBA8(const uint16_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = convertColor16BigTo8(source[0]);
        destination[1] = convertColor16BigTo8(source[1]);
        destination[2] = convertColor16BigTo8(source[2]);
        destination[3] = 0xFF;
        source += 3;
        destination += 4;
    }
}

void unpackOneRowOfBGR8ToRGBA8(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = source[2];
        destination[1] = source[1];
        destination[2] = source[0];
        destination[3] = 0xFF;
        source += 3;
        destination += 4;
    }
}

void unpackOneRowOfARGB8ToRGBA8(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = source[1];
        destination[1] = source[2];
        destination[2] = source[3];
        destination[3] = source[0];
        source += 4;
        destination += 4;
    }
}

void unpackOneRowOfARGB16LittleToRGBA8(const uint16_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = convertColor16LittleTo8(source[1]);
        destination[1] = convertColor16LittleTo8(source[2]);
        destination[2] = convertColor16LittleTo8(source[3]);
        destination[3] = convertColor16LittleTo8(source[0]);
        source += 4;
        destination += 4;
    }
}

void unpackOneRowOfARGB16BigToRGBA8(const uint16_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = convertColor16BigTo8(source[1]);
        destination[1] = convertColor16BigTo8(source[2]);
        destination[2] = convertColor16BigTo8(source[3]);
        destination[3] = convertColor16BigTo8(source[0]);
        source += 4;
        destination += 4;
    }
}

void unpackOneRowOfABGR8ToRGBA8(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = source[3];
        destination[1] = source[2];
        destination[2] = source[1];
        destination[3] = source[0];
        source += 4;
        destination += 4;
    }
}

void unpackOneRowOfBGRA8ToRGBA8(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = source[2];
        destination[1] = source[1];
        destination[2] = source[0];
        destination[3] = source[3];
        source += 4;
        destination += 4;
    }
}

void unpackOneRowOfBGRA16LittleToRGBA8(const uint16_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = convertColor16LittleTo8(source[2]);
        destination[1] = convertColor16LittleTo8(source[1]);
        destination[2] = convertColor16LittleTo8(source[0]);
        destination[3] = convertColor16LittleTo8(source[3]);
        source += 4;
        destination += 4;
    }
}

void unpackOneRowOfBGRA16BigToRGBA8(const uint16_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = convertColor16BigTo8(source[2]);
        destination[1] = convertColor16BigTo8(source[1]);
        destination[2] = convertColor16BigTo8(source[0]);
        destination[3] = convertColor16BigTo8(source[3]);
        source += 4;
        destination += 4;
    }
}

void unpackOneRowOfRGBA5551ToRGBA8(const uint16_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        uint16_t packedValue = source[0];
        uint8_t r = packedValue >> 11;
        uint8_t g = (packedValue >> 6) & 0x1F;
        uint8_t b = (packedValue >> 1) & 0x1F;
        destination[0] = (r << 3) | (r & 0x7);
        destination[1] = (g << 3) | (g & 0x7);
        destination[2] = (b << 3) | (b & 0x7);
        destination[3] = (packedValue & 0x1) ? 0xFF : 0x0;
        source += 1;
        destination += 4;
    }
}

void unpackOneRowOfRGBA4444ToRGBA8(const uint16_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        uint16_t packedValue = source[0];
        uint8_t r = packedValue >> 12;
        uint8_t g = (packedValue >> 8) & 0x0F;
        uint8_t b = (packedValue >> 4) & 0x0F;
        uint8_t a = packedValue & 0x0F;
        destination[0] = r << 4 | r;
        destination[1] = g << 4 | g;
        destination[2] = b << 4 | b;
        destination[3] = a << 4 | a;
        source += 1;
        destination += 4;
    }
}

void unpackOneRowOfRGB565ToRGBA8(const uint16_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        uint16_t packedValue = source[0];
        uint8_t r = packedValue >> 11;
        uint8_t g = (packedValue >> 5) & 0x3F;
        uint8_t b = packedValue & 0x1F;
        destination[0] = (r << 3) | (r & 0x7);
        destination[1] = (g << 2) | (g & 0x3);
        destination[2] = (b << 3) | (b & 0x7);
        destination[3] = 0xFF;
        source += 1;
        destination += 4;
    }
}

void unpackOneRowOfR8ToRGBA8(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = source[0];
        destination[1] = source[0];
        destination[2] = source[0];
        destination[3] = 0xFF;
        source += 1;
        destination += 4;
    }
}

void unpackOneRowOfR16LittleToRGBA8(const uint16_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = convertColor16LittleTo8(source[0]);
        destination[1] = convertColor16LittleTo8(source[0]);
        destination[2] = convertColor16LittleTo8(source[0]);
        destination[3] = 0xFF;
        source += 1;
        destination += 4;
    }
}

void unpackOneRowOfR16BigToRGBA8(const uint16_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = convertColor16BigTo8(source[0]);
        destination[1] = convertColor16BigTo8(source[0]);
        destination[2] = convertColor16BigTo8(source[0]);
        destination[3] = 0xFF;
        source += 1;
        destination += 4;
    }
}

void unpackOneRowOfRA8ToRGBA8(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = source[0];
        destination[1] = source[0];
        destination[2] = source[0];
        destination[3] = source[1];
        source += 2;
        destination += 4;
    }
}

void unpackOneRowOfRA16LittleToRGBA8(const uint16_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = convertColor16LittleTo8(source[0]);
        destination[1] = convertColor16LittleTo8(source[0]);
        destination[2] = convertColor16LittleTo8(source[0]);
        destination[3] = convertColor16LittleTo8(source[1]);
        source += 2;
        destination += 4;
    }
}

void unpackOneRowOfRA16BigToRGBA8(const uint16_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = convertColor16BigTo8(source[0]);
        destination[1] = convertColor16BigTo8(source[0]);
        destination[2] = convertColor16BigTo8(source[0]);
        destination[3] = convertColor16BigTo8(source[1]);
        source += 2;
        destination += 4;
    }
}

void unpackOneRowOfAR8ToRGBA8(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = source[1];
        destination[1] = source[1];
        destination[2] = source[1];
        destination[3] = source[0];
        source += 2;
        destination += 4;
    }
}

void unpackOneRowOfAR16LittleToRGBA8(const uint16_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = convertColor16LittleTo8(source[1]);
        destination[1] = convertColor16LittleTo8(source[1]);
        destination[2] = convertColor16LittleTo8(source[1]);
        destination[3] = convertColor16LittleTo8(source[0]);
        source += 2;
        destination += 4;
    }
}

void unpackOneRowOfAR16BigToRGBA8(const uint16_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = convertColor16BigTo8(source[1]);
        destination[1] = convertColor16BigTo8(source[1]);
        destination[2] = convertColor16BigTo8(source[1]);
        destination[3] = convertColor16BigTo8(source[0]);
        source += 2;
        destination += 4;
    }
}

void unpackOneRowOfA8ToRGBA8(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = 0x0;
        destination[1] = 0x0;
        destination[2] = 0x0;
        destination[3] = source[0];
        source += 1;
        destination += 4;
    }
}

void unpackOneRowOfA16LittleToRGBA8(const uint16_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = 0x0;
        destination[1] = 0x0;
        destination[2] = 0x0;
        destination[3] = convertColor16LittleTo8(source[0]);
        source += 1;
        destination += 4;
    }
}

void unpackOneRowOfA16BigToRGBA8(const uint16_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = 0x0;
        destination[1] = 0x0;
        destination[2] = 0x0;
        destination[3] = convertColor16BigTo8(source[0]);
        source += 1;
        destination += 4;
    }
}

void unpackOneRowOfRGB32FToRGBA32F(const float* source, float* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = source[0];
        destination[1] = source[1];
        destination[2] = source[2];
        destination[3] = 1;
        source += 3;
        destination += 4;
    }
}

void unpackOneRowOfR32FToRGBA32F(const float* source, float* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = source[0];
        destination[1] = source[0];
        destination[2] = source[0];
        destination[3] = 1;
        source += 1;
        destination += 4;
    }
}

void unpackOneRowOfRA32FToRGBA32F(const float* source, float* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = source[0];
        destination[1] = source[0];
        destination[2] = source[0];
        destination[3] = source[1];
        source += 2;
        destination += 4;
    }
}

void unpackOneRowOfA32FToRGBA32F(const float* source, float* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = 0;
        destination[1] = 0;
        destination[2] = 0;
        destination[3] = source[0];
        source += 1;
        destination += 4;
    }
}

//----------------------------------------------------------------------
// Pixel packing routines.
//

void packOneRowOfRGBA8ToA8(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = source[3];
        source += 4;
        destination += 1;
    }
}

void packOneRowOfRGBA8ToR8(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = source[0];
        source += 4;
        destination += 1;
    }
}

void packOneRowOfRGBA8ToR8Premultiply(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        float scaleFactor = source[3] / 255.0f;
        uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
        destination[0] = sourceR;
        source += 4;
        destination += 1;
    }
}

// FIXME: this routine is lossy and must be removed.
void packOneRowOfRGBA8ToR8Unmultiply(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        float scaleFactor = 1.0f / (source[3] ? source[3] / 255.0f : 1.0f);
        uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
        destination[0] = sourceR;
        source += 4;
        destination += 1;
    }
}

void packOneRowOfRGBA8ToRA8(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = source[0];
        destination[1] = source[3];
        source += 4;
        destination += 2;
    }
}

void packOneRowOfRGBA8ToRA8Premultiply(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        float scaleFactor = source[3] / 255.0f;
        uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
        destination[0] = sourceR;
        destination[1] = source[3];
        source += 4;
        destination += 2;
    }
}

// FIXME: this routine is lossy and must be removed.
void packOneRowOfRGBA8ToRA8Unmultiply(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        float scaleFactor = 1.0f / (source[3] ? source[3] / 255.0f : 1.0f);
        uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
        destination[0] = sourceR;
        destination[1] = source[3];
        source += 4;
        destination += 2;
    }
}

void packOneRowOfRGBA8ToRGB8(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = source[0];
        destination[1] = source[1];
        destination[2] = source[2];
        source += 4;
        destination += 3;
    }
}

void packOneRowOfRGBA8ToRGB8Premultiply(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        float scaleFactor = source[3] / 255.0f;
        uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
        uint8_t sourceG = static_cast<uint8_t>(static_cast<float>(source[1]) * scaleFactor);
        uint8_t sourceB = static_cast<uint8_t>(static_cast<float>(source[2]) * scaleFactor);
        destination[0] = sourceR;
        destination[1] = sourceG;
        destination[2] = sourceB;
        source += 4;
        destination += 3;
    }
}

// FIXME: this routine is lossy and must be removed.
void packOneRowOfRGBA8ToRGB8Unmultiply(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        float scaleFactor = 1.0f / (source[3] ? source[3] / 255.0f : 1.0f);
        uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
        uint8_t sourceG = static_cast<uint8_t>(static_cast<float>(source[1]) * scaleFactor);
        uint8_t sourceB = static_cast<uint8_t>(static_cast<float>(source[2]) * scaleFactor);
        destination[0] = sourceR;
        destination[1] = sourceG;
        destination[2] = sourceB;
        source += 4;
        destination += 3;
    }
}

// This is only used when the source format is different than SourceFormatRGBA8.
void packOneRowOfRGBA8ToRGBA8(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = source[0];
        destination[1] = source[1];
        destination[2] = source[2];
        destination[3] = source[3];
        source += 4;
        destination += 4;
    }
}

void packOneRowOfRGBA8ToRGBA8Premultiply(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        float scaleFactor = source[3] / 255.0f;
        uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
        uint8_t sourceG = static_cast<uint8_t>(static_cast<float>(source[1]) * scaleFactor);
        uint8_t sourceB = static_cast<uint8_t>(static_cast<float>(source[2]) * scaleFactor);
        destination[0] = sourceR;
        destination[1] = sourceG;
        destination[2] = sourceB;
        destination[3] = source[3];
        source += 4;
        destination += 4;
    }
}

// FIXME: this routine is lossy and must be removed.
void packOneRowOfRGBA8ToRGBA8Unmultiply(const uint8_t* source, uint8_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        float scaleFactor = 1.0f / (source[3] ? source[3] / 255.0f : 1.0f);
        uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
        uint8_t sourceG = static_cast<uint8_t>(static_cast<float>(source[1]) * scaleFactor);
        uint8_t sourceB = static_cast<uint8_t>(static_cast<float>(source[2]) * scaleFactor);
        destination[0] = sourceR;
        destination[1] = sourceG;
        destination[2] = sourceB;
        destination[3] = source[3];
        source += 4;
        destination += 4;
    }
}

void packOneRowOfRGBA8ToUnsignedShort4444(const uint8_t* source, uint16_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        *destination = (((source[0] & 0xF0) << 8)
                        | ((source[1] & 0xF0) << 4)
                        | (source[2] & 0xF0)
                        | (source[3] >> 4));
        source += 4;
        destination += 1;
    }
}

void packOneRowOfRGBA8ToUnsignedShort4444Premultiply(const uint8_t* source, uint16_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        float scaleFactor = source[3] / 255.0f;
        uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
        uint8_t sourceG = static_cast<uint8_t>(static_cast<float>(source[1]) * scaleFactor);
        uint8_t sourceB = static_cast<uint8_t>(static_cast<float>(source[2]) * scaleFactor);
        *destination = (((sourceR & 0xF0) << 8)
                        | ((sourceG & 0xF0) << 4)
                        | (sourceB & 0xF0)
                        | (source[3] >> 4));
        source += 4;
        destination += 1;
    }
}

// FIXME: this routine is lossy and must be removed.
void packOneRowOfRGBA8ToUnsignedShort4444Unmultiply(const uint8_t* source, uint16_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        float scaleFactor = 1.0f / (source[3] ? source[3] / 255.0f : 1.0f);
        uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
        uint8_t sourceG = static_cast<uint8_t>(static_cast<float>(source[1]) * scaleFactor);
        uint8_t sourceB = static_cast<uint8_t>(static_cast<float>(source[2]) * scaleFactor);
        *destination = (((sourceR & 0xF0) << 8)
                        | ((sourceG & 0xF0) << 4)
                        | (sourceB & 0xF0)
                        | (source[3] >> 4));
        source += 4;
        destination += 1;
    }
}

void packOneRowOfRGBA8ToUnsignedShort5551(const uint8_t* source, uint16_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        *destination = (((source[0] & 0xF8) << 8)
                        | ((source[1] & 0xF8) << 3)
                        | ((source[2] & 0xF8) >> 2)
                        | (source[3] >> 7));
        source += 4;
        destination += 1;
    }
}

void packOneRowOfRGBA8ToUnsignedShort5551Premultiply(const uint8_t* source, uint16_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        float scaleFactor = source[3] / 255.0f;
        uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
        uint8_t sourceG = static_cast<uint8_t>(static_cast<float>(source[1]) * scaleFactor);
        uint8_t sourceB = static_cast<uint8_t>(static_cast<float>(source[2]) * scaleFactor);
        *destination = (((sourceR & 0xF8) << 8)
                        | ((sourceG & 0xF8) << 3)
                        | ((sourceB & 0xF8) >> 2)
                        | (source[3] >> 7));
        source += 4;
        destination += 1;
    }
}

// FIXME: this routine is lossy and must be removed.
void packOneRowOfRGBA8ToUnsignedShort5551Unmultiply(const uint8_t* source, uint16_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        float scaleFactor = 1.0f / (source[3] ? source[3] / 255.0f : 1.0f);
        uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
        uint8_t sourceG = static_cast<uint8_t>(static_cast<float>(source[1]) * scaleFactor);
        uint8_t sourceB = static_cast<uint8_t>(static_cast<float>(source[2]) * scaleFactor);
        *destination = (((sourceR & 0xF8) << 8)
                        | ((sourceG & 0xF8) << 3)
                        | ((sourceB & 0xF8) >> 2)
                        | (source[3] >> 7));
        source += 4;
        destination += 1;
    }
}

void packOneRowOfRGBA8ToUnsignedShort565(const uint8_t* source, uint16_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        *destination = (((source[0] & 0xF8) << 8)
                        | ((source[1] & 0xFC) << 3)
                        | ((source[2] & 0xF8) >> 3));
        source += 4;
        destination += 1;
    }
}

void packOneRowOfRGBA8ToUnsignedShort565Premultiply(const uint8_t* source, uint16_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        float scaleFactor = source[3] / 255.0f;
        uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
        uint8_t sourceG = static_cast<uint8_t>(static_cast<float>(source[1]) * scaleFactor);
        uint8_t sourceB = static_cast<uint8_t>(static_cast<float>(source[2]) * scaleFactor);
        *destination = (((sourceR & 0xF8) << 8)
                        | ((sourceG & 0xFC) << 3)
                        | ((sourceB & 0xF8) >> 3));
        source += 4;
        destination += 1;
    }
}

// FIXME: this routine is lossy and must be removed.
void packOneRowOfRGBA8ToUnsignedShort565Unmultiply(const uint8_t* source, uint16_t* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        float scaleFactor = 1.0f / (source[3] ? source[3] / 255.0f : 1.0f);
        uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
        uint8_t sourceG = static_cast<uint8_t>(static_cast<float>(source[1]) * scaleFactor);
        uint8_t sourceB = static_cast<uint8_t>(static_cast<float>(source[2]) * scaleFactor);
        *destination = (((sourceR & 0xF8) << 8)
                        | ((sourceG & 0xFC) << 3)
                        | ((sourceB & 0xF8) >> 3));
        source += 4;
        destination += 1;
    }
}

void packOneRowOfRGBA32FToRGB32F(const float* source, float* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = source[0];
        destination[1] = source[1];
        destination[2] = source[2];
        source += 4;
        destination += 3;
    }
}

void packOneRowOfRGBA32FToRGB32FPremultiply(const float* source, float* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        float scaleFactor = source[3];
        destination[0] = source[0] * scaleFactor;
        destination[1] = source[1] * scaleFactor;
        destination[2] = source[2] * scaleFactor;
        source += 4;
        destination += 3;
    }
}

void packOneRowOfRGBA32FToRGBA32FPremultiply(const float* source, float* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        float scaleFactor = source[3];
        destination[0] = source[0] * scaleFactor;
        destination[1] = source[1] * scaleFactor;
        destination[2] = source[2] * scaleFactor;
        destination[3] = source[3];
        source += 4;
        destination += 4;
    }
}

void packOneRowOfRGBA32FToA32F(const float* source, float* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = source[3];
        source += 4;
        destination += 1;
    }
}

void packOneRowOfRGBA32FToR32F(const float* source, float* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = source[0];
        source += 4;
        destination += 1;
    }
}

void packOneRowOfRGBA32FToR32FPremultiply(const float* source, float* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        float scaleFactor = source[3];
        destination[0] = source[0] * scaleFactor;
        source += 4;
        destination += 1;
    }
}


void packOneRowOfRGBA32FToRA32F(const float* source, float* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        destination[0] = source[0];
        destination[1] = source[3];
        source += 4;
        destination += 2;
    }
}

void packOneRowOfRGBA32FToRA32FPremultiply(const float* source, float* destination, unsigned int pixelsPerRow)
{
    for (unsigned int i = 0; i < pixelsPerRow; ++i) {
        float scaleFactor = source[3];
        destination[0] = source[0] * scaleFactor;
        destination[1] = scaleFactor;
        source += 4;
        destination += 2;
    }
}

} // anonymous namespace

// This is used whenever unpacking is necessary; i.e., the source data
// is not in RGBA8/RGBA32F format, or the unpack alignment specifies
// that rows are not tightly packed.
template<typename SourceType, typename IntermediateType, typename DestType>
static void doUnpackingAndPacking(const SourceType* sourceData,
                                  void (*rowUnpackingFunc)(const SourceType*, IntermediateType*, unsigned int),
                                  unsigned int width,
                                  unsigned int height,
                                  unsigned int sourceElementsPerRow,
                                  DestType* destinationData,
                                  void (*rowPackingFunc)(const IntermediateType*, DestType*, unsigned int),
                                  unsigned int destinationElementsPerPixel)
{
    OwnArrayPtr<IntermediateType> temporaryRGBAData = adoptArrayPtr(new IntermediateType[width * 4]);
    const SourceType* endPointer = sourceData + height * sourceElementsPerRow;
    unsigned int destinationElementsPerRow = width * destinationElementsPerPixel;
    while (sourceData < endPointer) {
        rowUnpackingFunc(sourceData, temporaryRGBAData.get(), width);
        rowPackingFunc(temporaryRGBAData.get(), destinationData, width);
        sourceData += sourceElementsPerRow;
        destinationData += destinationElementsPerRow;
    }
}

template<typename SourceType>
static unsigned int computeSourceElementsPerRow(unsigned int width,
                                                unsigned int bytesPerPixel,
                                                unsigned int unpackAlignment)
{
    unsigned int elementSizeInBytes = sizeof(SourceType);
    ASSERT(elementSizeInBytes <= bytesPerPixel);
    ASSERT(!(bytesPerPixel % elementSizeInBytes));
    unsigned int validRowBytes = width * bytesPerPixel;
    unsigned int totalRowBytes = validRowBytes;
    if (unpackAlignment) {
        unsigned int remainder = validRowBytes % unpackAlignment;
        if (remainder)
            totalRowBytes += (unpackAlignment - remainder);
    }
    return totalRowBytes / elementSizeInBytes;
}

// This handles all conversions with a faster path for tightly packed RGBA8 source data.
template<typename DestType>
static void doPacking(const void* sourceData,
                      GraphicsContext3D::SourceDataFormat sourceDataFormat,
                      unsigned int width,
                      unsigned int height,
                      unsigned int sourceUnpackAlignment,
                      DestType* destinationData,
                      void (*rowPackingFunc)(const uint8_t*, DestType*, unsigned int),
                      unsigned int destinationElementsPerPixel)
{
    switch (sourceDataFormat) {
    case GraphicsContext3D::SourceFormatRGBA8: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint8_t>(width, 4, sourceUnpackAlignment);
        const uint8_t* source = static_cast<const uint8_t*>(sourceData);
        const uint8_t* endPointer = source + height * sourceElementsPerRow;
        unsigned int destinationElementsPerRow = width * destinationElementsPerPixel;
        while (source < endPointer) {
            rowPackingFunc(source, destinationData, width);
            source += sourceElementsPerRow;
            destinationData += destinationElementsPerRow;
        }
        break;
    }
    case GraphicsContext3D::SourceFormatRGBA16Little: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint16_t>(width, 8, sourceUnpackAlignment);
        doUnpackingAndPacking<uint16_t, uint8_t, DestType>(static_cast<const uint16_t*>(sourceData), unpackOneRowOfRGBA16LittleToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatRGBA16Big: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint16_t>(width, 8, sourceUnpackAlignment);
        doUnpackingAndPacking<uint16_t, uint8_t, DestType>(static_cast<const uint16_t*>(sourceData), unpackOneRowOfRGBA16BigToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatRGB8: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint8_t>(width, 3, sourceUnpackAlignment);
        doUnpackingAndPacking<uint8_t, uint8_t, DestType>(static_cast<const uint8_t*>(sourceData), unpackOneRowOfRGB8ToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatRGB16Little: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint16_t>(width, 6, sourceUnpackAlignment);
        doUnpackingAndPacking<uint16_t, uint8_t, DestType>(static_cast<const uint16_t*>(sourceData), unpackOneRowOfRGB16LittleToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatRGB16Big: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint16_t>(width, 6, sourceUnpackAlignment);
        doUnpackingAndPacking<uint16_t, uint8_t, DestType>(static_cast<const uint16_t*>(sourceData), unpackOneRowOfRGB16BigToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatBGR8: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint8_t>(width, 3, sourceUnpackAlignment);
        doUnpackingAndPacking<uint8_t, uint8_t, DestType>(static_cast<const uint8_t*>(sourceData), unpackOneRowOfBGR8ToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatARGB8: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint8_t>(width, 4, sourceUnpackAlignment);
        doUnpackingAndPacking<uint8_t, uint8_t, DestType>(static_cast<const uint8_t*>(sourceData), unpackOneRowOfARGB8ToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatARGB16Little: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint16_t>(width, 8, sourceUnpackAlignment);
        doUnpackingAndPacking<uint16_t, uint8_t, DestType>(static_cast<const uint16_t*>(sourceData), unpackOneRowOfARGB16LittleToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatARGB16Big: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint16_t>(width, 8, sourceUnpackAlignment);
        doUnpackingAndPacking<uint16_t, uint8_t, DestType>(static_cast<const uint16_t*>(sourceData), unpackOneRowOfARGB16BigToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatABGR8: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint8_t>(width, 4, sourceUnpackAlignment);
        doUnpackingAndPacking<uint8_t, uint8_t, DestType>(static_cast<const uint8_t*>(sourceData), unpackOneRowOfABGR8ToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatBGRA8: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint8_t>(width, 4, sourceUnpackAlignment);
        doUnpackingAndPacking<uint8_t, uint8_t, DestType>(static_cast<const uint8_t*>(sourceData), unpackOneRowOfBGRA8ToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatBGRA16Little: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint16_t>(width, 8, sourceUnpackAlignment);
        doUnpackingAndPacking<uint16_t, uint8_t, DestType>(static_cast<const uint16_t*>(sourceData), unpackOneRowOfBGRA16LittleToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatBGRA16Big: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint16_t>(width, 8, sourceUnpackAlignment);
        doUnpackingAndPacking<uint16_t, uint8_t, DestType>(static_cast<const uint16_t*>(sourceData), unpackOneRowOfBGRA16BigToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatRGBA5551: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint16_t>(width, 2, sourceUnpackAlignment);
        doUnpackingAndPacking<uint16_t, uint8_t, DestType>(static_cast<const uint16_t*>(sourceData), unpackOneRowOfRGBA5551ToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatRGBA4444: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint16_t>(width, 2, sourceUnpackAlignment);
        doUnpackingAndPacking<uint16_t, uint8_t, DestType>(static_cast<const uint16_t*>(sourceData), unpackOneRowOfRGBA4444ToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatRGB565: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint16_t>(width, 2, sourceUnpackAlignment);
        doUnpackingAndPacking<uint16_t, uint8_t, DestType>(static_cast<const uint16_t*>(sourceData), unpackOneRowOfRGB565ToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatR8: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint8_t>(width, 1, sourceUnpackAlignment);
        doUnpackingAndPacking<uint8_t, uint8_t, DestType>(static_cast<const uint8_t*>(sourceData), unpackOneRowOfR8ToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatR16Little: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint16_t>(width, 2, sourceUnpackAlignment);
        doUnpackingAndPacking<uint16_t, uint8_t, DestType>(static_cast<const uint16_t*>(sourceData), unpackOneRowOfR16LittleToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatR16Big: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint16_t>(width, 2, sourceUnpackAlignment);
        doUnpackingAndPacking<uint16_t, uint8_t, DestType>(static_cast<const uint16_t*>(sourceData), unpackOneRowOfR16BigToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatRA8: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint8_t>(width, 2, sourceUnpackAlignment);
        doUnpackingAndPacking<uint8_t, uint8_t, DestType>(static_cast<const uint8_t*>(sourceData), unpackOneRowOfRA8ToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatRA16Little: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint16_t>(width, 4, sourceUnpackAlignment);
        doUnpackingAndPacking<uint16_t, uint8_t, DestType>(static_cast<const uint16_t*>(sourceData), unpackOneRowOfRA16LittleToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatRA16Big: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint16_t>(width, 4, sourceUnpackAlignment);
        doUnpackingAndPacking<uint16_t, uint8_t, DestType>(static_cast<const uint16_t*>(sourceData), unpackOneRowOfRA16BigToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatAR8: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint8_t>(width, 2, sourceUnpackAlignment);
        doUnpackingAndPacking<uint8_t, uint8_t, DestType>(static_cast<const uint8_t*>(sourceData), unpackOneRowOfAR8ToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatAR16Little: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint16_t>(width, 4, sourceUnpackAlignment);
        doUnpackingAndPacking<uint16_t, uint8_t, DestType>(static_cast<const uint16_t*>(sourceData), unpackOneRowOfAR16LittleToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatAR16Big: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint16_t>(width, 4, sourceUnpackAlignment);
        doUnpackingAndPacking<uint16_t, uint8_t, DestType>(static_cast<const uint16_t*>(sourceData), unpackOneRowOfAR16BigToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatA8: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint8_t>(width, 1, sourceUnpackAlignment);
        doUnpackingAndPacking<uint8_t, uint8_t, DestType>(static_cast<const uint8_t*>(sourceData), unpackOneRowOfA8ToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatA16Little: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint16_t>(width, 2, sourceUnpackAlignment);
        doUnpackingAndPacking<uint16_t, uint8_t, DestType>(static_cast<const uint16_t*>(sourceData), unpackOneRowOfA16LittleToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatA16Big: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<uint16_t>(width, 2, sourceUnpackAlignment);
        doUnpackingAndPacking<uint16_t, uint8_t, DestType>(static_cast<const uint16_t*>(sourceData), unpackOneRowOfA16BigToRGBA8, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    default:
        ASSERT(false);
    }
}

// This specialized routine is used only for floating-point texture uploads. It
// does not need to be as general as doPacking, above; because there are
// currently no native floating-point image formats in WebKit, there are only a
// few upload paths.
static void doFloatingPointPacking(const void* sourceData,
                                   GraphicsContext3D::SourceDataFormat sourceDataFormat,
                                   unsigned int width,
                                   unsigned int height,
                                   unsigned int sourceUnpackAlignment,
                                   float* destinationData,
                                   void rowPackingFunc(const float*, float*, unsigned int),
                                   unsigned int destinationElementsPerPixel)
{
    switch (sourceDataFormat) {
    case GraphicsContext3D::SourceFormatRGBA8: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<float>(width, 4, sourceUnpackAlignment);
        const float* source = static_cast<const float*>(sourceData);
        const float* endPointer = source + height * sourceElementsPerRow;
        unsigned int destinationElementsPerRow = width * destinationElementsPerPixel;
        while (source < endPointer) {
            rowPackingFunc(source, destinationData, width);
            source += sourceElementsPerRow;
            destinationData += destinationElementsPerRow;
        }
        break;
    }
    case GraphicsContext3D::SourceFormatRGB32F: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<float>(width, 3, sourceUnpackAlignment);
        doUnpackingAndPacking<float, float, float>(static_cast<const float*>(sourceData), unpackOneRowOfRGB32FToRGBA32F, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatR32F: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<float>(width, 1, sourceUnpackAlignment);
        doUnpackingAndPacking<float, float, float>(static_cast<const float*>(sourceData), unpackOneRowOfR32FToRGBA32F, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatRA32F: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<float>(width, 2, sourceUnpackAlignment);
        doUnpackingAndPacking<float, float, float>(static_cast<const float*>(sourceData), unpackOneRowOfRA32FToRGBA32F, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    case GraphicsContext3D::SourceFormatA32F: {
        unsigned int sourceElementsPerRow = computeSourceElementsPerRow<float>(width, 1, sourceUnpackAlignment);
        doUnpackingAndPacking<float, float, float>(static_cast<const float*>(sourceData), unpackOneRowOfA32FToRGBA32F, width, height, sourceElementsPerRow, destinationData, rowPackingFunc, destinationElementsPerPixel);
        break;
    }
    default:
        ASSERT_NOT_REACHED();
    }
}

bool GraphicsContext3D::packPixels(const uint8_t* sourceData,
                                   GraphicsContext3D::SourceDataFormat sourceDataFormat,
                                   unsigned int width,
                                   unsigned int height,
                                   unsigned int sourceUnpackAlignment,
                                   unsigned int destinationFormat,
                                   unsigned int destinationType,
                                   AlphaOp alphaOp,
                                   void* destinationData)
{
    switch (destinationType) {
    case UNSIGNED_BYTE: {
        uint8_t* destination = static_cast<uint8_t*>(destinationData);
        if (sourceDataFormat == SourceFormatRGBA8 && destinationFormat == RGBA && sourceUnpackAlignment <= 4 && alphaOp == AlphaDoNothing) {
            // No conversion necessary.
            memcpy(destinationData, sourceData, width * height * 4);
            break;
        }
        switch (destinationFormat) {
        case RGB:
            switch (alphaOp) {
            case AlphaDoNothing:
                doPacking<uint8_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToRGB8, 3);
                break;
            case AlphaDoPremultiply:
                doPacking<uint8_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToRGB8Premultiply, 3);
                break;
            case AlphaDoUnmultiply:
                doPacking<uint8_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToRGB8Unmultiply, 3);
                break;
            }
            break;
        case RGBA:
            switch (alphaOp) {
            case AlphaDoNothing:
                ASSERT(sourceDataFormat != SourceFormatRGBA8 || sourceUnpackAlignment > 4); // Handled above with fast case.
                doPacking<uint8_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToRGBA8, 4);
                break;
            case AlphaDoPremultiply:
                doPacking<uint8_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToRGBA8Premultiply, 4);
                break;
            case AlphaDoUnmultiply:
                doPacking<uint8_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToRGBA8Unmultiply, 4);
                break;
            default:
                ASSERT_NOT_REACHED();
            }
            break;
        case ALPHA:
            // From the desktop OpenGL conversion rules (OpenGL 2.1
            // specification, Table 3.15), the alpha channel is chosen
            // from the RGBA data.
            doPacking<uint8_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToA8, 1);
            break;
        case LUMINANCE:
            // From the desktop OpenGL conversion rules (OpenGL 2.1
            // specification, Table 3.15), the red channel is chosen
            // from the RGBA data.
            switch (alphaOp) {
            case AlphaDoNothing:
                doPacking<uint8_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToR8, 1);
                break;
            case AlphaDoPremultiply:
                doPacking<uint8_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToR8Premultiply, 1);
                break;
            case AlphaDoUnmultiply:
                doPacking<uint8_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToR8Unmultiply, 1);
                break;
            }
            break;
        case LUMINANCE_ALPHA:
            // From the desktop OpenGL conversion rules (OpenGL 2.1
            // specification, Table 3.15), the red and alpha channels
            // are chosen from the RGBA data.
            switch (alphaOp) {
            case AlphaDoNothing:
                doPacking<uint8_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToRA8, 2);
                break;
            case AlphaDoPremultiply:
                doPacking<uint8_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToRA8Premultiply, 2);
                break;
            case AlphaDoUnmultiply:
                doPacking<uint8_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToRA8Unmultiply, 2);
                break;
            }
            break;
        }
        break;
    }
    case UNSIGNED_SHORT_4_4_4_4: {
        uint16_t* destination = static_cast<uint16_t*>(destinationData);
        switch (alphaOp) {
        case AlphaDoNothing:
            doPacking<uint16_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToUnsignedShort4444, 1);
            break;
        case AlphaDoPremultiply:
            doPacking<uint16_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToUnsignedShort4444Premultiply, 1);
            break;
        case AlphaDoUnmultiply:
            doPacking<uint16_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToUnsignedShort4444Unmultiply, 1);
            break;
        }
        break;
    }
    case UNSIGNED_SHORT_5_5_5_1: {
        uint16_t* destination = static_cast<uint16_t*>(destinationData);
        switch (alphaOp) {
        case AlphaDoNothing:
            doPacking<uint16_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToUnsignedShort5551, 1);
            break;
        case AlphaDoPremultiply:
            doPacking<uint16_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToUnsignedShort5551Premultiply, 1);
            break;
        case AlphaDoUnmultiply:
            doPacking<uint16_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToUnsignedShort5551Unmultiply, 1);
            break;
        }
        break;
    }
    case UNSIGNED_SHORT_5_6_5: {
        uint16_t* destination = static_cast<uint16_t*>(destinationData);
        switch (alphaOp) {
        case AlphaDoNothing:
            doPacking<uint16_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToUnsignedShort565, 1);
            break;
        case AlphaDoPremultiply:
            doPacking<uint16_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToUnsignedShort565Premultiply, 1);
            break;
        case AlphaDoUnmultiply:
            doPacking<uint16_t>(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA8ToUnsignedShort565Unmultiply, 1);
            break;
        }
        break;
    }
    case FLOAT: {
        // OpenGL ES, and therefore WebGL, require that the format and
        // internalformat be identical, which implies that the source and
        // destination formats will both be floating-point in this branch -- at
        // least, until WebKit supports floating-point image formats natively.
        ASSERT(sourceDataFormat == SourceFormatRGBA32F || sourceDataFormat == SourceFormatRGB32F
               || sourceDataFormat == SourceFormatRA32F || sourceDataFormat == SourceFormatR32F
               || sourceDataFormat == SourceFormatA32F);
        // Because WebKit doesn't use floating-point color channels for anything
        // internally, there's no chance we have to do a (lossy) unmultiply
        // operation.
        ASSERT(alphaOp == AlphaDoNothing || alphaOp == AlphaDoPremultiply);
        // For the source formats with an even number of channels (RGBA32F,
        // RA32F) it is guaranteed that the pixel data is tightly packed because
        // unpack alignment <= sizeof(float) * number of channels.
        float* destination = static_cast<float*>(destinationData);
        if (alphaOp == AlphaDoNothing
            && ((sourceDataFormat == SourceFormatRGBA32F && destinationFormat == RGBA)
                || (sourceDataFormat == SourceFormatRA32F && destinationFormat == LUMINANCE_ALPHA))) {
            // No conversion necessary.
            int numChannels = (sourceDataFormat == SourceFormatRGBA32F ? 4 : 2);
            memcpy(destinationData, sourceData, width * height * numChannels * sizeof(float));
            break;
        }
        switch (destinationFormat) {
        case RGB:
            switch (alphaOp) {
            case AlphaDoNothing:
                doFloatingPointPacking(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA32FToRGB32F, 3);
                break;
            case AlphaDoPremultiply:
                doFloatingPointPacking(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA32FToRGB32FPremultiply, 3);
                break;
            default:
                ASSERT_NOT_REACHED();
            }
            break;
        case RGBA:
            // AlphaDoNothing is handled above with fast path.
            ASSERT(alphaOp == AlphaDoPremultiply);
            doFloatingPointPacking(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA32FToRGBA32FPremultiply, 4);
            break;
        case ALPHA:
            // From the desktop OpenGL conversion rules (OpenGL 2.1
            // specification, Table 3.15), the alpha channel is chosen
            // from the RGBA data.
            doFloatingPointPacking(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA32FToA32F, 1);
            break;
        case LUMINANCE:
            // From the desktop OpenGL conversion rules (OpenGL 2.1
            // specification, Table 3.15), the red channel is chosen
            // from the RGBA data.
            switch (alphaOp) {
            case AlphaDoNothing:
                doFloatingPointPacking(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA32FToR32F, 1);
                break;
            case AlphaDoPremultiply:
                doFloatingPointPacking(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA32FToR32FPremultiply, 1);
                break;
            default:
                ASSERT_NOT_REACHED();
            }
            break;
        case LUMINANCE_ALPHA:
            // From the desktop OpenGL conversion rules (OpenGL 2.1
            // specification, Table 3.15), the red and alpha channels
            // are chosen from the RGBA data.
            switch (alphaOp) {
            case AlphaDoNothing:
                doFloatingPointPacking(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA32FToRA32F, 2);
                break;
            case AlphaDoPremultiply:
                doFloatingPointPacking(sourceData, sourceDataFormat, width, height, sourceUnpackAlignment, destination, packOneRowOfRGBA32FToRA32FPremultiply, 2);
                break;
            default:
                ASSERT_NOT_REACHED();
            }
            break;
        }
        break;
    }
    }
    return true;
}

} // namespace WebCore

#endif // ENABLE(WEBGL)
