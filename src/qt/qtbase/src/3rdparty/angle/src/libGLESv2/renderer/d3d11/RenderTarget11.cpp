#include "precompiled.h"
//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderTarget11.cpp: Implements a DX11-specific wrapper for ID3D11View pointers
// retained by Renderbuffers.

#include "libGLESv2/renderer/d3d11/RenderTarget11.h"
#include "libGLESv2/renderer/d3d11/Renderer11.h"

#include "libGLESv2/renderer/d3d11/renderer11_utils.h"
#include "libGLESv2/main.h"

namespace rx
{

static unsigned int getRTVSubresourceIndex(ID3D11Texture2D *texture, ID3D11RenderTargetView *view)
{
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
    view->GetDesc(&rtvDesc);

    D3D11_TEXTURE2D_DESC texDesc;
    texture->GetDesc(&texDesc);

    unsigned int mipSlice = 0;
    unsigned int arraySlice = 0;
    unsigned int mipLevels = texDesc.MipLevels;

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

    return D3D11CalcSubresource(mipSlice, arraySlice, mipLevels);
}

static unsigned int getDSVSubresourceIndex(ID3D11Texture2D *texture, ID3D11DepthStencilView *view)
{
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    view->GetDesc(&dsvDesc);

    D3D11_TEXTURE2D_DESC texDesc;
    texture->GetDesc(&texDesc);

    unsigned int mipSlice = 0;
    unsigned int arraySlice = 0;
    unsigned int mipLevels = texDesc.MipLevels;

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

      case D3D11_RTV_DIMENSION_UNKNOWN:
        UNIMPLEMENTED();
        break;

      default:
        UNREACHABLE();
        break;
    }

    return D3D11CalcSubresource(mipSlice, arraySlice, mipLevels);
}

RenderTarget11::RenderTarget11(Renderer *renderer, ID3D11RenderTargetView *rtv, ID3D11Texture2D *tex, ID3D11ShaderResourceView *srv, GLsizei width, GLsizei height)
{
    mRenderer = Renderer11::makeRenderer11(renderer);
    mTexture = tex;
    mRenderTarget = rtv;
    mDepthStencil = NULL;
    mShaderResource = srv;
    mSubresourceIndex = 0;

    if (mRenderTarget && mTexture)
    {
        D3D11_RENDER_TARGET_VIEW_DESC desc;
        mRenderTarget->GetDesc(&desc);

        D3D11_TEXTURE2D_DESC texDesc;
        mTexture->GetDesc(&texDesc);

        mSubresourceIndex = getRTVSubresourceIndex(mTexture, mRenderTarget);
        mWidth = width;
        mHeight = height;
        mSamples = (texDesc.SampleDesc.Count > 1) ? texDesc.SampleDesc.Count : 0;

        mInternalFormat = d3d11_gl::ConvertTextureInternalFormat(desc.Format);
        mActualFormat = d3d11_gl::ConvertTextureInternalFormat(desc.Format);
    }
}

RenderTarget11::RenderTarget11(Renderer *renderer, ID3D11DepthStencilView *dsv, ID3D11Texture2D *tex, ID3D11ShaderResourceView *srv, GLsizei width, GLsizei height)
{
    mRenderer = Renderer11::makeRenderer11(renderer);
    mTexture = tex;
    mRenderTarget = NULL;
    mDepthStencil = dsv;
    mShaderResource = srv;
    mSubresourceIndex = 0;

    if (mDepthStencil && mTexture)
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC desc;
        mDepthStencil->GetDesc(&desc);

        D3D11_TEXTURE2D_DESC texDesc;
        mTexture->GetDesc(&texDesc);

        mSubresourceIndex = getDSVSubresourceIndex(mTexture, mDepthStencil);
        mWidth = width;
        mHeight = height;
        mSamples = (texDesc.SampleDesc.Count > 1) ? texDesc.SampleDesc.Count : 0;

        mInternalFormat = d3d11_gl::ConvertTextureInternalFormat(desc.Format);
        mActualFormat = d3d11_gl::ConvertTextureInternalFormat(desc.Format);
    }
}

RenderTarget11::RenderTarget11(Renderer *renderer, GLsizei width, GLsizei height, GLenum format, GLsizei samples, bool depth)
{
    mRenderer = Renderer11::makeRenderer11(renderer);
    mTexture = NULL;
    mRenderTarget = NULL;
    mDepthStencil = NULL;
    mShaderResource = NULL;

    DXGI_FORMAT requestedFormat = gl_d3d11::ConvertRenderbufferFormat(format);

    int supportedSamples = mRenderer->getNearestSupportedSamples(requestedFormat, samples);
    if (supportedSamples < 0)
    {
        gl::error(GL_OUT_OF_MEMORY);
        return;
    }

    if (width > 0 && height > 0)
    {
        // Create texture resource
        D3D11_TEXTURE2D_DESC desc;
        desc.Width = width; 
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = requestedFormat;
        desc.SampleDesc.Count = (supportedSamples == 0) ? 1 : supportedSamples;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.BindFlags = (depth ? D3D11_BIND_DEPTH_STENCIL : (D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE));

        ID3D11Device *device = mRenderer->getDevice();
        HRESULT result = device->CreateTexture2D(&desc, NULL, &mTexture);

        if (result == E_OUTOFMEMORY)
        {
            gl::error(GL_OUT_OF_MEMORY);
            return;
        }
        ASSERT(SUCCEEDED(result));

        if (depth)
        {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
            dsvDesc.Format = requestedFormat;
            dsvDesc.ViewDimension = (supportedSamples == 0) ? D3D11_DSV_DIMENSION_TEXTURE2D : D3D11_DSV_DIMENSION_TEXTURE2DMS;
            dsvDesc.Texture2D.MipSlice = 0;
            dsvDesc.Flags = 0;
            result = device->CreateDepthStencilView(mTexture, &dsvDesc, &mDepthStencil);

            if (result == E_OUTOFMEMORY)
            {
                mTexture->Release();
                mTexture = NULL;
                gl::error(GL_OUT_OF_MEMORY);
            }
            ASSERT(SUCCEEDED(result));
        }
        else
        {
            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
            rtvDesc.Format = requestedFormat;
            rtvDesc.ViewDimension = (supportedSamples == 0) ? D3D11_RTV_DIMENSION_TEXTURE2D : D3D11_RTV_DIMENSION_TEXTURE2DMS;
            rtvDesc.Texture2D.MipSlice = 0;
            result = device->CreateRenderTargetView(mTexture, &rtvDesc, &mRenderTarget);

            if (result == E_OUTOFMEMORY)
            {
                mTexture->Release();
                mTexture = NULL;
                gl::error(GL_OUT_OF_MEMORY);
                return;
            }
            ASSERT(SUCCEEDED(result));

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            srvDesc.Format = requestedFormat;
            srvDesc.ViewDimension = (supportedSamples == 0) ? D3D11_SRV_DIMENSION_TEXTURE2D : D3D11_SRV_DIMENSION_TEXTURE2DMS;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = 1;
            result = device->CreateShaderResourceView(mTexture, &srvDesc, &mShaderResource);

            if (result == E_OUTOFMEMORY)
            {
                mTexture->Release();
                mTexture = NULL;
                mRenderTarget->Release();
                mRenderTarget = NULL;
                gl::error(GL_OUT_OF_MEMORY);
                return;
            }
            ASSERT(SUCCEEDED(result));
        }
    }

    mWidth = width;
    mHeight = height;
    mInternalFormat = format;
    mSamples = supportedSamples;
    mActualFormat = d3d11_gl::ConvertTextureInternalFormat(requestedFormat);
    mSubresourceIndex = D3D11CalcSubresource(0, 0, 1);
}

RenderTarget11::~RenderTarget11()
{
    if (mTexture)
    {
        mTexture->Release();
        mTexture = NULL;
    }

    if (mRenderTarget)
    {
        mRenderTarget->Release();
        mRenderTarget = NULL;
    }

    if (mDepthStencil)
    {
        mDepthStencil->Release();
        mDepthStencil = NULL;
    }

    if (mShaderResource)
    {
        mShaderResource->Release();
        mShaderResource = NULL;
    }
}

RenderTarget11 *RenderTarget11::makeRenderTarget11(RenderTarget *target)
{
    ASSERT(HAS_DYNAMIC_TYPE(rx::RenderTarget11*, target));
    return static_cast<rx::RenderTarget11*>(target);
}

ID3D11Texture2D *RenderTarget11::getTexture() const
{
    return mTexture;
}

ID3D11RenderTargetView *RenderTarget11::getRenderTargetView() const
{
    return mRenderTarget;
}

ID3D11DepthStencilView *RenderTarget11::getDepthStencilView() const
{
    return mDepthStencil;
}

ID3D11ShaderResourceView *RenderTarget11::getShaderResourceView() const
{
    return mShaderResource;
}

unsigned int RenderTarget11::getSubresourceIndex() const
{
    return mSubresourceIndex;
}

}
