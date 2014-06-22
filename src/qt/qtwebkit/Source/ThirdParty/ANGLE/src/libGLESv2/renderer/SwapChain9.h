//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SwapChain9.h: Defines a back-end specific class for the D3D9 swap chain.

#ifndef LIBGLESV2_RENDERER_SWAPCHAIN9_H_
#define LIBGLESV2_RENDERER_SWAPCHAIN9_H_

#include "common/angleutils.h"
#include "libGLESv2/renderer/SwapChain.h"

namespace rx
{
class Renderer9;

class SwapChain9 : public SwapChain
{
  public:
    SwapChain9(Renderer9 *renderer, HWND window, HANDLE shareHandle,
               GLenum backBufferFormat, GLenum depthBufferFormat);
    virtual ~SwapChain9();

    EGLint resize(EGLint backbufferWidth, EGLint backbufferHeight);
    virtual EGLint reset(EGLint backbufferWidth, EGLint backbufferHeight, EGLint swapInterval);
    virtual EGLint swapRect(EGLint x, EGLint y, EGLint width, EGLint height);
    virtual void recreate();

    virtual IDirect3DSurface9 *getRenderTarget();
    virtual IDirect3DSurface9 *getDepthStencil();
    virtual IDirect3DTexture9 *getOffscreenTexture();

    static SwapChain9 *makeSwapChain9(SwapChain *swapChain);

  private:
    DISALLOW_COPY_AND_ASSIGN(SwapChain9);

    void release();

    Renderer9 *mRenderer;
    EGLint mHeight;
    EGLint mWidth;
    EGLint mSwapInterval;

    IDirect3DSwapChain9 *mSwapChain;
    IDirect3DSurface9 *mBackBuffer;
    IDirect3DSurface9 *mRenderTarget;
    IDirect3DSurface9 *mDepthStencil;
    IDirect3DTexture9* mOffscreenTexture;
};

}
#endif // LIBGLESV2_RENDERER_SWAPCHAIN9_H_
