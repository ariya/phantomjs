//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ImageIndex.cpp: Implementation for ImageIndex methods.

#include "libGLESv2/ImageIndex.h"
#include "libGLESv2/Texture.h"
#include "common/utilities.h"

namespace gl
{

ImageIndex::ImageIndex(const ImageIndex &other)
    : type(other.type),
      mipIndex(other.mipIndex),
      layerIndex(other.layerIndex)
{}

ImageIndex &ImageIndex::operator=(const ImageIndex &other)
{
    type = other.type;
    mipIndex = other.mipIndex;
    layerIndex = other.layerIndex;
    return *this;
}

ImageIndex ImageIndex::Make2D(GLint mipIndex)
{
    return ImageIndex(GL_TEXTURE_2D, mipIndex, ENTIRE_LEVEL);
}

ImageIndex ImageIndex::MakeCube(GLenum target, GLint mipIndex)
{
    ASSERT(gl::IsCubemapTextureTarget(target));
    return ImageIndex(target, mipIndex, TextureCubeMap::targetToLayerIndex(target));
}

ImageIndex ImageIndex::Make2DArray(GLint mipIndex, GLint layerIndex)
{
    return ImageIndex(GL_TEXTURE_2D_ARRAY, mipIndex, layerIndex);
}

ImageIndex ImageIndex::Make3D(GLint mipIndex, GLint layerIndex)
{
    return ImageIndex(GL_TEXTURE_3D, mipIndex, layerIndex);
}

ImageIndex ImageIndex::MakeInvalid()
{
    return ImageIndex(GL_NONE, -1, -1);
}

ImageIndex::ImageIndex(GLenum typeIn, GLint mipIndexIn, GLint layerIndexIn)
    : type(typeIn),
      mipIndex(mipIndexIn),
      layerIndex(layerIndexIn)
{}

ImageIndexIterator ImageIndexIterator::Make2D(GLint minMip, GLint maxMip)
{
    return ImageIndexIterator(GL_TEXTURE_2D, rx::Range<GLint>(minMip, maxMip),
                              rx::Range<GLint>(ImageIndex::ENTIRE_LEVEL, ImageIndex::ENTIRE_LEVEL), NULL);
}

ImageIndexIterator ImageIndexIterator::MakeCube(GLint minMip, GLint maxMip)
{
    return ImageIndexIterator(GL_TEXTURE_CUBE_MAP, rx::Range<GLint>(minMip, maxMip), rx::Range<GLint>(0, 6), NULL);
}

ImageIndexIterator ImageIndexIterator::Make3D(GLint minMip, GLint maxMip,
                                              GLint minLayer, GLint maxLayer)
{
    return ImageIndexIterator(GL_TEXTURE_3D, rx::Range<GLint>(minMip, maxMip), rx::Range<GLint>(minLayer, maxLayer), NULL);
}

ImageIndexIterator ImageIndexIterator::Make2DArray(GLint minMip, GLint maxMip,
                                                   const GLsizei *layerCounts)
{
    return ImageIndexIterator(GL_TEXTURE_2D_ARRAY, rx::Range<GLint>(minMip, maxMip),
                              rx::Range<GLint>(0, IMPLEMENTATION_MAX_2D_ARRAY_TEXTURE_LAYERS), layerCounts);
}

ImageIndexIterator::ImageIndexIterator(GLenum type, const rx::Range<GLint> &mipRange,
                                       const rx::Range<GLint> &layerRange, const GLsizei *layerCounts)
    : mType(type),
      mMipRange(mipRange),
      mLayerRange(layerRange),
      mLayerCounts(layerCounts),
      mCurrentMip(mipRange.start),
      mCurrentLayer(layerRange.start)
{}

GLint ImageIndexIterator::maxLayer() const
{
    return (mLayerCounts ? static_cast<GLint>(mLayerCounts[mCurrentMip]) : mLayerRange.end);
}

ImageIndex ImageIndexIterator::next()
{
    ASSERT(hasNext());

    ImageIndex value = current();

    // Iterate layers in the inner loop for now. We can add switchable
    // layer or mip iteration if we need it.

    if (mCurrentLayer != ImageIndex::ENTIRE_LEVEL)
    {
        if (mCurrentLayer < maxLayer()-1)
        {
            mCurrentLayer++;
        }
        else if (mCurrentMip < mMipRange.end-1)
        {
            mCurrentMip++;
            mCurrentLayer = mLayerRange.start;
        }
    }
    else if (mCurrentMip < mMipRange.end-1)
    {
        mCurrentMip++;
        mCurrentLayer = mLayerRange.start;
    }

    return value;
}

ImageIndex ImageIndexIterator::current() const
{
    ImageIndex value(mType, mCurrentMip, mCurrentLayer);

    if (mType == GL_TEXTURE_CUBE_MAP)
    {
        value.type = TextureCubeMap::layerIndexToTarget(mCurrentLayer);
    }

    return value;
}

bool ImageIndexIterator::hasNext() const
{
    return (mCurrentMip < mMipRange.end || mCurrentLayer < maxLayer());
}

}
