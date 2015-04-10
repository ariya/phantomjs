//
// Copyright (c) 2012-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderStateCache.h: Defines rx::RenderStateCache, a cache of Direct3D render
// state objects.

#ifndef LIBGLESV2_RENDERER_RENDERSTATECACHE_H_
#define LIBGLESV2_RENDERER_RENDERSTATECACHE_H_

#include "libGLESv2/angletypes.h"
#include "common/angleutils.h"

namespace gl
{
class Framebuffer;
}

namespace rx
{

class RenderStateCache
{
  public:
    RenderStateCache();
    virtual ~RenderStateCache();

    void initialize(ID3D11Device *device);
    void clear();

    // Increments refcount on the returned blend state, Release() must be called.
    ID3D11BlendState *getBlendState(gl::Framebuffer *framebuffer, const gl::BlendState &blendState);
    ID3D11RasterizerState *getRasterizerState(const gl::RasterizerState &rasterState,
                                              bool scissorEnabled, unsigned int depthSize);
    ID3D11DepthStencilState *getDepthStencilState(const gl::DepthStencilState &dsState);
    ID3D11SamplerState *getSamplerState(const gl::SamplerState &samplerState);

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderStateCache);

    unsigned long long mCounter;

    // Blend state cache
    struct BlendStateKey
    {
        gl::BlendState blendState;
        bool rtChannels[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT][4];
    };
    static std::size_t hashBlendState(const BlendStateKey &blendState);
    static bool compareBlendStates(const BlendStateKey &a, const BlendStateKey &b);
    static const unsigned int kMaxBlendStates;

    typedef std::size_t (*BlendStateHashFunction)(const BlendStateKey &);
    typedef bool (*BlendStateEqualityFunction)(const BlendStateKey &, const BlendStateKey &);
    typedef std::pair<ID3D11BlendState*, unsigned long long> BlendStateCounterPair;
    typedef std::unordered_map<BlendStateKey, BlendStateCounterPair, BlendStateHashFunction, BlendStateEqualityFunction> BlendStateMap;
    BlendStateMap mBlendStateCache;

    // Rasterizer state cache
    struct RasterizerStateKey
    {
        gl::RasterizerState rasterizerState;
        bool scissorEnabled;
        unsigned int depthSize;
    };
    static std::size_t hashRasterizerState(const RasterizerStateKey &rasterState);
    static bool compareRasterizerStates(const RasterizerStateKey &a, const RasterizerStateKey &b);
    static const unsigned int kMaxRasterizerStates;

    typedef std::size_t (*RasterizerStateHashFunction)(const RasterizerStateKey &);
    typedef bool (*RasterizerStateEqualityFunction)(const RasterizerStateKey &, const RasterizerStateKey &);
    typedef std::pair<ID3D11RasterizerState*, unsigned long long> RasterizerStateCounterPair;
    typedef std::unordered_map<RasterizerStateKey, RasterizerStateCounterPair, RasterizerStateHashFunction, RasterizerStateEqualityFunction> RasterizerStateMap;
    RasterizerStateMap mRasterizerStateCache;

    // Depth stencil state cache
    static std::size_t hashDepthStencilState(const gl::DepthStencilState &dsState);
    static bool compareDepthStencilStates(const gl::DepthStencilState &a, const gl::DepthStencilState &b);
    static const unsigned int kMaxDepthStencilStates;

    typedef std::size_t (*DepthStencilStateHashFunction)(const gl::DepthStencilState &);
    typedef bool (*DepthStencilStateEqualityFunction)(const gl::DepthStencilState &, const gl::DepthStencilState &);
    typedef std::pair<ID3D11DepthStencilState*, unsigned long long> DepthStencilStateCounterPair;
    typedef std::unordered_map<gl::DepthStencilState,
                               DepthStencilStateCounterPair,
                               DepthStencilStateHashFunction,
                               DepthStencilStateEqualityFunction> DepthStencilStateMap;
    DepthStencilStateMap mDepthStencilStateCache;

    // Sample state cache
    static std::size_t hashSamplerState(const gl::SamplerState &samplerState);
    static bool compareSamplerStates(const gl::SamplerState &a, const gl::SamplerState &b);
    static const unsigned int kMaxSamplerStates;

    typedef std::size_t (*SamplerStateHashFunction)(const gl::SamplerState &);
    typedef bool (*SamplerStateEqualityFunction)(const gl::SamplerState &, const gl::SamplerState &);
    typedef std::pair<ID3D11SamplerState*, unsigned long long> SamplerStateCounterPair;
    typedef std::unordered_map<gl::SamplerState,
                               SamplerStateCounterPair,
                               SamplerStateHashFunction,
                               SamplerStateEqualityFunction> SamplerStateMap;
    SamplerStateMap mSamplerStateCache;

    ID3D11Device *mDevice;
};

}

#endif // LIBGLESV2_RENDERER_RENDERSTATECACHE_H_