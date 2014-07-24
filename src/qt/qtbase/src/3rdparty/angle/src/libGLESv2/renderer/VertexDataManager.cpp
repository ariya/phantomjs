#include "precompiled.h"
//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexDataManager.h: Defines the VertexDataManager, a class that
// runs the Buffer translation process.

#include "libGLESv2/renderer/VertexDataManager.h"
#include "libGLESv2/renderer/BufferStorage.h"

#include "libGLESv2/Buffer.h"
#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/renderer/VertexBuffer.h"

namespace
{
    enum { INITIAL_STREAM_BUFFER_SIZE = 1024*1024 };
    // This has to be at least 4k or else it fails on ATI cards.
    enum { CONSTANT_VERTEX_BUFFER_SIZE = 4096 };
}

namespace rx
{

static int elementsInBuffer(const gl::VertexAttribute &attribute, unsigned int size)
{
    // Size cannot be larger than a GLsizei
    if (size > static_cast<unsigned int>(std::numeric_limits<int>::max()))
    {
        size = static_cast<unsigned int>(std::numeric_limits<int>::max());
    }

    GLsizei stride = attribute.stride();
    return (size - attribute.mOffset % stride + (stride - attribute.typeSize())) / stride;
}

static int StreamingBufferElementCount(const gl::VertexAttribute &attribute, int vertexDrawCount, int instanceDrawCount)
{
    // For instanced rendering, we draw "instanceDrawCount" sets of "vertexDrawCount" vertices.
    //
    // A vertex attribute with a positive divisor loads one instanced vertex for every set of
    // non-instanced vertices, and the instanced vertex index advances once every "mDivisor" instances.
    if (instanceDrawCount > 0 && attribute.mDivisor > 0)
    {
        return instanceDrawCount / attribute.mDivisor;
    }

    return vertexDrawCount;
}

VertexDataManager::VertexDataManager(Renderer *renderer) : mRenderer(renderer)
{
    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        mCurrentValue[i][0] = std::numeric_limits<float>::quiet_NaN();
        mCurrentValue[i][1] = std::numeric_limits<float>::quiet_NaN();
        mCurrentValue[i][2] = std::numeric_limits<float>::quiet_NaN();
        mCurrentValue[i][3] = std::numeric_limits<float>::quiet_NaN();
        mCurrentValueBuffer[i] = NULL;
        mCurrentValueOffsets[i] = 0;
    }

    mStreamingBuffer = new StreamingVertexBufferInterface(renderer, INITIAL_STREAM_BUFFER_SIZE);

    if (!mStreamingBuffer)
    {
        ERR("Failed to allocate the streaming vertex buffer.");
    }
}

VertexDataManager::~VertexDataManager()
{
    delete mStreamingBuffer;

    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        delete mCurrentValueBuffer[i];
    }
}

static bool directStoragePossible(VertexBufferInterface* vb, const gl::VertexAttribute& attrib)
{
    gl::Buffer *buffer = attrib.mBoundBuffer.get();
    BufferStorage *storage = buffer ? buffer->getStorage() : NULL;

    const bool isAligned = (attrib.stride() % 4 == 0) && (attrib.mOffset % 4 == 0);

    return storage && storage->supportsDirectBinding() && !vb->getVertexBuffer()->requiresConversion(attrib) && isAligned;
}

GLenum VertexDataManager::prepareVertexData(const gl::VertexAttribute attribs[], gl::ProgramBinary *programBinary, GLint start, GLsizei count, TranslatedAttribute *translated, GLsizei instances)
{
    if (!mStreamingBuffer)
    {
        return GL_OUT_OF_MEMORY;
    }

    for (int attributeIndex = 0; attributeIndex < gl::MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        translated[attributeIndex].active = (programBinary->getSemanticIndex(attributeIndex) != -1);
    }

    // Invalidate static buffers that don't contain matching attributes
    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        if (translated[i].active && attribs[i].mArrayEnabled)
        {
            gl::Buffer *buffer = attribs[i].mBoundBuffer.get();
            StaticVertexBufferInterface *staticBuffer = buffer ? buffer->getStaticVertexBuffer() : NULL;

            if (staticBuffer && staticBuffer->getBufferSize() > 0 && !staticBuffer->lookupAttribute(attribs[i], NULL) &&
                !directStoragePossible(staticBuffer, attribs[i]))
            {
                buffer->invalidateStaticData();
            }
        }
    }

    // Reserve the required space in the buffers
    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        if (translated[i].active && attribs[i].mArrayEnabled)
        {
            gl::Buffer *buffer = attribs[i].mBoundBuffer.get();
            StaticVertexBufferInterface *staticBuffer = buffer ? buffer->getStaticVertexBuffer() : NULL;
            VertexBufferInterface *vertexBuffer = staticBuffer ? staticBuffer : static_cast<VertexBufferInterface*>(mStreamingBuffer);

            if (!directStoragePossible(vertexBuffer, attribs[i]))
            {
                if (staticBuffer)
                {
                    if (staticBuffer->getBufferSize() == 0)
                    {
                        int totalCount = elementsInBuffer(attribs[i], buffer->size());
                        if (!staticBuffer->reserveVertexSpace(attribs[i], totalCount, 0))
                        {
                            return GL_OUT_OF_MEMORY;
                        }
                    }
                }
                else
                {
                    int totalCount = StreamingBufferElementCount(attribs[i], count, instances);

                    // Undefined behaviour:
                    // We can return INVALID_OPERATION if our vertex attribute does not have enough backing data.
                    if (buffer && elementsInBuffer(attribs[i], buffer->size()) < totalCount)
                    {
                        return GL_INVALID_OPERATION;
                    }

                    if (!mStreamingBuffer->reserveVertexSpace(attribs[i], totalCount, instances))
                    {
                        return GL_OUT_OF_MEMORY;
                    }
                }
            }
        }
    }

    // Perform the vertex data translations
    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        if (translated[i].active)
        {
            if (attribs[i].mArrayEnabled)
            {
                gl::Buffer *buffer = attribs[i].mBoundBuffer.get();

                if (!buffer && attribs[i].mPointer == NULL)
                {
                    // This is an application error that would normally result in a crash, but we catch it and return an error
                    ERR("An enabled vertex array has no buffer and no pointer.");
                    return GL_INVALID_OPERATION;
                }

                StaticVertexBufferInterface *staticBuffer = buffer ? buffer->getStaticVertexBuffer() : NULL;
                VertexBufferInterface *vertexBuffer = staticBuffer ? staticBuffer : static_cast<VertexBufferInterface*>(mStreamingBuffer);

                BufferStorage *storage = buffer ? buffer->getStorage() : NULL;
                bool directStorage = directStoragePossible(vertexBuffer, attribs[i]);

                unsigned int streamOffset = 0;
                unsigned int outputElementSize = 0;

                if (directStorage)
                {
                    outputElementSize = attribs[i].stride();
                    streamOffset = attribs[i].mOffset + outputElementSize * start;
                    storage->markBufferUsage();
                }
                else if (staticBuffer)
                {
                    if (!staticBuffer->getVertexBuffer()->getSpaceRequired(attribs[i], 1, 0, &outputElementSize))
                    {
                        return GL_OUT_OF_MEMORY;
                    }

                    if (!staticBuffer->lookupAttribute(attribs[i], &streamOffset))
                    {
                        // Convert the entire buffer
                        int totalCount = elementsInBuffer(attribs[i], storage->getSize());
                        int startIndex = attribs[i].mOffset / attribs[i].stride();

                        if (!staticBuffer->storeVertexAttributes(attribs[i], -startIndex, totalCount, 0, &streamOffset))
                        {
                            return GL_OUT_OF_MEMORY;
                        }
                    }

                    unsigned int firstElementOffset = (attribs[i].mOffset / attribs[i].stride()) * outputElementSize;
                    unsigned int startOffset = (instances == 0 || attribs[i].mDivisor == 0) ? start * outputElementSize : 0;
                    if (streamOffset + firstElementOffset + startOffset < streamOffset)
                    {
                        return GL_OUT_OF_MEMORY;
                    }

                    streamOffset += firstElementOffset + startOffset;
                }
                else
                {
                    int totalCount = StreamingBufferElementCount(attribs[i], count, instances);
                    if (!mStreamingBuffer->getVertexBuffer()->getSpaceRequired(attribs[i], 1, 0, &outputElementSize) ||
                        !mStreamingBuffer->storeVertexAttributes(attribs[i], start, totalCount, instances, &streamOffset))
                    {
                        return GL_OUT_OF_MEMORY;
                    }
                }

                translated[i].storage = directStorage ? storage : NULL;
                translated[i].vertexBuffer = vertexBuffer->getVertexBuffer();
                translated[i].serial = directStorage ? storage->getSerial() : vertexBuffer->getSerial();
                translated[i].divisor = attribs[i].mDivisor;

                translated[i].attribute = &attribs[i];
                translated[i].stride = outputElementSize;
                translated[i].offset = streamOffset;
            }
            else
            {
                if (!mCurrentValueBuffer[i])
                {
                    mCurrentValueBuffer[i] = new StreamingVertexBufferInterface(mRenderer, CONSTANT_VERTEX_BUFFER_SIZE);
                }

                StreamingVertexBufferInterface *buffer = mCurrentValueBuffer[i];

                if (mCurrentValue[i][0] != attribs[i].mCurrentValue[0] ||
                    mCurrentValue[i][1] != attribs[i].mCurrentValue[1] ||
                    mCurrentValue[i][2] != attribs[i].mCurrentValue[2] ||
                    mCurrentValue[i][3] != attribs[i].mCurrentValue[3])
                {
                    unsigned int requiredSpace = sizeof(float) * 4;
                    if (!buffer->reserveRawDataSpace(requiredSpace))
                    {
                        return GL_OUT_OF_MEMORY;
                    }

                    unsigned int streamOffset;
                    if (!buffer->storeRawData(attribs[i].mCurrentValue, requiredSpace, &streamOffset))
                    {
                        return GL_OUT_OF_MEMORY;
                    }

                    mCurrentValue[i][0] = attribs[i].mCurrentValue[0];
                    mCurrentValue[i][1] = attribs[i].mCurrentValue[1];
                    mCurrentValue[i][2] = attribs[i].mCurrentValue[2];
                    mCurrentValue[i][3] = attribs[i].mCurrentValue[3];
                    mCurrentValueOffsets[i] = streamOffset;
                }

                translated[i].storage = NULL;
                translated[i].vertexBuffer = mCurrentValueBuffer[i]->getVertexBuffer();
                translated[i].serial = mCurrentValueBuffer[i]->getSerial();
                translated[i].divisor = 0;

                translated[i].attribute = &attribs[i];
                translated[i].stride = 0;
                translated[i].offset = mCurrentValueOffsets[i];
            }
        }
    }

    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        if (translated[i].active && attribs[i].mArrayEnabled)
        {
            gl::Buffer *buffer = attribs[i].mBoundBuffer.get();

            if (buffer)
            {
                buffer->promoteStaticUsage(count * attribs[i].typeSize());
            }
        }
    }

    return GL_NO_ERROR;
}

}
