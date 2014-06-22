#include "precompiled.h"
//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// BufferStorage11.cpp Defines the BufferStorage11 class.

#include "libGLESv2/renderer/BufferStorage11.h"
#include "libGLESv2/main.h"
#include "libGLESv2/renderer/Renderer11.h"

namespace rx
{

BufferStorage11::BufferStorage11(Renderer11 *renderer)
{
    mRenderer = renderer;

    mStagingBuffer = NULL;
    mStagingBufferSize = 0;

    mBuffer = NULL;
    mBufferSize = 0;

    mSize = 0;

    mResolvedData = NULL;
    mResolvedDataSize = 0;
    mResolvedDataValid = false;

    mReadUsageCount = 0;
    mWriteUsageCount = 0;
}

BufferStorage11::~BufferStorage11()
{
    if (mStagingBuffer)
    {
        mStagingBuffer->Release();
        mStagingBuffer = NULL;
    }

    if (mBuffer)
    {
        mBuffer->Release();
        mBuffer = NULL;
    }

    if (mResolvedData)
    {
        free(mResolvedData);
        mResolvedData = NULL;
    }
}

BufferStorage11 *BufferStorage11::makeBufferStorage11(BufferStorage *bufferStorage)
{
    ASSERT(HAS_DYNAMIC_TYPE(BufferStorage11*, bufferStorage));
    return static_cast<BufferStorage11*>(bufferStorage);
}

void *BufferStorage11::getData()
{
    if (!mResolvedDataValid)
    {
        ID3D11Device *device = mRenderer->getDevice();
        ID3D11DeviceContext *context = mRenderer->getDeviceContext();
        HRESULT result;

        if (!mStagingBuffer || mStagingBufferSize < mBufferSize)
        {
            if (mStagingBuffer)
            {
                mStagingBuffer->Release();
                mStagingBuffer = NULL;
                mStagingBufferSize = 0;
            }

            D3D11_BUFFER_DESC bufferDesc;
            bufferDesc.ByteWidth = mSize;
            bufferDesc.Usage = D3D11_USAGE_STAGING;
            bufferDesc.BindFlags = 0;
            bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
            bufferDesc.MiscFlags = 0;
            bufferDesc.StructureByteStride = 0;

            result = device->CreateBuffer(&bufferDesc, NULL, &mStagingBuffer);
            if (FAILED(result))
            {
                return gl::error(GL_OUT_OF_MEMORY, (void*)NULL);
            }

            mStagingBufferSize = bufferDesc.ByteWidth;
        }

        if (!mResolvedData || mResolvedDataSize < mBufferSize)
        {
            free(mResolvedData);
            mResolvedData = malloc(mSize);
            mResolvedDataSize = mSize;
        }

        D3D11_BOX srcBox;
        srcBox.left = 0;
        srcBox.right = mSize;
        srcBox.top = 0;
        srcBox.bottom = 1;
        srcBox.front = 0;
        srcBox.back = 1;

        context->CopySubresourceRegion(mStagingBuffer, 0, 0, 0, 0, mBuffer, 0, &srcBox);

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

    unsigned int requiredBufferSize = size + offset;
    unsigned int requiredStagingSize = size;
    bool directInitialization = offset == 0 && (!mBuffer || mBufferSize < size + offset);

    if (!directInitialization)
    {
        if (!mStagingBuffer || mStagingBufferSize < requiredStagingSize)
        {
            if (mStagingBuffer)
            {
                mStagingBuffer->Release();
                mStagingBuffer = NULL;
                mStagingBufferSize = 0;
            }

            D3D11_BUFFER_DESC bufferDesc;
            bufferDesc.ByteWidth = size;
            bufferDesc.Usage = D3D11_USAGE_STAGING;
            bufferDesc.BindFlags = 0;
            bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
            bufferDesc.MiscFlags = 0;
            bufferDesc.StructureByteStride = 0;

            if (data)
            {
                D3D11_SUBRESOURCE_DATA initialData;
                initialData.pSysMem = data;
                initialData.SysMemPitch = size;
                initialData.SysMemSlicePitch = 0;

                result = device->CreateBuffer(&bufferDesc, &initialData, &mStagingBuffer);
            }
            else
            {
                result = device->CreateBuffer(&bufferDesc, NULL, &mStagingBuffer);
            }

            if (FAILED(result))
            {
                return gl::error(GL_OUT_OF_MEMORY);
            }

            mStagingBufferSize = size;
        }
        else if (data)
        {
            D3D11_MAPPED_SUBRESOURCE mappedResource;
            result = context->Map(mStagingBuffer, 0, D3D11_MAP_WRITE, 0, &mappedResource);
            if (FAILED(result))
            {
                return gl::error(GL_OUT_OF_MEMORY);
            }

            memcpy(mappedResource.pData, data, size);

            context->Unmap(mStagingBuffer, 0);
        }
    }

    if (!mBuffer || mBufferSize < size + offset)
    {
        D3D11_BUFFER_DESC bufferDesc;
        bufferDesc.ByteWidth = requiredBufferSize;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        bufferDesc.MiscFlags = 0;
        bufferDesc.StructureByteStride = 0;

        if (directInitialization)
        {
            // Since the data will fill the entire buffer (being larger than the initial size and having
            // no offset), the buffer can be initialized with the data so no staging buffer is required

            // No longer need the old buffer
            if (mBuffer)
            {
                mBuffer->Release();
                mBuffer = NULL;
                mBufferSize = 0;
            }

            if (data)
            {
                D3D11_SUBRESOURCE_DATA initialData;
                initialData.pSysMem = data;
                initialData.SysMemPitch = size;
                initialData.SysMemSlicePitch = 0;

                result = device->CreateBuffer(&bufferDesc, &initialData, &mBuffer);
            }
            else
            {
                result = device->CreateBuffer(&bufferDesc, NULL, &mBuffer);
            }

            if (FAILED(result))
            {
                return gl::error(GL_OUT_OF_MEMORY);
            }
        }
        else if (mBuffer && offset > 0)
        {
            // If offset is greater than zero and the buffer is non-null, need to preserve the data from
            // the old buffer up to offset
            ID3D11Buffer *newBuffer = NULL;

            result = device->CreateBuffer(&bufferDesc, NULL, &newBuffer);
            if (FAILED(result))
            {
                return gl::error(GL_OUT_OF_MEMORY);
            }

            D3D11_BOX srcBox;
            srcBox.left = 0;
            srcBox.right = std::min(offset, mBufferSize);
            srcBox.top = 0;
            srcBox.bottom = 1;
            srcBox.front = 0;
            srcBox.back = 1;

            context->CopySubresourceRegion(newBuffer, 0, 0, 0, 0, mBuffer, 0, &srcBox);

            mBuffer->Release();
            mBuffer = newBuffer;
        }
        else
        {
            // Simple case, nothing needs to be copied from the old buffer to the new one, just create
            // a new buffer

            // No longer need the old buffer
            if (mBuffer)
            {
                mBuffer->Release();
                mBuffer = NULL;
                mBufferSize = 0;
            }

            // Create a new buffer for data storage
            result = device->CreateBuffer(&bufferDesc, NULL, &mBuffer);
            if (FAILED(result))
            {
                return gl::error(GL_OUT_OF_MEMORY);
            }
        }

        updateSerial();
        mBufferSize = bufferDesc.ByteWidth;
    }

    if (!directInitialization)
    {
        ASSERT(mStagingBuffer && mStagingBufferSize >= requiredStagingSize);

        // Data is already put into the staging buffer, copy it over to the data buffer
        D3D11_BOX srcBox;
        srcBox.left = 0;
        srcBox.right = size;
        srcBox.top = 0;
        srcBox.bottom = 1;
        srcBox.front = 0;
        srcBox.back = 1;

        context->CopySubresourceRegion(mBuffer, 0, offset, 0, 0, mStagingBuffer, 0, &srcBox);
    }

    mSize = std::max(mSize, offset + size);

    mWriteUsageCount = 0;

    mResolvedDataValid = false;
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

    static const unsigned int usageLimit = 5;

    if (mReadUsageCount > usageLimit && mResolvedData)
    {
        free(mResolvedData);
        mResolvedData = NULL;
        mResolvedDataSize = 0;
        mResolvedDataValid = false;
    }

    if (mReadUsageCount > usageLimit && mWriteUsageCount > usageLimit && mStagingBuffer)
    {
        mStagingBuffer->Release();
        mStagingBuffer = NULL;
        mStagingBufferSize = 0;
    }
}

ID3D11Buffer *BufferStorage11::getBuffer() const
{
    return mBuffer;
}

}
