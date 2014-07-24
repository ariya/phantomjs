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
class Renderer;
class Renderer11;

class RenderTarget11 : public RenderTarget
{
  public:
    RenderTarget11(Renderer *renderer, ID3D11RenderTargetView *rtv, ID3D11Texture2D *tex, ID3D11ShaderResourceView *srv, GLsizei width, GLsizei height);
    RenderTarget11(Renderer *renderer, ID3D11DepthStencilView *dsv, ID3D11Texture2D *tex, ID3D11ShaderResourceView *srv, GLsizei width, GLsizei height);
    RenderTarget11(Renderer *renderer, GLsizei width, GLsizei height, GLenum format, GLsizei samples, bool depth);
    virtual ~RenderTarget11();

    static RenderTarget11 *makeRenderTarget11(RenderTarget *renderTarget);

    ID3D11Texture2D *getTexture() const;
    ID3D11RenderTargetView *getRenderTargetView() const;
    ID3D11DepthStencilView *getDepthStencilView() const;
    ID3D11ShaderResourceView *getShaderResourceView() const;

    unsigned int getSubresourceIndex() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderTarget11);

    unsigned int mSubresourceIndex;
    ID3D11Texture2D *mTexture;
    ID3D11RenderTargetView *mRenderTarget;
    ID3D11DepthStencilView *mDepthStencil;
    ID3D11ShaderResourceView *mShaderResource;

    Renderer11 *mRenderer;
};

}

#endif LIBGLESV2_RENDERER_RENDERTARGET11_H_