#include "precompiled.h"
//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Indexffer9.cpp: Defines the D3D9 IndexBuffer implementation.

#include "libGLESv2/renderer/IndexBuffer9.h"
#include "libGLESv2/renderer/Renderer9.h"

namespace rx
{

IndexBuffer9::IndexBuffer9(Renderer9 *const renderer) : mRenderer(renderer)
{
    mIndexBuffer = NULL;
    mBufferSize = 0;
    mIndexType = 0;
    mDynamic = false;
}

IndexBuffer9::~IndexBuffer9()
{
    if (mIndexBuffer)
    {
        mIndexBuffer->Release();
        mIndexBuffer = NULL;
    }
}

bool IndexBuffer9::initialize(unsigned int bufferSize, GLenum indexType, bool dynamic)
{
    if (mIndexBuffer)
    {
        mIndexBuffer->Release();
        mIndexBuffer = NULL;
    }

    updateSerial();

    if (bufferSize > 0)
    {
        D3DFORMAT format;
        if (indexType == GL_UNSIGNED_SHORT || indexType == GL_UNSIGNED_BYTE)
        {
            format = D3DFMT_INDEX16;
        }
        else if (indexType == GL_UNSIGNED_INT)
        {
            if (mRenderer->get32BitIndexSupport())
            {
                format = D3DFMT_INDEX32;
            }
            else
            {
                ERR("Attempted to create a 32-bit index buffer but renderer does not support 32-bit indices.");
                return false;
            }
        }
        else
        {
            ERR("Invalid index type %u.", indexType);
            return false;
        }

        DWORD usageFlags = D3DUSAGE_WRITEONLY;
        if (dynamic)
        {
            usageFlags |= D3DUSAGE_DYNAMIC;
        }

        HRESULT result = mRenderer->createIndexBuffer(bufferSize, usageFlags, format, &mIndexBuffer);
        if (FAILED(result))
        {
            ERR("Failed to create an index buffer of size %u, result: 0x%08x.", mBufferSize, result);
            return false;
        }
    }

    mBufferSize = bufferSize;
    mIndexType = indexType;
    mDynamic = dynamic;

    return true;
}

IndexBuffer9 *IndexBuffer9::makeIndexBuffer9(IndexBuffer *indexBuffer)
{
    ASSERT(HAS_DYNAMIC_TYPE(IndexBuffer9*, indexBuffer));
    return static_cast<IndexBuffer9*>(indexBuffer);
}

bool IndexBuffer9::mapBuffer(unsigned int offset, unsigned int size, void** outMappedMemory)
{
    if (mIndexBuffer)
    {
        DWORD lockFlags = mDynamic ? D3DLOCK_NOOVERWRITE : 0;

        void *mapPtr = NULL;
        HRESULT result = mIndexBuffer->Lock(offset, size, &mapPtr, lockFlags);
        if (FAILED(result))
        {
            ERR("Index buffer lock failed with error 0x%08x", result);
            return false;
        }

        *outMappedMemory = mapPtr;
        return true;
    }
    else
    {
        ERR("Index buffer not initialized.");
        return false;
    }
}

bool IndexBuffer9::unmapBuffer()
{
    if (mIndexBuffer)
    {
        HRESULT result = mIndexBuffer->Unlock();
        if (FAILED(result))
        {
            ERR("Index buffer unlock failed with error 0x%08x", result);
            return false;
        }

        return true;
    }
    else
    {
        ERR("Index buffer not initialized.");
        return false;
    }
}

GLenum IndexBuffer9::getIndexType() const
{
    return mIndexType;
}

unsigned int IndexBuffer9::getBufferSize() const
{
    return mBufferSize;
}

bool IndexBuffer9::setSize(unsigned int bufferSize, GLenum indexType)
{
    if (bufferSize > mBufferSize || indexType != mIndexType)
    {
        return initialize(bufferSize, indexType, mDynamic);
    }
    else
    {
        return true;
    }
}

bool IndexBuffer9::discard()
{
    if (mIndexBuffer)
    {
        void *dummy;
        HRESULT result;

        result = mIndexBuffer->Lock(0, 1, &dummy, D3DLOCK_DISCARD);
        if (FAILED(result))
        {
            ERR("Discard lock failed with error 0x%08x", result);
            return false;
        }

        result = mIndexBuffer->Unlock();
        if (FAILED(result))
        {
            ERR("Discard unlock failed with error 0x%08x", result);
            return false;
        }

        return true;
    }
    else
    {
        ERR("Index buffer not initialized.");
        return false;
    }
}

D3DFORMAT IndexBuffer9::getIndexFormat() const
{
    switch (mIndexType)
    {
      case GL_UNSIGNED_BYTE:    return D3DFMT_INDEX16;
      case GL_UNSIGNED_SHORT:   return D3DFMT_INDEX16;
      case GL_UNSIGNED_INT:     return D3DFMT_INDEX32;
      default: UNREACHABLE();   return D3DFMT_UNKNOWN;
    }
}

IDirect3DIndexBuffer9 * IndexBuffer9::getBuffer() const
{
    return mIndexBuffer;
}

}