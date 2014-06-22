//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Contants.h: Defines some implementation specific and gl constants

#ifndef LIBGLESV2_CONSTANTS_H_
#define LIBGLESV2_CONSTANTS_H_

namespace gl
{

enum
{
    MAX_VERTEX_ATTRIBS = 16,
    MAX_TEXTURE_IMAGE_UNITS = 16,

    // Implementation upper limits, real maximums depend on the hardware
    IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS = 16,
    IMPLEMENTATION_MAX_COMBINED_TEXTURE_IMAGE_UNITS = MAX_TEXTURE_IMAGE_UNITS + IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS,    

    IMPLEMENTATION_MAX_VARYING_VECTORS = 32,
    IMPLEMENTATION_MAX_DRAW_BUFFERS = 8
};

const float ALIASED_LINE_WIDTH_RANGE_MIN = 1.0f;
const float ALIASED_LINE_WIDTH_RANGE_MAX = 1.0f;
const float ALIASED_POINT_SIZE_RANGE_MIN = 1.0f;

}

#endif // LIBGLESV2_CONSTANTS_H_
