//
// Copyright (c) 2013-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// formatutils9.h: Queries for GL image formats and their translations to D3D9
// formats.

#ifndef LIBGLESV2_RENDERER_FORMATUTILS9_H_
#define LIBGLESV2_RENDERER_FORMATUTILS9_H_

#include "libGLESv2/formatutils.h"

#include <map>

namespace rx
{

class Renderer9;

namespace d3d9
{

typedef std::map<std::pair<GLenum, GLenum>, ColorCopyFunction> FastCopyFunctionMap;

struct D3DFormat
{
    D3DFormat();

    GLuint pixelBytes;
    GLuint blockWidth;
    GLuint blockHeight;

    GLenum internalFormat;

    MipGenerationFunction mipGenerationFunction;
    ColorReadFunction colorReadFunction;

    FastCopyFunctionMap fastCopyFunctions;
    ColorCopyFunction getFastCopyFunction(GLenum format, GLenum type) const;
};
const D3DFormat &GetD3DFormatInfo(D3DFORMAT format);

struct VertexFormat
{
    VertexFormat();

    VertexConversionType conversionType;
    size_t outputElementSize;
    VertexCopyFunction copyFunction;
    D3DDECLTYPE nativeFormat;
    GLenum componentType;
};
const VertexFormat &GetVertexFormatInfo(DWORD supportedDeclTypes, const gl::VertexFormat &vertexFormat);

struct TextureFormat
{
    TextureFormat();

    D3DFORMAT texFormat;
    D3DFORMAT renderFormat;

    InitializeTextureDataFunction dataInitializerFunction;

    LoadImageFunction loadFunction;
};
const TextureFormat &GetTextureFormatInfo(GLenum internalFormat);

}

}

#endif // LIBGLESV2_RENDERER_FORMATUTILS9_H_
