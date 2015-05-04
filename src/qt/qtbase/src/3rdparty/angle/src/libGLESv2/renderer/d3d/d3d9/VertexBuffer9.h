//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexBuffer9.h: Defines the D3D9 VertexBuffer implementation.

#ifndef LIBGLESV2_RENDERER_VERTEXBUFFER9_H_
#define LIBGLESV2_RENDERER_VERTEXBUFFER9_H_

#include "libGLESv2/renderer/d3d/VertexBuffer.h"

namespace rx
{
class Renderer9;

class VertexBuffer9 : public VertexBuffer
{
  public:
    explicit VertexBuffer9(Renderer9 *renderer);
    virtual ~VertexBuffer9();

    virtual gl::Error initialize(unsigned int size, bool dynamicUsage);

    static VertexBuffer9 *makeVertexBuffer9(VertexBuffer *vertexBuffer);

    virtual gl::Error storeVertexAttributes(const gl::VertexAttribute &attrib, const gl::VertexAttribCurrentValueData &currentValue,
                                            GLint start, GLsizei count, GLsizei instances, unsigned int offset);

    virtual gl::Error getSpaceRequired(const gl::VertexAttribute &attrib, GLsizei count, GLsizei instances, unsigned int *outSpaceRequired) const;

    virtual unsigned int getBufferSize() const;
    virtual gl::Error setBufferSize(unsigned int size);
    virtual gl::Error discard();

    IDirect3DVertexBuffer9 *getBuffer() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexBuffer9);

    Renderer9 *mRenderer;

    IDirect3DVertexBuffer9 *mVertexBuffer;
    unsigned int mBufferSize;
    bool mDynamicUsage;

    gl::Error spaceRequired(const gl::VertexAttribute &attrib, std::size_t count, GLsizei instances,
                            unsigned int *outSpaceRequired) const;
};

}

#endif // LIBGLESV2_RENDERER_VERTEXBUFFER9_H_
