//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SwapChain11.h: Defines a back-end specific class for the D3D11 swap chain.

#ifndef LIBGLESV2_RENDERER_SWAPCHAIN11_H_
#define LIBGLESV2_RENDERER_SWAPCHAIN11_H_

#include "common/angleutils.h"
#include "libGLESv2/renderer/SwapChain.h"

namespace rx
{
class Renderer11;

class SwapChain11 : public SwapChain
{
  public:
    SwapChain11(Renderer11 *renderer, EGLNativeWindowType window, HANDLE shareHandle,
                GLenum backBufferFormat, GLenum depthBufferFormat);
    virtual ~SwapChain11();

    EGLint resize(EGLint backbufferWidth, EGLint backbufferHeight);
    virtual EGLint reset(EGLint backbufferWidth, EGLint backbufferHeight, EGLint swapInterval);
    virtual EGLint swapRect(EGLint x, EGLint y, EGLint width, EGLint height);
    virtual void recreate();

    virtual ID3D11Texture2D *getOffscreenTexture();
    virtual ID3D11RenderTargetView *getRenderTarget();
    virtual ID3D11ShaderResourceView *getRenderTargetShaderResource();

    virtual ID3D11Texture2D *getDepthStencilTexture();
    virtual ID3D11DepthStencilView *getDepthStencil();

    EGLint getWidth() const { return mWidth; }
    EGLint getHeight() const { return mHeight; }

    static SwapChain11 *makeSwapChain11(SwapChain *swapChain);

  private:
    DISALLOW_COPY_AND_ASSIGN(SwapChain11);

    void release();
    void initPassThroughResources();
    void releaseOffscreenTexture();
    EGLint resetOffscreenTexture(int backbufferWidth, int backbufferHeight);

    Renderer11 *mRenderer;
    EGLint mHeight;
    EGLint mWidth;
    bool mAppCreatedShareHandle;
    unsigned int mSwapInterval;
    bool mPassThroughResourcesInit;

    IDXGISwapChain *mSwapChain;

    ID3D11Texture2D *mBackBufferTexture;
    ID3D11RenderTargetView *mBackBufferRTView;

    ID3D11Texture2D *mOffscreenTexture;
    ID3D11RenderTargetView *mOffscreenRTView;
    ID3D11ShaderResourceView *mOffscreenSRView;

    ID3D11Texture2D *mDepthStencilTexture;
    ID3D11DepthStencilView *mDepthStencilDSView;

    ID3D11Buffer *mQuadVB;
    ID3D11SamplerState *mPassThroughSampler;
    ID3D11InputLayout *mPassThroughIL;
    ID3D11VertexShader *mPassThroughVS;
    ID3D11PixelShader *mPassThroughPS;
};

}
#endif // LIBGLESV2_RENDERER_SWAPCHAIN11_H_
