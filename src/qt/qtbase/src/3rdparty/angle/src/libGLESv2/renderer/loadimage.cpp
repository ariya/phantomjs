//
// Copyright (c) 2013-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// loadimage.cpp: Defines image loading functions.

#include "libGLESv2/renderer/loadimage.h"

namespace rx
{

void LoadA8ToRGBA8(size_t width, size_t height, size_t depth,
                   const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                   uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint8_t *source = OffsetDataPointer<uint8_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint32_t *dest = OffsetDataPointer<uint32_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                dest[x] = static_cast<uint32_t>(source[x]) << 24;
            }
        }
    }
}

void LoadA8ToBGRA8(size_t width, size_t height, size_t depth,
                   const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                   uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    // Same as loading to RGBA
    LoadA8ToRGBA8(width, height, depth, input, inputRowPitch, inputDepthPitch, output, outputRowPitch, outputDepthPitch);
}

void LoadA32FToRGBA32F(size_t width, size_t height, size_t depth,
                       const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                       uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const float *source = OffsetDataPointer<float>(input, y, z, inputRowPitch, inputDepthPitch);
            float *dest = OffsetDataPointer<float>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                dest[4 * x + 0] = 0.0f;
                dest[4 * x + 1] = 0.0f;
                dest[4 * x + 2] = 0.0f;
                dest[4 * x + 3] = source[x];
            }
        }
    }
}

void LoadA16FToRGBA16F(size_t width, size_t height, size_t depth,
                       const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                       uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint16_t *source = OffsetDataPointer<uint16_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint16_t *dest = OffsetDataPointer<uint16_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                dest[4 * x + 0] = 0;
                dest[4 * x + 1] = 0;
                dest[4 * x + 2] = 0;
                dest[4 * x + 3] = source[x];
            }
        }
    }
}

void LoadL8ToRGBA8(size_t width, size_t height, size_t depth,
                   const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                   uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint8_t *source = OffsetDataPointer<uint8_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint8_t *dest = OffsetDataPointer<uint8_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                dest[4 * x + 0] = source[x];
                dest[4 * x + 1] = source[x];
                dest[4 * x + 2] = source[x];
                dest[4 * x + 3] = 0xFF;
            }
        }
    }
}

void LoadL8ToBGRA8(size_t width, size_t height, size_t depth,
                   const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                   uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    // Same as loading to RGBA
    LoadL8ToRGBA8(width, height, depth, input, inputRowPitch, inputDepthPitch, output, outputRowPitch, outputDepthPitch);
}

void LoadL32FToRGBA32F(size_t width, size_t height, size_t depth,
                       const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                       uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const float *source = OffsetDataPointer<float>(input, y, z, inputRowPitch, inputDepthPitch);
            float *dest = OffsetDataPointer<float>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                dest[4 * x + 0] = source[x];
                dest[4 * x + 1] = source[x];
                dest[4 * x + 2] = source[x];
                dest[4 * x + 3] = 1.0f;
            }
        }
    }
}

void LoadL16FToRGBA16F(size_t width, size_t height, size_t depth,
                       const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                       uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint16_t *source = OffsetDataPointer<uint16_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint16_t *dest = OffsetDataPointer<uint16_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                dest[4 * x + 0] = source[x];
                dest[4 * x + 1] = source[x];
                dest[4 * x + 2] = source[x];
                dest[4 * x + 3] = gl::Float16One;
            }
        }
    }
}

void LoadLA8ToRGBA8(size_t width, size_t height, size_t depth,
                    const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                    uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint8_t *source = OffsetDataPointer<uint8_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint8_t *dest = OffsetDataPointer<uint8_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                dest[4 * x + 0] = source[2 * x + 0];
                dest[4 * x + 1] = source[2 * x + 0];
                dest[4 * x + 2] = source[2 * x + 0];
                dest[4 * x + 3] = source[2 * x + 1];
            }
        }
    }
}

void LoadLA8ToBGRA8(size_t width, size_t height, size_t depth,
                    const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                    uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    // Same as loading to RGBA
    LoadLA8ToRGBA8(width, height, depth, input, inputRowPitch, inputDepthPitch, output, outputRowPitch, outputDepthPitch);
}

void LoadLA32FToRGBA32F(size_t width, size_t height, size_t depth,
                        const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                        uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const float *source = OffsetDataPointer<float>(input, y, z, inputRowPitch, inputDepthPitch);
            float *dest = OffsetDataPointer<float>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                dest[4 * x + 0] = source[2 * x + 0];
                dest[4 * x + 1] = source[2 * x + 0];
                dest[4 * x + 2] = source[2 * x + 0];
                dest[4 * x + 3] = source[2 * x + 1];
            }
        }
    }
}

void LoadLA16FToRGBA16F(size_t width, size_t height, size_t depth,
                        const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                        uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint16_t *source = OffsetDataPointer<uint16_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint16_t *dest = OffsetDataPointer<uint16_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                dest[4 * x + 0] = source[2 * x + 0];
                dest[4 * x + 1] = source[2 * x + 0];
                dest[4 * x + 2] = source[2 * x + 0];
                dest[4 * x + 3] = source[2 * x + 1];
            }
        }
    }
}

void LoadRGB8ToBGRX8(size_t width, size_t height, size_t depth,
                     const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                     uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint8_t *source = OffsetDataPointer<uint8_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint8_t *dest = OffsetDataPointer<uint8_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                dest[4 * x + 0] = source[x * 3 + 2];
                dest[4 * x + 1] = source[x * 3 + 1];
                dest[4 * x + 2] = source[x * 3 + 0];
                dest[4 * x + 3] = 0xFF;
            }
        }
    }
}

void LoadRG8ToBGRX8(size_t width, size_t height, size_t depth,
                    const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                    uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint8_t *source = OffsetDataPointer<uint8_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint8_t *dest = OffsetDataPointer<uint8_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                dest[4 * x + 0] = 0x00;
                dest[4 * x + 1] = source[x * 2 + 1];
                dest[4 * x + 2] = source[x * 2 + 0];
                dest[4 * x + 3] = 0xFF;
            }
        }
    }
}

void LoadR8ToBGRX8(size_t width, size_t height, size_t depth,
                   const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                   uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint8_t *source = OffsetDataPointer<uint8_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint8_t *dest = OffsetDataPointer<uint8_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                dest[4 * x + 0] = 0x00;
                dest[4 * x + 1] = 0x00;
                dest[4 * x + 2] = source[x];
                dest[4 * x + 3] = 0xFF;
            }
        }
    }
}

void LoadR5G6B5ToBGRA8(size_t width, size_t height, size_t depth,
                       const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                       uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint16_t *source = OffsetDataPointer<uint16_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint8_t *dest = OffsetDataPointer<uint8_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                uint16_t rgb = source[x];
                dest[4 * x + 0] = ((rgb & 0x001F) << 3) | ((rgb & 0x001F) >> 2);
                dest[4 * x + 1] = ((rgb & 0x07E0) >> 3) | ((rgb & 0x07E0) >> 9);
                dest[4 * x + 2] = ((rgb & 0xF800) >> 8) | ((rgb & 0xF800) >> 13);
                dest[4 * x + 3] = 0xFF;
            }
        }
    }
}

void LoadR5G6B5ToRGBA8(size_t width, size_t height, size_t depth,
                       const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                       uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint16_t *source = OffsetDataPointer<uint16_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint8_t *dest = OffsetDataPointer<uint8_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                uint16_t rgb = source[x];
                dest[4 * x + 0] = ((rgb & 0xF800) >> 8) | ((rgb & 0xF800) >> 13);
                dest[4 * x + 1] = ((rgb & 0x07E0) >> 3) | ((rgb & 0x07E0) >> 9);
                dest[4 * x + 2] = ((rgb & 0x001F) << 3) | ((rgb & 0x001F) >> 2);
                dest[4 * x + 3] = 0xFF;
            }
        }
    }
}

void LoadRGBA8ToBGRA8(size_t width, size_t height, size_t depth,
                      const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                      uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint32_t *source = OffsetDataPointer<uint32_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint32_t *dest = OffsetDataPointer<uint32_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                uint32_t rgba = source[x];
                dest[x] = (_rotl(rgba, 16) & 0x00ff00ff) | (rgba & 0xff00ff00);
            }
        }
    }
}

void LoadRGBA4ToBGRA8(size_t width, size_t height, size_t depth,
                      const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                      uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint16_t *source = OffsetDataPointer<uint16_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint8_t *dest = OffsetDataPointer<uint8_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                uint16_t rgba = source[x];
                dest[4 * x + 0] = ((rgba & 0x00F0) << 0) | ((rgba & 0x00F0) >> 4);
                dest[4 * x + 1] = ((rgba & 0x0F00) >> 4) | ((rgba & 0x0F00) >> 8);
                dest[4 * x + 2] = ((rgba & 0xF000) >> 8) | ((rgba & 0xF000) >> 12);
                dest[4 * x + 3] = ((rgba & 0x000F) << 4) | ((rgba & 0x000F) >> 0);
            }
        }
    }
}

void LoadRGBA4ToRGBA8(size_t width, size_t height, size_t depth,
                      const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                      uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint16_t *source = OffsetDataPointer<uint16_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint8_t *dest = OffsetDataPointer<uint8_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                uint16_t rgba = source[x];
                dest[4 * x + 0] = ((rgba & 0xF000) >> 8) | ((rgba & 0xF000) >> 12);
                dest[4 * x + 1] = ((rgba & 0x0F00) >> 4) | ((rgba & 0x0F00) >> 8);
                dest[4 * x + 2] = ((rgba & 0x00F0) << 0) | ((rgba & 0x00F0) >> 4);
                dest[4 * x + 3] = ((rgba & 0x000F) << 4) | ((rgba & 0x000F) >> 0);
            }
        }
    }
}

void LoadBGRA4ToBGRA8(size_t width, size_t height, size_t depth,
                      const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                      uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint16_t *source = OffsetDataPointer<uint16_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint8_t *dest = OffsetDataPointer<uint8_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                uint16_t bgra = source[x];
                dest[4 * x + 0] = ((bgra & 0xF000) >> 8) | ((bgra & 0xF000) >> 12);
                dest[4 * x + 1] = ((bgra & 0x0F00) >> 4) | ((bgra & 0x0F00) >> 8);
                dest[4 * x + 2] = ((bgra & 0x00F0) << 0) | ((bgra & 0x00F0) >> 4);
                dest[4 * x + 3] = ((bgra & 0x000F) << 4) | ((bgra & 0x000F) >> 0);
            }
        }
    }
}

void LoadRGB5A1ToBGRA8(size_t width, size_t height, size_t depth,
                       const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                       uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint16_t *source = OffsetDataPointer<uint16_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint8_t *dest = OffsetDataPointer<uint8_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                uint16_t rgba = source[x];
                dest[4 * x + 0] = ((rgba & 0x003E) << 2) | ((rgba & 0x003E) >> 3);
                dest[4 * x + 1] = ((rgba & 0x07C0) >> 3) | ((rgba & 0x07C0) >> 8);
                dest[4 * x + 2] = ((rgba & 0xF800) >> 8) | ((rgba & 0xF800) >> 13);
                dest[4 * x + 3] = (rgba & 0x0001) ? 0xFF : 0;
            }
        }
    }
}

void LoadRGB5A1ToRGBA8(size_t width, size_t height, size_t depth,
                       const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                       uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint16_t *source = OffsetDataPointer<uint16_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint8_t *dest = OffsetDataPointer<uint8_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                uint16_t rgba = source[x];
                dest[4 * x + 0] = ((rgba & 0xF800) >> 8) | ((rgba & 0xF800) >> 13);
                dest[4 * x + 1] = ((rgba & 0x07C0) >> 3) | ((rgba & 0x07C0) >> 8);
                dest[4 * x + 2] = ((rgba & 0x003E) << 2) | ((rgba & 0x003E) >> 3);
                dest[4 * x + 3] = (rgba & 0x0001) ? 0xFF : 0;
            }
        }
    }
}


void LoadBGR5A1ToBGRA8(size_t width, size_t height, size_t depth,
                       const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                       uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint16_t *source = OffsetDataPointer<uint16_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint8_t *dest = OffsetDataPointer<uint8_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                uint16_t bgra = source[x];
                dest[4 * x + 0] = ((bgra & 0xF800) >> 8) | ((bgra & 0xF800) >> 13);
                dest[4 * x + 1] = ((bgra & 0x07C0) >> 3) | ((bgra & 0x07C0) >> 8);
                dest[4 * x + 2] = ((bgra & 0x003E) << 2) | ((bgra & 0x003E) >> 3);
                dest[4 * x + 3] = (bgra & 0x0001) ? 0xFF : 0;
            }
        }
    }
}

void LoadRGB10A2ToRGBA8(size_t width, size_t height, size_t depth,
                        const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                        uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint32_t *source = OffsetDataPointer<uint32_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint8_t *dest = OffsetDataPointer<uint8_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                uint32_t rgba = source[x];
                dest[4 * x + 0] = (rgba & 0x000003FF) >>  2;
                dest[4 * x + 1] = (rgba & 0x000FFC00) >> 12;
                dest[4 * x + 2] = (rgba & 0x3FF00000) >> 22;
                dest[4 * x + 3] = ((rgba & 0xC0000000) >> 30) * 0x55;
            }
        }
    }
}

void LoadRGB16FToRGB9E5(size_t width, size_t height, size_t depth,
                        const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                        uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint16_t *source = OffsetDataPointer<uint16_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint32_t *dest = OffsetDataPointer<uint32_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                dest[x] = gl::convertRGBFloatsTo999E5(gl::float16ToFloat32(source[x * 3 + 0]),
                                                      gl::float16ToFloat32(source[x * 3 + 1]),
                                                      gl::float16ToFloat32(source[x * 3 + 2]));
            }
        }
    }
}

void LoadRGB32FToRGB9E5(size_t width, size_t height, size_t depth,
                        const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                        uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const float *source = OffsetDataPointer<float>(input, y, z, inputRowPitch, inputDepthPitch);
            uint32_t *dest = OffsetDataPointer<uint32_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                dest[x] = gl::convertRGBFloatsTo999E5(source[x * 3 + 0], source[x * 3 + 1], source[x * 3 + 2]);
            }
        }
    }
}

void LoadRGB16FToRG11B10F(size_t width, size_t height, size_t depth,
                          const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                          uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint16_t *source = OffsetDataPointer<uint16_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint32_t *dest = OffsetDataPointer<uint32_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                dest[x] = (gl::float32ToFloat11(gl::float16ToFloat32(source[x * 3 + 0])) <<  0) |
                          (gl::float32ToFloat11(gl::float16ToFloat32(source[x * 3 + 1])) << 11) |
                          (gl::float32ToFloat10(gl::float16ToFloat32(source[x * 3 + 2])) << 22);
            }
        }
    }
}

void LoadRGB32FToRG11B10F(size_t width, size_t height, size_t depth,
                          const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                          uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const float *source = OffsetDataPointer<float>(input, y, z, inputRowPitch, inputDepthPitch);
            uint32_t *dest = OffsetDataPointer<uint32_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                dest[x] = (gl::float32ToFloat11(source[x * 3 + 0]) <<  0) |
                          (gl::float32ToFloat11(source[x * 3 + 1]) << 11) |
                          (gl::float32ToFloat10(source[x * 3 + 2]) << 22);
            }
        }
    }
}

void LoadG8R24ToR24G8(size_t width, size_t height, size_t depth,
                      const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                      uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint32_t *source = OffsetDataPointer<uint32_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint32_t *dest = OffsetDataPointer<uint32_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                uint32_t d = source[x] >> 8;
                uint8_t  s = source[x] & 0xFF;
                dest[x] = d | (s << 24);
            }
        }
    }
}

void LoadRGB32FToRGBA16F(size_t width, size_t height, size_t depth,
                         const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                         uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const float *source = OffsetDataPointer<float>(input, y, z, inputRowPitch, inputDepthPitch);
            uint16_t *dest = OffsetDataPointer<uint16_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                dest[x * 4 + 0] = gl::float32ToFloat16(source[x * 3 + 0]);
                dest[x * 4 + 1] = gl::float32ToFloat16(source[x * 3 + 1]);
                dest[x * 4 + 2] = gl::float32ToFloat16(source[x * 3 + 2]);
                dest[x * 4 + 3] = gl::Float16One;
            }
        }
    }
}

void LoadR32ToR16(size_t width, size_t height, size_t depth,
                  const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                  uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint32_t *source = OffsetDataPointer<uint32_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint16_t *dest = OffsetDataPointer<uint16_t>(output, y, z, outputRowPitch, outputDepthPitch);
            for (size_t x = 0; x < width; x++)
            {
                dest[x] = source[x] >> 16;
            }
        }
    }
}

void LoadR32ToR24G8(size_t width, size_t height, size_t depth,
                    const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                    uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint32_t *source = OffsetDataPointer<uint32_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint32_t *dest = OffsetDataPointer<uint32_t>(output, y, z, outputRowPitch, outputDepthPitch);

            for (size_t x = 0; x < width; x++)
            {
                dest[x] = source[x] >> 8;
            }
        }
    }
}

}
