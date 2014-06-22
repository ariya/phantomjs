#include "precompiled.h"
//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// utilities.cpp: Conversion functions and other utility routines.

#include "libGLESv2/utilities.h"
#include "libGLESv2/mathutil.h"
#if defined(ANGLE_OS_WINRT)
#  include <locale>
#  include <codecvt>
#  include <wrl.h>
#  include <windows.storage.h>
   using namespace Microsoft::WRL;
   using namespace ABI::Windows::Storage;
#endif

namespace gl
{

int UniformComponentCount(GLenum type)
{
    switch (type)
    {
      case GL_BOOL:
      case GL_FLOAT:
      case GL_INT:
      case GL_SAMPLER_2D:
      case GL_SAMPLER_CUBE:
          return 1;
      case GL_BOOL_VEC2:
      case GL_FLOAT_VEC2:
      case GL_INT_VEC2:
          return 2;
      case GL_INT_VEC3:
      case GL_FLOAT_VEC3:
      case GL_BOOL_VEC3:
          return 3;
      case GL_BOOL_VEC4:
      case GL_FLOAT_VEC4:
      case GL_INT_VEC4:
      case GL_FLOAT_MAT2:
          return 4;
      case GL_FLOAT_MAT3:
          return 9;
      case GL_FLOAT_MAT4:
          return 16;
      default:
          UNREACHABLE();
    }

    return 0;
}

GLenum UniformComponentType(GLenum type)
{
    switch(type)
    {
      case GL_BOOL:
      case GL_BOOL_VEC2:
      case GL_BOOL_VEC3:
      case GL_BOOL_VEC4:
          return GL_BOOL;
      case GL_FLOAT:
      case GL_FLOAT_VEC2:
      case GL_FLOAT_VEC3:
      case GL_FLOAT_VEC4:
      case GL_FLOAT_MAT2:
      case GL_FLOAT_MAT3:
      case GL_FLOAT_MAT4:
          return GL_FLOAT;
      case GL_INT:
      case GL_SAMPLER_2D:
      case GL_SAMPLER_CUBE:
      case GL_INT_VEC2:
      case GL_INT_VEC3:
      case GL_INT_VEC4:
          return GL_INT;
      default:
          UNREACHABLE();
    }

    return GL_NONE;
}

size_t UniformComponentSize(GLenum type)
{
    switch(type)
    {
      case GL_BOOL:  return sizeof(GLint);
      case GL_FLOAT: return sizeof(GLfloat);
      case GL_INT:   return sizeof(GLint);
      default:       UNREACHABLE();
    }

    return 0;
}

size_t UniformInternalSize(GLenum type)
{
    // Expanded to 4-element vectors
    return UniformComponentSize(UniformComponentType(type)) * VariableRowCount(type) * 4;
}

size_t UniformExternalSize(GLenum type)
{
    return UniformComponentSize(UniformComponentType(type)) * UniformComponentCount(type);
}

int VariableRowCount(GLenum type)
{
    switch (type)
    {
      case GL_NONE:
        return 0;
      case GL_BOOL:
      case GL_FLOAT:
      case GL_INT:
      case GL_BOOL_VEC2:
      case GL_FLOAT_VEC2:
      case GL_INT_VEC2:
      case GL_INT_VEC3:
      case GL_FLOAT_VEC3:
      case GL_BOOL_VEC3:
      case GL_BOOL_VEC4:
      case GL_FLOAT_VEC4:
      case GL_INT_VEC4:
      case GL_SAMPLER_2D:
      case GL_SAMPLER_CUBE:
        return 1;
      case GL_FLOAT_MAT2:
        return 2;
      case GL_FLOAT_MAT3:
        return 3;
      case GL_FLOAT_MAT4:
        return 4;
      default:
        UNREACHABLE();
    }

    return 0;
}

int VariableColumnCount(GLenum type)
{
    switch (type)
    {
      case GL_NONE:
        return 0;
      case GL_BOOL:
      case GL_FLOAT:
      case GL_INT:
      case GL_SAMPLER_2D:
      case GL_SAMPLER_CUBE:
        return 1;
      case GL_BOOL_VEC2:
      case GL_FLOAT_VEC2:
      case GL_INT_VEC2:
      case GL_FLOAT_MAT2:
        return 2;
      case GL_INT_VEC3:
      case GL_FLOAT_VEC3:
      case GL_BOOL_VEC3:
      case GL_FLOAT_MAT3:
        return 3;
      case GL_BOOL_VEC4:
      case GL_FLOAT_VEC4:
      case GL_INT_VEC4:
      case GL_FLOAT_MAT4:
        return 4;
      default:
        UNREACHABLE();
    }

    return 0;
}

int AllocateFirstFreeBits(unsigned int *bits, unsigned int allocationSize, unsigned int bitsSize)
{
    ASSERT(allocationSize <= bitsSize);

    unsigned int mask = std::numeric_limits<unsigned int>::max() >> (std::numeric_limits<unsigned int>::digits - allocationSize);

    for (unsigned int i = 0; i < bitsSize - allocationSize + 1; i++)
    {
        if ((*bits & mask) == 0)
        {
            *bits |= mask;
            return i;
        }

        mask <<= 1;
    }

    return -1;
}

GLsizei ComputePitch(GLsizei width, GLint internalformat, GLint alignment)
{
    ASSERT(alignment > 0 && isPow2(alignment));

    GLsizei rawPitch = ComputePixelSize(internalformat) * width;
    return (rawPitch + alignment - 1) & ~(alignment - 1);
}

GLsizei ComputeCompressedPitch(GLsizei width, GLenum internalformat)
{
    return ComputeCompressedSize(width, 1, internalformat);
}

GLsizei ComputeCompressedSize(GLsizei width, GLsizei height, GLenum internalformat)
{
    switch (internalformat)
    {
      case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        return 8 * ((width + 3) / 4) * ((height + 3) / 4);
      case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
      case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
        return 16 * ((width + 3) / 4) * ((height + 3) / 4);
      default:
        return 0;
    }
}

GLsizei ComputeTypeSize(GLenum type)
{
    switch (type)
    {
      case GL_BYTE:                            return 1;
      case GL_UNSIGNED_BYTE:                   return 1;
      case GL_SHORT:                           return 2;
      case GL_UNSIGNED_SHORT:                  return 2;
      case GL_INT:                             return 4;
      case GL_UNSIGNED_INT:                    return 4;
      case GL_FLOAT:                           return 4;
      case GL_HALF_FLOAT_OES:                  return 2;
      case GL_UNSIGNED_SHORT_5_6_5:            return 2;
      case GL_UNSIGNED_SHORT_4_4_4_4:          return 2;
      case GL_UNSIGNED_SHORT_5_5_5_1:          return 2;
      case GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT:  return 2;
      case GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT:  return 2;
      case GL_UNSIGNED_INT_2_10_10_10_REV_EXT: return 4;
      case GL_UNSIGNED_INT_24_8_OES:           return 4;
      default: UNREACHABLE();                  return 0;
    }
}

bool IsCompressed(GLenum format)
{
    if(format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ||
       format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
       format == GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE ||
       format == GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool IsDepthTexture(GLenum format)
{
    if (format == GL_DEPTH_COMPONENT ||
        format == GL_DEPTH_STENCIL_OES ||
        format == GL_DEPTH_COMPONENT16 ||
        format == GL_DEPTH_COMPONENT32_OES ||
        format == GL_DEPTH24_STENCIL8_OES)
    {
        return true;
    }

    return false;
}

bool IsStencilTexture(GLenum format)
{
    if (format == GL_DEPTH_STENCIL_OES ||
        format == GL_DEPTH24_STENCIL8_OES)
    {
        return true;
    }

    return false;
}

void MakeValidSize(bool isImage, bool isCompressed, GLsizei *requestWidth, GLsizei *requestHeight, int *levelOffset)
{
    int upsampleCount = 0;

    if (isCompressed)
    {
        // Don't expand the size of full textures that are at least 4x4
        // already.
        if (isImage || *requestWidth < 4 || *requestHeight < 4)
        {
            while (*requestWidth % 4 != 0 || *requestHeight % 4 != 0)
            {
                *requestWidth <<= 1;
                *requestHeight <<= 1;
                upsampleCount++;
            }
        }
    }
    *levelOffset = upsampleCount;
}

// Returns the size, in bytes, of a single texel in an Image
int ComputePixelSize(GLint internalformat)
{
    switch (internalformat)
    {
      case GL_ALPHA8_EXT:                       return sizeof(unsigned char);
      case GL_LUMINANCE8_EXT:                   return sizeof(unsigned char);
      case GL_ALPHA32F_EXT:                     return sizeof(float);
      case GL_LUMINANCE32F_EXT:                 return sizeof(float);
      case GL_ALPHA16F_EXT:                     return sizeof(unsigned short);
      case GL_LUMINANCE16F_EXT:                 return sizeof(unsigned short);
      case GL_LUMINANCE8_ALPHA8_EXT:            return sizeof(unsigned char) * 2;
      case GL_LUMINANCE_ALPHA32F_EXT:           return sizeof(float) * 2;
      case GL_LUMINANCE_ALPHA16F_EXT:           return sizeof(unsigned short) * 2;
      case GL_RGB8_OES:                         return sizeof(unsigned char) * 3;
      case GL_RGB565:                           return sizeof(unsigned short);
      case GL_RGB32F_EXT:                       return sizeof(float) * 3;
      case GL_RGB16F_EXT:                       return sizeof(unsigned short) * 3;
      case GL_RGBA8_OES:                        return sizeof(unsigned char) * 4;
      case GL_RGBA4:                            return sizeof(unsigned short);
      case GL_RGB5_A1:                          return sizeof(unsigned short);
      case GL_RGBA32F_EXT:                      return sizeof(float) * 4;
      case GL_RGBA16F_EXT:                      return sizeof(unsigned short) * 4;
      case GL_BGRA8_EXT:                        return sizeof(unsigned char) * 4;
      case GL_BGRA4_ANGLEX:                     return sizeof(unsigned short);
      case GL_BGR5_A1_ANGLEX:                   return sizeof(unsigned short);
      default: UNREACHABLE();
    }

    return 0;
}

bool IsCubemapTextureTarget(GLenum target)
{
    return (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
}

bool IsInternalTextureTarget(GLenum target)
{
    return target == GL_TEXTURE_2D || IsCubemapTextureTarget(target);
}

GLint ConvertSizedInternalFormat(GLenum format, GLenum type)
{
    switch (format)
    {
      case GL_ALPHA:
        switch (type)
        {
          case GL_UNSIGNED_BYTE:    return GL_ALPHA8_EXT;
          case GL_FLOAT:            return GL_ALPHA32F_EXT;
          case GL_HALF_FLOAT_OES:   return GL_ALPHA16F_EXT;
          default:                  UNIMPLEMENTED();
        }
        break;
      case GL_LUMINANCE:
        switch (type)
        {
          case GL_UNSIGNED_BYTE:    return GL_LUMINANCE8_EXT;
          case GL_FLOAT:            return GL_LUMINANCE32F_EXT;
          case GL_HALF_FLOAT_OES:   return GL_LUMINANCE16F_EXT;
          default:                  UNIMPLEMENTED();
        }
        break;
      case GL_LUMINANCE_ALPHA:
        switch (type)
        {
          case GL_UNSIGNED_BYTE:    return GL_LUMINANCE8_ALPHA8_EXT;
          case GL_FLOAT:            return GL_LUMINANCE_ALPHA32F_EXT;
          case GL_HALF_FLOAT_OES:   return GL_LUMINANCE_ALPHA16F_EXT;
          default:                  UNIMPLEMENTED();
        }
        break;
      case GL_RGB:
        switch (type)
        {
          case GL_UNSIGNED_BYTE:            return GL_RGB8_OES;
          case GL_UNSIGNED_SHORT_5_6_5:     return GL_RGB565;
          case GL_FLOAT:                    return GL_RGB32F_EXT;
          case GL_HALF_FLOAT_OES:           return GL_RGB16F_EXT;
          default:                          UNIMPLEMENTED();
        }
        break;
      case GL_RGBA:
        switch (type)
        {
          case GL_UNSIGNED_BYTE:            return GL_RGBA8_OES;
          case GL_UNSIGNED_SHORT_4_4_4_4:   return GL_RGBA4;
          case GL_UNSIGNED_SHORT_5_5_5_1:   return GL_RGB5_A1;
          case GL_FLOAT:                    return GL_RGBA32F_EXT;
          case GL_HALF_FLOAT_OES:           return GL_RGBA16F_EXT;
            break;
          default:                          UNIMPLEMENTED();
        }
        break;
      case GL_BGRA_EXT:
        switch (type)
        {
          case GL_UNSIGNED_BYTE:                    return GL_BGRA8_EXT;
          case GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT:   return GL_BGRA4_ANGLEX;
          case GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT:   return GL_BGR5_A1_ANGLEX;
          default:                                  UNIMPLEMENTED();
        }
        break;
      case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
      case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
      case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
        return format;
      case GL_DEPTH_COMPONENT:
        switch (type)
        {
          case GL_UNSIGNED_SHORT:           return GL_DEPTH_COMPONENT16;
          case GL_UNSIGNED_INT:             return GL_DEPTH_COMPONENT32_OES;
          default:                          UNIMPLEMENTED();
        }
        break;
      case GL_DEPTH_STENCIL_OES:
        switch (type)
        {
          case GL_UNSIGNED_INT_24_8_OES:    return GL_DEPTH24_STENCIL8_OES;
          default:                          UNIMPLEMENTED();
        }
        break;
      default:
        UNIMPLEMENTED();
    }

    return GL_NONE;
}

GLenum ExtractFormat(GLenum internalformat)
{
    switch (internalformat)
    {
      case GL_RGB565:                          return GL_RGB;
      case GL_RGBA4:                           return GL_RGBA;
      case GL_RGB5_A1:                         return GL_RGBA;
      case GL_RGB8_OES:                        return GL_RGB;
      case GL_RGBA8_OES:                       return GL_RGBA;
      case GL_LUMINANCE8_ALPHA8_EXT:           return GL_LUMINANCE_ALPHA;
      case GL_LUMINANCE8_EXT:                  return GL_LUMINANCE;
      case GL_ALPHA8_EXT:                      return GL_ALPHA;
      case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:    return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
      case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:   return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
      case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE: return GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE;
      case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE: return GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE;
      case GL_RGBA32F_EXT:                     return GL_RGBA;
      case GL_RGB32F_EXT:                      return GL_RGB;
      case GL_ALPHA32F_EXT:                    return GL_ALPHA;
      case GL_LUMINANCE32F_EXT:                return GL_LUMINANCE;
      case GL_LUMINANCE_ALPHA32F_EXT:          return GL_LUMINANCE_ALPHA;
      case GL_RGBA16F_EXT:                     return GL_RGBA;
      case GL_RGB16F_EXT:                      return GL_RGB;
      case GL_ALPHA16F_EXT:                    return GL_ALPHA;
      case GL_LUMINANCE16F_EXT:                return GL_LUMINANCE;
      case GL_LUMINANCE_ALPHA16F_EXT:          return GL_LUMINANCE_ALPHA;
      case GL_BGRA8_EXT:                       return GL_BGRA_EXT;
      case GL_DEPTH_COMPONENT16:               return GL_DEPTH_COMPONENT;
      case GL_DEPTH_COMPONENT32_OES:           return GL_DEPTH_COMPONENT;
      case GL_DEPTH24_STENCIL8_OES:            return GL_DEPTH_STENCIL_OES;
      default:                                 return GL_NONE;   // Unsupported
    }
}

GLenum ExtractType(GLenum internalformat)
{
    switch (internalformat)
    {
      case GL_RGB565:                          return GL_UNSIGNED_SHORT_5_6_5;
      case GL_RGBA4:                           return GL_UNSIGNED_SHORT_4_4_4_4;
      case GL_RGB5_A1:                         return GL_UNSIGNED_SHORT_5_5_5_1;
      case GL_RGB8_OES:                        return GL_UNSIGNED_BYTE;
      case GL_RGBA8_OES:                       return GL_UNSIGNED_BYTE;
      case GL_LUMINANCE8_ALPHA8_EXT:           return GL_UNSIGNED_BYTE;
      case GL_LUMINANCE8_EXT:                  return GL_UNSIGNED_BYTE;
      case GL_ALPHA8_EXT:                      return GL_UNSIGNED_BYTE;
      case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:    return GL_UNSIGNED_BYTE;
      case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:   return GL_UNSIGNED_BYTE;
      case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE: return GL_UNSIGNED_BYTE;
      case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE: return GL_UNSIGNED_BYTE;
      case GL_RGBA32F_EXT:                     return GL_FLOAT;
      case GL_RGB32F_EXT:                      return GL_FLOAT;
      case GL_ALPHA32F_EXT:                    return GL_FLOAT;
      case GL_LUMINANCE32F_EXT:                return GL_FLOAT;
      case GL_LUMINANCE_ALPHA32F_EXT:          return GL_FLOAT;
      case GL_RGBA16F_EXT:                     return GL_HALF_FLOAT_OES;
      case GL_RGB16F_EXT:                      return GL_HALF_FLOAT_OES;
      case GL_ALPHA16F_EXT:                    return GL_HALF_FLOAT_OES;
      case GL_LUMINANCE16F_EXT:                return GL_HALF_FLOAT_OES;
      case GL_LUMINANCE_ALPHA16F_EXT:          return GL_HALF_FLOAT_OES;
      case GL_BGRA8_EXT:                       return GL_UNSIGNED_BYTE;
      case GL_DEPTH_COMPONENT16:               return GL_UNSIGNED_SHORT;
      case GL_DEPTH_COMPONENT32_OES:           return GL_UNSIGNED_INT;
      case GL_DEPTH24_STENCIL8_OES:            return GL_UNSIGNED_INT_24_8_OES;
      default:                                 return GL_NONE;   // Unsupported
    }
}

bool IsColorRenderable(GLenum internalformat)
{
    switch (internalformat)
    {
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGB565:
      case GL_RGB8_OES:
      case GL_RGBA8_OES:
        return true;
      case GL_DEPTH_COMPONENT16:
      case GL_STENCIL_INDEX8:
      case GL_DEPTH24_STENCIL8_OES:
        return false;
      case GL_BGRA8_EXT:
          return true;
      default:
        UNIMPLEMENTED();
    }

    return false;
}

bool IsDepthRenderable(GLenum internalformat)
{
    switch (internalformat)
    {
      case GL_DEPTH_COMPONENT16:
      case GL_DEPTH24_STENCIL8_OES:
        return true;
      case GL_STENCIL_INDEX8:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGB565:
      case GL_RGB8_OES:
      case GL_RGBA8_OES:
        return false;
      default:
        UNIMPLEMENTED();
    }

    return false;
}

bool IsStencilRenderable(GLenum internalformat)
{
    switch (internalformat)
    {
      case GL_STENCIL_INDEX8:
      case GL_DEPTH24_STENCIL8_OES:
        return true;
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGB565:
      case GL_RGB8_OES:
      case GL_RGBA8_OES:
      case GL_DEPTH_COMPONENT16:
        return false;
      default:
        UNIMPLEMENTED();
    }

    return false;
}

bool IsFloat32Format(GLint internalformat)
{
    switch (internalformat)
    {
      case GL_RGBA32F_EXT:
      case GL_RGB32F_EXT:
      case GL_ALPHA32F_EXT:
      case GL_LUMINANCE32F_EXT:
      case GL_LUMINANCE_ALPHA32F_EXT:
        return true;
      default:
        return false;
    }
}

bool IsFloat16Format(GLint internalformat)
{
    switch (internalformat)
    {
      case GL_RGBA16F_EXT:
      case GL_RGB16F_EXT:
      case GL_ALPHA16F_EXT:
      case GL_LUMINANCE16F_EXT:
      case GL_LUMINANCE_ALPHA16F_EXT:
        return true;
      default:
        return false;
    }
}

unsigned int GetAlphaSize(GLenum colorFormat)
{
    switch (colorFormat)
    {
      case GL_RGBA16F_EXT:
        return 16;
      case GL_RGBA32F_EXT:
        return 32;
      case GL_RGBA4:
        return 4;
      case GL_RGBA8_OES:
      case GL_BGRA8_EXT:
        return 8;
      case GL_RGB5_A1:
        return 1;
      case GL_RGB8_OES:
      case GL_RGB565:
      case GL_RGB32F_EXT:
      case GL_RGB16F_EXT:
        return 0;
      default:
        return 0;
    }
}

unsigned int GetRedSize(GLenum colorFormat)
{
    switch (colorFormat)
    {
      case GL_RGBA16F_EXT:
      case GL_RGB16F_EXT:
        return 16;
      case GL_RGBA32F_EXT:
      case GL_RGB32F_EXT:
        return 32;
      case GL_RGBA4:
        return 4;
      case GL_RGBA8_OES:
      case GL_BGRA8_EXT:
      case GL_RGB8_OES:
        return 8;
      case GL_RGB5_A1:
      case GL_RGB565:
        return 5;
      default:
        return 0;
    }
}

unsigned int GetGreenSize(GLenum colorFormat)
{
    switch (colorFormat)
    {
      case GL_RGBA16F_EXT:
      case GL_RGB16F_EXT:
        return 16;
      case GL_RGBA32F_EXT:
      case GL_RGB32F_EXT:
        return 32;
      case GL_RGBA4:
        return 4;
      case GL_RGBA8_OES:
      case GL_BGRA8_EXT:
      case GL_RGB8_OES:
        return 8;
      case GL_RGB5_A1:
        return 5;
      case GL_RGB565:
        return 6;
      default:
        return 0;
    }
}

unsigned int GetBlueSize(GLenum colorFormat)
{
    switch (colorFormat)
    {
      case GL_RGBA16F_EXT:
      case GL_RGB16F_EXT:
        return 16;
      case GL_RGBA32F_EXT:
      case GL_RGB32F_EXT:
        return 32;
      case GL_RGBA4:
        return 4;
      case GL_RGBA8_OES:
      case GL_BGRA8_EXT:
      case GL_RGB8_OES:
        return 8;
      case GL_RGB5_A1:
      case GL_RGB565:
        return 5;
      default:
        return 0;
    }
}

unsigned int GetDepthSize(GLenum depthFormat)
{
    switch (depthFormat)
    {
      case GL_DEPTH_COMPONENT16:        return 16;
      case GL_DEPTH_COMPONENT32_OES:    return 32;
      case GL_DEPTH24_STENCIL8_OES:     return 24;
      default:                          return 0;
    }
}

unsigned int GetStencilSize(GLenum stencilFormat)
{
    switch (stencilFormat)
    {
      case GL_DEPTH24_STENCIL8_OES:     return 8;
      default:                          return 0;
    }
}

bool IsTriangleMode(GLenum drawMode)
{
    switch (drawMode)
    {
      case GL_TRIANGLES:
      case GL_TRIANGLE_FAN:
      case GL_TRIANGLE_STRIP:
        return true;
      case GL_POINTS:
      case GL_LINES:
      case GL_LINE_LOOP:
      case GL_LINE_STRIP:
        return false;
      default: UNREACHABLE();
    }

    return false;
}

}

std::string getTempPath()
{
#if !defined(ANGLE_OS_WINRT)
    char path[MAX_PATH];
    DWORD pathLen = GetTempPathA(sizeof(path) / sizeof(path[0]), path);
    if (pathLen == 0)
    {
        UNREACHABLE();
        return std::string();
    }

    UINT unique = GetTempFileNameA(path, "sh", 0, path);
    if (unique == 0)
    {
        UNREACHABLE();
        return std::string();
    }
#else
    static std::string path;

    while (path.empty()) {
        ComPtr<IApplicationDataStatics> factory;
        Wrappers::HStringReference classId(RuntimeClass_Windows_Storage_ApplicationData);
        HRESULT result = RoGetActivationFactory(classId.Get(), IID_PPV_ARGS(&factory));
        if (FAILED(result))
            break;

        ComPtr<IApplicationData> applicationData;
        result = factory->get_Current(&applicationData);
        if (FAILED(result))
            break;

        ComPtr<IStorageFolder> storageFolder;
        result = applicationData->get_LocalFolder(&storageFolder);
        if (FAILED(result))
            break;

        ComPtr<IStorageItem> localFolder;
        result = storageFolder.As(&localFolder);
        if (FAILED(result))
            break;

        HSTRING localFolderPath;
        result = localFolder->get_Path(&localFolderPath);
        if (FAILED(result))
            break;

        std::wstring_convert< std::codecvt_utf8<wchar_t> > converter;
        path = converter.to_bytes(WindowsGetStringRawBuffer(localFolderPath, NULL));
        if (path.empty())
        {
            UNREACHABLE();
            break;
        }
    }
#endif
    
    return path;
}

void writeFile(const char* path, const void* content, size_t size)
{
    FILE* file = fopen(path, "w");
    if (!file)
    {
        UNREACHABLE();
        return;
    }

    fwrite(content, sizeof(char), size, file);
    fclose(file);
}
