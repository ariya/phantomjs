//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderbuffer.h: Defines the wrapper class gl::Renderbuffer, as well as the
// class hierarchy used to store its contents: RenderbufferStorage, Colorbuffer,
// DepthStencilbuffer, Depthbuffer and Stencilbuffer. Implements GL renderbuffer
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4.3 page 108.

#ifndef LIBGLESV2_RENDERBUFFER_H_
#define LIBGLESV2_RENDERBUFFER_H_

#define GL_APICALL
#include <GLES2/gl2.h>

#include "common/angleutils.h"
#include "common/RefCountObject.h"

namespace rx
{
class Renderer;
class SwapChain;
class RenderTarget;
}

namespace gl
{
class Texture2D;
class TextureCubeMap;
class Renderbuffer;
class Colorbuffer;
class DepthStencilbuffer;

class RenderbufferInterface
{
  public:
    RenderbufferInterface();

    virtual ~RenderbufferInterface() {};

    virtual void addProxyRef(const Renderbuffer *proxy);
    virtual void releaseProxy(const Renderbuffer *proxy);

    virtual rx::RenderTarget *getRenderTarget() = 0;
    virtual rx::RenderTarget *getDepthStencil() = 0;

    virtual GLsizei getWidth() const = 0;
    virtual GLsizei getHeight() const = 0;
    virtual GLenum getInternalFormat() const = 0;
    virtual GLenum getActualFormat() const = 0;
    virtual GLsizei getSamples() const = 0;

    GLuint getRedSize() const;
    GLuint getGreenSize() const;
    GLuint getBlueSize() const;
    GLuint getAlphaSize() const;
    GLuint getDepthSize() const;
    GLuint getStencilSize() const;

    virtual unsigned int getSerial() const = 0;

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderbufferInterface);
};

class RenderbufferTexture2D : public RenderbufferInterface
{
  public:
    RenderbufferTexture2D(Texture2D *texture, GLenum target);

    virtual ~RenderbufferTexture2D();

    void addProxyRef(const Renderbuffer *proxy);
    void releaseProxy(const Renderbuffer *proxy);

    rx::RenderTarget *getRenderTarget();
    rx::RenderTarget *getDepthStencil();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    virtual unsigned int getSerial() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderbufferTexture2D);

    BindingPointer <Texture2D> mTexture2D;
    GLenum mTarget;
};

class RenderbufferTextureCubeMap : public RenderbufferInterface
{
  public:
    RenderbufferTextureCubeMap(TextureCubeMap *texture, GLenum target);

    virtual ~RenderbufferTextureCubeMap();

    void addProxyRef(const Renderbuffer *proxy);
    void releaseProxy(const Renderbuffer *proxy);

    rx::RenderTarget *getRenderTarget();
    rx::RenderTarget *getDepthStencil();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    virtual unsigned int getSerial() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderbufferTextureCubeMap);

    BindingPointer <TextureCubeMap> mTextureCubeMap;
    GLenum mTarget;
};

// A class derived from RenderbufferStorage is created whenever glRenderbufferStorage
// is called. The specific concrete type depends on whether the internal format is
// colour depth, stencil or packed depth/stencil.
class RenderbufferStorage : public RenderbufferInterface
{
  public:
    RenderbufferStorage();

    virtual ~RenderbufferStorage() = 0;

    virtual rx::RenderTarget *getRenderTarget();
    virtual rx::RenderTarget *getDepthStencil();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    virtual unsigned int getSerial() const;

    static unsigned int issueSerial();
    static unsigned int issueCubeSerials();

  protected:
    GLsizei mWidth;
    GLsizei mHeight;
    GLenum mInternalFormat;
    GLenum mActualFormat;
    GLsizei mSamples;

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderbufferStorage);

    const unsigned int mSerial;

    static unsigned int mCurrentSerial;
};

// Renderbuffer implements the GL renderbuffer object.
// It's only a proxy for a RenderbufferInterface instance; the internal object
// can change whenever glRenderbufferStorage is called.
class Renderbuffer : public RefCountObject
{
  public:
    Renderbuffer(rx::Renderer *renderer, GLuint id, RenderbufferInterface *storage);

    virtual ~Renderbuffer();

    // These functions from RefCountObject are overloaded here because
    // Textures need to maintain their own count of references to them via
    // Renderbuffers/RenderbufferTextures. These functions invoke those
    // reference counting functions on the RenderbufferInterface.
    void addRef() const;
    void release() const;

    rx::RenderTarget *getRenderTarget();
    rx::RenderTarget *getDepthStencil();

    GLsizei getWidth() const;
    GLsizei getHeight() const;
    GLenum getInternalFormat() const;
    GLenum getActualFormat() const;
    GLuint getRedSize() const;
    GLuint getGreenSize() const;
    GLuint getBlueSize() const;
    GLuint getAlphaSize() const;
    GLuint getDepthSize() const;
    GLuint getStencilSize() const;
    GLsizei getSamples() const;

    unsigned int getSerial() const;

    void setStorage(RenderbufferStorage *newStorage);

  private:
    DISALLOW_COPY_AND_ASSIGN(Renderbuffer);

    RenderbufferInterface *mInstance;
};

class Colorbuffer : public RenderbufferStorage
{
  public:
    Colorbuffer(rx::Renderer *renderer, rx::SwapChain *swapChain);
    Colorbuffer(rx::Renderer *renderer, GLsizei width, GLsizei height, GLenum format, GLsizei samples);

    virtual ~Colorbuffer();

    virtual rx::RenderTarget *getRenderTarget();

  private:
    DISALLOW_COPY_AND_ASSIGN(Colorbuffer);

    rx::RenderTarget *mRenderTarget;
};

class DepthStencilbuffer : public RenderbufferStorage
{
  public:
    DepthStencilbuffer(rx::Renderer *renderer, rx::SwapChain *swapChain);
    DepthStencilbuffer(rx::Renderer *renderer, GLsizei width, GLsizei height, GLsizei samples);

    ~DepthStencilbuffer();

    virtual rx::RenderTarget *getDepthStencil();

  protected:
    rx::RenderTarget  *mDepthStencil;

  private:
    DISALLOW_COPY_AND_ASSIGN(DepthStencilbuffer);
};

class Depthbuffer : public DepthStencilbuffer
{
  public:
    Depthbuffer(rx::Renderer *renderer, GLsizei width, GLsizei height, GLsizei samples);

    virtual ~Depthbuffer();

  private:
    DISALLOW_COPY_AND_ASSIGN(Depthbuffer);
};

class Stencilbuffer : public DepthStencilbuffer
{
  public:
    Stencilbuffer(rx::Renderer *renderer, GLsizei width, GLsizei height, GLsizei samples);

    virtual ~Stencilbuffer();

  private:
    DISALLOW_COPY_AND_ASSIGN(Stencilbuffer);
};
}

#endif   // LIBGLESV2_RENDERBUFFER_H_
