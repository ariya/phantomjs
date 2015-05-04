//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexAttribImpl.h: Defines the abstract rx::VertexAttribImpl class.

#ifndef LIBGLESV2_RENDERER_VERTEXARRAYIMPL_H_
#define LIBGLESV2_RENDERER_VERTEXARRAYIMPL_H_

#include "common/angleutils.h"
#include "libGLESv2/Buffer.h"
#include "libGLESv2/VertexAttribute.h"

namespace rx
{

class VertexArrayImpl
{
  public:
    virtual ~VertexArrayImpl() { }

    virtual void setElementArrayBuffer(const gl::Buffer *buffer) = 0;
    virtual void setAttribute(size_t idx, const gl::VertexAttribute &attr) = 0;
    virtual void setAttributeDivisor(size_t idx, GLuint divisor) = 0;
    virtual void enableAttribute(size_t idx, bool enabledState) = 0;
};

}

#endif // LIBGLESV2_RENDERER_VERTEXARRAYIMPL_H_
