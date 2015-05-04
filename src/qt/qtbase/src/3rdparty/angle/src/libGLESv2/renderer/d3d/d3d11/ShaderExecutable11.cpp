//
// Copyright (c) 2012-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ShaderExecutable11.cpp: Implements a D3D11-specific class to contain shader
// executable implementation details.

#include "libGLESv2/renderer/d3d/d3d11/ShaderExecutable11.h"
#include "libGLESv2/renderer/d3d/d3d11/Renderer11.h"

namespace rx
{

ShaderExecutable11::ShaderExecutable11(const void *function, size_t length, ID3D11PixelShader *executable)
    : ShaderExecutable(function, length)
{
    mPixelExecutable = executable;
    mVertexExecutable = NULL;
    mGeometryExecutable = NULL;
    mStreamOutExecutable = NULL;
}

ShaderExecutable11::ShaderExecutable11(const void *function, size_t length, ID3D11VertexShader *executable, ID3D11GeometryShader *streamOut)
    : ShaderExecutable(function, length)
{
    mVertexExecutable = executable;
    mPixelExecutable = NULL;
    mGeometryExecutable = NULL;
    mStreamOutExecutable = streamOut;
}

ShaderExecutable11::ShaderExecutable11(const void *function, size_t length, ID3D11GeometryShader *executable)
    : ShaderExecutable(function, length)
{
    mGeometryExecutable = executable;
    mVertexExecutable = NULL;
    mPixelExecutable = NULL;
    mStreamOutExecutable = NULL;
}

ShaderExecutable11::~ShaderExecutable11()
{
    SafeRelease(mVertexExecutable);
    SafeRelease(mPixelExecutable);
    SafeRelease(mGeometryExecutable);
    SafeRelease(mStreamOutExecutable);
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

ID3D11GeometryShader *ShaderExecutable11::getStreamOutShader() const
{
    return mStreamOutExecutable;
}

UniformStorage11::UniformStorage11(Renderer11 *renderer, size_t initialSize)
    : UniformStorage(initialSize),
      mConstantBuffer(NULL)
{
    ID3D11Device *d3d11Device = renderer->getDevice();

    if (initialSize > 0)
    {
        D3D11_BUFFER_DESC constantBufferDescription = {0};
        constantBufferDescription.ByteWidth = initialSize;
        constantBufferDescription.Usage = D3D11_USAGE_DYNAMIC;
        constantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        constantBufferDescription.MiscFlags = 0;
        constantBufferDescription.StructureByteStride = 0;

        HRESULT result = d3d11Device->CreateBuffer(&constantBufferDescription, NULL, &mConstantBuffer);
        UNUSED_ASSERTION_VARIABLE(result);
        ASSERT(SUCCEEDED(result));
    }
}

UniformStorage11::~UniformStorage11()
{
    SafeRelease(mConstantBuffer);
}

const UniformStorage11 *UniformStorage11::makeUniformStorage11(const UniformStorage *uniformStorage)
{
    ASSERT(HAS_DYNAMIC_TYPE(const UniformStorage11*, uniformStorage));
    return static_cast<const UniformStorage11*>(uniformStorage);
}

}
