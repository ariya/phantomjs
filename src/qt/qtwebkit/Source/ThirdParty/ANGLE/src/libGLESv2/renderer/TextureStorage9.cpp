#include "precompiled.h"
//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureStorage9.cpp: Implements the abstract rx::TextureStorage9 class and its concrete derived
// classes TextureStorage9_2D and TextureStorage9_Cube, which act as the interface to the
// D3D9 texture.

#include "libGLESv2/main.h"
#include "libGLESv2/renderer/Renderer9.h"
#include "libGLESv2/renderer/TextureStorage9.h"
#include "libGLESv2/renderer/SwapChain9.h"
#include "libGLESv2/renderer/RenderTarget9.h"
#include "libGLESv2/renderer/renderer9_utils.h"
#include "libGLESv2/Texture.h"

namespace rx
{
TextureStorage9::TextureStorage9(Renderer *renderer, DWORD usage)
    : mLodOffset(0),
      mRenderer(Renderer9::makeRenderer9(renderer)),
      mD3DUsage(usage),
      mD3DPool(mRenderer->getTexturePool(usage))
{
}

TextureStorage9::~TextureStorage9()
{
}

TextureStorage9 *TextureStorage9::makeTextureStorage9(TextureStorage *storage)
{
    ASSERT(HAS_DYNAMIC_TYPE(TextureStorage9*, storage));
    return static_cast<TextureStorage9*>(storage);
}

DWORD TextureStorage9::GetTextureUsage(D3DFORMAT d3dfmt, GLenum glusage, bool forceRenderable)
{
    DWORD d3dusage = 0;

    if (d3dfmt == D3DFMT_INTZ)
    {
        d3dusage |= D3DUSAGE_DEPTHSTENCIL;
    }
    else if(forceRenderable || (TextureStorage9::IsTextureFormatRenderable(d3dfmt) && (glusage == GL_FRAMEBUFFER_ATTACHMENT_ANGLE)))
    {
        d3dusage |= D3DUSAGE_RENDERTARGET;
    }
    return d3dusage;
}

bool TextureStorage9::IsTextureFormatRenderable(D3DFORMAT format)
{
    if (format == D3DFMT_INTZ)
    {
        return true;
    }
    switch(format)
    {
      case D3DFMT_L8:
      case D3DFMT_A8L8:
      case D3DFMT_DXT1:
      case D3DFMT_DXT3:
      case D3DFMT_DXT5:
        return false;
      case D3DFMT_A8R8G8B8:
      case D3DFMT_X8R8G8B8:
      case D3DFMT_A16B16G16R16F:
      case D3DFMT_A32B32G32R32F:
        return true;
      default:
        UNREACHABLE();
    }

    return false;
}

bool TextureStorage9::isRenderTarget() const
{
    return (mD3DUsage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL)) != 0;
}

bool TextureStorage9::isManaged() const
{
    return (mD3DPool == D3DPOOL_MANAGED);
}

D3DPOOL TextureStorage9::getPool() const
{
    return mD3DPool;
}

DWORD TextureStorage9::getUsage() const
{
    return mD3DUsage;
}

int TextureStorage9::getLodOffset() const
{
    return mLodOffset;
}

int TextureStorage9::levelCount()
{
    return getBaseTexture() ? getBaseTexture()->GetLevelCount() - getLodOffset() : 0;
}

TextureStorage9_2D::TextureStorage9_2D(Renderer *renderer, SwapChain9 *swapchain) : TextureStorage9(renderer, D3DUSAGE_RENDERTARGET)
{
    IDirect3DTexture9 *surfaceTexture = swapchain->getOffscreenTexture();
    mTexture = surfaceTexture;
    mRenderTarget = NULL;

    initializeRenderTarget();
}

TextureStorage9_2D::TextureStorage9_2D(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, GLsizei width, GLsizei height)
    : TextureStorage9(renderer, GetTextureUsage(Renderer9::makeRenderer9(renderer)->ConvertTextureInternalFormat(internalformat), usage, forceRenderable))
{
    mTexture = NULL;
    mRenderTarget = NULL;
    // if the width or height is not positive this should be treated as an incomplete texture
    // we handle that here by skipping the d3d texture creation
    if (width > 0 && height > 0)
    {
        IDirect3DDevice9 *device = mRenderer->getDevice();
        gl::MakeValidSize(false, gl::IsCompressed(internalformat), &width, &height, &mLodOffset);
        HRESULT result = device->CreateTexture(width, height, levels ? levels + mLodOffset : 0, getUsage(),
                                               mRenderer->ConvertTextureInternalFormat(internalformat), getPool(), &mTexture, NULL);

        if (FAILED(result))
        {
            ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
            gl::error(GL_OUT_OF_MEMORY);
        }
    }

    initializeRenderTarget();
}

TextureStorage9_2D::~TextureStorage9_2D()
{
    if (mTexture)
    {
        mTexture->Release();
    }

    delete mRenderTarget;
}

TextureStorage9_2D *TextureStorage9_2D::makeTextureStorage9_2D(TextureStorage *storage)
{
    ASSERT(HAS_DYNAMIC_TYPE(TextureStorage9_2D*, storage));
    return static_cast<TextureStorage9_2D*>(storage);
}

// Increments refcount on surface.
// caller must Release() the returned surface
IDirect3DSurface9 *TextureStorage9_2D::getSurfaceLevel(int level, bool dirty)
{
    IDirect3DSurface9 *surface = NULL;

    if (mTexture)
    {
        HRESULT result = mTexture->GetSurfaceLevel(level + mLodOffset, &surface);
        ASSERT(SUCCEEDED(result));

        // With managed textures the driver needs to be informed of updates to the lower mipmap levels
        if (level + mLodOffset != 0 && isManaged() && dirty)
        {
            mTexture->AddDirtyRect(NULL);
        }
    }

    return surface;
}

RenderTarget *TextureStorage9_2D::getRenderTarget()
{
    return mRenderTarget;
}

void TextureStorage9_2D::generateMipmap(int level)
{
    IDirect3DSurface9 *upper = getSurfaceLevel(level - 1, false);
    IDirect3DSurface9 *lower = getSurfaceLevel(level, true);

    if (upper != NULL && lower != NULL)
    {
        mRenderer->boxFilter(upper, lower);
    }

    if (upper != NULL) upper->Release();
    if (lower != NULL) lower->Release();
}

IDirect3DBaseTexture9 *TextureStorage9_2D::getBaseTexture() const
{
    return mTexture;
}

void TextureStorage9_2D::initializeRenderTarget()
{
    ASSERT(mRenderTarget == NULL);

    if (mTexture != NULL && isRenderTarget())
    {
        IDirect3DSurface9 *surface = getSurfaceLevel(0, false);

        mRenderTarget = new RenderTarget9(mRenderer, surface);
    }
}

TextureStorage9_Cube::TextureStorage9_Cube(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, int size)
    : TextureStorage9(renderer, GetTextureUsage(Renderer9::makeRenderer9(renderer)->ConvertTextureInternalFormat(internalformat), usage, forceRenderable))
{
    mTexture = NULL;
    for (int i = 0; i < 6; ++i)
    {
        mRenderTarget[i] = NULL;
    }

    // if the size is not positive this should be treated as an incomplete texture
    // we handle that here by skipping the d3d texture creation
    if (size > 0)
    {
        IDirect3DDevice9 *device = mRenderer->getDevice();
        int height = size;
        gl::MakeValidSize(false, gl::IsCompressed(internalformat), &size, &height, &mLodOffset);
        HRESULT result = device->CreateCubeTexture(size, levels ? levels + mLodOffset : 0, getUsage(),
                                                   mRenderer->ConvertTextureInternalFormat(internalformat), getPool(), &mTexture, NULL);

        if (FAILED(result))
        {
            ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
            gl::error(GL_OUT_OF_MEMORY);
        }
    }

    initializeRenderTarget();
}

TextureStorage9_Cube::~TextureStorage9_Cube()
{
    if (mTexture)
    {
        mTexture->Release();
    }

    for (int i = 0; i < 6; ++i)
    {
        delete mRenderTarget[i];
    }
}

TextureStorage9_Cube *TextureStorage9_Cube::makeTextureStorage9_Cube(TextureStorage *storage)
{
    ASSERT(HAS_DYNAMIC_TYPE(TextureStorage9_Cube*, storage));
    return static_cast<TextureStorage9_Cube*>(storage);
}

// Increments refcount on surface.
// caller must Release() the returned surface
IDirect3DSurface9 *TextureStorage9_Cube::getCubeMapSurface(GLenum faceTarget, int level, bool dirty)
{
    IDirect3DSurface9 *surface = NULL;

    if (mTexture)
    {
        D3DCUBEMAP_FACES face = gl_d3d9::ConvertCubeFace(faceTarget);
        HRESULT result = mTexture->GetCubeMapSurface(face, level + mLodOffset, &surface);
        ASSERT(SUCCEEDED(result));

        // With managed textures the driver needs to be informed of updates to the lower mipmap levels
        if (level != 0 && isManaged() && dirty)
        {
            mTexture->AddDirtyRect(face, NULL);
        }
    }

    return surface;
}

RenderTarget *TextureStorage9_Cube::getRenderTarget(GLenum faceTarget)
{
    return mRenderTarget[gl::TextureCubeMap::faceIndex(faceTarget)];
}

void TextureStorage9_Cube::generateMipmap(int face, int level)
{
    IDirect3DSurface9 *upper = getCubeMapSurface(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level - 1, false);
    IDirect3DSurface9 *lower = getCubeMapSurface(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, true);

    if (upper != NULL && lower != NULL)
    {
        mRenderer->boxFilter(upper, lower);
    }

    if (upper != NULL) upper->Release();
    if (lower != NULL) lower->Release();
}

IDirect3DBaseTexture9 *TextureStorage9_Cube::getBaseTexture() const
{
    return mTexture;
}

void TextureStorage9_Cube::initializeRenderTarget()
{
    if (mTexture != NULL && isRenderTarget())
    {
        IDirect3DSurface9 *surface = NULL;

        for (int i = 0; i < 6; ++i)
        {
            ASSERT(mRenderTarget[i] == NULL);

            surface = getCubeMapSurface(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, false);

            mRenderTarget[i] = new RenderTarget9(mRenderer, surface);
        }
    }
}

}