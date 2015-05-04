//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexArray11.h: Defines the rx::VertexArray11 class which implements rx::VertexArrayImpl.

#ifndef LIBGLESV2_RENDERER_VERTEXARRAY11_H_
#define LIBGLESV2_RENDERER_VERTEXARRAY11_H_

#include "libGLESv2/renderer/VertexArrayImpl.h"
#include "libGLESv2/renderer/d3d/d3d11/Renderer11.h"

namespace rx
{
class Renderer11;

class VertexArray11 : public VertexArrayImpl
{
  public:
    VertexArray11(Renderer11 *renderer)
        : VertexArrayImpl(),
          mRenderer(renderer)
    {
    }
    virtual ~VertexArray11() { }

    virtual void setElementArrayBuffer(const gl::Buffer *buffer) { }
    virtual void setAttribute(size_t idx, const gl::VertexAttribute &attr) { }
    virtual void setAttributeDivisor(size_t idx, GLuint divisor) { }
    virtual void enableAttribute(size_t idx, bool enabledState) { }

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexArray11);

    Renderer11 *mRenderer;
};

}

#endif // LIBGLESV2_RENDERER_VERTEXARRAY11_H_
