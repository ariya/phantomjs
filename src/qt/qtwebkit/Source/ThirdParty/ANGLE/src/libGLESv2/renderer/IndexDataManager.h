//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// IndexDataManager.h: Defines the IndexDataManager, a class that
// runs the Buffer translation process for index buffers.

#ifndef LIBGLESV2_INDEXDATAMANAGER_H_
#define LIBGLESV2_INDEXDATAMANAGER_H_

#include "common/angleutils.h"

namespace
{
    enum { INITIAL_INDEX_BUFFER_SIZE = 4096 * sizeof(GLuint) };
}

namespace gl
{
class Buffer;
}

namespace rx
{
class StaticIndexBufferInterface;
class StreamingIndexBufferInterface;
class IndexBuffer;
class BufferStorage;
class Renderer;

struct TranslatedIndexData
{
    unsigned int minIndex;
    unsigned int maxIndex;
    unsigned int startIndex;
    unsigned int startOffset;   // In bytes

    IndexBuffer *indexBuffer;
    BufferStorage *storage;
    unsigned int serial;
};

class IndexDataManager
{
  public:
    explicit IndexDataManager(Renderer *renderer);
    virtual ~IndexDataManager();

    GLenum prepareIndexData(GLenum type, GLsizei count, gl::Buffer *arrayElementBuffer, const GLvoid *indices, TranslatedIndexData *translated);
    StaticIndexBufferInterface *getCountingIndices(GLsizei count);

  private:
    DISALLOW_COPY_AND_ASSIGN(IndexDataManager);

    Renderer *const mRenderer;

    StreamingIndexBufferInterface *mStreamingBufferShort;
    StreamingIndexBufferInterface *mStreamingBufferInt;
    StaticIndexBufferInterface *mCountingBuffer;
};

}

#endif   // LIBGLESV2_INDEXDATAMANAGER_H_
