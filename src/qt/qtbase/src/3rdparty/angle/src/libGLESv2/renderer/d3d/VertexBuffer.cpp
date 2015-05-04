//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexBuffer.cpp: Defines the abstract VertexBuffer class and VertexBufferInterface
// class with derivations, classes that perform graphics API agnostic vertex buffer operations.

#include "libGLESv2/renderer/d3d/VertexBuffer.h"
#include "libGLESv2/renderer/d3d/BufferD3D.h"
#include "libGLESv2/renderer/d3d/RendererD3D.h"
#include "libGLESv2/VertexAttribute.h"

#include "common/mathutil.h"

namespace rx
{

unsigned int VertexBuffer::mNextSerial = 1;

VertexBuffer::VertexBuffer()
{
    updateSerial();
}

VertexBuffer::~VertexBuffer()
{
}

void VertexBuffer::updateSerial()
{
    mSerial = mNextSerial++;
}

unsigned int VertexBuffer::getSerial() const
{
    return mSerial;
}

VertexBufferInterface::VertexBufferInterface(RendererD3D *renderer, bool dynamic) : mRenderer(renderer)
{
    mDynamic = dynamic;
    mWritePosition = 0;
    mReservedSpace = 0;

    mVertexBuffer = renderer->createVertexBuffer();
}

VertexBufferInterface::~VertexBufferInterface()
{
    delete mVertexBuffer;
}

unsigned int VertexBufferInterface::getSerial() const
{
    return mVertexBuffer->getSerial();
}

unsigned int VertexBufferInterface::getBufferSize() const
{
    return mVertexBuffer->getBufferSize();
}

gl::Error VertexBufferInterface::setBufferSize(unsigned int size)
{
    if (mVertexBuffer->getBufferSize() == 0)
    {
        return mVertexBuffer->initialize(size, mDynamic);
    }
    else
    {
        return mVertexBuffer->setBufferSize(size);
    }
}

unsigned int VertexBufferInterface::getWritePosition() const
{
    return mWritePosition;
}

void VertexBufferInterface::setWritePosition(unsigned int writePosition)
{
    mWritePosition = writePosition;
}

gl::Error VertexBufferInterface::discard()
{
    return mVertexBuffer->discard();
}

gl::Error VertexBufferInterface::storeVertexAttributes(const gl::VertexAttribute &attrib, const gl::VertexAttribCurrentValueData &currentValue,
                                                       GLint start, GLsizei count, GLsizei instances, unsigned int *outStreamOffset)
{
    gl::Error error(GL_NO_ERROR);

    unsigned int spaceRequired;
    error = mVertexBuffer->getSpaceRequired(attrib, count, instances, &spaceRequired);
    if (error.isError())
    {
        return error;
    }

    if (mWritePosition + spaceRequired < mWritePosition)
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Internal error, new vertex buffer write position would overflow.");
    }

    error = reserveSpace(mReservedSpace);
    if (error.isError())
    {
        return error;
    }
    mReservedSpace = 0;

    error = mVertexBuffer->storeVertexAttributes(attrib, currentValue, start, count, instances, mWritePosition);
    if (error.isError())
    {
        return error;
    }

    if (outStreamOffset)
    {
        *outStreamOffset = mWritePosition;
    }

    mWritePosition += spaceRequired;

    // Align to 16-byte boundary
    mWritePosition = roundUp(mWritePosition, 16u);

    return gl::Error(GL_NO_ERROR);
}

gl::Error VertexBufferInterface::reserveVertexSpace(const gl::VertexAttribute &attrib, GLsizei count, GLsizei instances)
{
    gl::Error error(GL_NO_ERROR);

    unsigned int requiredSpace;
    error = mVertexBuffer->getSpaceRequired(attrib, count, instances, &requiredSpace);
    if (error.isError())
    {
        return error;
    }

    // Protect against integer overflow
    if (mReservedSpace + requiredSpace < mReservedSpace)
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Unable to reserve %u extra bytes in internal vertex buffer, "
                         "it would result in an overflow.", requiredSpace);
    }

    mReservedSpace += requiredSpace;

    // Align to 16-byte boundary
    mReservedSpace = roundUp(mReservedSpace, 16u);

    return gl::Error(GL_NO_ERROR);
}

VertexBuffer* VertexBufferInterface::getVertexBuffer() const
{
    return mVertexBuffer;
}

bool VertexBufferInterface::directStoragePossible(const gl::VertexAttribute &attrib,
                                                  const gl::VertexAttribCurrentValueData &currentValue) const
{
    gl::Buffer *buffer = attrib.buffer.get();
    BufferD3D *storage = buffer ? BufferD3D::makeBufferD3D(buffer->getImplementation()) : NULL;

    if (!storage || !storage->supportsDirectBinding())
    {
        return false;
    }

    // Alignment restrictions: In D3D, vertex data must be aligned to
    //  the format stride, or to a 4-byte boundary, whichever is smaller.
    //  (Undocumented, and experimentally confirmed)
    size_t alignment = 4;
    bool requiresConversion = false;

    if (attrib.type != GL_FLOAT)
    {
        gl::VertexFormat vertexFormat(attrib, currentValue.Type);

        unsigned int outputElementSize;
        getVertexBuffer()->getSpaceRequired(attrib, 1, 0, &outputElementSize);
        alignment = std::min<size_t>(outputElementSize, 4);

        requiresConversion = (mRenderer->getVertexConversionType(vertexFormat) & VERTEX_CONVERT_CPU) != 0;
    }

    bool isAligned = (static_cast<size_t>(ComputeVertexAttributeStride(attrib)) % alignment == 0) &&
                     (static_cast<size_t>(attrib.offset) % alignment == 0);

    return !requiresConversion && isAligned;
}

StreamingVertexBufferInterface::StreamingVertexBufferInterface(RendererD3D *renderer, std::size_t initialSize) : VertexBufferInterface(renderer, true)
{
    setBufferSize(initialSize);
}

StreamingVertexBufferInterface::~StreamingVertexBufferInterface()
{
}

gl::Error StreamingVertexBufferInterface::reserveSpace(unsigned int size)
{
    unsigned int curBufferSize = getBufferSize();
    if (size > curBufferSize)
    {
        gl::Error error = setBufferSize(std::max(size, 3 * curBufferSize / 2));
        if (error.isError())
        {
            return error;
        }
        setWritePosition(0);
    }
    else if (getWritePosition() + size > curBufferSize)
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

StaticVertexBufferInterface::StaticVertexBufferInterface(RendererD3D *renderer) : VertexBufferInterface(renderer, false)
{
}

StaticVertexBufferInterface::~StaticVertexBufferInterface()
{
}

bool StaticVertexBufferInterface::lookupAttribute(const gl::VertexAttribute &attrib, unsigned int *outStreamOffset)
{
    for (unsigned int element = 0; element < mCache.size(); element++)
    {
        if (mCache[element].type == attrib.type &&
            mCache[element].size == attrib.size &&
            mCache[element].stride == ComputeVertexAttributeStride(attrib) &&
            mCache[element].normalized == attrib.normalized &&
            mCache[element].pureInteger == attrib.pureInteger)
        {
            size_t offset = (static_cast<size_t>(attrib.offset) % ComputeVertexAttributeStride(attrib));
            if (mCache[element].attributeOffset == offset)
            {
                if (outStreamOffset)
                {
                    *outStreamOffset = mCache[element].streamOffset;
                }
                return true;
            }
        }
    }

    return false;
}

gl::Error StaticVertexBufferInterface::reserveSpace(unsigned int size)
{
    unsigned int curSize = getBufferSize();
    if (curSize == 0)
    {
        return setBufferSize(size);
    }
    else if (curSize >= size)
    {
        return gl::Error(GL_NO_ERROR);
    }
    else
    {
        UNREACHABLE();
        return gl::Error(GL_INVALID_OPERATION, "Internal error, Static vertex buffers can't be resized.");
    }
}

gl::Error StaticVertexBufferInterface::storeVertexAttributes(const gl::VertexAttribute &attrib, const gl::VertexAttribCurrentValueData &currentValue,
                                                             GLint start, GLsizei count, GLsizei instances, unsigned int *outStreamOffset)
{
    unsigned int streamOffset;
    gl::Error error = VertexBufferInterface::storeVertexAttributes(attrib, currentValue, start, count, instances, &streamOffset);
    if (error.isError())
    {
        return error;
    }

    size_t attributeOffset = static_cast<size_t>(attrib.offset) % ComputeVertexAttributeStride(attrib);
    VertexElement element = { attrib.type, attrib.size, ComputeVertexAttributeStride(attrib), attrib.normalized, attrib.pureInteger, attributeOffset, streamOffset };
    mCache.push_back(element);

    if (outStreamOffset)
    {
        *outStreamOffset = streamOffset;
    }

    return gl::Error(GL_NO_ERROR);
}

}
