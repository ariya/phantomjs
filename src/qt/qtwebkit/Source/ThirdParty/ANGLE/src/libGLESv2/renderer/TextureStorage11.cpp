#include "precompiled.h"
//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureStorage11.cpp: Implements the abstract rx::TextureStorage11 class and its concrete derived
// classes TextureStorage11_2D and TextureStorage11_Cube, which act as the interface to the D3D11 texture.

#include "libGLESv2/renderer/TextureStorage11.h"

#include "libGLESv2/renderer/Renderer11.h"
#include "libGLESv2/renderer/RenderTarget11.h"
#include "libGLESv2/renderer/SwapChain11.h"
#include "libGLESv2/renderer/renderer11_utils.h"

#include "libGLESv2/utilities.h"
#include "libGLESv2/main.h"

namespace rx
{

TextureStorage11::TextureStorage11(Renderer *renderer, UINT bindFlags)
    : mBindFlags(bindFlags),
      mLodOffset(0),
      mMipLevels(0),
      mTexture(NULL),
      mTextureFormat(DXGI_FORMAT_UNKNOWN),
      mShaderResourceFormat(DXGI_FORMAT_UNKNOWN),
      mRenderTargetFormat(DXGI_FORMAT_UNKNOWN),
      mDepthStencilFormat(DXGI_FORMAT_UNKNOWN),
      mSRV(NULL),
      mTextureWidth(0),
      mTextureHeight(0)
{
    mRenderer = Renderer11::makeRenderer11(renderer);
}

TextureStorage11::~TextureStorage11()
{
}

TextureStorage11 *TextureStorage11::makeTextureStorage11(TextureStorage *storage)
{
    ASSERT(HAS_DYNAMIC_TYPE(TextureStorage11*, storage));
    return static_cast<TextureStorage11*>(storage);
}

DWORD TextureStorage11::GetTextureBindFlags(DXGI_FORMAT format, GLenum glusage, bool forceRenderable)
{
    UINT bindFlags = D3D11_BIND_SHADER_RESOURCE;
    
    if (d3d11::IsDepthStencilFormat(format))
    {
        bindFlags |= D3D11_BIND_DEPTH_STENCIL;
    }
    else if(forceRenderable || (TextureStorage11::IsTextureFormatRenderable(format) && (glusage == GL_FRAMEBUFFER_ATTACHMENT_ANGLE)))
    {
        bindFlags |= D3D11_BIND_RENDER_TARGET;
    }
    return bindFlags;
}

bool TextureStorage11::IsTextureFormatRenderable(DXGI_FORMAT format)
{
    switch(format)
    {
      case DXGI_FORMAT_R8G8B8A8_UNORM:
      case DXGI_FORMAT_A8_UNORM:
      case DXGI_FORMAT_R32G32B32A32_FLOAT:
      case DXGI_FORMAT_R32G32B32_FLOAT:
      case DXGI_FORMAT_R16G16B16A16_FLOAT:
      case DXGI_FORMAT_B8G8R8A8_UNORM:
      case DXGI_FORMAT_R8_UNORM:
      case DXGI_FORMAT_R8G8_UNORM:
      case DXGI_FORMAT_R16_FLOAT:
      case DXGI_FORMAT_R16G16_FLOAT:
        return true;
      case DXGI_FORMAT_BC1_UNORM:
      case DXGI_FORMAT_BC2_UNORM: 
      case DXGI_FORMAT_BC3_UNORM:
        return false;
      default:
        UNREACHABLE();
        return false;
    }
}

UINT TextureStorage11::getBindFlags() const
{
    return mBindFlags;
}

ID3D11Texture2D *TextureStorage11::getBaseTexture() const
{
    return mTexture;
}

int TextureStorage11::getLodOffset() const
{
    return mLodOffset;
}

bool TextureStorage11::isRenderTarget() const
{
    return (mBindFlags & (D3D11_BIND_RENDER_TARGET | D3D11_BIND_DEPTH_STENCIL)) != 0;
}
    
bool TextureStorage11::isManaged() const
{
    return false;
}

int TextureStorage11::levelCount()
{
    int levels = 0;
    if (getBaseTexture())
    {
        levels = mMipLevels - getLodOffset();
    }
    return levels;
}

UINT TextureStorage11::getSubresourceIndex(int level, int faceIndex)
{
    UINT index = 0;
    if (getBaseTexture())
    {
        index = D3D11CalcSubresource(level, faceIndex, mMipLevels);
    }
    return index;
}

bool TextureStorage11::updateSubresourceLevel(ID3D11Texture2D *srcTexture, unsigned int sourceSubresource,
                                              int level, int face, GLint xoffset, GLint yoffset,
                                              GLsizei width, GLsizei height)
{
    if (srcTexture)
    {
        // Round up the width and height to the nearest multiple of dimension alignment
        unsigned int dimensionAlignment = d3d11::GetTextureFormatDimensionAlignment(mTextureFormat);
        width = width + dimensionAlignment - 1 - (width - 1) % dimensionAlignment;
        height = height + dimensionAlignment - 1 - (height - 1) % dimensionAlignment;

        D3D11_BOX srcBox;
        srcBox.left = xoffset;
        srcBox.top = yoffset;
        srcBox.right = xoffset + width;
        srcBox.bottom = yoffset + height;
        srcBox.front = 0;
        srcBox.back = 1;

        ID3D11DeviceContext *context = mRenderer->getDeviceContext();
        
        ASSERT(getBaseTexture());
        context->CopySubresourceRegion(getBaseTexture(), getSubresourceIndex(level + mLodOffset, face),
                                       xoffset, yoffset, 0, srcTexture, sourceSubresource, &srcBox);
        return true;
    }

    return false;
}

void TextureStorage11::generateMipmapLayer(RenderTarget11 *source, RenderTarget11 *dest)
{
    if (source && dest)
    {
        ID3D11ShaderResourceView *sourceSRV = source->getShaderResourceView();
        ID3D11RenderTargetView *destRTV = dest->getRenderTargetView();

        if (sourceSRV && destRTV)
        {
            gl::Rectangle sourceArea;
            sourceArea.x = 0;
            sourceArea.y = 0;
            sourceArea.width = source->getWidth();
            sourceArea.height = source->getHeight();

            gl::Rectangle destArea;
            destArea.x = 0;
            destArea.y = 0;
            destArea.width = dest->getWidth();
            destArea.height = dest->getHeight();

            mRenderer->copyTexture(sourceSRV, sourceArea, source->getWidth(), source->getHeight(),
                                   destRTV, destArea, dest->getWidth(), dest->getHeight(),
                                   GL_RGBA);
        }

        if (sourceSRV)
        {
            sourceSRV->Release();
        }
        if (destRTV)
        {
            destRTV->Release();
        }
    }
}

TextureStorage11_2D::TextureStorage11_2D(Renderer *renderer, SwapChain11 *swapchain)
    : TextureStorage11(renderer, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
{
    mTexture = swapchain->getOffscreenTexture();
    mSRV = swapchain->getRenderTargetShaderResource();

    for (unsigned int i = 0; i < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
    {
        mRenderTarget[i] = NULL;
    }

    D3D11_TEXTURE2D_DESC texDesc;
    mTexture->GetDesc(&texDesc);
    mMipLevels = texDesc.MipLevels;
    mTextureFormat = texDesc.Format;
    mTextureWidth = texDesc.Width;
    mTextureHeight = texDesc.Height;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    mSRV->GetDesc(&srvDesc);
    mShaderResourceFormat = srvDesc.Format;

    ID3D11RenderTargetView* offscreenRTV = swapchain->getRenderTarget();
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
    offscreenRTV->GetDesc(&rtvDesc);
    mRenderTargetFormat = rtvDesc.Format;
    offscreenRTV->Release();

    mDepthStencilFormat = DXGI_FORMAT_UNKNOWN;
}

TextureStorage11_2D::TextureStorage11_2D(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, GLsizei width, GLsizei height)
    : TextureStorage11(renderer, GetTextureBindFlags(gl_d3d11::ConvertTextureFormat(internalformat), usage, forceRenderable))
{
    for (unsigned int i = 0; i < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
    {
        mRenderTarget[i] = NULL;
    }

    DXGI_FORMAT convertedFormat = gl_d3d11::ConvertTextureFormat(internalformat);
    if (d3d11::IsDepthStencilFormat(convertedFormat))
    {
        mTextureFormat = d3d11::GetDepthTextureFormat(convertedFormat);
        mShaderResourceFormat = d3d11::GetDepthShaderResourceFormat(convertedFormat);
        mDepthStencilFormat = convertedFormat;
        mRenderTargetFormat = DXGI_FORMAT_UNKNOWN;
    }
    else
    {
        mTextureFormat = convertedFormat;
        mShaderResourceFormat = convertedFormat;
        mDepthStencilFormat = DXGI_FORMAT_UNKNOWN;
        mRenderTargetFormat = convertedFormat;
    }

    // if the width or height is not positive this should be treated as an incomplete texture
    // we handle that here by skipping the d3d texture creation
    if (width > 0 && height > 0)
    {
        // adjust size if needed for compressed textures
        gl::MakeValidSize(false, gl::IsCompressed(internalformat), &width, &height, &mLodOffset);

        ID3D11Device *device = mRenderer->getDevice();

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = width;      // Compressed texture size constraints?
        desc.Height = height;
        desc.MipLevels = (levels > 0) ? levels + mLodOffset : 0;
        desc.ArraySize = 1;
        desc.Format = mTextureFormat;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = getBindFlags();
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        HRESULT result = device->CreateTexture2D(&desc, NULL, &mTexture);

        // this can happen from windows TDR
        if (d3d11::isDeviceLostError(result))
        {
            mRenderer->notifyDeviceLost();
            gl::error(GL_OUT_OF_MEMORY);
        }
        else if (FAILED(result))
        {
            ASSERT(result == E_OUTOFMEMORY);
            ERR("Creating image failed.");
            gl::error(GL_OUT_OF_MEMORY);
        }
        else
        {
            mTexture->GetDesc(&desc);
            mMipLevels = desc.MipLevels;
            mTextureWidth = desc.Width;
            mTextureHeight = desc.Height;
        }
    }
}

TextureStorage11_2D::~TextureStorage11_2D()
{
    if (mTexture)
    {
        mTexture->Release();
        mTexture = NULL;
    }

    if (mSRV)
    {
        mSRV->Release();
        mSRV = NULL;
    }

    for (unsigned int i = 0; i < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
    {
        delete mRenderTarget[i];
        mRenderTarget[i] = NULL;
    }
}

TextureStorage11_2D *TextureStorage11_2D::makeTextureStorage11_2D(TextureStorage *storage)
{
    ASSERT(HAS_DYNAMIC_TYPE(TextureStorage11_2D*, storage));
    return static_cast<TextureStorage11_2D*>(storage);
}

RenderTarget *TextureStorage11_2D::getRenderTarget(int level)
{
    if (level >= 0 && level < static_cast<int>(mMipLevels))
    {
        if (!mRenderTarget[level])
        {
            ID3D11Device *device = mRenderer->getDevice();
            HRESULT result;

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            srvDesc.Format = mShaderResourceFormat;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MostDetailedMip = level;
            srvDesc.Texture2D.MipLevels = 1;

            ID3D11ShaderResourceView *srv;
            result = device->CreateShaderResourceView(mTexture, &srvDesc, &srv);

            if (result == E_OUTOFMEMORY)
            {
                return gl::error(GL_OUT_OF_MEMORY, static_cast<RenderTarget*>(NULL));
            }
            ASSERT(SUCCEEDED(result));

            if (mRenderTargetFormat != DXGI_FORMAT_UNKNOWN)
            {
                D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
                rtvDesc.Format = mRenderTargetFormat;
                rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                rtvDesc.Texture2D.MipSlice = level;

                ID3D11RenderTargetView *rtv;
                result = device->CreateRenderTargetView(mTexture, &rtvDesc, &rtv);

                if (result == E_OUTOFMEMORY)
                {
                    srv->Release();
                    return gl::error(GL_OUT_OF_MEMORY, static_cast<RenderTarget*>(NULL));
                }
                ASSERT(SUCCEEDED(result));

                // RenderTarget11 expects to be the owner of the resources it is given but TextureStorage11
                // also needs to keep a reference to the texture.
                mTexture->AddRef();

                mRenderTarget[level] = new RenderTarget11(mRenderer, rtv, mTexture, srv,
                                                          std::max(mTextureWidth >> level, 1U),
                                                          std::max(mTextureHeight >> level, 1U));
            }
            else if (mDepthStencilFormat != DXGI_FORMAT_UNKNOWN)
            {
                D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
                dsvDesc.Format = mDepthStencilFormat;
                dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                dsvDesc.Texture2D.MipSlice = level;
                dsvDesc.Flags = 0;

                ID3D11DepthStencilView *dsv;
                result = device->CreateDepthStencilView(mTexture, &dsvDesc, &dsv);

                if (result == E_OUTOFMEMORY)
                {
                    srv->Release();
                    return gl::error(GL_OUT_OF_MEMORY, static_cast<RenderTarget*>(NULL));
                }
                ASSERT(SUCCEEDED(result));

                // RenderTarget11 expects to be the owner of the resources it is given but TextureStorage11
                // also needs to keep a reference to the texture.
                mTexture->AddRef();

                mRenderTarget[level] = new RenderTarget11(mRenderer, dsv, mTexture, srv,
                                                          std::max(mTextureWidth >> level, 1U),
                                                          std::max(mTextureHeight >> level, 1U));
            }
            else
            {
                UNREACHABLE();
            }
        }

        return mRenderTarget[level];
    }
    else
    {
        return NULL;
    }
}

ID3D11ShaderResourceView *TextureStorage11_2D::getSRV()
{
    if (!mSRV)
    {
        ID3D11Device *device = mRenderer->getDevice();

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = mShaderResourceFormat;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = (mMipLevels == 0 ? -1 : mMipLevels);
        srvDesc.Texture2D.MostDetailedMip = 0;

        HRESULT result = device->CreateShaderResourceView(mTexture, &srvDesc, &mSRV);

        if (result == E_OUTOFMEMORY)
        {
            return gl::error(GL_OUT_OF_MEMORY, static_cast<ID3D11ShaderResourceView*>(NULL));
        }
        ASSERT(SUCCEEDED(result));
    }

    return mSRV;
}

void TextureStorage11_2D::generateMipmap(int level)
{
    RenderTarget11 *source = RenderTarget11::makeRenderTarget11(getRenderTarget(level - 1));
    RenderTarget11 *dest = RenderTarget11::makeRenderTarget11(getRenderTarget(level));

    generateMipmapLayer(source, dest);
}

TextureStorage11_Cube::TextureStorage11_Cube(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, int size)
    : TextureStorage11(renderer, GetTextureBindFlags(gl_d3d11::ConvertTextureFormat(internalformat), usage, forceRenderable))
{
    for (unsigned int i = 0; i < 6; i++)
    {
        for (unsigned int j = 0; j < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; j++)
        {
            mRenderTarget[i][j] = NULL;
        }
    }

    DXGI_FORMAT convertedFormat = gl_d3d11::ConvertTextureFormat(internalformat);
    if (d3d11::IsDepthStencilFormat(convertedFormat))
    {
        mTextureFormat = d3d11::GetDepthTextureFormat(convertedFormat);
        mShaderResourceFormat = d3d11::GetDepthShaderResourceFormat(convertedFormat);
        mDepthStencilFormat = convertedFormat;
        mRenderTargetFormat = DXGI_FORMAT_UNKNOWN;
    }
    else
    {
        mTextureFormat = convertedFormat;
        mShaderResourceFormat = convertedFormat;
        mDepthStencilFormat = DXGI_FORMAT_UNKNOWN;
        mRenderTargetFormat = convertedFormat;
    }

    // if the size is not positive this should be treated as an incomplete texture
    // we handle that here by skipping the d3d texture creation
    if (size > 0)
    {
        // adjust size if needed for compressed textures
        int height = size;
        gl::MakeValidSize(false, gl::IsCompressed(internalformat), &size, &height, &mLodOffset);

        ID3D11Device *device = mRenderer->getDevice();

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = size;
        desc.Height = size;
        desc.MipLevels = (levels > 0) ? levels + mLodOffset : 0;
        desc.ArraySize = 6;
        desc.Format = mTextureFormat;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = getBindFlags();
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

        HRESULT result = device->CreateTexture2D(&desc, NULL, &mTexture);

        if (FAILED(result))
        {
            ASSERT(result == E_OUTOFMEMORY);
            ERR("Creating image failed.");
            gl::error(GL_OUT_OF_MEMORY);
        }
        else
        {
            mTexture->GetDesc(&desc);
            mMipLevels = desc.MipLevels;
            mTextureWidth = desc.Width;
            mTextureHeight = desc.Height;
        }
    }
}

TextureStorage11_Cube::~TextureStorage11_Cube()
{
    if (mTexture)
    {
        mTexture->Release();
        mTexture = NULL;
    }

    if (mSRV)
    {
        mSRV->Release();
        mSRV = NULL;
    }

    for (unsigned int i = 0; i < 6; i++)
    {
        for (unsigned int j = 0; j < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; j++)
        {
            delete mRenderTarget[i][j];
            mRenderTarget[i][j] = NULL;
        }
    }
}

TextureStorage11_Cube *TextureStorage11_Cube::makeTextureStorage11_Cube(TextureStorage *storage)
{
    ASSERT(HAS_DYNAMIC_TYPE(TextureStorage11_Cube*, storage));
    return static_cast<TextureStorage11_Cube*>(storage);
}

RenderTarget *TextureStorage11_Cube::getRenderTarget(GLenum faceTarget, int level)
{
    unsigned int faceIdx = gl::TextureCubeMap::faceIndex(faceTarget);
    if (level >= 0 && level < static_cast<int>(mMipLevels))
    {
        if (!mRenderTarget[faceIdx][level])
        {
            ID3D11Device *device = mRenderer->getDevice();
            HRESULT result;

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            srvDesc.Format = mShaderResourceFormat;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
            srvDesc.Texture2DArray.MostDetailedMip = level;
            srvDesc.Texture2DArray.MipLevels = 1;
            srvDesc.Texture2DArray.FirstArraySlice = faceIdx;
            srvDesc.Texture2DArray.ArraySize = 1;

            ID3D11ShaderResourceView *srv;
            result = device->CreateShaderResourceView(mTexture, &srvDesc, &srv);

            if (result == E_OUTOFMEMORY)
            {
                return gl::error(GL_OUT_OF_MEMORY, static_cast<RenderTarget*>(NULL));
            }
            ASSERT(SUCCEEDED(result));

            if (mRenderTargetFormat != DXGI_FORMAT_UNKNOWN)
            {
                D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
                rtvDesc.Format = mRenderTargetFormat;
                rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                rtvDesc.Texture2DArray.MipSlice = level;
                rtvDesc.Texture2DArray.FirstArraySlice = faceIdx;
                rtvDesc.Texture2DArray.ArraySize = 1;

                ID3D11RenderTargetView *rtv;
                result = device->CreateRenderTargetView(mTexture, &rtvDesc, &rtv);

                if (result == E_OUTOFMEMORY)
                {
                    srv->Release();
                    return gl::error(GL_OUT_OF_MEMORY, static_cast<RenderTarget*>(NULL));
                }
                ASSERT(SUCCEEDED(result));

                // RenderTarget11 expects to be the owner of the resources it is given but TextureStorage11
                // also needs to keep a reference to the texture.
                mTexture->AddRef();

                mRenderTarget[faceIdx][level] = new RenderTarget11(mRenderer, rtv, mTexture, srv,
                                                                   std::max(mTextureWidth >> level, 1U),
                                                                   std::max(mTextureHeight >> level, 1U));
            }
            else if (mDepthStencilFormat != DXGI_FORMAT_UNKNOWN)
            {
                D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
                dsvDesc.Format = mRenderTargetFormat;
                dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                dsvDesc.Texture2DArray.MipSlice = level;
                dsvDesc.Texture2DArray.FirstArraySlice = faceIdx;
                dsvDesc.Texture2DArray.ArraySize = 1;

                ID3D11DepthStencilView *dsv;
                result = device->CreateDepthStencilView(mTexture, &dsvDesc, &dsv);

                if (result == E_OUTOFMEMORY)
                {
                    srv->Release();
                    return gl::error(GL_OUT_OF_MEMORY, static_cast<RenderTarget*>(NULL));
                }
                ASSERT(SUCCEEDED(result));

                // RenderTarget11 expects to be the owner of the resources it is given but TextureStorage11
                // also needs to keep a reference to the texture.
                mTexture->AddRef();

                mRenderTarget[faceIdx][level] = new RenderTarget11(mRenderer, dsv, mTexture, srv,
                                                                   std::max(mTextureWidth >> level, 1U),
                                                                   std::max(mTextureHeight >> level, 1U));
            }
            else
            {
                UNREACHABLE();
            }
        }

        return mRenderTarget[faceIdx][level];
    }
    else
    {
        return NULL;
    }
}

ID3D11ShaderResourceView *TextureStorage11_Cube::getSRV()
{
    if (!mSRV)
    {
        ID3D11Device *device = mRenderer->getDevice();

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = mShaderResourceFormat;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels = (mMipLevels == 0 ? -1 : mMipLevels);
        srvDesc.TextureCube.MostDetailedMip = 0;

        HRESULT result = device->CreateShaderResourceView(mTexture, &srvDesc, &mSRV);

        if (result == E_OUTOFMEMORY)
        {
            return gl::error(GL_OUT_OF_MEMORY, static_cast<ID3D11ShaderResourceView*>(NULL));
        }
        ASSERT(SUCCEEDED(result));
    }

    return mSRV;
}

void TextureStorage11_Cube::generateMipmap(int face, int level)
{
    RenderTarget11 *source = RenderTarget11::makeRenderTarget11(getRenderTarget(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level - 1));
    RenderTarget11 *dest = RenderTarget11::makeRenderTarget11(getRenderTarget(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level));

    generateMipmapLayer(source, dest);
}

}
