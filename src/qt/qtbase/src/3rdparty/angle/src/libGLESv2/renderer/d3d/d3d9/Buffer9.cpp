//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Buffer9.cpp Defines the Buffer9 class.

#include "libGLESv2/renderer/d3d/d3d9/Buffer9.h"
#include "libGLESv2/renderer/d3d/d3d9/Renderer9.h"
#include "libGLESv2/main.h"

namespace rx
{

Buffer9::Buffer9(Renderer9 *renderer)
    : BufferD3D(),
      mRenderer(renderer),
      mSize(0)
{}

Buffer9::~Buffer9()
{
    mSize = 0;
}

Buffer9 *Buffer9::makeBuffer9(BufferImpl *buffer)
{
    ASSERT(HAS_DYNAMIC_TYPE(Buffer9*, buffer));
    return static_cast<Buffer9*>(buffer);
}

gl::Error Buffer9::setData(const void* data, size_t size, GLenum usage)
{
    if (size > mMemory.size())
    {
        if (!mMemory.resize(size))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to resize internal buffer.");
        }
    }

    mSize = size;
    if (data && size > 0)
    {
        memcpy(mMemory.data(), data, size);
    }

    invalidateStaticData();

    if (usage == GL_STATIC_DRAW)
    {
        initializeStaticData();
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error Buffer9::getData(const uint8_t **outData)
{
    *outData = mMemory.data();
    return gl::Error(GL_NO_ERROR);
}

gl::Error Buffer9::setSubData(const void* data, size_t size, size_t offset)
{
    if (offset + size > mMemory.size())
    {
        if (!mMemory.resize(offset + size))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to resize internal buffer.");
        }
    }

    mSize = std::max(mSize, offset + size);
    if (data && size > 0)
    {
        memcpy(mMemory.data() + offset, data, size);
    }

    invalidateStaticData();

    return gl::Error(GL_NO_ERROR);
}

gl::Error Buffer9::copySubData(BufferImpl* source, GLintptr sourceOffset, GLintptr destOffset, GLsizeiptr size)
{
    // Note: this method is currently unreachable
    Buffer9* sourceBuffer = makeBuffer9(source);
    ASSERT(sourceBuffer);

    memcpy(mMemory.data() + destOffset, sourceBuffer->mMemory.data() + sourceOffset, size);

    invalidateStaticData();

    return gl::Error(GL_NO_ERROR);
}

// We do not support buffer mapping in D3D9
gl::Error Buffer9::map(size_t offset, size_t length, GLbitfield access, GLvoid **mapPtr)
{
    UNREACHABLE();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error Buffer9::unmap()
{
    UNREACHABLE();
    return gl::Error(GL_INVALID_OPERATION);
}

void Buffer9::markTransformFeedbackUsage()
{
    UNREACHABLE();
}

RendererD3D *Buffer9::getRenderer()
{
    return mRenderer;
}

}
