//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Indexffer9.h: Defines the D3D9 IndexBuffer implementation.

#ifndef LIBGLESV2_RENDERER_INDEXBUFFER9_H_
#define LIBGLESV2_RENDERER_INDEXBUFFER9_H_

#include "libGLESv2/renderer/IndexBuffer.h"

namespace rx
{
class Renderer9;

class IndexBuffer9 : public IndexBuffer
{
  public:
    explicit IndexBuffer9(Renderer9 *const renderer);
    virtual ~IndexBuffer9();

    virtual bool initialize(unsigned int bufferSize, GLenum indexType, bool dynamic);

    static IndexBuffer9 *makeIndexBuffer9(IndexBuffer *indexBuffer);

    virtual bool mapBuffer(unsigned int offset, unsigned int size, void** outMappedMemory);
    virtual bool unmapBuffer();

    virtual GLenum getIndexType() const;
    virtual unsigned int getBufferSize() const;
    virtual bool setSize(unsigned int bufferSize, GLenum indexType);

    virtual bool discard();

    D3DFORMAT getIndexFormat() const;
    IDirect3DIndexBuffer9 *getBuffer() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(IndexBuffer9);

    rx::Renderer9 *const mRenderer;

    IDirect3DIndexBuffer9 *mIndexBuffer;
    unsigned int mBufferSize;
    GLenum mIndexType;
    bool mDynamic;
};

}

#endif // LIBGLESV2_RENDERER_INDEXBUFFER9_H_
