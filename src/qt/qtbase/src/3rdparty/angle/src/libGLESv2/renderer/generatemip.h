//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// generatemip.h: Defines the GenerateMip function, templated on the format
// type of the image for which mip levels are being generated.

#ifndef LIBGLESV2_RENDERER_GENERATEMIP_H_
#define LIBGLESV2_RENDERER_GENERATEMIP_H_

#include "libGLESv2/renderer/imageformats.h"
#include "libGLESv2/angletypes.h"

namespace rx
{

template <typename T>
inline void GenerateMip(size_t sourceWidth, size_t sourceHeight, size_t sourceDepth,
                        const uint8_t *sourceData, size_t sourceRowPitch, size_t sourceDepthPitch,
                        uint8_t *destData, size_t destRowPitch, size_t destDepthPitch);

}

#include "generatemip.inl"

#endif // LIBGLESV2_RENDERER_GENERATEMIP_H_
