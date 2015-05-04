//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Texture.h: Defines the abstract gl::Texture class and its concrete derived
// classes Texture2D and TextureCubeMap. Implements GL texture objects and
// related functionality. [OpenGL ES 2.0.24] section 3.7 page 63.

#ifndef LIBGLESV2_TEXTURE_H_
#define LIBGLESV2_TEXTURE_H_

#include "common/debug.h"
#include "common/RefCountObject.h"
#include "libGLESv2/angletypes.h"
#include "libGLESv2/Constants.h"
#include "libGLESv2/renderer/TextureImpl.h"
#include "libGLESv2/Caps.h"

#include "angle_gl.h"

#include <vector>

namespace egl
{
class Surface;
}

namespace rx
{
class TextureStorageInterface;
class Image;
}

namespace gl
{
class Framebuffer;
class FramebufferAttachment;
struct ImageIndex;

bool IsMipmapFiltered(const gl::SamplerState &samplerState);

class Texture : public RefCountObject
{
  public:
    Texture(rx::TextureImpl *impl, GLuint id, GLenum target);

    virtual ~Texture();

    GLenum getTarget() const;

    const SamplerState &getSamplerState() const { return mSamplerState; }
    SamplerState &getSamplerState() { return mSamplerState; }

    void setUsage(GLenum usage);
    GLenum getUsage() const;

    GLint getBaseLevelWidth() const;
    GLint getBaseLevelHeight() const;
    GLint getBaseLevelDepth() const;
    GLenum getBaseLevelInternalFormat() const;

    GLsizei getWidth(const ImageIndex &index) const;
    GLsizei getHeight(const ImageIndex &index) const;
    GLenum getInternalFormat(const ImageIndex &index) const;
    GLenum getActualFormat(const ImageIndex &index) const;

    virtual bool isSamplerComplete(const SamplerState &samplerState, const TextureCapsMap &textureCaps, const Extensions &extensions, int clientVersion) const = 0;

    virtual Error generateMipmaps();

    virtual Error copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source);

    // Texture serials provide a unique way of identifying a Texture that isn't a raw pointer.
    // "id" is not good enough, as Textures can be deleted, then re-allocated with the same id.
    unsigned int getTextureSerial() const;

    bool isImmutable() const;
    GLsizei immutableLevelCount();

    rx::TextureImpl *getImplementation() { return mTexture; }
    const rx::TextureImpl *getImplementation() const { return mTexture; }

    static const GLuint INCOMPLETE_TEXTURE_ID = static_cast<GLuint>(-1);   // Every texture takes an id at creation time. The value is arbitrary because it is never registered with the resource manager.

  protected:
    int mipLevels() const;
    const rx::Image *getBaseLevelImage() const;
    static unsigned int issueTextureSerial();

    rx::TextureImpl *mTexture;

    SamplerState mSamplerState;
    GLenum mUsage;

    GLsizei mImmutableLevelCount;

    GLenum mTarget;

    const unsigned int mTextureSerial;
    static unsigned int mCurrentTextureSerial;

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture);
};

class Texture2D : public Texture
{
  public:
    Texture2D(rx::TextureImpl *impl, GLuint id);

    virtual ~Texture2D();

    GLsizei getWidth(GLint level) const;
    GLsizei getHeight(GLint level) const;
    GLenum getInternalFormat(GLint level) const;
    GLenum getActualFormat(GLint level) const;
    bool isCompressed(GLint level) const;
    bool isDepth(GLint level) const;

    Error setImage(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels);
    Error setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const PixelUnpackState &unpack, const void *pixels);
    Error subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels);
    Error subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const PixelUnpackState &unpack, const void *pixels);
    Error copyImage(GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source);
    Error storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);

    virtual bool isSamplerComplete(const SamplerState &samplerState, const TextureCapsMap &textureCaps, const Extensions &extensions, int clientVersion) const;
    virtual void bindTexImage(egl::Surface *surface);
    virtual void releaseTexImage();

    virtual Error generateMipmaps();

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture2D);

    bool isMipmapComplete() const;
    bool isLevelComplete(int level) const;

    egl::Surface *mSurface;
};

class TextureCubeMap : public Texture
{
  public:
    TextureCubeMap(rx::TextureImpl *impl, GLuint id);

    virtual ~TextureCubeMap();

    GLsizei getWidth(GLenum target, GLint level) const;
    GLsizei getHeight(GLenum target, GLint level) const;
    GLenum getInternalFormat(GLenum target, GLint level) const;
    GLenum getActualFormat(GLenum target, GLint level) const;
    bool isCompressed(GLenum target, GLint level) const;
    bool isDepth(GLenum target, GLint level) const;

    Error setImage(GLenum target, GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels);
    Error setCompressedImage(GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const PixelUnpackState &unpack, const void *pixels);
    Error subImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels);
    Error subImageCompressed(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const PixelUnpackState &unpack, const void *pixels);
    Error copyImage(GLenum target, GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source);
    Error storage(GLsizei levels, GLenum internalformat, GLsizei size);

    virtual bool isSamplerComplete(const SamplerState &samplerState, const TextureCapsMap &textureCaps, const Extensions &extensions, int clientVersion) const;

    bool isCubeComplete() const;

    static int targetToLayerIndex(GLenum target);
    static GLenum layerIndexToTarget(GLint layer);

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureCubeMap);

    bool isMipmapComplete() const;
    bool isFaceLevelComplete(int faceIndex, int level) const;
};

class Texture3D : public Texture
{
  public:
    Texture3D(rx::TextureImpl *impl, GLuint id);

    virtual ~Texture3D();

    GLsizei getWidth(GLint level) const;
    GLsizei getHeight(GLint level) const;
    GLsizei getDepth(GLint level) const;
    GLenum getInternalFormat(GLint level) const;
    GLenum getActualFormat(GLint level) const;
    bool isCompressed(GLint level) const;
    bool isDepth(GLint level) const;

    Error setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels);
    Error setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const PixelUnpackState &unpack, const void *pixels);
    Error subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels);
    Error subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const PixelUnpackState &unpack, const void *pixels);
    Error storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);

    virtual bool isSamplerComplete(const SamplerState &samplerState, const TextureCapsMap &textureCaps, const Extensions &extensions, int clientVersion) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture3D);

    bool isMipmapComplete() const;
    bool isLevelComplete(int level) const;
};

class Texture2DArray : public Texture
{
  public:
    Texture2DArray(rx::TextureImpl *impl, GLuint id);

    virtual ~Texture2DArray();

    GLsizei getWidth(GLint level) const;
    GLsizei getHeight(GLint level) const;
    GLsizei getLayers(GLint level) const;
    GLenum getInternalFormat(GLint level) const;
    GLenum getActualFormat(GLint level) const;
    bool isCompressed(GLint level) const;
    bool isDepth(GLint level) const;

    Error setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels);
    Error setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const PixelUnpackState &unpack, const void *pixels);
    Error subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels);
    Error subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const PixelUnpackState &unpack, const void *pixels);
    Error storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);

    virtual bool isSamplerComplete(const SamplerState &samplerState, const TextureCapsMap &textureCaps, const Extensions &extensions, int clientVersion) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture2DArray);

    bool isMipmapComplete() const;
    bool isLevelComplete(int level) const;
};

}

#endif   // LIBGLESV2_TEXTURE_H_
