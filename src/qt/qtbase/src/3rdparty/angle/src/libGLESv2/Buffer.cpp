#include "precompiled.h"
//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Buffer.cpp: Implements the gl::Buffer class, representing storage of vertex and/or
// index data. Implements GL buffer objects and related functionality.
// [OpenGL ES 2.0.24] section 2.9 page 21.

#include "libGLESv2/Buffer.h"

#include "libGLESv2/renderer/VertexBuffer.h"
#include "libGLESv2/renderer/IndexBuffer.h"
#include "libGLESv2/renderer/BufferStorage.h"
#include "libGLESv2/renderer/Renderer.h"

namespace gl
{

Buffer::Buffer(rx::Renderer *renderer, GLuint id) : RefCountObject(id)
{
    mRenderer = renderer;
    mUsage = GL_DYNAMIC_DRAW;

    mBufferStorage = renderer->createBufferStorage();
    mStaticVertexBuffer = NULL;
    mStaticIndexBuffer = NULL;
    mUnmodifiedDataUse = 0;
}

Buffer::~Buffer()
{
    delete mBufferStorage;
    delete mStaticVertexBuffer;
    delete mStaticIndexBuffer;
}

void Buffer::bufferData(const void *data, GLsizeiptr size, GLenum usage)
{
    mBufferStorage->clear();
    mIndexRangeCache.clear();
    mBufferStorage->setData(data, size, 0);

    mUsage = usage;

    invalidateStaticData();

    if (usage == GL_STATIC_DRAW)
    {
        mStaticVertexBuffer = new rx::StaticVertexBufferInterface(mRenderer);
        mStaticIndexBuffer = new rx::StaticIndexBufferInterface(mRenderer);
    }
}

void Buffer::bufferSubData(const void *data, GLsizeiptr size, GLintptr offset)
{
    mBufferStorage->setData(data, size, offset);
    mIndexRangeCache.invalidateRange(offset, size);

    if ((mStaticVertexBuffer && mStaticVertexBuffer->getBufferSize() != 0) || (mStaticIndexBuffer && mStaticIndexBuffer->getBufferSize() != 0))
    {
        invalidateStaticData();
    }

    mUnmodifiedDataUse = 0;
}

rx::BufferStorage *Buffer::getStorage() const
{
    return mBufferStorage;
}

unsigned int Buffer::size() const
{
    return mBufferStorage->getSize();
}

GLenum Buffer::usage() const
{
    return mUsage;
}

rx::StaticVertexBufferInterface *Buffer::getStaticVertexBuffer()
{
    return mStaticVertexBuffer;
}

rx::StaticIndexBufferInterface *Buffer::getStaticIndexBuffer()
{
    return mStaticIndexBuffer;
}

void Buffer::invalidateStaticData()
{
    delete mStaticVertexBuffer;
    mStaticVertexBuffer = NULL;

    delete mStaticIndexBuffer;
    mStaticIndexBuffer = NULL;

    mUnmodifiedDataUse = 0;
}

// Creates static buffers if sufficient used data has been left unmodified
void Buffer::promoteStaticUsage(int dataSize)
{
    if (!mStaticVertexBuffer && !mStaticIndexBuffer)
    {
        mUnmodifiedDataUse += dataSize;

        if (mUnmodifiedDataUse > 3 * mBufferStorage->getSize())
        {
            mStaticVertexBuffer = new rx::StaticVertexBufferInterface(mRenderer);
            mStaticIndexBuffer = new rx::StaticIndexBufferInterface(mRenderer);
        }
    }
}

rx::IndexRangeCache *Buffer::getIndexRangeCache()
{
    return &mIndexRangeCache;
}

}
