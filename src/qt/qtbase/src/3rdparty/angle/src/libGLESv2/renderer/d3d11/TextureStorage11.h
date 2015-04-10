//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureStorage11.h: Defines the abstract rx::TextureStorage11 class and its concrete derived
// classes TextureStorage11_2D and TextureStorage11_Cube, which act as the interface to the D3D11 texture.

#ifndef LIBGLESV2_RENDERER_TEXTURESTORAGE11_H_
#define LIBGLESV2_RENDERER_TEXTURESTORAGE11_H_

#include "libGLESv2/Texture.h"
#include "libGLESv2/renderer/TextureStorage.h"

namespace rx
{
class RenderTarget;
class RenderTarget11;
class Renderer;
class Renderer11;
class SwapChain11;

class TextureStorage11 : public TextureStorage
{
  public:
    TextureStorage11(Renderer *renderer, UINT bindFlags);
    virtual ~TextureStorage11();

    static TextureStorage11 *makeTextureStorage11(TextureStorage *storage);

    static DWORD GetTextureBindFlags(DXGI_FORMAT d3dfmt, GLenum glusage, bool forceRenderable);
    static bool IsTextureFormatRenderable(DXGI_FORMAT format);

    UINT getBindFlags() const;

    virtual ID3D11Texture2D *getBaseTexture() const;
    virtual ID3D11ShaderResourceView *getSRV() = 0;
    virtual RenderTarget *getRenderTarget() { return getRenderTarget(0); }
    virtual RenderTarget *getRenderTarget(int level) { return NULL; }
    virtual RenderTarget *getRenderTarget(GLenum faceTarget) { return getRenderTarget(faceTarget, 0); }
    virtual RenderTarget *getRenderTarget(GLenum faceTarget, int level) { return NULL; }

    virtual void generateMipmap(int level) {};
    virtual void generateMipmap(int face, int level) {};

    virtual int getLodOffset() const;
    virtual bool isRenderTarget() const;
    virtual bool isManaged() const;
    virtual int levelCount();
    UINT getSubresourceIndex(int level, int faceTarget);

    bool updateSubresourceLevel(ID3D11Texture2D *texture, unsigned int sourceSubresource, int level,
                                int faceTarget, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);

  protected:
    void generateMipmapLayer(RenderTarget11 *source, RenderTarget11 *dest);

    Renderer11 *mRenderer;
    int mLodOffset;
    unsigned int mMipLevels;

    ID3D11Texture2D *mTexture;
    DXGI_FORMAT mTextureFormat;
    DXGI_FORMAT mShaderResourceFormat;
    DXGI_FORMAT mRenderTargetFormat;
    DXGI_FORMAT mDepthStencilFormat;
    unsigned int mTextureWidth;
    unsigned int mTextureHeight;

    ID3D11ShaderResourceView *mSRV;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorage11);

    const UINT mBindFlags;
};

class TextureStorage11_2D : public TextureStorage11
{
  public:
    TextureStorage11_2D(Renderer *renderer, SwapChain11 *swapchain);
    TextureStorage11_2D(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, GLsizei width, GLsizei height);
    virtual ~TextureStorage11_2D();

    static TextureStorage11_2D *makeTextureStorage11_2D(TextureStorage *storage);

    virtual ID3D11ShaderResourceView *getSRV();
    virtual RenderTarget *getRenderTarget(int level);

    virtual void generateMipmap(int level);

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorage11_2D);

    RenderTarget11 *mRenderTarget[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];
};

class TextureStorage11_Cube : public TextureStorage11
{
  public:
    TextureStorage11_Cube(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, int size);
    virtual ~TextureStorage11_Cube();

    static TextureStorage11_Cube *makeTextureStorage11_Cube(TextureStorage *storage);

    virtual ID3D11ShaderResourceView *getSRV();
    virtual RenderTarget *getRenderTarget(GLenum faceTarget, int level);

    virtual void generateMipmap(int face, int level);

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorage11_Cube);

    RenderTarget11 *mRenderTarget[6][gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];
};

}

#endif // LIBGLESV2_RENDERER_TEXTURESTORAGE11_H_
