//
// Copyright (c) 2012-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderStateCache.cpp: Defines rx::RenderStateCache, a cache of Direct3D render
// state objects.

#include "libGLESv2/renderer/d3d/d3d11/RenderStateCache.h"
#include "libGLESv2/renderer/d3d/d3d11/renderer11_utils.h"
#include "libGLESv2/renderer/d3d/d3d11/Renderer11.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/FramebufferAttachment.h"

#include "common/debug.h"

#include "third_party/murmurhash/MurmurHash3.h"

namespace rx
{

template <typename mapType>
static void ClearStateMap(mapType &map)
{
    for (typename mapType::iterator i = map.begin(); i != map.end(); i++)
    {
        SafeRelease(i->second.first);
    }
    map.clear();
}

// MSDN's documentation of ID3D11Device::CreateBlendState, ID3D11Device::CreateRasterizerState,
// ID3D11Device::CreateDepthStencilState and ID3D11Device::CreateSamplerState claims the maximum
// number of unique states of each type an application can create is 4096
const unsigned int RenderStateCache::kMaxBlendStates = 4096;
const unsigned int RenderStateCache::kMaxRasterizerStates = 4096;
const unsigned int RenderStateCache::kMaxDepthStencilStates = 4096;
const unsigned int RenderStateCache::kMaxSamplerStates = 4096;

RenderStateCache::RenderStateCache(Renderer11 *renderer)
    : mRenderer(renderer),
      mDevice(NULL),
      mCounter(0),
      mBlendStateCache(kMaxBlendStates, hashBlendState, compareBlendStates),
      mRasterizerStateCache(kMaxRasterizerStates, hashRasterizerState, compareRasterizerStates),
      mDepthStencilStateCache(kMaxDepthStencilStates, hashDepthStencilState, compareDepthStencilStates),
      mSamplerStateCache(kMaxSamplerStates, hashSamplerState, compareSamplerStates)
{
}

RenderStateCache::~RenderStateCache()
{
    clear();
}

void RenderStateCache::initialize(ID3D11Device *device)
{
    clear();
    mDevice = device;
}

void RenderStateCache::clear()
{
    ClearStateMap(mBlendStateCache);
    ClearStateMap(mRasterizerStateCache);
    ClearStateMap(mDepthStencilStateCache);
    ClearStateMap(mSamplerStateCache);
}

std::size_t RenderStateCache::hashBlendState(const BlendStateKey &blendState)
{
    static const unsigned int seed = 0xABCDEF98;

    std::size_t hash = 0;
    MurmurHash3_x86_32(&blendState, sizeof(gl::BlendState), seed, &hash);
    return hash;
}

bool RenderStateCache::compareBlendStates(const BlendStateKey &a, const BlendStateKey &b)
{
    return memcmp(&a, &b, sizeof(BlendStateKey)) == 0;
}

gl::Error RenderStateCache::getBlendState(const gl::Framebuffer *framebuffer, const gl::BlendState &blendState,
                                          ID3D11BlendState **outBlendState)
{
    if (!mDevice)
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Internal error, RenderStateCache is not initialized.");
    }

    bool mrt = false;

    const gl::ColorbufferInfo &colorbuffers = framebuffer->getColorbuffersForRender(mRenderer->getWorkarounds());

    BlendStateKey key = { 0 };
    key.blendState = blendState;
    for (size_t colorAttachment = 0; colorAttachment < colorbuffers.size(); ++colorAttachment)
    {
        const gl::FramebufferAttachment *attachment = colorbuffers[colorAttachment];

        auto rtChannels = key.rtChannels[colorAttachment];

        if (attachment)
        {
            if (colorAttachment > 0)
            {
                mrt = true;
            }

            rtChannels[0] = attachment->getRedSize()   > 0;
            rtChannels[1] = attachment->getGreenSize() > 0;
            rtChannels[2] = attachment->getBlueSize()  > 0;
            rtChannels[3] = attachment->getAlphaSize() > 0;
        }
    }

    BlendStateMap::iterator keyIter = mBlendStateCache.find(key);
    if (keyIter != mBlendStateCache.end())
    {
        BlendStateCounterPair &state = keyIter->second;
        state.second = mCounter++;
        *outBlendState = state.first;
        return gl::Error(GL_NO_ERROR);
    }
    else
    {
        if (mBlendStateCache.size() >= kMaxBlendStates)
        {
            TRACE("Overflowed the limit of %u blend states, removing the least recently used "
                  "to make room.", kMaxBlendStates);

            BlendStateMap::iterator leastRecentlyUsed = mBlendStateCache.begin();
            for (BlendStateMap::iterator i = mBlendStateCache.begin(); i != mBlendStateCache.end(); i++)
            {
                if (i->second.second < leastRecentlyUsed->second.second)
                {
                    leastRecentlyUsed = i;
                }
            }
            SafeRelease(leastRecentlyUsed->second.first);
            mBlendStateCache.erase(leastRecentlyUsed);
        }

        // Create a new blend state and insert it into the cache
        D3D11_BLEND_DESC blendDesc = { 0 };
        blendDesc.AlphaToCoverageEnable = blendState.sampleAlphaToCoverage;
        blendDesc.IndependentBlendEnable = mrt ? TRUE : FALSE;

        for (unsigned int i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
        {
            D3D11_RENDER_TARGET_BLEND_DESC &rtBlend = blendDesc.RenderTarget[i];

            rtBlend.BlendEnable = blendState.blend;
            if (blendState.blend)
            {
                rtBlend.SrcBlend = gl_d3d11::ConvertBlendFunc(blendState.sourceBlendRGB, false);
                rtBlend.DestBlend = gl_d3d11::ConvertBlendFunc(blendState.destBlendRGB, false);
                rtBlend.BlendOp = gl_d3d11::ConvertBlendOp(blendState.blendEquationRGB);

                rtBlend.SrcBlendAlpha = gl_d3d11::ConvertBlendFunc(blendState.sourceBlendAlpha, true);
                rtBlend.DestBlendAlpha = gl_d3d11::ConvertBlendFunc(blendState.destBlendAlpha, true);
                rtBlend.BlendOpAlpha = gl_d3d11::ConvertBlendOp(blendState.blendEquationAlpha);
            }

            rtBlend.RenderTargetWriteMask = gl_d3d11::ConvertColorMask(key.rtChannels[i][0] && blendState.colorMaskRed,
                                                                       key.rtChannels[i][1] && blendState.colorMaskGreen,
                                                                       key.rtChannels[i][2] && blendState.colorMaskBlue,
                                                                       key.rtChannels[i][3] && blendState.colorMaskAlpha);
        }

        ID3D11BlendState *dx11BlendState = NULL;
        HRESULT result = mDevice->CreateBlendState(&blendDesc, &dx11BlendState);
        if (FAILED(result) || !dx11BlendState)
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Unable to create a ID3D11BlendState, HRESULT: 0x%X.", result);
        }

        mBlendStateCache.insert(std::make_pair(key, std::make_pair(dx11BlendState, mCounter++)));

        *outBlendState = dx11BlendState;
        return gl::Error(GL_NO_ERROR);
    }
}

std::size_t RenderStateCache::hashRasterizerState(const RasterizerStateKey &rasterState)
{
    static const unsigned int seed = 0xABCDEF98;

    std::size_t hash = 0;
    MurmurHash3_x86_32(&rasterState, sizeof(RasterizerStateKey), seed, &hash);
    return hash;
}

bool RenderStateCache::compareRasterizerStates(const RasterizerStateKey &a, const RasterizerStateKey &b)
{
    return memcmp(&a, &b, sizeof(RasterizerStateKey)) == 0;
}

gl::Error RenderStateCache::getRasterizerState(const gl::RasterizerState &rasterState, bool scissorEnabled,
                                               ID3D11RasterizerState **outRasterizerState)
{
    if (!mDevice)
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Internal error, RenderStateCache is not initialized.");
    }

    RasterizerStateKey key = { 0 };
    key.rasterizerState = rasterState;
    key.scissorEnabled = scissorEnabled;

    RasterizerStateMap::iterator keyIter = mRasterizerStateCache.find(key);
    if (keyIter != mRasterizerStateCache.end())
    {
        RasterizerStateCounterPair &state = keyIter->second;
        state.second = mCounter++;
        *outRasterizerState = state.first;
        return gl::Error(GL_NO_ERROR);
    }
    else
    {
        if (mRasterizerStateCache.size() >= kMaxRasterizerStates)
        {
            TRACE("Overflowed the limit of %u rasterizer states, removing the least recently used "
                  "to make room.", kMaxRasterizerStates);

            RasterizerStateMap::iterator leastRecentlyUsed = mRasterizerStateCache.begin();
            for (RasterizerStateMap::iterator i = mRasterizerStateCache.begin(); i != mRasterizerStateCache.end(); i++)
            {
                if (i->second.second < leastRecentlyUsed->second.second)
                {
                    leastRecentlyUsed = i;
                }
            }
            SafeRelease(leastRecentlyUsed->second.first);
            mRasterizerStateCache.erase(leastRecentlyUsed);
        }

        D3D11_CULL_MODE cullMode = gl_d3d11::ConvertCullMode(rasterState.cullFace, rasterState.cullMode);

        // Disable culling if drawing points
        if (rasterState.pointDrawMode)
        {
            cullMode = D3D11_CULL_NONE;
        }

        D3D11_RASTERIZER_DESC rasterDesc;
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.CullMode = cullMode;
        rasterDesc.FrontCounterClockwise = (rasterState.frontFace == GL_CCW) ? FALSE: TRUE;
        rasterDesc.DepthBiasClamp = 0.0f; // MSDN documentation of DepthBiasClamp implies a value of zero will preform no clamping, must be tested though.
        rasterDesc.DepthClipEnable = TRUE;
        rasterDesc.ScissorEnable = scissorEnabled ? TRUE : FALSE;
        rasterDesc.MultisampleEnable = rasterState.multiSample;
        rasterDesc.AntialiasedLineEnable = FALSE;

        if (rasterState.polygonOffsetFill)
        {
            rasterDesc.SlopeScaledDepthBias = rasterState.polygonOffsetFactor;
            rasterDesc.DepthBias = (INT)rasterState.polygonOffsetUnits;
        }
        else
        {
            rasterDesc.SlopeScaledDepthBias = 0.0f;
            rasterDesc.DepthBias = 0;
        }

        ID3D11RasterizerState *dx11RasterizerState = NULL;
        HRESULT result = mDevice->CreateRasterizerState(&rasterDesc, &dx11RasterizerState);
        if (FAILED(result) || !dx11RasterizerState)
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Unable to create a ID3D11RasterizerState, HRESULT: 0x%X.", result);
        }

        mRasterizerStateCache.insert(std::make_pair(key, std::make_pair(dx11RasterizerState, mCounter++)));

        *outRasterizerState = dx11RasterizerState;
        return gl::Error(GL_NO_ERROR);
    }
}

std::size_t RenderStateCache::hashDepthStencilState(const gl::DepthStencilState &dsState)
{
    static const unsigned int seed = 0xABCDEF98;

    std::size_t hash = 0;
    MurmurHash3_x86_32(&dsState, sizeof(gl::DepthStencilState), seed, &hash);
    return hash;
}

bool RenderStateCache::compareDepthStencilStates(const gl::DepthStencilState &a, const gl::DepthStencilState &b)
{
    return memcmp(&a, &b, sizeof(gl::DepthStencilState)) == 0;
}

gl::Error RenderStateCache::getDepthStencilState(const gl::DepthStencilState &dsState, ID3D11DepthStencilState **outDSState)
{
    if (!mDevice)
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Internal error, RenderStateCache is not initialized.");
    }

    DepthStencilStateMap::iterator keyIter = mDepthStencilStateCache.find(dsState);
    if (keyIter != mDepthStencilStateCache.end())
    {
        DepthStencilStateCounterPair &state = keyIter->second;
        state.second = mCounter++;
        *outDSState = state.first;
        return gl::Error(GL_NO_ERROR);
    }
    else
    {
        if (mDepthStencilStateCache.size() >= kMaxDepthStencilStates)
        {
            TRACE("Overflowed the limit of %u depth stencil states, removing the least recently used "
                  "to make room.", kMaxDepthStencilStates);

            DepthStencilStateMap::iterator leastRecentlyUsed = mDepthStencilStateCache.begin();
            for (DepthStencilStateMap::iterator i = mDepthStencilStateCache.begin(); i != mDepthStencilStateCache.end(); i++)
            {
                if (i->second.second < leastRecentlyUsed->second.second)
                {
                    leastRecentlyUsed = i;
                }
            }
            SafeRelease(leastRecentlyUsed->second.first);
            mDepthStencilStateCache.erase(leastRecentlyUsed);
        }

        D3D11_DEPTH_STENCIL_DESC dsDesc = { 0 };
        dsDesc.DepthEnable = dsState.depthTest ? TRUE : FALSE;
        dsDesc.DepthWriteMask = gl_d3d11::ConvertDepthMask(dsState.depthMask);
        dsDesc.DepthFunc = gl_d3d11::ConvertComparison(dsState.depthFunc);
        dsDesc.StencilEnable = dsState.stencilTest ? TRUE : FALSE;
        dsDesc.StencilReadMask = gl_d3d11::ConvertStencilMask(dsState.stencilMask);
        dsDesc.StencilWriteMask = gl_d3d11::ConvertStencilMask(dsState.stencilWritemask);
        dsDesc.FrontFace.StencilFailOp = gl_d3d11::ConvertStencilOp(dsState.stencilFail);
        dsDesc.FrontFace.StencilDepthFailOp = gl_d3d11::ConvertStencilOp(dsState.stencilPassDepthFail);
        dsDesc.FrontFace.StencilPassOp = gl_d3d11::ConvertStencilOp(dsState.stencilPassDepthPass);
        dsDesc.FrontFace.StencilFunc = gl_d3d11::ConvertComparison(dsState.stencilFunc);
        dsDesc.BackFace.StencilFailOp = gl_d3d11::ConvertStencilOp(dsState.stencilBackFail);
        dsDesc.BackFace.StencilDepthFailOp = gl_d3d11::ConvertStencilOp(dsState.stencilBackPassDepthFail);
        dsDesc.BackFace.StencilPassOp = gl_d3d11::ConvertStencilOp(dsState.stencilBackPassDepthPass);
        dsDesc.BackFace.StencilFunc = gl_d3d11::ConvertComparison(dsState.stencilBackFunc);

        ID3D11DepthStencilState *dx11DepthStencilState = NULL;
        HRESULT result = mDevice->CreateDepthStencilState(&dsDesc, &dx11DepthStencilState);
        if (FAILED(result) || !dx11DepthStencilState)
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Unable to create a ID3D11DepthStencilState, HRESULT: 0x%X.", result);
        }

        mDepthStencilStateCache.insert(std::make_pair(dsState, std::make_pair(dx11DepthStencilState, mCounter++)));

        *outDSState = dx11DepthStencilState;
        return gl::Error(GL_NO_ERROR);
    }
}

std::size_t RenderStateCache::hashSamplerState(const gl::SamplerState &samplerState)
{
    static const unsigned int seed = 0xABCDEF98;

    std::size_t hash = 0;
    MurmurHash3_x86_32(&samplerState, sizeof(gl::SamplerState), seed, &hash);
    return hash;
}

bool RenderStateCache::compareSamplerStates(const gl::SamplerState &a, const gl::SamplerState &b)
{
    return memcmp(&a, &b, sizeof(gl::SamplerState)) == 0;
}

gl::Error RenderStateCache::getSamplerState(const gl::SamplerState &samplerState, ID3D11SamplerState **outSamplerState)
{
    if (!mDevice)
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Internal error, RenderStateCache is not initialized.");
    }

    SamplerStateMap::iterator keyIter = mSamplerStateCache.find(samplerState);
    if (keyIter != mSamplerStateCache.end())
    {
        SamplerStateCounterPair &state = keyIter->second;
        state.second = mCounter++;
        *outSamplerState = state.first;
        return gl::Error(GL_NO_ERROR);
    }
    else
    {
        if (mSamplerStateCache.size() >= kMaxSamplerStates)
        {
            TRACE("Overflowed the limit of %u sampler states, removing the least recently used "
                  "to make room.", kMaxSamplerStates);

            SamplerStateMap::iterator leastRecentlyUsed = mSamplerStateCache.begin();
            for (SamplerStateMap::iterator i = mSamplerStateCache.begin(); i != mSamplerStateCache.end(); i++)
            {
                if (i->second.second < leastRecentlyUsed->second.second)
                {
                    leastRecentlyUsed = i;
                }
            }
            SafeRelease(leastRecentlyUsed->second.first);
            mSamplerStateCache.erase(leastRecentlyUsed);
        }

        D3D11_SAMPLER_DESC samplerDesc;
        samplerDesc.Filter = gl_d3d11::ConvertFilter(samplerState.minFilter, samplerState.magFilter,
                                                     samplerState.maxAnisotropy, samplerState.compareMode);
        samplerDesc.AddressU = gl_d3d11::ConvertTextureWrap(samplerState.wrapS);
        samplerDesc.AddressV = gl_d3d11::ConvertTextureWrap(samplerState.wrapT);
        samplerDesc.AddressW = gl_d3d11::ConvertTextureWrap(samplerState.wrapR);
        samplerDesc.MipLODBias = 0;
        samplerDesc.MaxAnisotropy = samplerState.maxAnisotropy;
        samplerDesc.ComparisonFunc = gl_d3d11::ConvertComparison(samplerState.compareFunc);
        samplerDesc.BorderColor[0] = 0.0f;
        samplerDesc.BorderColor[1] = 0.0f;
        samplerDesc.BorderColor[2] = 0.0f;
        samplerDesc.BorderColor[3] = 0.0f;
        samplerDesc.MinLOD = samplerState.minLod;
        samplerDesc.MaxLOD = samplerState.maxLod;

        ID3D11SamplerState *dx11SamplerState = NULL;
        HRESULT result = mDevice->CreateSamplerState(&samplerDesc, &dx11SamplerState);
        if (FAILED(result) || !dx11SamplerState)
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Unable to create a ID3D11SamplerState, HRESULT: 0x%X.", result);
        }

        mSamplerStateCache.insert(std::make_pair(samplerState, std::make_pair(dx11SamplerState, mCounter++)));

        *outSamplerState = dx11SamplerState;
        return gl::Error(GL_NO_ERROR);
    }
}

}
