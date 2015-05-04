//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderTarget11.h: Defines a DX11-specific wrapper for ID3D11View pointers
// retained by Renderbuffers.

#ifndef LIBGLESV2_RENDERER_RENDERTARGET11_H_
#define LIBGLESV2_RENDERER_RENDERTARGET11_H_

#include "libGLESv2/renderer/RenderTarget.h"

namespace rx
{
class SwapChain11;

class RenderTarget11 : public RenderTarget
{
  public:
    RenderTarget11() { }
    virtual ~RenderTarget11() { }

    static RenderTarget11 *makeRenderTarget11(RenderTarget *renderTarget);

    void invalidate(GLint x, GLint y, GLsizei width, GLsizei height) override;

    virtual ID3D11Resource *getTexture() const = 0;
    virtual ID3D11RenderTargetView *getRenderTargetView() const = 0;
    virtual ID3D11DepthStencilView *getDepthStencilView() const = 0;
    virtual ID3D11ShaderResourceView *getShaderResourceView() const = 0;

    virtual unsigned int getSubresourceIndex() const = 0;

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderTarget11);
};

class TextureRenderTarget11 : public RenderTarget11
{
  public:
    // TextureRenderTarget11 takes ownership of any D3D11 resources it is given and will AddRef them
    TextureRenderTarget11(ID3D11RenderTargetView *rtv, ID3D11Resource *resource, ID3D11ShaderResourceView *srv,
                           GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei samples);
    TextureRenderTarget11(ID3D11DepthStencilView *dsv, ID3D11Resource *resource, ID3D11ShaderResourceView *srv,
                           GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei samples);
    virtual ~TextureRenderTarget11();

    GLsizei getWidth() const override;
    GLsizei getHeight() const override;
    GLsizei getDepth() const override;
    GLenum getInternalFormat() const override;
    GLenum getActualFormat() const override;
    GLsizei getSamples() const override;

    ID3D11Resource *getTexture() const override;
    ID3D11RenderTargetView *getRenderTargetView() const override;
    ID3D11DepthStencilView *getDepthStencilView() const override;
    ID3D11ShaderResourceView *getShaderResourceView() const override;

    unsigned int getSubresourceIndex() const override;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureRenderTarget11);

    GLsizei mWidth;
    GLsizei mHeight;
    GLsizei mDepth;
    GLenum mInternalFormat;
    GLenum mActualFormat;
    GLsizei mSamples;

    unsigned int mSubresourceIndex;
    ID3D11Resource *mTexture;
    ID3D11RenderTargetView *mRenderTarget;
    ID3D11DepthStencilView *mDepthStencil;
    ID3D11ShaderResourceView *mShaderResource;
};

class SurfaceRenderTarget11 : public RenderTarget11
{
  public:
    SurfaceRenderTarget11(SwapChain11 *swapChain, bool depth);
    virtual ~SurfaceRenderTarget11();

    GLsizei getWidth() const override;
    GLsizei getHeight() const override;
    GLsizei getDepth() const override;
    GLenum getInternalFormat() const override;
    GLenum getActualFormat() const override;
    GLsizei getSamples() const override;

    ID3D11Resource *getTexture() const override;
    ID3D11RenderTargetView *getRenderTargetView() const override;
    ID3D11DepthStencilView *getDepthStencilView() const override;
    ID3D11ShaderResourceView *getShaderResourceView() const override;

    unsigned int getSubresourceIndex() const override;

  private:
    DISALLOW_COPY_AND_ASSIGN(SurfaceRenderTarget11);

    SwapChain11 *mSwapChain;
    bool mDepth;
};

}

#endif // LIBGLESV2_RENDERER_RENDERTARGET11_H_
