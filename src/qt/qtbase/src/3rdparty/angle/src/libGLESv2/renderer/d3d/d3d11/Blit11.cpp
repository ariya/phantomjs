//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Blit11.cpp: Texture copy utility class.

#include "libGLESv2/renderer/d3d/d3d11/Blit11.h"
#include "libGLESv2/renderer/d3d/d3d11/Renderer11.h"
#include "libGLESv2/renderer/d3d/d3d11/renderer11_utils.h"
#include "libGLESv2/renderer/d3d/d3d11/formatutils11.h"
#include "libGLESv2/main.h"
#include "libGLESv2/formatutils.h"

#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthrough2dvs.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughdepth2dps.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughrgba2dps.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughrgba2duips.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughrgba2dips.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughrgb2dps.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughrgb2duips.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughrgb2dips.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughrg2dps.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughrg2duips.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughrg2dips.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughr2dps.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughr2duips.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughr2dips.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughlum2dps.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughlumalpha2dps.h"

#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthrough3dvs.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthrough3dgs.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughrgba3dps.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughrgba3duips.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughrgba3dips.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughrgb3dps.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughrgb3duips.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughrgb3dips.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughrg3dps.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughrg3duips.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughrg3dips.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughr3dps.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughr3duips.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughr3dips.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughlum3dps.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/passthroughlumalpha3dps.h"

#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/swizzlef2dps.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/swizzlei2dps.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/swizzleui2dps.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/swizzlef3dps.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/swizzlei3dps.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/swizzleui3dps.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/swizzlef2darrayps.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/swizzlei2darrayps.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/swizzleui2darrayps.h"

namespace rx
{

static DXGI_FORMAT GetTextureFormat(ID3D11Resource *resource)
{
    ID3D11Texture2D *texture = d3d11::DynamicCastComObject<ID3D11Texture2D>(resource);
    if (!texture)
    {
        return DXGI_FORMAT_UNKNOWN;
    }

    D3D11_TEXTURE2D_DESC desc;
    texture->GetDesc(&desc);

    SafeRelease(texture);

    return desc.Format;
}

static ID3D11Resource *CreateStagingTexture(ID3D11Device *device, ID3D11DeviceContext *context,
                                            ID3D11Resource *source, unsigned int subresource,
                                            const gl::Extents &size, unsigned int cpuAccessFlags)
{
    D3D11_TEXTURE2D_DESC stagingDesc;
    stagingDesc.Width = size.width;
    stagingDesc.Height = size.height;
    stagingDesc.MipLevels = 1;
    stagingDesc.ArraySize = 1;
    stagingDesc.Format = GetTextureFormat(source);
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.SampleDesc.Quality = 0;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.CPUAccessFlags = cpuAccessFlags;
    stagingDesc.MiscFlags = 0;
    stagingDesc.BindFlags = 0;

    ID3D11Texture2D *stagingTexture = NULL;
    HRESULT result = device->CreateTexture2D(&stagingDesc, NULL, &stagingTexture);
    if (FAILED(result))
    {
        ERR("Failed to create staging texture for depth stencil blit. HRESULT: 0x%X.", result);
        return NULL;
    }

    context->CopySubresourceRegion(stagingTexture, 0, 0, 0, 0, source, subresource, NULL);

    return stagingTexture;
}

inline static void GenerateVertexCoords(const gl::Box &sourceArea, const gl::Extents &sourceSize,
                                        const gl::Box &destArea, const gl::Extents &destSize,
                                        float *x1, float *y1, float *x2, float *y2,
                                        float *u1, float *v1, float *u2, float *v2)
{
    *x1 = (destArea.x / float(destSize.width)) * 2.0f - 1.0f;
    *y1 = ((destSize.height - destArea.y - destArea.height) / float(destSize.height)) * 2.0f - 1.0f;
    *x2 = ((destArea.x + destArea.width) / float(destSize.width)) * 2.0f - 1.0f;
    *y2 = ((destSize.height - destArea.y) / float(destSize.height)) * 2.0f - 1.0f;

    *u1 = sourceArea.x / float(sourceSize.width);
    *v1 = sourceArea.y / float(sourceSize.height);
    *u2 = (sourceArea.x + sourceArea.width) / float(sourceSize.width);
    *v2 = (sourceArea.y + sourceArea.height) / float(sourceSize.height);
}

static void Write2DVertices(const gl::Box &sourceArea, const gl::Extents &sourceSize,
                            const gl::Box &destArea, const gl::Extents &destSize,
                            void *outVertices, unsigned int *outStride, unsigned int *outVertexCount,
                            D3D11_PRIMITIVE_TOPOLOGY *outTopology)
{
    float x1, y1, x2, y2, u1, v1, u2, v2;
    GenerateVertexCoords(sourceArea, sourceSize, destArea, destSize, &x1, &y1, &x2, &y2, &u1, &v1, &u2, &v2);

    d3d11::PositionTexCoordVertex *vertices = static_cast<d3d11::PositionTexCoordVertex*>(outVertices);

    d3d11::SetPositionTexCoordVertex(&vertices[0], x1, y1, u1, v2);
    d3d11::SetPositionTexCoordVertex(&vertices[1], x1, y2, u1, v1);
    d3d11::SetPositionTexCoordVertex(&vertices[2], x2, y1, u2, v2);
    d3d11::SetPositionTexCoordVertex(&vertices[3], x2, y2, u2, v1);

    *outStride = sizeof(d3d11::PositionTexCoordVertex);
    *outVertexCount = 4;
    *outTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
}

static void Write3DVertices(const gl::Box &sourceArea, const gl::Extents &sourceSize,
                            const gl::Box &destArea, const gl::Extents &destSize,
                            void *outVertices, unsigned int *outStride, unsigned int *outVertexCount,
                            D3D11_PRIMITIVE_TOPOLOGY *outTopology)
{
    ASSERT(sourceSize.depth > 0 && destSize.depth > 0);

    float x1, y1, x2, y2, u1, v1, u2, v2;
    GenerateVertexCoords(sourceArea, sourceSize, destArea, destSize, &x1, &y1, &x2, &y2, &u1, &v1, &u2, &v2);

    d3d11::PositionLayerTexCoord3DVertex *vertices = static_cast<d3d11::PositionLayerTexCoord3DVertex*>(outVertices);

    for (int i = 0; i < destSize.depth; i++)
    {
        float readDepth = (float)i / std::max(destSize.depth - 1, 1);

        d3d11::SetPositionLayerTexCoord3DVertex(&vertices[i * 6 + 0], x1, y1, i, u1, v2, readDepth);
        d3d11::SetPositionLayerTexCoord3DVertex(&vertices[i * 6 + 1], x1, y2, i, u1, v1, readDepth);
        d3d11::SetPositionLayerTexCoord3DVertex(&vertices[i * 6 + 2], x2, y1, i, u2, v2, readDepth);

        d3d11::SetPositionLayerTexCoord3DVertex(&vertices[i * 6 + 3], x1, y2, i, u1, v1, readDepth);
        d3d11::SetPositionLayerTexCoord3DVertex(&vertices[i * 6 + 4], x2, y2, i, u2, v1, readDepth);
        d3d11::SetPositionLayerTexCoord3DVertex(&vertices[i * 6 + 5], x2, y1, i, u2, v2, readDepth);
    }

    *outStride = sizeof(d3d11::PositionLayerTexCoord3DVertex);
    *outVertexCount = destSize.depth * 6;
    *outTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

Blit11::Blit11(Renderer11 *renderer)
    : mRenderer(renderer), mBlitShaderMap(compareBlitParameters), mSwizzleShaderMap(compareSwizzleParameters),
      mVertexBuffer(NULL), mPointSampler(NULL), mLinearSampler(NULL), mScissorEnabledRasterizerState(NULL),
      mScissorDisabledRasterizerState(NULL), mDepthStencilState(NULL),
      mQuad2DIL(NULL), mQuad2DVS(NULL), mDepthPS(NULL),
      mQuad3DIL(NULL), mQuad3DVS(NULL), mQuad3DGS(NULL),
      mSwizzleCB(NULL)
{
    HRESULT result;
    ID3D11Device *device = mRenderer->getDevice();

    D3D11_BUFFER_DESC vbDesc;
    vbDesc.ByteWidth = std::max(sizeof(d3d11::PositionLayerTexCoord3DVertex), sizeof(d3d11::PositionTexCoordVertex)) *
                       6 * renderer->getRendererCaps().max3DTextureSize;
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vbDesc.MiscFlags = 0;
    vbDesc.StructureByteStride = 0;

    result = device->CreateBuffer(&vbDesc, NULL, &mVertexBuffer);
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mVertexBuffer, "Blit11 vertex buffer");

    D3D11_SAMPLER_DESC pointSamplerDesc;
    pointSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
    pointSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    pointSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    pointSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    pointSamplerDesc.MipLODBias = 0.0f;
    pointSamplerDesc.MaxAnisotropy = 0;
    pointSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    pointSamplerDesc.BorderColor[0] = 0.0f;
    pointSamplerDesc.BorderColor[1] = 0.0f;
    pointSamplerDesc.BorderColor[2] = 0.0f;
    pointSamplerDesc.BorderColor[3] = 0.0f;
    pointSamplerDesc.MinLOD = 0.0f;
    pointSamplerDesc.MaxLOD = mRenderer->isLevel9() ? D3D11_FLOAT32_MAX : 0.0f;

    result = device->CreateSamplerState(&pointSamplerDesc, &mPointSampler);
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mPointSampler, "Blit11 point sampler");

    D3D11_SAMPLER_DESC linearSamplerDesc;
    linearSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    linearSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    linearSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    linearSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    linearSamplerDesc.MipLODBias = 0.0f;
    linearSamplerDesc.MaxAnisotropy = 0;
    linearSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    linearSamplerDesc.BorderColor[0] = 0.0f;
    linearSamplerDesc.BorderColor[1] = 0.0f;
    linearSamplerDesc.BorderColor[2] = 0.0f;
    linearSamplerDesc.BorderColor[3] = 0.0f;
    linearSamplerDesc.MinLOD = 0.0f;
    linearSamplerDesc.MaxLOD = mRenderer->isLevel9() ? D3D11_FLOAT32_MAX : 0.0f;

    result = device->CreateSamplerState(&linearSamplerDesc, &mLinearSampler);
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mLinearSampler, "Blit11 linear sampler");

    // Use a rasterizer state that will not cull so that inverted quads will not be culled
    D3D11_RASTERIZER_DESC rasterDesc;
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.FrontCounterClockwise = FALSE;
    rasterDesc.DepthBias = 0;
    rasterDesc.SlopeScaledDepthBias = 0.0f;
    rasterDesc.DepthBiasClamp = 0.0f;
    rasterDesc.DepthClipEnable = TRUE;
    rasterDesc.MultisampleEnable = FALSE;
    rasterDesc.AntialiasedLineEnable = FALSE;

    rasterDesc.ScissorEnable = TRUE;
    result = device->CreateRasterizerState(&rasterDesc, &mScissorEnabledRasterizerState);
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mScissorEnabledRasterizerState, "Blit11 scissoring rasterizer state");

    rasterDesc.ScissorEnable = FALSE;
    result = device->CreateRasterizerState(&rasterDesc, &mScissorDisabledRasterizerState);
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mScissorDisabledRasterizerState, "Blit11 no scissoring rasterizer state");

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

    result = device->CreateDepthStencilState(&depthStencilDesc, &mDepthStencilState);
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mDepthStencilState, "Blit11 depth stencil state");

    D3D11_INPUT_ELEMENT_DESC quad2DLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    result = device->CreateInputLayout(quad2DLayout, ArraySize(quad2DLayout), g_VS_Passthrough2D, ArraySize(g_VS_Passthrough2D), &mQuad2DIL);
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mQuad2DIL, "Blit11 2D input layout");

    result = device->CreateVertexShader(g_VS_Passthrough2D, ArraySize(g_VS_Passthrough2D), NULL, &mQuad2DVS);
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mQuad2DVS, "Blit11 2D vertex shader");

    if (!renderer->isLevel9())
    {
        result = device->CreatePixelShader(g_PS_PassthroughDepth2D, ArraySize(g_PS_PassthroughDepth2D), NULL, &mDepthPS);
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mDepthPS, "Blit11 2D depth pixel shader");

        D3D11_INPUT_ELEMENT_DESC quad3DLayout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "LAYER",    0, DXGI_FORMAT_R32_UINT,        0,  8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        result = device->CreateInputLayout(quad3DLayout, ArraySize(quad3DLayout), g_VS_Passthrough3D, ArraySize(g_VS_Passthrough3D), &mQuad3DIL);
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mQuad3DIL, "Blit11 3D input layout");

        result = device->CreateVertexShader(g_VS_Passthrough3D, ArraySize(g_VS_Passthrough3D), NULL, &mQuad3DVS);
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mQuad3DVS, "Blit11 3D vertex shader");

        result = device->CreateGeometryShader(g_GS_Passthrough3D, ArraySize(g_GS_Passthrough3D), NULL, &mQuad3DGS);
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mQuad3DGS, "Renderer11 copy 3D texture geometry shader");
    }

    buildShaderMap();

    D3D11_BUFFER_DESC swizzleBufferDesc;
    swizzleBufferDesc.ByteWidth = sizeof(unsigned int) * 4;
    swizzleBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    swizzleBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    swizzleBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    swizzleBufferDesc.MiscFlags = 0;
    swizzleBufferDesc.StructureByteStride = 0;

    result = device->CreateBuffer(&swizzleBufferDesc, NULL, &mSwizzleCB);
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mSwizzleCB, "Blit11 swizzle constant buffer");
}

Blit11::~Blit11()
{
    SafeRelease(mVertexBuffer);
    SafeRelease(mPointSampler);
    SafeRelease(mLinearSampler);
    SafeRelease(mScissorEnabledRasterizerState);
    SafeRelease(mScissorDisabledRasterizerState);
    SafeRelease(mDepthStencilState);

    SafeRelease(mQuad2DIL);
    SafeRelease(mQuad2DVS);
    SafeRelease(mDepthPS);

    SafeRelease(mQuad3DIL);
    SafeRelease(mQuad3DVS);
    SafeRelease(mQuad3DGS);

    SafeRelease(mSwizzleCB);

    clearShaderMap();
}

static inline unsigned int GetSwizzleIndex(GLenum swizzle)
{
    unsigned int colorIndex = 0;

    switch (swizzle)
    {
      case GL_RED:   colorIndex = 0; break;
      case GL_GREEN: colorIndex = 1; break;
      case GL_BLUE:  colorIndex = 2; break;
      case GL_ALPHA: colorIndex = 3; break;
      case GL_ZERO:  colorIndex = 4; break;
      case GL_ONE:   colorIndex = 5; break;
      default:       UNREACHABLE();  break;
    }

    return colorIndex;
}

gl::Error Blit11::swizzleTexture(ID3D11ShaderResourceView *source, ID3D11RenderTargetView *dest, const gl::Extents &size,
                                 GLenum swizzleRed, GLenum swizzleGreen, GLenum swizzleBlue, GLenum swizzleAlpha)
{
    HRESULT result;
    ID3D11DeviceContext *deviceContext = mRenderer->getDeviceContext();

    D3D11_SHADER_RESOURCE_VIEW_DESC sourceSRVDesc;
    source->GetDesc(&sourceSRVDesc);

    const d3d11::DXGIFormat &dxgiFormatInfo = d3d11::GetDXGIFormatInfo(sourceSRVDesc.Format);
    const gl::InternalFormat &sourceFormatInfo = gl::GetInternalFormatInfo(dxgiFormatInfo.internalFormat);

    GLenum shaderType = GL_NONE;
    switch (sourceFormatInfo.componentType)
    {
      case GL_UNSIGNED_NORMALIZED:
      case GL_SIGNED_NORMALIZED:
      case GL_FLOAT:
        shaderType = GL_FLOAT;
        break;
      case GL_INT:
        shaderType = GL_INT;
        break;
      case GL_UNSIGNED_INT:
        shaderType = GL_UNSIGNED_INT;
        break;
      default:
        UNREACHABLE();
        break;
    }

    SwizzleParameters parameters = { 0 };
    parameters.mDestinationType = shaderType;
    parameters.mViewDimension = sourceSRVDesc.ViewDimension;

    SwizzleShaderMap::const_iterator i = mSwizzleShaderMap.find(parameters);
    if (i == mSwizzleShaderMap.end())
    {
        UNREACHABLE();
        return gl::Error(GL_INVALID_OPERATION, "Internal error, missing swizzle shader.");
    }

    const Shader &shader = i->second;

    // Set vertices
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    result = deviceContext->Map(mVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(result))
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to map internal vertex buffer for swizzle, HRESULT: 0x%X.", result);
    }

    UINT stride = 0;
    UINT startIdx = 0;
    UINT drawCount = 0;
    D3D11_PRIMITIVE_TOPOLOGY topology;

    gl::Box area(0, 0, 0, size.width, size.height, size.depth);
    shader.mVertexWriteFunction(area, size, area, size, mappedResource.pData, &stride, &drawCount, &topology);

    deviceContext->Unmap(mVertexBuffer, 0);

    // Set constant buffer
    result = deviceContext->Map(mSwizzleCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(result))
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to map internal constant buffer for swizzle, HRESULT: 0x%X.", result);
    }

    unsigned int *swizzleIndices = reinterpret_cast<unsigned int*>(mappedResource.pData);
    swizzleIndices[0] = GetSwizzleIndex(swizzleRed);
    swizzleIndices[1] = GetSwizzleIndex(swizzleGreen);
    swizzleIndices[2] = GetSwizzleIndex(swizzleBlue);
    swizzleIndices[3] = GetSwizzleIndex(swizzleAlpha);

    deviceContext->Unmap(mSwizzleCB, 0);

    // Apply vertex buffer
    deviceContext->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &startIdx);

    // Apply constant buffer
    deviceContext->PSSetConstantBuffers(0, 1, &mSwizzleCB);

    // Apply state
    deviceContext->OMSetBlendState(NULL, NULL, 0xFFFFFFF);
    deviceContext->OMSetDepthStencilState(NULL, 0xFFFFFFFF);
    deviceContext->RSSetState(mScissorDisabledRasterizerState);

    // Apply shaders
    deviceContext->IASetInputLayout(shader.mInputLayout);
    deviceContext->IASetPrimitiveTopology(topology);
    deviceContext->VSSetShader(shader.mVertexShader, NULL, 0);

    deviceContext->PSSetShader(shader.mPixelShader, NULL, 0);
    deviceContext->GSSetShader(shader.mGeometryShader, NULL, 0);

    // Unset the currently bound shader resource to avoid conflicts
    mRenderer->setShaderResource(gl::SAMPLER_PIXEL, 0, NULL);

    // Apply render target
    mRenderer->setOneTimeRenderTarget(dest);

    // Set the viewport
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = size.width;
    viewport.Height = size.height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    deviceContext->RSSetViewports(1, &viewport);

    // Apply textures
    mRenderer->setShaderResource(gl::SAMPLER_PIXEL, 0, source);

    // Apply samplers
    deviceContext->PSSetSamplers(0, 1, &mPointSampler);

    // Draw the quad
    deviceContext->Draw(drawCount, 0);

    // Unbind textures and render targets and vertex buffer
    mRenderer->setShaderResource(gl::SAMPLER_PIXEL, 0, NULL);

    mRenderer->unapplyRenderTargets();

    UINT zero = 0;
    ID3D11Buffer *const nullBuffer = NULL;
    deviceContext->IASetVertexBuffers(0, 1, &nullBuffer, &zero, &zero);

    mRenderer->markAllStateDirty();

    return gl::Error(GL_NO_ERROR);
}

gl::Error Blit11::copyTexture(ID3D11ShaderResourceView *source, const gl::Box &sourceArea, const gl::Extents &sourceSize,
                              ID3D11RenderTargetView *dest, const gl::Box &destArea, const gl::Extents &destSize,
                              const gl::Rectangle *scissor, GLenum destFormat, GLenum filter)
{
    HRESULT result;
    ID3D11DeviceContext *deviceContext = mRenderer->getDeviceContext();

    // Determine if the source format is a signed integer format, the destFormat will already
    // be GL_XXXX_INTEGER but it does not tell us if it is signed or unsigned.
    D3D11_SHADER_RESOURCE_VIEW_DESC sourceSRVDesc;
    source->GetDesc(&sourceSRVDesc);

    const d3d11::DXGIFormat &dxgiFormatInfo = d3d11::GetDXGIFormatInfo(sourceSRVDesc.Format);
    const gl::InternalFormat &internalFormatInfo = gl::GetInternalFormatInfo(dxgiFormatInfo.internalFormat);

    BlitParameters parameters = { 0 };
    parameters.mDestinationFormat = destFormat;
    parameters.mSignedInteger = (internalFormatInfo.componentType == GL_INT);
    parameters.m3DBlit = sourceArea.depth > 1;

    BlitShaderMap::const_iterator i = mBlitShaderMap.find(parameters);
    if (i == mBlitShaderMap.end())
    {
        UNREACHABLE();
        return gl::Error(GL_OUT_OF_MEMORY, "Could not find appropriate shader for internal texture blit.");
    }

    const Shader& shader = i->second;

    // Set vertices
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    result = deviceContext->Map(mVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(result))
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to map internal vertex buffer for texture copy, HRESULT: 0x%X.", result);
    }

    UINT stride = 0;
    UINT startIdx = 0;
    UINT drawCount = 0;
    D3D11_PRIMITIVE_TOPOLOGY topology;

    shader.mVertexWriteFunction(sourceArea, sourceSize, destArea, destSize, mappedResource.pData,
                                &stride, &drawCount, &topology);

    deviceContext->Unmap(mVertexBuffer, 0);

    // Apply vertex buffer
    deviceContext->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &startIdx);

    // Apply state
    deviceContext->OMSetBlendState(NULL, NULL, 0xFFFFFFF);
    deviceContext->OMSetDepthStencilState(NULL, 0xFFFFFFFF);

    if (scissor)
    {
        D3D11_RECT scissorRect;
        scissorRect.left = scissor->x;
        scissorRect.right = scissor->x + scissor->width;
        scissorRect.top = scissor->y;
        scissorRect.bottom = scissor->y + scissor->height;

        deviceContext->RSSetScissorRects(1, &scissorRect);
        deviceContext->RSSetState(mScissorEnabledRasterizerState);
    }
    else
    {
        deviceContext->RSSetState(mScissorDisabledRasterizerState);
    }

    // Apply shaders
    deviceContext->IASetInputLayout(shader.mInputLayout);
    deviceContext->IASetPrimitiveTopology(topology);
    deviceContext->VSSetShader(shader.mVertexShader, NULL, 0);

    deviceContext->PSSetShader(shader.mPixelShader, NULL, 0);
    deviceContext->GSSetShader(shader.mGeometryShader, NULL, 0);

    // Unset the currently bound shader resource to avoid conflicts
    mRenderer->setShaderResource(gl::SAMPLER_PIXEL, 0, NULL);

    // Apply render target
    mRenderer->setOneTimeRenderTarget(dest);

    // Set the viewport
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = destSize.width;
    viewport.Height = destSize.height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    deviceContext->RSSetViewports(1, &viewport);

    // Apply textures
    mRenderer->setShaderResource(gl::SAMPLER_PIXEL, 0, source);

    // Apply samplers
    ID3D11SamplerState *sampler = NULL;
    switch (filter)
    {
      case GL_NEAREST: sampler = mPointSampler;  break;
      case GL_LINEAR:  sampler = mLinearSampler; break;

      default:
        UNREACHABLE();
        return gl::Error(GL_OUT_OF_MEMORY, "Internal error, unknown blit filter mode.");
    }
    deviceContext->PSSetSamplers(0, 1, &sampler);

    // Draw the quad
    deviceContext->Draw(drawCount, 0);

    // Unbind textures and render targets and vertex buffer
    mRenderer->setShaderResource(gl::SAMPLER_PIXEL, 0, NULL);

    mRenderer->unapplyRenderTargets();

    UINT zero = 0;
    ID3D11Buffer *const nullBuffer = NULL;
    deviceContext->IASetVertexBuffers(0, 1, &nullBuffer, &zero, &zero);

    mRenderer->markAllStateDirty();

    return gl::Error(GL_NO_ERROR);
}

gl::Error Blit11::copyStencil(ID3D11Resource *source, unsigned int sourceSubresource, const gl::Box &sourceArea, const gl::Extents &sourceSize,
                              ID3D11Resource *dest, unsigned int destSubresource, const gl::Box &destArea, const gl::Extents &destSize,
                              const gl::Rectangle *scissor)
{
    return copyDepthStencil(source, sourceSubresource, sourceArea, sourceSize,
                            dest, destSubresource, destArea, destSize,
                            scissor, true);
}

gl::Error Blit11::copyDepth(ID3D11ShaderResourceView *source, const gl::Box &sourceArea, const gl::Extents &sourceSize,
                            ID3D11DepthStencilView *dest, const gl::Box &destArea, const gl::Extents &destSize,
                            const gl::Rectangle *scissor)
{
    HRESULT result;
    ID3D11DeviceContext *deviceContext = mRenderer->getDeviceContext();

    // Set vertices
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    result = deviceContext->Map(mVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(result))
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to map internal vertex buffer for texture copy, HRESULT: 0x%X.", result);
    }

    UINT stride = 0;
    UINT startIdx = 0;
    UINT drawCount = 0;
    D3D11_PRIMITIVE_TOPOLOGY topology;

    Write2DVertices(sourceArea, sourceSize, destArea, destSize, mappedResource.pData,
                    &stride, &drawCount, &topology);

    deviceContext->Unmap(mVertexBuffer, 0);

    // Apply vertex buffer
    deviceContext->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &startIdx);

    // Apply state
    deviceContext->OMSetBlendState(NULL, NULL, 0xFFFFFFF);
    deviceContext->OMSetDepthStencilState(mDepthStencilState, 0xFFFFFFFF);

    if (scissor)
    {
        D3D11_RECT scissorRect;
        scissorRect.left = scissor->x;
        scissorRect.right = scissor->x + scissor->width;
        scissorRect.top = scissor->y;
        scissorRect.bottom = scissor->y + scissor->height;

        deviceContext->RSSetScissorRects(1, &scissorRect);
        deviceContext->RSSetState(mScissorEnabledRasterizerState);
    }
    else
    {
        deviceContext->RSSetState(mScissorDisabledRasterizerState);
    }

    // Apply shaders
    deviceContext->IASetInputLayout(mQuad2DIL);
    deviceContext->IASetPrimitiveTopology(topology);
    deviceContext->VSSetShader(mQuad2DVS, NULL, 0);

    deviceContext->PSSetShader(mDepthPS, NULL, 0);
    deviceContext->GSSetShader(NULL, NULL, 0);

    // Unset the currently bound shader resource to avoid conflicts
    mRenderer->setShaderResource(gl::SAMPLER_PIXEL, 0, NULL);

    // Apply render target
    deviceContext->OMSetRenderTargets(0, NULL, dest);

    // Set the viewport
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = destSize.width;
    viewport.Height = destSize.height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    deviceContext->RSSetViewports(1, &viewport);

    // Apply textures
    mRenderer->setShaderResource(gl::SAMPLER_PIXEL, 0, source);

    // Apply samplers
    deviceContext->PSSetSamplers(0, 1, &mPointSampler);

    // Draw the quad
    deviceContext->Draw(drawCount, 0);

    // Unbind textures and render targets and vertex buffer
    mRenderer->setShaderResource(gl::SAMPLER_PIXEL, 0, NULL);

    mRenderer->unapplyRenderTargets();

    UINT zero = 0;
    ID3D11Buffer *const nullBuffer = NULL;
    deviceContext->IASetVertexBuffers(0, 1, &nullBuffer, &zero, &zero);

    mRenderer->markAllStateDirty();

    return gl::Error(GL_NO_ERROR);
}

gl::Error Blit11::copyDepthStencil(ID3D11Resource *source, unsigned int sourceSubresource, const gl::Box &sourceArea, const gl::Extents &sourceSize,
                                   ID3D11Resource *dest, unsigned int destSubresource, const gl::Box &destArea, const gl::Extents &destSize,
                                   const gl::Rectangle *scissor)
{
    return copyDepthStencil(source, sourceSubresource, sourceArea, sourceSize,
                            dest, destSubresource, destArea, destSize,
                            scissor, false);
}

gl::Error Blit11::copyDepthStencil(ID3D11Resource *source, unsigned int sourceSubresource, const gl::Box &sourceArea, const gl::Extents &sourceSize,
                                   ID3D11Resource *dest, unsigned int destSubresource, const gl::Box &destArea, const gl::Extents &destSize,
                                   const gl::Rectangle *scissor, bool stencilOnly)
{
    ID3D11Device *device = mRenderer->getDevice();
    ID3D11DeviceContext *deviceContext = mRenderer->getDeviceContext();

    ID3D11Resource *sourceStaging = CreateStagingTexture(device, deviceContext, source, sourceSubresource, sourceSize, D3D11_CPU_ACCESS_READ);
    // HACK: Create the destination staging buffer as a read/write texture so ID3D11DevicContext::UpdateSubresource can be called
    //       using it's mapped data as a source
    ID3D11Resource *destStaging = CreateStagingTexture(device, deviceContext, dest, destSubresource, destSize, D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE);

    if (!sourceStaging || !destStaging)
    {
        SafeRelease(sourceStaging);
        SafeRelease(destStaging);
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal staging textures for depth stencil blit.");
    }

    DXGI_FORMAT format = GetTextureFormat(source);
    ASSERT(format == GetTextureFormat(dest));

    const d3d11::DXGIFormat &dxgiFormatInfo = d3d11::GetDXGIFormatInfo(format);
    unsigned int pixelSize = dxgiFormatInfo.pixelBytes;
    unsigned int copyOffset = 0;
    unsigned int copySize = pixelSize;
    if (stencilOnly)
    {
        copyOffset = dxgiFormatInfo.depthBits / 8;
        copySize = dxgiFormatInfo.stencilBits / 8;

        // It would be expensive to have non-byte sized stencil sizes since it would
        // require reading from the destination, currently there aren't any though.
        ASSERT(dxgiFormatInfo.stencilBits % 8 == 0 &&
               dxgiFormatInfo.depthBits   % 8 == 0);
    }

    D3D11_MAPPED_SUBRESOURCE sourceMapping;
    HRESULT result = deviceContext->Map(sourceStaging, 0, D3D11_MAP_READ, 0, &sourceMapping);
    if (FAILED(result))
    {
        SafeRelease(sourceStaging);
        SafeRelease(destStaging);
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to map internal source staging texture for depth stencil blit, HRESULT: 0x%X.", result);
    }

    D3D11_MAPPED_SUBRESOURCE destMapping;
    result = deviceContext->Map(destStaging, 0, D3D11_MAP_WRITE, 0, &destMapping);
    if (FAILED(result))
    {
        deviceContext->Unmap(sourceStaging, 0);
        SafeRelease(sourceStaging);
        SafeRelease(destStaging);
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to map internal destination staging texture for depth stencil blit, HRESULT: 0x%X.", result);
    }

    gl::Rectangle clippedDestArea(destArea.x, destArea.y, destArea.width, destArea.height);

    // Clip dest area to the destination size
    gl::ClipRectangle(clippedDestArea, gl::Rectangle(0, 0, destSize.width, destSize.height), &clippedDestArea);

    // Clip dest area to the scissor
    if (scissor)
    {
        gl::ClipRectangle(clippedDestArea, *scissor, &clippedDestArea);
    }

    // Determine if entire rows can be copied at once instead of each individual pixel, requires that there is
    // no out of bounds lookups required, the entire pixel is copied and no stretching
    bool wholeRowCopy = sourceArea.width == clippedDestArea.width &&
                        sourceArea.x >= 0 && sourceArea.x + sourceArea.width <= sourceSize.width &&
                        copySize == pixelSize;

    for (int y = clippedDestArea.y; y < clippedDestArea.y + clippedDestArea.height; y++)
    {
        float yPerc = static_cast<float>(y - destArea.y) / (destArea.height - 1);

        // Interpolate using the original source rectangle to determine which row to sample from while clamping to the edges
        unsigned int readRow = gl::clamp(sourceArea.y + floor(yPerc * (sourceArea.height - 1) + 0.5f), 0, sourceSize.height - 1);
        unsigned int writeRow = y;

        if (wholeRowCopy)
        {
            void *sourceRow = reinterpret_cast<char*>(sourceMapping.pData) +
                              readRow * sourceMapping.RowPitch +
                              sourceArea.x * pixelSize;

            void *destRow = reinterpret_cast<char*>(destMapping.pData) +
                            writeRow * destMapping.RowPitch +
                            destArea.x * pixelSize;

            memcpy(destRow, sourceRow, pixelSize * destArea.width);
        }
        else
        {
            for (int x = clippedDestArea.x; x < clippedDestArea.x + clippedDestArea.width; x++)
            {
                float xPerc = static_cast<float>(x - destArea.x) / (destArea.width - 1);

                // Interpolate the original source rectangle to determine which column to sample from while clamping to the edges
                unsigned int readColumn = gl::clamp(sourceArea.x + floor(xPerc * (sourceArea.width - 1) + 0.5f), 0, sourceSize.width - 1);
                unsigned int writeColumn = x;

                void *sourcePixel = reinterpret_cast<char*>(sourceMapping.pData) +
                                    readRow * sourceMapping.RowPitch +
                                    readColumn * pixelSize +
                                    copyOffset;

                void *destPixel = reinterpret_cast<char*>(destMapping.pData) +
                                  writeRow * destMapping.RowPitch +
                                  writeColumn * pixelSize +
                                  copyOffset;

                memcpy(destPixel, sourcePixel, copySize);
            }
        }
    }

    // HACK: Use ID3D11DevicContext::UpdateSubresource which causes an extra copy compared to ID3D11DevicContext::CopySubresourceRegion
    //       according to MSDN.
    deviceContext->UpdateSubresource(dest, destSubresource, NULL, destMapping.pData, destMapping.RowPitch, destMapping.DepthPitch);

    deviceContext->Unmap(sourceStaging, 0);
    deviceContext->Unmap(destStaging, 0);

    // TODO: Determine why this call to ID3D11DevicContext::CopySubresourceRegion causes a TDR timeout on some
    //       systems when called repeatedly.
    // deviceContext->CopySubresourceRegion(dest, destSubresource, 0, 0, 0, destStaging, 0, NULL);

    SafeRelease(sourceStaging);
    SafeRelease(destStaging);

    return gl::Error(GL_NO_ERROR);
}

bool Blit11::compareBlitParameters(const Blit11::BlitParameters &a, const Blit11::BlitParameters &b)
{
    return memcmp(&a, &b, sizeof(Blit11::BlitParameters)) < 0;
}

bool Blit11::compareSwizzleParameters(const SwizzleParameters &a, const SwizzleParameters &b)
{
    return memcmp(&a, &b, sizeof(Blit11::SwizzleParameters)) < 0;
}

void Blit11::add2DBlitShaderToMap(GLenum destFormat, bool signedInteger, ID3D11PixelShader *ps)
{
    BlitParameters params = { 0 };
    params.mDestinationFormat = destFormat;
    params.mSignedInteger = signedInteger;
    params.m3DBlit = false;

    ASSERT(mBlitShaderMap.find(params) == mBlitShaderMap.end());
    ASSERT(ps);

    Shader shader;
    shader.mVertexWriteFunction = Write2DVertices;
    shader.mInputLayout = mQuad2DIL;
    shader.mVertexShader = mQuad2DVS;
    shader.mGeometryShader = NULL;
    shader.mPixelShader = ps;

    mBlitShaderMap[params] = shader;
}

void Blit11::add3DBlitShaderToMap(GLenum destFormat, bool signedInteger, ID3D11PixelShader *ps)
{
    BlitParameters params = { 0 };
    params.mDestinationFormat = destFormat;
    params.mSignedInteger = signedInteger;
    params.m3DBlit = true;

    ASSERT(mBlitShaderMap.find(params) == mBlitShaderMap.end());
    ASSERT(ps);

    Shader shader;
    shader.mVertexWriteFunction = Write3DVertices;
    shader.mInputLayout = mQuad3DIL;
    shader.mVertexShader = mQuad3DVS;
    shader.mGeometryShader = mQuad3DGS;
    shader.mPixelShader = ps;

    mBlitShaderMap[params] = shader;
}

void Blit11::addSwizzleShaderToMap(GLenum destType, D3D11_SRV_DIMENSION viewDimension, ID3D11PixelShader *ps)
{
    SwizzleParameters params = { 0 };
    params.mDestinationType = destType;
    params.mViewDimension = viewDimension;

    ASSERT(mSwizzleShaderMap.find(params) == mSwizzleShaderMap.end());
    ASSERT(ps);

    Shader shader;
    switch (viewDimension)
    {
      case D3D_SRV_DIMENSION_TEXTURE2D:
        shader.mVertexWriteFunction = Write2DVertices;
        shader.mInputLayout = mQuad2DIL;
        shader.mVertexShader = mQuad2DVS;
        shader.mGeometryShader = NULL;
        break;

      case D3D_SRV_DIMENSION_TEXTURE3D:
      case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
      case D3D_SRV_DIMENSION_TEXTURECUBE:
        shader.mVertexWriteFunction = Write3DVertices;
        shader.mInputLayout = mQuad3DIL;
        shader.mVertexShader = mQuad3DVS;
        shader.mGeometryShader = mQuad3DGS;
        break;

      default:
        UNREACHABLE();
        break;
    }
    shader.mPixelShader = ps;

    mSwizzleShaderMap[params] = shader;
}

void Blit11::buildShaderMap()
{
    ID3D11Device *device = mRenderer->getDevice();

    add2DBlitShaderToMap(GL_RGBA,            false, d3d11::CompilePS(device, g_PS_PassthroughRGBA2D,     "Blit11 2D RGBA pixel shader"           ));
    add2DBlitShaderToMap(GL_BGRA_EXT,        false, d3d11::CompilePS(device, g_PS_PassthroughRGBA2D,     "Blit11 2D BGRA pixel shader"           ));
    add2DBlitShaderToMap(GL_RGB,             false, d3d11::CompilePS(device, g_PS_PassthroughRGB2D,      "Blit11 2D RGB pixel shader"            ));
    add2DBlitShaderToMap(GL_RG,              false, d3d11::CompilePS(device, g_PS_PassthroughRG2D,       "Blit11 2D RG pixel shader"             ));
    add2DBlitShaderToMap(GL_RED,             false, d3d11::CompilePS(device, g_PS_PassthroughR2D,        "Blit11 2D R pixel shader"              ));
    add2DBlitShaderToMap(GL_ALPHA,           false, d3d11::CompilePS(device, g_PS_PassthroughRGBA2D,     "Blit11 2D alpha pixel shader"          ));
    add2DBlitShaderToMap(GL_LUMINANCE,       false, d3d11::CompilePS(device, g_PS_PassthroughLum2D,      "Blit11 2D lum pixel shader"            ));
    add2DBlitShaderToMap(GL_LUMINANCE_ALPHA, false, d3d11::CompilePS(device, g_PS_PassthroughLumAlpha2D, "Blit11 2D luminance alpha pixel shader"));

    addSwizzleShaderToMap(GL_FLOAT,        D3D_SRV_DIMENSION_TEXTURE2D,      d3d11::CompilePS(device, g_PS_SwizzleF2D,       "Blit11 2D F swizzle pixel shader" ));

    if (mRenderer->isLevel9())
        return;

    add2DBlitShaderToMap(GL_RGBA_INTEGER,    false, d3d11::CompilePS(device, g_PS_PassthroughRGBA2DUI,   "Blit11 2D RGBA UI pixel shader"        ));
    add2DBlitShaderToMap(GL_RGBA_INTEGER,    true,  d3d11::CompilePS(device, g_PS_PassthroughRGBA2DI,    "Blit11 2D RGBA I pixel shader"         ));
    add2DBlitShaderToMap(GL_RGB_INTEGER,     false, d3d11::CompilePS(device, g_PS_PassthroughRGB2DUI,    "Blit11 2D RGB UI pixel shader"         ));
    add2DBlitShaderToMap(GL_RGB_INTEGER,     true,  d3d11::CompilePS(device, g_PS_PassthroughRGB2DI,     "Blit11 2D RGB I pixel shader"          ));
    add2DBlitShaderToMap(GL_RG_INTEGER,      false, d3d11::CompilePS(device, g_PS_PassthroughRG2DUI,     "Blit11 2D RG UI pixel shader"          ));
    add2DBlitShaderToMap(GL_RG_INTEGER,      true,  d3d11::CompilePS(device, g_PS_PassthroughRG2DI,      "Blit11 2D RG I pixel shader"           ));
    add2DBlitShaderToMap(GL_RED_INTEGER,     false, d3d11::CompilePS(device, g_PS_PassthroughR2DUI,      "Blit11 2D R UI pixel shader"           ));
    add2DBlitShaderToMap(GL_RED_INTEGER,     true,  d3d11::CompilePS(device, g_PS_PassthroughR2DI,       "Blit11 2D R I pixel shader"            ));
    add3DBlitShaderToMap(GL_RGBA,            false, d3d11::CompilePS(device, g_PS_PassthroughRGBA3D,     "Blit11 3D RGBA pixel shader"           ));
    add3DBlitShaderToMap(GL_RGBA_INTEGER,    false, d3d11::CompilePS(device, g_PS_PassthroughRGBA3DUI,   "Blit11 3D UI RGBA pixel shader"        ));
    add3DBlitShaderToMap(GL_RGBA_INTEGER,    true,  d3d11::CompilePS(device, g_PS_PassthroughRGBA3DI,    "Blit11 3D I RGBA pixel shader"         ));
    add3DBlitShaderToMap(GL_BGRA_EXT,        false, d3d11::CompilePS(device, g_PS_PassthroughRGBA3D,     "Blit11 3D BGRA pixel shader"           ));
    add3DBlitShaderToMap(GL_RGB,             false, d3d11::CompilePS(device, g_PS_PassthroughRGB3D,      "Blit11 3D RGB pixel shader"            ));
    add3DBlitShaderToMap(GL_RGB_INTEGER,     false, d3d11::CompilePS(device, g_PS_PassthroughRGB3DUI,    "Blit11 3D RGB UI pixel shader"         ));
    add3DBlitShaderToMap(GL_RGB_INTEGER,     true,  d3d11::CompilePS(device, g_PS_PassthroughRGB3DI,     "Blit11 3D RGB I pixel shader"          ));
    add3DBlitShaderToMap(GL_RG,              false, d3d11::CompilePS(device, g_PS_PassthroughRG3D,       "Blit11 3D RG pixel shader"             ));
    add3DBlitShaderToMap(GL_RG_INTEGER,      false, d3d11::CompilePS(device, g_PS_PassthroughRG3DUI,     "Blit11 3D RG UI pixel shader"          ));
    add3DBlitShaderToMap(GL_RG_INTEGER,      true,  d3d11::CompilePS(device, g_PS_PassthroughRG3DI,      "Blit11 3D RG I pixel shader"           ));
    add3DBlitShaderToMap(GL_RED,             false, d3d11::CompilePS(device, g_PS_PassthroughR3D,        "Blit11 3D R pixel shader"              ));
    add3DBlitShaderToMap(GL_RED_INTEGER,     false, d3d11::CompilePS(device, g_PS_PassthroughR3DUI,      "Blit11 3D R UI pixel shader"           ));
    add3DBlitShaderToMap(GL_RED_INTEGER,     true,  d3d11::CompilePS(device, g_PS_PassthroughR3DI,       "Blit11 3D R I pixel shader"            ));
    add3DBlitShaderToMap(GL_ALPHA,           false, d3d11::CompilePS(device, g_PS_PassthroughRGBA3D,     "Blit11 3D alpha pixel shader"          ));
    add3DBlitShaderToMap(GL_LUMINANCE,       false, d3d11::CompilePS(device, g_PS_PassthroughLum3D,      "Blit11 3D luminance pixel shader"      ));
    add3DBlitShaderToMap(GL_LUMINANCE_ALPHA, false, d3d11::CompilePS(device, g_PS_PassthroughLumAlpha3D, "Blit11 3D luminance alpha pixel shader"));

    addSwizzleShaderToMap(GL_UNSIGNED_INT, D3D_SRV_DIMENSION_TEXTURE2D,      d3d11::CompilePS(device, g_PS_SwizzleUI2D,      "Blit11 2D UI swizzle pixel shader"));
    addSwizzleShaderToMap(GL_INT,          D3D_SRV_DIMENSION_TEXTURE2D,      d3d11::CompilePS(device, g_PS_SwizzleI2D,       "Blit11 2D I swizzle pixel shader" ));

    addSwizzleShaderToMap(GL_FLOAT,        D3D_SRV_DIMENSION_TEXTURECUBE,    d3d11::CompilePS(device, g_PS_SwizzleF2DArray,  "Blit11 2D Cube F swizzle pixel shader" ));
    addSwizzleShaderToMap(GL_UNSIGNED_INT, D3D_SRV_DIMENSION_TEXTURECUBE,    d3d11::CompilePS(device, g_PS_SwizzleUI2DArray, "Blit11 2D Cube UI swizzle pixel shader"));
    addSwizzleShaderToMap(GL_INT,          D3D_SRV_DIMENSION_TEXTURECUBE,    d3d11::CompilePS(device, g_PS_SwizzleI2DArray,  "Blit11 2D Cube I swizzle pixel shader" ));

    addSwizzleShaderToMap(GL_FLOAT,        D3D_SRV_DIMENSION_TEXTURE3D,      d3d11::CompilePS(device, g_PS_SwizzleF3D,       "Blit11 3D F swizzle pixel shader" ));
    addSwizzleShaderToMap(GL_UNSIGNED_INT, D3D_SRV_DIMENSION_TEXTURE3D,      d3d11::CompilePS(device, g_PS_SwizzleUI3D,      "Blit11 3D UI swizzle pixel shader"));
    addSwizzleShaderToMap(GL_INT,          D3D_SRV_DIMENSION_TEXTURE3D,      d3d11::CompilePS(device, g_PS_SwizzleI3D,       "Blit11 3D I swizzle pixel shader" ));

    addSwizzleShaderToMap(GL_FLOAT,        D3D_SRV_DIMENSION_TEXTURE2DARRAY, d3d11::CompilePS(device, g_PS_SwizzleF2DArray,  "Blit11 2D Array F swizzle pixel shader" ));
    addSwizzleShaderToMap(GL_UNSIGNED_INT, D3D_SRV_DIMENSION_TEXTURE2DARRAY, d3d11::CompilePS(device, g_PS_SwizzleUI2DArray, "Blit11 2D Array UI swizzle pixel shader"));
    addSwizzleShaderToMap(GL_INT,          D3D_SRV_DIMENSION_TEXTURE2DARRAY, d3d11::CompilePS(device, g_PS_SwizzleI2DArray,  "Blit11 2D Array I swizzle pixel shader" ));
}

void Blit11::clearShaderMap()
{
    for (BlitShaderMap::iterator i = mBlitShaderMap.begin(); i != mBlitShaderMap.end(); ++i)
    {
        Shader &shader = i->second;
        SafeRelease(shader.mPixelShader);
    }
    mBlitShaderMap.clear();

    for (SwizzleShaderMap::iterator i = mSwizzleShaderMap.begin(); i != mSwizzleShaderMap.end(); ++i)
    {
        Shader &shader = i->second;
        SafeRelease(shader.mPixelShader);
    }
    mSwizzleShaderMap.clear();
}

}
