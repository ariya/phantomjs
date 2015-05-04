//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureD3D.h: Implementations of the Texture interfaces shared betweeen the D3D backends.

#ifndef LIBGLESV2_RENDERER_TEXTURED3D_H_
#define LIBGLESV2_RENDERER_TEXTURED3D_H_

#include "libGLESv2/renderer/TextureImpl.h"
#include "libGLESv2/angletypes.h"
#include "libGLESv2/Constants.h"

namespace gl
{
class Framebuffer;
}

namespace rx
{

class Image;
class ImageD3D;
class RendererD3D;
class RenderTarget;
class TextureStorage;

class TextureD3D : public TextureImpl
{
  public:
    TextureD3D(RendererD3D *renderer);
    virtual ~TextureD3D();

    static TextureD3D *makeTextureD3D(TextureImpl *texture);

    TextureStorage *getNativeTexture();

    virtual void setUsage(GLenum usage) { mUsage = usage; }
    bool hasDirtyImages() const { return mDirtyImages; }
    void resetDirty() { mDirtyImages = false; }

    GLint getBaseLevelWidth() const;
    GLint getBaseLevelHeight() const;
    GLint getBaseLevelDepth() const;
    GLenum getBaseLevelInternalFormat() const;

    bool isImmutable() const { return mImmutable; }

    virtual gl::Error getRenderTarget(const gl::ImageIndex &index, RenderTarget **outRT) = 0;
    virtual unsigned int getRenderTargetSerial(const gl::ImageIndex &index) = 0;

    // Returns an iterator over all "Images" for this particular Texture.
    virtual gl::ImageIndexIterator imageIterator() const = 0;

    // Returns an ImageIndex for a particular "Image". 3D Textures do not have images for
    // slices of their depth texures, so 3D textures ignore the layer parameter.
    virtual gl::ImageIndex getImageIndex(GLint mip, GLint layer) const = 0;
    virtual bool isValidIndex(const gl::ImageIndex &index) const = 0;

    virtual gl::Error generateMipmaps();
    TextureStorage *getStorage();
    Image *getBaseLevelImage() const;

  protected:
    gl::Error setImage(const gl::PixelUnpackState &unpack, GLenum type, const void *pixels, const gl::ImageIndex &index);
    gl::Error subImage(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                       GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels, const gl::ImageIndex &index);
    gl::Error setCompressedImage(const gl::PixelUnpackState &unpack, GLsizei imageSize, const void *pixels, Image *image);
    gl::Error subImageCompressed(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                                 GLenum format, GLsizei imageSize, const gl::PixelUnpackState &unpack, const void *pixels, Image *image);
    bool isFastUnpackable(const gl::PixelUnpackState &unpack, GLenum sizedInternalFormat);
    gl::Error fastUnpackPixels(const gl::PixelUnpackState &unpack, const void *pixels, const gl::Box &destArea,
                               GLenum sizedInternalFormat, GLenum type, RenderTarget *destRenderTarget);

    GLint creationLevels(GLsizei width, GLsizei height, GLsizei depth) const;
    int mipLevels() const;
    virtual void initMipmapsImages() = 0;
    bool isBaseImageZeroSize() const;
    virtual bool isImageComplete(const gl::ImageIndex &index) const = 0;

    bool canCreateRenderTargetForImage(const gl::ImageIndex &index) const;
    virtual gl::Error ensureRenderTarget();

    virtual gl::Error createCompleteStorage(bool renderTarget, TextureStorage **outTexStorage) const = 0;
    virtual gl::Error setCompleteTexStorage(TextureStorage *newCompleteTexStorage) = 0;
    gl::Error commitRegion(const gl::ImageIndex &index, const gl::Box &region);

    RendererD3D *mRenderer;

    GLenum mUsage;

    bool mDirtyImages;

    bool mImmutable;
    TextureStorage *mTexStorage;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureD3D);

    virtual gl::Error initializeStorage(bool renderTarget) = 0;

    virtual gl::Error updateStorage() = 0;

    bool shouldUseSetData(const Image *image) const;
};

class TextureD3D_2D : public TextureD3D
{
  public:
    TextureD3D_2D(RendererD3D *renderer);
    virtual ~TextureD3D_2D();

    virtual Image *getImage(int level, int layer) const;
    virtual Image *getImage(const gl::ImageIndex &index) const;
    virtual GLsizei getLayerCount(int level) const;

    GLsizei getWidth(GLint level) const;
    GLsizei getHeight(GLint level) const;
    GLenum getInternalFormat(GLint level) const;
    GLenum getActualFormat(GLint level) const;
    bool isDepth(GLint level) const;

    virtual gl::Error setImage(GLenum target, GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual gl::Error setCompressedImage(GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual gl::Error subImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual gl::Error subImageCompressed(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual gl::Error copyImage(GLenum target, GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source);
    virtual gl::Error copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source);
    virtual gl::Error storage(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);

    virtual void bindTexImage(egl::Surface *surface);
    virtual void releaseTexImage();

    virtual gl::Error getRenderTarget(const gl::ImageIndex &index, RenderTarget **outRT);
    virtual unsigned int getRenderTargetSerial(const gl::ImageIndex &index);

    virtual gl::ImageIndexIterator imageIterator() const;
    virtual gl::ImageIndex getImageIndex(GLint mip, GLint layer) const;
    virtual bool isValidIndex(const gl::ImageIndex &index) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureD3D_2D);

    virtual gl::Error initializeStorage(bool renderTarget);
    virtual gl::Error createCompleteStorage(bool renderTarget, TextureStorage **outTexStorage) const;
    virtual gl::Error setCompleteTexStorage(TextureStorage *newCompleteTexStorage);

    virtual gl::Error updateStorage();
    virtual void initMipmapsImages();

    bool isValidLevel(int level) const;
    bool isLevelComplete(int level) const;
    virtual bool isImageComplete(const gl::ImageIndex &index) const;

    gl::Error updateStorageLevel(int level);

    void redefineImage(GLint level, GLenum internalformat, GLsizei width, GLsizei height);

    ImageD3D *mImageArray[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];
};

class TextureD3D_Cube : public TextureD3D
{
  public:
    TextureD3D_Cube(RendererD3D *renderer);
    virtual ~TextureD3D_Cube();

    virtual Image *getImage(int level, int layer) const;
    virtual Image *getImage(const gl::ImageIndex &index) const;
    virtual GLsizei getLayerCount(int level) const;

    virtual bool hasDirtyImages() const { return mDirtyImages; }
    virtual void resetDirty() { mDirtyImages = false; }
    virtual void setUsage(GLenum usage) { mUsage = usage; }

    GLenum getInternalFormat(GLint level, GLint layer) const;
    bool isDepth(GLint level, GLint layer) const;

    virtual gl::Error setImage(GLenum target, GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual gl::Error setCompressedImage(GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual gl::Error subImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual gl::Error subImageCompressed(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual gl::Error copyImage(GLenum target, GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source);
    virtual gl::Error copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source);
    virtual gl::Error storage(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);

    virtual void bindTexImage(egl::Surface *surface);
    virtual void releaseTexImage();

    virtual gl::Error getRenderTarget(const gl::ImageIndex &index, RenderTarget **outRT);
    virtual unsigned int getRenderTargetSerial(const gl::ImageIndex &index);

    virtual gl::ImageIndexIterator imageIterator() const;
    virtual gl::ImageIndex getImageIndex(GLint mip, GLint layer) const;
    virtual bool isValidIndex(const gl::ImageIndex &index) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureD3D_Cube);

    virtual gl::Error initializeStorage(bool renderTarget);
    virtual gl::Error createCompleteStorage(bool renderTarget, TextureStorage **outTexStorage) const;
    virtual gl::Error setCompleteTexStorage(TextureStorage *newCompleteTexStorage);

    virtual gl::Error updateStorage();
    virtual void initMipmapsImages();

    bool isValidFaceLevel(int faceIndex, int level) const;
    bool isFaceLevelComplete(int faceIndex, int level) const;
    bool isCubeComplete() const;
    virtual bool isImageComplete(const gl::ImageIndex &index) const;
    gl::Error updateStorageFaceLevel(int faceIndex, int level);

    void redefineImage(int faceIndex, GLint level, GLenum internalformat, GLsizei width, GLsizei height);

    ImageD3D *mImageArray[6][gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];
};

class TextureD3D_3D : public TextureD3D
{
  public:
    TextureD3D_3D(RendererD3D *renderer);
    virtual ~TextureD3D_3D();

    virtual Image *getImage(int level, int layer) const;
    virtual Image *getImage(const gl::ImageIndex &index) const;
    virtual GLsizei getLayerCount(int level) const;

    GLsizei getWidth(GLint level) const;
    GLsizei getHeight(GLint level) const;
    GLsizei getDepth(GLint level) const;
    GLenum getInternalFormat(GLint level) const;
    bool isDepth(GLint level) const;

    virtual gl::Error setImage(GLenum target, GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual gl::Error setCompressedImage(GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual gl::Error subImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual gl::Error subImageCompressed(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual gl::Error copyImage(GLenum target, GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source);
    virtual gl::Error copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source);
    virtual gl::Error storage(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);

    virtual void bindTexImage(egl::Surface *surface);
    virtual void releaseTexImage();

    virtual gl::Error getRenderTarget(const gl::ImageIndex &index, RenderTarget **outRT);
    virtual unsigned int getRenderTargetSerial(const gl::ImageIndex &index);

    virtual gl::ImageIndexIterator imageIterator() const;
    virtual gl::ImageIndex getImageIndex(GLint mip, GLint layer) const;
    virtual bool isValidIndex(const gl::ImageIndex &index) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureD3D_3D);

    virtual gl::Error initializeStorage(bool renderTarget);
    virtual gl::Error createCompleteStorage(bool renderTarget, TextureStorage **outStorage) const;
    virtual gl::Error setCompleteTexStorage(TextureStorage *newCompleteTexStorage);

    virtual gl::Error updateStorage();
    virtual void initMipmapsImages();

    bool isValidLevel(int level) const;
    bool isLevelComplete(int level) const;
    virtual bool isImageComplete(const gl::ImageIndex &index) const;
    gl::Error updateStorageLevel(int level);

    void redefineImage(GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);

    ImageD3D *mImageArray[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];
};

class TextureD3D_2DArray : public TextureD3D
{
  public:
    TextureD3D_2DArray(RendererD3D *renderer);
    virtual ~TextureD3D_2DArray();

    virtual Image *getImage(int level, int layer) const;
    virtual Image *getImage(const gl::ImageIndex &index) const;
    virtual GLsizei getLayerCount(int level) const;

    GLsizei getWidth(GLint level) const;
    GLsizei getHeight(GLint level) const;
    GLenum getInternalFormat(GLint level) const;
    bool isDepth(GLint level) const;

    virtual gl::Error setImage(GLenum target, GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual gl::Error setCompressedImage(GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual gl::Error subImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual gl::Error subImageCompressed(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual gl::Error copyImage(GLenum target, GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source);
    virtual gl::Error copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source);
    virtual gl::Error storage(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);

    virtual void bindTexImage(egl::Surface *surface);
    virtual void releaseTexImage();

    virtual gl::Error getRenderTarget(const gl::ImageIndex &index, RenderTarget **outRT);
    virtual unsigned int getRenderTargetSerial(const gl::ImageIndex &index);

    virtual gl::ImageIndexIterator imageIterator() const;
    virtual gl::ImageIndex getImageIndex(GLint mip, GLint layer) const;
    virtual bool isValidIndex(const gl::ImageIndex &index) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureD3D_2DArray);

    virtual gl::Error initializeStorage(bool renderTarget);
    virtual gl::Error createCompleteStorage(bool renderTarget, TextureStorage **outStorage) const;
    virtual gl::Error setCompleteTexStorage(TextureStorage *newCompleteTexStorage);

    virtual gl::Error updateStorage();
    virtual void initMipmapsImages();

    bool isValidLevel(int level) const;
    bool isLevelComplete(int level) const;
    virtual bool isImageComplete(const gl::ImageIndex &index) const;
    gl::Error updateStorageLevel(int level);

    void deleteImages();
    void redefineImage(GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);

    // Storing images as an array of single depth textures since D3D11 treats each array level of a
    // Texture2D object as a separate subresource.  Each layer would have to be looped over
    // to update all the texture layers since they cannot all be updated at once and it makes the most
    // sense for the Image class to not have to worry about layer subresource as well as mip subresources.
    GLsizei mLayerCounts[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];
    ImageD3D **mImageArray[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];
};

}

#endif // LIBGLESV2_RENDERER_TEXTURED3D_H_
