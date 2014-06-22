#include "precompiled.h"
//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Texture.cpp: Implements the gl::Texture class and its derived classes
// Texture2D and TextureCubeMap. Implements GL texture objects and related
// functionality. [OpenGL ES 2.0.24] section 3.7 page 63.

#include "libGLESv2/Texture.h"

#include "libGLESv2/main.h"
#include "libGLESv2/mathutil.h"
#include "libGLESv2/utilities.h"
#include "libGLESv2/renderer/Blit.h"
#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/renderer/Image.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/TextureStorage.h"
#include "libEGL/Surface.h"

namespace gl
{

Texture::Texture(rx::Renderer *renderer, GLuint id) : RefCountObject(id)
{
    mRenderer = renderer;

    mSamplerState.minFilter = GL_NEAREST_MIPMAP_LINEAR;
    mSamplerState.magFilter = GL_LINEAR;
    mSamplerState.wrapS = GL_REPEAT;
    mSamplerState.wrapT = GL_REPEAT;
    mSamplerState.maxAnisotropy = 1.0f;
    mSamplerState.lodOffset = 0;
    mUsage = GL_NONE;
    
    mDirtyImages = true;

    mImmutable = false;
}

Texture::~Texture()
{
}

// Returns true on successful filter state update (valid enum parameter)
bool Texture::setMinFilter(GLenum filter)
{
    switch (filter)
    {
      case GL_NEAREST:
      case GL_LINEAR:
      case GL_NEAREST_MIPMAP_NEAREST:
      case GL_LINEAR_MIPMAP_NEAREST:
      case GL_NEAREST_MIPMAP_LINEAR:
      case GL_LINEAR_MIPMAP_LINEAR:
        mSamplerState.minFilter = filter;
        return true;
      default:
        return false;
    }
}

// Returns true on successful filter state update (valid enum parameter)
bool Texture::setMagFilter(GLenum filter)
{
    switch (filter)
    {
      case GL_NEAREST:
      case GL_LINEAR:
        mSamplerState.magFilter = filter;
        return true;
      default:
        return false;
    }
}

// Returns true on successful wrap state update (valid enum parameter)
bool Texture::setWrapS(GLenum wrap)
{
    switch (wrap)
    {
      case GL_REPEAT:
      case GL_CLAMP_TO_EDGE:
      case GL_MIRRORED_REPEAT:
        mSamplerState.wrapS = wrap;
        return true;
      default:
        return false;
    }
}

// Returns true on successful wrap state update (valid enum parameter)
bool Texture::setWrapT(GLenum wrap)
{
    switch (wrap)
    {
      case GL_REPEAT:
      case GL_CLAMP_TO_EDGE:
      case GL_MIRRORED_REPEAT:
        mSamplerState.wrapT = wrap;
        return true;
      default:
        return false;
    }
}

// Returns true on successful max anisotropy update (valid anisotropy value)
bool Texture::setMaxAnisotropy(float textureMaxAnisotropy, float contextMaxAnisotropy)
{
    textureMaxAnisotropy = std::min(textureMaxAnisotropy, contextMaxAnisotropy);
    if (textureMaxAnisotropy < 1.0f)
    {
        return false;
    }

    mSamplerState.maxAnisotropy = textureMaxAnisotropy;

    return true;
}

// Returns true on successful usage state update (valid enum parameter)
bool Texture::setUsage(GLenum usage)
{
    switch (usage)
    {
      case GL_NONE:
      case GL_FRAMEBUFFER_ATTACHMENT_ANGLE:
        mUsage = usage;
        return true;
      default:
        return false;
    }
}

GLenum Texture::getMinFilter() const
{
    return mSamplerState.minFilter;
}

GLenum Texture::getMagFilter() const
{
    return mSamplerState.magFilter;
}

GLenum Texture::getWrapS() const
{
    return mSamplerState.wrapS;
}

GLenum Texture::getWrapT() const
{
    return mSamplerState.wrapT;
}

float Texture::getMaxAnisotropy() const
{
    return mSamplerState.maxAnisotropy;
}

int Texture::getLodOffset()
{
    rx::TextureStorageInterface *texture = getStorage(false);
    return texture ? texture->getLodOffset() : 0;
}

void Texture::getSamplerState(SamplerState *sampler)
{
    *sampler = mSamplerState;
    sampler->lodOffset = getLodOffset();
}

GLenum Texture::getUsage() const
{
    return mUsage;
}

bool Texture::isMipmapFiltered() const
{
    switch (mSamplerState.minFilter)
    {
      case GL_NEAREST:
      case GL_LINEAR:
        return false;
      case GL_NEAREST_MIPMAP_NEAREST:
      case GL_LINEAR_MIPMAP_NEAREST:
      case GL_NEAREST_MIPMAP_LINEAR:
      case GL_LINEAR_MIPMAP_LINEAR:
        return true;
      default: UNREACHABLE();
        return false;
    }
}

void Texture::setImage(GLint unpackAlignment, const void *pixels, rx::Image *image)
{
    if (pixels != NULL)
    {
        image->loadData(0, 0, image->getWidth(), image->getHeight(), unpackAlignment, pixels);
        mDirtyImages = true;
    }
}

void Texture::setCompressedImage(GLsizei imageSize, const void *pixels, rx::Image *image)
{
    if (pixels != NULL)
    {
        image->loadCompressedData(0, 0, image->getWidth(), image->getHeight(), pixels);
        mDirtyImages = true;
    }
}

bool Texture::subImage(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels, rx::Image *image)
{
    if (pixels != NULL)
    {
        image->loadData(xoffset, yoffset, width, height, unpackAlignment, pixels);
        mDirtyImages = true;
    }

    return true;
}

bool Texture::subImageCompressed(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels, rx::Image *image)
{
    if (pixels != NULL)
    {
        image->loadCompressedData(xoffset, yoffset, width, height, pixels);
        mDirtyImages = true;
    }

    return true;
}

rx::TextureStorageInterface *Texture::getNativeTexture()
{
    // ensure the underlying texture is created

    rx::TextureStorageInterface *storage = getStorage(false);
    if (storage)
    {
        updateTexture();
    }

    return storage;
}

bool Texture::hasDirtyImages() const
{
    return mDirtyImages;
}

void Texture::resetDirty()
{
    mDirtyImages = false;
}

unsigned int Texture::getTextureSerial()
{
    rx::TextureStorageInterface *texture = getStorage(false);
    return texture ? texture->getTextureSerial() : 0;
}

unsigned int Texture::getRenderTargetSerial(GLenum target)
{
    rx::TextureStorageInterface *texture = getStorage(true);
    return texture ? texture->getRenderTargetSerial(target) : 0;
}

bool Texture::isImmutable() const
{
    return mImmutable;
}

GLint Texture::creationLevels(GLsizei width, GLsizei height) const
{
    if ((isPow2(width) && isPow2(height)) || mRenderer->getNonPower2TextureSupport())
    {
        return 0;   // Maximum number of levels
    }
    else
    {
        // OpenGL ES 2.0 without GL_OES_texture_npot does not permit NPOT mipmaps.
        return 1;
    }
}

GLint Texture::creationLevels(GLsizei size) const
{
    return creationLevels(size, size);
}

Texture2D::Texture2D(rx::Renderer *renderer, GLuint id) : Texture(renderer, id)
{
    mTexStorage = NULL;
    mSurface = NULL;
    mColorbufferProxy = NULL;
    mProxyRefs = 0;

    for (int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; ++i)
    {
        mImageArray[i] = renderer->createImage();
    }
}

Texture2D::~Texture2D()
{
    mColorbufferProxy = NULL;

    delete mTexStorage;
    mTexStorage = NULL;
    
    if (mSurface)
    {
        mSurface->setBoundTexture(NULL);
        mSurface = NULL;
    }

    for (int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; ++i)
    {
        delete mImageArray[i];
    }
}

// We need to maintain a count of references to renderbuffers acting as 
// proxies for this texture, so that we do not attempt to use a pointer 
// to a renderbuffer proxy which has been deleted.
void Texture2D::addProxyRef(const Renderbuffer *proxy)
{
    mProxyRefs++;
}

void Texture2D::releaseProxy(const Renderbuffer *proxy)
{
    if (mProxyRefs > 0)
        mProxyRefs--;

    if (mProxyRefs == 0)
        mColorbufferProxy = NULL;
}

GLenum Texture2D::getTarget() const
{
    return GL_TEXTURE_2D;
}

GLsizei Texture2D::getWidth(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[level]->getWidth();
    else
        return 0;
}

GLsizei Texture2D::getHeight(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[level]->getHeight();
    else
        return 0;
}

GLenum Texture2D::getInternalFormat(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[level]->getInternalFormat();
    else
        return GL_NONE;
}

GLenum Texture2D::getActualFormat(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[level]->getActualFormat();
    else
        return D3DFMT_UNKNOWN;
}

void Texture2D::redefineImage(GLint level, GLint internalformat, GLsizei width, GLsizei height)
{
    releaseTexImage();

    // If there currently is a corresponding storage texture image, it has these parameters
    const int storageWidth = std::max(1, mImageArray[0]->getWidth() >> level);
    const int storageHeight = std::max(1, mImageArray[0]->getHeight() >> level);
    const int storageFormat = mImageArray[0]->getInternalFormat();

    mImageArray[level]->redefine(mRenderer, internalformat, width, height, false);

    if (mTexStorage)
    {
        const int storageLevels = mTexStorage->levelCount();
        
        if ((level >= storageLevels && storageLevels != 0) ||
            width != storageWidth ||
            height != storageHeight ||
            internalformat != storageFormat)   // Discard mismatched storage
        {
            for (int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
            {
                mImageArray[i]->markDirty();
            }

            delete mTexStorage;
            mTexStorage = NULL;
            mDirtyImages = true;
        }
    }
}

void Texture2D::setImage(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
    GLint internalformat = ConvertSizedInternalFormat(format, type);
    redefineImage(level, internalformat, width, height);

    Texture::setImage(unpackAlignment, pixels, mImageArray[level]);
}

void Texture2D::bindTexImage(egl::Surface *surface)
{
    releaseTexImage();

    GLint internalformat = surface->getFormat();

    mImageArray[0]->redefine(mRenderer, internalformat, surface->getWidth(), surface->getHeight(), true);

    delete mTexStorage;
    mTexStorage = new rx::TextureStorageInterface2D(mRenderer, surface->getSwapChain());

    mDirtyImages = true;
    mSurface = surface;
    mSurface->setBoundTexture(this);
}

void Texture2D::releaseTexImage()
{
    if (mSurface)
    {
        mSurface->setBoundTexture(NULL);
        mSurface = NULL;

        if (mTexStorage)
        {
            delete mTexStorage;
            mTexStorage = NULL;
        }

        for (int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
        {
            mImageArray[i]->redefine(mRenderer, GL_NONE, 0, 0, true);
        }
    }
}

void Texture2D::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels)
{
    // compressed formats don't have separate sized internal formats-- we can just use the compressed format directly
    redefineImage(level, format, width, height);

    Texture::setCompressedImage(imageSize, pixels, mImageArray[level]);
}

void Texture2D::commitRect(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height)
{
    if (level < levelCount())
    {
        rx::Image *image = mImageArray[level];
        if (image->updateSurface(mTexStorage, level, xoffset, yoffset, width, height))
        {
            image->markClean();
        }
    }
}

void Texture2D::subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
    if (Texture::subImage(xoffset, yoffset, width, height, format, type, unpackAlignment, pixels, mImageArray[level]))
    {
        commitRect(level, xoffset, yoffset, width, height);
    }
}

void Texture2D::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels)
{
    if (Texture::subImageCompressed(xoffset, yoffset, width, height, format, imageSize, pixels, mImageArray[level]))
    {
        commitRect(level, xoffset, yoffset, width, height);
    }
}

void Texture2D::copyImage(GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    GLint internalformat = ConvertSizedInternalFormat(format, GL_UNSIGNED_BYTE);
    redefineImage(level, internalformat, width, height);

    if (!mImageArray[level]->isRenderableFormat())
    {
        mImageArray[level]->copy(0, 0, x, y, width, height, source);
        mDirtyImages = true;
    }
    else
    {
        if (!mTexStorage || !mTexStorage->isRenderTarget())
        {
            convertToRenderTarget();
        }
        
        mImageArray[level]->markClean();

        if (width != 0 && height != 0 && level < levelCount())
        {
            gl::Rectangle sourceRect;
            sourceRect.x = x;
            sourceRect.width = width;
            sourceRect.y = y;
            sourceRect.height = height;

            mRenderer->copyImage(source, sourceRect, format, 0, 0, mTexStorage, level);
        }
    }
}

void Texture2D::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    if (xoffset + width > mImageArray[level]->getWidth() || yoffset + height > mImageArray[level]->getHeight())
    {
        return gl::error(GL_INVALID_VALUE);
    }

    if (!mImageArray[level]->isRenderableFormat() || (!mTexStorage && !isSamplerComplete()))
    {
        mImageArray[level]->copy(xoffset, yoffset, x, y, width, height, source);
        mDirtyImages = true;
    }
    else
    {
        if (!mTexStorage || !mTexStorage->isRenderTarget())
        {
            convertToRenderTarget();
        }
        
        updateTexture();

        if (level < levelCount())
        {
            gl::Rectangle sourceRect;
            sourceRect.x = x;
            sourceRect.width = width;
            sourceRect.y = y;
            sourceRect.height = height;

            mRenderer->copyImage(source, sourceRect, 
                                 gl::ExtractFormat(mImageArray[0]->getInternalFormat()),
                                 xoffset, yoffset, mTexStorage, level);
        }
    }
}

void Texture2D::storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
    delete mTexStorage;
    mTexStorage = new rx::TextureStorageInterface2D(mRenderer, levels, internalformat, mUsage, false, width, height);
    mImmutable = true;

    for (int level = 0; level < levels; level++)
    {
        mImageArray[level]->redefine(mRenderer, internalformat, width, height, true);
        width = std::max(1, width >> 1);
        height = std::max(1, height >> 1);
    }

    for (int level = levels; level < IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
    {
        mImageArray[level]->redefine(mRenderer, GL_NONE, 0, 0, true);
    }

    if (mTexStorage->isManaged())
    {
        int levels = levelCount();

        for (int level = 0; level < levels; level++)
        {
            mImageArray[level]->setManagedSurface(mTexStorage, level);
        }
    }
}

// Tests for 2D texture sampling completeness. [OpenGL ES 2.0.24] section 3.8.2 page 85.
bool Texture2D::isSamplerComplete() const
{
    GLsizei width = mImageArray[0]->getWidth();
    GLsizei height = mImageArray[0]->getHeight();

    if (width <= 0 || height <= 0)
    {
        return false;
    }

    bool mipmapping = isMipmapFiltered();
    bool filtering, renderable;

    if ((IsFloat32Format(getInternalFormat(0)) && !mRenderer->getFloat32TextureSupport(&filtering, &renderable)) ||
        (IsFloat16Format(getInternalFormat(0)) && !mRenderer->getFloat16TextureSupport(&filtering, &renderable)))
    {
        if (mSamplerState.magFilter != GL_NEAREST ||
            (mSamplerState.minFilter != GL_NEAREST && mSamplerState.minFilter != GL_NEAREST_MIPMAP_NEAREST))
        {
            return false;
        }
    }

    bool npotSupport = mRenderer->getNonPower2TextureSupport();

    if (!npotSupport)
    {
        if ((mSamplerState.wrapS != GL_CLAMP_TO_EDGE && !isPow2(width)) ||
            (mSamplerState.wrapT != GL_CLAMP_TO_EDGE && !isPow2(height)))
        {
            return false;
        }
    }

    if (mipmapping)
    {
        if (!npotSupport)
        {
            if (!isPow2(width) || !isPow2(height))
            {
                return false;
            }
        }

        if (!isMipmapComplete())
        {
            return false;
        }
    }

    return true;
}

// Tests for 2D texture (mipmap) completeness. [OpenGL ES 2.0.24] section 3.7.10 page 81.
bool Texture2D::isMipmapComplete() const
{
    if (isImmutable())
    {
        return true;
    }

    GLsizei width = mImageArray[0]->getWidth();
    GLsizei height = mImageArray[0]->getHeight();

    if (width <= 0 || height <= 0)
    {
        return false;
    }

    int q = log2(std::max(width, height));

    for (int level = 1; level <= q; level++)
    {
        if (mImageArray[level]->getInternalFormat() != mImageArray[0]->getInternalFormat())
        {
            return false;
        }

        if (mImageArray[level]->getWidth() != std::max(1, width >> level))
        {
            return false;
        }

        if (mImageArray[level]->getHeight() != std::max(1, height >> level))
        {
            return false;
        }
    }

    return true;
}

bool Texture2D::isCompressed(GLint level) const
{
    return IsCompressed(getInternalFormat(level));
}

bool Texture2D::isDepth(GLint level) const
{
    return IsDepthTexture(getInternalFormat(level));
}

// Constructs a native texture resource from the texture images
void Texture2D::createTexture()
{
    GLsizei width = mImageArray[0]->getWidth();
    GLsizei height = mImageArray[0]->getHeight();

    if (!(width > 0 && height > 0))
        return; // do not attempt to create native textures for nonexistant data

    GLint levels = creationLevels(width, height);
    GLenum internalformat = mImageArray[0]->getInternalFormat();

    delete mTexStorage;
    mTexStorage = new rx::TextureStorageInterface2D(mRenderer, levels, internalformat, mUsage, false, width, height);
    
    if (mTexStorage->isManaged())
    {
        int levels = levelCount();

        for (int level = 0; level < levels; level++)
        {
            mImageArray[level]->setManagedSurface(mTexStorage, level);
        }
    }

    mDirtyImages = true;
}

void Texture2D::updateTexture()
{
    bool mipmapping = (isMipmapFiltered() && isMipmapComplete());

    int levels = (mipmapping ? levelCount() : 1);

    for (int level = 0; level < levels; level++)
    {
        rx::Image *image = mImageArray[level];

        if (image->isDirty())
        {
            commitRect(level, 0, 0, mImageArray[level]->getWidth(), mImageArray[level]->getHeight());
        }
    }
}

void Texture2D::convertToRenderTarget()
{
    rx::TextureStorageInterface2D *newTexStorage = NULL;

    if (mImageArray[0]->getWidth() != 0 && mImageArray[0]->getHeight() != 0)
    {
        GLsizei width = mImageArray[0]->getWidth();
        GLsizei height = mImageArray[0]->getHeight();
        GLint levels = mTexStorage != NULL ? mTexStorage->levelCount() : creationLevels(width, height);
        GLenum internalformat = mImageArray[0]->getInternalFormat();

        newTexStorage = new rx::TextureStorageInterface2D(mRenderer, levels, internalformat, GL_FRAMEBUFFER_ATTACHMENT_ANGLE, true, width, height);

        if (mTexStorage != NULL)
        {
            if (!mRenderer->copyToRenderTarget(newTexStorage, mTexStorage))
            {   
                delete newTexStorage;
                return gl::error(GL_OUT_OF_MEMORY);
            }
        }
    }

    delete mTexStorage;
    mTexStorage = newTexStorage;

    mDirtyImages = true;
}

void Texture2D::generateMipmaps()
{
    if (!mRenderer->getNonPower2TextureSupport())
    {
        if (!isPow2(mImageArray[0]->getWidth()) || !isPow2(mImageArray[0]->getHeight()))
        {
            return gl::error(GL_INVALID_OPERATION);
        }
    }

    // Purge array levels 1 through q and reset them to represent the generated mipmap levels.
    unsigned int q = log2(std::max(mImageArray[0]->getWidth(), mImageArray[0]->getHeight()));
    for (unsigned int i = 1; i <= q; i++)
    {
        redefineImage(i, mImageArray[0]->getInternalFormat(),
                      std::max(mImageArray[0]->getWidth() >> i, 1),
                      std::max(mImageArray[0]->getHeight() >> i, 1));
    }

    if (mTexStorage && mTexStorage->isRenderTarget())
    {
        for (unsigned int i = 1; i <= q; i++)
        {
            mTexStorage->generateMipmap(i);

            mImageArray[i]->markClean();
        }
    }
    else
    {
        for (unsigned int i = 1; i <= q; i++)
        {
            mRenderer->generateMipmap(mImageArray[i], mImageArray[i - 1]);
        }
    }
}

Renderbuffer *Texture2D::getRenderbuffer(GLenum target)
{
    if (target != GL_TEXTURE_2D)
    {
        return gl::error(GL_INVALID_OPERATION, (Renderbuffer *)NULL);
    }

    if (mColorbufferProxy == NULL)
    {
        mColorbufferProxy = new Renderbuffer(mRenderer, id(), new RenderbufferTexture2D(this, target));
    }

    return mColorbufferProxy;
}

rx::RenderTarget *Texture2D::getRenderTarget(GLenum target)
{
    ASSERT(target == GL_TEXTURE_2D);

    // ensure the underlying texture is created
    if (getStorage(true) == NULL)
    {
        return NULL;
    }

    updateTexture();
    
    // ensure this is NOT a depth texture
    if (isDepth(0))
    {
        return NULL;
    }

    return mTexStorage->getRenderTarget();
}

rx::RenderTarget *Texture2D::getDepthStencil(GLenum target)
{
    ASSERT(target == GL_TEXTURE_2D);

    // ensure the underlying texture is created
    if (getStorage(true) == NULL)
    {
        return NULL;
    }

    updateTexture();

    // ensure this is actually a depth texture
    if (!isDepth(0))
    {
        return NULL;
    }
    return mTexStorage->getRenderTarget();
}

int Texture2D::levelCount()
{
    return mTexStorage ? mTexStorage->levelCount() : 0;
}

rx::TextureStorageInterface *Texture2D::getStorage(bool renderTarget)
{
    if (!mTexStorage || (renderTarget && !mTexStorage->isRenderTarget()))
    {
        if (renderTarget)
        {
            convertToRenderTarget();
        }
        else
        {
            createTexture();
        }
    }

    return mTexStorage;
}

TextureCubeMap::TextureCubeMap(rx::Renderer *renderer, GLuint id) : Texture(renderer, id)
{
    mTexStorage = NULL;
    for (int i = 0; i < 6; i++)
    {
        mFaceProxies[i] = NULL;
        mFaceProxyRefs[i] = 0;

        for (int j = 0; j < IMPLEMENTATION_MAX_TEXTURE_LEVELS; ++j)
        {
            mImageArray[i][j] = renderer->createImage();
        }
    }
}

TextureCubeMap::~TextureCubeMap()
{
    for (int i = 0; i < 6; i++)
    {
        mFaceProxies[i] = NULL;

        for (int j = 0; j < IMPLEMENTATION_MAX_TEXTURE_LEVELS; ++j)
        {
            delete mImageArray[i][j];
        }
    }

    delete mTexStorage;
    mTexStorage = NULL;
}

// We need to maintain a count of references to renderbuffers acting as 
// proxies for this texture, so that the texture is not deleted while 
// proxy references still exist. If the reference count drops to zero,
// we set our proxy pointer NULL, so that a new attempt at referencing
// will cause recreation.
void TextureCubeMap::addProxyRef(const Renderbuffer *proxy)
{
    for (int i = 0; i < 6; i++)
    {
        if (mFaceProxies[i] == proxy)
            mFaceProxyRefs[i]++;
    }
}

void TextureCubeMap::releaseProxy(const Renderbuffer *proxy)
{
    for (int i = 0; i < 6; i++)
    {
        if (mFaceProxies[i] == proxy)
        {
            if (mFaceProxyRefs[i] > 0)
                mFaceProxyRefs[i]--;

            if (mFaceProxyRefs[i] == 0)
                mFaceProxies[i] = NULL;
        }
    }
}

GLenum TextureCubeMap::getTarget() const
{
    return GL_TEXTURE_CUBE_MAP;
}

GLsizei TextureCubeMap::getWidth(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[faceIndex(target)][level]->getWidth();
    else
        return 0;
}

GLsizei TextureCubeMap::getHeight(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[faceIndex(target)][level]->getHeight();
    else
        return 0;
}

GLenum TextureCubeMap::getInternalFormat(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[faceIndex(target)][level]->getInternalFormat();
    else
        return GL_NONE;
}

GLenum TextureCubeMap::getActualFormat(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[faceIndex(target)][level]->getActualFormat();
    else
        return D3DFMT_UNKNOWN;
}

void TextureCubeMap::setImagePosX(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
    setImage(0, level, width, height, format, type, unpackAlignment, pixels);
}

void TextureCubeMap::setImageNegX(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
    setImage(1, level, width, height, format, type, unpackAlignment, pixels);
}

void TextureCubeMap::setImagePosY(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
    setImage(2, level, width, height, format, type, unpackAlignment, pixels);
}

void TextureCubeMap::setImageNegY(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
    setImage(3, level, width, height, format, type, unpackAlignment, pixels);
}

void TextureCubeMap::setImagePosZ(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
    setImage(4, level, width, height, format, type, unpackAlignment, pixels);
}

void TextureCubeMap::setImageNegZ(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
    setImage(5, level, width, height, format, type, unpackAlignment, pixels);
}

void TextureCubeMap::setCompressedImage(GLenum face, GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels)
{
    // compressed formats don't have separate sized internal formats-- we can just use the compressed format directly
    redefineImage(faceIndex(face), level, format, width, height);

    Texture::setCompressedImage(imageSize, pixels, mImageArray[faceIndex(face)][level]);
}

void TextureCubeMap::commitRect(int face, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height)
{
    if (level < levelCount())
    {
        rx::Image *image = mImageArray[face][level];
        if (image->updateSurface(mTexStorage, face, level, xoffset, yoffset, width, height))
            image->markClean();
    }
}

void TextureCubeMap::subImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
    if (Texture::subImage(xoffset, yoffset, width, height, format, type, unpackAlignment, pixels, mImageArray[faceIndex(target)][level]))
    {
        commitRect(faceIndex(target), level, xoffset, yoffset, width, height);
    }
}

void TextureCubeMap::subImageCompressed(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels)
{
    if (Texture::subImageCompressed(xoffset, yoffset, width, height, format, imageSize, pixels, mImageArray[faceIndex(target)][level]))
    {
        commitRect(faceIndex(target), level, xoffset, yoffset, width, height);
    }
}

// Tests for cube map sampling completeness. [OpenGL ES 2.0.24] section 3.8.2 page 86.
bool TextureCubeMap::isSamplerComplete() const
{
    int size = mImageArray[0][0]->getWidth();

    bool mipmapping = isMipmapFiltered();
    bool filtering, renderable;

    if ((gl::ExtractType(getInternalFormat(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0)) == GL_FLOAT && !mRenderer->getFloat32TextureSupport(&filtering, &renderable)) ||
        (gl::ExtractType(getInternalFormat(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0) == GL_HALF_FLOAT_OES) && !mRenderer->getFloat16TextureSupport(&filtering, &renderable)))
    {
        if (mSamplerState.magFilter != GL_NEAREST ||
            (mSamplerState.minFilter != GL_NEAREST && mSamplerState.minFilter != GL_NEAREST_MIPMAP_NEAREST))
        {
            return false;
        }
    }

    if (!isPow2(size) && !mRenderer->getNonPower2TextureSupport())
    {
        if (mSamplerState.wrapS != GL_CLAMP_TO_EDGE || mSamplerState.wrapT != GL_CLAMP_TO_EDGE || mipmapping)
        {
            return false;
        }
    }

    if (!mipmapping)
    {
        if (!isCubeComplete())
        {
            return false;
        }
    }
    else
    {
        if (!isMipmapCubeComplete())   // Also tests for isCubeComplete()
        {
            return false;
        }
    }

    return true;
}

// Tests for cube texture completeness. [OpenGL ES 2.0.24] section 3.7.10 page 81.
bool TextureCubeMap::isCubeComplete() const
{
    if (mImageArray[0][0]->getWidth() <= 0 || mImageArray[0][0]->getHeight() != mImageArray[0][0]->getWidth())
    {
        return false;
    }

    for (unsigned int face = 1; face < 6; face++)
    {
        if (mImageArray[face][0]->getWidth() != mImageArray[0][0]->getWidth() ||
            mImageArray[face][0]->getWidth() != mImageArray[0][0]->getHeight() ||
            mImageArray[face][0]->getInternalFormat() != mImageArray[0][0]->getInternalFormat())
        {
            return false;
        }
    }

    return true;
}

bool TextureCubeMap::isMipmapCubeComplete() const
{
    if (isImmutable())
    {
        return true;
    }

    if (!isCubeComplete())
    {
        return false;
    }

    GLsizei size = mImageArray[0][0]->getWidth();

    int q = log2(size);

    for (int face = 0; face < 6; face++)
    {
        for (int level = 1; level <= q; level++)
        {
            if (mImageArray[face][level]->getInternalFormat() != mImageArray[0][0]->getInternalFormat())
            {
                return false;
            }

            if (mImageArray[face][level]->getWidth() != std::max(1, size >> level))
            {
                return false;
            }
        }
    }

    return true;
}

bool TextureCubeMap::isCompressed(GLenum target, GLint level) const
{
    return IsCompressed(getInternalFormat(target, level));
}

// Constructs a native texture resource from the texture images, or returns an existing one
void TextureCubeMap::createTexture()
{
    GLsizei size = mImageArray[0][0]->getWidth();

    if (!(size > 0))
        return; // do not attempt to create native textures for nonexistant data

    GLint levels = creationLevels(size);
    GLenum internalformat = mImageArray[0][0]->getInternalFormat();

    delete mTexStorage;
    mTexStorage = new rx::TextureStorageInterfaceCube(mRenderer, levels, internalformat, mUsage, false, size);

    if (mTexStorage->isManaged())
    {
        int levels = levelCount();

        for (int face = 0; face < 6; face++)
        {
            for (int level = 0; level < levels; level++)
            {
                mImageArray[face][level]->setManagedSurface(mTexStorage, face, level);
            }
        }
    }

    mDirtyImages = true;
}

void TextureCubeMap::updateTexture()
{
    bool mipmapping = isMipmapFiltered() && isMipmapCubeComplete();

    for (int face = 0; face < 6; face++)
    {
        int levels = (mipmapping ? levelCount() : 1);

        for (int level = 0; level < levels; level++)
        {
            rx::Image *image = mImageArray[face][level];

            if (image->isDirty())
            {
                commitRect(face, level, 0, 0, image->getWidth(), image->getHeight());
            }
        }
    }
}

void TextureCubeMap::convertToRenderTarget()
{
    rx::TextureStorageInterfaceCube *newTexStorage = NULL;

    if (mImageArray[0][0]->getWidth() != 0)
    {
        GLsizei size = mImageArray[0][0]->getWidth();
        GLint levels = mTexStorage != NULL ? mTexStorage->levelCount() : creationLevels(size);
        GLenum internalformat = mImageArray[0][0]->getInternalFormat();

        newTexStorage = new rx::TextureStorageInterfaceCube(mRenderer, levels, internalformat, GL_FRAMEBUFFER_ATTACHMENT_ANGLE, true, size);

        if (mTexStorage != NULL)
        {
            if (!mRenderer->copyToRenderTarget(newTexStorage, mTexStorage))
            {
                delete newTexStorage;
                return gl::error(GL_OUT_OF_MEMORY);
            }
        }
    }

    delete mTexStorage;
    mTexStorage = newTexStorage;

    mDirtyImages = true;
}

void TextureCubeMap::setImage(int faceIndex, GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
    GLint internalformat = ConvertSizedInternalFormat(format, type);
    redefineImage(faceIndex, level, internalformat, width, height);

    Texture::setImage(unpackAlignment, pixels, mImageArray[faceIndex][level]);
}

unsigned int TextureCubeMap::faceIndex(GLenum face)
{
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_X - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 1);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_POSITIVE_Y - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 2);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 3);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_POSITIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 4);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 5);

    return face - GL_TEXTURE_CUBE_MAP_POSITIVE_X;
}

void TextureCubeMap::redefineImage(int face, GLint level, GLint internalformat, GLsizei width, GLsizei height)
{
    // If there currently is a corresponding storage texture image, it has these parameters
    const int storageWidth = std::max(1, mImageArray[0][0]->getWidth() >> level);
    const int storageHeight = std::max(1, mImageArray[0][0]->getHeight() >> level);
    const int storageFormat = mImageArray[0][0]->getInternalFormat();

    mImageArray[face][level]->redefine(mRenderer, internalformat, width, height, false);

    if (mTexStorage)
    {
        const int storageLevels = mTexStorage->levelCount();
        
        if ((level >= storageLevels && storageLevels != 0) ||
            width != storageWidth ||
            height != storageHeight ||
            internalformat != storageFormat)   // Discard mismatched storage
        {
            for (int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
            {
                for (int f = 0; f < 6; f++)
                {
                    mImageArray[f][i]->markDirty();
                }
            }

            delete mTexStorage;
            mTexStorage = NULL;

            mDirtyImages = true;
        }
    }
}

void TextureCubeMap::copyImage(GLenum target, GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    unsigned int faceindex = faceIndex(target);
    GLint internalformat = gl::ConvertSizedInternalFormat(format, GL_UNSIGNED_BYTE);
    redefineImage(faceindex, level, internalformat, width, height);

    if (!mImageArray[faceindex][level]->isRenderableFormat())
    {
        mImageArray[faceindex][level]->copy(0, 0, x, y, width, height, source);
        mDirtyImages = true;
    }
    else
    {
        if (!mTexStorage || !mTexStorage->isRenderTarget())
        {
            convertToRenderTarget();
        }
        
        mImageArray[faceindex][level]->markClean();

        ASSERT(width == height);

        if (width > 0 && level < levelCount())
        {
            gl::Rectangle sourceRect;
            sourceRect.x = x;
            sourceRect.width = width;
            sourceRect.y = y;
            sourceRect.height = height;

            mRenderer->copyImage(source, sourceRect, format, 0, 0, mTexStorage, target, level);
        }
    }
}

void TextureCubeMap::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    GLsizei size = mImageArray[faceIndex(target)][level]->getWidth();

    if (xoffset + width > size || yoffset + height > size)
    {
        return gl::error(GL_INVALID_VALUE);
    }

    unsigned int faceindex = faceIndex(target);

    if (!mImageArray[faceindex][level]->isRenderableFormat() || (!mTexStorage && !isSamplerComplete()))
    {
        mImageArray[faceindex][level]->copy(0, 0, x, y, width, height, source);
        mDirtyImages = true;
    }
    else
    {
        if (!mTexStorage || !mTexStorage->isRenderTarget())
        {
            convertToRenderTarget();
        }
        
        updateTexture();

        if (level < levelCount())
        {
            gl::Rectangle sourceRect;
            sourceRect.x = x;
            sourceRect.width = width;
            sourceRect.y = y;
            sourceRect.height = height;

            mRenderer->copyImage(source, sourceRect, gl::ExtractFormat(mImageArray[0][0]->getInternalFormat()), 
                                 xoffset, yoffset, mTexStorage, target, level);
        }
    }
}

void TextureCubeMap::storage(GLsizei levels, GLenum internalformat, GLsizei size)
{
    delete mTexStorage;
    mTexStorage = new rx::TextureStorageInterfaceCube(mRenderer, levels, internalformat, mUsage, false, size);
    mImmutable = true;

    for (int level = 0; level < levels; level++)
    {
        for (int face = 0; face < 6; face++)
        {
            mImageArray[face][level]->redefine(mRenderer, internalformat, size, size, true);
            size = std::max(1, size >> 1);
        }
    }

    for (int level = levels; level < IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
    {
        for (int face = 0; face < 6; face++)
        {
            mImageArray[face][level]->redefine(mRenderer, GL_NONE, 0, 0, true);
        }
    }

    if (mTexStorage->isManaged())
    {
        int levels = levelCount();

        for (int face = 0; face < 6; face++)
        {
            for (int level = 0; level < levels; level++)
            {
                mImageArray[face][level]->setManagedSurface(mTexStorage, face, level);
            }
        }
    }
}

void TextureCubeMap::generateMipmaps()
{
    if (!isCubeComplete())
    {
        return gl::error(GL_INVALID_OPERATION);
    }

    if (!mRenderer->getNonPower2TextureSupport())
    {
        if (!isPow2(mImageArray[0][0]->getWidth()))
        {
            return gl::error(GL_INVALID_OPERATION);
        }
    }

    // Purge array levels 1 through q and reset them to represent the generated mipmap levels.
    unsigned int q = log2(mImageArray[0][0]->getWidth());
    for (unsigned int f = 0; f < 6; f++)
    {
        for (unsigned int i = 1; i <= q; i++)
        {
            redefineImage(f, i, mImageArray[f][0]->getInternalFormat(),
                          std::max(mImageArray[f][0]->getWidth() >> i, 1),
                          std::max(mImageArray[f][0]->getWidth() >> i, 1));
        }
    }

    if (mTexStorage && mTexStorage->isRenderTarget())
    {
        for (unsigned int f = 0; f < 6; f++)
        {
            for (unsigned int i = 1; i <= q; i++)
            {
                mTexStorage->generateMipmap(f, i);

                mImageArray[f][i]->markClean();
            }
        }
    }
    else
    {
        for (unsigned int f = 0; f < 6; f++)
        {
            for (unsigned int i = 1; i <= q; i++)
            {
                mRenderer->generateMipmap(mImageArray[f][i], mImageArray[f][i - 1]);
            }
        }
    }
}

Renderbuffer *TextureCubeMap::getRenderbuffer(GLenum target)
{
    if (!IsCubemapTextureTarget(target))
    {
        return gl::error(GL_INVALID_OPERATION, (Renderbuffer *)NULL);
    }

    unsigned int face = faceIndex(target);

    if (mFaceProxies[face] == NULL)
    {
        mFaceProxies[face] = new Renderbuffer(mRenderer, id(), new RenderbufferTextureCubeMap(this, target));
    }

    return mFaceProxies[face];
}

rx::RenderTarget *TextureCubeMap::getRenderTarget(GLenum target)
{
    ASSERT(IsCubemapTextureTarget(target));

    // ensure the underlying texture is created
    if (getStorage(true) == NULL)
    {
        return NULL;
    }

    updateTexture();
    
    return mTexStorage->getRenderTarget(target);
}

int TextureCubeMap::levelCount()
{
    return mTexStorage ? mTexStorage->levelCount() - getLodOffset() : 0;
}

rx::TextureStorageInterface *TextureCubeMap::getStorage(bool renderTarget)
{
    if (!mTexStorage || (renderTarget && !mTexStorage->isRenderTarget()))
    {
        if (renderTarget)
        {
            convertToRenderTarget();
        }
        else
        {
            createTexture();
        }
    }

    return mTexStorage;
}

}
