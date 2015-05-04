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

    // Implementation upper limits, real maximums depend on the hardware
    IMPLEMENTATION_MAX_VARYING_VECTORS = 32,
    IMPLEMENTATION_MAX_DRAW_BUFFERS = 8,
    IMPLEMENTATION_MAX_FRAMEBUFFER_ATTACHMENTS = IMPLEMENTATION_MAX_DRAW_BUFFERS + 2, // 2 extra for depth and/or stencil buffers

    IMPLEMENTATION_MAX_VERTEX_SHADER_UNIFORM_BUFFERS = 16,
    IMPLEMENTATION_MAX_FRAGMENT_SHADER_UNIFORM_BUFFERS = 16,
    IMPLEMENTATION_MAX_COMBINED_SHADER_UNIFORM_BUFFERS = IMPLEMENTATION_MAX_VERTEX_SHADER_UNIFORM_BUFFERS +
                                                         IMPLEMENTATION_MAX_FRAGMENT_SHADER_UNIFORM_BUFFERS,

    IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_BUFFERS = 4,

    // These are the maximums the implementation can support
    // The actual GL caps are limited by the device caps
    // and should be queried from the Context
    IMPLEMENTATION_MAX_2D_TEXTURE_SIZE = 16384,
    IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE = 16384,
    IMPLEMENTATION_MAX_3D_TEXTURE_SIZE = 2048,
    IMPLEMENTATION_MAX_2D_ARRAY_TEXTURE_LAYERS = 2048,

    IMPLEMENTATION_MAX_TEXTURE_LEVELS = 15   // 1+log2 of MAX_TEXTURE_SIZE
};

}

#endif // LIBGLESV2_CONSTANTS_H_
