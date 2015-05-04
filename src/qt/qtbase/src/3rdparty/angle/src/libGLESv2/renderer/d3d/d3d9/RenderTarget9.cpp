//
// Copyright (c) 2012-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderTarget9.cpp: Implements a D3D9-specific wrapper for IDirect3DSurface9
// pointers retained by renderbuffers.

#include "libGLESv2/renderer/d3d/d3d9/RenderTarget9.h"
#include "libGLESv2/renderer/d3d/d3d9/Renderer9.h"
#include "libGLESv2/renderer/d3d/d3d9/renderer9_utils.h"
#include "libGLESv2/renderer/d3d/d3d9/SwapChain9.h"
#include "libGLESv2/renderer/d3d/d3d9/formatutils9.h"
#include "libGLESv2/main.h"

namespace rx
{

RenderTarget9 *RenderTarget9::makeRenderTarget9(RenderTarget *target)
{
    ASSERT(HAS_DYNAMIC_TYPE(RenderTarget9*, target));
    return static_cast<RenderTarget9*>(target);
}

void RenderTarget9::invalidate(GLint x, GLint y, GLsizei width, GLsizei height)
{
        // Currently a no-op
}

// TODO: AddRef the incoming surface to take ownership instead of expecting that its ref is being given.
TextureRenderTarget9::TextureRenderTarget9(IDirect3DSurface9 *surface, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth,
                                           GLsizei samples)
    : mWidth(width),
      mHeight(height),
      mDepth(depth),
      mInternalFormat(internalFormat),
      mActualFormat(internalFormat),
      mSamples(samples),
      mRenderTarget(surface)
{
    ASSERT(mDepth == 1);

    if (mRenderTarget)
    {
        D3DSURFACE_DESC description;
        mRenderTarget->GetDesc(&description);

        const d3d9::D3DFormat &d3dFormatInfo = d3d9::GetD3DFormatInfo(description.Format);
        mActualFormat = d3dFormatInfo.internalFormat;
    }
}

TextureRenderTarget9::~TextureRenderTarget9()
{
    SafeRelease(mRenderTarget);
}

GLsizei TextureRenderTarget9::getWidth() const
{
    return mWidth;
}

GLsizei TextureRenderTarget9::getHeight() const
{
    return mHeight;
}

GLsizei TextureRenderTarget9::getDepth() const
{
    return mDepth;
}

GLenum TextureRenderTarget9::getInternalFormat() const
{
    return mInternalFormat;
}

GLenum TextureRenderTarget9::getActualFormat() const
{
    return mActualFormat;
}

GLsizei TextureRenderTarget9::getSamples() const
{
    return mSamples;
}

IDirect3DSurface9 *TextureRenderTarget9::getSurface()
{
    // Caller is responsible for releasing the returned surface reference.
    // TODO: remove the AddRef to match RenderTarget11
    if (mRenderTarget)
    {
        mRenderTarget->AddRef();
    }

    return mRenderTarget;
}


SurfaceRenderTarget9::SurfaceRenderTarget9(SwapChain9 *swapChain, bool depth)
    : mSwapChain(swapChain),
      mDepth(depth)
{
}

SurfaceRenderTarget9::~SurfaceRenderTarget9()
{
}

GLsizei SurfaceRenderTarget9::getWidth() const
{
    return mSwapChain->getWidth();
}

GLsizei SurfaceRenderTarget9::getHeight() const
{
    return mSwapChain->getHeight();
}

GLsizei SurfaceRenderTarget9::getDepth() const
{
    return 1;
}

GLenum SurfaceRenderTarget9::getInternalFormat() const
{
    return (mDepth ? mSwapChain->GetDepthBufferInternalFormat() : mSwapChain->GetBackBufferInternalFormat());
}

GLenum SurfaceRenderTarget9::getActualFormat() const
{
    return d3d9::GetD3DFormatInfo(d3d9::GetTextureFormatInfo(getInternalFormat()).texFormat).internalFormat;
}

GLsizei SurfaceRenderTarget9::getSamples() const
{
    // Our EGL surfaces do not support multisampling.
    return 0;
}

IDirect3DSurface9 *SurfaceRenderTarget9::getSurface()
{
    return (mDepth ? mSwapChain->getDepthStencil() : mSwapChain->getRenderTarget());
}

}
