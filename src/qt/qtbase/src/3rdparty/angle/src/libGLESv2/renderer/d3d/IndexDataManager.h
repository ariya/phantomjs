//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// IndexDataManager.h: Defines the IndexDataManager, a class that
// runs the Buffer translation process for index buffers.

#ifndef LIBGLESV2_INDEXDATAMANAGER_H_
#define LIBGLESV2_INDEXDATAMANAGER_H_

#include "common/angleutils.h"
#include "common/mathutil.h"
#include "libGLESv2/Error.h"

#include <GLES2/gl2.h>

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
class IndexBufferInterface;
class StaticIndexBufferInterface;
class StreamingIndexBufferInterface;
class IndexBuffer;
class BufferD3D;
class RendererD3D;

struct TranslatedIndexData
{
    RangeUI indexRange;
    unsigned int startIndex;
    unsigned int startOffset;   // In bytes

    IndexBuffer *indexBuffer;
    BufferD3D *storage;
    GLenum indexType;
    unsigned int serial;
};

class IndexDataManager
{
  public:
    explicit IndexDataManager(RendererD3D *renderer);
    virtual ~IndexDataManager();

    gl::Error prepareIndexData(GLenum type, GLsizei count, gl::Buffer *arrayElementBuffer, const GLvoid *indices, TranslatedIndexData *translated);

  private:
     gl::Error getStreamingIndexBuffer(GLenum destinationIndexType, IndexBufferInterface **outBuffer);

    DISALLOW_COPY_AND_ASSIGN(IndexDataManager);

    RendererD3D *const mRenderer;

    StreamingIndexBufferInterface *mStreamingBufferShort;
    StreamingIndexBufferInterface *mStreamingBufferInt;
};

}

#endif   // LIBGLESV2_INDEXDATAMANAGER_H_
