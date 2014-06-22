#include "precompiled.h"
//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// renderer9_utils.cpp: Conversion functions and other utility routines
// specific to the D3D9 renderer.

#include "libGLESv2/renderer/d3d9/renderer9_utils.h"
#include "libGLESv2/mathutil.h"
#include "libGLESv2/Context.h"

#include "common/debug.h"

namespace gl_d3d9
{

D3DCMPFUNC ConvertComparison(GLenum comparison)
{
    D3DCMPFUNC d3dComp = D3DCMP_ALWAYS;
    switch (comparison)
    {
      case GL_NEVER:    d3dComp = D3DCMP_NEVER;        break;
      case GL_ALWAYS:   d3dComp = D3DCMP_ALWAYS;       break;
      case GL_LESS:     d3dComp = D3DCMP_LESS;         break;
      case GL_LEQUAL:   d3dComp = D3DCMP_LESSEQUAL;    break;
      case GL_EQUAL:    d3dComp = D3DCMP_EQUAL;        break;
      case GL_GREATER:  d3dComp = D3DCMP_GREATER;      break;
      case GL_GEQUAL:   d3dComp = D3DCMP_GREATEREQUAL; break;
      case GL_NOTEQUAL: d3dComp = D3DCMP_NOTEQUAL;     break;
      default: UNREACHABLE();
    }

    return d3dComp;
}

D3DCOLOR ConvertColor(gl::Color color)
{
    return D3DCOLOR_RGBA(gl::unorm<8>(color.red),
                         gl::unorm<8>(color.green),
                         gl::unorm<8>(color.blue),
                         gl::unorm<8>(color.alpha));
}

D3DBLEND ConvertBlendFunc(GLenum blend)
{
    D3DBLEND d3dBlend = D3DBLEND_ZERO;

    switch (blend)
    {
      case GL_ZERO:                     d3dBlend = D3DBLEND_ZERO;           break;
      case GL_ONE:                      d3dBlend = D3DBLEND_ONE;            break;
      case GL_SRC_COLOR:                d3dBlend = D3DBLEND_SRCCOLOR;       break;
      case GL_ONE_MINUS_SRC_COLOR:      d3dBlend = D3DBLEND_INVSRCCOLOR;    break;
      case GL_DST_COLOR:                d3dBlend = D3DBLEND_DESTCOLOR;      break;
      case GL_ONE_MINUS_DST_COLOR:      d3dBlend = D3DBLEND_INVDESTCOLOR;   break;
      case GL_SRC_ALPHA:                d3dBlend = D3DBLEND_SRCALPHA;       break;
      case GL_ONE_MINUS_SRC_ALPHA:      d3dBlend = D3DBLEND_INVSRCALPHA;    break;
      case GL_DST_ALPHA:                d3dBlend = D3DBLEND_DESTALPHA;      break;
      case GL_ONE_MINUS_DST_ALPHA:      d3dBlend = D3DBLEND_INVDESTALPHA;   break;
      case GL_CONSTANT_COLOR:           d3dBlend = D3DBLEND_BLENDFACTOR;    break;
      case GL_ONE_MINUS_CONSTANT_COLOR: d3dBlend = D3DBLEND_INVBLENDFACTOR; break;
      case GL_CONSTANT_ALPHA:           d3dBlend = D3DBLEND_BLENDFACTOR;    break;
      case GL_ONE_MINUS_CONSTANT_ALPHA: d3dBlend = D3DBLEND_INVBLENDFACTOR; break;
      case GL_SRC_ALPHA_SATURATE:       d3dBlend = D3DBLEND_SRCALPHASAT;    break;
      default: UNREACHABLE();
    }

    return d3dBlend;
}

D3DBLENDOP ConvertBlendOp(GLenum blendOp)
{
    D3DBLENDOP d3dBlendOp = D3DBLENDOP_ADD;

    switch (blendOp)
    {
      case GL_FUNC_ADD:              d3dBlendOp = D3DBLENDOP_ADD;         break;
      case GL_FUNC_SUBTRACT:         d3dBlendOp = D3DBLENDOP_SUBTRACT;    break;
      case GL_FUNC_REVERSE_SUBTRACT: d3dBlendOp = D3DBLENDOP_REVSUBTRACT; break;
      default: UNREACHABLE();
    }

    return d3dBlendOp;
}

D3DSTENCILOP ConvertStencilOp(GLenum stencilOp)
{
    D3DSTENCILOP d3dStencilOp = D3DSTENCILOP_KEEP;

    switch (stencilOp)
    {
      case GL_ZERO:      d3dStencilOp = D3DSTENCILOP_ZERO;    break;
      case GL_KEEP:      d3dStencilOp = D3DSTENCILOP_KEEP;    break;
      case GL_REPLACE:   d3dStencilOp = D3DSTENCILOP_REPLACE; break;
      case GL_INCR:      d3dStencilOp = D3DSTENCILOP_INCRSAT; break;
      case GL_DECR:      d3dStencilOp = D3DSTENCILOP_DECRSAT; break;
      case GL_INVERT:    d3dStencilOp = D3DSTENCILOP_INVERT;  break;
      case GL_INCR_WRAP: d3dStencilOp = D3DSTENCILOP_INCR;    break;
      case GL_DECR_WRAP: d3dStencilOp = D3DSTENCILOP_DECR;    break;
      default: UNREACHABLE();
    }

    return d3dStencilOp;
}

D3DTEXTUREADDRESS ConvertTextureWrap(GLenum wrap)
{
    D3DTEXTUREADDRESS d3dWrap = D3DTADDRESS_WRAP;

    switch (wrap)
    {
      case GL_REPEAT:            d3dWrap = D3DTADDRESS_WRAP;   break;
      case GL_CLAMP_TO_EDGE:     d3dWrap = D3DTADDRESS_CLAMP;  break;
      case GL_MIRRORED_REPEAT:   d3dWrap = D3DTADDRESS_MIRROR; break;
      default: UNREACHABLE();
    }

    return d3dWrap;
}

D3DCULL ConvertCullMode(GLenum cullFace, GLenum frontFace)
{
    D3DCULL cull = D3DCULL_CCW;
    switch (cullFace)
    {
      case GL_FRONT:
        cull = (frontFace == GL_CCW ? D3DCULL_CW : D3DCULL_CCW);
        break;
      case GL_BACK:
        cull = (frontFace == GL_CCW ? D3DCULL_CCW : D3DCULL_CW);
        break;
      case GL_FRONT_AND_BACK:
        cull = D3DCULL_NONE; // culling will be handled during draw
        break;
      default: UNREACHABLE();
    }

    return cull;
}

D3DCUBEMAP_FACES ConvertCubeFace(GLenum cubeFace)
{
    D3DCUBEMAP_FACES face = D3DCUBEMAP_FACE_POSITIVE_X;

    switch (cubeFace)
    {
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        face = D3DCUBEMAP_FACE_POSITIVE_X;
        break;
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        face = D3DCUBEMAP_FACE_NEGATIVE_X;
        break;
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        face = D3DCUBEMAP_FACE_POSITIVE_Y;
        break;
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        face = D3DCUBEMAP_FACE_NEGATIVE_Y;
        break;
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        face = D3DCUBEMAP_FACE_POSITIVE_Z;
        break;
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        face = D3DCUBEMAP_FACE_NEGATIVE_Z;
        break;
      default: UNREACHABLE();
    }

    return face;
}

DWORD ConvertColorMask(bool red, bool green, bool blue, bool alpha)
{
    return (red   ? D3DCOLORWRITEENABLE_RED   : 0) |
           (green ? D3DCOLORWRITEENABLE_GREEN : 0) |
           (blue  ? D3DCOLORWRITEENABLE_BLUE  : 0) |
           (alpha ? D3DCOLORWRITEENABLE_ALPHA : 0);
}

D3DTEXTUREFILTERTYPE ConvertMagFilter(GLenum magFilter, float maxAnisotropy)
{
    if (maxAnisotropy > 1.0f)
    {
        return D3DTEXF_ANISOTROPIC;
    }

    D3DTEXTUREFILTERTYPE d3dMagFilter = D3DTEXF_POINT;
    switch (magFilter)
    {
      case GL_NEAREST: d3dMagFilter = D3DTEXF_POINT;  break;
      case GL_LINEAR:  d3dMagFilter = D3DTEXF_LINEAR; break;
      default: UNREACHABLE();
    }

    return d3dMagFilter;
}

void ConvertMinFilter(GLenum minFilter, D3DTEXTUREFILTERTYPE *d3dMinFilter, D3DTEXTUREFILTERTYPE *d3dMipFilter, float maxAnisotropy)
{
    switch (minFilter)
    {
      case GL_NEAREST:
        *d3dMinFilter = D3DTEXF_POINT;
        *d3dMipFilter = D3DTEXF_NONE;
        break;
      case GL_LINEAR:
        *d3dMinFilter = D3DTEXF_LINEAR;
        *d3dMipFilter = D3DTEXF_NONE;
        break;
      case GL_NEAREST_MIPMAP_NEAREST:
        *d3dMinFilter = D3DTEXF_POINT;
        *d3dMipFilter = D3DTEXF_POINT;
        break;
      case GL_LINEAR_MIPMAP_NEAREST:
        *d3dMinFilter = D3DTEXF_LINEAR;
        *d3dMipFilter = D3DTEXF_POINT;
        break;
      case GL_NEAREST_MIPMAP_LINEAR:
        *d3dMinFilter = D3DTEXF_POINT;
        *d3dMipFilter = D3DTEXF_LINEAR;
        break;
      case GL_LINEAR_MIPMAP_LINEAR:
        *d3dMinFilter = D3DTEXF_LINEAR;
        *d3dMipFilter = D3DTEXF_LINEAR;
        break;
      default:
        *d3dMinFilter = D3DTEXF_POINT;
        *d3dMipFilter = D3DTEXF_NONE;
        UNREACHABLE();
    }

    if (maxAnisotropy > 1.0f)
    {
        *d3dMinFilter = D3DTEXF_ANISOTROPIC;
    }
}

D3DFORMAT ConvertRenderbufferFormat(GLenum format)
{
    switch (format)
    {
      case GL_NONE:                 return D3DFMT_NULL;
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8_OES:            return D3DFMT_A8R8G8B8;
      case GL_RGB565:               return D3DFMT_R5G6B5;
      case GL_RGB8_OES:             return D3DFMT_X8R8G8B8;
      case GL_DEPTH_COMPONENT16:
      case GL_STENCIL_INDEX8:       
      case GL_DEPTH24_STENCIL8_OES: return D3DFMT_D24S8;
      default: UNREACHABLE();       return D3DFMT_A8R8G8B8;
    }
}

D3DMULTISAMPLE_TYPE GetMultisampleTypeFromSamples(GLsizei samples)
{
    if (samples <= 1)
        return D3DMULTISAMPLE_NONE;
    else
        return (D3DMULTISAMPLE_TYPE)samples;
}

}

namespace d3d9_gl
{

unsigned int GetStencilSize(D3DFORMAT stencilFormat)
{
    if (stencilFormat == D3DFMT_INTZ)
    {
        return 8;
    }
    switch(stencilFormat)
    {
      case D3DFMT_D24FS8:
      case D3DFMT_D24S8:
        return 8;
      case D3DFMT_D24X4S4:
        return 4;
      case D3DFMT_D15S1:
        return 1;
      case D3DFMT_D16_LOCKABLE:
      case D3DFMT_D32:
      case D3DFMT_D24X8:
      case D3DFMT_D32F_LOCKABLE:
      case D3DFMT_D16:
        return 0;
    //case D3DFMT_D32_LOCKABLE:  return 0;   // DirectX 9Ex only
    //case D3DFMT_S8_LOCKABLE:   return 8;   // DirectX 9Ex only
      default:
        return 0;
    }
}

unsigned int GetAlphaSize(D3DFORMAT colorFormat)
{
    switch (colorFormat)
    {
      case D3DFMT_A16B16G16R16F:
        return 16;
      case D3DFMT_A32B32G32R32F:
        return 32;
      case D3DFMT_A2R10G10B10:
        return 2;
      case D3DFMT_A8R8G8B8:
        return 8;
      case D3DFMT_A1R5G5B5:
        return 1;
      case D3DFMT_X8R8G8B8:
      case D3DFMT_R5G6B5:
        return 0;
      default:
        return 0;
    }
}

GLsizei GetSamplesFromMultisampleType(D3DMULTISAMPLE_TYPE type)
{
    if (type == D3DMULTISAMPLE_NONMASKABLE)
        return 0;
    else
        return type;
}

bool IsFormatChannelEquivalent(D3DFORMAT d3dformat, GLenum format)
{
    switch (d3dformat)
    {
      case D3DFMT_L8:
        return (format == GL_LUMINANCE);
      case D3DFMT_A8L8:
        return (format == GL_LUMINANCE_ALPHA);
      case D3DFMT_DXT1:
        return (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT || format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT);
      case D3DFMT_DXT3:
        return (format == GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE);
      case D3DFMT_DXT5:
        return (format == GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE);
      case D3DFMT_A8R8G8B8:
      case D3DFMT_A16B16G16R16F:
      case D3DFMT_A32B32G32R32F:
        return (format == GL_RGBA || format == GL_BGRA_EXT);
      case D3DFMT_X8R8G8B8:
        return (format == GL_RGB);
      default:
        if (d3dformat == D3DFMT_INTZ && gl::IsDepthTexture(format))
            return true;
        return false;
    }
}

GLenum ConvertBackBufferFormat(D3DFORMAT format)
{
    switch (format)
    {
      case D3DFMT_A4R4G4B4: return GL_RGBA4;
      case D3DFMT_A8R8G8B8: return GL_RGBA8_OES;
      case D3DFMT_A1R5G5B5: return GL_RGB5_A1;
      case D3DFMT_R5G6B5:   return GL_RGB565;
      case D3DFMT_X8R8G8B8: return GL_RGB8_OES;
      default:
        UNREACHABLE();
    }

    return GL_RGBA4;
}

GLenum ConvertDepthStencilFormat(D3DFORMAT format)
{
    if (format == D3DFMT_INTZ)
    {
        return GL_DEPTH24_STENCIL8_OES;
    }
    switch (format)
    {
      case D3DFMT_D16:
      case D3DFMT_D24X8:
        return GL_DEPTH_COMPONENT16;
      case D3DFMT_D24S8:
        return GL_DEPTH24_STENCIL8_OES;
      case D3DFMT_UNKNOWN:
        return GL_NONE;
      default:
        UNREACHABLE();
    }

    return GL_DEPTH24_STENCIL8_OES;
}

GLenum ConvertRenderTargetFormat(D3DFORMAT format)
{
    if (format == D3DFMT_INTZ)
    {
        return GL_DEPTH24_STENCIL8_OES;
    }
    
    switch (format)
    {
      case D3DFMT_A4R4G4B4: return GL_RGBA4;
      case D3DFMT_A8R8G8B8: return GL_RGBA8_OES;
      case D3DFMT_A1R5G5B5: return GL_RGB5_A1;
      case D3DFMT_R5G6B5:   return GL_RGB565;
      case D3DFMT_X8R8G8B8: return GL_RGB8_OES;
      case D3DFMT_D16:
      case D3DFMT_D24X8:
        return GL_DEPTH_COMPONENT16;
      case D3DFMT_D24S8:
        return GL_DEPTH24_STENCIL8_OES;
      case D3DFMT_UNKNOWN:
        return GL_NONE;
      default:
        UNREACHABLE();
    }

    return GL_RGBA4;
}

GLenum GetEquivalentFormat(D3DFORMAT format)
{
    if (format == D3DFMT_INTZ)
        return GL_DEPTH24_STENCIL8_OES;
    if (format == D3DFMT_NULL)
        return GL_NONE;

    switch (format)
    {
      case D3DFMT_A4R4G4B4:             return GL_RGBA4;
      case D3DFMT_A8R8G8B8:             return GL_RGBA8_OES;
      case D3DFMT_A1R5G5B5:             return GL_RGB5_A1;
      case D3DFMT_R5G6B5:               return GL_RGB565;
      case D3DFMT_X8R8G8B8:             return GL_RGB8_OES;
      case D3DFMT_D16:                  return GL_DEPTH_COMPONENT16;
      case D3DFMT_D24S8:                return GL_DEPTH24_STENCIL8_OES;
      case D3DFMT_UNKNOWN:              return GL_NONE;
      case D3DFMT_DXT1:                 return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
      case D3DFMT_DXT3:                 return GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE;
      case D3DFMT_DXT5:                 return GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE;
      case D3DFMT_A32B32G32R32F:        return GL_RGBA32F_EXT;
      case D3DFMT_A16B16G16R16F:        return GL_RGBA16F_EXT;
      case D3DFMT_L8:                   return GL_LUMINANCE8_EXT;
      case D3DFMT_A8L8:                 return GL_LUMINANCE8_ALPHA8_EXT;
      default:              UNREACHABLE();
        return GL_NONE;
    }
}

}

namespace d3d9
{

bool IsCompressedFormat(D3DFORMAT surfaceFormat)
{
    switch(surfaceFormat)
    {
      case D3DFMT_DXT1:
      case D3DFMT_DXT2:
      case D3DFMT_DXT3:
      case D3DFMT_DXT4:
      case D3DFMT_DXT5:
        return true;
      default:
        return false;
    }
}

size_t ComputeRowSize(D3DFORMAT format, unsigned int width)
{
    if (format == D3DFMT_INTZ)
    {
        return 4 * width;
    }
    switch (format)
    {
      case D3DFMT_L8:
          return 1 * width;
      case D3DFMT_A8L8:
          return 2 * width;
      case D3DFMT_X8R8G8B8:
      case D3DFMT_A8R8G8B8:
        return 4 * width;
      case D3DFMT_A16B16G16R16F:
        return 8 * width;
      case D3DFMT_A32B32G32R32F:
        return 16 * width;
      case D3DFMT_DXT1:
        return 8 * ((width + 3) / 4);
      case D3DFMT_DXT3:
      case D3DFMT_DXT5:
        return 16 * ((width + 3) / 4);
      default:
        UNREACHABLE();
        return 0;
    }
}

}
