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

#include <vector>

#define GL_APICALL
#include <GLES2/gl2.h>

#include "common/debug.h"
#include "common/RefCountObject.h"
#include "libGLESv2/angletypes.h"

namespace egl
{
class Surface;
}

namespace rx
{
class Renderer;
class TextureStorageInterface;
class TextureStorageInterface2D;
class TextureStorageInterfaceCube;
class RenderTarget;
class Image;
}

namespace gl
{
class Framebuffer;
class Renderbuffer;

enum
{
    // These are the maximums the implementation can support
    // The actual GL caps are limited by the device caps
    // and should be queried from the Context
    IMPLEMENTATION_MAX_TEXTURE_SIZE = 16384,
    IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE = 16384,

    IMPLEMENTATION_MAX_TEXTURE_LEVELS = 15   // 1+log2 of MAX_TEXTURE_SIZE
};

class Texture : public RefCountObject
{
  public:
    Texture(rx::Renderer *renderer, GLuint id);

    virtual ~Texture();

    virtual void addProxyRef(const Renderbuffer *proxy) = 0;
    virtual void releaseProxy(const Renderbuffer *proxy) = 0;

    virtual GLenum getTarget() const = 0;

    bool setMinFilter(GLenum filter);
    bool setMagFilter(GLenum filter);
    bool setWrapS(GLenum wrap);
    bool setWrapT(GLenum wrap);
    bool setMaxAnisotropy(float textureMaxAnisotropy, float contextMaxAnisotropy);
    bool setUsage(GLenum usage);

    GLenum getMinFilter() const;
    GLenum getMagFilter() const;
    GLenum getWrapS() const;
    GLenum getWrapT() const;
    float getMaxAnisotropy() const;
    int getLodOffset();
    void getSamplerState(SamplerState *sampler);
    GLenum getUsage() const;
    bool isMipmapFiltered() const;

    virtual bool isSamplerComplete() const = 0;

    rx::TextureStorageInterface *getNativeTexture();
    virtual Renderbuffer *getRenderbuffer(GLenum target) = 0;

    virtual void generateMipmaps() = 0;
    virtual void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source) = 0;

    bool hasDirtyParameters() const;
    bool hasDirtyImages() const;
    void resetDirty();
    unsigned int getTextureSerial();
    unsigned int getRenderTargetSerial(GLenum target);

    bool isImmutable() const;

    static const GLuint INCOMPLETE_TEXTURE_ID = static_cast<GLuint>(-1);   // Every texture takes an id at creation time. The value is arbitrary because it is never registered with the resource manager.

  protected:
    void setImage(GLint unpackAlignment, const void *pixels, rx::Image *image);
    bool subImage(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels, rx::Image *image);
    void setCompressedImage(GLsizei imageSize, const void *pixels, rx::Image *image);
    bool subImageCompressed(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels, rx::Image *image);

    GLint creationLevels(GLsizei width, GLsizei height) const;
    GLint creationLevels(GLsizei size) const;

    virtual void createTexture() = 0;
    virtual void updateTexture() = 0;
    virtual void convertToRenderTarget() = 0;
    virtual rx::RenderTarget *getRenderTarget(GLenum target) = 0;

    virtual int levelCount() = 0;

    rx::Renderer *mRenderer;

    SamplerState mSamplerState;
    GLenum mUsage;

    bool mDirtyImages;

    bool mImmutable;

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture);

    virtual rx::TextureStorageInterface *getStorage(bool renderTarget) = 0;
};

class Texture2D : public Texture
{
  public:
    Texture2D(rx::Renderer *renderer, GLuint id);

    ~Texture2D();

    void addProxyRef(const Renderbuffer *proxy);
    void releaseProxy(const Renderbuffer *proxy);

    virtual GLenum getTarget() const;

    GLsizei getWidth(GLint level) const;
    GLsizei getHeight(GLint level) const;
    GLenum getInternalFormat(GLint level) const;
    GLenum getActualFormat(GLint level) const;
    bool isCompressed(GLint level) const;
    bool isDepth(GLint level) const;

    void setImage(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels);
    void subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels);
    void copyImage(GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source);
    virtual void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source);
    void storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);

    virtual bool isSamplerComplete() const;
    virtual void bindTexImage(egl::Surface *surface);
    virtual void releaseTexImage();

    virtual void generateMipmaps();

    virtual Renderbuffer *getRenderbuffer(GLenum target);

  protected:
    friend class RenderbufferTexture2D;
    virtual rx::RenderTarget *getRenderTarget(GLenum target);
    virtual rx::RenderTarget *getDepthStencil(GLenum target);
    virtual int levelCount();

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture2D);

    virtual void createTexture();
    virtual void updateTexture();
    virtual void convertToRenderTarget();
    virtual rx::TextureStorageInterface *getStorage(bool renderTarget);

    bool isMipmapComplete() const;

    void redefineImage(GLint level, GLint internalformat, GLsizei width, GLsizei height);
    void commitRect(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);

    rx::Image *mImageArray[IMPLEMENTATION_MAX_TEXTURE_LEVELS];

    rx::TextureStorageInterface2D *mTexStorage;
    egl::Surface *mSurface;

    // A specific internal reference count is kept for colorbuffer proxy references,
    // because, as the renderbuffer acting as proxy will maintain a binding pointer
    // back to this texture, there would be a circular reference if we used a binding
    // pointer here. This reference count will cause the pointer to be set to NULL if
    // the count drops to zero, but will not cause deletion of the Renderbuffer.
    Renderbuffer *mColorbufferProxy;
    unsigned int mProxyRefs;
};

class TextureCubeMap : public Texture
{
  public:
    TextureCubeMap(rx::Renderer *renderer, GLuint id);

    ~TextureCubeMap();

    void addProxyRef(const Renderbuffer *proxy);
    void releaseProxy(const Renderbuffer *proxy);

    virtual GLenum getTarget() const;
    
    GLsizei getWidth(GLenum target, GLint level) const;
    GLsizei getHeight(GLenum target, GLint level) const;
    GLenum getInternalFormat(GLenum target, GLint level) const;
    GLenum getActualFormat(GLenum target, GLint level) const;
    bool isCompressed(GLenum target, GLint level) const;

    void setImagePosX(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setImageNegX(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setImagePosY(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setImageNegY(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setImagePosZ(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setImageNegZ(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);

    void setCompressedImage(GLenum face, GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels);

    void subImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void subImageCompressed(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels);
    void copyImage(GLenum target, GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source);
    virtual void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source);
    void storage(GLsizei levels, GLenum internalformat, GLsizei size);

    virtual bool isSamplerComplete() const;

    virtual void generateMipmaps();

    virtual Renderbuffer *getRenderbuffer(GLenum target);

    static unsigned int faceIndex(GLenum face);

  protected:
    friend class RenderbufferTextureCubeMap;
    virtual rx::RenderTarget *getRenderTarget(GLenum target);
    virtual int levelCount();

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureCubeMap);

    virtual void createTexture();
    virtual void updateTexture();
    virtual void convertToRenderTarget();
    virtual rx::TextureStorageInterface *getStorage(bool renderTarget);

    bool isCubeComplete() const;
    bool isMipmapCubeComplete() const;

    void setImage(int faceIndex, GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void commitRect(int faceIndex, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);
    void redefineImage(int faceIndex, GLint level, GLint internalformat, GLsizei width, GLsizei height);

    rx::Image *mImageArray[6][IMPLEMENTATION_MAX_TEXTURE_LEVELS];

    rx::TextureStorageInterfaceCube *mTexStorage;

    // A specific internal reference count is kept for colorbuffer proxy references,
    // because, as the renderbuffer acting as proxy will maintain a binding pointer
    // back to this texture, there would be a circular reference if we used a binding
    // pointer here. This reference count will cause the pointer to be set to NULL if
    // the count drops to zero, but will not cause deletion of the Renderbuffer.
    Renderbuffer *mFaceProxies[6];
    unsigned int *mFaceProxyRefs[6];
};
}

#endif   // LIBGLESV2_TEXTURE_H_
