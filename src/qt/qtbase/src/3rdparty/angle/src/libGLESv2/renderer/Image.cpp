#include "precompiled.h"
//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Image.h: Implements the rx::Image class, an abstract base class for the 
// renderer-specific classes which will define the interface to the underlying
// surfaces or resources.

#include "libGLESv2/renderer/Image.h"

namespace rx
{

Image::Image()
{
    mWidth = 0; 
    mHeight = 0;
    mInternalFormat = GL_NONE;
    mActualFormat = GL_NONE;
}

void Image::loadAlphaDataToBGRA(GLsizei width, GLsizei height,
                                int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const unsigned char *source = NULL;
    unsigned char *dest = NULL;
    
    for (int y = 0; y < height; y++)
    {
        source = static_cast<const unsigned char*>(input) + y * inputPitch;
        dest = static_cast<unsigned char*>(output) + y * outputPitch;
        for (int x = 0; x < width; x++)
        {
            dest[4 * x + 0] = 0;
            dest[4 * x + 1] = 0;
            dest[4 * x + 2] = 0;
            dest[4 * x + 3] = source[x];
        }
    }
}

void Image::loadAlphaDataToNative(GLsizei width, GLsizei height,
                                  int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const unsigned char *source = NULL;
    unsigned char *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = static_cast<const unsigned char*>(input) + y * inputPitch;
        dest = static_cast<unsigned char*>(output) + y * outputPitch;
        memcpy(dest, source, width);
    }
}

void Image::loadAlphaFloatDataToRGBA(GLsizei width, GLsizei height,
                                     int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const float *source = NULL;
    float *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const float*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<float*>(static_cast<unsigned char*>(output) + y * outputPitch);
        for (int x = 0; x < width; x++)
        {
            dest[4 * x + 0] = 0;
            dest[4 * x + 1] = 0;
            dest[4 * x + 2] = 0;
            dest[4 * x + 3] = source[x];
        }
    }
}

void Image::loadAlphaHalfFloatDataToRGBA(GLsizei width, GLsizei height,
                                         int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const unsigned short *source = NULL;
    unsigned short *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const unsigned short*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<unsigned short*>(static_cast<unsigned char*>(output) + y * outputPitch);
        for (int x = 0; x < width; x++)
        {
            dest[4 * x + 0] = 0;
            dest[4 * x + 1] = 0;
            dest[4 * x + 2] = 0;
            dest[4 * x + 3] = source[x];
        }
    }
}

void Image::loadLuminanceDataToNativeOrBGRA(GLsizei width, GLsizei height,
                                            int inputPitch, const void *input, size_t outputPitch, void *output, bool native)
{
    const unsigned char *source = NULL;
    unsigned char *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = static_cast<const unsigned char*>(input) + y * inputPitch;
        dest = static_cast<unsigned char*>(output) + y * outputPitch;

        if (!native)   // BGRA8 destination format
        {
            for (int x = 0; x < width; x++)
            {
                dest[4 * x + 0] = source[x];
                dest[4 * x + 1] = source[x];
                dest[4 * x + 2] = source[x];
                dest[4 * x + 3] = 0xFF;
            }
        }
        else   // L8 destination format
        {
            memcpy(dest, source, width);
        }
    }
}

void Image::loadLuminanceFloatDataToRGBA(GLsizei width, GLsizei height,
                                         int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const float *source = NULL;
    float *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const float*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<float*>(static_cast<unsigned char*>(output) + y * outputPitch);
        for (int x = 0; x < width; x++)
        {
            dest[4 * x + 0] = source[x];
            dest[4 * x + 1] = source[x];
            dest[4 * x + 2] = source[x];
            dest[4 * x + 3] = 1.0f;
        }
    }
}

void Image::loadLuminanceFloatDataToRGB(GLsizei width, GLsizei height,
                                        int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const float *source = NULL;
    float *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const float*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<float*>(static_cast<unsigned char*>(output) + y * outputPitch);
        for (int x = 0; x < width; x++)
        {
            dest[3 * x + 0] = source[x];
            dest[3 * x + 1] = source[x];
            dest[3 * x + 2] = source[x];
        }
    }
}

void Image::loadLuminanceHalfFloatDataToRGBA(GLsizei width, GLsizei height,
                                             int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const unsigned short *source = NULL;
    unsigned short *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const unsigned short*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<unsigned short*>(static_cast<unsigned char*>(output) + y * outputPitch);
        for (int x = 0; x < width; x++)
        {
            dest[4 * x + 0] = source[x];
            dest[4 * x + 1] = source[x];
            dest[4 * x + 2] = source[x];
            dest[4 * x + 3] = 0x3C00; // SEEEEEMMMMMMMMMM, S = 0, E = 15, M = 0: 16bit flpt representation of 1
        }
    }
}

void Image::loadLuminanceAlphaDataToNativeOrBGRA(GLsizei width, GLsizei height,
                                                 int inputPitch, const void *input, size_t outputPitch, void *output, bool native)
{
    const unsigned char *source = NULL;
    unsigned char *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = static_cast<const unsigned char*>(input) + y * inputPitch;
        dest = static_cast<unsigned char*>(output) + y * outputPitch;
        
        if (!native)   // BGRA8 destination format
        {
            for (int x = 0; x < width; x++)
            {
                dest[4 * x + 0] = source[2*x+0];
                dest[4 * x + 1] = source[2*x+0];
                dest[4 * x + 2] = source[2*x+0];
                dest[4 * x + 3] = source[2*x+1];
            }
        }
        else
        {
            memcpy(dest, source, width * 2);
        }
    }
}

void Image::loadLuminanceAlphaFloatDataToRGBA(GLsizei width, GLsizei height,
                                              int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const float *source = NULL;
    float *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const float*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<float*>(static_cast<unsigned char*>(output) + y * outputPitch);
        for (int x = 0; x < width; x++)
        {
            dest[4 * x + 0] = source[2*x+0];
            dest[4 * x + 1] = source[2*x+0];
            dest[4 * x + 2] = source[2*x+0];
            dest[4 * x + 3] = source[2*x+1];
        }
    }
}

void Image::loadLuminanceAlphaHalfFloatDataToRGBA(GLsizei width, GLsizei height,
                                                  int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const unsigned short *source = NULL;
    unsigned short *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const unsigned short*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<unsigned short*>(static_cast<unsigned char*>(output) + y * outputPitch);
        for (int x = 0; x < width; x++)
        {
            dest[4 * x + 0] = source[2*x+0];
            dest[4 * x + 1] = source[2*x+0];
            dest[4 * x + 2] = source[2*x+0];
            dest[4 * x + 3] = source[2*x+1];
        }
    }
}

void Image::loadRGBUByteDataToBGRX(GLsizei width, GLsizei height,
                                   int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const unsigned char *source = NULL;
    unsigned char *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = static_cast<const unsigned char*>(input) + y * inputPitch;
        dest = static_cast<unsigned char*>(output) + y * outputPitch;
        for (int x = 0; x < width; x++)
        {
            dest[4 * x + 0] = source[x * 3 + 2];
            dest[4 * x + 1] = source[x * 3 + 1];
            dest[4 * x + 2] = source[x * 3 + 0];
            dest[4 * x + 3] = 0xFF;
        }
    }
}

void Image::loadRGBUByteDataToRGBA(GLsizei width, GLsizei height,
                                   int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const unsigned char *source = NULL;
    unsigned char *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = static_cast<const unsigned char*>(input) + y * inputPitch;
        dest = static_cast<unsigned char*>(output) + y * outputPitch;
        for (int x = 0; x < width; x++)
        {
            dest[4 * x + 0] = source[x * 3 + 0];
            dest[4 * x + 1] = source[x * 3 + 1];
            dest[4 * x + 2] = source[x * 3 + 2];
            dest[4 * x + 3] = 0xFF;
        }
    }
}

void Image::loadRGB565DataToBGRA(GLsizei width, GLsizei height,
                                 int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const unsigned short *source = NULL;
    unsigned char *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const unsigned short*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = static_cast<unsigned char*>(output) + y * outputPitch;
        for (int x = 0; x < width; x++)
        {
            unsigned short rgba = source[x];
            dest[4 * x + 0] = ((rgba & 0x001F) << 3) | ((rgba & 0x001F) >> 2);
            dest[4 * x + 1] = ((rgba & 0x07E0) >> 3) | ((rgba & 0x07E0) >> 9);
            dest[4 * x + 2] = ((rgba & 0xF800) >> 8) | ((rgba & 0xF800) >> 13);
            dest[4 * x + 3] = 0xFF;
        }
    }
}

void Image::loadRGB565DataToRGBA(GLsizei width, GLsizei height,
                                 int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const unsigned short *source = NULL;
    unsigned char *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const unsigned short*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = static_cast<unsigned char*>(output) + y * outputPitch;
        for (int x = 0; x < width; x++)
        {
            unsigned short rgba = source[x];
            dest[4 * x + 0] = ((rgba & 0xF800) >> 8) | ((rgba & 0xF800) >> 13);
            dest[4 * x + 1] = ((rgba & 0x07E0) >> 3) | ((rgba & 0x07E0) >> 9);
            dest[4 * x + 2] = ((rgba & 0x001F) << 3) | ((rgba & 0x001F) >> 2);
            dest[4 * x + 3] = 0xFF;
        }
    }
}

void Image::loadRGBFloatDataToRGBA(GLsizei width, GLsizei height,
                                   int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const float *source = NULL;
    float *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const float*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<float*>(static_cast<unsigned char*>(output) + y * outputPitch);
        for (int x = 0; x < width; x++)
        {
            dest[4 * x + 0] = source[x * 3 + 0];
            dest[4 * x + 1] = source[x * 3 + 1];
            dest[4 * x + 2] = source[x * 3 + 2];
            dest[4 * x + 3] = 1.0f;
        }
    }
}

void Image::loadRGBFloatDataToNative(GLsizei width, GLsizei height,
                                     int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const float *source = NULL;
    float *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const float*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<float*>(static_cast<unsigned char*>(output) + y * outputPitch);
        memcpy(dest, source, width * 12);
    }
}

void Image::loadRGBHalfFloatDataToRGBA(GLsizei width, GLsizei height,
                                       int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const unsigned short *source = NULL;
    unsigned short *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const unsigned short*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<unsigned short*>(static_cast<unsigned char*>(output) + y * outputPitch);
        for (int x = 0; x < width; x++)
        {
            dest[4 * x + 0] = source[x * 3 + 0];
            dest[4 * x + 1] = source[x * 3 + 1];
            dest[4 * x + 2] = source[x * 3 + 2];
            dest[4 * x + 3] = 0x3C00; // SEEEEEMMMMMMMMMM, S = 0, E = 15, M = 0: 16bit flpt representation of 1
        }
    }
}

void Image::loadRGBAUByteDataToBGRA(GLsizei width, GLsizei height,
                                    int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const unsigned int *source = NULL;
    unsigned int *dest = NULL;
    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const unsigned int*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<unsigned int*>(static_cast<unsigned char*>(output) + y * outputPitch);

        for (int x = 0; x < width; x++)
        {
            unsigned int rgba = source[x];
            dest[x] = (_rotl(rgba, 16) & 0x00ff00ff) | (rgba & 0xff00ff00);
        }
    }
}

void Image::loadRGBAUByteDataToNative(GLsizei width, GLsizei height,
                                      int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const unsigned int *source = NULL;
    unsigned int *dest = NULL;
    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const unsigned int*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<unsigned int*>(static_cast<unsigned char*>(output) + y * outputPitch);

        memcpy(dest, source, width * 4);
    }
}

void Image::loadRGBA4444DataToBGRA(GLsizei width, GLsizei height,
                                   int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const unsigned short *source = NULL;
    unsigned char *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const unsigned short*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = static_cast<unsigned char*>(output) + y * outputPitch;
        for (int x = 0; x < width; x++)
        {
            unsigned short rgba = source[x];
            dest[4 * x + 0] = ((rgba & 0x00F0) << 0) | ((rgba & 0x00F0) >> 4);
            dest[4 * x + 1] = ((rgba & 0x0F00) >> 4) | ((rgba & 0x0F00) >> 8);
            dest[4 * x + 2] = ((rgba & 0xF000) >> 8) | ((rgba & 0xF000) >> 12);
            dest[4 * x + 3] = ((rgba & 0x000F) << 4) | ((rgba & 0x000F) >> 0);
        }
    }
}

void Image::loadRGBA4444DataToRGBA(GLsizei width, GLsizei height,
                                   int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const unsigned short *source = NULL;
    unsigned char *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const unsigned short*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = static_cast<unsigned char*>(output) + y * outputPitch;
        for (int x = 0; x < width; x++)
        {
            unsigned short rgba = source[x];
            dest[4 * x + 0] = ((rgba & 0xF000) >> 8) | ((rgba & 0xF000) >> 12);
            dest[4 * x + 1] = ((rgba & 0x0F00) >> 4) | ((rgba & 0x0F00) >> 8);
            dest[4 * x + 2] = ((rgba & 0x00F0) << 0) | ((rgba & 0x00F0) >> 4);
            dest[4 * x + 3] = ((rgba & 0x000F) << 4) | ((rgba & 0x000F) >> 0);
        }
    }
}

void Image::loadRGBA5551DataToBGRA(GLsizei width, GLsizei height,
                                   int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const unsigned short *source = NULL;
    unsigned char *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const unsigned short*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = static_cast<unsigned char*>(output) + y * outputPitch;
        for (int x = 0; x < width; x++)
        {
            unsigned short rgba = source[x];
            dest[4 * x + 0] = ((rgba & 0x003E) << 2) | ((rgba & 0x003E) >> 3);
            dest[4 * x + 1] = ((rgba & 0x07C0) >> 3) | ((rgba & 0x07C0) >> 8);
            dest[4 * x + 2] = ((rgba & 0xF800) >> 8) | ((rgba & 0xF800) >> 13);
            dest[4 * x + 3] = (rgba & 0x0001) ? 0xFF : 0;
        }
    }
}

void Image::loadRGBA5551DataToRGBA(GLsizei width, GLsizei height,
                                   int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const unsigned short *source = NULL;
    unsigned char *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const unsigned short*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = static_cast<unsigned char*>(output) + y * outputPitch;
        for (int x = 0; x < width; x++)
        {
            unsigned short rgba = source[x];
            dest[4 * x + 0] = ((rgba & 0xF800) >> 8) | ((rgba & 0xF800) >> 13);
            dest[4 * x + 1] = ((rgba & 0x07C0) >> 3) | ((rgba & 0x07C0) >> 8);
            dest[4 * x + 2] = ((rgba & 0x003E) << 2) | ((rgba & 0x003E) >> 3);
            dest[4 * x + 3] = (rgba & 0x0001) ? 0xFF : 0;
        }
    }
}

void Image::loadRGBAFloatDataToRGBA(GLsizei width, GLsizei height,
                                    int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const float *source = NULL;
    float *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const float*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<float*>(static_cast<unsigned char*>(output) + y * outputPitch);
        memcpy(dest, source, width * 16);
    }
}

void Image::loadRGBAHalfFloatDataToRGBA(GLsizei width, GLsizei height,
                                        int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const unsigned char *source = NULL;
    unsigned char *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = static_cast<const unsigned char*>(input) + y * inputPitch;
        dest = static_cast<unsigned char*>(output) + y * outputPitch;
        memcpy(dest, source, width * 8);
    }
}

void Image::loadBGRADataToBGRA(GLsizei width, GLsizei height,
                               int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const unsigned char *source = NULL;
    unsigned char *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = static_cast<const unsigned char*>(input) + y * inputPitch;
        dest = static_cast<unsigned char*>(output) + y * outputPitch;
        memcpy(dest, source, width*4);
    }
}

}
