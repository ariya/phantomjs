//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Image11.h: Defines the rx::Image11 class, which acts as the interface to
// the actual underlying resources of a Texture

#ifndef LIBGLESV2_RENDERER_IMAGE11_H_
#define LIBGLESV2_RENDERER_IMAGE11_H_

#include "libGLESv2/renderer/Image.h"

#include "common/debug.h"

namespace gl
{
class Framebuffer;
}

namespace rx
{
class Renderer;
class Renderer11;
class TextureStorageInterface2D;
class TextureStorageInterfaceCube;

class Image11 : public Image
{
  public:
    Image11();
    virtual ~Image11();

    static Image11 *makeImage11(Image *img);

    static void generateMipmap(Image11 *dest, Image11 *src);

    virtual bool isDirty() const;

    virtual bool updateSurface(TextureStorageInterface2D *storage, int level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);
    virtual bool updateSurface(TextureStorageInterfaceCube *storage, int face, int level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);

    virtual bool redefine(Renderer *renderer, GLint internalformat, GLsizei width, GLsizei height, bool forceRelease);

    virtual bool isRenderableFormat() const;
    DXGI_FORMAT getDXGIFormat() const;
    
    virtual void loadData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                  GLint unpackAlignment, const void *input);
    virtual void loadCompressedData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                    const void *input);

    virtual void copy(GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source);

  protected:
    HRESULT map(D3D11_MAPPED_SUBRESOURCE *map);
    void unmap();

  private:
    DISALLOW_COPY_AND_ASSIGN(Image11);

    ID3D11Texture2D *getStagingTexture();
    unsigned int getStagingSubresource();
    void createStagingTexture();

    Renderer11 *mRenderer;

    DXGI_FORMAT mDXGIFormat;
    ID3D11Texture2D *mStagingTexture;
    unsigned int mStagingSubresource;
};

}

#endif // LIBGLESV2_RENDERER_IMAGE11_H_
