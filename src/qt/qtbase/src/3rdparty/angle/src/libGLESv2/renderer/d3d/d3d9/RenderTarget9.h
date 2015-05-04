//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderTarget9.h: Defines a D3D9-specific wrapper for IDirect3DSurface9 pointers
// retained by Renderbuffers.

#ifndef LIBGLESV2_RENDERER_RENDERTARGET9_H_
#define LIBGLESV2_RENDERER_RENDERTARGET9_H_

#include "libGLESv2/renderer/RenderTarget.h"

namespace rx
{
class Renderer9;
class SwapChain9;

class RenderTarget9 : public RenderTarget
{
  public:
    RenderTarget9() { }
    virtual ~RenderTarget9() { }

    static RenderTarget9 *makeRenderTarget9(RenderTarget *renderTarget);

    void invalidate(GLint x, GLint y, GLsizei width, GLsizei height) override;

    virtual IDirect3DSurface9 *getSurface() = 0;

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderTarget9);
};

class TextureRenderTarget9 : public RenderTarget9
{
  public:
    TextureRenderTarget9(IDirect3DSurface9 *surface, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth,
                         GLsizei samples);
    virtual ~TextureRenderTarget9();

    GLsizei getWidth() const override;
    GLsizei getHeight() const override;
    GLsizei getDepth() const override;
    GLenum getInternalFormat() const override;
    GLenum getActualFormat() const override;
    GLsizei getSamples() const override;

    IDirect3DSurface9 *getSurface() override;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureRenderTarget9);

    GLsizei mWidth;
    GLsizei mHeight;
    GLsizei mDepth;
    GLenum mInternalFormat;
    GLenum mActualFormat;
    GLsizei mSamples;

    IDirect3DSurface9 *mRenderTarget;
};

class SurfaceRenderTarget9 : public RenderTarget9
{
  public:
    SurfaceRenderTarget9(SwapChain9 *swapChain, bool depth);
    virtual ~SurfaceRenderTarget9();

    GLsizei getWidth() const override;
    GLsizei getHeight() const override;
    GLsizei getDepth() const override;
    GLenum getInternalFormat() const override;
    GLenum getActualFormat() const override;
    GLsizei getSamples() const override;

    IDirect3DSurface9 *getSurface() override;

  private:
    DISALLOW_COPY_AND_ASSIGN(SurfaceRenderTarget9);

    SwapChain9 *mSwapChain;
    bool mDepth;
};

}

#endif // LIBGLESV2_RENDERER_RENDERTARGET9_H_
