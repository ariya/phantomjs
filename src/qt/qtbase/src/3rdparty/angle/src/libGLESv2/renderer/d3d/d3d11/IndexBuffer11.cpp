//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// IndexBuffer11.cpp: Defines the D3D11 IndexBuffer implementation.

#include "libGLESv2/renderer/d3d/d3d11/IndexBuffer11.h"
#include "libGLESv2/renderer/d3d/d3d11/Renderer11.h"

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
    SafeRelease(mBuffer);
}

gl::Error IndexBuffer11::initialize(unsigned int bufferSize, GLenum indexType, bool dynamic)
{
    SafeRelease(mBuffer);

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
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to allocate internal index buffer of size, %lu.", bufferSize);
        }
    }

    mBufferSize = bufferSize;
    mIndexType = indexType;
    mDynamicUsage = dynamic;

    return gl::Error(GL_NO_ERROR);
}

IndexBuffer11 *IndexBuffer11::makeIndexBuffer11(IndexBuffer *indexBuffer)
{
    ASSERT(HAS_DYNAMIC_TYPE(IndexBuffer11*, indexBuffer));
    return static_cast<IndexBuffer11*>(indexBuffer);
}

gl::Error IndexBuffer11::mapBuffer(unsigned int offset, unsigned int size, void** outMappedMemory)
{
    if (!mBuffer)
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Internal index buffer is not initialized.");
    }

    // Check for integer overflows and out-out-bounds map requests
    if (offset + size < offset || offset + size > mBufferSize)
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Index buffer map range is not inside the buffer.");
    }

    ID3D11DeviceContext *dxContext = mRenderer->getDeviceContext();

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT result = dxContext->Map(mBuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedResource);
    if (FAILED(result))
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to map internal index buffer, HRESULT: 0x%08x.", result);
    }

    *outMappedMemory = reinterpret_cast<char*>(mappedResource.pData) + offset;
    return gl::Error(GL_NO_ERROR);
}

gl::Error IndexBuffer11::unmapBuffer()
{
    if (!mBuffer)
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Internal index buffer is not initialized.");
    }

    ID3D11DeviceContext *dxContext = mRenderer->getDeviceContext();
    dxContext->Unmap(mBuffer, 0);
    return gl::Error(GL_NO_ERROR);
}

GLenum IndexBuffer11::getIndexType() const
{
    return mIndexType;
}

unsigned int IndexBuffer11::getBufferSize() const
{
    return mBufferSize;
}

gl::Error IndexBuffer11::setSize(unsigned int bufferSize, GLenum indexType)
{
    if (bufferSize > mBufferSize || indexType != mIndexType)
    {
        return initialize(bufferSize, indexType, mDynamicUsage);
    }
    else
    {
        return gl::Error(GL_NO_ERROR);
    }
}

gl::Error IndexBuffer11::discard()
{
    if (!mBuffer)
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Internal index buffer is not initialized.");
    }

    ID3D11DeviceContext *dxContext = mRenderer->getDeviceContext();

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT result = dxContext->Map(mBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(result))
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to map internal index buffer, HRESULT: 0x%08x.", result);
    }

    dxContext->Unmap(mBuffer, 0);

    return gl::Error(GL_NO_ERROR);
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
