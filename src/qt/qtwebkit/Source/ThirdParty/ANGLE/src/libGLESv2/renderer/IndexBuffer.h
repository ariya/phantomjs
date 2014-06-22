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

namespace rx
{
class Renderer;

class IndexBuffer
{
  public:
    IndexBuffer();
    virtual ~IndexBuffer();

    virtual bool initialize(unsigned int bufferSize, GLenum indexType, bool dynamic) = 0;

    virtual bool mapBuffer(unsigned int offset, unsigned int size, void** outMappedMemory) = 0;
    virtual bool unmapBuffer() = 0;

    virtual bool discard() = 0;

    virtual GLenum getIndexType() const = 0;
    virtual unsigned int getBufferSize() const = 0;
    virtual bool setSize(unsigned int bufferSize, GLenum indexType) = 0;

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
    IndexBufferInterface(Renderer *renderer, bool dynamic);
    virtual ~IndexBufferInterface();

    virtual bool reserveBufferSpace(unsigned int size, GLenum indexType) = 0;

    GLenum getIndexType() const;
    unsigned int getBufferSize() const;

    unsigned int getSerial() const;

    int mapBuffer(unsigned int size, void** outMappedMemory);
    bool unmapBuffer();

    IndexBuffer *getIndexBuffer() const;

  protected:
    unsigned int getWritePosition() const;
    void setWritePosition(unsigned int writePosition);

    bool discard();

    bool setBufferSize(unsigned int bufferSize, GLenum indexType);

  private:
    DISALLOW_COPY_AND_ASSIGN(IndexBufferInterface);

    rx::Renderer *const mRenderer;

    IndexBuffer* mIndexBuffer;

    unsigned int mWritePosition;
    bool mDynamic;
};

class StreamingIndexBufferInterface : public IndexBufferInterface
{
  public:
    StreamingIndexBufferInterface(Renderer *renderer);
    ~StreamingIndexBufferInterface();

    virtual bool reserveBufferSpace(unsigned int size, GLenum indexType);
};

class StaticIndexBufferInterface : public IndexBufferInterface
{
  public:
    explicit StaticIndexBufferInterface(Renderer *renderer);
    ~StaticIndexBufferInterface();

    virtual bool reserveBufferSpace(unsigned int size, GLenum indexType);

    unsigned int lookupRange(intptr_t offset, GLsizei count, unsigned int *minIndex, unsigned int *maxIndex);   // Returns the offset into the index buffer, or -1 if not found
    void addRange(intptr_t offset, GLsizei count, unsigned int minIndex, unsigned int maxIndex, unsigned int streamOffset);

  private:
    struct IndexRange
    {
        intptr_t offset;
        GLsizei count;

        bool operator<(const IndexRange& rhs) const
        {
            if (offset != rhs.offset)
            {
                return offset < rhs.offset;
            }
            if (count != rhs.count)
            {
                return count < rhs.count;
            }
            return false;
        }
    };

    struct IndexResult
    {
        unsigned int minIndex;
        unsigned int maxIndex;
        unsigned int streamOffset;
    };

    std::map<IndexRange, IndexResult> mCache;
};

}

#endif // LIBGLESV2_RENDERER_INDEXBUFFER_H_