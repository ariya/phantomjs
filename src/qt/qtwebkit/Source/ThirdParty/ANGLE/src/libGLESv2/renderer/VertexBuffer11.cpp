#include "precompiled.h"
//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexBuffer11.cpp: Defines the D3D11 VertexBuffer implementation.

#include "libGLESv2/renderer/VertexBuffer11.h"
#include "libGLESv2/renderer/BufferStorage.h"

#include "libGLESv2/Buffer.h"
#include "libGLESv2/renderer/Renderer11.h"
#include "libGLESv2/Context.h"

namespace rx
{

VertexBuffer11::VertexBuffer11(rx::Renderer11 *const renderer) : mRenderer(renderer)
{
    mBuffer = NULL;
    mBufferSize = 0;
    mDynamicUsage = false;
}

VertexBuffer11::~VertexBuffer11()
{
    if (mBuffer)
    {
        mBuffer->Release();
        mBuffer = NULL;
    }
}

bool VertexBuffer11::initialize(unsigned int size, bool dynamicUsage)
{
    if (mBuffer)
    {
        mBuffer->Release();
        mBuffer = NULL;
    }

    updateSerial();

    if (size > 0)
    {
        ID3D11Device* dxDevice = mRenderer->getDevice();

        D3D11_BUFFER_DESC bufferDesc;
        bufferDesc.ByteWidth = size;
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        bufferDesc.MiscFlags = 0;
        bufferDesc.StructureByteStride = 0;

        HRESULT result = dxDevice->CreateBuffer(&bufferDesc, NULL, &mBuffer);
        if (FAILED(result))
        {
            return false;
        }
    }

    mBufferSize = size;
    mDynamicUsage = dynamicUsage;
    return true;
}

VertexBuffer11 *VertexBuffer11::makeVertexBuffer11(VertexBuffer *vetexBuffer)
{
    ASSERT(HAS_DYNAMIC_TYPE(VertexBuffer11*, vetexBuffer));
    return static_cast<VertexBuffer11*>(vetexBuffer);
}

bool VertexBuffer11::storeVertexAttributes(const gl::VertexAttribute &attrib, GLint start, GLsizei count,
                                           GLsizei instances, unsigned int offset)
{
    if (mBuffer)
    {
        gl::Buffer *buffer = attrib.mBoundBuffer.get();

        int inputStride = attrib.stride();
        const VertexConverter &converter = getVertexConversion(attrib);

        ID3D11DeviceContext *dxContext = mRenderer->getDeviceContext();

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT result = dxContext->Map(mBuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedResource);
        if (FAILED(result))
        {
            ERR("Vertex buffer map failed with error 0x%08x", result);
            return false;
        }

        char* output = reinterpret_cast<char*>(mappedResource.pData) + offset;

        const char *input = NULL;
        if (buffer)
        {
            BufferStorage *storage = buffer->getStorage();
            input = static_cast<const char*>(storage->getData()) + static_cast<int>(attrib.mOffset);
        }
        else
        {
            input = static_cast<const char*>(attrib.mPointer);
        }

        if (instances == 0 || attrib.mDivisor == 0)
        {
            input += inputStride * start;
        }

        converter.conversionFunc(input, inputStride, count, output);

        dxContext->Unmap(mBuffer, 0);

        return true;
    }
    else
    {
        ERR("Vertex buffer not initialized.");
        return false;
    }
}

bool VertexBuffer11::storeRawData(const void* data, unsigned int size, unsigned int offset)
{
    if (mBuffer)
    {
        ID3D11DeviceContext *dxContext = mRenderer->getDeviceContext();

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT result = dxContext->Map(mBuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedResource);
        if (FAILED(result))
        {
            ERR("Vertex buffer map failed with error 0x%08x", result);
            return false;
        }

        char* bufferData = static_cast<char*>(mappedResource.pData);
        memcpy(bufferData + offset, data, size);

        dxContext->Unmap(mBuffer, 0);

        return true;
    }
    else
    {
        ERR("Vertex buffer not initialized.");
        return false;
    }
}

unsigned int VertexBuffer11::getSpaceRequired(const gl::VertexAttribute &attrib, GLsizei count,
                                              GLsizei instances) const
{
    unsigned int elementSize = getVertexConversion(attrib).outputElementSize;

    if (instances == 0 || attrib.mDivisor == 0)
    {
        return elementSize * count;
    }
    else
    {
        return elementSize * ((instances + attrib.mDivisor - 1) / attrib.mDivisor);
    }
}

bool VertexBuffer11::requiresConversion(const gl::VertexAttribute &attrib) const
{
    return !getVertexConversion(attrib).identity;
}

unsigned int VertexBuffer11::getBufferSize() const
{
    return mBufferSize;
}

bool VertexBuffer11::setBufferSize(unsigned int size)
{
    if (size > mBufferSize)
    {
        return initialize(size, mDynamicUsage);
    }
    else
    {
        return true;
    }
}

bool VertexBuffer11::discard()
{
    if (mBuffer)
    {
        ID3D11DeviceContext *dxContext = mRenderer->getDeviceContext();

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT result = dxContext->Map(mBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (FAILED(result))
        {
            ERR("Vertex buffer map failed with error 0x%08x", result);
            return false;
        }

        dxContext->Unmap(mBuffer, 0);

        return true;
    }
    else
    {
        ERR("Vertex buffer not initialized.");
        return false;
    }
}

unsigned int VertexBuffer11::getVertexSize(const gl::VertexAttribute &attrib) const
{
    return getVertexConversion(attrib).outputElementSize;
}

DXGI_FORMAT VertexBuffer11::getDXGIFormat(const gl::VertexAttribute &attrib) const
{
    return getVertexConversion(attrib).dxgiFormat;
}

ID3D11Buffer *VertexBuffer11::getBuffer() const
{
    return mBuffer;
}

template <typename T, unsigned int componentCount, bool widen, bool normalized>
static void copyVertexData(const void *input, unsigned int stride, unsigned int count, void *output)
{
    unsigned int attribSize = sizeof(T) * componentCount;

    if (attribSize == stride && !widen)
    {
        memcpy(output, input, count * attribSize);
    }
    else
    {
        unsigned int outputStride = widen ? 4 : componentCount;
        T defaultVal = normalized ? std::numeric_limits<T>::max() : T(1);

        for (unsigned int i = 0; i < count; i++)
        {
            const T *offsetInput = reinterpret_cast<const T*>(reinterpret_cast<const char*>(input) + i * stride);
            T *offsetOutput = reinterpret_cast<T*>(output) + i * outputStride;

            for (unsigned int j = 0; j < componentCount; j++)
            {
                offsetOutput[j] = offsetInput[j];
            }

            if (widen)
            {
                offsetOutput[3] = defaultVal;
            }
        }
    }
}

template <unsigned int componentCount>
static void copyFixedVertexData(const void* input, unsigned int stride, unsigned int count, void* output)
{
    static const float divisor = 1.0f / (1 << 16);

    for (unsigned int i = 0; i < count; i++)
    {
        const GLfixed* offsetInput = reinterpret_cast<const GLfixed*>(reinterpret_cast<const char*>(input) + stride * i);
        float* offsetOutput = reinterpret_cast<float*>(output) + i * componentCount;

        for (unsigned int j = 0; j < componentCount; j++)
        {
            offsetOutput[j] = static_cast<float>(offsetInput[j]) * divisor;
        }
    }
}

template <typename T, unsigned int componentCount, bool normalized>
static void copyToFloatVertexData(const void* input, unsigned int stride, unsigned int count, void* output)
{
    typedef std::numeric_limits<T> NL;

    for (unsigned int i = 0; i < count; i++)
    {
        const T *offsetInput = reinterpret_cast<const T*>(reinterpret_cast<const char*>(input) + stride * i);
        float *offsetOutput = reinterpret_cast<float*>(output) + i * componentCount;

        for (unsigned int j = 0; j < componentCount; j++)
        {
            if (normalized)
            {
                if (NL::is_signed)
                {
                    const float divisor = 1.0f / (2 * static_cast<float>(NL::max()) + 1);
                    offsetOutput[j] = (2 * static_cast<float>(offsetInput[j]) + 1) * divisor;
                }
                else
                {
                    offsetOutput[j] =  static_cast<float>(offsetInput[j]) / NL::max();
                }
            }
            else
            {
                offsetOutput[j] =  static_cast<float>(offsetInput[j]);
            }
        }
    }
}

const VertexBuffer11::VertexConverter VertexBuffer11::mPossibleTranslations[NUM_GL_VERTEX_ATTRIB_TYPES][2][4] =
{
    { // GL_BYTE
        { // unnormalized
            { &copyToFloatVertexData<GLbyte, 1, false>, false, DXGI_FORMAT_R32_FLOAT, 4 },
            { &copyToFloatVertexData<GLbyte, 2, false>, false, DXGI_FORMAT_R32G32_FLOAT, 8 },
            { &copyToFloatVertexData<GLbyte, 3, false>, false, DXGI_FORMAT_R32G32B32_FLOAT, 12 },
            { &copyToFloatVertexData<GLbyte, 4, false>, false, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
        { // normalized
            { &copyVertexData<GLbyte, 1, false, true>, true, DXGI_FORMAT_R8_SNORM, 1 },
            { &copyVertexData<GLbyte, 2, false, true>, true, DXGI_FORMAT_R8G8_SNORM, 2 },
            { &copyVertexData<GLbyte, 3, true, true>, false, DXGI_FORMAT_R8G8B8A8_SNORM, 4 },
            { &copyVertexData<GLbyte, 4, false, true>, true, DXGI_FORMAT_R8G8B8A8_SNORM, 4 },
        },
    },
    { // GL_UNSIGNED_BYTE
        { // unnormalized
            { &copyToFloatVertexData<GLubyte, 1, false>, false, DXGI_FORMAT_R32_FLOAT, 4 },
            { &copyToFloatVertexData<GLubyte, 2, false>, false, DXGI_FORMAT_R32G32_FLOAT, 8 },
            { &copyToFloatVertexData<GLubyte, 3, false>, false, DXGI_FORMAT_R32G32B32_FLOAT, 12 },
            { &copyToFloatVertexData<GLubyte, 4, false>, false, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
        { // normalized
            { &copyVertexData<GLubyte, 1, false, true>, true, DXGI_FORMAT_R8_UNORM, 1 },
            { &copyVertexData<GLubyte, 2, false, true>, true, DXGI_FORMAT_R8G8_UNORM, 2 },
            { &copyVertexData<GLubyte, 3, true, true>, false, DXGI_FORMAT_R8G8B8A8_UNORM, 4 },
            { &copyVertexData<GLubyte, 4, false, true>, true, DXGI_FORMAT_R8G8B8A8_UNORM, 4 },
        },
    },
    { // GL_SHORT
        { // unnormalized
            { &copyToFloatVertexData<GLshort, 1, false>, false, DXGI_FORMAT_R32_FLOAT, 4 },
            { &copyToFloatVertexData<GLshort, 2, false>, false, DXGI_FORMAT_R32G32_FLOAT, 8 },
            { &copyToFloatVertexData<GLshort, 3, false>, false, DXGI_FORMAT_R32G32B32_FLOAT, 12 },
            { &copyToFloatVertexData<GLshort, 4, false>, false, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
        { // normalized
            { &copyVertexData<GLshort, 1, false, true>, true, DXGI_FORMAT_R16_SNORM, 2 },
            { &copyVertexData<GLshort, 2, false, true>, true, DXGI_FORMAT_R16G16_SNORM, 4 },
            { &copyVertexData<GLshort, 3, true, true>, false, DXGI_FORMAT_R16G16B16A16_SNORM, 8 },
            { &copyVertexData<GLshort, 4, false, true>, true, DXGI_FORMAT_R16G16B16A16_SNORM, 8 },
        },
    },
    { // GL_UNSIGNED_SHORT
        { // unnormalized
            { &copyToFloatVertexData<GLushort, 1, false>, false, DXGI_FORMAT_R32_FLOAT, 4 },
            { &copyToFloatVertexData<GLushort, 2, false>, false, DXGI_FORMAT_R32G32_FLOAT, 8 },
            { &copyToFloatVertexData<GLushort, 3, false>, false, DXGI_FORMAT_R32G32B32_FLOAT, 12 },
            { &copyToFloatVertexData<GLushort, 4, false>, false, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
        { // normalized
            { &copyVertexData<GLushort, 1, false, true>, true, DXGI_FORMAT_R16_UNORM, 2 },
            { &copyVertexData<GLushort, 2, false, true>, true, DXGI_FORMAT_R16G16_UNORM, 4 },
            { &copyVertexData<GLushort, 3, true, true>, false, DXGI_FORMAT_R16G16B16A16_UNORM, 8 },
            { &copyVertexData<GLushort, 4, false, true>, true, DXGI_FORMAT_R16G16B16A16_UNORM, 8 },
        },
    },
    { // GL_FIXED
        { // unnormalized
            { &copyFixedVertexData<1>, false, DXGI_FORMAT_R32_FLOAT, 4 },
            { &copyFixedVertexData<2>, false, DXGI_FORMAT_R32G32_FLOAT, 8 },
            { &copyFixedVertexData<3>, false, DXGI_FORMAT_R32G32B32_FLOAT, 12 },
            { &copyFixedVertexData<4>, false, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
        { // normalized
            { &copyFixedVertexData<1>, false, DXGI_FORMAT_R32_FLOAT, 4 },
            { &copyFixedVertexData<2>, false, DXGI_FORMAT_R32G32_FLOAT, 8 },
            { &copyFixedVertexData<3>, false, DXGI_FORMAT_R32G32B32_FLOAT, 12 },
            { &copyFixedVertexData<4>, false, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
    },
    { // GL_FLOAT
        { // unnormalized
            { &copyVertexData<GLfloat, 1, false, false>, true, DXGI_FORMAT_R32_FLOAT, 4 },
            { &copyVertexData<GLfloat, 2, false, false>, true, DXGI_FORMAT_R32G32_FLOAT, 8 },
            { &copyVertexData<GLfloat, 3, false, false>, true, DXGI_FORMAT_R32G32B32_FLOAT, 12 },
            { &copyVertexData<GLfloat, 4, false, false>, true, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
        { // normalized
            { &copyVertexData<GLfloat, 1, false, false>, true, DXGI_FORMAT_R32_FLOAT, 4 },
            { &copyVertexData<GLfloat, 2, false, false>, true, DXGI_FORMAT_R32G32_FLOAT, 8 },
            { &copyVertexData<GLfloat, 3, false, false>, true, DXGI_FORMAT_R32G32B32_FLOAT, 12 },
            { &copyVertexData<GLfloat, 4, false, false>, true, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
    },
};

const VertexBuffer11::VertexConverter &VertexBuffer11::getVertexConversion(const gl::VertexAttribute &attribute)
{
    unsigned int typeIndex = 0;
    switch (attribute.mType)
    {
      case GL_BYTE:             typeIndex = 0; break;
      case GL_UNSIGNED_BYTE:    typeIndex = 1; break;
      case GL_SHORT:            typeIndex = 2; break;
      case GL_UNSIGNED_SHORT:   typeIndex = 3; break;
      case GL_FIXED:            typeIndex = 4; break;
      case GL_FLOAT:            typeIndex = 5; break;
      default:                  UNREACHABLE(); break;
    }

    return mPossibleTranslations[typeIndex][attribute.mNormalized ? 1 : 0][attribute.mSize - 1];
}

}
