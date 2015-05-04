//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexArray9.h: Defines the rx::VertexArray9 class which implements rx::VertexArrayImpl.

#ifndef LIBGLESV2_RENDERER_VERTEXARRAY9_H_
#define LIBGLESV2_RENDERER_VERTEXARRAY9_H_

#include "libGLESv2/renderer/VertexArrayImpl.h"
#include "libGLESv2/renderer/d3d/d3d9/Renderer9.h"

namespace rx
{
class Renderer9;

class VertexArray9 : public VertexArrayImpl
{
  public:
    VertexArray9(Renderer9 *renderer)
        : VertexArrayImpl(),
          mRenderer(renderer)
    {
    }

    virtual ~VertexArray9() { }

    virtual void setElementArrayBuffer(const gl::Buffer *buffer) { }
    virtual void setAttribute(size_t idx, const gl::VertexAttribute &attr) { }
    virtual void setAttributeDivisor(size_t idx, GLuint divisor) { }
    virtual void enableAttribute(size_t idx, bool enabledState) { }

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexArray9);

    Renderer9 *mRenderer;
};

}

#endif // LIBGLESV2_RENDERER_VERTEXARRAY9_H_
