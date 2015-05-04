//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// PixelTransfer11.cpp:
//   Implementation for buffer-to-texture and texture-to-buffer copies.
//   Used to implement pixel transfers from unpack and to pack buffers.
//

#include "libGLESv2/renderer/d3d/d3d11/PixelTransfer11.h"
#include "libGLESv2/renderer/d3d/d3d11/Renderer11.h"
#include "libGLESv2/renderer/d3d/d3d11/renderer11_utils.h"
#include "libGLESv2/renderer/d3d/d3d11/formatutils11.h"
#include "libGLESv2/renderer/d3d/d3d11/Buffer11.h"
#include "libGLESv2/renderer/d3d/d3d11/TextureStorage11.h"
#include "libGLESv2/renderer/d3d/d3d11/RenderTarget11.h"
#include "libGLESv2/formatutils.h"
#include "libGLESv2/Texture.h"
#include "libGLESv2/Buffer.h"
#include "libGLESv2/Context.h"

// Precompiled shaders
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/buffertotexturevs.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/buffertotexturegs.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/buffertotexture_4fps.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/buffertotexture_4ips.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/buffertotexture_4uips.h"

namespace rx
{

PixelTransfer11::PixelTransfer11(Renderer11 *renderer)
    : mRenderer(renderer),
      mResourcesLoaded(false),
      mBufferToTextureVS(NULL),
      mBufferToTextureGS(NULL),
      mParamsConstantBuffer(NULL),
      mCopyRasterizerState(NULL),
      mCopyDepthStencilState(NULL)
{
}

PixelTransfer11::~PixelTransfer11()
{
    for (auto shaderMapIt = mBufferToTexturePSMap.begin(); shaderMapIt != mBufferToTexturePSMap.end(); shaderMapIt++)
    {
        SafeRelease(shaderMapIt->second);
    }

    mBufferToTexturePSMap.clear();

    SafeRelease(mBufferToTextureVS);
    SafeRelease(mBufferToTextureGS);
    SafeRelease(mParamsConstantBuffer);
    SafeRelease(mCopyRasterizerState);
    SafeRelease(mCopyDepthStencilState);
}

gl::Error PixelTransfer11::loadResources()
{
    if (mResourcesLoaded)
    {
        return gl::Error(GL_NO_ERROR);
    }

    HRESULT result = S_OK;
    ID3D11Device *device = mRenderer->getDevice();

    D3D11_RASTERIZER_DESC rasterDesc;
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.FrontCounterClockwise = FALSE;
    rasterDesc.DepthBias = 0;
    rasterDesc.SlopeScaledDepthBias = 0.0f;
    rasterDesc.DepthBiasClamp = 0.0f;
    rasterDesc.DepthClipEnable = TRUE;
    rasterDesc.ScissorEnable = FALSE;
    rasterDesc.MultisampleEnable = FALSE;
    rasterDesc.AntialiasedLineEnable = FALSE;

    result = device->CreateRasterizerState(&rasterDesc, &mCopyRasterizerState);
    ASSERT(SUCCEEDED(result));
    if (FAILED(result))
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal pixel transfer rasterizer state, result: 0x%X.", result);
    }

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    depthStencilDesc.StencilEnable = FALSE;
    depthStencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    depthStencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    result = device->CreateDepthStencilState(&depthStencilDesc, &mCopyDepthStencilState);
    ASSERT(SUCCEEDED(result));
    if (FAILED(result))
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal pixel transfer depth stencil state, result: 0x%X.", result);
    }

    D3D11_BUFFER_DESC constantBufferDesc = { 0 };
    constantBufferDesc.ByteWidth = roundUp<UINT>(sizeof(CopyShaderParams), 32u);
    constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    constantBufferDesc.MiscFlags = 0;
    constantBufferDesc.StructureByteStride = 0;

    result = device->CreateBuffer(&constantBufferDesc, NULL, &mParamsConstantBuffer);
    ASSERT(SUCCEEDED(result));
    if (FAILED(result))
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal pixel transfer constant buffer, result: 0x%X.", result);
    }
    d3d11::SetDebugName(mParamsConstantBuffer, "PixelTransfer11 constant buffer");

    // init shaders
    mBufferToTextureVS = d3d11::CompileVS(device, g_VS_BufferToTexture, "BufferToTexture VS");
    if (!mBufferToTextureVS)
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal buffer to texture vertex shader.");
    }

    if (!mRenderer->isLevel9())
    {
        mBufferToTextureGS = d3d11::CompileGS(device, g_GS_BufferToTexture, "BufferToTexture GS");
        if (!mBufferToTextureGS)
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal buffer to texture geometry shader.");
        }
    }

    gl::Error error = buildShaderMap();
    if (error.isError())
    {
        return error;
    }

    StructZero(&mParamsData);

    mResourcesLoaded = true;

    return gl::Error(GL_NO_ERROR);
}

void PixelTransfer11::setBufferToTextureCopyParams(const gl::Box &destArea, const gl::Extents &destSize, GLenum internalFormat,
                                                   const gl::PixelUnpackState &unpack, unsigned int offset, CopyShaderParams *parametersOut)
{
    StructZero(parametersOut);

    float texelCenterX = 0.5f / static_cast<float>(destSize.width - 1);
    float texelCenterY = 0.5f / static_cast<float>(destSize.height - 1);

    unsigned int bytesPerPixel = gl::GetInternalFormatInfo(internalFormat).pixelBytes;
    unsigned int alignmentBytes = static_cast<unsigned int>(unpack.alignment);
    unsigned int alignmentPixels = (alignmentBytes <= bytesPerPixel ? 1 : alignmentBytes / bytesPerPixel);

    parametersOut->FirstPixelOffset     = offset;
    parametersOut->PixelsPerRow         = static_cast<unsigned int>(destArea.width);
    parametersOut->RowStride            = roundUp(parametersOut->PixelsPerRow, alignmentPixels);
    parametersOut->RowsPerSlice         = static_cast<unsigned int>(destArea.height);
    parametersOut->PositionOffset[0]    = texelCenterX + (destArea.x / float(destSize.width)) * 2.0f - 1.0f;
    parametersOut->PositionOffset[1]    = texelCenterY + ((destSize.height - destArea.y - 1) / float(destSize.height)) * 2.0f - 1.0f;
    parametersOut->PositionScale[0]     = 2.0f / static_cast<float>(destSize.width);
    parametersOut->PositionScale[1]     = -2.0f / static_cast<float>(destSize.height);
}

gl::Error PixelTransfer11::copyBufferToTexture(const gl::PixelUnpackState &unpack, unsigned int offset, RenderTarget *destRenderTarget,
                                               GLenum destinationFormat, GLenum sourcePixelsType, const gl::Box &destArea)
{
    gl::Error error = loadResources();
    if (error.isError())
    {
        return error;
    }

    gl::Extents destSize = destRenderTarget->getExtents();

    ASSERT(destArea.x >= 0 && destArea.x + destArea.width  <= destSize.width  &&
           destArea.y >= 0 && destArea.y + destArea.height <= destSize.height &&
           destArea.z >= 0 && destArea.z + destArea.depth  <= destSize.depth  );

    const gl::Buffer &sourceBuffer = *unpack.pixelBuffer.get();

    ASSERT(mRenderer->supportsFastCopyBufferToTexture(destinationFormat));

    ID3D11PixelShader *pixelShader = findBufferToTexturePS(destinationFormat);
    ASSERT(pixelShader);

    // The SRV must be in the proper read format, which may be different from the destination format
    // EG: for half float data, we can load full precision floats with implicit conversion
    GLenum unsizedFormat = gl::GetInternalFormatInfo(destinationFormat).format;
    GLenum sourceFormat = gl::GetFormatTypeInfo(unsizedFormat, sourcePixelsType).internalFormat;

    const d3d11::TextureFormat &sourceFormatInfo = d3d11::GetTextureFormatInfo(sourceFormat);
    DXGI_FORMAT srvFormat = sourceFormatInfo.srvFormat;
    ASSERT(srvFormat != DXGI_FORMAT_UNKNOWN);
    Buffer11 *bufferStorage11 = Buffer11::makeBuffer11(sourceBuffer.getImplementation());
    ID3D11ShaderResourceView *bufferSRV = bufferStorage11->getSRV(srvFormat);
    ASSERT(bufferSRV != NULL);

    ID3D11RenderTargetView *textureRTV = RenderTarget11::makeRenderTarget11(destRenderTarget)->getRenderTargetView();
    ASSERT(textureRTV != NULL);

    CopyShaderParams shaderParams;
    setBufferToTextureCopyParams(destArea, destSize, sourceFormat, unpack, offset, &shaderParams);

    ID3D11DeviceContext *deviceContext = mRenderer->getDeviceContext();

    ID3D11Buffer *nullBuffer = NULL;
    UINT zero = 0;

    // Are we doing a 2D or 3D copy?
    ID3D11GeometryShader *geometryShader = ((destSize.depth > 1) ? mBufferToTextureGS : NULL);

    deviceContext->VSSetShader(mBufferToTextureVS, NULL, 0);
    deviceContext->GSSetShader(geometryShader, NULL, 0);
    deviceContext->PSSetShader(pixelShader, NULL, 0);
    mRenderer->setShaderResource(gl::SAMPLER_PIXEL, 0, bufferSRV);
    deviceContext->IASetInputLayout(NULL);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

    deviceContext->IASetVertexBuffers(0, 1, &nullBuffer, &zero, &zero);
    deviceContext->OMSetBlendState(NULL, NULL, 0xFFFFFFF);
    deviceContext->OMSetDepthStencilState(mCopyDepthStencilState, 0xFFFFFFFF);
    deviceContext->RSSetState(mCopyRasterizerState);

    mRenderer->setOneTimeRenderTarget(textureRTV);

    if (!StructEquals(mParamsData, shaderParams))
    {
        d3d11::SetBufferData(deviceContext, mParamsConstantBuffer, shaderParams);
        mParamsData = shaderParams;
    }

    deviceContext->VSSetConstantBuffers(0, 1, &mParamsConstantBuffer);

    // Set the viewport
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = destSize.width;
    viewport.Height = destSize.height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    deviceContext->RSSetViewports(1, &viewport);

    UINT numPixels = (destArea.width * destArea.height * destArea.depth);
    deviceContext->Draw(numPixels, 0);

    // Unbind textures and render targets and vertex buffer
    mRenderer->setShaderResource(gl::SAMPLER_PIXEL, 0, NULL);
    deviceContext->VSSetConstantBuffers(0, 1, &nullBuffer);

    mRenderer->markAllStateDirty();

    return gl::Error(GL_NO_ERROR);
}

gl::Error PixelTransfer11::buildShaderMap()
{
    ID3D11Device *device = mRenderer->getDevice();

    mBufferToTexturePSMap[GL_FLOAT]        = d3d11::CompilePS(device, g_PS_BufferToTexture_4F,  "BufferToTexture RGBA ps");
    mBufferToTexturePSMap[GL_INT]          = d3d11::CompilePS(device, g_PS_BufferToTexture_4I,  "BufferToTexture RGBA-I ps");
    mBufferToTexturePSMap[GL_UNSIGNED_INT] = d3d11::CompilePS(device, g_PS_BufferToTexture_4UI, "BufferToTexture RGBA-UI ps");

    // Check that all the shaders were created successfully
    for (auto shaderMapIt = mBufferToTexturePSMap.begin(); shaderMapIt != mBufferToTexturePSMap.end(); shaderMapIt++)
    {
        if (shaderMapIt->second == NULL)
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal buffer to texture pixel shader.");
        }
    }

    return gl::Error(GL_NO_ERROR);
}

ID3D11PixelShader *PixelTransfer11::findBufferToTexturePS(GLenum internalFormat) const
{
    GLenum componentType = gl::GetInternalFormatInfo(internalFormat).componentType;
    if (componentType == GL_SIGNED_NORMALIZED || componentType == GL_UNSIGNED_NORMALIZED)
    {
        componentType = GL_FLOAT;
    }

    auto shaderMapIt = mBufferToTexturePSMap.find(componentType);
    return (shaderMapIt == mBufferToTexturePSMap.end() ? NULL : shaderMapIt->second);
}

}
