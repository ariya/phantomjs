#include "precompiled.h"
//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// IndexBuffer.cpp: Defines the abstract IndexBuffer class and IndexBufferInterface
// class with derivations, classes that perform graphics API agnostic index buffer operations.

#include "libGLESv2/renderer/IndexBuffer.h"
#include "libGLESv2/renderer/Renderer.h"

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


IndexBufferInterface::IndexBufferInterface(Renderer *renderer, bool dynamic) : mRenderer(renderer)
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

bool IndexBufferInterface::mapBuffer(unsigned int size, void** outMappedMemory, unsigned int *streamOffset)
{
    // Protect against integer overflow
    if (mWritePosition + size < mWritePosition)
    {
        return false;
    }

    if (!mIndexBuffer->mapBuffer(mWritePosition, size, outMappedMemory))
    {
        if (outMappedMemory)
        {
            *outMappedMemory = NULL;
        }
        return false;
    }

    if (streamOffset)
    {
        *streamOffset = mWritePosition;
    }

    mWritePosition += size;
    return true;
}

bool IndexBufferInterface::unmapBuffer()
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

bool IndexBufferInterface::discard()
{
    return mIndexBuffer->discard();
}

bool IndexBufferInterface::setBufferSize(unsigned int bufferSize, GLenum indexType)
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

StreamingIndexBufferInterface::StreamingIndexBufferInterface(Renderer *renderer) : IndexBufferInterface(renderer, true)
{
}

StreamingIndexBufferInterface::~StreamingIndexBufferInterface()
{
}

bool StreamingIndexBufferInterface::reserveBufferSpace(unsigned int size, GLenum indexType)
{
    bool result = true;
    unsigned int curBufferSize = getBufferSize();
    unsigned int writePos = getWritePosition();
    if (size > curBufferSize)
    {
        result = setBufferSize(std::max(size, 2 * curBufferSize), indexType);
        setWritePosition(0);
    }
    else if (writePos + size > curBufferSize || writePos + size < writePos)
    {
        if (!discard())
        {
            return false;
        }
        setWritePosition(0);
    }

    return result;
}


StaticIndexBufferInterface::StaticIndexBufferInterface(Renderer *renderer) : IndexBufferInterface(renderer, false)
{
}

StaticIndexBufferInterface::~StaticIndexBufferInterface()
{
}

bool StaticIndexBufferInterface::reserveBufferSpace(unsigned int size, GLenum indexType)
{
    unsigned int curSize = getBufferSize();
    if (curSize == 0)
    {
        return setBufferSize(size, indexType);
    }
    else if (curSize >= size && indexType == getIndexType())
    {
        return true;
    }
    else
    {
        ERR("Static index buffers can't be resized");
        UNREACHABLE();
        return false;
    }
}

IndexRangeCache *StaticIndexBufferInterface::getIndexRangeCache()
{
    return &mIndexRangeCache;
}

}

