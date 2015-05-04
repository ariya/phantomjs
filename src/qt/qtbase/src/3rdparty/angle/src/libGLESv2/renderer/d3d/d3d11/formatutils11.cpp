//
// Copyright (c) 2013-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// formatutils11.cpp: Queries for GL image formats and their translations to D3D11
// formats.

#include "libGLESv2/renderer/d3d/d3d11/formatutils11.h"
#include "libGLESv2/renderer/generatemip.h"
#include "libGLESv2/renderer/loadimage.h"
#include "libGLESv2/renderer/copyimage.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/copyvertex.h"

namespace rx
{

namespace d3d11
{

typedef std::map<DXGI_FORMAT, GLenum> DXGIToESFormatMap;

inline void AddDXGIToESEntry(DXGIToESFormatMap *map, DXGI_FORMAT key, GLenum value)
{
    map->insert(std::make_pair(key, value));
}

static DXGIToESFormatMap BuildDXGIToESFormatMap()
{
    DXGIToESFormatMap map;

    AddDXGIToESEntry(&map, DXGI_FORMAT_UNKNOWN,                  GL_NONE);

    AddDXGIToESEntry(&map, DXGI_FORMAT_A8_UNORM,                 GL_ALPHA8_EXT);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R8_UNORM,                 GL_R8);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R8G8_UNORM,               GL_RG8);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R8G8B8A8_UNORM,           GL_RGBA8);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,      GL_SRGB8_ALPHA8);
    AddDXGIToESEntry(&map, DXGI_FORMAT_B8G8R8A8_UNORM,           GL_BGRA8_EXT);

    AddDXGIToESEntry(&map, DXGI_FORMAT_R8_SNORM,                 GL_R8_SNORM);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R8G8_SNORM,               GL_RG8_SNORM);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R8G8B8A8_SNORM,           GL_RGBA8_SNORM);

    AddDXGIToESEntry(&map, DXGI_FORMAT_R8_UINT,                  GL_R8UI);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R16_UINT,                 GL_R16UI);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R32_UINT,                 GL_R32UI);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R8G8_UINT,                GL_RG8UI);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R16G16_UINT,              GL_RG16UI);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R32G32_UINT,              GL_RG32UI);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R32G32B32_UINT,           GL_RGB32UI);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R8G8B8A8_UINT,            GL_RGBA8UI);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R16G16B16A16_UINT,        GL_RGBA16UI);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R32G32B32A32_UINT,        GL_RGBA32UI);

    AddDXGIToESEntry(&map, DXGI_FORMAT_R8_SINT,                  GL_R8I);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R16_SINT,                 GL_R16I);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R32_SINT,                 GL_R32I);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R8G8_SINT,                GL_RG8I);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R16G16_SINT,              GL_RG16I);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R32G32_SINT,              GL_RG32I);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R32G32B32_SINT,           GL_RGB32I);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R8G8B8A8_SINT,            GL_RGBA8I);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R16G16B16A16_SINT,        GL_RGBA16I);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R32G32B32A32_SINT,        GL_RGBA32I);

    AddDXGIToESEntry(&map, DXGI_FORMAT_R10G10B10A2_UNORM,        GL_RGB10_A2);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R10G10B10A2_UINT,         GL_RGB10_A2UI);

    AddDXGIToESEntry(&map, DXGI_FORMAT_R16_FLOAT,                GL_R16F);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R16G16_FLOAT,             GL_RG16F);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R16G16B16A16_FLOAT,       GL_RGBA16F);

    AddDXGIToESEntry(&map, DXGI_FORMAT_R32_FLOAT,                GL_R32F);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R32G32_FLOAT,             GL_RG32F);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R32G32B32_FLOAT,          GL_RGB32F);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R32G32B32A32_FLOAT,       GL_RGBA32F);

    AddDXGIToESEntry(&map, DXGI_FORMAT_R9G9B9E5_SHAREDEXP,       GL_RGB9_E5);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R11G11B10_FLOAT,          GL_R11F_G11F_B10F);

    AddDXGIToESEntry(&map, DXGI_FORMAT_R16_TYPELESS,             GL_DEPTH_COMPONENT16);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R16_UNORM,                GL_DEPTH_COMPONENT16);
    AddDXGIToESEntry(&map, DXGI_FORMAT_D16_UNORM,                GL_DEPTH_COMPONENT16);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R24G8_TYPELESS,           GL_DEPTH24_STENCIL8_OES);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R24_UNORM_X8_TYPELESS,    GL_DEPTH24_STENCIL8_OES);
    AddDXGIToESEntry(&map, DXGI_FORMAT_D24_UNORM_S8_UINT,        GL_DEPTH24_STENCIL8_OES);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R32G8X24_TYPELESS,        GL_DEPTH32F_STENCIL8);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, GL_DEPTH32F_STENCIL8);
    AddDXGIToESEntry(&map, DXGI_FORMAT_D32_FLOAT_S8X24_UINT,     GL_DEPTH32F_STENCIL8);
    AddDXGIToESEntry(&map, DXGI_FORMAT_R32_TYPELESS,             GL_DEPTH_COMPONENT32F);
    AddDXGIToESEntry(&map, DXGI_FORMAT_D32_FLOAT,                GL_DEPTH_COMPONENT32F);

    AddDXGIToESEntry(&map, DXGI_FORMAT_BC1_UNORM,                GL_COMPRESSED_RGBA_S3TC_DXT1_EXT);
    AddDXGIToESEntry(&map, DXGI_FORMAT_BC2_UNORM,                GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE);
    AddDXGIToESEntry(&map, DXGI_FORMAT_BC3_UNORM,                GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE);

    return map;
}

struct D3D11FastCopyFormat
{
    GLenum destFormat;
    GLenum destType;
    ColorCopyFunction copyFunction;

    D3D11FastCopyFormat(GLenum destFormat, GLenum destType, ColorCopyFunction copyFunction)
        : destFormat(destFormat), destType(destType), copyFunction(copyFunction)
    { }

    bool operator<(const D3D11FastCopyFormat& other) const
    {
        return memcmp(this, &other, sizeof(D3D11FastCopyFormat)) < 0;
    }
};

typedef std::multimap<DXGI_FORMAT, D3D11FastCopyFormat> D3D11FastCopyMap;

static D3D11FastCopyMap BuildFastCopyMap()
{
    D3D11FastCopyMap map;

    map.insert(std::make_pair(DXGI_FORMAT_B8G8R8A8_UNORM, D3D11FastCopyFormat(GL_RGBA, GL_UNSIGNED_BYTE, CopyBGRA8ToRGBA8)));

    return map;
}

struct DXGIDepthStencilInfo
{
    unsigned int depthBits;
    unsigned int depthOffset;
    unsigned int stencilBits;
    unsigned int stencilOffset;
};

typedef std::map<DXGI_FORMAT, DXGIDepthStencilInfo> DepthStencilInfoMap;
typedef std::pair<DXGI_FORMAT, DXGIDepthStencilInfo> DepthStencilInfoPair;

static inline void InsertDXGIDepthStencilInfo(DepthStencilInfoMap *map, DXGI_FORMAT format, unsigned int depthBits,
                                              unsigned int depthOffset, unsigned int stencilBits, unsigned int stencilOffset)
{
    DXGIDepthStencilInfo info;
    info.depthBits = depthBits;
    info.depthOffset = depthOffset;
    info.stencilBits = stencilBits;
    info.stencilOffset = stencilOffset;

    map->insert(std::make_pair(format, info));
}

static DepthStencilInfoMap BuildDepthStencilInfoMap()
{
    DepthStencilInfoMap map;

    InsertDXGIDepthStencilInfo(&map, DXGI_FORMAT_R16_TYPELESS,             16, 0, 0,  0);
    InsertDXGIDepthStencilInfo(&map, DXGI_FORMAT_R16_UNORM,                16, 0, 0,  0);
    InsertDXGIDepthStencilInfo(&map, DXGI_FORMAT_D16_UNORM,                16, 0, 0,  0);

    InsertDXGIDepthStencilInfo(&map, DXGI_FORMAT_R24G8_TYPELESS,           24, 0, 8, 24);
    InsertDXGIDepthStencilInfo(&map, DXGI_FORMAT_R24_UNORM_X8_TYPELESS,    24, 0, 8, 24);
    InsertDXGIDepthStencilInfo(&map, DXGI_FORMAT_D24_UNORM_S8_UINT,        24, 0, 8, 24);

    InsertDXGIDepthStencilInfo(&map, DXGI_FORMAT_R32_TYPELESS,             32, 0, 0,  0);
    InsertDXGIDepthStencilInfo(&map, DXGI_FORMAT_R32_FLOAT,                32, 0, 0,  0);
    InsertDXGIDepthStencilInfo(&map, DXGI_FORMAT_D32_FLOAT,                32, 0, 0,  0);

    InsertDXGIDepthStencilInfo(&map, DXGI_FORMAT_R32G8X24_TYPELESS,        32, 0, 8, 32);
    InsertDXGIDepthStencilInfo(&map, DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, 32, 0, 8, 32);
    InsertDXGIDepthStencilInfo(&map, DXGI_FORMAT_D32_FLOAT_S8X24_UINT,     32, 0, 8, 32);

    return map;
}

typedef std::map<DXGI_FORMAT, DXGIFormat> DXGIFormatInfoMap;

DXGIFormat::DXGIFormat()
    : pixelBytes(0),
      blockWidth(0),
      blockHeight(0),
      depthBits(0),
      depthOffset(0),
      stencilBits(0),
      stencilOffset(0),
      internalFormat(GL_NONE),
      componentType(GL_NONE),
      mipGenerationFunction(NULL),
      colorReadFunction(NULL),
      fastCopyFunctions()
{
}

ColorCopyFunction DXGIFormat::getFastCopyFunction(GLenum format, GLenum type) const
{
    FastCopyFunctionMap::const_iterator iter = fastCopyFunctions.find(std::make_pair(format, type));
    return (iter != fastCopyFunctions.end()) ? iter->second : NULL;
}

void AddDXGIFormat(DXGIFormatInfoMap *map, DXGI_FORMAT dxgiFormat, GLuint pixelBits, GLuint blockWidth, GLuint blockHeight,
                   GLenum componentType, MipGenerationFunction mipFunc, ColorReadFunction readFunc)
{
    DXGIFormat info;
    info.pixelBytes = pixelBits / 8;
    info.blockWidth = blockWidth;
    info.blockHeight = blockHeight;

    static const DepthStencilInfoMap dsInfoMap = BuildDepthStencilInfoMap();
    DepthStencilInfoMap::const_iterator dsInfoIter = dsInfoMap.find(dxgiFormat);
    if (dsInfoIter != dsInfoMap.end())
    {
        info.depthBits = dsInfoIter->second.depthBits;
        info.depthOffset = dsInfoIter->second.depthOffset;
        info.stencilBits = dsInfoIter->second.stencilBits;
        info.stencilOffset = dsInfoIter->second.stencilOffset;
    }
    else
    {
        info.depthBits = 0;
        info.depthOffset = 0;
        info.stencilBits = 0;
        info.stencilOffset = 0;
    }

    static const DXGIToESFormatMap dxgiToESMap = BuildDXGIToESFormatMap();
    DXGIToESFormatMap::const_iterator dxgiToESIter = dxgiToESMap.find(dxgiFormat);
    info.internalFormat = (dxgiToESIter != dxgiToESMap.end()) ? dxgiToESIter->second : GL_NONE;

    info.componentType = componentType;

    info.mipGenerationFunction = mipFunc;
    info.colorReadFunction = readFunc;

    static const D3D11FastCopyMap fastCopyMap = BuildFastCopyMap();
    std::pair<D3D11FastCopyMap::const_iterator, D3D11FastCopyMap::const_iterator> fastCopyIter = fastCopyMap.equal_range(dxgiFormat);
    for (D3D11FastCopyMap::const_iterator i = fastCopyIter.first; i != fastCopyIter.second; i++)
    {
        info.fastCopyFunctions.insert(std::make_pair(std::make_pair(i->second.destFormat, i->second.destType), i->second.copyFunction));
    }

    map->insert(std::make_pair(dxgiFormat, info));
}

// A map to determine the pixel size and mipmap generation function of a given DXGI format
static DXGIFormatInfoMap BuildDXGIFormatInfoMap()
{
    DXGIFormatInfoMap map;

    //                | DXGI format                          |S   |W |H |Component Type         | Mip generation function   | Color read function
    AddDXGIFormat(&map, DXGI_FORMAT_UNKNOWN,                  0,   0, 0, GL_NONE,                NULL,                       NULL);

    AddDXGIFormat(&map, DXGI_FORMAT_A8_UNORM,                 8,   1, 1, GL_UNSIGNED_NORMALIZED, GenerateMip<A8>,            ReadColor<A8, GLfloat>);
    AddDXGIFormat(&map, DXGI_FORMAT_R8_UNORM,                 8,   1, 1, GL_UNSIGNED_NORMALIZED, GenerateMip<R8>,            ReadColor<R8, GLfloat>);
    AddDXGIFormat(&map, DXGI_FORMAT_R8G8_UNORM,               16,  1, 1, GL_UNSIGNED_NORMALIZED, GenerateMip<R8G8>,          ReadColor<R8G8, GLfloat>);
    AddDXGIFormat(&map, DXGI_FORMAT_R8G8B8A8_UNORM,           32,  1, 1, GL_UNSIGNED_NORMALIZED, GenerateMip<R8G8B8A8>,      ReadColor<R8G8B8A8, GLfloat>);
    AddDXGIFormat(&map, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,      32,  1, 1, GL_UNSIGNED_NORMALIZED, GenerateMip<R8G8B8A8>,      ReadColor<R8G8B8A8, GLfloat>);
    AddDXGIFormat(&map, DXGI_FORMAT_B8G8R8A8_UNORM,           32,  1, 1, GL_UNSIGNED_NORMALIZED, GenerateMip<B8G8R8A8>,      ReadColor<B8G8R8A8, GLfloat>);

    AddDXGIFormat(&map, DXGI_FORMAT_R8_SNORM,                 8,   1, 1, GL_SIGNED_NORMALIZED,   GenerateMip<R8S>,           ReadColor<R8S, GLfloat>);
    AddDXGIFormat(&map, DXGI_FORMAT_R8G8_SNORM,               16,  1, 1, GL_SIGNED_NORMALIZED,   GenerateMip<R8G8S>,         ReadColor<R8G8S, GLfloat>);
    AddDXGIFormat(&map, DXGI_FORMAT_R8G8B8A8_SNORM,           32,  1, 1, GL_SIGNED_NORMALIZED,   GenerateMip<R8G8B8A8S>,     ReadColor<R8G8B8A8S, GLfloat>);

    AddDXGIFormat(&map, DXGI_FORMAT_R8_UINT,                  8,   1, 1, GL_UNSIGNED_INT,        GenerateMip<R8>,            ReadColor<R8, GLuint>);
    AddDXGIFormat(&map, DXGI_FORMAT_R16_UINT,                 16,  1, 1, GL_UNSIGNED_INT,        GenerateMip<R16>,           ReadColor<R16, GLuint>);
    AddDXGIFormat(&map, DXGI_FORMAT_R32_UINT,                 32,  1, 1, GL_UNSIGNED_INT,        GenerateMip<R32>,           ReadColor<R32, GLuint>);
    AddDXGIFormat(&map, DXGI_FORMAT_R8G8_UINT,                16,  1, 1, GL_UNSIGNED_INT,        GenerateMip<R8G8>,          ReadColor<R8G8, GLuint>);
    AddDXGIFormat(&map, DXGI_FORMAT_R16G16_UINT,              32,  1, 1, GL_UNSIGNED_INT,        GenerateMip<R16G16>,        ReadColor<R16G16, GLuint>);
    AddDXGIFormat(&map, DXGI_FORMAT_R32G32_UINT,              64,  1, 1, GL_UNSIGNED_INT,        GenerateMip<R32G32>,        ReadColor<R32G32, GLuint>);
    AddDXGIFormat(&map, DXGI_FORMAT_R32G32B32_UINT,           96,  1, 1, GL_UNSIGNED_INT,        GenerateMip<R32G32B32>,     ReadColor<R32G32B32, GLuint>);
    AddDXGIFormat(&map, DXGI_FORMAT_R8G8B8A8_UINT,            32,  1, 1, GL_UNSIGNED_INT,        GenerateMip<R8G8B8A8>,      ReadColor<R8G8B8A8, GLuint>);
    AddDXGIFormat(&map, DXGI_FORMAT_R16G16B16A16_UINT,        64,  1, 1, GL_UNSIGNED_INT,        GenerateMip<R16G16B16A16>,  ReadColor<R16G16B16A16, GLuint>);
    AddDXGIFormat(&map, DXGI_FORMAT_R32G32B32A32_UINT,        128, 1, 1, GL_UNSIGNED_INT,        GenerateMip<R32G32B32A32>,  ReadColor<R32G32B32A32, GLuint>);

    AddDXGIFormat(&map, DXGI_FORMAT_R8_SINT,                  8,   1, 1, GL_INT,                 GenerateMip<R8S>,           ReadColor<R8S, GLint>);
    AddDXGIFormat(&map, DXGI_FORMAT_R16_SINT,                 16,  1, 1, GL_INT,                 GenerateMip<R16S>,          ReadColor<R16S, GLint>);
    AddDXGIFormat(&map, DXGI_FORMAT_R32_SINT,                 32,  1, 1, GL_INT,                 GenerateMip<R32S>,          ReadColor<R32S, GLint>);
    AddDXGIFormat(&map, DXGI_FORMAT_R8G8_SINT,                16,  1, 1, GL_INT,                 GenerateMip<R8G8S>,         ReadColor<R8G8S, GLint>);
    AddDXGIFormat(&map, DXGI_FORMAT_R16G16_SINT,              32,  1, 1, GL_INT,                 GenerateMip<R16G16S>,       ReadColor<R16G16S, GLint>);
    AddDXGIFormat(&map, DXGI_FORMAT_R32G32_SINT,              64,  1, 1, GL_INT,                 GenerateMip<R32G32S>,       ReadColor<R32G32S, GLint>);
    AddDXGIFormat(&map, DXGI_FORMAT_R32G32B32_SINT,           96,  1, 1, GL_INT,                 GenerateMip<R32G32B32S>,    ReadColor<R32G32B32S, GLint>);
    AddDXGIFormat(&map, DXGI_FORMAT_R8G8B8A8_SINT,            32,  1, 1, GL_INT,                 GenerateMip<R8G8B8A8S>,     ReadColor<R8G8B8A8S, GLint>);
    AddDXGIFormat(&map, DXGI_FORMAT_R16G16B16A16_SINT,        64,  1, 1, GL_INT,                 GenerateMip<R16G16B16A16S>, ReadColor<R16G16B16A16S, GLint>);
    AddDXGIFormat(&map, DXGI_FORMAT_R32G32B32A32_SINT,        128, 1, 1, GL_INT,                 GenerateMip<R32G32B32A32S>, ReadColor<R32G32B32A32S, GLint>);

    AddDXGIFormat(&map, DXGI_FORMAT_R10G10B10A2_UNORM,        32,  1, 1, GL_UNSIGNED_NORMALIZED, GenerateMip<R10G10B10A2>,   ReadColor<R10G10B10A2, GLfloat>);
    AddDXGIFormat(&map, DXGI_FORMAT_R10G10B10A2_UINT,         32,  1, 1, GL_UNSIGNED_INT,        GenerateMip<R10G10B10A2>,   ReadColor<R10G10B10A2, GLuint>);

    AddDXGIFormat(&map, DXGI_FORMAT_R16_FLOAT,                16,  1, 1, GL_FLOAT,               GenerateMip<R16F>,          ReadColor<R16F, GLfloat>);
    AddDXGIFormat(&map, DXGI_FORMAT_R16G16_FLOAT,             32,  1, 1, GL_FLOAT,               GenerateMip<R16G16F>,       ReadColor<R16G16F, GLfloat>);
    AddDXGIFormat(&map, DXGI_FORMAT_R16G16B16A16_FLOAT,       64,  1, 1, GL_FLOAT,               GenerateMip<R16G16B16A16F>, ReadColor<R16G16B16A16F, GLfloat>);

    AddDXGIFormat(&map, DXGI_FORMAT_R32_FLOAT,                32,  1, 1, GL_FLOAT,               GenerateMip<R32F>,          ReadColor<R32F, GLfloat>);
    AddDXGIFormat(&map, DXGI_FORMAT_R32G32_FLOAT,             64,  1, 1, GL_FLOAT,               GenerateMip<R32G32F>,       ReadColor<R32G32F, GLfloat>);
    AddDXGIFormat(&map, DXGI_FORMAT_R32G32B32_FLOAT,          96,  1, 1, GL_FLOAT,               NULL,                       NULL);
    AddDXGIFormat(&map, DXGI_FORMAT_R32G32B32A32_FLOAT,       128, 1, 1, GL_FLOAT,               GenerateMip<R32G32B32A32F>, ReadColor<R32G32B32A32F, GLfloat>);

    AddDXGIFormat(&map, DXGI_FORMAT_R9G9B9E5_SHAREDEXP,       32,  1, 1, GL_FLOAT,               GenerateMip<R9G9B9E5>,      ReadColor<R9G9B9E5, GLfloat>);
    AddDXGIFormat(&map, DXGI_FORMAT_R11G11B10_FLOAT,          32,  1, 1, GL_FLOAT,               GenerateMip<R11G11B10F>,    ReadColor<R11G11B10F, GLfloat>);

    AddDXGIFormat(&map, DXGI_FORMAT_R16_TYPELESS,             16,  1, 1, GL_NONE,                NULL,                       NULL);
    AddDXGIFormat(&map, DXGI_FORMAT_R16_UNORM,                16,  1, 1, GL_UNSIGNED_NORMALIZED, NULL,                       NULL);
    AddDXGIFormat(&map, DXGI_FORMAT_D16_UNORM,                16,  1, 1, GL_UNSIGNED_NORMALIZED, NULL,                       NULL);
    AddDXGIFormat(&map, DXGI_FORMAT_R24G8_TYPELESS,           32,  1, 1, GL_NONE,                NULL,                       NULL);
    AddDXGIFormat(&map, DXGI_FORMAT_R24_UNORM_X8_TYPELESS,    32,  1, 1, GL_NONE,                NULL,                       NULL);
    AddDXGIFormat(&map, DXGI_FORMAT_D24_UNORM_S8_UINT,        32,  1, 1, GL_UNSIGNED_INT,        NULL,                       NULL);
    AddDXGIFormat(&map, DXGI_FORMAT_R32G8X24_TYPELESS,        64,  1, 1, GL_NONE,                NULL,                       NULL);
    AddDXGIFormat(&map, DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, 64,  1, 1, GL_NONE,                NULL,                       NULL);
    AddDXGIFormat(&map, DXGI_FORMAT_D32_FLOAT_S8X24_UINT,     64,  1, 1, GL_UNSIGNED_INT,        NULL,                       NULL);
    AddDXGIFormat(&map, DXGI_FORMAT_R32_TYPELESS,             32,  1, 1, GL_NONE,                NULL,                       NULL);
    AddDXGIFormat(&map, DXGI_FORMAT_D32_FLOAT,                32,  1, 1, GL_FLOAT,               NULL,                       NULL);

    AddDXGIFormat(&map, DXGI_FORMAT_BC1_UNORM,                64,  4, 4, GL_UNSIGNED_NORMALIZED, NULL,                       NULL);
    AddDXGIFormat(&map, DXGI_FORMAT_BC2_UNORM,                128, 4, 4, GL_UNSIGNED_NORMALIZED, NULL,                       NULL);
    AddDXGIFormat(&map, DXGI_FORMAT_BC3_UNORM,                128, 4, 4, GL_UNSIGNED_NORMALIZED, NULL,                       NULL);

    // Useful formats for vertex buffers
    AddDXGIFormat(&map, DXGI_FORMAT_R16_UNORM,                16,  1, 1, GL_UNSIGNED_NORMALIZED, NULL,                       NULL);
    AddDXGIFormat(&map, DXGI_FORMAT_R16_SNORM,                16,  1, 1, GL_SIGNED_NORMALIZED,   NULL,                       NULL);
    AddDXGIFormat(&map, DXGI_FORMAT_R16G16_UNORM,             32,  1, 1, GL_UNSIGNED_NORMALIZED, NULL,                       NULL);
    AddDXGIFormat(&map, DXGI_FORMAT_R16G16_SNORM,             32,  1, 1, GL_SIGNED_NORMALIZED,   NULL,                       NULL);
    AddDXGIFormat(&map, DXGI_FORMAT_R16G16B16A16_UNORM,       64,  1, 1, GL_UNSIGNED_NORMALIZED, NULL,                       NULL);
    AddDXGIFormat(&map, DXGI_FORMAT_R16G16B16A16_SNORM,       64,  1, 1, GL_SIGNED_NORMALIZED,   NULL,                       NULL);

    return map;
}

const DXGIFormat &GetDXGIFormatInfo(DXGI_FORMAT format)
{
    static const DXGIFormatInfoMap infoMap = BuildDXGIFormatInfoMap();
    DXGIFormatInfoMap::const_iterator iter = infoMap.find(format);
    if (iter != infoMap.end())
    {
        return iter->second;
    }
    else
    {
        static DXGIFormat defaultInfo;
        return defaultInfo;
    }
}

struct SwizzleSizeType
{
    size_t maxComponentSize;
    GLenum componentType;

    SwizzleSizeType()
        : maxComponentSize(0), componentType(GL_NONE)
    { }

    SwizzleSizeType(size_t maxComponentSize, GLenum componentType)
        : maxComponentSize(maxComponentSize), componentType(componentType)
    { }

    bool operator<(const SwizzleSizeType& other) const
    {
        return (maxComponentSize != other.maxComponentSize) ? (maxComponentSize < other.maxComponentSize)
                                                            : (componentType < other.componentType);
    }
};

struct SwizzleFormatInfo
{
    DXGI_FORMAT mTexFormat;
    DXGI_FORMAT mSRVFormat;
    DXGI_FORMAT mRTVFormat;

    SwizzleFormatInfo()
        : mTexFormat(DXGI_FORMAT_UNKNOWN), mSRVFormat(DXGI_FORMAT_UNKNOWN), mRTVFormat(DXGI_FORMAT_UNKNOWN)
    { }

    SwizzleFormatInfo(DXGI_FORMAT texFormat, DXGI_FORMAT srvFormat, DXGI_FORMAT rtvFormat)
        : mTexFormat(texFormat), mSRVFormat(srvFormat), mRTVFormat(rtvFormat)
    { }
};

typedef std::map<SwizzleSizeType, SwizzleFormatInfo> SwizzleInfoMap;
typedef std::pair<SwizzleSizeType, SwizzleFormatInfo> SwizzleInfoPair;

static SwizzleInfoMap BuildSwizzleInfoMap()
{
    SwizzleInfoMap map;

    map.insert(SwizzleInfoPair(SwizzleSizeType( 8, GL_UNSIGNED_NORMALIZED), SwizzleFormatInfo(DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_R8G8B8A8_UNORM    )));
    map.insert(SwizzleInfoPair(SwizzleSizeType(16, GL_UNSIGNED_NORMALIZED), SwizzleFormatInfo(DXGI_FORMAT_R16G16B16A16_UNORM, DXGI_FORMAT_R16G16B16A16_UNORM, DXGI_FORMAT_R16G16B16A16_UNORM)));
    map.insert(SwizzleInfoPair(SwizzleSizeType(24, GL_UNSIGNED_NORMALIZED), SwizzleFormatInfo(DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT)));
    map.insert(SwizzleInfoPair(SwizzleSizeType(32, GL_UNSIGNED_NORMALIZED), SwizzleFormatInfo(DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT)));

    map.insert(SwizzleInfoPair(SwizzleSizeType( 8, GL_SIGNED_NORMALIZED  ), SwizzleFormatInfo(DXGI_FORMAT_R8G8B8A8_SNORM,     DXGI_FORMAT_R8G8B8A8_SNORM,     DXGI_FORMAT_R8G8B8A8_SNORM    )));

    map.insert(SwizzleInfoPair(SwizzleSizeType(16, GL_FLOAT              ), SwizzleFormatInfo(DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT)));
    map.insert(SwizzleInfoPair(SwizzleSizeType(32, GL_FLOAT              ), SwizzleFormatInfo(DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT)));

    map.insert(SwizzleInfoPair(SwizzleSizeType( 8, GL_UNSIGNED_INT       ), SwizzleFormatInfo(DXGI_FORMAT_R8G8B8A8_UINT,      DXGI_FORMAT_R8G8B8A8_UINT,      DXGI_FORMAT_R8G8B8A8_UINT     )));
    map.insert(SwizzleInfoPair(SwizzleSizeType(16, GL_UNSIGNED_INT       ), SwizzleFormatInfo(DXGI_FORMAT_R16G16B16A16_UINT,  DXGI_FORMAT_R16G16B16A16_UINT,  DXGI_FORMAT_R16G16B16A16_UINT )));
    map.insert(SwizzleInfoPair(SwizzleSizeType(32, GL_UNSIGNED_INT       ), SwizzleFormatInfo(DXGI_FORMAT_R32G32B32A32_UINT,  DXGI_FORMAT_R32G32B32A32_UINT,  DXGI_FORMAT_R32G32B32A32_UINT )));

    map.insert(SwizzleInfoPair(SwizzleSizeType( 8, GL_INT                ), SwizzleFormatInfo(DXGI_FORMAT_R8G8B8A8_SINT,      DXGI_FORMAT_R8G8B8A8_SINT,      DXGI_FORMAT_R8G8B8A8_SINT     )));
    map.insert(SwizzleInfoPair(SwizzleSizeType(16, GL_INT                ), SwizzleFormatInfo(DXGI_FORMAT_R16G16B16A16_SINT,  DXGI_FORMAT_R16G16B16A16_SINT,  DXGI_FORMAT_R16G16B16A16_SINT )));
    map.insert(SwizzleInfoPair(SwizzleSizeType(32, GL_INT                ), SwizzleFormatInfo(DXGI_FORMAT_R32G32B32A32_SINT,  DXGI_FORMAT_R32G32B32A32_SINT,  DXGI_FORMAT_R32G32B32A32_SINT )));

    return map;
}

typedef std::pair<GLint, InitializeTextureDataFunction> InternalFormatInitializerPair;
typedef std::map<GLint, InitializeTextureDataFunction> InternalFormatInitializerMap;

static InternalFormatInitializerMap BuildInternalFormatInitializerMap()
{
    InternalFormatInitializerMap map;

    map.insert(InternalFormatInitializerPair(GL_RGB8,    Initialize4ComponentData<GLubyte,  0x00,       0x00,       0x00,       0xFF>          ));
    map.insert(InternalFormatInitializerPair(GL_RGB565,  Initialize4ComponentData<GLubyte,  0x00,       0x00,       0x00,       0xFF>          ));
    map.insert(InternalFormatInitializerPair(GL_SRGB8,   Initialize4ComponentData<GLubyte,  0x00,       0x00,       0x00,       0xFF>          ));
    map.insert(InternalFormatInitializerPair(GL_RGB16F,  Initialize4ComponentData<GLhalf,   0x0000,     0x0000,     0x0000,     gl::Float16One>));
    map.insert(InternalFormatInitializerPair(GL_RGB32F,  Initialize4ComponentData<GLfloat,  0x00000000, 0x00000000, 0x00000000, gl::Float32One>));
    map.insert(InternalFormatInitializerPair(GL_RGB8UI,  Initialize4ComponentData<GLubyte,  0x00,       0x00,       0x00,       0x01>          ));
    map.insert(InternalFormatInitializerPair(GL_RGB8I,   Initialize4ComponentData<GLbyte,   0x00,       0x00,       0x00,       0x01>          ));
    map.insert(InternalFormatInitializerPair(GL_RGB16UI, Initialize4ComponentData<GLushort, 0x0000,     0x0000,     0x0000,     0x0001>        ));
    map.insert(InternalFormatInitializerPair(GL_RGB16I,  Initialize4ComponentData<GLshort,  0x0000,     0x0000,     0x0000,     0x0001>        ));
    map.insert(InternalFormatInitializerPair(GL_RGB32UI, Initialize4ComponentData<GLuint,   0x00000000, 0x00000000, 0x00000000, 0x00000001>    ));
    map.insert(InternalFormatInitializerPair(GL_RGB32I,  Initialize4ComponentData<GLint,    0x00000000, 0x00000000, 0x00000000, 0x00000001>    ));

    return map;
}

// ES3 image loading functions vary based on the internal format and data type given,
// this map type determines the loading function from the internal format and type supplied
// to glTex*Image*D and the destination DXGI_FORMAT. Source formats and types are taken from
// Tables 3.2 and 3.3 of the ES 3 spec.
typedef std::pair<GLenum, LoadImageFunction> TypeLoadFunctionPair;
typedef std::map<GLenum, std::vector<TypeLoadFunctionPair> > D3D11LoadFunctionMap;

static void UnimplementedLoadFunction(size_t width, size_t height, size_t depth,
                                      const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                                      uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    UNIMPLEMENTED();
}

static void UnreachableLoadFunction(size_t width, size_t height, size_t depth,
                                    const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                                    uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    UNREACHABLE();
}

// A helper function to insert data into the D3D11LoadFunctionMap with fewer characters.
static inline void InsertLoadFunction(D3D11LoadFunctionMap *map, GLenum internalFormat, GLenum type,
                                      LoadImageFunction loadFunc)
{
    (*map)[internalFormat].push_back(TypeLoadFunctionPair(type, loadFunc));
}

D3D11LoadFunctionMap BuildD3D11LoadFunctionMap()
{
    D3D11LoadFunctionMap map;

    //                      | Internal format      | Type                             | Load function                       |
    InsertLoadFunction(&map, GL_RGBA8,              GL_UNSIGNED_BYTE,                  LoadToNative<GLubyte, 4>             );
    InsertLoadFunction(&map, GL_RGB5_A1,            GL_UNSIGNED_BYTE,                  LoadToNative<GLubyte, 4>             );
    InsertLoadFunction(&map, GL_RGBA4,              GL_UNSIGNED_BYTE,                  LoadToNative<GLubyte, 4>             );
    InsertLoadFunction(&map, GL_SRGB8_ALPHA8,       GL_UNSIGNED_BYTE,                  LoadToNative<GLubyte, 4>             );
    InsertLoadFunction(&map, GL_RGBA8_SNORM,        GL_BYTE,                           LoadToNative<GLbyte, 4>              );
    InsertLoadFunction(&map, GL_RGBA4,              GL_UNSIGNED_SHORT_4_4_4_4,         LoadRGBA4ToRGBA8                     );
    InsertLoadFunction(&map, GL_RGB10_A2,           GL_UNSIGNED_INT_2_10_10_10_REV,    LoadToNative<GLuint, 1>              );
    InsertLoadFunction(&map, GL_RGB5_A1,            GL_UNSIGNED_SHORT_5_5_5_1,         LoadRGB5A1ToRGBA8                    );
    InsertLoadFunction(&map, GL_RGB5_A1,            GL_UNSIGNED_INT_2_10_10_10_REV,    LoadRGB10A2ToRGBA8                   );
    InsertLoadFunction(&map, GL_RGBA16F,            GL_HALF_FLOAT,                     LoadToNative<GLhalf, 4>              );
    InsertLoadFunction(&map, GL_RGBA16F,            GL_HALF_FLOAT_OES,                 LoadToNative<GLhalf, 4>              );
    InsertLoadFunction(&map, GL_RGBA32F,            GL_FLOAT,                          LoadToNative<GLfloat, 4>             );
    InsertLoadFunction(&map, GL_RGBA16F,            GL_FLOAT,                          Load32FTo16F<4>                      );
    InsertLoadFunction(&map, GL_RGBA8UI,            GL_UNSIGNED_BYTE,                  LoadToNative<GLubyte, 4>             );
    InsertLoadFunction(&map, GL_RGBA8I,             GL_BYTE,                           LoadToNative<GLbyte, 4>              );
    InsertLoadFunction(&map, GL_RGBA16UI,           GL_UNSIGNED_SHORT,                 LoadToNative<GLushort, 4>            );
    InsertLoadFunction(&map, GL_RGBA16I,            GL_SHORT,                          LoadToNative<GLshort, 4>             );
    InsertLoadFunction(&map, GL_RGBA32UI,           GL_UNSIGNED_INT,                   LoadToNative<GLuint, 4>              );
    InsertLoadFunction(&map, GL_RGBA32I,            GL_INT,                            LoadToNative<GLint, 4>               );
    InsertLoadFunction(&map, GL_RGB10_A2UI,         GL_UNSIGNED_INT_2_10_10_10_REV,    LoadToNative<GLuint, 1>              );
    InsertLoadFunction(&map, GL_RGB8,               GL_UNSIGNED_BYTE,                  LoadToNative3To4<GLubyte, 0xFF>      );
    InsertLoadFunction(&map, GL_RGB565,             GL_UNSIGNED_BYTE,                  LoadToNative3To4<GLubyte, 0xFF>      );
    InsertLoadFunction(&map, GL_SRGB8,              GL_UNSIGNED_BYTE,                  LoadToNative3To4<GLubyte, 0xFF>      );
    InsertLoadFunction(&map, GL_RGB8_SNORM,         GL_BYTE,                           LoadToNative3To4<GLbyte, 0x7F>       );
    InsertLoadFunction(&map, GL_RGB565,             GL_UNSIGNED_SHORT_5_6_5,           LoadR5G6B5ToRGBA8                    );
    InsertLoadFunction(&map, GL_R11F_G11F_B10F,     GL_UNSIGNED_INT_10F_11F_11F_REV,   LoadToNative<GLuint, 1>              );
    InsertLoadFunction(&map, GL_RGB9_E5,            GL_UNSIGNED_INT_5_9_9_9_REV,       LoadToNative<GLuint, 1>              );
    InsertLoadFunction(&map, GL_RGB16F,             GL_HALF_FLOAT,                     LoadToNative3To4<GLhalf, gl::Float16One>);
    InsertLoadFunction(&map, GL_RGB16F,             GL_HALF_FLOAT_OES,                 LoadToNative3To4<GLhalf, gl::Float16One>);
    InsertLoadFunction(&map, GL_R11F_G11F_B10F,     GL_HALF_FLOAT,                     LoadRGB16FToRG11B10F                 );
    InsertLoadFunction(&map, GL_R11F_G11F_B10F,     GL_HALF_FLOAT_OES,                 LoadRGB16FToRG11B10F                 );
    InsertLoadFunction(&map, GL_RGB9_E5,            GL_HALF_FLOAT,                     LoadRGB16FToRGB9E5                   );
    InsertLoadFunction(&map, GL_RGB9_E5,            GL_HALF_FLOAT_OES,                 LoadRGB16FToRGB9E5                   );
    InsertLoadFunction(&map, GL_RGB32F,             GL_FLOAT,                          LoadToNative3To4<GLfloat, gl::Float32One>);
    InsertLoadFunction(&map, GL_RGB16F,             GL_FLOAT,                          LoadRGB32FToRGBA16F                  );
    InsertLoadFunction(&map, GL_R11F_G11F_B10F,     GL_FLOAT,                          LoadRGB32FToRG11B10F                 );
    InsertLoadFunction(&map, GL_RGB9_E5,            GL_FLOAT,                          LoadRGB32FToRGB9E5                   );
    InsertLoadFunction(&map, GL_RGB8UI,             GL_UNSIGNED_BYTE,                  LoadToNative3To4<GLubyte, 0x01>      );
    InsertLoadFunction(&map, GL_RGB8I,              GL_BYTE,                           LoadToNative3To4<GLbyte, 0x01>       );
    InsertLoadFunction(&map, GL_RGB16UI,            GL_UNSIGNED_SHORT,                 LoadToNative3To4<GLushort, 0x0001>   );
    InsertLoadFunction(&map, GL_RGB16I,             GL_SHORT,                          LoadToNative3To4<GLshort, 0x0001>    );
    InsertLoadFunction(&map, GL_RGB32UI,            GL_UNSIGNED_INT,                   LoadToNative3To4<GLuint, 0x00000001> );
    InsertLoadFunction(&map, GL_RGB32I,             GL_INT,                            LoadToNative3To4<GLint, 0x00000001>  );
    InsertLoadFunction(&map, GL_RG8,                GL_UNSIGNED_BYTE,                  LoadToNative<GLubyte, 2>             );
    InsertLoadFunction(&map, GL_RG8_SNORM,          GL_BYTE,                           LoadToNative<GLbyte, 2>              );
    InsertLoadFunction(&map, GL_RG16F,              GL_HALF_FLOAT,                     LoadToNative<GLhalf, 2>              );
    InsertLoadFunction(&map, GL_RG16F,              GL_HALF_FLOAT_OES,                 LoadToNative<GLhalf, 2>              );
    InsertLoadFunction(&map, GL_RG32F,              GL_FLOAT,                          LoadToNative<GLfloat, 2>             );
    InsertLoadFunction(&map, GL_RG16F,              GL_FLOAT,                          Load32FTo16F<2>                      );
    InsertLoadFunction(&map, GL_RG8UI,              GL_UNSIGNED_BYTE,                  LoadToNative<GLubyte, 2>             );
    InsertLoadFunction(&map, GL_RG8I,               GL_BYTE,                           LoadToNative<GLbyte, 2>              );
    InsertLoadFunction(&map, GL_RG16UI,             GL_UNSIGNED_SHORT,                 LoadToNative<GLushort, 2>            );
    InsertLoadFunction(&map, GL_RG16I,              GL_SHORT,                          LoadToNative<GLshort, 2>             );
    InsertLoadFunction(&map, GL_RG32UI,             GL_UNSIGNED_INT,                   LoadToNative<GLuint, 2>              );
    InsertLoadFunction(&map, GL_RG32I,              GL_INT,                            LoadToNative<GLint, 2>               );
    InsertLoadFunction(&map, GL_R8,                 GL_UNSIGNED_BYTE,                  LoadToNative<GLubyte, 1>             );
    InsertLoadFunction(&map, GL_R8_SNORM,           GL_BYTE,                           LoadToNative<GLbyte, 1>              );
    InsertLoadFunction(&map, GL_R16F,               GL_HALF_FLOAT,                     LoadToNative<GLhalf, 1>              );
    InsertLoadFunction(&map, GL_R16F,               GL_HALF_FLOAT_OES,                 LoadToNative<GLhalf, 1>              );
    InsertLoadFunction(&map, GL_R32F,               GL_FLOAT,                          LoadToNative<GLfloat, 1>             );
    InsertLoadFunction(&map, GL_R16F,               GL_FLOAT,                          Load32FTo16F<1>                      );
    InsertLoadFunction(&map, GL_R8UI,               GL_UNSIGNED_BYTE,                  LoadToNative<GLubyte, 1>             );
    InsertLoadFunction(&map, GL_R8I,                GL_BYTE,                           LoadToNative<GLbyte, 1>              );
    InsertLoadFunction(&map, GL_R16UI,              GL_UNSIGNED_SHORT,                 LoadToNative<GLushort, 1>            );
    InsertLoadFunction(&map, GL_R16I,               GL_SHORT,                          LoadToNative<GLshort, 1>             );
    InsertLoadFunction(&map, GL_R32UI,              GL_UNSIGNED_INT,                   LoadToNative<GLuint, 1>              );
    InsertLoadFunction(&map, GL_R32I,               GL_INT,                            LoadToNative<GLint, 1>               );
    InsertLoadFunction(&map, GL_DEPTH_COMPONENT16,  GL_UNSIGNED_SHORT,                 LoadToNative<GLushort, 1>            );
    InsertLoadFunction(&map, GL_DEPTH_COMPONENT24,  GL_UNSIGNED_INT,                   LoadR32ToR24G8                       );
    InsertLoadFunction(&map, GL_DEPTH_COMPONENT16,  GL_UNSIGNED_INT,                   LoadR32ToR16                         );
    InsertLoadFunction(&map, GL_DEPTH_COMPONENT32F, GL_FLOAT,                          LoadToNative<GLfloat, 1>             );
    InsertLoadFunction(&map, GL_DEPTH24_STENCIL8,   GL_UNSIGNED_INT_24_8,              LoadR32ToR24G8                       );
    InsertLoadFunction(&map, GL_DEPTH32F_STENCIL8,  GL_FLOAT_32_UNSIGNED_INT_24_8_REV, LoadToNative<GLuint, 2>              );

    // Unsized formats
    // Load functions are unreachable because they are converted to sized internal formats based on
    // the format and type before loading takes place.
    InsertLoadFunction(&map, GL_RGBA,               GL_UNSIGNED_BYTE,                  UnreachableLoadFunction              );
    InsertLoadFunction(&map, GL_RGBA,               GL_UNSIGNED_SHORT_4_4_4_4,         UnreachableLoadFunction              );
    InsertLoadFunction(&map, GL_RGBA,               GL_UNSIGNED_SHORT_5_5_5_1,         UnreachableLoadFunction              );
    InsertLoadFunction(&map, GL_RGB,                GL_UNSIGNED_BYTE,                  UnreachableLoadFunction              );
    InsertLoadFunction(&map, GL_RGB,                GL_UNSIGNED_SHORT_5_6_5,           UnreachableLoadFunction              );
    InsertLoadFunction(&map, GL_LUMINANCE_ALPHA,    GL_UNSIGNED_BYTE,                  UnreachableLoadFunction              );
    InsertLoadFunction(&map, GL_LUMINANCE,          GL_UNSIGNED_BYTE,                  UnreachableLoadFunction              );
    InsertLoadFunction(&map, GL_ALPHA,              GL_UNSIGNED_BYTE,                  UnreachableLoadFunction              );

    // From GL_OES_texture_float
    InsertLoadFunction(&map, GL_LUMINANCE_ALPHA,    GL_FLOAT,                          LoadLA32FToRGBA32F                   );
    InsertLoadFunction(&map, GL_LUMINANCE,          GL_FLOAT,                          LoadL32FToRGBA32F                    );
    InsertLoadFunction(&map, GL_ALPHA,              GL_FLOAT,                          LoadA32FToRGBA32F                    );

    // From GL_OES_texture_half_float
    InsertLoadFunction(&map, GL_LUMINANCE_ALPHA,    GL_HALF_FLOAT,                     LoadLA16FToRGBA16F                   );
    InsertLoadFunction(&map, GL_LUMINANCE_ALPHA,    GL_HALF_FLOAT_OES,                 LoadLA16FToRGBA16F                   );
    InsertLoadFunction(&map, GL_LUMINANCE,          GL_HALF_FLOAT,                     LoadL16FToRGBA16F                    );
    InsertLoadFunction(&map, GL_LUMINANCE,          GL_HALF_FLOAT_OES,                 LoadL16FToRGBA16F                    );
    InsertLoadFunction(&map, GL_ALPHA,              GL_HALF_FLOAT,                     LoadA16FToRGBA16F                    );
    InsertLoadFunction(&map, GL_ALPHA,              GL_HALF_FLOAT_OES,                 LoadA16FToRGBA16F                    );

    // From GL_EXT_texture_storage
    InsertLoadFunction(&map, GL_ALPHA8_EXT,             GL_UNSIGNED_BYTE,              LoadA8ToRGBA8                        );
    InsertLoadFunction(&map, GL_LUMINANCE8_EXT,         GL_UNSIGNED_BYTE,              LoadL8ToRGBA8                        );
    InsertLoadFunction(&map, GL_LUMINANCE8_ALPHA8_EXT,  GL_UNSIGNED_BYTE,              LoadLA8ToRGBA8                       );
    InsertLoadFunction(&map, GL_ALPHA32F_EXT,           GL_FLOAT,                      LoadA32FToRGBA32F                    );
    InsertLoadFunction(&map, GL_LUMINANCE32F_EXT,       GL_FLOAT,                      LoadL32FToRGBA32F                    );
    InsertLoadFunction(&map, GL_LUMINANCE_ALPHA32F_EXT, GL_FLOAT,                      LoadLA32FToRGBA32F                   );
    InsertLoadFunction(&map, GL_ALPHA16F_EXT,           GL_HALF_FLOAT,                 LoadA16FToRGBA16F                    );
    InsertLoadFunction(&map, GL_ALPHA16F_EXT,           GL_HALF_FLOAT_OES,             LoadA16FToRGBA16F                    );
    InsertLoadFunction(&map, GL_LUMINANCE16F_EXT,       GL_HALF_FLOAT,                 LoadL16FToRGBA16F                    );
    InsertLoadFunction(&map, GL_LUMINANCE16F_EXT,       GL_HALF_FLOAT_OES,             LoadL16FToRGBA16F                    );
    InsertLoadFunction(&map, GL_LUMINANCE_ALPHA16F_EXT, GL_HALF_FLOAT,                 LoadLA16FToRGBA16F                   );
    InsertLoadFunction(&map, GL_LUMINANCE_ALPHA16F_EXT, GL_HALF_FLOAT_OES,             LoadLA16FToRGBA16F                   );

    // From GL_ANGLE_depth_texture
    InsertLoadFunction(&map, GL_DEPTH_COMPONENT32_OES,  GL_UNSIGNED_INT,               LoadR32ToR24G8                       );

    // From GL_EXT_texture_format_BGRA8888
    InsertLoadFunction(&map, GL_BGRA8_EXT,              GL_UNSIGNED_BYTE,                  LoadToNative<GLubyte, 4>         );
    InsertLoadFunction(&map, GL_BGRA4_ANGLEX,           GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT, LoadRGBA4ToRGBA8                 );
    InsertLoadFunction(&map, GL_BGRA4_ANGLEX,           GL_UNSIGNED_BYTE,                  LoadToNative<GLubyte, 4>         );
    InsertLoadFunction(&map, GL_BGR5_A1_ANGLEX,         GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT, LoadRGB5A1ToRGBA8                );
    InsertLoadFunction(&map, GL_BGR5_A1_ANGLEX,         GL_UNSIGNED_BYTE,                  LoadToNative<GLubyte, 4>         );

    // Compressed formats
    // From ES 3.0.1 spec, table 3.16
    //                      | Internal format                             | Type            | Load function                  |
    InsertLoadFunction(&map, GL_COMPRESSED_R11_EAC,                        GL_UNSIGNED_BYTE, UnimplementedLoadFunction       );
    InsertLoadFunction(&map, GL_COMPRESSED_R11_EAC,                        GL_UNSIGNED_BYTE, UnimplementedLoadFunction       );
    InsertLoadFunction(&map, GL_COMPRESSED_SIGNED_R11_EAC,                 GL_UNSIGNED_BYTE, UnimplementedLoadFunction       );
    InsertLoadFunction(&map, GL_COMPRESSED_RG11_EAC,                       GL_UNSIGNED_BYTE, UnimplementedLoadFunction       );
    InsertLoadFunction(&map, GL_COMPRESSED_SIGNED_RG11_EAC,                GL_UNSIGNED_BYTE, UnimplementedLoadFunction       );
    InsertLoadFunction(&map, GL_COMPRESSED_RGB8_ETC2,                      GL_UNSIGNED_BYTE, UnimplementedLoadFunction       );
    InsertLoadFunction(&map, GL_COMPRESSED_SRGB8_ETC2,                     GL_UNSIGNED_BYTE, UnimplementedLoadFunction       );
    InsertLoadFunction(&map, GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,  GL_UNSIGNED_BYTE, UnimplementedLoadFunction       );
    InsertLoadFunction(&map, GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_UNSIGNED_BYTE, UnimplementedLoadFunction       );
    InsertLoadFunction(&map, GL_COMPRESSED_RGBA8_ETC2_EAC,                 GL_UNSIGNED_BYTE, UnimplementedLoadFunction       );
    InsertLoadFunction(&map, GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,          GL_UNSIGNED_BYTE, UnimplementedLoadFunction       );

    // From GL_EXT_texture_compression_dxt1
    InsertLoadFunction(&map, GL_COMPRESSED_RGB_S3TC_DXT1_EXT,              GL_UNSIGNED_BYTE, LoadCompressedToNative<4, 4,  8>);
    InsertLoadFunction(&map, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,             GL_UNSIGNED_BYTE, LoadCompressedToNative<4, 4,  8>);

    // From GL_ANGLE_texture_compression_dxt3
    InsertLoadFunction(&map, GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE,           GL_UNSIGNED_BYTE, LoadCompressedToNative<4, 4, 16>);

    // From GL_ANGLE_texture_compression_dxt5
    InsertLoadFunction(&map, GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE,           GL_UNSIGNED_BYTE, LoadCompressedToNative<4, 4, 16>);

    return map;
}

// For sized GL internal formats, there is only one corresponding D3D11 format. This map type allows
// querying for the DXGI texture formats to use for textures, SRVs, RTVs and DSVs given a GL internal
// format.
typedef std::map<GLenum, TextureFormat> D3D11ES3FormatMap;

TextureFormat::TextureFormat()
    : texFormat(DXGI_FORMAT_UNKNOWN),
      srvFormat(DXGI_FORMAT_UNKNOWN),
      rtvFormat(DXGI_FORMAT_UNKNOWN),
      dsvFormat(DXGI_FORMAT_UNKNOWN),
      renderFormat(DXGI_FORMAT_UNKNOWN),
      swizzleTexFormat(DXGI_FORMAT_UNKNOWN),
      swizzleSRVFormat(DXGI_FORMAT_UNKNOWN),
      swizzleRTVFormat(DXGI_FORMAT_UNKNOWN),
      dataInitializerFunction(NULL),
      loadFunctions()
{
}

static inline void InsertD3D11FormatInfo(D3D11ES3FormatMap *map, GLenum internalFormat, DXGI_FORMAT texFormat,
                                         DXGI_FORMAT srvFormat, DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat)
{
    TextureFormat info;
    info.texFormat = texFormat;
    info.srvFormat = srvFormat;
    info.rtvFormat = rtvFormat;
    info.dsvFormat = dsvFormat;

    // Given a GL internal format, the renderFormat is the DSV format if it is depth- or stencil-renderable,
    // the RTV format if it is color-renderable, and the (nonrenderable) texture format otherwise.
    if (dsvFormat != DXGI_FORMAT_UNKNOWN)
    {
        info.renderFormat = dsvFormat;
    }
    else if (rtvFormat != DXGI_FORMAT_UNKNOWN)
    {
        info.renderFormat = rtvFormat;
    }
    else if (texFormat != DXGI_FORMAT_UNKNOWN)
    {
        info.renderFormat = texFormat;
    }
    else
    {
        info.renderFormat = DXGI_FORMAT_UNKNOWN;
    }

    // Compute the swizzle formats
    const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(internalFormat);
    if (internalFormat != GL_NONE && formatInfo.pixelBytes > 0)
    {
        if (formatInfo.componentCount != 4 || texFormat == DXGI_FORMAT_UNKNOWN ||
            srvFormat == DXGI_FORMAT_UNKNOWN || rtvFormat == DXGI_FORMAT_UNKNOWN)
        {
            // Get the maximum sized component
            unsigned int maxBits = 1;
            if (formatInfo.compressed)
            {
                unsigned int compressedBitsPerBlock = formatInfo.pixelBytes * 8;
                unsigned int blockSize = formatInfo.compressedBlockWidth * formatInfo.compressedBlockHeight;
                maxBits = std::max(compressedBitsPerBlock / blockSize, maxBits);
            }
            else
            {
                maxBits = std::max(maxBits, formatInfo.alphaBits);
                maxBits = std::max(maxBits, formatInfo.redBits);
                maxBits = std::max(maxBits, formatInfo.greenBits);
                maxBits = std::max(maxBits, formatInfo.blueBits);
                maxBits = std::max(maxBits, formatInfo.luminanceBits);
                maxBits = std::max(maxBits, formatInfo.depthBits);
            }

            maxBits = roundUp(maxBits, 8U);

            static const SwizzleInfoMap swizzleMap = BuildSwizzleInfoMap();
            SwizzleInfoMap::const_iterator swizzleIter = swizzleMap.find(SwizzleSizeType(maxBits, formatInfo.componentType));
            ASSERT(swizzleIter != swizzleMap.end());

            const SwizzleFormatInfo &swizzleInfo = swizzleIter->second;
            info.swizzleTexFormat = swizzleInfo.mTexFormat;
            info.swizzleSRVFormat = swizzleInfo.mSRVFormat;
            info.swizzleRTVFormat = swizzleInfo.mRTVFormat;
        }
        else
        {
            // The original texture format is suitable for swizzle operations
            info.swizzleTexFormat = texFormat;
            info.swizzleSRVFormat = srvFormat;
            info.swizzleRTVFormat = rtvFormat;
        }
    }
    else
    {
        // Not possible to swizzle with this texture format since it is either unsized or GL_NONE
        info.swizzleTexFormat = DXGI_FORMAT_UNKNOWN;
        info.swizzleSRVFormat = DXGI_FORMAT_UNKNOWN;
        info.swizzleRTVFormat = DXGI_FORMAT_UNKNOWN;
    }

    // Check if there is an initialization function for this texture format
    static const InternalFormatInitializerMap initializerMap = BuildInternalFormatInitializerMap();
    InternalFormatInitializerMap::const_iterator initializerIter = initializerMap.find(internalFormat);
    info.dataInitializerFunction = (initializerIter != initializerMap.end()) ? initializerIter->second : NULL;

    // Gather all the load functions for this internal format
    static const D3D11LoadFunctionMap loadFunctions = BuildD3D11LoadFunctionMap();
    D3D11LoadFunctionMap::const_iterator loadFunctionIter = loadFunctions.find(internalFormat);
    if (loadFunctionIter != loadFunctions.end())
    {
        const std::vector<TypeLoadFunctionPair> &loadFunctionVector = loadFunctionIter->second;
        for (size_t i = 0; i < loadFunctionVector.size(); i++)
        {
            GLenum type = loadFunctionVector[i].first;
            LoadImageFunction function = loadFunctionVector[i].second;
            info.loadFunctions.insert(std::make_pair(type, function));
        }
    }

    map->insert(std::make_pair(internalFormat, info));
}

static D3D11ES3FormatMap BuildD3D11FormatMap()
{
    D3D11ES3FormatMap map;

    //                         | GL internal format  | D3D11 texture format            | D3D11 SRV format               | D3D11 RTV format               | D3D11 DSV format   |
    InsertD3D11FormatInfo(&map, GL_NONE,              DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_R8,                DXGI_FORMAT_R8_UNORM,             DXGI_FORMAT_R8_UNORM,            DXGI_FORMAT_R8_UNORM,            DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_R8_SNORM,          DXGI_FORMAT_R8_SNORM,             DXGI_FORMAT_R8_SNORM,            DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RG8,               DXGI_FORMAT_R8G8_UNORM,           DXGI_FORMAT_R8G8_UNORM,          DXGI_FORMAT_R8G8_UNORM,          DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RG8_SNORM,         DXGI_FORMAT_R8G8_SNORM,           DXGI_FORMAT_R8G8_SNORM,          DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGB8,              DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGB8_SNORM,        DXGI_FORMAT_R8G8B8A8_SNORM,       DXGI_FORMAT_R8G8B8A8_SNORM,      DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGB565,            DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGBA4,             DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGB5_A1,           DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGBA8,             DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGBA8_SNORM,       DXGI_FORMAT_R8G8B8A8_SNORM,       DXGI_FORMAT_R8G8B8A8_SNORM,      DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGB10_A2,          DXGI_FORMAT_R10G10B10A2_UNORM,    DXGI_FORMAT_R10G10B10A2_UNORM,   DXGI_FORMAT_R10G10B10A2_UNORM,   DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGB10_A2UI,        DXGI_FORMAT_R10G10B10A2_UINT,     DXGI_FORMAT_R10G10B10A2_UINT,    DXGI_FORMAT_R10G10B10A2_UINT,    DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_SRGB8,             DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,  DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_SRGB8_ALPHA8,      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,  DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_R16F,              DXGI_FORMAT_R16_FLOAT,            DXGI_FORMAT_R16_FLOAT,           DXGI_FORMAT_R16_FLOAT,           DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RG16F,             DXGI_FORMAT_R16G16_FLOAT,         DXGI_FORMAT_R16G16_FLOAT,        DXGI_FORMAT_R16G16_FLOAT,        DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGB16F,            DXGI_FORMAT_R16G16B16A16_FLOAT,   DXGI_FORMAT_R16G16B16A16_FLOAT,  DXGI_FORMAT_R16G16B16A16_FLOAT,  DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGBA16F,           DXGI_FORMAT_R16G16B16A16_FLOAT,   DXGI_FORMAT_R16G16B16A16_FLOAT,  DXGI_FORMAT_R16G16B16A16_FLOAT,  DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_R32F,              DXGI_FORMAT_R32_FLOAT,            DXGI_FORMAT_R32_FLOAT,           DXGI_FORMAT_R32_FLOAT,           DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RG32F,             DXGI_FORMAT_R32G32_FLOAT,         DXGI_FORMAT_R32G32_FLOAT,        DXGI_FORMAT_R32G32_FLOAT,        DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGB32F,            DXGI_FORMAT_R32G32B32A32_FLOAT,   DXGI_FORMAT_R32G32B32A32_FLOAT,  DXGI_FORMAT_R32G32B32A32_FLOAT,  DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGBA32F,           DXGI_FORMAT_R32G32B32A32_FLOAT,   DXGI_FORMAT_R32G32B32A32_FLOAT,  DXGI_FORMAT_R32G32B32A32_FLOAT,  DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_R11F_G11F_B10F,    DXGI_FORMAT_R11G11B10_FLOAT,      DXGI_FORMAT_R11G11B10_FLOAT,     DXGI_FORMAT_R11G11B10_FLOAT,     DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGB9_E5,           DXGI_FORMAT_R9G9B9E5_SHAREDEXP,   DXGI_FORMAT_R9G9B9E5_SHAREDEXP,  DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_R8I,               DXGI_FORMAT_R8_SINT,              DXGI_FORMAT_R8_SINT,             DXGI_FORMAT_R8_SINT,             DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_R8UI,              DXGI_FORMAT_R8_UINT,              DXGI_FORMAT_R8_UINT,             DXGI_FORMAT_R8_UINT,             DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_R16I,              DXGI_FORMAT_R16_SINT,             DXGI_FORMAT_R16_SINT,            DXGI_FORMAT_R16_SINT,            DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_R16UI,             DXGI_FORMAT_R16_UINT,             DXGI_FORMAT_R16_UINT,            DXGI_FORMAT_R16_UINT,            DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_R32I,              DXGI_FORMAT_R32_SINT,             DXGI_FORMAT_R32_SINT,            DXGI_FORMAT_R32_SINT,            DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_R32UI,             DXGI_FORMAT_R32_UINT,             DXGI_FORMAT_R32_UINT,            DXGI_FORMAT_R32_UINT,            DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RG8I,              DXGI_FORMAT_R8G8_SINT,            DXGI_FORMAT_R8G8_SINT,           DXGI_FORMAT_R8G8_SINT,           DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RG8UI,             DXGI_FORMAT_R8G8_UINT,            DXGI_FORMAT_R8G8_UINT,           DXGI_FORMAT_R8G8_UINT,           DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RG16I,             DXGI_FORMAT_R16G16_SINT,          DXGI_FORMAT_R16G16_SINT,         DXGI_FORMAT_R16G16_SINT,         DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RG16UI,            DXGI_FORMAT_R16G16_UINT,          DXGI_FORMAT_R16G16_UINT,         DXGI_FORMAT_R16G16_UINT,         DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RG32I,             DXGI_FORMAT_R32G32_SINT,          DXGI_FORMAT_R32G32_SINT,         DXGI_FORMAT_R32G32_SINT,         DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RG32UI,            DXGI_FORMAT_R32G32_UINT,          DXGI_FORMAT_R32G32_UINT,         DXGI_FORMAT_R32G32_UINT,         DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGB8I,             DXGI_FORMAT_R8G8B8A8_SINT,        DXGI_FORMAT_R8G8B8A8_SINT,       DXGI_FORMAT_R8G8B8A8_SINT,       DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGB8UI,            DXGI_FORMAT_R8G8B8A8_UINT,        DXGI_FORMAT_R8G8B8A8_UINT,       DXGI_FORMAT_R8G8B8A8_UINT,       DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGB16I,            DXGI_FORMAT_R16G16B16A16_SINT,    DXGI_FORMAT_R16G16B16A16_SINT,   DXGI_FORMAT_R16G16B16A16_SINT,   DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGB16UI,           DXGI_FORMAT_R16G16B16A16_UINT,    DXGI_FORMAT_R16G16B16A16_UINT,   DXGI_FORMAT_R16G16B16A16_UINT,   DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGB32I,            DXGI_FORMAT_R32G32B32A32_SINT,    DXGI_FORMAT_R32G32B32A32_SINT,   DXGI_FORMAT_R32G32B32A32_SINT,   DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGB32UI,           DXGI_FORMAT_R32G32B32A32_UINT,    DXGI_FORMAT_R32G32B32A32_UINT,   DXGI_FORMAT_R32G32B32A32_UINT,   DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGBA8I,            DXGI_FORMAT_R8G8B8A8_SINT,        DXGI_FORMAT_R8G8B8A8_SINT,       DXGI_FORMAT_R8G8B8A8_SINT,       DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGBA8UI,           DXGI_FORMAT_R8G8B8A8_UINT,        DXGI_FORMAT_R8G8B8A8_UINT,       DXGI_FORMAT_R8G8B8A8_UINT,       DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGBA16I,           DXGI_FORMAT_R16G16B16A16_SINT,    DXGI_FORMAT_R16G16B16A16_SINT,   DXGI_FORMAT_R16G16B16A16_SINT,   DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGBA16UI,          DXGI_FORMAT_R16G16B16A16_UINT,    DXGI_FORMAT_R16G16B16A16_UINT,   DXGI_FORMAT_R16G16B16A16_UINT,   DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGBA32I,           DXGI_FORMAT_R32G32B32A32_SINT,    DXGI_FORMAT_R32G32B32A32_SINT,   DXGI_FORMAT_R32G32B32A32_SINT,   DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGBA32UI,          DXGI_FORMAT_R32G32B32A32_UINT,    DXGI_FORMAT_R32G32B32A32_UINT,   DXGI_FORMAT_R32G32B32A32_UINT,   DXGI_FORMAT_UNKNOWN);

    // Unsized formats, TODO: Are types of float and half float allowed for the unsized types? Would it change the DXGI format?
    InsertD3D11FormatInfo(&map, GL_ALPHA,             DXGI_FORMAT_A8_UNORM,             DXGI_FORMAT_A8_UNORM,            DXGI_FORMAT_A8_UNORM,            DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_LUMINANCE,         DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_LUMINANCE_ALPHA,   DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGB,               DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_RGBA,              DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_BGRA_EXT,          DXGI_FORMAT_B8G8R8A8_UNORM,       DXGI_FORMAT_B8G8R8A8_UNORM,      DXGI_FORMAT_B8G8R8A8_UNORM,      DXGI_FORMAT_UNKNOWN);

    // From GL_EXT_texture_storage
    //                           | GL internal format     | D3D11 texture format          | D3D11 SRV format                    | D3D11 RTV format              | D3D11 DSV format               |
    InsertD3D11FormatInfo(&map, GL_ALPHA8_EXT,             DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_R8G8B8A8_UNORM,           DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_UNKNOWN           );
    InsertD3D11FormatInfo(&map, GL_LUMINANCE8_EXT,         DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_R8G8B8A8_UNORM,           DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_UNKNOWN           );
    InsertD3D11FormatInfo(&map, GL_ALPHA32F_EXT,           DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT,       DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_UNKNOWN           );
    InsertD3D11FormatInfo(&map, GL_LUMINANCE32F_EXT,       DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT,       DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_UNKNOWN           );
    InsertD3D11FormatInfo(&map, GL_ALPHA16F_EXT,           DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT,       DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN           );
    InsertD3D11FormatInfo(&map, GL_LUMINANCE16F_EXT,       DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT,       DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN           );
    InsertD3D11FormatInfo(&map, GL_LUMINANCE8_ALPHA8_EXT,  DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_R8G8B8A8_UNORM,           DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_UNKNOWN           );
    InsertD3D11FormatInfo(&map, GL_LUMINANCE_ALPHA32F_EXT, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT,       DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_UNKNOWN           );
    InsertD3D11FormatInfo(&map, GL_LUMINANCE_ALPHA16F_EXT, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT,       DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN           );
    InsertD3D11FormatInfo(&map, GL_BGRA8_EXT,              DXGI_FORMAT_B8G8R8A8_UNORM,     DXGI_FORMAT_B8G8R8A8_UNORM,           DXGI_FORMAT_B8G8R8A8_UNORM,     DXGI_FORMAT_UNKNOWN           );
    InsertD3D11FormatInfo(&map, GL_BGRA4_ANGLEX,           DXGI_FORMAT_B8G8R8A8_UNORM,     DXGI_FORMAT_B8G8R8A8_UNORM,           DXGI_FORMAT_B8G8R8A8_UNORM,     DXGI_FORMAT_UNKNOWN           );
    InsertD3D11FormatInfo(&map, GL_BGR5_A1_ANGLEX,         DXGI_FORMAT_B8G8R8A8_UNORM,     DXGI_FORMAT_B8G8R8A8_UNORM,           DXGI_FORMAT_B8G8R8A8_UNORM,     DXGI_FORMAT_UNKNOWN           );

    // Depth stencil formats
    InsertD3D11FormatInfo(&map, GL_DEPTH_COMPONENT16,     DXGI_FORMAT_R16_TYPELESS,        DXGI_FORMAT_R16_UNORM,                DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_D16_UNORM         );
    InsertD3D11FormatInfo(&map, GL_DEPTH_COMPONENT24,     DXGI_FORMAT_R24G8_TYPELESS,      DXGI_FORMAT_R24_UNORM_X8_TYPELESS,    DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_D24_UNORM_S8_UINT );
    InsertD3D11FormatInfo(&map, GL_DEPTH_COMPONENT32F,    DXGI_FORMAT_R32_TYPELESS,        DXGI_FORMAT_R32_FLOAT,                DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_D32_FLOAT         );
    InsertD3D11FormatInfo(&map, GL_DEPTH24_STENCIL8,      DXGI_FORMAT_R24G8_TYPELESS,      DXGI_FORMAT_R24_UNORM_X8_TYPELESS,    DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_D24_UNORM_S8_UINT );
    InsertD3D11FormatInfo(&map, GL_DEPTH32F_STENCIL8,     DXGI_FORMAT_R32G8X24_TYPELESS,   DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_D32_FLOAT_S8X24_UINT);
    InsertD3D11FormatInfo(&map, GL_STENCIL_INDEX8,        DXGI_FORMAT_R24G8_TYPELESS,      DXGI_FORMAT_X24_TYPELESS_G8_UINT,     DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_D24_UNORM_S8_UINT );

    // From GL_ANGLE_depth_texture
    // Since D3D11 doesn't have a D32_UNORM format, use D24S8 which has comparable precision and matches the ES3 format.
    InsertD3D11FormatInfo(&map, GL_DEPTH_COMPONENT32_OES, DXGI_FORMAT_R24G8_TYPELESS,      DXGI_FORMAT_R24_UNORM_X8_TYPELESS,    DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_D24_UNORM_S8_UINT);

    // Compressed formats, From ES 3.0.1 spec, table 3.16
    //                           | GL internal format                        | D3D11 texture format | D3D11 SRV format     | D3D11 RTV format   | D3D11 DSV format  |
    InsertD3D11FormatInfo(&map, GL_COMPRESSED_R11_EAC,                        DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_COMPRESSED_SIGNED_R11_EAC,                 DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_COMPRESSED_RG11_EAC,                       DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_COMPRESSED_SIGNED_RG11_EAC,                DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_COMPRESSED_RGB8_ETC2,                      DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_COMPRESSED_SRGB8_ETC2,                     DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,  DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_COMPRESSED_RGBA8_ETC2_EAC,                 DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,          DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN);

    // From GL_EXT_texture_compression_dxt1
    InsertD3D11FormatInfo(&map, GL_COMPRESSED_RGB_S3TC_DXT1_EXT,              DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN);
    InsertD3D11FormatInfo(&map, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,             DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN);

    // From GL_ANGLE_texture_compression_dxt3
    InsertD3D11FormatInfo(&map, GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE,           DXGI_FORMAT_BC2_UNORM, DXGI_FORMAT_BC2_UNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN);

    // From GL_ANGLE_texture_compression_dxt5
    InsertD3D11FormatInfo(&map, GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE,           DXGI_FORMAT_BC3_UNORM, DXGI_FORMAT_BC3_UNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN);

    return map;
}

const TextureFormat &GetTextureFormatInfo(GLenum internalFormat)
{
    static const D3D11ES3FormatMap formatMap = BuildD3D11FormatMap();
    D3D11ES3FormatMap::const_iterator iter = formatMap.find(internalFormat);
    if (iter != formatMap.end())
    {
        return iter->second;
    }
    else
    {
        static const TextureFormat defaultInfo;
        return defaultInfo;
    }
}

typedef std::map<gl::VertexFormat, VertexFormat> D3D11VertexFormatInfoMap;
typedef std::pair<gl::VertexFormat, VertexFormat> D3D11VertexFormatPair;

VertexFormat::VertexFormat()
    : conversionType(VERTEX_CONVERT_NONE),
      nativeFormat(DXGI_FORMAT_UNKNOWN),
      copyFunction(NULL)
{
}

static void AddVertexFormatInfo(D3D11VertexFormatInfoMap *map, GLenum inputType, GLboolean normalized, GLuint componentCount,
                                VertexConversionType conversionType, DXGI_FORMAT nativeFormat, VertexCopyFunction copyFunction)
{
    gl::VertexFormat inputFormat(inputType, normalized, componentCount, false);

    VertexFormat info;
    info.conversionType = conversionType;
    info.nativeFormat = nativeFormat;
    info.copyFunction = copyFunction;

    map->insert(D3D11VertexFormatPair(inputFormat, info));
}

static void AddIntegerVertexFormatInfo(D3D11VertexFormatInfoMap *map, GLenum inputType, GLuint componentCount,
                                       VertexConversionType conversionType, DXGI_FORMAT nativeFormat, VertexCopyFunction copyFunction)
{
    gl::VertexFormat inputFormat(inputType, GL_FALSE, componentCount, true);

    VertexFormat info;
    info.conversionType = conversionType;
    info.nativeFormat = nativeFormat;
    info.copyFunction = copyFunction;

    map->insert(D3D11VertexFormatPair(inputFormat, info));
}

static D3D11VertexFormatInfoMap BuildD3D11VertexFormatInfoMap()
{
    D3D11VertexFormatInfoMap map;

    // TODO: column legend

    //
    // Float formats
    //

    // GL_BYTE -- un-normalized
    AddVertexFormatInfo(&map, GL_BYTE,           GL_FALSE, 1, VERTEX_CONVERT_GPU,  DXGI_FORMAT_R8_SINT,            &CopyNativeVertexData<GLbyte, 1, 0>);
    AddVertexFormatInfo(&map, GL_BYTE,           GL_FALSE, 2, VERTEX_CONVERT_GPU,  DXGI_FORMAT_R8G8_SINT,          &CopyNativeVertexData<GLbyte, 2, 0>);
    AddVertexFormatInfo(&map, GL_BYTE,           GL_FALSE, 3, VERTEX_CONVERT_BOTH, DXGI_FORMAT_R8G8B8A8_SINT,      &CopyNativeVertexData<GLbyte, 3, 1>);
    AddVertexFormatInfo(&map, GL_BYTE,           GL_FALSE, 4, VERTEX_CONVERT_GPU,  DXGI_FORMAT_R8G8B8A8_SINT,      &CopyNativeVertexData<GLbyte, 4, 0>);

    // GL_BYTE -- normalized
    AddVertexFormatInfo(&map, GL_BYTE,           GL_TRUE,  1, VERTEX_CONVERT_NONE, DXGI_FORMAT_R8_SNORM,           &CopyNativeVertexData<GLbyte, 1, 0>);
    AddVertexFormatInfo(&map, GL_BYTE,           GL_TRUE,  2, VERTEX_CONVERT_NONE, DXGI_FORMAT_R8G8_SNORM,         &CopyNativeVertexData<GLbyte, 2, 0>);
    AddVertexFormatInfo(&map, GL_BYTE,           GL_TRUE,  3, VERTEX_CONVERT_CPU,  DXGI_FORMAT_R8G8B8A8_SNORM,     &CopyNativeVertexData<GLbyte, 3, INT8_MAX>);
    AddVertexFormatInfo(&map, GL_BYTE,           GL_TRUE,  4, VERTEX_CONVERT_NONE, DXGI_FORMAT_R8G8B8A8_SNORM,     &CopyNativeVertexData<GLbyte, 4, 0>);

    // GL_UNSIGNED_BYTE -- un-normalized
    AddVertexFormatInfo(&map, GL_UNSIGNED_BYTE,  GL_FALSE, 1, VERTEX_CONVERT_GPU,  DXGI_FORMAT_R8_UINT,            &CopyNativeVertexData<GLubyte, 1, 0>);
    AddVertexFormatInfo(&map, GL_UNSIGNED_BYTE,  GL_FALSE, 2, VERTEX_CONVERT_GPU,  DXGI_FORMAT_R8G8_UINT,          &CopyNativeVertexData<GLubyte, 2, 0>);
    AddVertexFormatInfo(&map, GL_UNSIGNED_BYTE,  GL_FALSE, 3, VERTEX_CONVERT_BOTH, DXGI_FORMAT_R8G8B8A8_UINT,      &CopyNativeVertexData<GLubyte, 3, 1>);
    AddVertexFormatInfo(&map, GL_UNSIGNED_BYTE,  GL_FALSE, 4, VERTEX_CONVERT_GPU,  DXGI_FORMAT_R8G8B8A8_UINT,      &CopyNativeVertexData<GLubyte, 4, 0>);

    // GL_UNSIGNED_BYTE -- normalized
    AddVertexFormatInfo(&map, GL_UNSIGNED_BYTE,  GL_TRUE,  1, VERTEX_CONVERT_NONE, DXGI_FORMAT_R8_UNORM,           &CopyNativeVertexData<GLubyte, 1, 0>);
    AddVertexFormatInfo(&map, GL_UNSIGNED_BYTE,  GL_TRUE,  2, VERTEX_CONVERT_NONE, DXGI_FORMAT_R8G8_UNORM,         &CopyNativeVertexData<GLubyte, 2, 0>);
    AddVertexFormatInfo(&map, GL_UNSIGNED_BYTE,  GL_TRUE,  3, VERTEX_CONVERT_CPU,  DXGI_FORMAT_R8G8B8A8_UNORM,     &CopyNativeVertexData<GLubyte, 3, UINT8_MAX>);
    AddVertexFormatInfo(&map, GL_UNSIGNED_BYTE,  GL_TRUE,  4, VERTEX_CONVERT_NONE, DXGI_FORMAT_R8G8B8A8_UNORM,     &CopyNativeVertexData<GLubyte, 4, 0>);

    // GL_SHORT -- un-normalized
    AddVertexFormatInfo(&map, GL_SHORT,          GL_FALSE, 1, VERTEX_CONVERT_GPU,  DXGI_FORMAT_R16_SINT,           &CopyNativeVertexData<GLshort, 1, 0>);
    AddVertexFormatInfo(&map, GL_SHORT,          GL_FALSE, 2, VERTEX_CONVERT_GPU,  DXGI_FORMAT_R16G16_SINT,        &CopyNativeVertexData<GLshort, 2, 0>);
    AddVertexFormatInfo(&map, GL_SHORT,          GL_FALSE, 3, VERTEX_CONVERT_BOTH, DXGI_FORMAT_R16G16B16A16_SINT,  &CopyNativeVertexData<GLshort, 4, 1>);
    AddVertexFormatInfo(&map, GL_SHORT,          GL_FALSE, 4, VERTEX_CONVERT_GPU,  DXGI_FORMAT_R16G16B16A16_SINT,  &CopyNativeVertexData<GLshort, 4, 0>);

    // GL_SHORT -- normalized
    AddVertexFormatInfo(&map, GL_SHORT,          GL_TRUE,  1, VERTEX_CONVERT_NONE, DXGI_FORMAT_R16_SNORM,          &CopyNativeVertexData<GLshort, 1, 0>);
    AddVertexFormatInfo(&map, GL_SHORT,          GL_TRUE,  2, VERTEX_CONVERT_NONE, DXGI_FORMAT_R16G16_SNORM,       &CopyNativeVertexData<GLshort, 2, 0>);
    AddVertexFormatInfo(&map, GL_SHORT,          GL_TRUE,  3, VERTEX_CONVERT_CPU,  DXGI_FORMAT_R16G16B16A16_SNORM, &CopyNativeVertexData<GLshort, 3, INT16_MAX>);
    AddVertexFormatInfo(&map, GL_SHORT,          GL_TRUE,  4, VERTEX_CONVERT_NONE, DXGI_FORMAT_R16G16B16A16_SNORM, &CopyNativeVertexData<GLshort, 4, 0>);

    // GL_UNSIGNED_SHORT -- un-normalized
    AddVertexFormatInfo(&map, GL_UNSIGNED_SHORT, GL_FALSE, 1, VERTEX_CONVERT_GPU,  DXGI_FORMAT_R16_UINT,           &CopyNativeVertexData<GLushort, 1, 0>);
    AddVertexFormatInfo(&map, GL_UNSIGNED_SHORT, GL_FALSE, 2, VERTEX_CONVERT_GPU,  DXGI_FORMAT_R16G16_UINT,        &CopyNativeVertexData<GLushort, 2, 0>);
    AddVertexFormatInfo(&map, GL_UNSIGNED_SHORT, GL_FALSE, 3, VERTEX_CONVERT_BOTH, DXGI_FORMAT_R16G16B16A16_UINT,  &CopyNativeVertexData<GLushort, 3, 1>);
    AddVertexFormatInfo(&map, GL_UNSIGNED_SHORT, GL_FALSE, 4, VERTEX_CONVERT_GPU,  DXGI_FORMAT_R16G16B16A16_UINT,  &CopyNativeVertexData<GLushort, 4, 0>);

    // GL_UNSIGNED_SHORT -- normalized
    AddVertexFormatInfo(&map, GL_UNSIGNED_SHORT, GL_TRUE,  1, VERTEX_CONVERT_NONE, DXGI_FORMAT_R16_UNORM,          &CopyNativeVertexData<GLushort, 1, 0>);
    AddVertexFormatInfo(&map, GL_UNSIGNED_SHORT, GL_TRUE,  2, VERTEX_CONVERT_NONE, DXGI_FORMAT_R16G16_UNORM,       &CopyNativeVertexData<GLushort, 2, 0>);
    AddVertexFormatInfo(&map, GL_UNSIGNED_SHORT, GL_TRUE,  3, VERTEX_CONVERT_CPU,  DXGI_FORMAT_R16G16B16A16_UNORM, &CopyNativeVertexData<GLushort, 3, UINT16_MAX>);
    AddVertexFormatInfo(&map, GL_UNSIGNED_SHORT, GL_TRUE,  4, VERTEX_CONVERT_NONE, DXGI_FORMAT_R16G16B16A16_UNORM, &CopyNativeVertexData<GLushort, 4, 0>);

    // GL_INT -- un-normalized
    AddVertexFormatInfo(&map, GL_INT,            GL_FALSE, 1, VERTEX_CONVERT_GPU,  DXGI_FORMAT_R32_SINT,           &CopyNativeVertexData<GLint, 1, 0>);
    AddVertexFormatInfo(&map, GL_INT,            GL_FALSE, 2, VERTEX_CONVERT_GPU,  DXGI_FORMAT_R32G32_SINT,        &CopyNativeVertexData<GLint, 2, 0>);
    AddVertexFormatInfo(&map, GL_INT,            GL_FALSE, 3, VERTEX_CONVERT_GPU,  DXGI_FORMAT_R32G32B32_SINT,     &CopyNativeVertexData<GLint, 3, 0>);
    AddVertexFormatInfo(&map, GL_INT,            GL_FALSE, 4, VERTEX_CONVERT_GPU,  DXGI_FORMAT_R32G32B32A32_SINT,  &CopyNativeVertexData<GLint, 4, 0>);

    // GL_INT -- normalized
    AddVertexFormatInfo(&map, GL_INT,            GL_TRUE,  1, VERTEX_CONVERT_CPU,  DXGI_FORMAT_R32_FLOAT,          &CopyTo32FVertexData<GLint, 1, true>);
    AddVertexFormatInfo(&map, GL_INT,            GL_TRUE,  2, VERTEX_CONVERT_CPU,  DXGI_FORMAT_R32G32_FLOAT,       &CopyTo32FVertexData<GLint, 2, true>);
    AddVertexFormatInfo(&map, GL_INT,            GL_TRUE,  3, VERTEX_CONVERT_CPU,  DXGI_FORMAT_R32G32B32_FLOAT,    &CopyTo32FVertexData<GLint, 3, true>);
    AddVertexFormatInfo(&map, GL_INT,            GL_TRUE,  4, VERTEX_CONVERT_CPU,  DXGI_FORMAT_R32G32B32A32_FLOAT, &CopyTo32FVertexData<GLint, 4, true>);

    // GL_UNSIGNED_INT -- un-normalized
    AddVertexFormatInfo(&map, GL_UNSIGNED_INT,   GL_FALSE, 1, VERTEX_CONVERT_GPU,  DXGI_FORMAT_R32_UINT,           &CopyNativeVertexData<GLuint, 1, 0>);
    AddVertexFormatInfo(&map, GL_UNSIGNED_INT,   GL_FALSE, 2, VERTEX_CONVERT_GPU,  DXGI_FORMAT_R32G32_UINT,        &CopyNativeVertexData<GLuint, 2, 0>);
    AddVertexFormatInfo(&map, GL_UNSIGNED_INT,   GL_FALSE, 3, VERTEX_CONVERT_GPU,  DXGI_FORMAT_R32G32B32_UINT,     &CopyNativeVertexData<GLuint, 3, 0>);
    AddVertexFormatInfo(&map, GL_UNSIGNED_INT,   GL_FALSE, 4, VERTEX_CONVERT_GPU,  DXGI_FORMAT_R32G32B32A32_UINT,  &CopyNativeVertexData<GLuint, 4, 0>);

    // GL_UNSIGNED_INT -- normalized
    AddVertexFormatInfo(&map, GL_UNSIGNED_INT,   GL_TRUE,  1, VERTEX_CONVERT_NONE, DXGI_FORMAT_R32_FLOAT,          &CopyTo32FVertexData<GLuint, 1, true>);
    AddVertexFormatInfo(&map, GL_UNSIGNED_INT,   GL_TRUE,  2, VERTEX_CONVERT_NONE, DXGI_FORMAT_R32G32_FLOAT,       &CopyTo32FVertexData<GLuint, 2, true>);
    AddVertexFormatInfo(&map, GL_UNSIGNED_INT,   GL_TRUE,  3, VERTEX_CONVERT_CPU,  DXGI_FORMAT_R32G32B32_FLOAT,    &CopyTo32FVertexData<GLuint, 3, true>);
    AddVertexFormatInfo(&map, GL_UNSIGNED_INT,   GL_TRUE,  4, VERTEX_CONVERT_NONE, DXGI_FORMAT_R32G32B32A32_FLOAT, &CopyTo32FVertexData<GLuint, 4, true>);

    // GL_FIXED
    AddVertexFormatInfo(&map, GL_FIXED,          GL_FALSE, 1, VERTEX_CONVERT_CPU,  DXGI_FORMAT_R32_FLOAT,          &Copy32FixedTo32FVertexData<1>);
    AddVertexFormatInfo(&map, GL_FIXED,          GL_FALSE, 2, VERTEX_CONVERT_CPU,  DXGI_FORMAT_R32G32_FLOAT,       &Copy32FixedTo32FVertexData<2>);
    AddVertexFormatInfo(&map, GL_FIXED,          GL_FALSE, 3, VERTEX_CONVERT_CPU,  DXGI_FORMAT_R32G32B32_FLOAT,    &Copy32FixedTo32FVertexData<3>);
    AddVertexFormatInfo(&map, GL_FIXED,          GL_FALSE, 4, VERTEX_CONVERT_CPU,  DXGI_FORMAT_R32G32B32A32_FLOAT, &Copy32FixedTo32FVertexData<4>);

    // GL_HALF_FLOAT
    AddVertexFormatInfo(&map, GL_HALF_FLOAT,     GL_FALSE, 1, VERTEX_CONVERT_NONE, DXGI_FORMAT_R16_FLOAT,          &CopyNativeVertexData<GLhalf, 1, 0>);
    AddVertexFormatInfo(&map, GL_HALF_FLOAT,     GL_FALSE, 2, VERTEX_CONVERT_NONE, DXGI_FORMAT_R16G16_FLOAT,       &CopyNativeVertexData<GLhalf, 2, 0>);
    AddVertexFormatInfo(&map, GL_HALF_FLOAT,     GL_FALSE, 3, VERTEX_CONVERT_CPU,  DXGI_FORMAT_R16G16B16A16_FLOAT, &CopyNativeVertexData<GLhalf, 3, gl::Float16One>);
    AddVertexFormatInfo(&map, GL_HALF_FLOAT,     GL_FALSE, 4, VERTEX_CONVERT_NONE, DXGI_FORMAT_R16G16B16A16_FLOAT, &CopyNativeVertexData<GLhalf, 4, 0>);

    // GL_FLOAT
    AddVertexFormatInfo(&map, GL_FLOAT,          GL_FALSE, 1, VERTEX_CONVERT_NONE, DXGI_FORMAT_R32_FLOAT,          &CopyNativeVertexData<GLfloat, 1, 0>);
    AddVertexFormatInfo(&map, GL_FLOAT,          GL_FALSE, 2, VERTEX_CONVERT_NONE, DXGI_FORMAT_R32G32_FLOAT,       &CopyNativeVertexData<GLfloat, 2, 0>);
    AddVertexFormatInfo(&map, GL_FLOAT,          GL_FALSE, 3, VERTEX_CONVERT_NONE, DXGI_FORMAT_R32G32B32_FLOAT,    &CopyNativeVertexData<GLfloat, 3, 0>);
    AddVertexFormatInfo(&map, GL_FLOAT,          GL_FALSE, 4, VERTEX_CONVERT_NONE, DXGI_FORMAT_R32G32B32A32_FLOAT, &CopyNativeVertexData<GLfloat, 4, 0>);

    // GL_INT_2_10_10_10_REV
    AddVertexFormatInfo(&map, GL_INT_2_10_10_10_REV,          GL_FALSE,  4, VERTEX_CONVERT_CPU,  DXGI_FORMAT_R32G32B32A32_FLOAT, &CopyXYZ10W2ToXYZW32FVertexData<true, false, true>);
    AddVertexFormatInfo(&map, GL_INT_2_10_10_10_REV,          GL_TRUE,   4, VERTEX_CONVERT_CPU,  DXGI_FORMAT_R32G32B32A32_FLOAT, &CopyXYZ10W2ToXYZW32FVertexData<true, true,  true>);

    // GL_UNSIGNED_INT_2_10_10_10_REV
    AddVertexFormatInfo(&map, GL_UNSIGNED_INT_2_10_10_10_REV, GL_FALSE,  4, VERTEX_CONVERT_CPU,  DXGI_FORMAT_R32G32B32A32_FLOAT, &CopyXYZ10W2ToXYZW32FVertexData<false, false, true>);
    AddVertexFormatInfo(&map, GL_UNSIGNED_INT_2_10_10_10_REV, GL_TRUE,   4, VERTEX_CONVERT_NONE, DXGI_FORMAT_R10G10B10A2_UNORM,  &CopyNativeVertexData<GLuint, 1, 0>);

    //
    // Integer Formats
    //

    // GL_BYTE
    AddIntegerVertexFormatInfo(&map, GL_BYTE,           1, VERTEX_CONVERT_NONE,  DXGI_FORMAT_R8_SINT,           &CopyNativeVertexData<GLbyte, 1, 0>);
    AddIntegerVertexFormatInfo(&map, GL_BYTE,           2, VERTEX_CONVERT_NONE,  DXGI_FORMAT_R8G8_SINT,         &CopyNativeVertexData<GLbyte, 2, 0>);
    AddIntegerVertexFormatInfo(&map, GL_BYTE,           3, VERTEX_CONVERT_CPU,   DXGI_FORMAT_R8G8B8A8_SINT,     &CopyNativeVertexData<GLbyte, 3, 1>);
    AddIntegerVertexFormatInfo(&map, GL_BYTE,           4, VERTEX_CONVERT_NONE,  DXGI_FORMAT_R8G8B8A8_SINT,     &CopyNativeVertexData<GLbyte, 4, 0>);

    // GL_UNSIGNED_BYTE
    AddIntegerVertexFormatInfo(&map, GL_UNSIGNED_BYTE,  1, VERTEX_CONVERT_NONE,  DXGI_FORMAT_R8_UINT,           &CopyNativeVertexData<GLubyte, 1, 0>);
    AddIntegerVertexFormatInfo(&map, GL_UNSIGNED_BYTE,  2, VERTEX_CONVERT_NONE,  DXGI_FORMAT_R8G8_UINT,         &CopyNativeVertexData<GLubyte, 2, 0>);
    AddIntegerVertexFormatInfo(&map, GL_UNSIGNED_BYTE,  3, VERTEX_CONVERT_CPU,   DXGI_FORMAT_R8G8B8A8_UINT,     &CopyNativeVertexData<GLubyte, 3, 1>);
    AddIntegerVertexFormatInfo(&map, GL_UNSIGNED_BYTE,  4, VERTEX_CONVERT_NONE,  DXGI_FORMAT_R8G8B8A8_UINT,     &CopyNativeVertexData<GLubyte, 4, 0>);

    // GL_SHORT
    AddIntegerVertexFormatInfo(&map, GL_SHORT,          1, VERTEX_CONVERT_NONE,  DXGI_FORMAT_R16_SINT,          &CopyNativeVertexData<GLshort, 1, 0>);
    AddIntegerVertexFormatInfo(&map, GL_SHORT,          2, VERTEX_CONVERT_NONE,  DXGI_FORMAT_R16G16_SINT,       &CopyNativeVertexData<GLshort, 2, 0>);
    AddIntegerVertexFormatInfo(&map, GL_SHORT,          3, VERTEX_CONVERT_CPU,   DXGI_FORMAT_R16G16B16A16_SINT, &CopyNativeVertexData<GLshort, 3, 1>);
    AddIntegerVertexFormatInfo(&map, GL_SHORT,          4, VERTEX_CONVERT_NONE,  DXGI_FORMAT_R16G16B16A16_SINT, &CopyNativeVertexData<GLshort, 4, 0>);

    // GL_UNSIGNED_SHORT
    AddIntegerVertexFormatInfo(&map, GL_UNSIGNED_SHORT, 1, VERTEX_CONVERT_NONE,  DXGI_FORMAT_R16_UINT,          &CopyNativeVertexData<GLushort, 1, 0>);
    AddIntegerVertexFormatInfo(&map, GL_UNSIGNED_SHORT, 2, VERTEX_CONVERT_NONE,  DXGI_FORMAT_R16G16_UINT,       &CopyNativeVertexData<GLushort, 2, 0>);
    AddIntegerVertexFormatInfo(&map, GL_UNSIGNED_SHORT, 3, VERTEX_CONVERT_CPU,   DXGI_FORMAT_R16G16B16A16_UINT, &CopyNativeVertexData<GLushort, 3, 1>);
    AddIntegerVertexFormatInfo(&map, GL_UNSIGNED_SHORT, 4, VERTEX_CONVERT_NONE,  DXGI_FORMAT_R16G16B16A16_UINT, &CopyNativeVertexData<GLushort, 4, 0>);

    // GL_INT
    AddIntegerVertexFormatInfo(&map, GL_INT,            1, VERTEX_CONVERT_NONE,  DXGI_FORMAT_R32_SINT,          &CopyNativeVertexData<GLint, 1, 0>);
    AddIntegerVertexFormatInfo(&map, GL_INT,            2, VERTEX_CONVERT_NONE,  DXGI_FORMAT_R32G32_SINT,       &CopyNativeVertexData<GLint, 2, 0>);
    AddIntegerVertexFormatInfo(&map, GL_INT,            3, VERTEX_CONVERT_NONE,  DXGI_FORMAT_R32G32B32_SINT,    &CopyNativeVertexData<GLint, 3, 0>);
    AddIntegerVertexFormatInfo(&map, GL_INT,            4, VERTEX_CONVERT_NONE,  DXGI_FORMAT_R32G32B32A32_SINT, &CopyNativeVertexData<GLint, 4, 0>);

    // GL_UNSIGNED_INT
    AddIntegerVertexFormatInfo(&map, GL_UNSIGNED_INT,   1, VERTEX_CONVERT_NONE,  DXGI_FORMAT_R32_SINT,          &CopyNativeVertexData<GLuint, 1, 0>);
    AddIntegerVertexFormatInfo(&map, GL_UNSIGNED_INT,   2, VERTEX_CONVERT_NONE,  DXGI_FORMAT_R32G32_SINT,       &CopyNativeVertexData<GLuint, 2, 0>);
    AddIntegerVertexFormatInfo(&map, GL_UNSIGNED_INT,   3, VERTEX_CONVERT_NONE,  DXGI_FORMAT_R32G32B32_SINT,    &CopyNativeVertexData<GLuint, 3, 0>);
    AddIntegerVertexFormatInfo(&map, GL_UNSIGNED_INT,   4, VERTEX_CONVERT_NONE,  DXGI_FORMAT_R32G32B32A32_SINT, &CopyNativeVertexData<GLuint, 4, 0>);

    // GL_INT_2_10_10_10_REV
    AddIntegerVertexFormatInfo(&map, GL_INT_2_10_10_10_REV, 4, VERTEX_CONVERT_CPU, DXGI_FORMAT_R16G16B16A16_SINT, &CopyXYZ10W2ToXYZW32FVertexData<true, true, false>);

    // GL_UNSIGNED_INT_2_10_10_10_REV
    AddIntegerVertexFormatInfo(&map, GL_UNSIGNED_INT_2_10_10_10_REV, 4, VERTEX_CONVERT_NONE, DXGI_FORMAT_R10G10B10A2_UINT, &CopyNativeVertexData<GLuint, 1, 0>);

    return map;
}

const VertexFormat &GetVertexFormatInfo(const gl::VertexFormat &vertexFormat)
{
    static const D3D11VertexFormatInfoMap vertexFormatMap = BuildD3D11VertexFormatInfoMap();

    D3D11VertexFormatInfoMap::const_iterator iter = vertexFormatMap.find(vertexFormat);
    if (iter != vertexFormatMap.end())
    {
        return iter->second;
    }
    else
    {
        static const VertexFormat defaultInfo;
        return defaultInfo;
    }
}

}

}
