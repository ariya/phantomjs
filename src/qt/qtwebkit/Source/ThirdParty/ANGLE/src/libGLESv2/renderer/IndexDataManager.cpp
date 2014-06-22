#include "precompiled.h"
//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// IndexDataManager.cpp: Defines the IndexDataManager, a class that
// runs the Buffer translation process for index buffers.

#include "libGLESv2/renderer/IndexDataManager.h"
#include "libGLESv2/renderer/BufferStorage.h"

#include "libGLESv2/Buffer.h"
#include "libGLESv2/main.h"
#include "libGLESv2/renderer/IndexBuffer.h"

namespace rx
{

IndexDataManager::IndexDataManager(Renderer *renderer) : mRenderer(renderer)
{
    mStreamingBufferShort = new StreamingIndexBufferInterface(mRenderer);
    if (!mStreamingBufferShort->reserveBufferSpace(INITIAL_INDEX_BUFFER_SIZE, GL_UNSIGNED_SHORT))
    {
        delete mStreamingBufferShort;
        mStreamingBufferShort = NULL;
    }

    mStreamingBufferInt = new StreamingIndexBufferInterface(mRenderer);
    if (!mStreamingBufferInt->reserveBufferSpace(INITIAL_INDEX_BUFFER_SIZE, GL_UNSIGNED_INT))
    {
        delete mStreamingBufferInt;
        mStreamingBufferInt = NULL;
    }

    if (!mStreamingBufferShort)
    {
        // Make sure both buffers are deleted.
        delete mStreamingBufferInt;
        mStreamingBufferInt = NULL;

        ERR("Failed to allocate the streaming index buffer(s).");
    }

    mCountingBuffer = NULL;
}

IndexDataManager::~IndexDataManager()
{
    delete mStreamingBufferShort;
    delete mStreamingBufferInt;
    delete mCountingBuffer;
}

static unsigned int indexTypeSize(GLenum type)
{
    switch (type)
    {
      case GL_UNSIGNED_INT:   return sizeof(GLuint);
      case GL_UNSIGNED_SHORT: return sizeof(GLushort);
      case GL_UNSIGNED_BYTE:  return sizeof(GLubyte);
      default: UNREACHABLE(); return sizeof(GLushort);
    }
}

static void convertIndices(GLenum type, const void *input, GLsizei count, void *output)
{
    if (type == GL_UNSIGNED_BYTE)
    {
        const GLubyte *in = static_cast<const GLubyte*>(input);
        GLushort *out = static_cast<GLushort*>(output);

        for (GLsizei i = 0; i < count; i++)
        {
            out[i] = in[i];
        }
    }
    else if (type == GL_UNSIGNED_INT)
    {
        memcpy(output, input, count * sizeof(GLuint));
    }
    else if (type == GL_UNSIGNED_SHORT)
    {
        memcpy(output, input, count * sizeof(GLushort));
    }
    else UNREACHABLE();
}

template <class IndexType>
static void computeRange(const IndexType *indices, GLsizei count, GLuint *minIndex, GLuint *maxIndex)
{
    *minIndex = indices[0];
    *maxIndex = indices[0];

    for (GLsizei i = 0; i < count; i++)
    {
        if (*minIndex > indices[i]) *minIndex = indices[i];
        if (*maxIndex < indices[i]) *maxIndex = indices[i];
    }
}

static void computeRange(GLenum type, const GLvoid *indices, GLsizei count, GLuint *minIndex, GLuint *maxIndex)
{
    if (type == GL_UNSIGNED_BYTE)
    {
        computeRange(static_cast<const GLubyte*>(indices), count, minIndex, maxIndex);
    }
    else if (type == GL_UNSIGNED_INT)
    {
        computeRange(static_cast<const GLuint*>(indices), count, minIndex, maxIndex);
    }
    else if (type == GL_UNSIGNED_SHORT)
    {
        computeRange(static_cast<const GLushort*>(indices), count, minIndex, maxIndex);
    }
    else UNREACHABLE();
}

GLenum IndexDataManager::prepareIndexData(GLenum type, GLsizei count, gl::Buffer *buffer, const GLvoid *indices, TranslatedIndexData *translated)
{
    if (!mStreamingBufferShort)
    {
        return GL_OUT_OF_MEMORY;
    }

    GLenum destinationIndexType = (type == GL_UNSIGNED_INT) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
    intptr_t offset = reinterpret_cast<intptr_t>(indices);
    bool alignedOffset = false;

    BufferStorage *storage = NULL;

    if (buffer != NULL)
    {
        storage = buffer->getStorage();

        switch (type)
        {
          case GL_UNSIGNED_BYTE:  alignedOffset = (offset % sizeof(GLubyte) == 0);  break;
          case GL_UNSIGNED_SHORT: alignedOffset = (offset % sizeof(GLushort) == 0); break;
          case GL_UNSIGNED_INT:   alignedOffset = (offset % sizeof(GLuint) == 0);   break;
          default: UNREACHABLE(); alignedOffset = false;
        }

        if (indexTypeSize(type) * count + offset > storage->getSize())
        {
            return GL_INVALID_OPERATION;
        }

        indices = static_cast<const GLubyte*>(storage->getData()) + offset;
    }

    StreamingIndexBufferInterface *streamingBuffer = (type == GL_UNSIGNED_INT) ? mStreamingBufferInt : mStreamingBufferShort;

    StaticIndexBufferInterface *staticBuffer = buffer ? buffer->getStaticIndexBuffer() : NULL;
    IndexBufferInterface *indexBuffer = streamingBuffer;
    bool directStorage = alignedOffset && storage && storage->supportsDirectBinding() &&
                         destinationIndexType == type;
    UINT streamOffset = 0;

    if (directStorage)
    {
        indexBuffer = streamingBuffer;
        streamOffset = offset;
        storage->markBufferUsage();
        computeRange(type, indices, count, &translated->minIndex, &translated->maxIndex);
    }
    else if (staticBuffer && staticBuffer->getBufferSize() != 0 && staticBuffer->getIndexType() == type && alignedOffset)
    {
        indexBuffer = staticBuffer;
        streamOffset = staticBuffer->lookupRange(offset, count, &translated->minIndex, &translated->maxIndex);

        if (streamOffset == -1)
        {
            streamOffset = (offset / indexTypeSize(type)) * indexTypeSize(destinationIndexType);
            computeRange(type, indices, count, &translated->minIndex, &translated->maxIndex);
            staticBuffer->addRange(offset, count, translated->minIndex, translated->maxIndex, streamOffset);
        }
    }
    else
    {
        int convertCount = count;

        if (staticBuffer)
        {
            if (staticBuffer->getBufferSize() == 0 && alignedOffset)
            {
                indexBuffer = staticBuffer;
                convertCount = storage->getSize() / indexTypeSize(type);
            }
            else
            {
                buffer->invalidateStaticData();
                staticBuffer = NULL;
            }
        }

        if (!indexBuffer)
        {
            ERR("No valid index buffer.");
            return GL_INVALID_OPERATION;
        }

        unsigned int bufferSizeRequired = convertCount * indexTypeSize(destinationIndexType);
        indexBuffer->reserveBufferSpace(bufferSizeRequired, type);

        void* output = NULL;
        streamOffset = indexBuffer->mapBuffer(bufferSizeRequired, &output);
        if (streamOffset == -1 || output == NULL)
        {
            ERR("Failed to map index buffer.");
            return GL_OUT_OF_MEMORY;
        }

        convertIndices(type, staticBuffer ? storage->getData() : indices, convertCount, output);

        if (!indexBuffer->unmapBuffer())
        {
            ERR("Failed to unmap index buffer.");
            return GL_OUT_OF_MEMORY;
        }

        computeRange(type, indices, count, &translated->minIndex, &translated->maxIndex);

        if (staticBuffer)
        {
            streamOffset = (offset / indexTypeSize(type)) * indexTypeSize(destinationIndexType);
            staticBuffer->addRange(offset, count, translated->minIndex, translated->maxIndex, streamOffset);
        }
    }

    translated->storage = directStorage ? storage : NULL;
    translated->indexBuffer = indexBuffer->getIndexBuffer();
    translated->serial = directStorage ? storage->getSerial() : indexBuffer->getSerial();
    translated->startIndex = streamOffset / indexTypeSize(destinationIndexType);
    translated->startOffset = streamOffset;

    if (buffer)
    {
        buffer->promoteStaticUsage(count * indexTypeSize(type));
    }

    return GL_NO_ERROR;
}

StaticIndexBufferInterface *IndexDataManager::getCountingIndices(GLsizei count)
{
    if (count <= 65536)   // 16-bit indices
    {
        const unsigned int spaceNeeded = count * sizeof(unsigned short);

        if (!mCountingBuffer || mCountingBuffer->getBufferSize() < spaceNeeded)
        {
            delete mCountingBuffer;
            mCountingBuffer = new StaticIndexBufferInterface(mRenderer);
            mCountingBuffer->reserveBufferSpace(spaceNeeded, GL_UNSIGNED_SHORT);

            void* mappedMemory = NULL;
            if (mCountingBuffer->mapBuffer(spaceNeeded, &mappedMemory) == -1 || mappedMemory == NULL)
            {
                ERR("Failed to map counting buffer.");
                return NULL;
            }

            unsigned short *data = reinterpret_cast<unsigned short*>(mappedMemory);
            for(int i = 0; i < count; i++)
            {
                data[i] = i;
            }

            if (!mCountingBuffer->unmapBuffer())
            {
                ERR("Failed to unmap counting buffer.");
                return NULL;
            }
        }
    }
    else if (mStreamingBufferInt)   // 32-bit indices supported
    {
        const unsigned int spaceNeeded = count * sizeof(unsigned int);

        if (!mCountingBuffer || mCountingBuffer->getBufferSize() < spaceNeeded)
        {
            delete mCountingBuffer;
            mCountingBuffer = new StaticIndexBufferInterface(mRenderer);
            mCountingBuffer->reserveBufferSpace(spaceNeeded, GL_UNSIGNED_INT);

            void* mappedMemory = NULL;
            if (mCountingBuffer->mapBuffer(spaceNeeded, &mappedMemory) == -1 || mappedMemory == NULL)
            {
                ERR("Failed to map counting buffer.");
                return NULL;
            }

            unsigned int *data = reinterpret_cast<unsigned int*>(mappedMemory);
            for(int i = 0; i < count; i++)
            {
                data[i] = i;
            }

            if (!mCountingBuffer->unmapBuffer())
            {
                ERR("Failed to unmap counting buffer.");
                return NULL;
            }
        }
    }
    else
    {
        return NULL;
    }

    return mCountingBuffer;
}

}
