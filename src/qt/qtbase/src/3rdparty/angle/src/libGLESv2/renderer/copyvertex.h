//
// Copyright (c) 2013-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// copyvertex.h: Defines vertex buffer copying and conversion functions

#ifndef LIBGLESV2_RENDERER_COPYVERTEX_H_
#define LIBGLESV2_RENDERER_COPYVERTEX_H_

#include "common/mathutil.h"

namespace rx
{

// 'widenDefaultValueBits' gives the default value for the alpha channel (4th component)
//  the sentinel value 0 means we do not want to widen the input or add an alpha channel
template <typename T, size_t componentCount, uint32_t widenDefaultValueBits>
inline void CopyNativeVertexData(const uint8_t *input, size_t stride, size_t count, uint8_t *output);

template <size_t componentCount>
inline void Copy32FixedTo32FVertexData(const uint8_t *input, size_t stride, size_t count, uint8_t *output);

template <typename T, size_t componentCount, bool normalized>
inline void CopyTo32FVertexData(const uint8_t *input, size_t stride, size_t count, uint8_t *output);

template <bool isSigned, bool normalized, bool toFloat>
inline void CopyXYZ10W2ToXYZW32FVertexData(const uint8_t *input, size_t stride, size_t count, uint8_t *output);

}

#include "copyvertex.inl"

#endif // LIBGLESV2_RENDERER_COPYVERTEX_H_
