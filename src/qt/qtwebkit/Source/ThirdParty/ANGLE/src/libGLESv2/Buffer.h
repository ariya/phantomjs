//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Buffer.h: Defines the gl::Buffer class, representing storage of vertex and/or
// index data. Implements GL buffer objects and related functionality.
// [OpenGL ES 2.0.24] section 2.9 page 21.

#ifndef LIBGLESV2_BUFFER_H_
#define LIBGLESV2_BUFFER_H_

#include "common/angleutils.h"
#include "common/RefCountObject.h"

namespace rx
{
class Renderer;
class BufferStorage;
class StaticIndexBufferInterface;
class StaticVertexBufferInterface;
};

namespace gl
{

class Buffer : public RefCountObject
{
  public:
    Buffer(rx::Renderer *renderer, GLuint id);

    virtual ~Buffer();

    void bufferData(const void *data, GLsizeiptr size, GLenum usage);
    void bufferSubData(const void *data, GLsizeiptr size, GLintptr offset);

    GLenum usage() const;

    rx::BufferStorage *getStorage() const;
    unsigned int size();

    rx::StaticVertexBufferInterface *getStaticVertexBuffer();
    rx::StaticIndexBufferInterface *getStaticIndexBuffer();
    void invalidateStaticData();
    void promoteStaticUsage(int dataSize);

  private:
    DISALLOW_COPY_AND_ASSIGN(Buffer);

    rx::Renderer *mRenderer;
    GLenum mUsage;

    rx::BufferStorage *mBufferStorage;

    rx::StaticVertexBufferInterface *mStaticVertexBuffer;
    rx::StaticIndexBufferInterface *mStaticIndexBuffer;
    unsigned int mUnmodifiedDataUse;
};

}

#endif   // LIBGLESV2_BUFFER_H_
