//
// Copyright (c) 2013-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// loadimage.h: Defines image loading functions

#ifndef LIBGLESV2_RENDERER_LOADIMAGE_H_
#define LIBGLESV2_RENDERER_LOADIMAGE_H_

#include "libGLESv2/angletypes.h"

#include <cstdint>

namespace rx
{

void LoadA8ToRGBA8(size_t width, size_t height, size_t depth,
                   const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                   uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadA8ToBGRA8(size_t width, size_t height, size_t depth,
                   const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                   uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadA8ToBGRA8_SSE2(size_t width, size_t height, size_t depth,
                        const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                        uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadA32FToRGBA32F(size_t width, size_t height, size_t depth,
                       const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                       uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadA16FToRGBA16F(size_t width, size_t height, size_t depth,
                       const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                       uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadL8ToRGBA8(size_t width, size_t height, size_t depth,
                   const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                   uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadL8ToBGRA8(size_t width, size_t height, size_t depth,
                   const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                   uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadL32FToRGBA32F(size_t width, size_t height, size_t depth,
                       const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                       uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadL16FToRGBA16F(size_t width, size_t height, size_t depth,
                       const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                       uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadLA8ToRGBA8(size_t width, size_t height, size_t depth,
                    const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                    uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadLA8ToBGRA8(size_t width, size_t height, size_t depth,
                    const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                    uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadLA32FToRGBA32F(size_t width, size_t height, size_t depth,
                        const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                        uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadLA16FToRGBA16F(size_t width, size_t height, size_t depth,
                        const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                        uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadRGB8ToBGRX8(size_t width, size_t height, size_t depth,
                     const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                     uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadRG8ToBGRX8(size_t width, size_t height, size_t depth,
                    const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                    uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadR8ToBGRX8(size_t width, size_t height, size_t depth,
                   const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                   uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadR5G6B5ToBGRA8(size_t width, size_t height, size_t depth,
                       const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                       uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadR5G6B5ToRGBA8(size_t width, size_t height, size_t depth,
                       const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                       uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadRGBA8ToBGRA8_SSE2(size_t width, size_t height, size_t depth,
                           const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                           uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadRGBA8ToBGRA8(size_t width, size_t height, size_t depth,
                      const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                      uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadRGBA4ToBGRA8(size_t width, size_t height, size_t depth,
                      const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                      uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadRGBA4ToRGBA8(size_t width, size_t height, size_t depth,
                      const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                      uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadBGRA4ToBGRA8(size_t width, size_t height, size_t depth,
                      const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                      uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadRGB5A1ToBGRA8(size_t width, size_t height, size_t depth,
                       const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                       uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadRGB5A1ToRGBA8(size_t width, size_t height, size_t depth,
                       const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                       uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadBGR5A1ToBGRA8(size_t width, size_t height, size_t depth,
                       const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                       uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadRGB10A2ToRGBA8(size_t width, size_t height, size_t depth,
                        const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                        uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadRGB16FToRGB9E5(size_t width, size_t height, size_t depth,
                          const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                          uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadRGB32FToRGB9E5(size_t width, size_t height, size_t depth,
                        const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                        uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadRGB16FToRG11B10F(size_t width, size_t height, size_t depth,
                          const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                          uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadRGB32FToRG11B10F(size_t width, size_t height, size_t depth,
                          const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                          uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadG8R24ToR24G8(size_t width, size_t height, size_t depth,
                      const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                      uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

template <typename type, size_t componentCount>
inline void LoadToNative(size_t width, size_t height, size_t depth,
                         const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                         uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

template <typename type, uint32_t fourthComponentBits>
inline void LoadToNative3To4(size_t width, size_t height, size_t depth,
                             const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                             uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

template <size_t componentCount>
inline void Load32FTo16F(size_t width, size_t height, size_t depth,
                         const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                         uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadRGB32FToRGBA16F(size_t width, size_t height, size_t depth,
                         const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                         uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

template <size_t blockWidth, size_t blockHeight, size_t blockSize>
inline void LoadCompressedToNative(size_t width, size_t height, size_t depth,
                                   const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                                   uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadR32ToR16(size_t width, size_t height, size_t depth,
                  const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                  uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

template <typename type, uint32_t firstBits, uint32_t secondBits, uint32_t thirdBits, uint32_t fourthBits>
inline void Initialize4ComponentData(size_t width, size_t height, size_t depth,
                                     uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

void LoadR32ToR24G8(size_t width, size_t height, size_t depth,
                    const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                    uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch);

template <typename T>
inline T *OffsetDataPointer(uint8_t *data, size_t y, size_t z, size_t rowPitch, size_t depthPitch);

template <typename T>
inline const T *OffsetDataPointer(const uint8_t *data, size_t y, size_t z, size_t rowPitch, size_t depthPitch);

}

#include "loadimage.inl"

#endif // LIBGLESV2_RENDERER_LOADIMAGE_H_
