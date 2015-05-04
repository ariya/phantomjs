//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// IndexBuffer.cpp: Defines the abstract IndexBuffer class and IndexBufferInterface
// class with derivations, classes that perform graphics API agnostic index buffer operations.

#include "libGLESv2/renderer/d3d/IndexBuffer.h"
#include "libGLESv2/renderer/d3d/RendererD3D.h"

namespace rx
{

unsigned int IndexBuffer::mNextSerial = 1;

IndexBuffer::IndexBuffer()
{
    updateSerial();
}

IndexBuffer::~IndexBuffer()
{
}

unsigned int IndexBuffer::getSerial() const
{
    return mSerial;
}

void IndexBuffer::updateSerial()
{
    mSerial = mNextSerial++;
}


IndexBufferInterface::IndexBufferInterface(RendererD3D *renderer, bool dynamic) : mRenderer(renderer)
{
    mIndexBuffer = renderer->createIndexBuffer();

    mDynamic = dynamic;
    mWritePosition = 0;
}

IndexBufferInterface::~IndexBufferInterface()
{
    if (mIndexBuffer)
    {
        delete mIndexBuffer;
    }
}

GLenum IndexBufferInterface::getIndexType() const
{
    return mIndexBuffer->getIndexType();
}

unsigned int IndexBufferInterface::getBufferSize() const
{
    return mIndexBuffer->getBufferSize();
}

unsigned int IndexBufferInterface::getSerial() const
{
    return mIndexBuffer->getSerial();
}

gl::Error IndexBufferInterface::mapBuffer(unsigned int size, void** outMappedMemory, unsigned int *streamOffset)
{
    // Protect against integer overflow
    if (mWritePosition + size < mWritePosition)
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Mapping of internal index buffer would cause an integer overflow.");
    }

    gl::Error error = mIndexBuffer->mapBuffer(mWritePosition, size, outMappedMemory);
    if (error.isError())
    {
        if (outMappedMemory)
        {
            *outMappedMemory = NULL;
        }
        return error;
    }

    if (streamOffset)
    {
        *streamOffset = mWritePosition;
    }

    mWritePosition += size;
    return gl::Error(GL_NO_ERROR);
}

gl::Error IndexBufferInterface::unmapBuffer()
{
    return mIndexBuffer->unmapBuffer();
}

IndexBuffer * IndexBufferInterface::getIndexBuffer() const
{
    return mIndexBuffer;
}

unsigned int IndexBufferInterface::getWritePosition() const
{
    return mWritePosition;
}

void IndexBufferInterface::setWritePosition(unsigned int writePosition)
{
    mWritePosition = writePosition;
}

gl::Error IndexBufferInterface::discard()
{
    return mIndexBuffer->discard();
}

gl::Error IndexBufferInterface::setBufferSize(unsigned int bufferSize, GLenum indexType)
{
    if (mIndexBuffer->getBufferSize() == 0)
    {
        return mIndexBuffer->initialize(bufferSize, indexType, mDynamic);
    }
    else
    {
        return mIndexBuffer->setSize(bufferSize, indexType);
    }
}

StreamingIndexBufferInterface::StreamingIndexBufferInterface(RendererD3D *renderer) : IndexBufferInterface(renderer, true)
{
}

StreamingIndexBufferInterface::~StreamingIndexBufferInterface()
{
}

gl::Error StreamingIndexBufferInterface::reserveBufferSpace(unsigned int size, GLenum indexType)
{
    unsigned int curBufferSize = getBufferSize();
    unsigned int writePos = getWritePosition();
    if (size > curBufferSize)
    {
        gl::Error error = setBufferSize(std::max(size, 2 * curBufferSize), indexType);
        if (error.isError())
        {
            return error;
        }
        setWritePosition(0);
    }
    else if (writePos + size > curBufferSize || writePos + size < writePos)
    {
        gl::Error error = discard();
        if (error.isError())
        {
            return error;
        }
        setWritePosition(0);
    }

    return gl::Error(GL_NO_ERROR);
}


StaticIndexBufferInterface::StaticIndexBufferInterface(RendererD3D *renderer) : IndexBufferInterface(renderer, false)
{
}

StaticIndexBufferInterface::~StaticIndexBufferInterface()
{
}

gl::Error StaticIndexBufferInterface::reserveBufferSpace(unsigned int size, GLenum indexType)
{
    unsigned int curSize = getBufferSize();
    if (curSize == 0)
    {
        return setBufferSize(size, indexType);
    }
    else if (curSize >= size && indexType == getIndexType())
    {
        return gl::Error(GL_NO_ERROR);
    }
    else
    {
        UNREACHABLE();
        return gl::Error(GL_INVALID_OPERATION, "Internal static index buffers can't be resized");
    }
}

IndexRangeCache *StaticIndexBufferInterface::getIndexRangeCache()
{
    return &mIndexRangeCache;
}

}
