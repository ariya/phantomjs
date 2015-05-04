//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderTarget11.cpp: Implements a DX11-specific wrapper for ID3D11View pointers
// retained by Renderbuffers.

#include "libGLESv2/renderer/d3d/d3d11/RenderTarget11.h"
#include "libGLESv2/renderer/d3d/d3d11/Renderer11.h"
#include "libGLESv2/renderer/d3d/d3d11/renderer11_utils.h"
#include "libGLESv2/renderer/d3d/d3d11/SwapChain11.h"
#include "libGLESv2/renderer/d3d/d3d11/formatutils11.h"
#include "libGLESv2/main.h"

namespace rx
{

static bool getTextureProperties(ID3D11Resource *resource, unsigned int *mipLevels, unsigned int *samples)
{
    ID3D11Texture1D *texture1D = d3d11::DynamicCastComObject<ID3D11Texture1D>(resource);
    if (texture1D)
    {
        D3D11_TEXTURE1D_DESC texDesc;
        texture1D->GetDesc(&texDesc);
        SafeRelease(texture1D);

        *mipLevels = texDesc.MipLevels;
        *samples = 0;

        return true;
    }

    ID3D11Texture2D *texture2D = d3d11::DynamicCastComObject<ID3D11Texture2D>(resource);
    if (texture2D)
    {
        D3D11_TEXTURE2D_DESC texDesc;
        texture2D->GetDesc(&texDesc);
        SafeRelease(texture2D);

        *mipLevels = texDesc.MipLevels;
        *samples = texDesc.SampleDesc.Count > 1 ? texDesc.SampleDesc.Count : 0;

        return true;
    }

    ID3D11Texture3D *texture3D = d3d11::DynamicCastComObject<ID3D11Texture3D>(resource);
    if (texture3D)
    {
        D3D11_TEXTURE3D_DESC texDesc;
        texture3D->GetDesc(&texDesc);
        SafeRelease(texture3D);

        *mipLevels = texDesc.MipLevels;
        *samples = 0;

        return true;
    }

    return false;
}

static unsigned int getRTVSubresourceIndex(ID3D11Resource *resource, ID3D11RenderTargetView *view)
{
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
    view->GetDesc(&rtvDesc);

    unsigned int mipSlice = 0;
    unsigned int arraySlice = 0;

    switch (rtvDesc.ViewDimension)
    {
      case D3D11_RTV_DIMENSION_TEXTURE1D:
        mipSlice = rtvDesc.Texture1D.MipSlice;
        arraySlice = 0;
        break;

      case D3D11_RTV_DIMENSION_TEXTURE1DARRAY:
        mipSlice = rtvDesc.Texture1DArray.MipSlice;
        arraySlice = rtvDesc.Texture1DArray.FirstArraySlice;
        break;

      case D3D11_RTV_DIMENSION_TEXTURE2D:
        mipSlice = rtvDesc.Texture2D.MipSlice;
        arraySlice = 0;
        break;

      case D3D11_RTV_DIMENSION_TEXTURE2DARRAY:
        mipSlice = rtvDesc.Texture2DArray.MipSlice;
        arraySlice = rtvDesc.Texture2DArray.FirstArraySlice;
        break;

      case D3D11_RTV_DIMENSION_TEXTURE2DMS:
        mipSlice = 0;
        arraySlice = 0;
        break;

      case D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY:
        mipSlice = 0;
        arraySlice = rtvDesc.Texture2DMSArray.FirstArraySlice;
        break;

      case D3D11_RTV_DIMENSION_TEXTURE3D:
        mipSlice = rtvDesc.Texture3D.MipSlice;
        arraySlice = 0;
        break;

      case D3D11_RTV_DIMENSION_UNKNOWN:
      case D3D11_RTV_DIMENSION_BUFFER:
        UNIMPLEMENTED();
        break;

      default:
        UNREACHABLE();
        break;
    }

    unsigned int mipLevels, samples;
    getTextureProperties(resource,  &mipLevels, &samples);

    return D3D11CalcSubresource(mipSlice, arraySlice, mipLevels);
}

static unsigned int getDSVSubresourceIndex(ID3D11Resource *resource, ID3D11DepthStencilView *view)
{
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    view->GetDesc(&dsvDesc);

    unsigned int mipSlice = 0;
    unsigned int arraySlice = 0;

    switch (dsvDesc.ViewDimension)
    {
      case D3D11_DSV_DIMENSION_TEXTURE1D:
        mipSlice = dsvDesc.Texture1D.MipSlice;
        arraySlice = 0;
        break;

      case D3D11_DSV_DIMENSION_TEXTURE1DARRAY:
        mipSlice = dsvDesc.Texture1DArray.MipSlice;
        arraySlice = dsvDesc.Texture1DArray.FirstArraySlice;
        break;

      case D3D11_DSV_DIMENSION_TEXTURE2D:
        mipSlice = dsvDesc.Texture2D.MipSlice;
        arraySlice = 0;
        break;

      case D3D11_DSV_DIMENSION_TEXTURE2DARRAY:
        mipSlice = dsvDesc.Texture2DArray.MipSlice;
        arraySlice = dsvDesc.Texture2DArray.FirstArraySlice;
        break;

      case D3D11_DSV_DIMENSION_TEXTURE2DMS:
        mipSlice = 0;
        arraySlice = 0;
        break;

      case D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY:
        mipSlice = 0;
        arraySlice = dsvDesc.Texture2DMSArray.FirstArraySlice;
        break;

      case D3D11_DSV_DIMENSION_UNKNOWN:
        UNIMPLEMENTED();
        break;

      default:
        UNREACHABLE();
        break;
    }

    unsigned int mipLevels, samples;
    getTextureProperties(resource, &mipLevels, &samples);

    return D3D11CalcSubresource(mipSlice, arraySlice, mipLevels);
}

RenderTarget11 *RenderTarget11::makeRenderTarget11(RenderTarget *target)
{
    ASSERT(HAS_DYNAMIC_TYPE(RenderTarget11*, target));
    return static_cast<RenderTarget11*>(target);
}

void RenderTarget11::invalidate(GLint x, GLint y, GLsizei width, GLsizei height)
{
    // Currently a no-op
}

TextureRenderTarget11::TextureRenderTarget11(ID3D11RenderTargetView *rtv, ID3D11Resource *resource, ID3D11ShaderResourceView *srv,
                                             GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei samples)
    : mWidth(width),
      mHeight(height),
      mDepth(depth),
      mInternalFormat(internalFormat),
      mActualFormat(internalFormat),
      mSamples(samples),
      mSubresourceIndex(0),
      mTexture(resource),
      mRenderTarget(rtv),
      mDepthStencil(NULL),
      mShaderResource(srv)
{
    if (mTexture)
    {
        mTexture->AddRef();
    }

    if (mRenderTarget)
    {
        mRenderTarget->AddRef();
    }

    if (mShaderResource)
    {
        mShaderResource->AddRef();
    }

    if (mRenderTarget && mTexture)
    {
        mSubresourceIndex = getRTVSubresourceIndex(mTexture, mRenderTarget);

        D3D11_RENDER_TARGET_VIEW_DESC desc;
        mRenderTarget->GetDesc(&desc);

        const d3d11::DXGIFormat &dxgiFormatInfo = d3d11::GetDXGIFormatInfo(desc.Format);
        mActualFormat = dxgiFormatInfo.internalFormat;
    }
}

TextureRenderTarget11::TextureRenderTarget11(ID3D11DepthStencilView *dsv, ID3D11Resource *resource, ID3D11ShaderResourceView *srv,
                                             GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei samples)
    : mWidth(width),
      mHeight(height),
      mDepth(depth),
      mInternalFormat(internalFormat),
      mActualFormat(internalFormat),
      mSamples(samples),
      mSubresourceIndex(0),
      mTexture(resource),
      mRenderTarget(NULL),
      mDepthStencil(dsv),
      mShaderResource(srv)
{
    if (mTexture)
    {
        mTexture->AddRef();
    }

    if (mDepthStencil)
    {
        mDepthStencil->AddRef();
    }

    if (mShaderResource)
    {
        mShaderResource->AddRef();
    }

    if (mDepthStencil && mTexture)
    {
        mSubresourceIndex = getDSVSubresourceIndex(mTexture, mDepthStencil);

        D3D11_DEPTH_STENCIL_VIEW_DESC desc;
        mDepthStencil->GetDesc(&desc);

        const d3d11::DXGIFormat &dxgiFormatInfo = d3d11::GetDXGIFormatInfo(desc.Format);
        mActualFormat = dxgiFormatInfo.internalFormat;
    }
}

TextureRenderTarget11::~TextureRenderTarget11()
{
    SafeRelease(mTexture);
    SafeRelease(mRenderTarget);
    SafeRelease(mDepthStencil);
    SafeRelease(mShaderResource);
}

ID3D11Resource *TextureRenderTarget11::getTexture() const
{
    return mTexture;
}

ID3D11RenderTargetView *TextureRenderTarget11::getRenderTargetView() const
{
    return mRenderTarget;
}

ID3D11DepthStencilView *TextureRenderTarget11::getDepthStencilView() const
{
    return mDepthStencil;
}

ID3D11ShaderResourceView *TextureRenderTarget11::getShaderResourceView() const
{
    return mShaderResource;
}

GLsizei TextureRenderTarget11::getWidth() const
{
    return mWidth;
}

GLsizei TextureRenderTarget11::getHeight() const
{
    return mHeight;
}

GLsizei TextureRenderTarget11::getDepth() const
{
    return mDepth;
}

GLenum TextureRenderTarget11::getInternalFormat() const
{
    return mInternalFormat;
}

GLenum TextureRenderTarget11::getActualFormat() const
{
    return mActualFormat;
}

GLsizei TextureRenderTarget11::getSamples() const
{
    return mSamples;
}

unsigned int TextureRenderTarget11::getSubresourceIndex() const
{
    return mSubresourceIndex;
}


SurfaceRenderTarget11::SurfaceRenderTarget11(SwapChain11 *swapChain, bool depth)
    : mSwapChain(swapChain),
      mDepth(depth)
{
    ASSERT(mSwapChain);
}

SurfaceRenderTarget11::~SurfaceRenderTarget11()
{
}

GLsizei SurfaceRenderTarget11::getWidth() const
{
    return mSwapChain->getWidth();
}

GLsizei SurfaceRenderTarget11::getHeight() const
{
    return mSwapChain->getHeight();
}

GLsizei SurfaceRenderTarget11::getDepth() const
{
    return 1;
}

GLenum SurfaceRenderTarget11::getInternalFormat() const
{
    return (mDepth ? mSwapChain->GetDepthBufferInternalFormat() : mSwapChain->GetBackBufferInternalFormat());
}

GLenum SurfaceRenderTarget11::getActualFormat() const
{
    return d3d11::GetDXGIFormatInfo(d3d11::GetTextureFormatInfo(getInternalFormat()).texFormat).internalFormat;
}

GLsizei SurfaceRenderTarget11::getSamples() const
{
    // Our EGL surfaces do not support multisampling.
    return 0;
}

ID3D11Resource *SurfaceRenderTarget11::getTexture() const
{
    return (mDepth ? mSwapChain->getDepthStencilTexture() : mSwapChain->getOffscreenTexture());
}

ID3D11RenderTargetView *SurfaceRenderTarget11::getRenderTargetView() const
{
    return (mDepth ? NULL : mSwapChain->getRenderTarget());
}

ID3D11DepthStencilView *SurfaceRenderTarget11::getDepthStencilView() const
{
    return (mDepth ? mSwapChain->getDepthStencil() : NULL);
}

ID3D11ShaderResourceView *SurfaceRenderTarget11::getShaderResourceView() const
{
    return (mDepth ? mSwapChain->getDepthStencilShaderResource() : mSwapChain->getRenderTargetShaderResource());
}

unsigned int SurfaceRenderTarget11::getSubresourceIndex() const
{
    return 0;
}

}
