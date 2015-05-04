//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Texture.cpp: Implements the gl::Texture class and its derived classes
// Texture2D and TextureCubeMap. Implements GL texture objects and related
// functionality. [OpenGL ES 2.0.24] section 3.7 page 63.

#include "libGLESv2/Texture.h"
#include "libGLESv2/main.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/formatutils.h"
#include "libGLESv2/ImageIndex.h"
#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/renderer/Image.h"
#include "libGLESv2/renderer/d3d/TextureStorage.h"

#include "libEGL/Surface.h"

#include "common/mathutil.h"
#include "common/utilities.h"

namespace gl
{

bool IsMipmapFiltered(const gl::SamplerState &samplerState)
{
    switch (samplerState.minFilter)
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

bool IsPointSampled(const gl::SamplerState &samplerState)
{
    return (samplerState.magFilter == GL_NEAREST && (samplerState.minFilter == GL_NEAREST || samplerState.minFilter == GL_NEAREST_MIPMAP_NEAREST));
}

unsigned int Texture::mCurrentTextureSerial = 1;

Texture::Texture(rx::TextureImpl *impl, GLuint id, GLenum target)
    : RefCountObject(id),
      mTexture(impl),
      mTextureSerial(issueTextureSerial()),
      mUsage(GL_NONE),
      mImmutableLevelCount(0),
      mTarget(target)
{
}

Texture::~Texture()
{
    SafeDelete(mTexture);
}

GLenum Texture::getTarget() const
{
    return mTarget;
}

void Texture::setUsage(GLenum usage)
{
    mUsage = usage;
    getImplementation()->setUsage(usage);
}

GLenum Texture::getUsage() const
{
    return mUsage;
}

GLint Texture::getBaseLevelWidth() const
{
    const rx::Image *baseImage = getBaseLevelImage();
    return (baseImage ? baseImage->getWidth() : 0);
}

GLint Texture::getBaseLevelHeight() const
{
    const rx::Image *baseImage = getBaseLevelImage();
    return (baseImage ? baseImage->getHeight() : 0);
}

GLint Texture::getBaseLevelDepth() const
{
    const rx::Image *baseImage = getBaseLevelImage();
    return (baseImage ? baseImage->getDepth() : 0);
}

// Note: "base level image" is loosely defined to be any image from the base level,
// where in the base of 2D array textures and cube maps there are several. Don't use
// the base level image for anything except querying texture format and size.
GLenum Texture::getBaseLevelInternalFormat() const
{
    const rx::Image *baseImage = getBaseLevelImage();
    return (baseImage ? baseImage->getInternalFormat() : GL_NONE);
}

GLsizei Texture::getWidth(const ImageIndex &index) const
{
    rx::Image *image = mTexture->getImage(index);
    return image->getWidth();
}

GLsizei Texture::getHeight(const ImageIndex &index) const
{
    rx::Image *image = mTexture->getImage(index);
    return image->getHeight();
}

GLenum Texture::getInternalFormat(const ImageIndex &index) const
{
    rx::Image *image = mTexture->getImage(index);
    return image->getInternalFormat();
}

GLenum Texture::getActualFormat(const ImageIndex &index) const
{
    rx::Image *image = mTexture->getImage(index);
    return image->getActualFormat();
}

Error Texture::generateMipmaps()
{
    return getImplementation()->generateMipmaps();
}

Error Texture::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                           GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    return mTexture->copySubImage(target, level, xoffset, yoffset, zoffset, x, y, width, height, source);
}

unsigned int Texture::getTextureSerial() const
{
    return mTextureSerial;
}

unsigned int Texture::issueTextureSerial()
{
    return mCurrentTextureSerial++;
}

bool Texture::isImmutable() const
{
    return (mImmutableLevelCount > 0);
}

int Texture::immutableLevelCount()
{
    return mImmutableLevelCount;
}

int Texture::mipLevels() const
{
    return log2(std::max(std::max(getBaseLevelWidth(), getBaseLevelHeight()), getBaseLevelDepth())) + 1;
}

const rx::Image *Texture::getBaseLevelImage() const
{
    return (getImplementation()->getLayerCount(0) > 0 ? getImplementation()->getImage(0, 0) : NULL);
}

Texture2D::Texture2D(rx::TextureImpl *impl, GLuint id)
    : Texture(impl, id, GL_TEXTURE_2D)
{
    mSurface = NULL;
}

Texture2D::~Texture2D()
{
    if (mSurface)
    {
        mSurface->setBoundTexture(NULL);
        mSurface = NULL;
    }
}

GLsizei Texture2D::getWidth(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, 0)->getWidth();
    else
        return 0;
}

GLsizei Texture2D::getHeight(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, 0)->getHeight();
    else
        return 0;
}

GLenum Texture2D::getInternalFormat(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, 0)->getInternalFormat();
    else
        return GL_NONE;
}

GLenum Texture2D::getActualFormat(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, 0)->getActualFormat();
    else
        return GL_NONE;
}

Error Texture2D::setImage(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    releaseTexImage();

    return mTexture->setImage(GL_TEXTURE_2D, level, width, height, 1, internalFormat, format, type, unpack, pixels);
}

void Texture2D::bindTexImage(egl::Surface *surface)
{
    releaseTexImage();

    mTexture->bindTexImage(surface);

    mSurface = surface;
    mSurface->setBoundTexture(this);
}

void Texture2D::releaseTexImage()
{
    if (mSurface)
    {
        mSurface->setBoundTexture(NULL);
        mSurface = NULL;

        mTexture->releaseTexImage();
    }
}

Error Texture2D::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize,
                                    const PixelUnpackState &unpack, const void *pixels)
{
    releaseTexImage();

    return mTexture->setCompressedImage(GL_TEXTURE_2D, level, format, width, height, 1, imageSize, unpack, pixels);
}

Error Texture2D::subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    return mTexture->subImage(GL_TEXTURE_2D, level, xoffset, yoffset, 0, width, height, 1, format, type, unpack, pixels);
}

Error Texture2D::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                    GLenum format, GLsizei imageSize, const PixelUnpackState &unpack, const void *pixels)
{
    return mTexture->subImageCompressed(GL_TEXTURE_2D, level, xoffset, yoffset, 0, width, height, 1, format, imageSize, unpack, pixels);
}

Error Texture2D::copyImage(GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height,
                           Framebuffer *source)
{
    releaseTexImage();

    return mTexture->copyImage(GL_TEXTURE_2D, level, format, x, y, width, height, source);
}

Error Texture2D::storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
    Error error = mTexture->storage(GL_TEXTURE_2D, levels, internalformat, width, height, 1);
    if (error.isError())
    {
        return error;
    }

    mImmutableLevelCount = levels;

    return Error(GL_NO_ERROR);
}

// Tests for 2D texture sampling completeness. [OpenGL ES 2.0.24] section 3.8.2 page 85.
bool Texture2D::isSamplerComplete(const SamplerState &samplerState, const TextureCapsMap &textureCaps, const Extensions &extensions, int clientVersion) const
{
    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();

    if (width <= 0 || height <= 0)
    {
        return false;
    }

    if (!textureCaps.get(getInternalFormat(0)).filterable && !IsPointSampled(samplerState))
    {
        return false;
    }

    bool npotSupport = extensions.textureNPOT;

    if (!npotSupport)
    {
        if ((samplerState.wrapS != GL_CLAMP_TO_EDGE && !gl::isPow2(width)) ||
            (samplerState.wrapT != GL_CLAMP_TO_EDGE && !gl::isPow2(height)))
        {
            return false;
        }
    }

    if (IsMipmapFiltered(samplerState))
    {
        if (!npotSupport)
        {
            if (!gl::isPow2(width) || !gl::isPow2(height))
            {
                return false;
            }
        }

        if (!isMipmapComplete())
        {
            return false;
        }
    }

    // OpenGLES 3.0.2 spec section 3.8.13 states that a texture is not mipmap complete if:
    // The internalformat specified for the texture arrays is a sized internal depth or
    // depth and stencil format (see table 3.13), the value of TEXTURE_COMPARE_-
    // MODE is NONE, and either the magnification filter is not NEAREST or the mini-
    // fication filter is neither NEAREST nor NEAREST_MIPMAP_NEAREST.
    const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(getInternalFormat(0));
    if (formatInfo.depthBits > 0 && clientVersion > 2)
    {
        if (samplerState.compareMode == GL_NONE)
        {
            if ((samplerState.minFilter != GL_NEAREST && samplerState.minFilter != GL_NEAREST_MIPMAP_NEAREST) ||
                samplerState.magFilter != GL_NEAREST)
            {
                return false;
            }
        }
    }

    return true;
}

bool Texture2D::isCompressed(GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(level)).compressed;
}

bool Texture2D::isDepth(GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(level)).depthBits > 0;
}

Error Texture2D::generateMipmaps()
{
    releaseTexImage();

    return mTexture->generateMipmaps();
}

// Tests for 2D texture (mipmap) completeness. [OpenGL ES 2.0.24] section 3.7.10 page 81.
bool Texture2D::isMipmapComplete() const
{
    int levelCount = mipLevels();

    for (int level = 0; level < levelCount; level++)
    {
        if (!isLevelComplete(level))
        {
            return false;
        }
    }

    return true;
}

bool Texture2D::isLevelComplete(int level) const
{
    if (isImmutable())
    {
        return true;
    }

    const rx::Image *baseImage = getBaseLevelImage();

    GLsizei width = baseImage->getWidth();
    GLsizei height = baseImage->getHeight();

    if (width <= 0 || height <= 0)
    {
        return false;
    }

    // The base image level is complete if the width and height are positive
    if (level == 0)
    {
        return true;
    }

    ASSERT(level >= 1 && level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mTexture->getImage(level, 0) != NULL);
    rx::Image *image = mTexture->getImage(level, 0);

    if (image->getInternalFormat() != baseImage->getInternalFormat())
    {
        return false;
    }

    if (image->getWidth() != std::max(1, width >> level))
    {
        return false;
    }

    if (image->getHeight() != std::max(1, height >> level))
    {
        return false;
    }

    return true;
}

TextureCubeMap::TextureCubeMap(rx::TextureImpl *impl, GLuint id)
    : Texture(impl, id, GL_TEXTURE_CUBE_MAP)
{
}

TextureCubeMap::~TextureCubeMap()
{
}

GLsizei TextureCubeMap::getWidth(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, targetToLayerIndex(target))->getWidth();
    else
        return 0;
}

GLsizei TextureCubeMap::getHeight(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, targetToLayerIndex(target))->getHeight();
    else
        return 0;
}

GLenum TextureCubeMap::getInternalFormat(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, targetToLayerIndex(target))->getInternalFormat();
    else
        return GL_NONE;
}

GLenum TextureCubeMap::getActualFormat(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, targetToLayerIndex(target))->getActualFormat();
    else
        return GL_NONE;
}

Error TextureCubeMap::setImage(GLenum target, GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    return mTexture->setImage(target, level, width, height, 1, internalFormat, format, type, unpack, pixels);
}

Error TextureCubeMap::setCompressedImage(GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height,
                                         GLsizei imageSize, const PixelUnpackState &unpack, const void *pixels)
{
    return mTexture->setCompressedImage(target, level, format, width, height, 1, imageSize, unpack, pixels);
}

Error TextureCubeMap::subImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    return mTexture->subImage(target, level, xoffset, yoffset, 0, width, height, 1, format, type, unpack, pixels);
}

Error TextureCubeMap::subImageCompressed(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                         GLsizei width, GLsizei height, GLenum format,
                                         GLsizei imageSize, const PixelUnpackState &unpack, const void *pixels)
{
    return mTexture->subImageCompressed(target, level, xoffset, yoffset, 0, width, height, 1, format, imageSize, unpack, pixels);
}

// Tests for cube texture completeness. [OpenGL ES 2.0.24] section 3.7.10 page 81.
bool TextureCubeMap::isCubeComplete() const
{
    int    baseWidth  = getBaseLevelWidth();
    int    baseHeight = getBaseLevelHeight();
    GLenum baseFormat = getBaseLevelInternalFormat();

    if (baseWidth <= 0 || baseWidth != baseHeight)
    {
        return false;
    }

    for (int faceIndex = 1; faceIndex < 6; faceIndex++)
    {
        const rx::Image *faceBaseImage = mTexture->getImage(0, faceIndex);

        if (faceBaseImage->getWidth()          != baseWidth  ||
            faceBaseImage->getHeight()         != baseHeight ||
            faceBaseImage->getInternalFormat() != baseFormat )
        {
            return false;
        }
    }

    return true;
}

bool TextureCubeMap::isCompressed(GLenum target, GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(target, level)).compressed;
}

bool TextureCubeMap::isDepth(GLenum target, GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(target, level)).depthBits > 0;
}

Error TextureCubeMap::copyImage(GLenum target, GLint level, GLenum format, GLint x, GLint y,
                                GLsizei width, GLsizei height, Framebuffer *source)
{
    return mTexture->copyImage(target, level, format, x, y, width, height, source);
}

Error TextureCubeMap::storage(GLsizei levels, GLenum internalformat, GLsizei size)
{
    Error error = mTexture->storage(GL_TEXTURE_CUBE_MAP, levels, internalformat, size, size, 1);
    if (error.isError())
    {
        return error;
    }

    mImmutableLevelCount = levels;

    return Error(GL_NO_ERROR);
}

// Tests for texture sampling completeness
bool TextureCubeMap::isSamplerComplete(const SamplerState &samplerState, const TextureCapsMap &textureCaps, const Extensions &extensions, int clientVersion) const
{
    int size = getBaseLevelWidth();

    bool mipmapping = IsMipmapFiltered(samplerState);

    if (!textureCaps.get(getInternalFormat(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0)).filterable && !IsPointSampled(samplerState))
    {
        return false;
    }

    if (!gl::isPow2(size) && !extensions.textureNPOT)
    {
        if (samplerState.wrapS != GL_CLAMP_TO_EDGE || samplerState.wrapT != GL_CLAMP_TO_EDGE || mipmapping)
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
        if (!isMipmapComplete())   // Also tests for isCubeComplete()
        {
            return false;
        }
    }

    return true;
}

int TextureCubeMap::targetToLayerIndex(GLenum target)
{
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_X - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 1);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_POSITIVE_Y - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 2);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 3);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_POSITIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 4);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 5);

    return target - GL_TEXTURE_CUBE_MAP_POSITIVE_X;
}

GLenum TextureCubeMap::layerIndexToTarget(GLint layer)
{
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_X - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 1);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_POSITIVE_Y - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 2);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 3);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_POSITIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 4);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 5);

    return GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer;
}

bool TextureCubeMap::isMipmapComplete() const
{
    if (isImmutable())
    {
        return true;
    }

    if (!isCubeComplete())
    {
        return false;
    }

    int levelCount = mipLevels();

    for (int face = 0; face < 6; face++)
    {
        for (int level = 1; level < levelCount; level++)
        {
            if (!isFaceLevelComplete(face, level))
            {
                return false;
            }
        }
    }

    return true;
}

bool TextureCubeMap::isFaceLevelComplete(int faceIndex, int level) const
{
    ASSERT(level >= 0 && faceIndex < 6 && level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mTexture->getImage(level, faceIndex) != NULL);

    if (isImmutable())
    {
        return true;
    }

    int baseSize = getBaseLevelWidth();

    if (baseSize <= 0)
    {
        return false;
    }

    // "isCubeComplete" checks for base level completeness and we must call that
    // to determine if any face at level 0 is complete. We omit that check here
    // to avoid re-checking cube-completeness for every face at level 0.
    if (level == 0)
    {
        return true;
    }

    // Check that non-zero levels are consistent with the base level.
    const rx::Image *faceLevelImage = mTexture->getImage(level, faceIndex);

    if (faceLevelImage->getInternalFormat() != getBaseLevelInternalFormat())
    {
        return false;
    }

    if (faceLevelImage->getWidth() != std::max(1, baseSize >> level))
    {
        return false;
    }

    return true;
}


Texture3D::Texture3D(rx::TextureImpl *impl, GLuint id)
    : Texture(impl, id, GL_TEXTURE_3D)
{
}

Texture3D::~Texture3D()
{
}

GLsizei Texture3D::getWidth(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getImage(level, 0)->getWidth() : 0;
}

GLsizei Texture3D::getHeight(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getImage(level, 0)->getHeight() : 0;
}

GLsizei Texture3D::getDepth(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getImage(level, 0)->getDepth() : 0;
}

GLenum Texture3D::getInternalFormat(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getImage(level, 0)->getInternalFormat() : GL_NONE;
}

GLenum Texture3D::getActualFormat(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getImage(level, 0)->getActualFormat() : GL_NONE;
}

bool Texture3D::isCompressed(GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(level)).compressed;
}

bool Texture3D::isDepth(GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(level)).depthBits > 0;
}

Error Texture3D::setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    return mTexture->setImage(GL_TEXTURE_3D, level, width, height, depth, internalFormat, format, type, unpack, pixels);
}

Error Texture3D::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth,
                                    GLsizei imageSize, const PixelUnpackState &unpack, const void *pixels)
{
    return mTexture->setCompressedImage(GL_TEXTURE_3D, level, format, width, height, depth, imageSize, unpack, pixels);
}

Error Texture3D::subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    return mTexture->subImage(GL_TEXTURE_3D, level, xoffset, yoffset, zoffset, width, height, depth, format, type, unpack, pixels);
}

Error Texture3D::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                                    GLsizei width, GLsizei height, GLsizei depth, GLenum format,
                                    GLsizei imageSize, const PixelUnpackState &unpack, const void *pixels)
{
    return mTexture->subImageCompressed(GL_TEXTURE_3D, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, unpack, pixels);
}

Error Texture3D::storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    Error error = mTexture->storage(GL_TEXTURE_3D, levels, internalformat, width, height, depth);
    if (error.isError())
    {
        return error;
    }

    mImmutableLevelCount = levels;

    return Error(GL_NO_ERROR);
}

bool Texture3D::isSamplerComplete(const SamplerState &samplerState, const TextureCapsMap &textureCaps, const Extensions &extensions, int clientVersion) const
{
    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();
    GLsizei depth = getBaseLevelDepth();

    if (width <= 0 || height <= 0 || depth <= 0)
    {
        return false;
    }

    if (!textureCaps.get(getInternalFormat(0)).filterable && !IsPointSampled(samplerState))
    {
        return false;
    }

    if (IsMipmapFiltered(samplerState) && !isMipmapComplete())
    {
        return false;
    }

    return true;
}

bool Texture3D::isMipmapComplete() const
{
    int levelCount = mipLevels();

    for (int level = 0; level < levelCount; level++)
    {
        if (!isLevelComplete(level))
        {
            return false;
        }
    }

    return true;
}

bool Texture3D::isLevelComplete(int level) const
{
    ASSERT(level >= 0 && level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mTexture->getImage(level, 0) != NULL);

    if (isImmutable())
    {
        return true;
    }

    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();
    GLsizei depth = getBaseLevelDepth();

    if (width <= 0 || height <= 0 || depth <= 0)
    {
        return false;
    }

    if (level == 0)
    {
        return true;
    }

    rx::Image *levelImage = mTexture->getImage(level, 0);

    if (levelImage->getInternalFormat() != getBaseLevelInternalFormat())
    {
        return false;
    }

    if (levelImage->getWidth() != std::max(1, width >> level))
    {
        return false;
    }

    if (levelImage->getHeight() != std::max(1, height >> level))
    {
        return false;
    }

    if (levelImage->getDepth() != std::max(1, depth >> level))
    {
        return false;
    }

    return true;
}

Texture2DArray::Texture2DArray(rx::TextureImpl *impl, GLuint id)
    : Texture(impl, id, GL_TEXTURE_2D_ARRAY)
{
}

Texture2DArray::~Texture2DArray()
{
}

GLsizei Texture2DArray::getWidth(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mTexture->getLayerCount(level) > 0) ? mTexture->getImage(level, 0)->getWidth() : 0;
}

GLsizei Texture2DArray::getHeight(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mTexture->getLayerCount(level) > 0) ? mTexture->getImage(level, 0)->getHeight() : 0;
}

GLsizei Texture2DArray::getLayers(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getLayerCount(level) : 0;
}

GLenum Texture2DArray::getInternalFormat(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mTexture->getLayerCount(level) > 0) ? mTexture->getImage(level, 0)->getInternalFormat() : GL_NONE;
}

GLenum Texture2DArray::getActualFormat(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mTexture->getLayerCount(level) > 0) ? mTexture->getImage(level, 0)->getActualFormat() : GL_NONE;
}

bool Texture2DArray::isCompressed(GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(level)).compressed;
}

bool Texture2DArray::isDepth(GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(level)).depthBits > 0;
}

Error Texture2DArray::setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    return mTexture->setImage(GL_TEXTURE_2D_ARRAY, level, width, height, depth, internalFormat, format, type, unpack, pixels);
}

Error Texture2DArray::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth,
                                         GLsizei imageSize, const PixelUnpackState &unpack, const void *pixels)
{
    return mTexture->setCompressedImage(GL_TEXTURE_2D_ARRAY, level, format, width, height, depth, imageSize, unpack, pixels);
}

Error Texture2DArray::subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    return mTexture->subImage(GL_TEXTURE_2D_ARRAY, level, xoffset, yoffset, zoffset, width, height, depth, format, type, unpack, pixels);
}

Error Texture2DArray::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                                         GLsizei width, GLsizei height, GLsizei depth, GLenum format,
                                         GLsizei imageSize, const PixelUnpackState &unpack, const void *pixels)
{
    return mTexture->subImageCompressed(GL_TEXTURE_2D_ARRAY, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, unpack, pixels);
}

Error Texture2DArray::storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    Error error = mTexture->storage(GL_TEXTURE_2D_ARRAY, levels, internalformat, width, height, depth);
    if (error.isError())
    {
        return error;
    }

    mImmutableLevelCount = levels;

    return Error(GL_NO_ERROR);
}

bool Texture2DArray::isSamplerComplete(const SamplerState &samplerState, const TextureCapsMap &textureCaps, const Extensions &extensions, int clientVersion) const
{
    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();
    GLsizei depth = getLayers(0);

    if (width <= 0 || height <= 0 || depth <= 0)
    {
        return false;
    }

    if (!textureCaps.get(getBaseLevelInternalFormat()).filterable && !IsPointSampled(samplerState))
    {
        return false;
    }

    if (IsMipmapFiltered(samplerState) && !isMipmapComplete())
    {
        return false;
    }

    return true;
}

bool Texture2DArray::isMipmapComplete() const
{
    int levelCount = mipLevels();

    for (int level = 1; level < levelCount; level++)
    {
        if (!isLevelComplete(level))
        {
            return false;
        }
    }

    return true;
}

bool Texture2DArray::isLevelComplete(int level) const
{
    ASSERT(level >= 0 && level < IMPLEMENTATION_MAX_TEXTURE_LEVELS);

    if (isImmutable())
    {
        return true;
    }

    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();
    GLsizei layers = getLayers(0);

    if (width <= 0 || height <= 0 || layers <= 0)
    {
        return false;
    }

    if (level == 0)
    {
        return true;
    }

    if (getInternalFormat(level) != getInternalFormat(0))
    {
        return false;
    }

    if (getWidth(level) != std::max(1, width >> level))
    {
        return false;
    }

    if (getHeight(level) != std::max(1, height >> level))
    {
        return false;
    }

    if (getLayers(level) != layers)
    {
        return false;
    }

    return true;
}

}
