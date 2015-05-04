//
// Copyright (c) 2013-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// formatutils.cpp: Queries for GL image formats.

#include "common/mathutil.h"
#include "libGLESv2/formatutils.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/imageformats.h"
#include "libGLESv2/renderer/copyimage.h"

namespace gl
{

// ES2 requires that format is equal to internal format at all glTex*Image2D entry points and the implementation
// can decide the true, sized, internal format. The ES2FormatMap determines the internal format for all valid
// format and type combinations.

FormatType::FormatType()
    : internalFormat(GL_NONE),
      colorWriteFunction(NULL)
{
}

typedef std::pair<GLenum, GLenum> FormatTypePair;
typedef std::pair<FormatTypePair, FormatType> FormatPair;
typedef std::map<FormatTypePair, FormatType> FormatMap;

// A helper function to insert data into the format map with fewer characters.
static inline void InsertFormatMapping(FormatMap *map, GLenum format, GLenum type, GLenum internalFormat, ColorWriteFunction writeFunc)
{
    FormatType info;
    info.internalFormat = internalFormat;
    info.colorWriteFunction = writeFunc;
    map->insert(FormatPair(FormatTypePair(format, type), info));
}

FormatMap BuildFormatMap()
{
    FormatMap map;

    using namespace rx;

    //                       | Format               | Type                             | Internal format          | Color write function             |
    InsertFormatMapping(&map, GL_RGBA,               GL_UNSIGNED_BYTE,                  GL_RGBA8,                  WriteColor<R8G8B8A8, GLfloat>     );
    InsertFormatMapping(&map, GL_RGBA,               GL_BYTE,                           GL_RGBA8_SNORM,            WriteColor<R8G8B8A8S, GLfloat>    );
    InsertFormatMapping(&map, GL_RGBA,               GL_UNSIGNED_SHORT_4_4_4_4,         GL_RGBA4,                  WriteColor<R4G4B4A4, GLfloat>     );
    InsertFormatMapping(&map, GL_RGBA,               GL_UNSIGNED_SHORT_5_5_5_1,         GL_RGB5_A1,                WriteColor<R5G5B5A1, GLfloat>     );
    InsertFormatMapping(&map, GL_RGBA,               GL_UNSIGNED_INT_2_10_10_10_REV,    GL_RGB10_A2,               WriteColor<R10G10B10A2, GLfloat>  );
    InsertFormatMapping(&map, GL_RGBA,               GL_FLOAT,                          GL_RGBA32F,                WriteColor<R32G32B32A32F, GLfloat>);
    InsertFormatMapping(&map, GL_RGBA,               GL_HALF_FLOAT,                     GL_RGBA16F,                WriteColor<R16G16B16A16F, GLfloat>);
    InsertFormatMapping(&map, GL_RGBA,               GL_HALF_FLOAT_OES,                 GL_RGBA16F,                WriteColor<R16G16B16A16F, GLfloat>);

    InsertFormatMapping(&map, GL_RGBA_INTEGER,       GL_UNSIGNED_BYTE,                  GL_RGBA8UI,                WriteColor<R8G8B8A8, GLuint>      );
    InsertFormatMapping(&map, GL_RGBA_INTEGER,       GL_BYTE,                           GL_RGBA8I,                 WriteColor<R8G8B8A8S, GLint>      );
    InsertFormatMapping(&map, GL_RGBA_INTEGER,       GL_UNSIGNED_SHORT,                 GL_RGBA16UI,               WriteColor<R16G16B16A16, GLuint>  );
    InsertFormatMapping(&map, GL_RGBA_INTEGER,       GL_SHORT,                          GL_RGBA16I,                WriteColor<R16G16B16A16S, GLint>  );
    InsertFormatMapping(&map, GL_RGBA_INTEGER,       GL_UNSIGNED_INT,                   GL_RGBA32UI,               WriteColor<R32G32B32A32, GLuint>  );
    InsertFormatMapping(&map, GL_RGBA_INTEGER,       GL_INT,                            GL_RGBA32I,                WriteColor<R32G32B32A32S, GLint>  );
    InsertFormatMapping(&map, GL_RGBA_INTEGER,       GL_UNSIGNED_INT_2_10_10_10_REV,    GL_RGB10_A2UI,             WriteColor<R10G10B10A2, GLuint>   );

    InsertFormatMapping(&map, GL_RGB,                GL_UNSIGNED_BYTE,                  GL_RGB8,                   WriteColor<R8G8B8, GLfloat>       );
    InsertFormatMapping(&map, GL_RGB,                GL_BYTE,                           GL_RGB8_SNORM,             WriteColor<R8G8B8S, GLfloat>      );
    InsertFormatMapping(&map, GL_RGB,                GL_UNSIGNED_SHORT_5_6_5,           GL_RGB565,                 WriteColor<R5G6B5, GLfloat>       );
    InsertFormatMapping(&map, GL_RGB,                GL_UNSIGNED_INT_10F_11F_11F_REV,   GL_R11F_G11F_B10F,         WriteColor<R11G11B10F, GLfloat>   );
    InsertFormatMapping(&map, GL_RGB,                GL_UNSIGNED_INT_5_9_9_9_REV,       GL_RGB9_E5,                WriteColor<R9G9B9E5, GLfloat>     );
    InsertFormatMapping(&map, GL_RGB,                GL_FLOAT,                          GL_RGB32F,                 WriteColor<R32G32B32F, GLfloat>   );
    InsertFormatMapping(&map, GL_RGB,                GL_HALF_FLOAT,                     GL_RGB16F,                 WriteColor<R16G16B16F, GLfloat>   );
    InsertFormatMapping(&map, GL_RGB,                GL_HALF_FLOAT_OES,                 GL_RGB16F,                 WriteColor<R16G16B16F, GLfloat>   );

    InsertFormatMapping(&map, GL_RGB_INTEGER,        GL_UNSIGNED_BYTE,                  GL_RGB8UI,                 WriteColor<R8G8B8, GLuint>        );
    InsertFormatMapping(&map, GL_RGB_INTEGER,        GL_BYTE,                           GL_RGB8I,                  WriteColor<R8G8B8S, GLint>        );
    InsertFormatMapping(&map, GL_RGB_INTEGER,        GL_UNSIGNED_SHORT,                 GL_RGB16UI,                WriteColor<R16G16B16, GLuint>     );
    InsertFormatMapping(&map, GL_RGB_INTEGER,        GL_SHORT,                          GL_RGB16I,                 WriteColor<R16G16B16S, GLint>     );
    InsertFormatMapping(&map, GL_RGB_INTEGER,        GL_UNSIGNED_INT,                   GL_RGB32UI,                WriteColor<R32G32B32, GLuint>     );
    InsertFormatMapping(&map, GL_RGB_INTEGER,        GL_INT,                            GL_RGB32I,                 WriteColor<R32G32B32S, GLint>     );

    InsertFormatMapping(&map, GL_RG,                 GL_UNSIGNED_BYTE,                  GL_RG8,                    WriteColor<R8G8, GLfloat>         );
    InsertFormatMapping(&map, GL_RG,                 GL_BYTE,                           GL_RG8_SNORM,              WriteColor<R8G8S, GLfloat>        );
    InsertFormatMapping(&map, GL_RG,                 GL_FLOAT,                          GL_RG32F,                  WriteColor<R32G32F, GLfloat>      );
    InsertFormatMapping(&map, GL_RG,                 GL_HALF_FLOAT,                     GL_RG16F,                  WriteColor<R16G16F, GLfloat>      );
    InsertFormatMapping(&map, GL_RG,                 GL_HALF_FLOAT_OES,                 GL_RG16F,                  WriteColor<R16G16F, GLfloat>      );

    InsertFormatMapping(&map, GL_RG_INTEGER,         GL_UNSIGNED_BYTE,                  GL_RG8UI,                  WriteColor<R8G8, GLuint>          );
    InsertFormatMapping(&map, GL_RG_INTEGER,         GL_BYTE,                           GL_RG8I,                   WriteColor<R8G8S, GLint>          );
    InsertFormatMapping(&map, GL_RG_INTEGER,         GL_UNSIGNED_SHORT,                 GL_RG16UI,                 WriteColor<R16G16, GLuint>        );
    InsertFormatMapping(&map, GL_RG_INTEGER,         GL_SHORT,                          GL_RG16I,                  WriteColor<R16G16S, GLint>        );
    InsertFormatMapping(&map, GL_RG_INTEGER,         GL_UNSIGNED_INT,                   GL_RG32UI,                 WriteColor<R32G32, GLuint>        );
    InsertFormatMapping(&map, GL_RG_INTEGER,         GL_INT,                            GL_RG32I,                  WriteColor<R32G32S, GLint>        );

    InsertFormatMapping(&map, GL_RED,                GL_UNSIGNED_BYTE,                  GL_R8,                     WriteColor<R8, GLfloat>           );
    InsertFormatMapping(&map, GL_RED,                GL_BYTE,                           GL_R8_SNORM,               WriteColor<R8S, GLfloat>          );
    InsertFormatMapping(&map, GL_RED,                GL_FLOAT,                          GL_R32F,                   WriteColor<R32F, GLfloat>         );
    InsertFormatMapping(&map, GL_RED,                GL_HALF_FLOAT,                     GL_R16F,                   WriteColor<R16F, GLfloat>         );
    InsertFormatMapping(&map, GL_RED,                GL_HALF_FLOAT_OES,                 GL_R16F,                   WriteColor<R16F, GLfloat>         );

    InsertFormatMapping(&map, GL_RED_INTEGER,        GL_UNSIGNED_BYTE,                  GL_R8UI,                   WriteColor<R8, GLuint>            );
    InsertFormatMapping(&map, GL_RED_INTEGER,        GL_BYTE,                           GL_R8I,                    WriteColor<R8S, GLint>            );
    InsertFormatMapping(&map, GL_RED_INTEGER,        GL_UNSIGNED_SHORT,                 GL_R16UI,                  WriteColor<R16, GLuint>           );
    InsertFormatMapping(&map, GL_RED_INTEGER,        GL_SHORT,                          GL_R16I,                   WriteColor<R16S, GLint>           );
    InsertFormatMapping(&map, GL_RED_INTEGER,        GL_UNSIGNED_INT,                   GL_R32UI,                  WriteColor<R32, GLuint>           );
    InsertFormatMapping(&map, GL_RED_INTEGER,        GL_INT,                            GL_R32I,                   WriteColor<R32S, GLint>           );

    InsertFormatMapping(&map, GL_LUMINANCE_ALPHA,    GL_UNSIGNED_BYTE,                  GL_LUMINANCE8_ALPHA8_EXT,  WriteColor<L8A8, GLfloat>         );
    InsertFormatMapping(&map, GL_LUMINANCE,          GL_UNSIGNED_BYTE,                  GL_LUMINANCE8_EXT,         WriteColor<L8, GLfloat>           );
    InsertFormatMapping(&map, GL_ALPHA,              GL_UNSIGNED_BYTE,                  GL_ALPHA8_EXT,             WriteColor<A8, GLfloat>           );
    InsertFormatMapping(&map, GL_LUMINANCE_ALPHA,    GL_FLOAT,                          GL_LUMINANCE_ALPHA32F_EXT, WriteColor<L32A32F, GLfloat>      );
    InsertFormatMapping(&map, GL_LUMINANCE,          GL_FLOAT,                          GL_LUMINANCE32F_EXT,       WriteColor<L32F, GLfloat>         );
    InsertFormatMapping(&map, GL_ALPHA,              GL_FLOAT,                          GL_ALPHA32F_EXT,           WriteColor<A32F, GLfloat>         );
    InsertFormatMapping(&map, GL_LUMINANCE_ALPHA,    GL_HALF_FLOAT,                     GL_LUMINANCE_ALPHA16F_EXT, WriteColor<L16A16F, GLfloat>      );
    InsertFormatMapping(&map, GL_LUMINANCE_ALPHA,    GL_HALF_FLOAT_OES,                 GL_LUMINANCE_ALPHA16F_EXT, WriteColor<L16A16F, GLfloat>      );
    InsertFormatMapping(&map, GL_LUMINANCE,          GL_HALF_FLOAT,                     GL_LUMINANCE16F_EXT,       WriteColor<L16F, GLfloat>         );
    InsertFormatMapping(&map, GL_LUMINANCE,          GL_HALF_FLOAT_OES,                 GL_LUMINANCE16F_EXT,       WriteColor<L16F, GLfloat>         );
    InsertFormatMapping(&map, GL_ALPHA,              GL_HALF_FLOAT,                     GL_ALPHA16F_EXT,           WriteColor<A16F, GLfloat>         );
    InsertFormatMapping(&map, GL_ALPHA,              GL_HALF_FLOAT_OES,                 GL_ALPHA16F_EXT,           WriteColor<A16F, GLfloat>         );

    InsertFormatMapping(&map, GL_BGRA_EXT,           GL_UNSIGNED_BYTE,                  GL_BGRA8_EXT,              WriteColor<B8G8R8A8, GLfloat>     );
    InsertFormatMapping(&map, GL_BGRA_EXT,           GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT, GL_BGRA4_ANGLEX,           WriteColor<B4G4R4A4, GLfloat>     );
    InsertFormatMapping(&map, GL_BGRA_EXT,           GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT, GL_BGR5_A1_ANGLEX,         WriteColor<B5G5R5A1, GLfloat>     );

    InsertFormatMapping(&map, GL_SRGB_EXT,           GL_UNSIGNED_BYTE,                  GL_SRGB8,                  WriteColor<R8G8B8, GLfloat>       );
    InsertFormatMapping(&map, GL_SRGB_ALPHA_EXT,     GL_UNSIGNED_BYTE,                  GL_SRGB8_ALPHA8,           WriteColor<R8G8B8A8, GLfloat>     );

    InsertFormatMapping(&map, GL_COMPRESSED_RGB_S3TC_DXT1_EXT,    GL_UNSIGNED_BYTE,     GL_COMPRESSED_RGB_S3TC_DXT1_EXT,    NULL                     );
    InsertFormatMapping(&map, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,   GL_UNSIGNED_BYTE,     GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,   NULL                     );
    InsertFormatMapping(&map, GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE, GL_UNSIGNED_BYTE,     GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE, NULL                     );
    InsertFormatMapping(&map, GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE, GL_UNSIGNED_BYTE,     GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE, NULL                     );

    InsertFormatMapping(&map, GL_DEPTH_COMPONENT,    GL_UNSIGNED_SHORT,                 GL_DEPTH_COMPONENT16,      NULL                              );
    InsertFormatMapping(&map, GL_DEPTH_COMPONENT,    GL_UNSIGNED_INT,                   GL_DEPTH_COMPONENT32_OES,  NULL                              );
    InsertFormatMapping(&map, GL_DEPTH_COMPONENT,    GL_FLOAT,                          GL_DEPTH_COMPONENT32F,     NULL                              );

    InsertFormatMapping(&map, GL_STENCIL,            GL_UNSIGNED_BYTE,                  GL_STENCIL_INDEX8,         NULL                              );

    InsertFormatMapping(&map, GL_DEPTH_STENCIL,      GL_UNSIGNED_INT_24_8,              GL_DEPTH24_STENCIL8,       NULL                              );
    InsertFormatMapping(&map, GL_DEPTH_STENCIL,      GL_FLOAT_32_UNSIGNED_INT_24_8_REV, GL_DEPTH32F_STENCIL8,      NULL                              );

    return map;
}

Type::Type()
    : bytes(0),
      specialInterpretation(false)
{
}

// Map of sizes of input types
typedef std::pair<GLenum, Type> TypeInfoPair;
typedef std::map<GLenum, Type> TypeInfoMap;

static inline void InsertTypeInfo(TypeInfoMap *map, GLenum type, GLuint bytes, bool specialInterpretation)
{
    Type info;
    info.bytes = bytes;
    info.specialInterpretation = specialInterpretation;

    map->insert(std::make_pair(type, info));
}

bool operator<(const Type& a, const Type& b)
{
    return memcmp(&a, &b, sizeof(Type)) < 0;
}

static TypeInfoMap BuildTypeInfoMap()
{
    TypeInfoMap map;

    InsertTypeInfo(&map, GL_UNSIGNED_BYTE,                  1, false);
    InsertTypeInfo(&map, GL_BYTE,                           1, false);
    InsertTypeInfo(&map, GL_UNSIGNED_SHORT,                 2, false);
    InsertTypeInfo(&map, GL_SHORT,                          2, false);
    InsertTypeInfo(&map, GL_UNSIGNED_INT,                   4, false);
    InsertTypeInfo(&map, GL_INT,                            4, false);
    InsertTypeInfo(&map, GL_HALF_FLOAT,                     2, false);
    InsertTypeInfo(&map, GL_HALF_FLOAT_OES,                 2, false);
    InsertTypeInfo(&map, GL_FLOAT,                          4, false);
    InsertTypeInfo(&map, GL_UNSIGNED_SHORT_5_6_5,           2, true );
    InsertTypeInfo(&map, GL_UNSIGNED_SHORT_4_4_4_4,         2, true );
    InsertTypeInfo(&map, GL_UNSIGNED_SHORT_5_5_5_1,         2, true );
    InsertTypeInfo(&map, GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT, 2, true );
    InsertTypeInfo(&map, GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT, 2, true );
    InsertTypeInfo(&map, GL_UNSIGNED_INT_2_10_10_10_REV,    4, true );
    InsertTypeInfo(&map, GL_UNSIGNED_INT_24_8,              4, true );
    InsertTypeInfo(&map, GL_UNSIGNED_INT_10F_11F_11F_REV,   4, true );
    InsertTypeInfo(&map, GL_UNSIGNED_INT_5_9_9_9_REV,       4, true );
    InsertTypeInfo(&map, GL_UNSIGNED_INT_24_8_OES,          4, true );
    InsertTypeInfo(&map, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, 8, true );

    return map;
}

// Information about internal formats
static bool AlwaysSupported(GLuint, const Extensions &)
{
    return true;
}

static bool UnimplementedSupport(GLuint, const Extensions &)
{
    return false;
}

static bool NeverSupported(GLuint, const Extensions &)
{
    return false;
}

template <GLuint minCoreGLVersion>
static bool RequireES(GLuint clientVersion, const Extensions &)
{
    return clientVersion >= minCoreGLVersion;
}

// Pointer to a boolean memeber of the Extensions struct
typedef bool(Extensions::*ExtensionBool);

// Check support for a single extension
template <ExtensionBool bool1>
static bool RequireExt(GLuint, const Extensions & extensions)
{
    return extensions.*bool1;
}

// Check for a minimum client version or a single extension
template <GLuint minCoreGLVersion, ExtensionBool bool1>
static bool RequireESOrExt(GLuint clientVersion, const Extensions &extensions)
{
    return clientVersion >= minCoreGLVersion || extensions.*bool1;
}

// Check for a minimum client version or two extensions
template <GLuint minCoreGLVersion, ExtensionBool bool1, ExtensionBool bool2>
static bool RequireESOrExtAndExt(GLuint clientVersion, const Extensions &extensions)
{
    return clientVersion >= minCoreGLVersion || (extensions.*bool1 && extensions.*bool2);
}

// Check for a minimum client version or at least one of two extensions
template <GLuint minCoreGLVersion, ExtensionBool bool1, ExtensionBool bool2>
static bool RequireESOrExtOrExt(GLuint clientVersion, const Extensions &extensions)
{
    return clientVersion >= minCoreGLVersion || extensions.*bool1 || extensions.*bool2;
}

// Check support for two extensions
template <ExtensionBool bool1, ExtensionBool bool2>
static bool RequireExtAndExt(GLuint, const Extensions &extensions)
{
    return extensions.*bool1 && extensions.*bool2;
}

InternalFormat::InternalFormat()
    : redBits(0),
      greenBits(0),
      blueBits(0),
      luminanceBits(0),
      alphaBits(0),
      sharedBits(0),
      depthBits(0),
      stencilBits(0),
      pixelBytes(0),
      componentCount(0),
      compressedBlockWidth(0),
      compressedBlockHeight(0),
      format(GL_NONE),
      type(GL_NONE),
      componentType(GL_NONE),
      colorEncoding(GL_NONE),
      compressed(false),
      textureSupport(NeverSupported),
      renderSupport(NeverSupported),
      filterSupport(NeverSupported)
{
}

static InternalFormat UnsizedFormat(GLenum format, InternalFormat::SupportCheckFunction textureSupport,
                                    InternalFormat::SupportCheckFunction renderSupport,
                                    InternalFormat::SupportCheckFunction filterSupport)
{
    InternalFormat formatInfo;
    formatInfo.format = format;
    formatInfo.textureSupport = textureSupport;
    formatInfo.renderSupport = renderSupport;
    formatInfo.filterSupport = filterSupport;
    return formatInfo;
}

static InternalFormat RGBAFormat(GLuint red, GLuint green, GLuint blue, GLuint alpha, GLuint shared,
                                 GLenum format, GLenum type, GLenum componentType, bool srgb,
                                 InternalFormat::SupportCheckFunction textureSupport,
                                 InternalFormat::SupportCheckFunction renderSupport,
                                 InternalFormat::SupportCheckFunction filterSupport)
{
    InternalFormat formatInfo;
    formatInfo.redBits = red;
    formatInfo.greenBits = green;
    formatInfo.blueBits = blue;
    formatInfo.alphaBits = alpha;
    formatInfo.sharedBits = shared;
    formatInfo.pixelBytes = (red + green + blue + alpha + shared) / 8;
    formatInfo.componentCount = ((red > 0) ? 1 : 0) + ((green > 0) ? 1 : 0) + ((blue > 0) ? 1 : 0) + ((alpha > 0) ? 1 : 0);
    formatInfo.format = format;
    formatInfo.type = type;
    formatInfo.componentType = componentType;
    formatInfo.colorEncoding = (srgb ? GL_SRGB : GL_LINEAR);
    formatInfo.textureSupport = textureSupport;
    formatInfo.renderSupport = renderSupport;
    formatInfo.filterSupport = filterSupport;
    return formatInfo;
}

static InternalFormat LUMAFormat(GLuint luminance, GLuint alpha, GLenum format, GLenum type, GLenum componentType,
                                 InternalFormat::SupportCheckFunction textureSupport,
                                 InternalFormat::SupportCheckFunction renderSupport,
                                 InternalFormat::SupportCheckFunction filterSupport)
{
    InternalFormat formatInfo;
    formatInfo.luminanceBits = luminance;
    formatInfo.alphaBits = alpha;
    formatInfo.pixelBytes = (luminance + alpha) / 8;
    formatInfo.componentCount = ((luminance > 0) ? 1 : 0) + ((alpha > 0) ? 1 : 0);
    formatInfo.format = format;
    formatInfo.type = type;
    formatInfo.componentType = componentType;
    formatInfo.colorEncoding = GL_LINEAR;
    formatInfo.textureSupport = textureSupport;
    formatInfo.renderSupport = renderSupport;
    formatInfo.filterSupport = filterSupport;
    return formatInfo;
}

static InternalFormat DepthStencilFormat(GLuint depthBits, GLuint stencilBits, GLuint unusedBits, GLenum format,
                                         GLenum type, GLenum componentType, InternalFormat::SupportCheckFunction textureSupport,
                                         InternalFormat::SupportCheckFunction renderSupport,
                                         InternalFormat::SupportCheckFunction filterSupport)
{
    InternalFormat formatInfo;
    formatInfo.depthBits = depthBits;
    formatInfo.stencilBits = stencilBits;
    formatInfo.pixelBytes = (depthBits + stencilBits + unusedBits) / 8;
    formatInfo.componentCount = ((depthBits > 0) ? 1 : 0) + ((stencilBits > 0) ? 1 : 0);
    formatInfo.format = format;
    formatInfo.type = type;
    formatInfo.componentType = componentType;
    formatInfo.colorEncoding = GL_LINEAR;
    formatInfo.textureSupport = textureSupport;
    formatInfo.renderSupport = renderSupport;
    formatInfo.filterSupport = filterSupport;
    return formatInfo;
}

static InternalFormat CompressedFormat(GLuint compressedBlockWidth, GLuint compressedBlockHeight, GLuint compressedBlockSize,
                                       GLuint componentCount, GLenum format, GLenum type, bool srgb,
                                       InternalFormat::SupportCheckFunction textureSupport,
                                       InternalFormat::SupportCheckFunction renderSupport,
                                       InternalFormat::SupportCheckFunction filterSupport)
{
    InternalFormat formatInfo;
    formatInfo.compressedBlockWidth = compressedBlockWidth;
    formatInfo.compressedBlockHeight = compressedBlockHeight;
    formatInfo.pixelBytes = compressedBlockSize / 8;
    formatInfo.componentCount = componentCount;
    formatInfo.format = format;
    formatInfo.type = type;
    formatInfo.componentType = GL_UNSIGNED_NORMALIZED;
    formatInfo.colorEncoding = (srgb ? GL_SRGB : GL_LINEAR);
    formatInfo.compressed = true;
    formatInfo.textureSupport = textureSupport;
    formatInfo.renderSupport = renderSupport;
    formatInfo.filterSupport = filterSupport;
    return formatInfo;
}

typedef std::pair<GLenum, InternalFormat> InternalFormatInfoPair;
typedef std::map<GLenum, InternalFormat> InternalFormatInfoMap;

static InternalFormatInfoMap BuildInternalFormatInfoMap()
{
    InternalFormatInfoMap map;

    // From ES 3.0.1 spec, table 3.12
    map.insert(InternalFormatInfoPair(GL_NONE,              InternalFormat()));

    //                               | Internal format     |          | R | G | B | A |S | Format         | Type                           | Component type        | SRGB | Texture supported                        | Renderable                               | Filterable    |
    map.insert(InternalFormatInfoPair(GL_R8,                RGBAFormat( 8,  0,  0,  0, 0, GL_RED,          GL_UNSIGNED_BYTE,                GL_UNSIGNED_NORMALIZED, false, RequireESOrExt<3, &Extensions::textureRG>, RequireESOrExt<3, &Extensions::textureRG>, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_R8_SNORM,          RGBAFormat( 8,  0,  0,  0, 0, GL_RED,          GL_BYTE,                         GL_SIGNED_NORMALIZED,   false, RequireES<3>,                              NeverSupported,                            AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RG8,               RGBAFormat( 8,  8,  0,  0, 0, GL_RG,           GL_UNSIGNED_BYTE,                GL_UNSIGNED_NORMALIZED, false, RequireESOrExt<3, &Extensions::textureRG>, RequireESOrExt<3, &Extensions::textureRG>, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RG8_SNORM,         RGBAFormat( 8,  8,  0,  0, 0, GL_RG,           GL_BYTE,                         GL_SIGNED_NORMALIZED,   false, RequireES<3>,                              NeverSupported,                            AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RGB8,              RGBAFormat( 8,  8,  8,  0, 0, GL_RGB,          GL_UNSIGNED_BYTE,                GL_UNSIGNED_NORMALIZED, false, RequireESOrExt<3, &Extensions::rgb8rgba8>, RequireESOrExt<3, &Extensions::rgb8rgba8>, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RGB8_SNORM,        RGBAFormat( 8,  8,  8,  0, 0, GL_RGB,          GL_BYTE,                         GL_SIGNED_NORMALIZED,   false, RequireES<3>,                              NeverSupported,                            AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RGB565,            RGBAFormat( 5,  6,  5,  0, 0, GL_RGB,          GL_UNSIGNED_SHORT_5_6_5,         GL_UNSIGNED_NORMALIZED, false, RequireES<2>,                              RequireES<2>,                              AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RGBA4,             RGBAFormat( 4,  4,  4,  4, 0, GL_RGBA,         GL_UNSIGNED_SHORT_4_4_4_4,       GL_UNSIGNED_NORMALIZED, false, RequireES<2>,                              RequireES<2>,                              AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RGB5_A1,           RGBAFormat( 5,  5,  5,  1, 0, GL_RGBA,         GL_UNSIGNED_SHORT_5_5_5_1,       GL_UNSIGNED_NORMALIZED, false, RequireES<2>,                              RequireES<2>,                              AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RGBA8,             RGBAFormat( 8,  8,  8,  8, 0, GL_RGBA,         GL_UNSIGNED_BYTE,                GL_UNSIGNED_NORMALIZED, false, RequireESOrExt<3, &Extensions::rgb8rgba8>, RequireESOrExt<3, &Extensions::rgb8rgba8>, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RGBA8_SNORM,       RGBAFormat( 8,  8,  8,  8, 0, GL_RGBA,         GL_BYTE,                         GL_SIGNED_NORMALIZED,   false, RequireES<3>,                              NeverSupported,                            AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RGB10_A2,          RGBAFormat(10, 10, 10,  2, 0, GL_RGBA,         GL_UNSIGNED_INT_2_10_10_10_REV,  GL_UNSIGNED_NORMALIZED, false, RequireES<3>,                              RequireES<3>,                              AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RGB10_A2UI,        RGBAFormat(10, 10, 10,  2, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT_2_10_10_10_REV,  GL_UNSIGNED_INT,        false, RequireES<3>,                              NeverSupported,                            NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_SRGB8,             RGBAFormat( 8,  8,  8,  0, 0, GL_RGB,          GL_UNSIGNED_BYTE,                GL_UNSIGNED_NORMALIZED, true,  RequireESOrExt<3, &Extensions::sRGB>,      NeverSupported,                            AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_SRGB8_ALPHA8,      RGBAFormat( 8,  8,  8,  8, 0, GL_RGBA,         GL_UNSIGNED_BYTE,                GL_UNSIGNED_NORMALIZED, true,  RequireESOrExt<3, &Extensions::sRGB>,      RequireESOrExt<3, &Extensions::sRGB>,      AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_R11F_G11F_B10F,    RGBAFormat(11, 11, 10,  0, 0, GL_RGB,          GL_UNSIGNED_INT_10F_11F_11F_REV, GL_FLOAT,               false, RequireES<3>,                              RequireExt<&Extensions::colorBufferFloat>, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RGB9_E5,           RGBAFormat( 9,  9,  9,  0, 5, GL_RGB,          GL_UNSIGNED_INT_5_9_9_9_REV,     GL_FLOAT,               false, RequireES<3>,                              NeverSupported,                            AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_R8I,               RGBAFormat( 8,  0,  0,  0, 0, GL_RED_INTEGER,  GL_BYTE,                         GL_INT,                 false, RequireES<3>,                              RequireES<3>,                              NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_R8UI,              RGBAFormat( 8,  0,  0,  0, 0, GL_RED_INTEGER,  GL_UNSIGNED_BYTE,                GL_UNSIGNED_INT,        false, RequireES<3>,                              RequireES<3>,                              NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_R16I,              RGBAFormat(16,  0,  0,  0, 0, GL_RED_INTEGER,  GL_SHORT,                        GL_INT,                 false, RequireES<3>,                              RequireES<3>,                              NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_R16UI,             RGBAFormat(16,  0,  0,  0, 0, GL_RED_INTEGER,  GL_UNSIGNED_SHORT,               GL_UNSIGNED_INT,        false, RequireES<3>,                              RequireES<3>,                              NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_R32I,              RGBAFormat(32,  0,  0,  0, 0, GL_RED_INTEGER,  GL_INT,                          GL_INT,                 false, RequireES<3>,                              RequireES<3>,                              NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_R32UI,             RGBAFormat(32,  0,  0,  0, 0, GL_RED_INTEGER,  GL_UNSIGNED_INT,                 GL_UNSIGNED_INT,        false, RequireES<3>,                              RequireES<3>,                              NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_RG8I,              RGBAFormat( 8,  8,  0,  0, 0, GL_RG_INTEGER,   GL_BYTE,                         GL_INT,                 false, RequireES<3>,                              RequireES<3>,                              NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_RG8UI,             RGBAFormat( 8,  8,  0,  0, 0, GL_RG_INTEGER,   GL_UNSIGNED_BYTE,                GL_UNSIGNED_INT,        false, RequireES<3>,                              RequireES<3>,                              NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_RG16I,             RGBAFormat(16, 16,  0,  0, 0, GL_RG_INTEGER,   GL_SHORT,                        GL_INT,                 false, RequireES<3>,                              RequireES<3>,                              NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_RG16UI,            RGBAFormat(16, 16,  0,  0, 0, GL_RG_INTEGER,   GL_UNSIGNED_SHORT,               GL_UNSIGNED_INT,        false, RequireES<3>,                              RequireES<3>,                              NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_RG32I,             RGBAFormat(32, 32,  0,  0, 0, GL_RG_INTEGER,   GL_INT,                          GL_INT,                 false, RequireES<3>,                              RequireES<3>,                              NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_RG32UI,            RGBAFormat(32, 32,  0,  0, 0, GL_RG_INTEGER,   GL_UNSIGNED_INT,                 GL_UNSIGNED_INT,        false, RequireES<3>,                              RequireES<3>,                              NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_RGB8I,             RGBAFormat( 8,  8,  8,  0, 0, GL_RGB_INTEGER,  GL_BYTE,                         GL_INT,                 false, RequireES<3>,                              NeverSupported,                            NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_RGB8UI,            RGBAFormat( 8,  8,  8,  0, 0, GL_RGB_INTEGER,  GL_UNSIGNED_BYTE,                GL_UNSIGNED_INT,        false, RequireES<3>,                              NeverSupported,                            NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_RGB16I,            RGBAFormat(16, 16, 16,  0, 0, GL_RGB_INTEGER,  GL_SHORT,                        GL_INT,                 false, RequireES<3>,                              NeverSupported,                            NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_RGB16UI,           RGBAFormat(16, 16, 16,  0, 0, GL_RGB_INTEGER,  GL_UNSIGNED_SHORT,               GL_UNSIGNED_INT,        false, RequireES<3>,                              NeverSupported,                            NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_RGB32I,            RGBAFormat(32, 32, 32,  0, 0, GL_RGB_INTEGER,  GL_INT,                          GL_INT,                 false, RequireES<3>,                              NeverSupported,                            NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_RGB32UI,           RGBAFormat(32, 32, 32,  0, 0, GL_RGB_INTEGER,  GL_UNSIGNED_INT,                 GL_UNSIGNED_INT,        false, RequireES<3>,                              NeverSupported,                            NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_RGBA8I,            RGBAFormat( 8,  8,  8,  8, 0, GL_RGBA_INTEGER, GL_BYTE,                         GL_INT,                 false, RequireES<3>,                              RequireES<3>,                              NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_RGBA8UI,           RGBAFormat( 8,  8,  8,  8, 0, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE,                GL_UNSIGNED_INT,        false, RequireES<3>,                              RequireES<3>,                              NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_RGBA16I,           RGBAFormat(16, 16, 16, 16, 0, GL_RGBA_INTEGER, GL_SHORT,                        GL_INT,                 false, RequireES<3>,                              RequireES<3>,                              NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_RGBA16UI,          RGBAFormat(16, 16, 16, 16, 0, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT,               GL_UNSIGNED_INT,        false, RequireES<3>,                              RequireES<3>,                              NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_RGBA32I,           RGBAFormat(32, 32, 32, 32, 0, GL_RGBA_INTEGER, GL_INT,                          GL_INT,                 false, RequireES<3>,                              RequireES<3>,                              NeverSupported)));
    map.insert(InternalFormatInfoPair(GL_RGBA32UI,          RGBAFormat(32, 32, 32, 32, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT,                 GL_UNSIGNED_INT,        false, RequireES<3>,                              RequireES<3>,                              NeverSupported)));

    map.insert(InternalFormatInfoPair(GL_BGRA8_EXT,         RGBAFormat( 8,  8,  8,  8, 0, GL_BGRA_EXT,     GL_UNSIGNED_BYTE,                  GL_UNSIGNED_NORMALIZED, false, RequireExt<&Extensions::textureFormatBGRA8888>, RequireExt<&Extensions::textureFormatBGRA8888>, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_BGRA4_ANGLEX,      RGBAFormat( 4,  4,  4,  4, 0, GL_BGRA_EXT,     GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT, GL_UNSIGNED_NORMALIZED, false, RequireExt<&Extensions::textureFormatBGRA8888>, RequireExt<&Extensions::textureFormatBGRA8888>, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_BGR5_A1_ANGLEX,    RGBAFormat( 5,  5,  5,  1, 0, GL_BGRA_EXT,     GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT, GL_UNSIGNED_NORMALIZED, false, RequireExt<&Extensions::textureFormatBGRA8888>, RequireExt<&Extensions::textureFormatBGRA8888>, AlwaysSupported)));

    // Floating point renderability and filtering is provided by OES_texture_float and OES_texture_half_float
    //                               | Internal format     |          | D |S | Format             | Type                                   | Comp   | SRGB |  Texture supported                                                             | Renderable                                                                    | Filterable                                    |
    //                               |                     |          |   |  |                    |                                        | type   |      |                                                                                |                                                                               |                                               |
    map.insert(InternalFormatInfoPair(GL_R16F,              RGBAFormat(16,  0,  0,  0, 0, GL_RED,          GL_HALF_FLOAT,                   GL_FLOAT, false, RequireESOrExtAndExt<3, &Extensions::textureHalfFloat, &Extensions::textureRG>, RequireESOrExtAndExt<3, &Extensions::textureHalfFloat, &Extensions::textureRG>, RequireExt<&Extensions::textureHalfFloatLinear>)));
    map.insert(InternalFormatInfoPair(GL_RG16F,             RGBAFormat(16, 16,  0,  0, 0, GL_RG,           GL_HALF_FLOAT,                   GL_FLOAT, false, RequireESOrExtAndExt<3, &Extensions::textureHalfFloat, &Extensions::textureRG>, RequireESOrExtAndExt<3, &Extensions::textureHalfFloat, &Extensions::textureRG>, RequireExt<&Extensions::textureHalfFloatLinear>)));
    map.insert(InternalFormatInfoPair(GL_RGB16F,            RGBAFormat(16, 16, 16,  0, 0, GL_RGB,          GL_HALF_FLOAT,                   GL_FLOAT, false, RequireESOrExt<3, &Extensions::textureHalfFloat>,                               RequireESOrExt<3, &Extensions::textureHalfFloat>,                               RequireExt<&Extensions::textureHalfFloatLinear>)));
    map.insert(InternalFormatInfoPair(GL_RGBA16F,           RGBAFormat(16, 16, 16, 16, 0, GL_RGBA,         GL_HALF_FLOAT,                   GL_FLOAT, false, RequireESOrExt<3, &Extensions::textureHalfFloat>,                               RequireESOrExt<3, &Extensions::textureHalfFloat>,                               RequireExt<&Extensions::textureHalfFloatLinear>)));
    map.insert(InternalFormatInfoPair(GL_R32F,              RGBAFormat(32,  0,  0,  0, 0, GL_RED,          GL_FLOAT,                        GL_FLOAT, false, RequireESOrExtAndExt<3, &Extensions::textureFloat, &Extensions::textureRG>,     RequireESOrExtAndExt<3, &Extensions::textureFloat, &Extensions::textureRG>,     RequireExt<&Extensions::textureFloatLinear>    )));
    map.insert(InternalFormatInfoPair(GL_RG32F,             RGBAFormat(32, 32,  0,  0, 0, GL_RG,           GL_FLOAT,                        GL_FLOAT, false, RequireESOrExtAndExt<3, &Extensions::textureFloat, &Extensions::textureRG>,     RequireESOrExtAndExt<3, &Extensions::textureFloat, &Extensions::textureRG>,     RequireExt<&Extensions::textureFloatLinear>    )));
    map.insert(InternalFormatInfoPair(GL_RGB32F,            RGBAFormat(32, 32, 32,  0, 0, GL_RGB,          GL_FLOAT,                        GL_FLOAT, false, RequireESOrExt<3, &Extensions::textureFloat>,                                   RequireESOrExt<3, &Extensions::textureFloat>,                                   RequireExt<&Extensions::textureFloatLinear>    )));
    map.insert(InternalFormatInfoPair(GL_RGBA32F,           RGBAFormat(32, 32, 32, 32, 0, GL_RGBA,         GL_FLOAT,                        GL_FLOAT, false, RequireESOrExt<3, &Extensions::textureFloat>,                                   RequireESOrExt<3, &Extensions::textureFloat>,                                   RequireExt<&Extensions::textureFloatLinear>    )));

    // Depth stencil formats
    //                               | Internal format         |                  | D |S | X | Format            | Type                             | Component type        | Supported                                    | Renderable                                                                         | Filterable                                  |
    map.insert(InternalFormatInfoPair(GL_DEPTH_COMPONENT16,     DepthStencilFormat(16, 0,  0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT,                 GL_UNSIGNED_NORMALIZED, RequireES<2>,                                  RequireES<2>,                                                                        RequireESOrExt<3, &Extensions::depthTextures>)));
    map.insert(InternalFormatInfoPair(GL_DEPTH_COMPONENT24,     DepthStencilFormat(24, 0,  0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,                   GL_UNSIGNED_NORMALIZED, RequireES<3>,                                  RequireES<3>,                                                                        RequireESOrExt<3, &Extensions::depthTextures>)));
    map.insert(InternalFormatInfoPair(GL_DEPTH_COMPONENT32F,    DepthStencilFormat(32, 0,  0, GL_DEPTH_COMPONENT, GL_FLOAT,                          GL_FLOAT,               RequireES<3>,                                  RequireES<3>,                                                                        RequireESOrExt<3, &Extensions::depthTextures>)));
    map.insert(InternalFormatInfoPair(GL_DEPTH_COMPONENT32_OES, DepthStencilFormat(32, 0,  0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,                   GL_UNSIGNED_NORMALIZED, RequireExt<&Extensions::depthTextures>,        RequireExt<&Extensions::depthTextures>,                                              AlwaysSupported                              )));
    map.insert(InternalFormatInfoPair(GL_DEPTH24_STENCIL8,      DepthStencilFormat(24, 8,  0, GL_DEPTH_STENCIL,   GL_UNSIGNED_INT_24_8,              GL_UNSIGNED_NORMALIZED, RequireESOrExt<3, &Extensions::depthTextures>, RequireESOrExtOrExt<3, &Extensions::depthTextures, &Extensions::packedDepthStencil>, AlwaysSupported                              )));
    map.insert(InternalFormatInfoPair(GL_DEPTH32F_STENCIL8,     DepthStencilFormat(32, 8, 24, GL_DEPTH_STENCIL,   GL_FLOAT_32_UNSIGNED_INT_24_8_REV, GL_FLOAT,               RequireES<3>,                                  RequireES<3>,                                                                        AlwaysSupported                              )));
    map.insert(InternalFormatInfoPair(GL_STENCIL_INDEX8,        DepthStencilFormat( 0, 8,  0, GL_DEPTH_STENCIL,   GL_UNSIGNED_BYTE,                  GL_UNSIGNED_INT,        RequireES<2>,                                  RequireES<2>,                                                                        NeverSupported                               )));

    // Luminance alpha formats
    //                               | Internal format          |          | L | A | Format            | Type            | Component type        | Supported                                                                    | Renderable    | Filterable    |
    map.insert(InternalFormatInfoPair(GL_ALPHA8_EXT,             LUMAFormat( 0,  8, GL_ALPHA,           GL_UNSIGNED_BYTE, GL_UNSIGNED_NORMALIZED, RequireExt<&Extensions::textureStorage>,                                      NeverSupported, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE8_EXT,         LUMAFormat( 8,  0, GL_LUMINANCE,       GL_UNSIGNED_BYTE, GL_UNSIGNED_NORMALIZED, RequireExt<&Extensions::textureStorage>,                                      NeverSupported, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_ALPHA32F_EXT,           LUMAFormat( 0, 32, GL_ALPHA,           GL_FLOAT,         GL_FLOAT,               RequireExtAndExt<&Extensions::textureStorage, &Extensions::textureFloat>,     NeverSupported, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE32F_EXT,       LUMAFormat(32,  0, GL_LUMINANCE,       GL_FLOAT,         GL_FLOAT,               RequireExtAndExt<&Extensions::textureStorage, &Extensions::textureFloat>,     NeverSupported, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_ALPHA16F_EXT,           LUMAFormat( 0, 16, GL_ALPHA,           GL_HALF_FLOAT,    GL_FLOAT,               RequireExtAndExt<&Extensions::textureStorage, &Extensions::textureHalfFloat>, NeverSupported, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE16F_EXT,       LUMAFormat(16,  0, GL_LUMINANCE,       GL_HALF_FLOAT,    GL_FLOAT,               RequireExtAndExt<&Extensions::textureStorage, &Extensions::textureHalfFloat>, NeverSupported, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE8_ALPHA8_EXT,  LUMAFormat( 8,  8, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, GL_UNSIGNED_NORMALIZED, RequireExt<&Extensions::textureStorage>,                                      NeverSupported, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE_ALPHA32F_EXT, LUMAFormat(32, 32, GL_LUMINANCE_ALPHA, GL_FLOAT,         GL_FLOAT,               RequireExtAndExt<&Extensions::textureStorage, &Extensions::textureFloat>,     NeverSupported, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE_ALPHA16F_EXT, LUMAFormat(16, 16, GL_LUMINANCE_ALPHA, GL_HALF_FLOAT,    GL_FLOAT,               RequireExtAndExt<&Extensions::textureStorage, &Extensions::textureHalfFloat>, NeverSupported, AlwaysSupported)));

    // Unsized formats
    //                               | Internal format   |             | Format            | Supported                                         | Renderable                                        | Filterable    |
    map.insert(InternalFormatInfoPair(GL_ALPHA,           UnsizedFormat(GL_ALPHA,           RequireES<2>,                                       NeverSupported,                                     AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE,       UnsizedFormat(GL_LUMINANCE,       RequireES<2>,                                       NeverSupported,                                     AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE_ALPHA, UnsizedFormat(GL_LUMINANCE_ALPHA, RequireES<2>,                                       NeverSupported,                                     AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RED,             UnsizedFormat(GL_RED,             RequireESOrExt<3, &Extensions::textureRG>,          NeverSupported,                                     AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RG,              UnsizedFormat(GL_RG,              RequireESOrExt<3, &Extensions::textureRG>,          NeverSupported,                                     AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RGB,             UnsizedFormat(GL_RGB,             RequireES<2>,                                       RequireES<2>,                                       AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RGBA,            UnsizedFormat(GL_RGBA,            RequireES<2>,                                       RequireES<2>,                                       AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RED_INTEGER,     UnsizedFormat(GL_RED_INTEGER,     RequireES<3>,                                       NeverSupported,                                     NeverSupported )));
    map.insert(InternalFormatInfoPair(GL_RG_INTEGER,      UnsizedFormat(GL_RG_INTEGER,      RequireES<3>,                                       NeverSupported,                                     NeverSupported )));
    map.insert(InternalFormatInfoPair(GL_RGB_INTEGER,     UnsizedFormat(GL_RGB_INTEGER,     RequireES<3>,                                       NeverSupported,                                     NeverSupported )));
    map.insert(InternalFormatInfoPair(GL_RGBA_INTEGER,    UnsizedFormat(GL_RGBA_INTEGER,    RequireES<3>,                                       NeverSupported,                                     NeverSupported )));
    map.insert(InternalFormatInfoPair(GL_BGRA_EXT,        UnsizedFormat(GL_BGRA_EXT,        RequireExt<&Extensions::textureFormatBGRA8888>,     RequireExt<&Extensions::textureFormatBGRA8888>,     AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_DEPTH_COMPONENT, UnsizedFormat(GL_DEPTH_COMPONENT, RequireES<2>,                                       RequireES<2>,                                       AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_DEPTH_STENCIL,   UnsizedFormat(GL_DEPTH_STENCIL,   RequireESOrExt<3, &Extensions::packedDepthStencil>, RequireESOrExt<3, &Extensions::packedDepthStencil>, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_SRGB_EXT,        UnsizedFormat(GL_RGB,             RequireESOrExt<3, &Extensions::sRGB>,               NeverSupported,                                     AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_SRGB_ALPHA_EXT,  UnsizedFormat(GL_RGBA,            RequireESOrExt<3, &Extensions::sRGB>,               RequireESOrExt<3, &Extensions::sRGB>,               AlwaysSupported)));

    // Compressed formats, From ES 3.0.1 spec, table 3.16
    //                               | Internal format                             |                |W |H | BS |CC| Format                                      | Type            | SRGB | Supported          | Renderable           | Filterable         |
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_R11_EAC,                        CompressedFormat(4, 4,  64, 1, GL_COMPRESSED_R11_EAC,                        GL_UNSIGNED_BYTE, false, UnimplementedSupport, UnimplementedSupport, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_SIGNED_R11_EAC,                 CompressedFormat(4, 4,  64, 1, GL_COMPRESSED_SIGNED_R11_EAC,                 GL_UNSIGNED_BYTE, false, UnimplementedSupport, UnimplementedSupport, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RG11_EAC,                       CompressedFormat(4, 4, 128, 2, GL_COMPRESSED_RG11_EAC,                       GL_UNSIGNED_BYTE, false, UnimplementedSupport, UnimplementedSupport, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_SIGNED_RG11_EAC,                CompressedFormat(4, 4, 128, 2, GL_COMPRESSED_SIGNED_RG11_EAC,                GL_UNSIGNED_BYTE, false, UnimplementedSupport, UnimplementedSupport, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGB8_ETC2,                      CompressedFormat(4, 4,  64, 3, GL_COMPRESSED_RGB8_ETC2,                      GL_UNSIGNED_BYTE, false, UnimplementedSupport, UnimplementedSupport, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_SRGB8_ETC2,                     CompressedFormat(4, 4,  64, 3, GL_COMPRESSED_SRGB8_ETC2,                     GL_UNSIGNED_BYTE, true,  UnimplementedSupport, UnimplementedSupport, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,  CompressedFormat(4, 4,  64, 3, GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,  GL_UNSIGNED_BYTE, false, UnimplementedSupport, UnimplementedSupport, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, CompressedFormat(4, 4,  64, 3, GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_UNSIGNED_BYTE, true,  UnimplementedSupport, UnimplementedSupport, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGBA8_ETC2_EAC,                 CompressedFormat(4, 4, 128, 4, GL_COMPRESSED_RGBA8_ETC2_EAC,                 GL_UNSIGNED_BYTE, false, UnimplementedSupport, UnimplementedSupport, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,          CompressedFormat(4, 4, 128, 4, GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,          GL_UNSIGNED_BYTE, true,  UnimplementedSupport, UnimplementedSupport, UnimplementedSupport)));

    // From GL_EXT_texture_compression_dxt1
    //                               | Internal format                   |                |W |H | BS |CC| Format                            | Type            | SRGB | Supported                                      | Renderable    | Filterable    |
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGB_S3TC_DXT1_EXT,    CompressedFormat(4, 4,  64, 3, GL_COMPRESSED_RGB_S3TC_DXT1_EXT,    GL_UNSIGNED_BYTE, false, RequireExt<&Extensions::textureCompressionDXT1>, NeverSupported, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,   CompressedFormat(4, 4,  64, 4, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,   GL_UNSIGNED_BYTE, false, RequireExt<&Extensions::textureCompressionDXT1>, NeverSupported, AlwaysSupported)));

    // From GL_ANGLE_texture_compression_dxt3
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE, CompressedFormat(4, 4, 128, 4, GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE, GL_UNSIGNED_BYTE, false, RequireExt<&Extensions::textureCompressionDXT5>, NeverSupported, AlwaysSupported)));

    // From GL_ANGLE_texture_compression_dxt5
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE, CompressedFormat(4, 4, 128, 4, GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE, GL_UNSIGNED_BYTE, false, RequireExt<&Extensions::textureCompressionDXT5>, NeverSupported, AlwaysSupported)));

    return map;
}

static const InternalFormatInfoMap &GetInternalFormatMap()
{
    static const InternalFormatInfoMap formatMap = BuildInternalFormatInfoMap();
    return formatMap;
}

static FormatSet BuildAllSizedInternalFormatSet()
{
    FormatSet result;

    const InternalFormatInfoMap &formats = GetInternalFormatMap();
    for (InternalFormatInfoMap::const_iterator i = formats.begin(); i != formats.end(); i++)
    {
        if (i->second.pixelBytes > 0)
        {
            result.insert(i->first);
        }
    }

    return result;
}

const FormatType &GetFormatTypeInfo(GLenum format, GLenum type)
{
    static const FormatMap formatMap = BuildFormatMap();
    FormatMap::const_iterator iter = formatMap.find(FormatTypePair(format, type));
    if (iter != formatMap.end())
    {
        return iter->second;
    }
    else
    {
        static const FormatType defaultInfo;
        return defaultInfo;
    }
}

const Type &GetTypeInfo(GLenum type)
{
    static const TypeInfoMap infoMap = BuildTypeInfoMap();
    TypeInfoMap::const_iterator iter = infoMap.find(type);
    if (iter != infoMap.end())
    {
        return iter->second;
    }
    else
    {
        static const Type defaultInfo;
        return defaultInfo;
    }
}

const InternalFormat &GetInternalFormatInfo(GLenum internalFormat)
{
    const InternalFormatInfoMap &formatMap = GetInternalFormatMap();
    InternalFormatInfoMap::const_iterator iter = formatMap.find(internalFormat);
    if (iter != formatMap.end())
    {
        return iter->second;
    }
    else
    {
        static const InternalFormat defaultInternalFormat;
        return defaultInternalFormat;
    }
}

GLuint InternalFormat::computeRowPitch(GLenum type, GLsizei width, GLint alignment) const
{
    ASSERT(alignment > 0 && isPow2(alignment));
    return rx::roundUp(computeBlockSize(type, width, 1), static_cast<GLuint>(alignment));
}

GLuint InternalFormat::computeDepthPitch(GLenum type, GLsizei width, GLsizei height, GLint alignment) const
{
    return computeRowPitch(type, width, alignment) * height;
}

GLuint InternalFormat::computeBlockSize(GLenum type, GLsizei width, GLsizei height) const
{
    if (compressed)
    {
        GLsizei numBlocksWide = (width + compressedBlockWidth - 1) / compressedBlockWidth;
        GLsizei numBlocksHight = (height + compressedBlockHeight - 1) / compressedBlockHeight;
        return (pixelBytes * numBlocksWide * numBlocksHight);
    }
    else
    {
        const Type &typeInfo = GetTypeInfo(type);
        if (typeInfo.specialInterpretation)
        {
            return typeInfo.bytes * width * height;
        }
        else
        {
            return componentCount * typeInfo.bytes * width * height;
        }
    }
}

GLenum GetSizedInternalFormat(GLenum internalFormat, GLenum type)
{
    const InternalFormat& formatInfo = GetInternalFormatInfo(internalFormat);
    return (formatInfo.pixelBytes > 0) ? internalFormat : GetFormatTypeInfo(internalFormat, type).internalFormat;
}

const FormatSet &GetAllSizedInternalFormats()
{
    static FormatSet formatSet = BuildAllSizedInternalFormatSet();
    return formatSet;
}

}
