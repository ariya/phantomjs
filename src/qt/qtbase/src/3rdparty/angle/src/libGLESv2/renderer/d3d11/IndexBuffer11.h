//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// IndexBuffer11.h: Defines the D3D11 IndexBuffer implementation.

#ifndef LIBGLESV2_RENDERER_INDEXBUFFER11_H_
#define LIBGLESV2_RENDERER_INDEXBUFFER11_H_

#include "libGLESv2/renderer/IndexBuffer.h"

namespace rx
{
class Renderer11;

class IndexBuffer11 : public IndexBuffer
{
  public:
    explicit IndexBuffer11(Renderer11 *const renderer);
    virtual ~IndexBuffer11();

    virtual bool initialize(unsigned int bufferSize, GLenum indexType, bool dynamic);

    static IndexBuffer11 *makeIndexBuffer11(IndexBuffer *indexBuffer);

    virtual bool mapBuffer(unsigned int offset, unsigned int size, void** outMappedMemory);
    virtual bool unmapBuffer();

    virtual GLenum getIndexType() const;
    virtual unsigned int getBufferSize() const;
    virtual bool setSize(unsigned int bufferSize, GLenum indexType);

    virtual bool discard();

    DXGI_FORMAT getIndexFormat() const;
    ID3D11Buffer *getBuffer() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(IndexBuffer11);

    rx::Renderer11 *const mRenderer;

    ID3D11Buffer *mBuffer;
    unsigned int mBufferSize;
    GLenum mIndexType;
    bool mDynamicUsage;
};

}

#endif // LIBGLESV2_RENDERER_INDEXBUFFER11_H_