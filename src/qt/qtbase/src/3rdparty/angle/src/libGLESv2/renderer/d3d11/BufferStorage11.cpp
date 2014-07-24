#include "precompiled.h"
//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// BufferStorage11.cpp Defines the BufferStorage11 class.

#include "libGLESv2/renderer/d3d11/BufferStorage11.h"
#include "libGLESv2/main.h"
#include "libGLESv2/renderer/d3d11/Renderer11.h"

namespace rx
{

BufferStorage11::BufferStorage11(Renderer11 *renderer)
{
    mRenderer = renderer;

    mStagingBuffer = NULL;
    mStagingBufferSize = 0;

    mSize = 0;

    mResolvedData = NULL;
    mResolvedDataSize = 0;
    mResolvedDataValid = false;

    mReadUsageCount = 0;
    mWriteUsageCount = 0;
}

BufferStorage11::~BufferStorage11()
{
    SafeRelease(mStagingBuffer);

    if (mResolvedData)
    {
        free(mResolvedData);
        mResolvedData = NULL;
    }

    for (auto it = mDirectBuffers.begin(); it != mDirectBuffers.end(); it++)
    {
        SafeDelete(it->second);
    }
}

BufferStorage11 *BufferStorage11::makeBufferStorage11(BufferStorage *bufferStorage)
{
    ASSERT(HAS_DYNAMIC_TYPE(BufferStorage11*, bufferStorage));
    return static_cast<BufferStorage11*>(bufferStorage);
}

void *BufferStorage11::getData()
{
    ASSERT(mStagingBuffer);

    if (!mResolvedDataValid)
    {
        ID3D11Device *device = mRenderer->getDevice();
        ID3D11DeviceContext *context = mRenderer->getDeviceContext();
        HRESULT result;

        if (!mResolvedData || mResolvedDataSize < mStagingBufferSize)
        {
            free(mResolvedData);
            mResolvedData = malloc(mSize);
            mResolvedDataSize = mSize;
        }

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        result = context->Map(mStagingBuffer, 0, D3D11_MAP_READ, 0, &mappedResource);
        if (FAILED(result))
        {
            return gl::error(GL_OUT_OF_MEMORY, (void*)NULL);
        }

        memcpy(mResolvedData, mappedResource.pData, mSize);

        context->Unmap(mStagingBuffer, 0);

        mResolvedDataValid = true;
    }

    mReadUsageCount = 0;

    return mResolvedData;
}

void BufferStorage11::setData(const void* data, unsigned int size, unsigned int offset)
{
    ID3D11Device *device = mRenderer->getDevice();
    ID3D11DeviceContext *context = mRenderer->getDeviceContext();
    HRESULT result;

    const unsigned int requiredStagingBufferSize = size + offset;
    const bool createStagingBuffer = !mStagingBuffer || mStagingBufferSize < requiredStagingBufferSize;

    if (createStagingBuffer)
    {
        D3D11_BUFFER_DESC bufferDesc;
        bufferDesc.ByteWidth = requiredStagingBufferSize;
        bufferDesc.Usage = D3D11_USAGE_STAGING;
        bufferDesc.BindFlags = 0;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
        bufferDesc.MiscFlags = 0;
        bufferDesc.StructureByteStride = 0;

        HRESULT result;
        ID3D11Device *device = mRenderer->getDevice();
        ID3D11DeviceContext *context = mRenderer->getDeviceContext();
        ID3D11Buffer *newStagingBuffer;

        if (data && offset == 0)
        {
            D3D11_SUBRESOURCE_DATA initialData;
            initialData.pSysMem = data;
            initialData.SysMemPitch = requiredStagingBufferSize;
            initialData.SysMemSlicePitch = 0;

            result = device->CreateBuffer(&bufferDesc, &initialData, &newStagingBuffer);
        }
        else
        {
            result = device->CreateBuffer(&bufferDesc, NULL, &newStagingBuffer);
        }

        if (FAILED(result))
        {
            mStagingBufferSize = 0;
            return gl::error(GL_OUT_OF_MEMORY);
        }

        mStagingBufferSize = requiredStagingBufferSize;

        if (mStagingBuffer && offset > 0)
        {
            // If offset is greater than zero and the buffer is non-null, need to preserve the data from
            // the old buffer up to offset
            D3D11_BOX srcBox;
            srcBox.left = 0;
            srcBox.right = std::min(offset, requiredStagingBufferSize);
            srcBox.top = 0;
            srcBox.bottom = 1;
            srcBox.front = 0;
            srcBox.back = 1;

            context->CopySubresourceRegion(newStagingBuffer, 0, 0, 0, 0, mStagingBuffer, 0, &srcBox);
        }

        SafeRelease(mStagingBuffer);
        mStagingBuffer = newStagingBuffer;
    }

    if (data && (offset != 0 || !createStagingBuffer))
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        result = context->Map(mStagingBuffer, 0, D3D11_MAP_WRITE, 0, &mappedResource);
        if (FAILED(result))
        {
            return gl::error(GL_OUT_OF_MEMORY);
        }

        unsigned char *offsetBufferPointer = reinterpret_cast<unsigned char *>(mappedResource.pData) + offset;
        memcpy(offsetBufferPointer, data, size);

        context->Unmap(mStagingBuffer, 0);
    }

    for (auto it = mDirectBuffers.begin(); it != mDirectBuffers.end(); it++)
    {
        it->second->markDirty();
    }

    mSize = std::max(mSize, requiredStagingBufferSize);
    mWriteUsageCount = 0;

    mResolvedDataValid = false;
}

void BufferStorage11::copyData(BufferStorage* sourceStorage, unsigned int size,
                               unsigned int sourceOffset, unsigned int destOffset)
{
    BufferStorage11* source = makeBufferStorage11(sourceStorage);
    if (source)
    {
        ID3D11DeviceContext *context = mRenderer->getDeviceContext();

        D3D11_BOX srcBox;
        srcBox.left = sourceOffset;
        srcBox.right = sourceOffset + size;
        srcBox.top = 0;
        srcBox.bottom = 1;
        srcBox.front = 0;
        srcBox.back = 1;

        ASSERT(mStagingBuffer && source->mStagingBuffer);
        context->CopySubresourceRegion(mStagingBuffer, 0, destOffset, 0, 0, source->mStagingBuffer, 0, &srcBox);
    }
}

void BufferStorage11::clear()
{
    mResolvedDataValid = false;
    mSize = 0;
}

unsigned int BufferStorage11::getSize() const
{
    return mSize;
}

bool BufferStorage11::supportsDirectBinding() const
{
    return true;
}

void BufferStorage11::markBufferUsage()
{
    mReadUsageCount++;
    mWriteUsageCount++;

    const unsigned int usageLimit = 5;

    if (mReadUsageCount > usageLimit && mResolvedData)
    {
        free(mResolvedData);
        mResolvedData = NULL;
        mResolvedDataSize = 0;
        mResolvedDataValid = false;
    }
}

ID3D11Buffer *BufferStorage11::getBuffer(BufferUsage usage)
{
    markBufferUsage();

    DirectBufferStorage11 *directBuffer = NULL;

    auto directBufferIt = mDirectBuffers.find(usage);
    if (directBufferIt != mDirectBuffers.end())
    {
        directBuffer = directBufferIt->second;
    }

    if (directBuffer)
    {
        if (directBuffer->isDirty())
        {
            // if updateFromStagingBuffer returns true, the D3D buffer has been recreated
            // and we should update our serial
            if (directBuffer->updateFromStagingBuffer(mStagingBuffer, mSize, 0))
            {
                updateSerial();
            }
        }
    }
    else
    {
        // buffer is not allocated, create it
        directBuffer = new DirectBufferStorage11(mRenderer, usage);
        directBuffer->updateFromStagingBuffer(mStagingBuffer, mSize, 0);

        mDirectBuffers.insert(std::make_pair(usage, directBuffer));
        updateSerial();
    }

    return directBuffer->getD3DBuffer();
}

DirectBufferStorage11::DirectBufferStorage11(Renderer11 *renderer, BufferUsage usage)
    : mRenderer(renderer),
      mUsage(usage),
      mDirectBuffer(NULL),
      mBufferSize(0),
      mDirty(false)
{
}

DirectBufferStorage11::~DirectBufferStorage11()
{
    SafeRelease(mDirectBuffer);
}

BufferUsage DirectBufferStorage11::getUsage() const
{
    return mUsage;
}

// Returns true if it recreates the direct buffer
bool DirectBufferStorage11::updateFromStagingBuffer(ID3D11Buffer *stagingBuffer, size_t size, size_t offset)
{
    ID3D11Device *device = mRenderer->getDevice();
    ID3D11DeviceContext *context = mRenderer->getDeviceContext();

    // unused for now
    ASSERT(offset == 0);

    unsigned int requiredBufferSize = size + offset;
    bool createBuffer = !mDirectBuffer || mBufferSize < requiredBufferSize;

    // (Re)initialize D3D buffer if needed
    if (createBuffer)
    {
        D3D11_BUFFER_DESC bufferDesc;
        fillBufferDesc(&bufferDesc, mRenderer, mUsage, requiredBufferSize);

        ID3D11Buffer *newBuffer;
        HRESULT result = device->CreateBuffer(&bufferDesc, NULL, &newBuffer);

        if (FAILED(result))
        {
            return gl::error(GL_OUT_OF_MEMORY, false);
        }

        // No longer need the old buffer
        SafeRelease(mDirectBuffer);
        mDirectBuffer = newBuffer;

        mBufferSize = bufferDesc.ByteWidth;
    }

    // Copy data via staging buffer
    D3D11_BOX srcBox;
    srcBox.left = 0;
    srcBox.right = size;
    srcBox.top = 0;
    srcBox.bottom = 1;
    srcBox.front = 0;
    srcBox.back = 1;

    context->CopySubresourceRegion(mDirectBuffer, 0, offset, 0, 0, stagingBuffer, 0, &srcBox);

    mDirty = false;

    return createBuffer;
}

void DirectBufferStorage11::fillBufferDesc(D3D11_BUFFER_DESC* bufferDesc, Renderer *renderer, BufferUsage usage, unsigned int bufferSize)
{
    bufferDesc->ByteWidth = bufferSize;
    bufferDesc->MiscFlags = 0;
    bufferDesc->StructureByteStride = 0;

    switch (usage)
    {
      case BUFFER_USAGE_VERTEX:
        bufferDesc->Usage = D3D11_USAGE_DEFAULT;
        bufferDesc->BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc->CPUAccessFlags = 0;
        break;

      case BUFFER_USAGE_INDEX:
        bufferDesc->Usage = D3D11_USAGE_DEFAULT;
        bufferDesc->BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferDesc->CPUAccessFlags = 0;
        break;

    default:
        UNREACHABLE();
    }
}

}
