//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// IndexBuffer.h: Defines the abstract IndexBuffer class and IndexBufferInterface
// class with derivations, classes that perform graphics API agnostic index buffer operations.

#ifndef LIBGLESV2_RENDERER_INDEXBUFFER_H_
#define LIBGLESV2_RENDERER_INDEXBUFFER_H_

#include "common/angleutils.h"
#include "libGLESv2/Error.h"
#include "libGLESv2/renderer/IndexRangeCache.h"

namespace rx
{
class RendererD3D;

class IndexBuffer
{
  public:
    IndexBuffer();
    virtual ~IndexBuffer();

    virtual gl::Error initialize(unsigned int bufferSize, GLenum indexType, bool dynamic) = 0;

    virtual gl::Error mapBuffer(unsigned int offset, unsigned int size, void** outMappedMemory) = 0;
    virtual gl::Error unmapBuffer() = 0;

    virtual gl::Error discard() = 0;

    virtual GLenum getIndexType() const = 0;
    virtual unsigned int getBufferSize() const = 0;
    virtual gl::Error setSize(unsigned int bufferSize, GLenum indexType) = 0;

    unsigned int getSerial() const;

  protected:
    void updateSerial();

  private:
    DISALLOW_COPY_AND_ASSIGN(IndexBuffer);

    unsigned int mSerial;
    static unsigned int mNextSerial;
};

class IndexBufferInterface
{
  public:
    IndexBufferInterface(RendererD3D *renderer, bool dynamic);
    virtual ~IndexBufferInterface();

    virtual gl::Error reserveBufferSpace(unsigned int size, GLenum indexType) = 0;

    GLenum getIndexType() const;
    unsigned int getBufferSize() const;

    unsigned int getSerial() const;

    gl::Error mapBuffer(unsigned int size, void** outMappedMemory, unsigned int *streamOffset);
    gl::Error unmapBuffer();

    IndexBuffer *getIndexBuffer() const;

  protected:
    unsigned int getWritePosition() const;
    void setWritePosition(unsigned int writePosition);

    gl::Error discard();

    gl::Error setBufferSize(unsigned int bufferSize, GLenum indexType);

  private:
    DISALLOW_COPY_AND_ASSIGN(IndexBufferInterface);

    RendererD3D *const mRenderer;

    IndexBuffer* mIndexBuffer;

    unsigned int mWritePosition;
    bool mDynamic;
};

class StreamingIndexBufferInterface : public IndexBufferInterface
{
  public:
    StreamingIndexBufferInterface(RendererD3D *renderer);
    ~StreamingIndexBufferInterface();

    virtual gl::Error reserveBufferSpace(unsigned int size, GLenum indexType);
};

class StaticIndexBufferInterface : public IndexBufferInterface
{
  public:
    explicit StaticIndexBufferInterface(RendererD3D *renderer);
    ~StaticIndexBufferInterface();

    virtual gl::Error reserveBufferSpace(unsigned int size, GLenum indexType);

    IndexRangeCache *getIndexRangeCache();

  private:
    IndexRangeCache mIndexRangeCache;
};

}

#endif // LIBGLESV2_RENDERER_INDEXBUFFER_H_
