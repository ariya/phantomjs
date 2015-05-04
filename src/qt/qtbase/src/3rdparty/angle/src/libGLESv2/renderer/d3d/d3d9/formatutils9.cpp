//
// Copyright (c) 2013-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// formatutils9.cpp: Queries for GL image formats and their translations to D3D9
// formats.

#include "libGLESv2/renderer/d3d/d3d9/formatutils9.h"
#include "libGLESv2/renderer/d3d/d3d9/Renderer9.h"
#include "libGLESv2/renderer/generatemip.h"
#include "libGLESv2/renderer/loadimage.h"
#include "libGLESv2/renderer/copyimage.h"
#include "libGLESv2/renderer/vertexconversion.h"

namespace rx
{

namespace d3d9
{

const D3DFORMAT D3DFMT_INTZ = ((D3DFORMAT)(MAKEFOURCC('I', 'N', 'T', 'Z')));
const D3DFORMAT D3DFMT_NULL = ((D3DFORMAT)(MAKEFOURCC('N', 'U', 'L', 'L')));

struct D3D9FastCopyFormat
{
    GLenum destFormat;
    GLenum destType;
    ColorCopyFunction copyFunction;

    D3D9FastCopyFormat(GLenum destFormat, GLenum destType, ColorCopyFunction copyFunction)
        : destFormat(destFormat), destType(destType), copyFunction(copyFunction)
    { }

    bool operator<(const D3D9FastCopyFormat& other) const
    {
        return memcmp(this, &other, sizeof(D3D9FastCopyFormat)) < 0;
    }
};

typedef std::multimap<D3DFORMAT, D3D9FastCopyFormat> D3D9FastCopyMap;

static D3D9FastCopyMap BuildFastCopyMap()
{
    D3D9FastCopyMap map;

    map.insert(std::make_pair(D3DFMT_A8R8G8B8, D3D9FastCopyFormat(GL_RGBA, GL_UNSIGNED_BYTE, CopyBGRA8ToRGBA8)));

    return map;
}

// A map to determine the pixel size and mip generation function of a given D3D format
typedef std::map<D3DFORMAT, D3DFormat> D3D9FormatInfoMap;

D3DFormat::D3DFormat()
    : pixelBytes(0),
      blockWidth(0),
      blockHeight(0),
      internalFormat(GL_NONE),
      mipGenerationFunction(NULL),
      colorReadFunction(NULL),
      fastCopyFunctions()
{
}

ColorCopyFunction D3DFormat::getFastCopyFunction(GLenum format, GLenum type) const
{
    FastCopyFunctionMap::const_iterator iter = fastCopyFunctions.find(std::make_pair(format, type));
    return (iter != fastCopyFunctions.end()) ? iter->second : NULL;
}

static inline void InsertD3DFormatInfo(D3D9FormatInfoMap *map, D3DFORMAT format, GLuint bits, GLuint blockWidth,
                                       GLuint blockHeight, GLenum internalFormat, MipGenerationFunction mipFunc,
                                       ColorReadFunction colorReadFunc)
{
    D3DFormat info;
    info.pixelBytes = bits / 8;
    info.blockWidth = blockWidth;
    info.blockHeight = blockHeight;
    info.internalFormat = internalFormat;
    info.mipGenerationFunction = mipFunc;
    info.colorReadFunction = colorReadFunc;

    static const D3D9FastCopyMap fastCopyMap = BuildFastCopyMap();
    std::pair<D3D9FastCopyMap::const_iterator, D3D9FastCopyMap::const_iterator> fastCopyIter = fastCopyMap.equal_range(format);
    for (D3D9FastCopyMap::const_iterator i = fastCopyIter.first; i != fastCopyIter.second; i++)
    {
        info.fastCopyFunctions.insert(std::make_pair(std::make_pair(i->second.destFormat, i->second.destType), i->second.copyFunction));
    }

    map->insert(std::make_pair(format, info));
}

static D3D9FormatInfoMap BuildD3D9FormatInfoMap()
{
    D3D9FormatInfoMap map;

    //                       | D3DFORMAT           | S  |W |H | Internal format                   | Mip generation function   | Color read function             |
    InsertD3DFormatInfo(&map, D3DFMT_NULL,            0, 0, 0, GL_NONE,                            NULL,                       NULL                             );
    InsertD3DFormatInfo(&map, D3DFMT_UNKNOWN,         0, 0, 0, GL_NONE,                            NULL,                       NULL                             );

    InsertD3DFormatInfo(&map, D3DFMT_L8,              8, 1, 1, GL_LUMINANCE8_EXT,                  GenerateMip<L8>,            ReadColor<L8, GLfloat>           );
    InsertD3DFormatInfo(&map, D3DFMT_A8,              8, 1, 1, GL_ALPHA8_EXT,                      GenerateMip<A8>,            ReadColor<A8, GLfloat>           );
    InsertD3DFormatInfo(&map, D3DFMT_A8L8,           16, 1, 1, GL_LUMINANCE8_ALPHA8_EXT,           GenerateMip<A8L8>,          ReadColor<A8L8, GLfloat>         );
    InsertD3DFormatInfo(&map, D3DFMT_A4R4G4B4,       16, 1, 1, GL_BGRA4_ANGLEX,                    GenerateMip<B4G4R4A4>,      ReadColor<B4G4R4A4, GLfloat>     );
    InsertD3DFormatInfo(&map, D3DFMT_A1R5G5B5,       16, 1, 1, GL_BGR5_A1_ANGLEX,                  GenerateMip<B5G5R5A1>,      ReadColor<B5G5R5A1, GLfloat>     );
    InsertD3DFormatInfo(&map, D3DFMT_R5G6B5,         16, 1, 1, GL_RGB565,                          GenerateMip<R5G6B5>,        ReadColor<R5G6B5, GLfloat>       );
    InsertD3DFormatInfo(&map, D3DFMT_X8R8G8B8,       32, 1, 1, GL_BGRA8_EXT,                       GenerateMip<B8G8R8X8>,      ReadColor<B8G8R8X8, GLfloat>     );
    InsertD3DFormatInfo(&map, D3DFMT_A8R8G8B8,       32, 1, 1, GL_BGRA8_EXT,                       GenerateMip<B8G8R8A8>,      ReadColor<B8G8R8A8, GLfloat>     );
    InsertD3DFormatInfo(&map, D3DFMT_R16F,           16, 1, 1, GL_R16F_EXT,                        GenerateMip<R16F>,          ReadColor<R16F, GLfloat>         );
    InsertD3DFormatInfo(&map, D3DFMT_G16R16F,        32, 1, 1, GL_RG16F_EXT,                       GenerateMip<R16G16F>,       ReadColor<R16G16F, GLfloat>      );
    InsertD3DFormatInfo(&map, D3DFMT_A16B16G16R16F,  64, 1, 1, GL_RGBA16F_EXT,                     GenerateMip<R16G16B16A16F>, ReadColor<R16G16B16A16F, GLfloat>);
    InsertD3DFormatInfo(&map, D3DFMT_R32F,           32, 1, 1, GL_R32F_EXT,                        GenerateMip<R32F>,          ReadColor<R32F, GLfloat>         );
    InsertD3DFormatInfo(&map, D3DFMT_G32R32F,        64, 1, 1, GL_RG32F_EXT,                       GenerateMip<R32G32F>,       ReadColor<R32G32F, GLfloat>      );
    InsertD3DFormatInfo(&map, D3DFMT_A32B32G32R32F, 128, 1, 1, GL_RGBA32F_EXT,                     GenerateMip<R32G32B32A32F>, ReadColor<R32G32B32A32F, GLfloat>);

    InsertD3DFormatInfo(&map, D3DFMT_D16,            16, 1, 1, GL_DEPTH_COMPONENT16,               NULL,                       NULL                             );
    InsertD3DFormatInfo(&map, D3DFMT_D24S8,          32, 1, 1, GL_DEPTH24_STENCIL8_OES,            NULL,                       NULL                             );
    InsertD3DFormatInfo(&map, D3DFMT_D24X8,          32, 1, 1, GL_DEPTH_COMPONENT16,               NULL,                       NULL                             );
    InsertD3DFormatInfo(&map, D3DFMT_D32,            32, 1, 1, GL_DEPTH_COMPONENT32_OES,           NULL,                       NULL                             );

    InsertD3DFormatInfo(&map, D3DFMT_INTZ,           32, 1, 1, GL_DEPTH24_STENCIL8_OES,            NULL,                       NULL                             );

    InsertD3DFormatInfo(&map, D3DFMT_DXT1,           64, 4, 4, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,   NULL,                       NULL                             );
    InsertD3DFormatInfo(&map, D3DFMT_DXT3,          128, 4, 4, GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE, NULL,                       NULL                             );
    InsertD3DFormatInfo(&map, D3DFMT_DXT5,          128, 4, 4, GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE, NULL,                       NULL                             );

    return map;
}

const D3DFormat &GetD3DFormatInfo(D3DFORMAT format)
{
    static const D3D9FormatInfoMap infoMap = BuildD3D9FormatInfoMap();
    D3D9FormatInfoMap::const_iterator iter = infoMap.find(format);
    if (iter != infoMap.end())
    {
        return iter->second;
    }
    else
    {
        static const D3DFormat defaultInfo;
        return defaultInfo;
    }
}



typedef std::pair<GLint, InitializeTextureDataFunction> InternalFormatInitialzerPair;
typedef std::map<GLint, InitializeTextureDataFunction> InternalFormatInitialzerMap;

static InternalFormatInitialzerMap BuildInternalFormatInitialzerMap()
{
    InternalFormatInitialzerMap map;

    map.insert(InternalFormatInitialzerPair(GL_RGB16F, Initialize4ComponentData<GLhalf,   0x0000,     0x0000,     0x0000,     gl::Float16One>));
    map.insert(InternalFormatInitialzerPair(GL_RGB32F, Initialize4ComponentData<GLfloat,  0x00000000, 0x00000000, 0x00000000, gl::Float32One>));

    return map;
}

// Each GL internal format corresponds to one D3D format and data loading function.
// Due to not all formats being available all the time, some of the function/format types are wrapped
// in templates that perform format support queries on a Renderer9 object which is supplied
// when requesting the function or format.

typedef bool(*FallbackPredicateFunction)();

template <FallbackPredicateFunction pred, LoadImageFunction prefered, LoadImageFunction fallback>
static void FallbackLoad(size_t width, size_t height, size_t depth,
                         const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                         uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    if (pred())
    {
        prefered(width, height, depth, input, inputRowPitch, inputDepthPitch, output, outputRowPitch, outputDepthPitch);
    }
    else
    {
        fallback(width, height, depth, input, inputRowPitch, inputDepthPitch, output, outputRowPitch, outputDepthPitch);
    }
}

static void UnreachableLoad(size_t width, size_t height, size_t depth,
                            const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                            uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    UNREACHABLE();
}

typedef std::pair<GLenum, TextureFormat> D3D9FormatPair;
typedef std::map<GLenum, TextureFormat> D3D9FormatMap;

TextureFormat::TextureFormat()
    : texFormat(D3DFMT_NULL),
      renderFormat(D3DFMT_NULL),
      dataInitializerFunction(NULL),
      loadFunction(UnreachableLoad)
{
}

static inline void InsertD3D9FormatInfo(D3D9FormatMap *map, GLenum internalFormat, D3DFORMAT texFormat,
                                        D3DFORMAT renderFormat, LoadImageFunction loadFunction)
{
    TextureFormat info;
    info.texFormat = texFormat;
    info.renderFormat = renderFormat;

    static const InternalFormatInitialzerMap dataInitializationMap = BuildInternalFormatInitialzerMap();
    InternalFormatInitialzerMap::const_iterator dataInitIter = dataInitializationMap.find(internalFormat);
    info.dataInitializerFunction = (dataInitIter != dataInitializationMap.end()) ? dataInitIter->second : NULL;

    info.loadFunction = loadFunction;

    map->insert(std::make_pair(internalFormat, info));
}

static D3D9FormatMap BuildD3D9FormatMap()
{
    D3D9FormatMap map;

    //                       | Internal format                     | Texture format      | Render format        | Load function                           |
    InsertD3D9FormatInfo(&map, GL_NONE,                             D3DFMT_NULL,          D3DFMT_NULL,           UnreachableLoad                          );

    // We choose to downsample the GL_DEPTH_COMPONENT32_OES format to a 24-bit format because D3DFMT_D32 is not widely
    // supported.  We're allowed to do this because:
    //  - The ES spec 2.0.25 sec 3.7.1 states that we're allowed to store texture formats with internal format
    //    resolutions of our own choosing.
    //  - OES_depth_texture states that downsampling of the depth formats is allowed.
    //  - ANGLE_depth_texture does not state minimum required resolutions of the depth texture formats it
    //    introduces.
    // In ES3 however, there are minimum resolutions for the texture formats and this would not be allowed.

    InsertD3D9FormatInfo(&map, GL_DEPTH_COMPONENT16,                D3DFMT_INTZ,          D3DFMT_D24S8,          UnreachableLoad                          );
    InsertD3D9FormatInfo(&map, GL_DEPTH_COMPONENT32_OES,            D3DFMT_INTZ,          D3DFMT_D24X8,          UnreachableLoad                          );
    InsertD3D9FormatInfo(&map, GL_DEPTH24_STENCIL8_OES,             D3DFMT_INTZ,          D3DFMT_D24S8,          UnreachableLoad                          );
    InsertD3D9FormatInfo(&map, GL_STENCIL_INDEX8,                   D3DFMT_UNKNOWN,       D3DFMT_D24S8,          UnreachableLoad                          ); // TODO: What's the texture format?

    InsertD3D9FormatInfo(&map, GL_RGBA32F_EXT,                      D3DFMT_A32B32G32R32F, D3DFMT_A32B32G32R32F,  LoadToNative<GLfloat, 4>                 );
    InsertD3D9FormatInfo(&map, GL_RGB32F_EXT,                       D3DFMT_A32B32G32R32F, D3DFMT_A32B32G32R32F,  LoadToNative3To4<GLfloat, gl::Float32One>);
    InsertD3D9FormatInfo(&map, GL_RG32F_EXT,                        D3DFMT_G32R32F,       D3DFMT_G32R32F,        LoadToNative<GLfloat, 2>                 );
    InsertD3D9FormatInfo(&map, GL_R32F_EXT,                         D3DFMT_R32F,          D3DFMT_R32F,           LoadToNative<GLfloat, 1>                 );
    InsertD3D9FormatInfo(&map, GL_ALPHA32F_EXT,                     D3DFMT_A32B32G32R32F, D3DFMT_UNKNOWN,        LoadA32FToRGBA32F                        );
    InsertD3D9FormatInfo(&map, GL_LUMINANCE32F_EXT,                 D3DFMT_A32B32G32R32F, D3DFMT_UNKNOWN,        LoadL32FToRGBA32F                        );
    InsertD3D9FormatInfo(&map, GL_LUMINANCE_ALPHA32F_EXT,           D3DFMT_A32B32G32R32F, D3DFMT_UNKNOWN,        LoadLA32FToRGBA32F                       );

    InsertD3D9FormatInfo(&map, GL_RGBA16F_EXT,                      D3DFMT_A16B16G16R16F, D3DFMT_A16B16G16R16F,  LoadToNative<GLhalf, 4>                  );
    InsertD3D9FormatInfo(&map, GL_RGB16F_EXT,                       D3DFMT_A16B16G16R16F, D3DFMT_A16B16G16R16F,  LoadToNative3To4<GLhalf, gl::Float16One> );
    InsertD3D9FormatInfo(&map, GL_RG16F_EXT,                        D3DFMT_G16R16F,       D3DFMT_G16R16F,        LoadToNative<GLhalf, 2>                  );
    InsertD3D9FormatInfo(&map, GL_R16F_EXT,                         D3DFMT_R16F,          D3DFMT_R16F,           LoadToNative<GLhalf, 1>                  );
    InsertD3D9FormatInfo(&map, GL_ALPHA16F_EXT,                     D3DFMT_A16B16G16R16F, D3DFMT_UNKNOWN,        LoadA16FToRGBA16F                        );
    InsertD3D9FormatInfo(&map, GL_LUMINANCE16F_EXT,                 D3DFMT_A16B16G16R16F, D3DFMT_UNKNOWN,        LoadL16FToRGBA16F                        );
    InsertD3D9FormatInfo(&map, GL_LUMINANCE_ALPHA16F_EXT,           D3DFMT_A16B16G16R16F, D3DFMT_UNKNOWN,        LoadLA16FToRGBA16F                       );

    InsertD3D9FormatInfo(&map, GL_ALPHA8_EXT,                       D3DFMT_A8R8G8B8,      D3DFMT_A8R8G8B8,       FallbackLoad<gl::supportsSSE2, LoadA8ToBGRA8_SSE2, LoadA8ToBGRA8>);

    InsertD3D9FormatInfo(&map, GL_RGB8_OES,                         D3DFMT_X8R8G8B8,      D3DFMT_X8R8G8B8,       LoadRGB8ToBGRX8                           );
    InsertD3D9FormatInfo(&map, GL_RGB565,                           D3DFMT_X8R8G8B8,      D3DFMT_X8R8G8B8,       LoadR5G6B5ToBGRA8                         );
    InsertD3D9FormatInfo(&map, GL_RGBA8_OES,                        D3DFMT_A8R8G8B8,      D3DFMT_A8R8G8B8,       FallbackLoad<gl::supportsSSE2, LoadRGBA8ToBGRA8_SSE2, LoadRGBA8ToBGRA8>);
    InsertD3D9FormatInfo(&map, GL_RGBA4,                            D3DFMT_A8R8G8B8,      D3DFMT_A8R8G8B8,       LoadRGBA4ToBGRA8                          );
    InsertD3D9FormatInfo(&map, GL_RGB5_A1,                          D3DFMT_A8R8G8B8,      D3DFMT_A8R8G8B8,       LoadRGB5A1ToBGRA8                         );
    InsertD3D9FormatInfo(&map, GL_R8_EXT,                           D3DFMT_X8R8G8B8,      D3DFMT_X8R8G8B8,       LoadR8ToBGRX8                             );
    InsertD3D9FormatInfo(&map, GL_RG8_EXT,                          D3DFMT_X8R8G8B8,      D3DFMT_X8R8G8B8,       LoadRG8ToBGRX8                            );

    InsertD3D9FormatInfo(&map, GL_BGRA8_EXT,                        D3DFMT_A8R8G8B8,      D3DFMT_A8R8G8B8,       LoadToNative<GLubyte, 4>                  );
    InsertD3D9FormatInfo(&map, GL_BGRA4_ANGLEX,                     D3DFMT_A8R8G8B8,      D3DFMT_A8R8G8B8,       LoadBGRA4ToBGRA8                          );
    InsertD3D9FormatInfo(&map, GL_BGR5_A1_ANGLEX,                   D3DFMT_A8R8G8B8,      D3DFMT_A8R8G8B8,       LoadBGR5A1ToBGRA8                         );

    InsertD3D9FormatInfo(&map, GL_COMPRESSED_RGB_S3TC_DXT1_EXT,     D3DFMT_DXT1,          D3DFMT_UNKNOWN,        LoadCompressedToNative<4, 4,  8>          );
    InsertD3D9FormatInfo(&map, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,    D3DFMT_DXT1,          D3DFMT_UNKNOWN,        LoadCompressedToNative<4, 4,  8>          );
    InsertD3D9FormatInfo(&map, GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE,  D3DFMT_DXT3,          D3DFMT_UNKNOWN,        LoadCompressedToNative<4, 4, 16>          );
    InsertD3D9FormatInfo(&map, GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE,  D3DFMT_DXT5,          D3DFMT_UNKNOWN,        LoadCompressedToNative<4, 4, 16>          );

    // These formats require checking if the renderer supports D3DFMT_L8 or D3DFMT_A8L8 and
    // then changing the format and loading function appropriately.
    InsertD3D9FormatInfo(&map, GL_LUMINANCE8_EXT,                   D3DFMT_L8,            D3DFMT_UNKNOWN,        LoadToNative<GLubyte, 1>                  );
    InsertD3D9FormatInfo(&map, GL_LUMINANCE8_ALPHA8_EXT,            D3DFMT_A8L8,          D3DFMT_UNKNOWN,        LoadToNative<GLubyte, 2>                  );

    return map;
}

const TextureFormat &GetTextureFormatInfo(GLenum internalFormat)
{
    static const D3D9FormatMap formatMap = BuildD3D9FormatMap();
    D3D9FormatMap::const_iterator iter = formatMap.find(internalFormat);
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

static GLenum GetDeclTypeComponentType(D3DDECLTYPE declType)
{
    switch (declType)
    {
      case D3DDECLTYPE_FLOAT1:   return GL_FLOAT;
      case D3DDECLTYPE_FLOAT2:   return GL_FLOAT;
      case D3DDECLTYPE_FLOAT3:   return GL_FLOAT;
      case D3DDECLTYPE_FLOAT4:   return GL_FLOAT;
      case D3DDECLTYPE_UBYTE4:   return GL_UNSIGNED_INT;
      case D3DDECLTYPE_SHORT2:   return GL_INT;
      case D3DDECLTYPE_SHORT4:   return GL_INT;
      case D3DDECLTYPE_UBYTE4N:  return GL_UNSIGNED_NORMALIZED;
      case D3DDECLTYPE_SHORT4N:  return GL_SIGNED_NORMALIZED;
      case D3DDECLTYPE_USHORT4N: return GL_UNSIGNED_NORMALIZED;
      case D3DDECLTYPE_SHORT2N:  return GL_SIGNED_NORMALIZED;
      case D3DDECLTYPE_USHORT2N: return GL_UNSIGNED_NORMALIZED;
      default: UNREACHABLE();    return GL_NONE;
    }
}

// Attribute format conversion
enum { NUM_GL_VERTEX_ATTRIB_TYPES = 6 };

struct TranslationDescription
{
    DWORD capsFlag;
    VertexFormat preferredConversion;
    VertexFormat fallbackConversion;
};

// Mapping from OpenGL-ES vertex attrib type to D3D decl type:
//
// BYTE                 SHORT (Cast)
// BYTE-norm            FLOAT (Normalize) (can't be exactly represented as SHORT-norm)
// UNSIGNED_BYTE        UBYTE4 (Identity) or SHORT (Cast)
// UNSIGNED_BYTE-norm   UBYTE4N (Identity) or FLOAT (Normalize)
// SHORT                SHORT (Identity)
// SHORT-norm           SHORT-norm (Identity) or FLOAT (Normalize)
// UNSIGNED_SHORT       FLOAT (Cast)
// UNSIGNED_SHORT-norm  USHORT-norm (Identity) or FLOAT (Normalize)
// FIXED (not in WebGL) FLOAT (FixedToFloat)
// FLOAT                FLOAT (Identity)

// GLToCType maps from GL type (as GLenum) to the C typedef.
template <GLenum GLType> struct GLToCType { };

template <> struct GLToCType<GL_BYTE>           { typedef GLbyte type;      };
template <> struct GLToCType<GL_UNSIGNED_BYTE>  { typedef GLubyte type;     };
template <> struct GLToCType<GL_SHORT>          { typedef GLshort type;     };
template <> struct GLToCType<GL_UNSIGNED_SHORT> { typedef GLushort type;    };
template <> struct GLToCType<GL_FIXED>          { typedef GLuint type;      };
template <> struct GLToCType<GL_FLOAT>          { typedef GLfloat type;     };

// This differs from D3DDECLTYPE in that it is unsized. (Size expansion is applied last.)
enum D3DVertexType
{
    D3DVT_FLOAT,
    D3DVT_SHORT,
    D3DVT_SHORT_NORM,
    D3DVT_UBYTE,
    D3DVT_UBYTE_NORM,
    D3DVT_USHORT_NORM
};

// D3DToCType maps from D3D vertex type (as enum D3DVertexType) to the corresponding C type.
template <unsigned int D3DType> struct D3DToCType { };

template <> struct D3DToCType<D3DVT_FLOAT> { typedef float type; };
template <> struct D3DToCType<D3DVT_SHORT> { typedef short type; };
template <> struct D3DToCType<D3DVT_SHORT_NORM> { typedef short type; };
template <> struct D3DToCType<D3DVT_UBYTE> { typedef unsigned char type; };
template <> struct D3DToCType<D3DVT_UBYTE_NORM> { typedef unsigned char type; };
template <> struct D3DToCType<D3DVT_USHORT_NORM> { typedef unsigned short type; };

// Encode the type/size combinations that D3D permits. For each type/size it expands to a widener that will provide the appropriate final size.
template <unsigned int type, int size> struct WidenRule { };

template <int size> struct WidenRule<D3DVT_FLOAT, size>          : NoWiden<size> { };
template <int size> struct WidenRule<D3DVT_SHORT, size>          : WidenToEven<size> { };
template <int size> struct WidenRule<D3DVT_SHORT_NORM, size>     : WidenToEven<size> { };
template <int size> struct WidenRule<D3DVT_UBYTE, size>          : WidenToFour<size> { };
template <int size> struct WidenRule<D3DVT_UBYTE_NORM, size>     : WidenToFour<size> { };
template <int size> struct WidenRule<D3DVT_USHORT_NORM, size>    : WidenToEven<size> { };

// VertexTypeFlags encodes the D3DCAPS9::DeclType flag and vertex declaration flag for each D3D vertex type & size combination.
template <unsigned int d3dtype, int size> struct VertexTypeFlags { };

template <unsigned int _capflag, unsigned int _declflag>
struct VertexTypeFlagsHelper
{
    enum { capflag = _capflag };
    enum { declflag = _declflag };
};

template <> struct VertexTypeFlags<D3DVT_FLOAT, 1> : VertexTypeFlagsHelper<0, D3DDECLTYPE_FLOAT1> { };
template <> struct VertexTypeFlags<D3DVT_FLOAT, 2> : VertexTypeFlagsHelper<0, D3DDECLTYPE_FLOAT2> { };
template <> struct VertexTypeFlags<D3DVT_FLOAT, 3> : VertexTypeFlagsHelper<0, D3DDECLTYPE_FLOAT3> { };
template <> struct VertexTypeFlags<D3DVT_FLOAT, 4> : VertexTypeFlagsHelper<0, D3DDECLTYPE_FLOAT4> { };
template <> struct VertexTypeFlags<D3DVT_SHORT, 2> : VertexTypeFlagsHelper<0, D3DDECLTYPE_SHORT2> { };
template <> struct VertexTypeFlags<D3DVT_SHORT, 4> : VertexTypeFlagsHelper<0, D3DDECLTYPE_SHORT4> { };
template <> struct VertexTypeFlags<D3DVT_SHORT_NORM, 2> : VertexTypeFlagsHelper<D3DDTCAPS_SHORT2N, D3DDECLTYPE_SHORT2N> { };
template <> struct VertexTypeFlags<D3DVT_SHORT_NORM, 4> : VertexTypeFlagsHelper<D3DDTCAPS_SHORT4N, D3DDECLTYPE_SHORT4N> { };
template <> struct VertexTypeFlags<D3DVT_UBYTE, 4> : VertexTypeFlagsHelper<D3DDTCAPS_UBYTE4, D3DDECLTYPE_UBYTE4> { };
template <> struct VertexTypeFlags<D3DVT_UBYTE_NORM, 4> : VertexTypeFlagsHelper<D3DDTCAPS_UBYTE4N, D3DDECLTYPE_UBYTE4N> { };
template <> struct VertexTypeFlags<D3DVT_USHORT_NORM, 2> : VertexTypeFlagsHelper<D3DDTCAPS_USHORT2N, D3DDECLTYPE_USHORT2N> { };
template <> struct VertexTypeFlags<D3DVT_USHORT_NORM, 4> : VertexTypeFlagsHelper<D3DDTCAPS_USHORT4N, D3DDECLTYPE_USHORT4N> { };


// VertexTypeMapping maps GL type & normalized flag to preferred and fallback D3D vertex types (as D3DVertexType enums).
template <GLenum GLtype, bool normalized> struct VertexTypeMapping { };

template <D3DVertexType Preferred, D3DVertexType Fallback = Preferred>
struct VertexTypeMappingBase
{
    enum { preferred = Preferred };
    enum { fallback = Fallback };
};

template <> struct VertexTypeMapping<GL_BYTE, false>                        : VertexTypeMappingBase<D3DVT_SHORT> { };                       // Cast
template <> struct VertexTypeMapping<GL_BYTE, true>                         : VertexTypeMappingBase<D3DVT_FLOAT> { };                       // Normalize
template <> struct VertexTypeMapping<GL_UNSIGNED_BYTE, false>               : VertexTypeMappingBase<D3DVT_UBYTE, D3DVT_FLOAT> { };          // Identity, Cast
template <> struct VertexTypeMapping<GL_UNSIGNED_BYTE, true>                : VertexTypeMappingBase<D3DVT_UBYTE_NORM, D3DVT_FLOAT> { };     // Identity, Normalize
template <> struct VertexTypeMapping<GL_SHORT, false>                       : VertexTypeMappingBase<D3DVT_SHORT> { };                       // Identity
template <> struct VertexTypeMapping<GL_SHORT, true>                        : VertexTypeMappingBase<D3DVT_SHORT_NORM, D3DVT_FLOAT> { };     // Cast, Normalize
template <> struct VertexTypeMapping<GL_UNSIGNED_SHORT, false>              : VertexTypeMappingBase<D3DVT_FLOAT> { };                       // Cast
template <> struct VertexTypeMapping<GL_UNSIGNED_SHORT, true>               : VertexTypeMappingBase<D3DVT_USHORT_NORM, D3DVT_FLOAT> { };    // Cast, Normalize
template <bool normalized> struct VertexTypeMapping<GL_FIXED, normalized>   : VertexTypeMappingBase<D3DVT_FLOAT> { };                       // FixedToFloat
template <bool normalized> struct VertexTypeMapping<GL_FLOAT, normalized>   : VertexTypeMappingBase<D3DVT_FLOAT> { };                       // Identity


// Given a GL type & norm flag and a D3D type, ConversionRule provides the type conversion rule (Cast, Normalize, Identity, FixedToFloat).
// The conversion rules themselves are defined in vertexconversion.h.

// Almost all cases are covered by Cast (including those that are actually Identity since Cast<T,T> knows it's an identity mapping).
template <GLenum fromType, bool normalized, unsigned int toType>
struct ConversionRule : Cast<typename GLToCType<fromType>::type, typename D3DToCType<toType>::type> { };

// All conversions from normalized types to float use the Normalize operator.
template <GLenum fromType> struct ConversionRule<fromType, true, D3DVT_FLOAT> : Normalize<typename GLToCType<fromType>::type> { };

// Use a full specialization for this so that it preferentially matches ahead of the generic normalize-to-float rules.
template <> struct ConversionRule<GL_FIXED, true, D3DVT_FLOAT>  : FixedToFloat<GLint, 16> { };
template <> struct ConversionRule<GL_FIXED, false, D3DVT_FLOAT> : FixedToFloat<GLint, 16> { };

// A 2-stage construction is used for DefaultVertexValues because float must use SimpleDefaultValues (i.e. 0/1)
// whether it is normalized or not.
template <class T, bool normalized> struct DefaultVertexValuesStage2 { };

template <class T> struct DefaultVertexValuesStage2<T, true>  : NormalizedDefaultValues<T> { };
template <class T> struct DefaultVertexValuesStage2<T, false> : SimpleDefaultValues<T> { };

// Work out the default value rule for a D3D type (expressed as the C type) and
template <class T, bool normalized> struct DefaultVertexValues : DefaultVertexValuesStage2<T, normalized> { };
template <bool normalized> struct DefaultVertexValues<float, normalized> : SimpleDefaultValues<float> { };

// Policy rules for use with Converter, to choose whether to use the preferred or fallback conversion.
// The fallback conversion produces an output that all D3D9 devices must support.
template <class T> struct UsePreferred { enum { type = T::preferred }; };
template <class T> struct UseFallback { enum { type = T::fallback }; };

// Converter ties it all together. Given an OpenGL type/norm/size and choice of preferred/fallback conversion,
// it provides all the members of the appropriate VertexDataConverter, the D3DCAPS9::DeclTypes flag in cap flag
// and the D3DDECLTYPE member needed for the vertex declaration in declflag.
template <GLenum fromType, bool normalized, int size, template <class T> class PreferenceRule>
struct Converter
    : VertexDataConverter<typename GLToCType<fromType>::type,
                          WidenRule<PreferenceRule< VertexTypeMapping<fromType, normalized> >::type, size>,
                          ConversionRule<fromType,
                                         normalized,
                                         PreferenceRule< VertexTypeMapping<fromType, normalized> >::type>,
                          DefaultVertexValues<typename D3DToCType<PreferenceRule< VertexTypeMapping<fromType, normalized> >::type>::type, normalized > >
{
private:
    enum { d3dtype = PreferenceRule< VertexTypeMapping<fromType, normalized> >::type };
    enum { d3dsize = WidenRule<d3dtype, size>::finalWidth };

public:
    enum { capflag = VertexTypeFlags<d3dtype, d3dsize>::capflag };
    enum { declflag = VertexTypeFlags<d3dtype, d3dsize>::declflag };
};

VertexFormat::VertexFormat()
    : conversionType(VERTEX_CONVERT_NONE),
      outputElementSize(0),
      copyFunction(NULL),
      nativeFormat(D3DDECLTYPE_UNUSED),
      componentType(GL_NONE)
{
}

// Initialize a TranslationInfo
VertexFormat CreateVertexFormatInfo(bool identity, size_t elementSize, VertexCopyFunction copyFunc, D3DDECLTYPE nativeFormat)
{
    VertexFormat formatInfo;
    formatInfo.conversionType = identity ? VERTEX_CONVERT_NONE : VERTEX_CONVERT_CPU;
    formatInfo.outputElementSize = elementSize;
    formatInfo.copyFunction = copyFunc;
    formatInfo.nativeFormat = nativeFormat;
    formatInfo.componentType = GetDeclTypeComponentType(nativeFormat);
    return formatInfo;
}

#define TRANSLATION(type, norm, size, preferred)                                    \
    CreateVertexFormatInfo                                                          \
    (                                                                               \
        Converter<type, norm, size, preferred>::identity,                           \
        Converter<type, norm, size, preferred>::finalSize,                          \
        Converter<type, norm, size, preferred>::convertArray,                       \
        static_cast<D3DDECLTYPE>(Converter<type, norm, size, preferred>::declflag)  \
    )

#define TRANSLATION_FOR_TYPE_NORM_SIZE(type, norm, size)    \
    {                                                       \
        Converter<type, norm, size, UsePreferred>::capflag, \
        TRANSLATION(type, norm, size, UsePreferred),        \
        TRANSLATION(type, norm, size, UseFallback)          \
    }

#define TRANSLATIONS_FOR_TYPE(type)                                                                                                                                                                         \
    {                                                                                                                                                                                                       \
        { TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 1), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 2), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 3), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 4) }, \
        { TRANSLATION_FOR_TYPE_NORM_SIZE(type, true, 1), TRANSLATION_FOR_TYPE_NORM_SIZE(type, true, 2), TRANSLATION_FOR_TYPE_NORM_SIZE(type, true, 3), TRANSLATION_FOR_TYPE_NORM_SIZE(type, true, 4) },     \
    }

#define TRANSLATIONS_FOR_TYPE_NO_NORM(type)                                                                                                                                                                 \
    {                                                                                                                                                                                                       \
        { TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 1), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 2), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 3), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 4) }, \
        { TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 1), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 2), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 3), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 4) }, \
    }

static inline unsigned int ComputeTypeIndex(GLenum type)
{
    switch (type)
    {
      case GL_BYTE:           return 0;
      case GL_UNSIGNED_BYTE:  return 1;
      case GL_SHORT:          return 2;
      case GL_UNSIGNED_SHORT: return 3;
      case GL_FIXED:          return 4;
      case GL_FLOAT:          return 5;

      default: UNREACHABLE(); return 5;
    }
}

const VertexFormat &GetVertexFormatInfo(DWORD supportedDeclTypes, const gl::VertexFormat &vertexFormat)
{
    static bool initialized = false;
    static DWORD intializedDeclTypes = 0;
    static VertexFormat formatConverters[NUM_GL_VERTEX_ATTRIB_TYPES][2][4];
    if (!initialized)
    {
        const TranslationDescription translations[NUM_GL_VERTEX_ATTRIB_TYPES][2][4] = // [GL types as enumerated by typeIndex()][normalized][size-1]
        {
            TRANSLATIONS_FOR_TYPE(GL_BYTE),
            TRANSLATIONS_FOR_TYPE(GL_UNSIGNED_BYTE),
            TRANSLATIONS_FOR_TYPE(GL_SHORT),
            TRANSLATIONS_FOR_TYPE(GL_UNSIGNED_SHORT),
            TRANSLATIONS_FOR_TYPE_NO_NORM(GL_FIXED),
            TRANSLATIONS_FOR_TYPE_NO_NORM(GL_FLOAT)
        };
        for (unsigned int i = 0; i < NUM_GL_VERTEX_ATTRIB_TYPES; i++)
        {
            for (unsigned int j = 0; j < 2; j++)
            {
                for (unsigned int k = 0; k < 4; k++)
                {
                    if (translations[i][j][k].capsFlag == 0 || (supportedDeclTypes & translations[i][j][k].capsFlag) != 0)
                    {
                        formatConverters[i][j][k] = translations[i][j][k].preferredConversion;
                    }
                    else
                    {
                        formatConverters[i][j][k] = translations[i][j][k].fallbackConversion;
                    }
                }
            }
        }
        initialized = true;
        intializedDeclTypes = supportedDeclTypes;
    }

    ASSERT(intializedDeclTypes == supportedDeclTypes);

    // Pure integer attributes only supported in ES3.0
    ASSERT(!vertexFormat.mPureInteger);
    return formatConverters[ComputeTypeIndex(vertexFormat.mType)][vertexFormat.mNormalized][vertexFormat.mComponents - 1];
}

}

}
