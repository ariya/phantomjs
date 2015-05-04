//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// angletypes.h : Defines a variety of structures and enum types that are used throughout libGLESv2

#include "libGLESv2/angletypes.h"
#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/VertexAttribute.h"
#include "libGLESv2/State.h"
#include "libGLESv2/VertexArray.h"

#include <float.h>

namespace gl
{

SamplerState::SamplerState()
    : minFilter(GL_NEAREST_MIPMAP_LINEAR),
      magFilter(GL_LINEAR),
      wrapS(GL_REPEAT),
      wrapT(GL_REPEAT),
      wrapR(GL_REPEAT),
      maxAnisotropy(1.0f),
      baseLevel(0),
      maxLevel(1000),
      minLod(-FLT_MAX),
      maxLod(FLT_MAX),
      compareMode(GL_NONE),
      compareFunc(GL_LEQUAL),
      swizzleRed(GL_RED),
      swizzleGreen(GL_GREEN),
      swizzleBlue(GL_BLUE),
      swizzleAlpha(GL_ALPHA)
{}

bool SamplerState::swizzleRequired() const
{
    return swizzleRed != GL_RED || swizzleGreen != GL_GREEN ||
           swizzleBlue != GL_BLUE || swizzleAlpha != GL_ALPHA;
}

static void MinMax(int a, int b, int *minimum, int *maximum)
{
    if (a < b)
    {
        *minimum = a;
        *maximum = b;
    }
    else
    {
        *minimum = b;
        *maximum = a;
    }
}

bool ClipRectangle(const Rectangle &source, const Rectangle &clip, Rectangle *intersection)
{
    int minSourceX, maxSourceX, minSourceY, maxSourceY;
    MinMax(source.x, source.x + source.width, &minSourceX, &maxSourceX);
    MinMax(source.y, source.y + source.height, &minSourceY, &maxSourceY);

    int minClipX, maxClipX, minClipY, maxClipY;
    MinMax(clip.x, clip.x + clip.width, &minClipX, &maxClipX);
    MinMax(clip.y, clip.y + clip.height, &minClipY, &maxClipY);

    if (minSourceX >= maxClipX || maxSourceX <= minClipX || minSourceY >= maxClipY || maxSourceY <= minClipY)
    {
        if (intersection)
        {
            intersection->x = minSourceX;
            intersection->y = maxSourceY;
            intersection->width = maxSourceX - minSourceX;
            intersection->height = maxSourceY - minSourceY;
        }

        return false;
    }
    else
    {
        if (intersection)
        {
            intersection->x = std::max(minSourceX, minClipX);
            intersection->y = std::max(minSourceY, minClipY);
            intersection->width  = std::min(maxSourceX, maxClipX) - std::max(minSourceX, minClipX);
            intersection->height = std::min(maxSourceY, maxClipY) - std::max(minSourceY, minClipY);
        }

        return true;
    }
}

VertexFormat::VertexFormat()
    : mType(GL_NONE),
      mNormalized(GL_FALSE),
      mComponents(0),
      mPureInteger(false)
{}

VertexFormat::VertexFormat(GLenum type, GLboolean normalized, GLuint components, bool pureInteger)
    : mType(type),
      mNormalized(normalized),
      mComponents(components),
      mPureInteger(pureInteger)
{
    // Float data can not be normalized, so ignore the user setting
    if (mType == GL_FLOAT || mType == GL_HALF_FLOAT || mType == GL_FIXED)
    {
        mNormalized = GL_FALSE;
    }
}

VertexFormat::VertexFormat(const VertexAttribute &attrib)
    : mType(attrib.type),
      mNormalized(attrib.normalized ? GL_TRUE : GL_FALSE),
      mComponents(attrib.size),
      mPureInteger(attrib.pureInteger)
{
    // Ensure we aren't initializing a vertex format which should be using
    // the current-value type
    ASSERT(attrib.enabled);

    // Float data can not be normalized, so ignore the user setting
    if (mType == GL_FLOAT || mType == GL_HALF_FLOAT || mType == GL_FIXED)
    {
        mNormalized = GL_FALSE;
    }
}

VertexFormat::VertexFormat(const VertexAttribute &attrib, GLenum currentValueType)
    : mType(attrib.type),
      mNormalized(attrib.normalized ? GL_TRUE : GL_FALSE),
      mComponents(attrib.size),
      mPureInteger(attrib.pureInteger)
{
    if (!attrib.enabled)
    {
        mType = currentValueType;
        mNormalized = GL_FALSE;
        mComponents = 4;
        mPureInteger = (currentValueType != GL_FLOAT);
    }

    // Float data can not be normalized, so ignore the user setting
    if (mType == GL_FLOAT || mType == GL_HALF_FLOAT || mType == GL_FIXED)
    {
        mNormalized = GL_FALSE;
    }
}

void VertexFormat::GetInputLayout(VertexFormat *inputLayout,
                                  ProgramBinary *programBinary,
                                  const State &state)
{
    const VertexAttribute *vertexAttributes = state.getVertexArray()->getVertexAttributes();
    for (unsigned int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        int semanticIndex = programBinary->getSemanticIndex(attributeIndex);

        if (semanticIndex != -1)
        {
            inputLayout[semanticIndex] = VertexFormat(vertexAttributes[attributeIndex], state.getVertexAttribCurrentValue(attributeIndex).Type);
        }
    }
}

bool VertexFormat::operator==(const VertexFormat &other) const
{
    return (mType == other.mType                &&
            mComponents == other.mComponents    &&
            mNormalized == other.mNormalized    &&
            mPureInteger == other.mPureInteger  );
}

bool VertexFormat::operator!=(const VertexFormat &other) const
{
    return !(*this == other);
}

bool VertexFormat::operator<(const VertexFormat& other) const
{
    if (mType != other.mType)
    {
        return mType < other.mType;
    }
    if (mNormalized != other.mNormalized)
    {
        return mNormalized < other.mNormalized;
    }
    if (mComponents != other.mComponents)
    {
        return mComponents < other.mComponents;
    }
    return mPureInteger < other.mPureInteger;
}

bool Box::operator==(const Box &other) const
{
    return (x == other.x && y == other.y && z == other.z &&
            width == other.width && height == other.height && depth == other.depth);
}

bool Box::operator!=(const Box &other) const
{
    return !(*this == other);
}

}
