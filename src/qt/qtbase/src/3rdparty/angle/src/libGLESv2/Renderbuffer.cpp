#include "precompiled.h"
//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderbuffer.cpp: the gl::Renderbuffer class and its derived classes
// Colorbuffer, Depthbuffer and Stencilbuffer. Implements GL renderbuffer
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4.3 page 108.

#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/renderer/RenderTarget.h"

#include "libGLESv2/Texture.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/utilities.h"

namespace gl
{
unsigned int RenderbufferStorage::mCurrentSerial = 1;

RenderbufferInterface::RenderbufferInterface()
{
}

// The default case for classes inherited from RenderbufferInterface is not to
// need to do anything upon the reference count to the parent Renderbuffer incrementing
// or decrementing. 
void RenderbufferInterface::addProxyRef(const Renderbuffer *proxy)
{
}

void RenderbufferInterface::releaseProxy(const Renderbuffer *proxy)
{
}

GLuint RenderbufferInterface::getRedSize() const
{
    return gl::GetRedSize(getActualFormat());
}

GLuint RenderbufferInterface::getGreenSize() const
{
    return gl::GetGreenSize(getActualFormat());
}

GLuint RenderbufferInterface::getBlueSize() const
{
    return gl::GetBlueSize(getActualFormat());
}

GLuint RenderbufferInterface::getAlphaSize() const
{
    return gl::GetAlphaSize(getActualFormat());
}

GLuint RenderbufferInterface::getDepthSize() const
{
    return gl::GetDepthSize(getActualFormat());
}

GLuint RenderbufferInterface::getStencilSize() const
{
    return gl::GetStencilSize(getActualFormat());
}

///// RenderbufferTexture2D Implementation ////////

RenderbufferTexture2D::RenderbufferTexture2D(Texture2D *texture, GLenum target) : mTarget(target)
{
    mTexture2D.set(texture);
}

RenderbufferTexture2D::~RenderbufferTexture2D()
{
    mTexture2D.set(NULL);
}

// Textures need to maintain their own reference count for references via
// Renderbuffers acting as proxies. Here, we notify the texture of a reference.
void RenderbufferTexture2D::addProxyRef(const Renderbuffer *proxy)
{
    mTexture2D->addProxyRef(proxy);
}

void RenderbufferTexture2D::releaseProxy(const Renderbuffer *proxy)
{
    mTexture2D->releaseProxy(proxy);
}

rx::RenderTarget *RenderbufferTexture2D::getRenderTarget()
{
    return mTexture2D->getRenderTarget(mTarget);
}

rx::RenderTarget *RenderbufferTexture2D::getDepthStencil()
{
    return mTexture2D->getDepthStencil(mTarget);
}

GLsizei RenderbufferTexture2D::getWidth() const
{
    return mTexture2D->getWidth(0);
}

GLsizei RenderbufferTexture2D::getHeight() const
{
    return mTexture2D->getHeight(0);
}

GLenum RenderbufferTexture2D::getInternalFormat() const
{
    return mTexture2D->getInternalFormat(0);
}

GLenum RenderbufferTexture2D::getActualFormat() const
{
    return mTexture2D->getActualFormat(0);
}

GLsizei RenderbufferTexture2D::getSamples() const
{
    return 0;
}

unsigned int RenderbufferTexture2D::getSerial() const
{
    return mTexture2D->getRenderTargetSerial(mTarget);
}

unsigned int RenderbufferTexture2D::getTextureSerial() const
{
    return mTexture2D->getTextureSerial();
}

///// RenderbufferTextureCubeMap Implementation ////////

RenderbufferTextureCubeMap::RenderbufferTextureCubeMap(TextureCubeMap *texture, GLenum target) : mTarget(target)
{
    mTextureCubeMap.set(texture);
}

RenderbufferTextureCubeMap::~RenderbufferTextureCubeMap()
{
    mTextureCubeMap.set(NULL);
}

// Textures need to maintain their own reference count for references via
// Renderbuffers acting as proxies. Here, we notify the texture of a reference.
void RenderbufferTextureCubeMap::addProxyRef(const Renderbuffer *proxy)
{
    mTextureCubeMap->addProxyRef(proxy);
}

void RenderbufferTextureCubeMap::releaseProxy(const Renderbuffer *proxy)
{
    mTextureCubeMap->releaseProxy(proxy);
}

rx::RenderTarget *RenderbufferTextureCubeMap::getRenderTarget()
{
    return mTextureCubeMap->getRenderTarget(mTarget);
}

rx::RenderTarget *RenderbufferTextureCubeMap::getDepthStencil()
{
    return NULL;
}

GLsizei RenderbufferTextureCubeMap::getWidth() const
{
    return mTextureCubeMap->getWidth(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0);
}

GLsizei RenderbufferTextureCubeMap::getHeight() const
{
    return mTextureCubeMap->getHeight(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0);
}

GLenum RenderbufferTextureCubeMap::getInternalFormat() const
{
    return mTextureCubeMap->getInternalFormat(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0);
}

GLenum RenderbufferTextureCubeMap::getActualFormat() const
{
    return mTextureCubeMap->getActualFormat(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0);
}

GLsizei RenderbufferTextureCubeMap::getSamples() const
{
    return 0;
}

unsigned int RenderbufferTextureCubeMap::getSerial() const
{
    return mTextureCubeMap->getRenderTargetSerial(mTarget);
}

unsigned int RenderbufferTextureCubeMap::getTextureSerial() const
{
    return mTextureCubeMap->getTextureSerial();
}

////// Renderbuffer Implementation //////

Renderbuffer::Renderbuffer(rx::Renderer *renderer, GLuint id, RenderbufferInterface *instance) : RefCountObject(id)
{
    ASSERT(instance != NULL);
    mInstance = instance;
}

Renderbuffer::~Renderbuffer()
{
    delete mInstance;
}

// The RenderbufferInterface contained in this Renderbuffer may need to maintain
// its own reference count, so we pass it on here.
void Renderbuffer::addRef() const
{
    mInstance->addProxyRef(this);

    RefCountObject::addRef();
}

void Renderbuffer::release() const
{
    mInstance->releaseProxy(this);

    RefCountObject::release();
}

rx::RenderTarget *Renderbuffer::getRenderTarget()
{
    return mInstance->getRenderTarget();
}

rx::RenderTarget *Renderbuffer::getDepthStencil()
{
    return mInstance->getDepthStencil();
}

GLsizei Renderbuffer::getWidth() const
{
    return mInstance->getWidth();
}

GLsizei Renderbuffer::getHeight() const
{
    return mInstance->getHeight();
}

GLenum Renderbuffer::getInternalFormat() const
{
    return mInstance->getInternalFormat();
}

GLenum Renderbuffer::getActualFormat() const
{
    return mInstance->getActualFormat();
}

GLuint Renderbuffer::getRedSize() const
{
    return mInstance->getRedSize();
}

GLuint Renderbuffer::getGreenSize() const
{
    return mInstance->getGreenSize();
}

GLuint Renderbuffer::getBlueSize() const
{
    return mInstance->getBlueSize();
}

GLuint Renderbuffer::getAlphaSize() const
{
    return mInstance->getAlphaSize();
}

GLuint Renderbuffer::getDepthSize() const
{
    return mInstance->getDepthSize();
}

GLuint Renderbuffer::getStencilSize() const
{
    return mInstance->getStencilSize();
}

GLsizei Renderbuffer::getSamples() const
{
    return mInstance->getSamples();
}

unsigned int Renderbuffer::getSerial() const
{
    return mInstance->getSerial();
}

unsigned int Renderbuffer::getTextureSerial() const
{
    return mInstance->getTextureSerial();
}

void Renderbuffer::setStorage(RenderbufferStorage *newStorage)
{
    ASSERT(newStorage != NULL);

    delete mInstance;
    mInstance = newStorage;
}

RenderbufferStorage::RenderbufferStorage() : mSerial(issueSerial())
{
    mWidth = 0;
    mHeight = 0;
    mInternalFormat = GL_RGBA4;
    mActualFormat = GL_RGBA8_OES;
    mSamples = 0;
}

RenderbufferStorage::~RenderbufferStorage()
{
}

rx::RenderTarget *RenderbufferStorage::getRenderTarget()
{
    return NULL;
}

rx::RenderTarget *RenderbufferStorage::getDepthStencil()
{
    return NULL;
}

GLsizei RenderbufferStorage::getWidth() const
{
    return mWidth;
}

GLsizei RenderbufferStorage::getHeight() const
{
    return mHeight;
}

GLenum RenderbufferStorage::getInternalFormat() const
{
    return mInternalFormat;
}

GLenum RenderbufferStorage::getActualFormat() const
{
    return mActualFormat;
}

GLsizei RenderbufferStorage::getSamples() const
{
    return mSamples;
}

unsigned int RenderbufferStorage::getSerial() const
{
    return mSerial;
}

unsigned int RenderbufferStorage::issueSerial()
{
    return mCurrentSerial++;
}

unsigned int RenderbufferStorage::issueCubeSerials()
{
    unsigned int firstSerial = mCurrentSerial;
    mCurrentSerial += 6;
    return firstSerial;
}

Colorbuffer::Colorbuffer(rx::Renderer *renderer, rx::SwapChain *swapChain)
{
    mRenderTarget = renderer->createRenderTarget(swapChain, false); 

    if (mRenderTarget)
    {
        mWidth = mRenderTarget->getWidth();
        mHeight = mRenderTarget->getHeight();
        mInternalFormat = mRenderTarget->getInternalFormat();
        mActualFormat = mRenderTarget->getActualFormat();
        mSamples = mRenderTarget->getSamples();
    }
}

Colorbuffer::Colorbuffer(rx::Renderer *renderer, int width, int height, GLenum format, GLsizei samples) : mRenderTarget(NULL)
{
    mRenderTarget = renderer->createRenderTarget(width, height, format, samples, false);

    if (mRenderTarget)
    {
        mWidth = width;
        mHeight = height;
        mInternalFormat = format;
        mActualFormat = mRenderTarget->getActualFormat();
        mSamples = mRenderTarget->getSamples();
    }
}

Colorbuffer::~Colorbuffer()
{
    if (mRenderTarget)
    {
        delete mRenderTarget;
    }
}

rx::RenderTarget *Colorbuffer::getRenderTarget()
{
    if (mRenderTarget)
    {
        return mRenderTarget;
    }

    return NULL;
}

DepthStencilbuffer::DepthStencilbuffer(rx::Renderer *renderer, rx::SwapChain *swapChain)
{
    mDepthStencil = renderer->createRenderTarget(swapChain, true);
    if (mDepthStencil)
    {
        mWidth = mDepthStencil->getWidth();
        mHeight = mDepthStencil->getHeight();
        mInternalFormat = mDepthStencil->getInternalFormat();
        mSamples = mDepthStencil->getSamples();
        mActualFormat = mDepthStencil->getActualFormat();
    }
}

DepthStencilbuffer::DepthStencilbuffer(rx::Renderer *renderer, int width, int height, GLsizei samples)
{

    mDepthStencil = renderer->createRenderTarget(width, height, GL_DEPTH24_STENCIL8_OES, samples, true);

    mWidth = mDepthStencil->getWidth();
    mHeight = mDepthStencil->getHeight();
    mInternalFormat = GL_DEPTH24_STENCIL8_OES;
    mActualFormat = mDepthStencil->getActualFormat();
    mSamples = mDepthStencil->getSamples();
}

DepthStencilbuffer::~DepthStencilbuffer()
{
    if (mDepthStencil)
    {
        delete mDepthStencil;
    }
}

rx::RenderTarget *DepthStencilbuffer::getDepthStencil()
{
    if (mDepthStencil)
    {
        return mDepthStencil;
    }

    return NULL;
}

Depthbuffer::Depthbuffer(rx::Renderer *renderer, int width, int height, GLsizei samples) : DepthStencilbuffer(renderer, width, height, samples)
{
    if (mDepthStencil)
    {
        mInternalFormat = GL_DEPTH_COMPONENT16;   // If the renderbuffer parameters are queried, the calling function
                                                  // will expect one of the valid renderbuffer formats for use in 
                                                  // glRenderbufferStorage
    }
}

Depthbuffer::~Depthbuffer()
{
}

Stencilbuffer::Stencilbuffer(rx::Renderer *renderer, int width, int height, GLsizei samples) : DepthStencilbuffer(renderer, width, height, samples)
{
    if (mDepthStencil)
    {
        mInternalFormat = GL_STENCIL_INDEX8;   // If the renderbuffer parameters are queried, the calling function
                                               // will expect one of the valid renderbuffer formats for use in 
                                               // glRenderbufferStorage
    }
}

Stencilbuffer::~Stencilbuffer()
{
}

}
