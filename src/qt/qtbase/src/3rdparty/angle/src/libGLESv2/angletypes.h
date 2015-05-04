//
// Copyright (c) 2012-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// angletypes.h : Defines a variety of structures and enum types that are used throughout libGLESv2

#ifndef LIBGLESV2_ANGLETYPES_H_
#define LIBGLESV2_ANGLETYPES_H_

#include "libGLESv2/Constants.h"
#include "common/RefCountObject.h"

namespace gl
{
class Buffer;
class State;
class ProgramBinary;
struct VertexAttribute;
struct VertexAttribCurrentValueData;

enum SamplerType
{
    SAMPLER_PIXEL,
    SAMPLER_VERTEX
};

template <typename T>
struct Color
{
    T red;
    T green;
    T blue;
    T alpha;

    Color() : red(0), green(0), blue(0), alpha(0) { }
    Color(T r, T g, T b, T a) : red(r), green(g), blue(b), alpha(a) { }
};

typedef Color<float> ColorF;
typedef Color<int> ColorI;
typedef Color<unsigned int> ColorUI;

struct Rectangle
{
    int x;
    int y;
    int width;
    int height;

    Rectangle() : x(0), y(0), width(0), height(0) { }
    Rectangle(int x_in, int y_in, int width_in, int height_in) : x(x_in), y(y_in), width(width_in), height(height_in) { }
};

bool ClipRectangle(const Rectangle &source, const Rectangle &clip, Rectangle *intersection);

struct Box
{
    int x;
    int y;
    int z;
    int width;
    int height;
    int depth;

    Box() : x(0), y(0), z(0), width(0), height(0), depth(0) { }
    Box(int x_in, int y_in, int z_in, int width_in, int height_in, int depth_in) : x(x_in), y(y_in), z(z_in), width(width_in), height(height_in), depth(depth_in) { }
    bool operator==(const Box &other) const;
    bool operator!=(const Box &other) const;
};

struct Extents
{
    int width;
    int height;
    int depth;

    Extents() : width(0), height(0), depth(0) { }
    Extents(int width_, int height_, int depth_) : width(width_), height(height_), depth(depth_) { }
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

    bool rasterizerDiscard;
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
    SamplerState();

    GLenum minFilter;
    GLenum magFilter;
    GLenum wrapS;
    GLenum wrapT;
    GLenum wrapR;
    float maxAnisotropy;

    GLint baseLevel;
    GLint maxLevel;
    GLfloat minLod;
    GLfloat maxLod;

    GLenum compareMode;
    GLenum compareFunc;

    GLenum swizzleRed;
    GLenum swizzleGreen;
    GLenum swizzleBlue;
    GLenum swizzleAlpha;

    bool swizzleRequired() const;
};

struct ClearParameters
{
    bool clearColor[gl::IMPLEMENTATION_MAX_DRAW_BUFFERS];
    ColorF colorFClearValue;
    ColorI colorIClearValue;
    ColorUI colorUIClearValue;
    GLenum colorClearType;
    bool colorMaskRed;
    bool colorMaskGreen;
    bool colorMaskBlue;
    bool colorMaskAlpha;

    bool clearDepth;
    float depthClearValue;

    bool clearStencil;
    GLint stencilClearValue;
    GLuint stencilWriteMask;

    bool scissorEnabled;
    Rectangle scissor;
};

struct PixelUnpackState
{
    BindingPointer<Buffer> pixelBuffer;
    GLint alignment;

    PixelUnpackState()
        : alignment(4)
    {}

    explicit PixelUnpackState(GLint alignmentIn)
        : alignment(alignmentIn)
    {}
};

struct PixelPackState
{
    BindingPointer<Buffer> pixelBuffer;
    GLint alignment;
    bool reverseRowOrder;

    PixelPackState()
        : alignment(4),
          reverseRowOrder(false)
    {}

    explicit PixelPackState(GLint alignmentIn, bool reverseRowOrderIn)
        : alignment(alignmentIn),
          reverseRowOrder(reverseRowOrderIn)
    {}
};

struct VertexFormat
{
    GLenum      mType;
    GLboolean   mNormalized;
    GLuint      mComponents;
    bool        mPureInteger;

    VertexFormat();
    VertexFormat(GLenum type, GLboolean normalized, GLuint components, bool pureInteger);
    explicit VertexFormat(const VertexAttribute &attribute);
    VertexFormat(const VertexAttribute &attribute, GLenum currentValueType);

    static void GetInputLayout(VertexFormat *inputLayout,
                               ProgramBinary *programBinary,
                               const State& currentValues);

    bool operator==(const VertexFormat &other) const;
    bool operator!=(const VertexFormat &other) const;
    bool operator<(const VertexFormat& other) const;
};

}

namespace rx
{

enum VertexConversionType
{
    VERTEX_CONVERT_NONE = 0,
    VERTEX_CONVERT_CPU  = 1,
    VERTEX_CONVERT_GPU  = 2,
    VERTEX_CONVERT_BOTH = 3
};

}

#endif // LIBGLESV2_ANGLETYPES_H_
