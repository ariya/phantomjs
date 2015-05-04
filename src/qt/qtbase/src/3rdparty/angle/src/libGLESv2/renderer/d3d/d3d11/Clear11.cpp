//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Clear11.cpp: Framebuffer clear utility class.

#include "libGLESv2/renderer/d3d/d3d11/Clear11.h"
#include "libGLESv2/renderer/d3d/d3d11/Renderer11.h"
#include "libGLESv2/renderer/d3d/d3d11/renderer11_utils.h"
#include "libGLESv2/renderer/d3d/d3d11/RenderTarget11.h"
#include "libGLESv2/formatutils.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/FramebufferAttachment.h"

// Precompiled shaders
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/clearfloatvs.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/clearfloatps.h"

#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/clearuintvs.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/clearuintps.h"

#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/clearsintvs.h"
#include "libGLESv2/renderer/d3d/d3d11/shaders/compiled/clearsintps.h"

namespace rx
{

template <typename T>
static void ApplyVertices(const gl::Extents &framebufferSize, const gl::Rectangle *scissor, const gl::Color<T> &color, float depth, void *buffer)
{
    d3d11::PositionDepthColorVertex<T> *vertices = reinterpret_cast<d3d11::PositionDepthColorVertex<T>*>(buffer);

    float depthClear = gl::clamp01(depth);
    float left = -1.0f;
    float right = 1.0f;
    float top = -1.0f;
    float bottom = 1.0f;

    // Clip the quad coordinates to the scissor if needed
    if (scissor != NULL)
    {
        left = std::max(left, (scissor->x / float(framebufferSize.width)) * 2.0f - 1.0f);
        right = std::min(right, ((scissor->x + scissor->width) / float(framebufferSize.width)) * 2.0f - 1.0f);
        top = std::max(top, ((framebufferSize.height - scissor->y - scissor->height) / float(framebufferSize.height)) * 2.0f - 1.0f);
        bottom = std::min(bottom, ((framebufferSize.height - scissor->y) / float(framebufferSize.height)) * 2.0f - 1.0f);
    }

    d3d11::SetPositionDepthColorVertex<T>(vertices + 0, left,  bottom, depthClear, color);
    d3d11::SetPositionDepthColorVertex<T>(vertices + 1, left,  top,    depthClear, color);
    d3d11::SetPositionDepthColorVertex<T>(vertices + 2, right, bottom, depthClear, color);
    d3d11::SetPositionDepthColorVertex<T>(vertices + 3, right, top,    depthClear, color);
}

template <unsigned int vsSize, unsigned int psSize>
Clear11::ClearShader Clear11::CreateClearShader(ID3D11Device *device, DXGI_FORMAT colorType, const BYTE (&vsByteCode)[vsSize], const BYTE (&psByteCode)[psSize])
{
    HRESULT result;

    ClearShader shader = { 0 };

    D3D11_INPUT_ELEMENT_DESC quadLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, colorType,                   0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    result = device->CreateInputLayout(quadLayout, ArraySize(quadLayout), vsByteCode, vsSize, &shader.inputLayout);
    ASSERT(SUCCEEDED(result));

    result = device->CreateVertexShader(vsByteCode, vsSize, NULL, &shader.vertexShader);
    ASSERT(SUCCEEDED(result));

    result = device->CreatePixelShader(psByteCode, psSize, NULL, &shader.pixelShader);
    ASSERT(SUCCEEDED(result));

    return shader;
}

Clear11::Clear11(Renderer11 *renderer)
    : mRenderer(renderer), mClearBlendStates(StructLessThan<ClearBlendInfo>), mClearDepthStencilStates(StructLessThan<ClearDepthStencilInfo>),
      mVertexBuffer(NULL), mRasterizerState(NULL)
{
    HRESULT result;
    ID3D11Device *device = renderer->getDevice();

    D3D11_BUFFER_DESC vbDesc;
    vbDesc.ByteWidth = sizeof(d3d11::PositionDepthColorVertex<float>) * 4;
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vbDesc.MiscFlags = 0;
    vbDesc.StructureByteStride = 0;

    result = device->CreateBuffer(&vbDesc, NULL, &mVertexBuffer);
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mVertexBuffer, "Clear11 masked clear vertex buffer");

    D3D11_RASTERIZER_DESC rsDesc;
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthBias = 0;
    rsDesc.DepthBiasClamp = 0.0f;
    rsDesc.SlopeScaledDepthBias = 0.0f;
    rsDesc.DepthClipEnable = renderer->isLevel9();
    rsDesc.ScissorEnable = FALSE;
    rsDesc.MultisampleEnable = FALSE;
    rsDesc.AntialiasedLineEnable = FALSE;

    result = device->CreateRasterizerState(&rsDesc, &mRasterizerState);
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mRasterizerState, "Clear11 masked clear rasterizer state");

    mFloatClearShader = CreateClearShader(device, DXGI_FORMAT_R32G32B32A32_FLOAT, g_VS_ClearFloat, g_PS_ClearFloat);
    if (mRenderer->isLevel9()) {
        memset(&mUintClearShader, 0, sizeof(ClearShader));
        memset(&mIntClearShader, 0, sizeof(ClearShader));
        return;
    }
    mUintClearShader  = CreateClearShader(device, DXGI_FORMAT_R32G32B32A32_UINT,  g_VS_ClearUint,  g_PS_ClearUint );
    mIntClearShader   = CreateClearShader(device, DXGI_FORMAT_R32G32B32A32_SINT,  g_VS_ClearSint,  g_PS_ClearSint );
}

Clear11::~Clear11()
{
    for (ClearBlendStateMap::iterator i = mClearBlendStates.begin(); i != mClearBlendStates.end(); i++)
    {
        SafeRelease(i->second);
    }
    mClearBlendStates.clear();

    SafeRelease(mFloatClearShader.inputLayout);
    SafeRelease(mFloatClearShader.vertexShader);
    SafeRelease(mFloatClearShader.pixelShader);

    SafeRelease(mUintClearShader.inputLayout);
    SafeRelease(mUintClearShader.vertexShader);
    SafeRelease(mUintClearShader.pixelShader);

    SafeRelease(mIntClearShader.inputLayout);
    SafeRelease(mIntClearShader.vertexShader);
    SafeRelease(mIntClearShader.pixelShader);

    for (ClearDepthStencilStateMap::iterator i = mClearDepthStencilStates.begin(); i != mClearDepthStencilStates.end(); i++)
    {
        SafeRelease(i->second);
    }
    mClearDepthStencilStates.clear();

    SafeRelease(mVertexBuffer);
    SafeRelease(mRasterizerState);
}

gl::Error Clear11::clearFramebuffer(const gl::ClearParameters &clearParams, const gl::Framebuffer *frameBuffer)
{
    // First determine if a scissored clear is needed, this will always require drawing a quad.
    //
    // Otherwise, iterate over the color buffers which require clearing and determine if they can be
    // cleared with ID3D11DeviceContext::ClearRenderTargetView... This requires:
    // 1) The render target is being cleared to a float value (will be cast to integer when clearing integer
    //    render targets as expected but does not work the other way around)
    // 2) The format of the render target has no color channels that are currently masked out.
    // Clear the easy-to-clear buffers on the spot and accumulate the ones that require special work.
    //
    // Also determine if the depth stencil can be cleared with ID3D11DeviceContext::ClearDepthStencilView
    // by checking if the stencil write mask covers the entire stencil.
    //
    // To clear the remaining buffers, quads must be drawn containing an int, uint or float vertex color
    // attribute.

    gl::Extents framebufferSize;
    if (frameBuffer->getFirstColorbuffer() != NULL)
    {
        gl::FramebufferAttachment *attachment = frameBuffer->getFirstColorbuffer();
        framebufferSize.width = attachment->getWidth();
        framebufferSize.height = attachment->getHeight();
        framebufferSize.depth = 1;
    }
    else if (frameBuffer->getDepthOrStencilbuffer() != NULL)
    {
        gl::FramebufferAttachment *attachment = frameBuffer->getDepthOrStencilbuffer();
        framebufferSize.width = attachment->getWidth();
        framebufferSize.height = attachment->getHeight();
        framebufferSize.depth = 1;
    }
    else
    {
        UNREACHABLE();
        return gl::Error(GL_INVALID_OPERATION);
    }

    if (clearParams.scissorEnabled && (clearParams.scissor.x >= framebufferSize.width || 
                                       clearParams.scissor.y >= framebufferSize.height ||
                                       clearParams.scissor.x + clearParams.scissor.width <= 0 ||
                                       clearParams.scissor.y + clearParams.scissor.height <= 0))
    {
        // Scissor is enabled and the scissor rectangle is outside the renderbuffer
        return gl::Error(GL_NO_ERROR);
    }

    bool needScissoredClear = clearParams.scissorEnabled && (clearParams.scissor.x > 0 || clearParams.scissor.y > 0 ||
                                                             clearParams.scissor.x + clearParams.scissor.width < framebufferSize.width ||
                                                             clearParams.scissor.y + clearParams.scissor.height < framebufferSize.height);

    std::vector<MaskedRenderTarget> maskedClearRenderTargets;
    RenderTarget11* maskedClearDepthStencil = NULL;

    ID3D11DeviceContext *deviceContext = mRenderer->getDeviceContext();

    for (unsigned int colorAttachment = 0; colorAttachment < gl::IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        if (clearParams.clearColor[colorAttachment] && frameBuffer->isEnabledColorAttachment(colorAttachment))
        {
            gl::FramebufferAttachment *attachment = frameBuffer->getColorbuffer(colorAttachment);
            if (attachment)
            {
                RenderTarget11 *renderTarget = NULL;
                gl::Error error = d3d11::GetAttachmentRenderTarget(attachment, &renderTarget);
                if (error.isError())
                {
                    return error;
                }

                const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(attachment->getInternalFormat());

                if (clearParams.colorClearType == GL_FLOAT &&
                    !(formatInfo.componentType == GL_FLOAT || formatInfo.componentType == GL_UNSIGNED_NORMALIZED || formatInfo.componentType == GL_SIGNED_NORMALIZED))
                {
                    ERR("It is undefined behaviour to clear a render buffer which is not normalized fixed point or floating-"
                        "point to floating point values (color attachment %u has internal format 0x%X).", colorAttachment,
                        attachment->getInternalFormat());
                }

                if ((formatInfo.redBits == 0 || !clearParams.colorMaskRed) &&
                    (formatInfo.greenBits == 0 || !clearParams.colorMaskGreen) &&
                    (formatInfo.blueBits == 0 || !clearParams.colorMaskBlue) &&
                    (formatInfo.alphaBits == 0 || !clearParams.colorMaskAlpha))
                {
                    // Every channel either does not exist in the render target or is masked out
                    continue;
                }
                else if (needScissoredClear || clearParams.colorClearType != GL_FLOAT ||
                         (formatInfo.redBits   > 0 && !clearParams.colorMaskRed)   ||
                         (formatInfo.greenBits > 0 && !clearParams.colorMaskGreen) ||
                         (formatInfo.blueBits  > 0 && !clearParams.colorMaskBlue) ||
                         (formatInfo.alphaBits > 0 && !clearParams.colorMaskAlpha))
                {
                    // A scissored or masked clear is required
                    MaskedRenderTarget maskAndRt;
                    bool clearColor = clearParams.clearColor[colorAttachment];
                    maskAndRt.colorMask[0] = (clearColor && clearParams.colorMaskRed);
                    maskAndRt.colorMask[1] = (clearColor && clearParams.colorMaskGreen);
                    maskAndRt.colorMask[2] = (clearColor && clearParams.colorMaskBlue);
                    maskAndRt.colorMask[3] = (clearColor && clearParams.colorMaskAlpha);
                    maskAndRt.renderTarget = renderTarget;
                    maskedClearRenderTargets.push_back(maskAndRt);
                }
                else
                {
                    // ID3D11DeviceContext::ClearRenderTargetView is possible

                    ID3D11RenderTargetView *framebufferRTV = renderTarget->getRenderTargetView();
                    if (!framebufferRTV)
                    {
                        return gl::Error(GL_OUT_OF_MEMORY, "Internal render target view pointer unexpectedly null.");
                    }

                    const gl::InternalFormat &actualFormatInfo = gl::GetInternalFormatInfo(attachment->getActualFormat());

                    // Check if the actual format has a channel that the internal format does not and set them to the
                    // default values
                    const float clearValues[4] =
                    {
                        ((formatInfo.redBits == 0 && actualFormatInfo.redBits   > 0) ? 0.0f : clearParams.colorFClearValue.red),
                        ((formatInfo.greenBits == 0 && actualFormatInfo.greenBits > 0) ? 0.0f : clearParams.colorFClearValue.green),
                        ((formatInfo.blueBits == 0 && actualFormatInfo.blueBits  > 0) ? 0.0f : clearParams.colorFClearValue.blue),
                        ((formatInfo.alphaBits == 0 && actualFormatInfo.alphaBits > 0) ? 1.0f : clearParams.colorFClearValue.alpha),
                    };

                    deviceContext->ClearRenderTargetView(framebufferRTV, clearValues);
                }
            }
        }
    }

    if (clearParams.clearDepth || clearParams.clearStencil)
    {
        gl::FramebufferAttachment *attachment = frameBuffer->getDepthOrStencilbuffer();
        if (attachment)
        {
            RenderTarget11 *renderTarget = NULL;
            gl::Error error = d3d11::GetAttachmentRenderTarget(attachment, &renderTarget);
            if (error.isError())
            {
                return error;
            }

            const gl::InternalFormat &actualFormatInfo = gl::GetInternalFormatInfo(attachment->getActualFormat());

            unsigned int stencilUnmasked = frameBuffer->hasStencil() ? (1 << actualFormatInfo.stencilBits) - 1 : 0;
            bool needMaskedStencilClear = clearParams.clearStencil && (clearParams.stencilWriteMask & stencilUnmasked) != stencilUnmasked;

            if (needScissoredClear || needMaskedStencilClear)
            {
                maskedClearDepthStencil = renderTarget;
            }
            else
            {
                ID3D11DepthStencilView *framebufferDSV = renderTarget->getDepthStencilView();
                if (!framebufferDSV)
                {
                    return gl::Error(GL_OUT_OF_MEMORY, "Internal depth stencil view pointer unexpectedly null.");
                }

                UINT clearFlags = (clearParams.clearDepth   ? D3D11_CLEAR_DEPTH   : 0) |
                                  (clearParams.clearStencil ? D3D11_CLEAR_STENCIL : 0);
                FLOAT depthClear = gl::clamp01(clearParams.depthClearValue);
                UINT8 stencilClear = clearParams.stencilClearValue & 0xFF;

                deviceContext->ClearDepthStencilView(framebufferDSV, clearFlags, depthClear, stencilClear);
            }
        }
    }

    if (maskedClearRenderTargets.size() > 0 || maskedClearDepthStencil)
    {
        // To clear the render targets and depth stencil in one pass:
        //
        // Render a quad clipped to the scissor rectangle which draws the clear color and a blend
        // state that will perform the required color masking.
        //
        // The quad's depth is equal to the depth clear value with a depth stencil state that
        // will enable or disable depth test/writes if the depth buffer should be cleared or not.
        //
        // The rasterizer state's stencil is set to always pass or fail based on if the stencil
        // should be cleared or not with a stencil write mask of the stencil clear value.
        //
        // ======================================================================================
        //
        // Luckily, the gl spec (ES 3.0.2 pg 183) states that the results of clearing a render-
        // buffer that is not normalized fixed point or floating point with floating point values
        // are undefined so we can just write floats to them and D3D11 will bit cast them to
        // integers.
        //
        // Also, we don't have to worry about attempting to clear a normalized fixed/floating point
        // buffer with integer values because there is no gl API call which would allow it,
        // glClearBuffer* calls only clear a single renderbuffer at a time which is verified to
        // be a compatible clear type.

        // Bind all the render targets which need clearing
        ASSERT(maskedClearRenderTargets.size() <= mRenderer->getRendererCaps().maxDrawBuffers);
        std::vector<ID3D11RenderTargetView*> rtvs(maskedClearRenderTargets.size());
        for (unsigned int i = 0; i < maskedClearRenderTargets.size(); i++)
        {
            RenderTarget11 *renderTarget = maskedClearRenderTargets[i].renderTarget;
            ID3D11RenderTargetView *rtv = renderTarget->getRenderTargetView();
            if (!rtv)
            {
                return gl::Error(GL_OUT_OF_MEMORY, "Internal render target view pointer unexpectedly null.");
            }

            rtvs[i] = rtv;
        }
        ID3D11DepthStencilView *dsv = maskedClearDepthStencil ? maskedClearDepthStencil->getDepthStencilView() : NULL;

        ID3D11BlendState *blendState = getBlendState(maskedClearRenderTargets);
        const FLOAT blendFactors[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        const UINT sampleMask = 0xFFFFFFFF;

        ID3D11DepthStencilState *dsState = getDepthStencilState(clearParams);
        const UINT stencilClear = clearParams.stencilClearValue & 0xFF;

        // Set the vertices
        UINT vertexStride = 0;
        const UINT startIdx = 0;
        const ClearShader* shader = NULL;
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT result = deviceContext->Map(mVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (FAILED(result))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to map internal masked clear vertex buffer, HRESULT: 0x%X.", result);
        }

        const gl::Rectangle *scissorPtr = clearParams.scissorEnabled ? &clearParams.scissor : NULL;
        switch (clearParams.colorClearType)
        {
          case GL_FLOAT:
            ApplyVertices(framebufferSize, scissorPtr, clearParams.colorFClearValue, clearParams.depthClearValue, mappedResource.pData);
            vertexStride = sizeof(d3d11::PositionDepthColorVertex<float>);
            shader = &mFloatClearShader;
            break;

          case GL_UNSIGNED_INT:
            ApplyVertices(framebufferSize, scissorPtr, clearParams.colorUIClearValue, clearParams.depthClearValue, mappedResource.pData);
            vertexStride = sizeof(d3d11::PositionDepthColorVertex<unsigned int>);
            shader = &mUintClearShader;
            break;

          case GL_INT:
            ApplyVertices(framebufferSize, scissorPtr, clearParams.colorIClearValue, clearParams.depthClearValue, mappedResource.pData);
            vertexStride = sizeof(d3d11::PositionDepthColorVertex<int>);
            shader = &mIntClearShader;
            break;

          default:
            UNREACHABLE();
            break;
        }

        deviceContext->Unmap(mVertexBuffer, 0);

        // Set the viewport to be the same size as the framebuffer
        D3D11_VIEWPORT viewport;
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.Width = framebufferSize.width;
        viewport.Height = framebufferSize.height;
        viewport.MinDepth = 0;
        viewport.MaxDepth = 1;
        deviceContext->RSSetViewports(1, &viewport);

        // Apply state
        deviceContext->OMSetBlendState(blendState, blendFactors, sampleMask);
        deviceContext->OMSetDepthStencilState(dsState, stencilClear);
        deviceContext->RSSetState(mRasterizerState);

        // Apply shaders
        deviceContext->IASetInputLayout(shader->inputLayout);
        deviceContext->VSSetShader(shader->vertexShader, NULL, 0);
        deviceContext->PSSetShader(shader->pixelShader, NULL, 0);
        deviceContext->GSSetShader(NULL, NULL, 0);

        // Apply vertex buffer
        deviceContext->IASetVertexBuffers(0, 1, &mVertexBuffer, &vertexStride, &startIdx);
        deviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        // Apply render targets
        deviceContext->OMSetRenderTargets(rtvs.size(), (rtvs.empty() ? NULL : &rtvs[0]), dsv);

        // Draw the clear quad
        deviceContext->Draw(4, 0);

        // Clean up
        mRenderer->markAllStateDirty();
    }

    return gl::Error(GL_NO_ERROR);
}

ID3D11BlendState *Clear11::getBlendState(const std::vector<MaskedRenderTarget>& rts)
{
    ClearBlendInfo blendKey = { 0 };
    for (unsigned int i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
    {
        if (i < rts.size())
        {
            RenderTarget11 *rt = rts[i].renderTarget;
            const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(rt->getInternalFormat());

            blendKey.maskChannels[i][0] = (rts[i].colorMask[0] && formatInfo.redBits   > 0);
            blendKey.maskChannels[i][1] = (rts[i].colorMask[1] && formatInfo.greenBits > 0);
            blendKey.maskChannels[i][2] = (rts[i].colorMask[2] && formatInfo.blueBits  > 0);
            blendKey.maskChannels[i][3] = (rts[i].colorMask[3] && formatInfo.alphaBits > 0);
        }
        else
        {
            blendKey.maskChannels[i][0] = false;
            blendKey.maskChannels[i][1] = false;
            blendKey.maskChannels[i][2] = false;
            blendKey.maskChannels[i][3] = false;
        }
    }

    ClearBlendStateMap::const_iterator i = mClearBlendStates.find(blendKey);
    if (i != mClearBlendStates.end())
    {
        return i->second;
    }
    else
    {
        D3D11_BLEND_DESC blendDesc = { 0 };
        blendDesc.AlphaToCoverageEnable = FALSE;
        blendDesc.IndependentBlendEnable = (rts.size() > 1) ? TRUE : FALSE;

        for (unsigned int i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
        {
            blendDesc.RenderTarget[i].BlendEnable = FALSE;
            blendDesc.RenderTarget[i].RenderTargetWriteMask = gl_d3d11::ConvertColorMask(blendKey.maskChannels[i][0],
                                                                                         blendKey.maskChannels[i][1],
                                                                                         blendKey.maskChannels[i][2],
                                                                                         blendKey.maskChannels[i][3]);
        }

        ID3D11Device *device = mRenderer->getDevice();
        ID3D11BlendState* blendState = NULL;
        HRESULT result = device->CreateBlendState(&blendDesc, &blendState);
        if (FAILED(result) || !blendState)
        {
            ERR("Unable to create a ID3D11BlendState, HRESULT: 0x%X.", result);
            return NULL;
        }

        mClearBlendStates[blendKey] = blendState;

        return blendState;
    }
}

ID3D11DepthStencilState *Clear11::getDepthStencilState(const gl::ClearParameters &clearParams)
{
    ClearDepthStencilInfo dsKey = { 0 };
    dsKey.clearDepth = clearParams.clearDepth;
    dsKey.clearStencil = clearParams.clearStencil;
    dsKey.stencilWriteMask = clearParams.stencilWriteMask & 0xFF;

    ClearDepthStencilStateMap::const_iterator i = mClearDepthStencilStates.find(dsKey);
    if (i != mClearDepthStencilStates.end())
    {
        return i->second;
    }
    else
    {
        D3D11_DEPTH_STENCIL_DESC dsDesc = { 0 };
        dsDesc.DepthEnable = dsKey.clearDepth ? TRUE : FALSE;
        dsDesc.DepthWriteMask = dsKey.clearDepth ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
        dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        dsDesc.StencilEnable = dsKey.clearStencil ? TRUE : FALSE;
        dsDesc.StencilReadMask = 0;
        dsDesc.StencilWriteMask = dsKey.stencilWriteMask;
        dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
        dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;
        dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
        dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
        dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;
        dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
        dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

        ID3D11Device *device = mRenderer->getDevice();
        ID3D11DepthStencilState* dsState = NULL;
        HRESULT result = device->CreateDepthStencilState(&dsDesc, &dsState);
        if (FAILED(result) || !dsState)
        {
            ERR("Unable to create a ID3D11DepthStencilState, HRESULT: 0x%X.", result);
            return NULL;
        }

        mClearDepthStencilStates[dsKey] = dsState;

        return dsState;
    }
}

}
