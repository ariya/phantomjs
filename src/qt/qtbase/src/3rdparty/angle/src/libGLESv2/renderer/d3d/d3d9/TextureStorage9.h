//
// Copyright (c) 2012-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureStorage9.h: Defines the abstract rx::TextureStorage9 class and its concrete derived
// classes TextureStorage9_2D and TextureStorage9_Cube, which act as the interface to the
// D3D9 texture.

#ifndef LIBGLESV2_RENDERER_TEXTURESTORAGE9_H_
#define LIBGLESV2_RENDERER_TEXTURESTORAGE9_H_

#include "libGLESv2/renderer/d3d/TextureStorage.h"
#include "common/debug.h"

namespace rx
{
class Renderer9;
class SwapChain9;
class RenderTarget;
class RenderTarget9;

class TextureStorage9 : public TextureStorage
{
  public:
    virtual ~TextureStorage9();

    static TextureStorage9 *makeTextureStorage9(TextureStorage *storage);

    static DWORD GetTextureUsage(GLenum internalformat, bool renderTarget);

    D3DPOOL getPool() const;
    DWORD getUsage() const;

    virtual gl::Error getBaseTexture(IDirect3DBaseTexture9 **outTexture) = 0;
    virtual gl::Error getRenderTarget(const gl::ImageIndex &index, RenderTarget **outRT) = 0;

    virtual int getTopLevel() const;
    virtual bool isRenderTarget() const;
    virtual bool isManaged() const;
    virtual int getLevelCount() const;

    virtual gl::Error setData(const gl::ImageIndex &index, Image *image, const gl::Box *destBox, GLenum type,
                              const gl::PixelUnpackState &unpack, const uint8_t *pixelData);

  protected:
    int mTopLevel;
    size_t mMipLevels;
    size_t mTextureWidth;
    size_t mTextureHeight;
    GLenum mInternalFormat;
    D3DFORMAT mTextureFormat;

    Renderer9 *mRenderer;

    TextureStorage9(Renderer9 *renderer, DWORD usage);

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorage9);

    const DWORD mD3DUsage;
    const D3DPOOL mD3DPool;
};

class TextureStorage9_2D : public TextureStorage9
{
  public:
    TextureStorage9_2D(Renderer9 *renderer, SwapChain9 *swapchain);
    TextureStorage9_2D(Renderer9 *renderer, GLenum internalformat, bool renderTarget, GLsizei width, GLsizei height, int levels);
    virtual ~TextureStorage9_2D();

    static TextureStorage9_2D *makeTextureStorage9_2D(TextureStorage *storage);

    gl::Error getSurfaceLevel(int level, bool dirty, IDirect3DSurface9 **outSurface);
    virtual gl::Error getRenderTarget(const gl::ImageIndex &index, RenderTarget **outRT);
    virtual gl::Error getBaseTexture(IDirect3DBaseTexture9 **outTexture);
    virtual gl::Error generateMipmap(const gl::ImageIndex &sourceIndex, const gl::ImageIndex &destIndex);
    virtual gl::Error copyToStorage(TextureStorage *destStorage);

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorage9_2D);

    IDirect3DTexture9 *mTexture;
    RenderTarget9 *mRenderTarget;
};

class TextureStorage9_Cube : public TextureStorage9
{
  public:
    TextureStorage9_Cube(Renderer9 *renderer, GLenum internalformat, bool renderTarget, int size, int levels);
    virtual ~TextureStorage9_Cube();

    static TextureStorage9_Cube *makeTextureStorage9_Cube(TextureStorage *storage);

    gl::Error getCubeMapSurface(GLenum faceTarget, int level, bool dirty, IDirect3DSurface9 **outSurface);
    virtual gl::Error getRenderTarget(const gl::ImageIndex &index, RenderTarget **outRT);
    virtual gl::Error getBaseTexture(IDirect3DBaseTexture9 **outTexture);
    virtual gl::Error generateMipmap(const gl::ImageIndex &sourceIndex, const gl::ImageIndex &destIndex);
    virtual gl::Error copyToStorage(TextureStorage *destStorage);

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorage9_Cube);

    static const size_t CUBE_FACE_COUNT = 6;

    IDirect3DCubeTexture9 *mTexture;
    RenderTarget9 *mRenderTarget[CUBE_FACE_COUNT];
};

}

#endif // LIBGLESV2_RENDERER_TEXTURESTORAGE9_H_

