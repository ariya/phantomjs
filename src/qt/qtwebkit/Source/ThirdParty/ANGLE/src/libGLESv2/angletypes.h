//
// Copyright (c) 2012-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// angletypes.h : Defines a variety of structures and enum types that are used throughout libGLESv2

#ifndef LIBGLESV2_ANGLETYPES_H_
#define LIBGLESV2_ANGLETYPES_H_

namespace gl
{

enum TextureType
{
    TEXTURE_2D,
    TEXTURE_CUBE,

    TEXTURE_TYPE_COUNT,
    TEXTURE_UNKNOWN
};

enum SamplerType
{
    SAMPLER_PIXEL,
    SAMPLER_VERTEX
};

struct Color
{
    float red;
    float green;
    float blue;
    float alpha;
};

struct Rectangle
{
    int x;
    int y;
    int width;
    int height;
};

struct RasterizerState
{
    bool cullFace;
    GLenum cullMode;
    GLenum frontFace;

    bool polygonOffsetFill;
    GLfloat polygonOffsetFactor;
    GLfloat polygonOffsetUnits;

    bool pointDrawMode;
    bool multiSample;
};

struct BlendState
{
    bool blend;
    GLenum sourceBlendRGB;
    GLenum destBlendRGB;
    GLenum sourceBlendAlpha;
    GLenum destBlendAlpha;
    GLenum blendEquationRGB;
    GLenum blendEquationAlpha;

    bool colorMaskRed;
    bool colorMaskGreen;
    bool colorMaskBlue;
    bool colorMaskAlpha;

    bool sampleAlphaToCoverage;

    bool dither;
};

struct DepthStencilState
{
    bool depthTest;
    GLenum depthFunc;
    bool depthMask;

    bool stencilTest;
    GLenum stencilFunc;
    GLuint stencilMask;
    GLenum stencilFail;
    GLenum stencilPassDepthFail;
    GLenum stencilPassDepthPass;
    GLuint stencilWritemask;
    GLenum stencilBackFunc;
    GLuint stencilBackMask;
    GLenum stencilBackFail;
    GLenum stencilBackPassDepthFail;
    GLenum stencilBackPassDepthPass;
    GLuint stencilBackWritemask;
};

struct SamplerState
{
    GLenum minFilter;
    GLenum magFilter;
    GLenum wrapS;
    GLenum wrapT;
    float maxAnisotropy;
    int lodOffset;
};

struct ClearParameters
{
    GLbitfield mask;

    Color colorClearValue;
    bool colorMaskRed;
    bool colorMaskGreen;
    bool colorMaskBlue;
    bool colorMaskAlpha;

    float depthClearValue;

    GLint stencilClearValue;
    GLuint stencilWriteMask;
};

}

#endif // LIBGLESV2_ANGLETYPES_H_
