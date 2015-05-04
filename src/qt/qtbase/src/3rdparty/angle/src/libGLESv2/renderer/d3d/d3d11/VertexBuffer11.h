//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexBuffer11.h: Defines the D3D11 VertexBuffer implementation.

#ifndef LIBGLESV2_RENDERER_VERTEXBUFFER11_H_
#define LIBGLESV2_RENDERER_VERTEXBUFFER11_H_

#include "libGLESv2/renderer/d3d/VertexBuffer.h"

namespace rx
{
class Renderer11;

class VertexBuffer11 : public VertexBuffer
{
  public:
    explicit VertexBuffer11(Renderer11 *const renderer);
    virtual ~VertexBuffer11();

    virtual gl::Error initialize(unsigned int size, bool dynamicUsage);

    static VertexBuffer11 *makeVertexBuffer11(VertexBuffer *vetexBuffer);

    virtual gl::Error storeVertexAttributes(const gl::VertexAttribute &attrib, const gl::VertexAttribCurrentValueData &currentValue,
                                            GLint start, GLsizei count, GLsizei instances, unsigned int offset);

    virtual gl::Error getSpaceRequired(const gl::VertexAttribute &attrib, GLsizei count, GLsizei instances,
                                       unsigned int *outSpaceRequired) const;

    virtual unsigned int getBufferSize() const;
    virtual gl::Error setBufferSize(unsigned int size);
    virtual gl::Error discard();

    ID3D11Buffer *getBuffer() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexBuffer11);

    Renderer11 *const mRenderer;

    ID3D11Buffer *mBuffer;
    unsigned int mBufferSize;
    bool mDynamicUsage;
};

}

#endif // LIBGLESV2_RENDERER_VERTEXBUFFER11_H_
