//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Image11.h: Defines the rx::Image11 class, which acts as the interface to
// the actual underlying resources of a Texture

#ifndef LIBGLESV2_RENDERER_IMAGE11_H_
#define LIBGLESV2_RENDERER_IMAGE11_H_

#include "libGLESv2/renderer/d3d/ImageD3D.h"
#include "libGLESv2/ImageIndex.h"

#include "common/debug.h"

namespace gl
{
class Framebuffer;
}

namespace rx
{
class Renderer11;
class TextureStorage11;

class Image11 : public ImageD3D
{
  public:
    Image11();
    virtual ~Image11();

    static Image11 *makeImage11(Image *img);

    static gl::Error generateMipmap(Image11 *dest, Image11 *src);

    virtual bool isDirty() const;

    virtual gl::Error copyToStorage(TextureStorage *storage, const gl::ImageIndex &index, const gl::Box &region);

    bool redefine(RendererD3D *renderer, GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, bool forceRelease) override;

    DXGI_FORMAT getDXGIFormat() const;

    virtual gl::Error loadData(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                               GLint unpackAlignment, GLenum type, const void *input);
    virtual gl::Error loadCompressedData(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                                         const void *input);

    virtual gl::Error copy(GLint xoffset, GLint yoffset, GLint zoffset, const gl::Rectangle &sourceArea, RenderTarget *source);
    virtual gl::Error copy(GLint xoffset, GLint yoffset, GLint zoffset, const gl::Rectangle &sourceArea,
                           const gl::ImageIndex &sourceIndex, TextureStorage *source);

    gl::Error recoverFromAssociatedStorage();
    bool isAssociatedStorageValid(TextureStorage11* textureStorage) const;
    void disassociateStorage();

  protected:
    gl::Error map(D3D11_MAP mapType, D3D11_MAPPED_SUBRESOURCE *map);
    void unmap();

  private:
    DISALLOW_COPY_AND_ASSIGN(Image11);

    gl::Error copyToStorageImpl(TextureStorage11 *storage11, const gl::ImageIndex &index, const gl::Box &region);
    gl::Error copy(GLint xoffset, GLint yoffset, GLint zoffset, const gl::Rectangle &sourceArea, ID3D11Texture2D *source, UINT sourceSubResource);

    gl::Error getStagingTexture(ID3D11Resource **outStagingTexture, unsigned int *outSubresourceIndex);
    gl::Error createStagingTexture();
    void releaseStagingTexture();

    Renderer11 *mRenderer;

    DXGI_FORMAT mDXGIFormat;
    ID3D11Resource *mStagingTexture;
    unsigned int mStagingSubresource;

    bool mRecoverFromStorage;
    TextureStorage11 *mAssociatedStorage;
    gl::ImageIndex mAssociatedImageIndex;
    unsigned int mRecoveredFromStorageCount;
};

}

#endif // LIBGLESV2_RENDERER_IMAGE11_H_
