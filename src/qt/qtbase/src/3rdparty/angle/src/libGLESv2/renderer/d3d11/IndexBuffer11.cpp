#include "precompiled.h"
//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// IndexBuffer11.cpp: Defines the D3D11 IndexBuffer implementation.

#include "libGLESv2/renderer/d3d11/IndexBuffer11.h"
#include "libGLESv2/renderer/d3d11/Renderer11.h"

namespace rx
{

IndexBuffer11::IndexBuffer11(Renderer11 *const renderer) : mRenderer(renderer)
{
    mBuffer = NULL;
    mBufferSize = 0;
    mDynamicUsage = false;
}

IndexBuffer11::~IndexBuffer11()
{
    if (mBuffer)
    {
        mBuffer->Release();
        mBuffer = NULL;
    }
}

bool IndexBuffer11::initialize(unsigned int bufferSize, GLenum indexType, bool dynamic)
{
    if (mBuffer)
    {
        mBuffer->Release();
        mBuffer = NULL;
    }

    updateSerial();

    if (bufferSize > 0)
    {
        ID3D11Device* dxDevice = mRenderer->getDevice();

        D3D11_BUFFER_DESC bufferDesc;
        bufferDesc.ByteWidth = bufferSize;
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        bufferDesc.MiscFlags = 0;
        bufferDesc.StructureByteStride = 0;

        HRESULT result = dxDevice->CreateBuffer(&bufferDesc, NULL, &mBuffer);
        if (FAILED(result))
        {
            return false;
        }
    }

    mBufferSize = bufferSize;
    mIndexType = indexType;
    mDynamicUsage = dynamic;

    return true;
}

IndexBuffer11 *IndexBuffer11::makeIndexBuffer11(IndexBuffer *indexBuffer)
{
    ASSERT(HAS_DYNAMIC_TYPE(IndexBuffer11*, indexBuffer));
    return static_cast<IndexBuffer11*>(indexBuffer);
}

bool IndexBuffer11::mapBuffer(unsigned int offset, unsigned int size, void** outMappedMemory)
{
    if (mBuffer)
    {
        // Check for integer overflows and out-out-bounds map requests
        if (offset + size < offset || offset + size > mBufferSize)
        {
            ERR("Index buffer map range is not inside the buffer.");
            return false;
        }

        ID3D11DeviceContext *dxContext = mRenderer->getDeviceContext();

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT result = dxContext->Map(mBuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedResource);
        if (FAILED(result))
        {
            ERR("Index buffer map failed with error 0x%08x", result);
            return false;
        }

        *outMappedMemory = reinterpret_cast<char*>(mappedResource.pData) + offset;
        return true;
    }
    else
    {
        ERR("Index buffer not initialized.");
        return false;
    }
}

bool IndexBuffer11::unmapBuffer()
{
    if (mBuffer)
    {
        ID3D11DeviceContext *dxContext = mRenderer->getDeviceContext();
        dxContext->Unmap(mBuffer, 0);
        return true;
    }
    else
    {
        ERR("Index buffer not initialized.");
        return false;
    }
}

GLenum IndexBuffer11::getIndexType() const
{
    return mIndexType;
}

unsigned int IndexBuffer11::getBufferSize() const
{
    return mBufferSize;
}

bool IndexBuffer11::setSize(unsigned int bufferSize, GLenum indexType)
{
    if (bufferSize > mBufferSize || indexType != mIndexType)
    {
        return initialize(bufferSize, indexType, mDynamicUsage);
    }
    else
    {
        return true;
    }
}

bool IndexBuffer11::discard()
{
    if (mBuffer)
    {
        ID3D11DeviceContext *dxContext = mRenderer->getDeviceContext();

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT result = dxContext->Map(mBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (FAILED(result))
        {
            ERR("Index buffer map failed with error 0x%08x", result);
            return false;
        }

        dxContext->Unmap(mBuffer, 0);

        return true;
    }
    else
    {
        ERR("Index buffer not initialized.");
        return false;
    }
}

DXGI_FORMAT IndexBuffer11::getIndexFormat() const
{
    switch (mIndexType)
    {
      case GL_UNSIGNED_BYTE:    return DXGI_FORMAT_R16_UINT;
      case GL_UNSIGNED_SHORT:   return DXGI_FORMAT_R16_UINT;
      case GL_UNSIGNED_INT:     return DXGI_FORMAT_R32_UINT;
      default: UNREACHABLE();   return DXGI_FORMAT_UNKNOWN;
    }
}

ID3D11Buffer *IndexBuffer11::getBuffer() const
{
    return mBuffer;
}

}