#include "precompiled.h"
//
// Copyright (c) 2012-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ShaderExecutable11.cpp: Implements a D3D11-specific class to contain shader
// executable implementation details.

#include "libGLESv2/renderer/ShaderExecutable11.h"

#include "common/debug.h"

namespace rx
{

ShaderExecutable11::ShaderExecutable11(const void *function, size_t length, ID3D11PixelShader *executable)
    : ShaderExecutable(function, length)
{
    mPixelExecutable = executable;
    mVertexExecutable = NULL;
    mGeometryExecutable = NULL;

    mConstantBuffer = NULL;
}

ShaderExecutable11::ShaderExecutable11(const void *function, size_t length, ID3D11VertexShader *executable)
    : ShaderExecutable(function, length)
{
    mVertexExecutable = executable;
    mPixelExecutable = NULL;
    mGeometryExecutable = NULL;

    mConstantBuffer = NULL;
}

ShaderExecutable11::ShaderExecutable11(const void *function, size_t length, ID3D11GeometryShader *executable)
    : ShaderExecutable(function, length)
{
    mGeometryExecutable = executable;
    mVertexExecutable = NULL;
    mPixelExecutable = NULL;

    mConstantBuffer = NULL;
}

ShaderExecutable11::~ShaderExecutable11()
{
    if (mVertexExecutable)
    {
        mVertexExecutable->Release();
    }
    if (mPixelExecutable)
    {
        mPixelExecutable->Release();
    }
    if (mGeometryExecutable)
    {
        mGeometryExecutable->Release();
    }
    
    if (mConstantBuffer)
    {
        mConstantBuffer->Release();
    }
}

ShaderExecutable11 *ShaderExecutable11::makeShaderExecutable11(ShaderExecutable *executable)
{
    ASSERT(HAS_DYNAMIC_TYPE(ShaderExecutable11*, executable));
    return static_cast<ShaderExecutable11*>(executable);
}

ID3D11VertexShader *ShaderExecutable11::getVertexShader() const
{
    return mVertexExecutable;
}

ID3D11PixelShader *ShaderExecutable11::getPixelShader() const
{
    return mPixelExecutable;
}

ID3D11GeometryShader *ShaderExecutable11::getGeometryShader() const
{
    return mGeometryExecutable;
}

ID3D11Buffer *ShaderExecutable11::getConstantBuffer(ID3D11Device *device, unsigned int registerCount)
{
    if (!mConstantBuffer && registerCount > 0)
    {
        D3D11_BUFFER_DESC constantBufferDescription = {0};
        constantBufferDescription.ByteWidth = registerCount * sizeof(float[4]);
        constantBufferDescription.Usage = D3D11_USAGE_DYNAMIC;
        constantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        constantBufferDescription.MiscFlags = 0;
        constantBufferDescription.StructureByteStride = 0;

        HRESULT result = device->CreateBuffer(&constantBufferDescription, NULL, &mConstantBuffer);
        ASSERT(SUCCEEDED(result));
    }

    return mConstantBuffer;
}

}