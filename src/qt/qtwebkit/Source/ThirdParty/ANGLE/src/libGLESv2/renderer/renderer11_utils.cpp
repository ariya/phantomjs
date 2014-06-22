#include "precompiled.h"
//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// renderer11_utils.cpp: Conversion functions and other utility routines
// specific to the D3D11 renderer.

#include "libGLESv2/renderer/renderer11_utils.h"

#include "common/debug.h"

namespace gl_d3d11
{

D3D11_BLEND ConvertBlendFunc(GLenum glBlend, bool isAlpha)
{
    D3D11_BLEND d3dBlend = D3D11_BLEND_ZERO;

    switch (glBlend)
    {
      case GL_ZERO:                     d3dBlend = D3D11_BLEND_ZERO;                break;
      case GL_ONE:                      d3dBlend = D3D11_BLEND_ONE;                 break;
      case GL_SRC_COLOR:                d3dBlend = (isAlpha ? D3D11_BLEND_SRC_ALPHA : D3D11_BLEND_SRC_COLOR);           break;
      case GL_ONE_MINUS_SRC_COLOR:      d3dBlend = (isAlpha ? D3D11_BLEND_INV_SRC_ALPHA : D3D11_BLEND_INV_SRC_COLOR);   break;
      case GL_DST_COLOR:                d3dBlend = (isAlpha ? D3D11_BLEND_DEST_ALPHA : D3D11_BLEND_DEST_COLOR);         break;
      case GL_ONE_MINUS_DST_COLOR:      d3dBlend = (isAlpha ? D3D11_BLEND_INV_DEST_ALPHA : D3D11_BLEND_INV_DEST_COLOR); break;
      case GL_SRC_ALPHA:                d3dBlend = D3D11_BLEND_SRC_ALPHA;           break;
      case GL_ONE_MINUS_SRC_ALPHA:      d3dBlend = D3D11_BLEND_INV_SRC_ALPHA;       break;
      case GL_DST_ALPHA:                d3dBlend = D3D11_BLEND_DEST_ALPHA;          break;
      case GL_ONE_MINUS_DST_ALPHA:      d3dBlend = D3D11_BLEND_INV_DEST_ALPHA;      break;
      case GL_CONSTANT_COLOR:           d3dBlend = D3D11_BLEND_BLEND_FACTOR;        break;
      case GL_ONE_MINUS_CONSTANT_COLOR: d3dBlend = D3D11_BLEND_INV_BLEND_FACTOR;    break;
      case GL_CONSTANT_ALPHA:           d3dBlend = D3D11_BLEND_BLEND_FACTOR;        break;
      case GL_ONE_MINUS_CONSTANT_ALPHA: d3dBlend = D3D11_BLEND_INV_BLEND_FACTOR;    break;
      case GL_SRC_ALPHA_SATURATE:       d3dBlend = D3D11_BLEND_SRC_ALPHA_SAT;       break;
      default: UNREACHABLE();
    }

    return d3dBlend;
}

D3D11_BLEND_OP ConvertBlendOp(GLenum glBlendOp)
{
    D3D11_BLEND_OP d3dBlendOp = D3D11_BLEND_OP_ADD;

    switch (glBlendOp)
    {
      case GL_FUNC_ADD:              d3dBlendOp = D3D11_BLEND_OP_ADD;           break;
      case GL_FUNC_SUBTRACT:         d3dBlendOp = D3D11_BLEND_OP_SUBTRACT;      break;
      case GL_FUNC_REVERSE_SUBTRACT: d3dBlendOp = D3D11_BLEND_OP_REV_SUBTRACT;  break;
      default: UNREACHABLE();
    }

    return d3dBlendOp;
}

UINT8 ConvertColorMask(bool red, bool green, bool blue, bool alpha)
{
    UINT8 mask = 0;
    if (red)
    {
        mask |= D3D11_COLOR_WRITE_ENABLE_RED;
    }
    if (green)
    {
        mask |= D3D11_COLOR_WRITE_ENABLE_GREEN;
    }
    if (blue)
    {
        mask |= D3D11_COLOR_WRITE_ENABLE_BLUE;
    }
    if (alpha)
    {
        mask |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
    }
    return mask;
}

D3D11_CULL_MODE ConvertCullMode(bool cullEnabled, GLenum cullMode)
{
    D3D11_CULL_MODE cull = D3D11_CULL_NONE;

    if (cullEnabled)
    {
        switch (cullMode)
        {
          case GL_FRONT:            cull = D3D11_CULL_FRONT;    break;
          case GL_BACK:             cull = D3D11_CULL_BACK;     break;
          case GL_FRONT_AND_BACK:   cull = D3D11_CULL_NONE;     break;
          default: UNREACHABLE();
        }
    }
    else
    {
        cull = D3D11_CULL_NONE;
    }

    return cull;
}

D3D11_COMPARISON_FUNC ConvertComparison(GLenum comparison)
{
    D3D11_COMPARISON_FUNC d3dComp = D3D11_COMPARISON_NEVER;
    switch (comparison)
    {
      case GL_NEVER:    d3dComp = D3D11_COMPARISON_NEVER;           break;
      case GL_ALWAYS:   d3dComp = D3D11_COMPARISON_ALWAYS;          break;
      case GL_LESS:     d3dComp = D3D11_COMPARISON_LESS;            break;
      case GL_LEQUAL:   d3dComp = D3D11_COMPARISON_LESS_EQUAL;      break;
      case GL_EQUAL:    d3dComp = D3D11_COMPARISON_EQUAL;           break;
      case GL_GREATER:  d3dComp = D3D11_COMPARISON_GREATER;         break;
      case GL_GEQUAL:   d3dComp = D3D11_COMPARISON_GREATER_EQUAL;   break;
      case GL_NOTEQUAL: d3dComp = D3D11_COMPARISON_NOT_EQUAL;       break;
      default: UNREACHABLE();
    }

    return d3dComp;
}

D3D11_DEPTH_WRITE_MASK ConvertDepthMask(bool depthWriteEnabled)
{
    return depthWriteEnabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
}

UINT8 ConvertStencilMask(GLuint stencilmask)
{
    return static_cast<UINT8>(stencilmask);
}

D3D11_STENCIL_OP ConvertStencilOp(GLenum stencilOp)
{
    D3D11_STENCIL_OP d3dStencilOp = D3D11_STENCIL_OP_KEEP;

    switch (stencilOp)
    {
      case GL_ZERO:      d3dStencilOp = D3D11_STENCIL_OP_ZERO;      break;
      case GL_KEEP:      d3dStencilOp = D3D11_STENCIL_OP_KEEP;      break;
      case GL_REPLACE:   d3dStencilOp = D3D11_STENCIL_OP_REPLACE;   break;
      case GL_INCR:      d3dStencilOp = D3D11_STENCIL_OP_INCR_SAT;  break;
      case GL_DECR:      d3dStencilOp = D3D11_STENCIL_OP_DECR_SAT;  break;
      case GL_INVERT:    d3dStencilOp = D3D11_STENCIL_OP_INVERT;    break;
      case GL_INCR_WRAP: d3dStencilOp = D3D11_STENCIL_OP_INCR;      break;
      case GL_DECR_WRAP: d3dStencilOp = D3D11_STENCIL_OP_DECR;      break;
      default: UNREACHABLE();
    }

    return d3dStencilOp;
}

D3D11_FILTER ConvertFilter(GLenum minFilter, GLenum magFilter, float maxAnisotropy)
{
    if (maxAnisotropy > 1.0f)
    {
        return D3D11_ENCODE_ANISOTROPIC_FILTER(false);
    }
    else
    {
        D3D11_FILTER_TYPE dxMin = D3D11_FILTER_TYPE_POINT;
        D3D11_FILTER_TYPE dxMip = D3D11_FILTER_TYPE_POINT;
        switch (minFilter)
        {
          case GL_NEAREST:                dxMin = D3D11_FILTER_TYPE_POINT;  dxMip = D3D11_FILTER_TYPE_POINT;  break;
          case GL_LINEAR:                 dxMin = D3D11_FILTER_TYPE_LINEAR; dxMip = D3D11_FILTER_TYPE_POINT;  break;
          case GL_NEAREST_MIPMAP_NEAREST: dxMin = D3D11_FILTER_TYPE_POINT;  dxMip = D3D11_FILTER_TYPE_POINT;  break;
          case GL_LINEAR_MIPMAP_NEAREST:  dxMin = D3D11_FILTER_TYPE_LINEAR; dxMip = D3D11_FILTER_TYPE_POINT;  break;
          case GL_NEAREST_MIPMAP_LINEAR:  dxMin = D3D11_FILTER_TYPE_POINT;  dxMip = D3D11_FILTER_TYPE_LINEAR; break;
          case GL_LINEAR_MIPMAP_LINEAR:   dxMin = D3D11_FILTER_TYPE_LINEAR; dxMip = D3D11_FILTER_TYPE_LINEAR; break;
          default:                        UNREACHABLE();
        }

        D3D11_FILTER_TYPE dxMag = D3D11_FILTER_TYPE_POINT;
        switch (magFilter)
        {
          case GL_NEAREST: dxMag = D3D11_FILTER_TYPE_POINT;  break;
          case GL_LINEAR:  dxMag = D3D11_FILTER_TYPE_LINEAR; break;
          default:         UNREACHABLE();
        }

        return D3D11_ENCODE_BASIC_FILTER(dxMin, dxMag, dxMip, false);
    }
}

D3D11_TEXTURE_ADDRESS_MODE ConvertTextureWrap(GLenum wrap)
{
    switch (wrap)
    {
      case GL_REPEAT:          return D3D11_TEXTURE_ADDRESS_WRAP;
      case GL_CLAMP_TO_EDGE:   return D3D11_TEXTURE_ADDRESS_CLAMP;
      case GL_MIRRORED_REPEAT: return D3D11_TEXTURE_ADDRESS_MIRROR;
      default:                 UNREACHABLE();
    }

    return D3D11_TEXTURE_ADDRESS_WRAP;
}

FLOAT ConvertMinLOD(GLenum minFilter, unsigned int lodOffset)
{
    return (minFilter == GL_NEAREST || minFilter == GL_LINEAR) ? static_cast<float>(lodOffset) : -FLT_MAX;
}

FLOAT ConvertMaxLOD(GLenum minFilter, unsigned int lodOffset)
{
    return (minFilter == GL_NEAREST || minFilter == GL_LINEAR) ? static_cast<float>(lodOffset) : FLT_MAX;
}

}

namespace d3d11_gl
{

GLenum ConvertBackBufferFormat(DXGI_FORMAT format)
{
    switch (format)
    {
      case DXGI_FORMAT_R8G8B8A8_UNORM: return GL_RGBA8_OES;
      case DXGI_FORMAT_B8G8R8A8_UNORM: return GL_BGRA8_EXT;
      default:
        UNREACHABLE();
    }

    return GL_RGBA8_OES;
}

GLenum ConvertDepthStencilFormat(DXGI_FORMAT format)
{
    switch (format)
    {
      case DXGI_FORMAT_UNKNOWN: return GL_NONE;
      case DXGI_FORMAT_D16_UNORM: return GL_DEPTH_COMPONENT16;
      case DXGI_FORMAT_D24_UNORM_S8_UINT: return GL_DEPTH24_STENCIL8_OES;
      default:
        UNREACHABLE();
    }

    return GL_DEPTH24_STENCIL8_OES;
}

GLenum ConvertRenderbufferFormat(DXGI_FORMAT format)
{
    switch (format)
    {
      case DXGI_FORMAT_B8G8R8A8_UNORM:
        return GL_BGRA8_EXT;
      case DXGI_FORMAT_R8G8B8A8_UNORM:
        return GL_RGBA8_OES;
      case DXGI_FORMAT_D16_UNORM:
        return GL_DEPTH_COMPONENT16;
      case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return GL_DEPTH24_STENCIL8_OES;
      default:
        UNREACHABLE();
    }

    return GL_RGBA8_OES;
}

GLenum ConvertTextureInternalFormat(DXGI_FORMAT format)
{
    switch (format)
    {
      case DXGI_FORMAT_R8G8B8A8_UNORM:
        return GL_RGBA8_OES;
      case DXGI_FORMAT_A8_UNORM:
        return GL_ALPHA8_EXT;
      case DXGI_FORMAT_BC1_UNORM:
        return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
      case DXGI_FORMAT_BC2_UNORM:
        return GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE;
      case DXGI_FORMAT_BC3_UNORM:
        return GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE;
      case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return GL_RGBA32F_EXT;
      case DXGI_FORMAT_R32G32B32_FLOAT:
        return GL_RGB32F_EXT;
      case DXGI_FORMAT_R16G16B16A16_FLOAT:
        return GL_RGBA16F_EXT;
      case DXGI_FORMAT_B8G8R8A8_UNORM:
        return GL_BGRA8_EXT;
      case DXGI_FORMAT_R8_UNORM:
        return GL_R8_EXT;
      case DXGI_FORMAT_R8G8_UNORM:
        return GL_RG8_EXT;
      case DXGI_FORMAT_R16_FLOAT:
        return GL_R16F_EXT;
      case DXGI_FORMAT_R16G16_FLOAT:
        return GL_RG16F_EXT;
      case DXGI_FORMAT_D16_UNORM:
        return GL_DEPTH_COMPONENT16;
      case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return GL_DEPTH24_STENCIL8_OES;
      case DXGI_FORMAT_UNKNOWN:
        return GL_NONE;
      default:
        UNREACHABLE();
    }

    return GL_RGBA8_OES;
}

}

namespace gl_d3d11
{

DXGI_FORMAT ConvertRenderbufferFormat(GLenum format)
{
    switch (format)
    {
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8_OES:
      case GL_RGB565:
      case GL_RGB8_OES:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
      case GL_BGRA8_EXT:
        return DXGI_FORMAT_B8G8R8A8_UNORM;
      case GL_DEPTH_COMPONENT16:
        return DXGI_FORMAT_D16_UNORM;
      case GL_STENCIL_INDEX8:
      case GL_DEPTH24_STENCIL8_OES:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
      default:
        UNREACHABLE();
    }

    return DXGI_FORMAT_R8G8B8A8_UNORM;
}

DXGI_FORMAT ConvertTextureFormat(GLenum internalformat)
{
    switch (internalformat)
    {
      case GL_RGB565:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGB8_OES:
      case GL_RGBA8_OES:
      case GL_LUMINANCE8_EXT:
      case GL_LUMINANCE8_ALPHA8_EXT:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
      case GL_ALPHA8_EXT:
        return DXGI_FORMAT_A8_UNORM;
      case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        return DXGI_FORMAT_BC1_UNORM;
      case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
        return DXGI_FORMAT_BC2_UNORM;
      case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
        return DXGI_FORMAT_BC3_UNORM;
      case GL_RGBA32F_EXT:
      case GL_ALPHA32F_EXT:
      case GL_LUMINANCE_ALPHA32F_EXT:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
      case GL_RGB32F_EXT:
      case GL_LUMINANCE32F_EXT:
        return DXGI_FORMAT_R32G32B32_FLOAT;
      case GL_RGBA16F_EXT:
      case GL_ALPHA16F_EXT:
      case GL_LUMINANCE_ALPHA16F_EXT:
      case GL_RGB16F_EXT:
      case GL_LUMINANCE16F_EXT:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
      case GL_BGRA8_EXT:
        return DXGI_FORMAT_B8G8R8A8_UNORM;
      case GL_R8_EXT:
        return DXGI_FORMAT_R8_UNORM;
      case GL_RG8_EXT:
        return DXGI_FORMAT_R8G8_UNORM;
      case GL_R16F_EXT:
        return DXGI_FORMAT_R16_FLOAT;
      case GL_RG16F_EXT:
        return DXGI_FORMAT_R16G16_FLOAT;
      case GL_DEPTH_COMPONENT16:
        return DXGI_FORMAT_D16_UNORM;
      case GL_DEPTH_COMPONENT32_OES:
      case GL_DEPTH24_STENCIL8_OES:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
      case GL_NONE:
        return DXGI_FORMAT_UNKNOWN;
      default:
        UNREACHABLE();
    }

    return DXGI_FORMAT_R8G8B8A8_UNORM;
}

}

namespace d3d11
{

void SetPositionTexCoordVertex(PositionTexCoordVertex* vertex, float x, float y, float u, float v)
{
    vertex->x = x;
    vertex->y = y;
    vertex->u = u;
    vertex->v = v;
}

void SetPositionDepthColorVertex(PositionDepthColorVertex* vertex, float x, float y, float z,
                                 const gl::Color &color)
{
    vertex->x = x;
    vertex->y = y;
    vertex->z = z;
    vertex->r = color.red;
    vertex->g = color.green;
    vertex->b = color.blue;
    vertex->a = color.alpha;
}

size_t ComputePixelSizeBits(DXGI_FORMAT format)
{
    switch (format)
    {
      case DXGI_FORMAT_R1_UNORM:
        return 1;

      case DXGI_FORMAT_A8_UNORM:
      case DXGI_FORMAT_R8_SINT:
      case DXGI_FORMAT_R8_SNORM:
      case DXGI_FORMAT_R8_TYPELESS:
      case DXGI_FORMAT_R8_UINT:
      case DXGI_FORMAT_R8_UNORM:
        return 8;

      case DXGI_FORMAT_B5G5R5A1_UNORM:
      case DXGI_FORMAT_B5G6R5_UNORM:
      case DXGI_FORMAT_D16_UNORM:
      case DXGI_FORMAT_R16_FLOAT:
      case DXGI_FORMAT_R16_SINT:
      case DXGI_FORMAT_R16_SNORM:
      case DXGI_FORMAT_R16_TYPELESS:
      case DXGI_FORMAT_R16_UINT:
      case DXGI_FORMAT_R16_UNORM:
      case DXGI_FORMAT_R8G8_SINT:
      case DXGI_FORMAT_R8G8_SNORM:
      case DXGI_FORMAT_R8G8_TYPELESS:
      case DXGI_FORMAT_R8G8_UINT:
      case DXGI_FORMAT_R8G8_UNORM:
        return 16;

      case DXGI_FORMAT_B8G8R8X8_TYPELESS:
      case DXGI_FORMAT_B8G8R8X8_UNORM:
      case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      case DXGI_FORMAT_D24_UNORM_S8_UINT:
      case DXGI_FORMAT_D32_FLOAT:
      case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
      case DXGI_FORMAT_G8R8_G8B8_UNORM:
      case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
      case DXGI_FORMAT_R10G10B10A2_TYPELESS:
      case DXGI_FORMAT_R10G10B10A2_UINT:
      case DXGI_FORMAT_R10G10B10A2_UNORM:
      case DXGI_FORMAT_R11G11B10_FLOAT:
      case DXGI_FORMAT_R16G16_FLOAT:
      case DXGI_FORMAT_R16G16_SINT:
      case DXGI_FORMAT_R16G16_SNORM:
      case DXGI_FORMAT_R16G16_TYPELESS:
      case DXGI_FORMAT_R16G16_UINT:
      case DXGI_FORMAT_R16G16_UNORM:
      case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
      case DXGI_FORMAT_R24G8_TYPELESS:
      case DXGI_FORMAT_R32_FLOAT:
      case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
      case DXGI_FORMAT_R32_SINT:
      case DXGI_FORMAT_R32_TYPELESS:
      case DXGI_FORMAT_R32_UINT:
      case DXGI_FORMAT_R8G8_B8G8_UNORM:
      case DXGI_FORMAT_R8G8B8A8_SINT:
      case DXGI_FORMAT_R8G8B8A8_SNORM:
      case DXGI_FORMAT_R8G8B8A8_TYPELESS:
      case DXGI_FORMAT_R8G8B8A8_UINT:
      case DXGI_FORMAT_R8G8B8A8_UNORM:
      case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      case DXGI_FORMAT_B8G8R8A8_TYPELESS:
      case DXGI_FORMAT_B8G8R8A8_UNORM:
      case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
      case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
      case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
      case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        return 32;

      case DXGI_FORMAT_R16G16B16A16_FLOAT:
      case DXGI_FORMAT_R16G16B16A16_SINT:
      case DXGI_FORMAT_R16G16B16A16_SNORM:
      case DXGI_FORMAT_R16G16B16A16_TYPELESS:
      case DXGI_FORMAT_R16G16B16A16_UINT:
      case DXGI_FORMAT_R16G16B16A16_UNORM:
      case DXGI_FORMAT_R32G32_FLOAT:
      case DXGI_FORMAT_R32G32_SINT:
      case DXGI_FORMAT_R32G32_TYPELESS:
      case DXGI_FORMAT_R32G32_UINT:
      case DXGI_FORMAT_R32G8X24_TYPELESS:
        return 64;

      case DXGI_FORMAT_R32G32B32_FLOAT:
      case DXGI_FORMAT_R32G32B32_SINT:
      case DXGI_FORMAT_R32G32B32_TYPELESS:
      case DXGI_FORMAT_R32G32B32_UINT:
        return 96;

      case DXGI_FORMAT_R32G32B32A32_FLOAT:
      case DXGI_FORMAT_R32G32B32A32_SINT:
      case DXGI_FORMAT_R32G32B32A32_TYPELESS:
      case DXGI_FORMAT_R32G32B32A32_UINT:
        return 128;

      case DXGI_FORMAT_BC1_TYPELESS:
      case DXGI_FORMAT_BC1_UNORM:
      case DXGI_FORMAT_BC1_UNORM_SRGB:
      case DXGI_FORMAT_BC4_SNORM:
      case DXGI_FORMAT_BC4_TYPELESS:
      case DXGI_FORMAT_BC4_UNORM:
        return 4;

      case DXGI_FORMAT_BC2_TYPELESS:
      case DXGI_FORMAT_BC2_UNORM:
      case DXGI_FORMAT_BC2_UNORM_SRGB:
      case DXGI_FORMAT_BC3_TYPELESS:
      case DXGI_FORMAT_BC3_UNORM:
      case DXGI_FORMAT_BC3_UNORM_SRGB:
      case DXGI_FORMAT_BC5_SNORM:
      case DXGI_FORMAT_BC5_TYPELESS:
      case DXGI_FORMAT_BC5_UNORM:
      case DXGI_FORMAT_BC6H_SF16:
      case DXGI_FORMAT_BC6H_TYPELESS:
      case DXGI_FORMAT_BC6H_UF16:
      case DXGI_FORMAT_BC7_TYPELESS:
      case DXGI_FORMAT_BC7_UNORM:
      case DXGI_FORMAT_BC7_UNORM_SRGB:
        return 8;

      default:
        return 0;
    }
}

size_t ComputeBlockSizeBits(DXGI_FORMAT format)
{
    switch (format)
    {
      case DXGI_FORMAT_BC1_TYPELESS:
      case DXGI_FORMAT_BC1_UNORM:
      case DXGI_FORMAT_BC1_UNORM_SRGB:
      case DXGI_FORMAT_BC4_SNORM:
      case DXGI_FORMAT_BC4_TYPELESS:
      case DXGI_FORMAT_BC4_UNORM:
      case DXGI_FORMAT_BC2_TYPELESS:
      case DXGI_FORMAT_BC2_UNORM:
      case DXGI_FORMAT_BC2_UNORM_SRGB:
      case DXGI_FORMAT_BC3_TYPELESS:
      case DXGI_FORMAT_BC3_UNORM:
      case DXGI_FORMAT_BC3_UNORM_SRGB:
      case DXGI_FORMAT_BC5_SNORM:
      case DXGI_FORMAT_BC5_TYPELESS:
      case DXGI_FORMAT_BC5_UNORM:
      case DXGI_FORMAT_BC6H_SF16:
      case DXGI_FORMAT_BC6H_TYPELESS:
      case DXGI_FORMAT_BC6H_UF16:
      case DXGI_FORMAT_BC7_TYPELESS:
      case DXGI_FORMAT_BC7_UNORM:
      case DXGI_FORMAT_BC7_UNORM_SRGB:
        return ComputePixelSizeBits(format) * 16;
      default:
        UNREACHABLE();
        return 0;
    }
}

bool IsCompressed(DXGI_FORMAT format)
{
    switch (format)
    {
      case DXGI_FORMAT_BC1_TYPELESS:
      case DXGI_FORMAT_BC1_UNORM:
      case DXGI_FORMAT_BC1_UNORM_SRGB:
      case DXGI_FORMAT_BC4_SNORM:
      case DXGI_FORMAT_BC4_TYPELESS:
      case DXGI_FORMAT_BC4_UNORM:
      case DXGI_FORMAT_BC2_TYPELESS:
      case DXGI_FORMAT_BC2_UNORM:
      case DXGI_FORMAT_BC2_UNORM_SRGB:
      case DXGI_FORMAT_BC3_TYPELESS:
      case DXGI_FORMAT_BC3_UNORM:
      case DXGI_FORMAT_BC3_UNORM_SRGB:
      case DXGI_FORMAT_BC5_SNORM:
      case DXGI_FORMAT_BC5_TYPELESS:
      case DXGI_FORMAT_BC5_UNORM:
      case DXGI_FORMAT_BC6H_SF16:
      case DXGI_FORMAT_BC6H_TYPELESS:
      case DXGI_FORMAT_BC6H_UF16:
      case DXGI_FORMAT_BC7_TYPELESS:
      case DXGI_FORMAT_BC7_UNORM:
      case DXGI_FORMAT_BC7_UNORM_SRGB:
        return true;
      case DXGI_FORMAT_UNKNOWN:
        UNREACHABLE();
        return false;
      default:
        return false;
    }
}

unsigned int GetTextureFormatDimensionAlignment(DXGI_FORMAT format)
{
    switch (format)
    {
      case DXGI_FORMAT_BC1_TYPELESS:
      case DXGI_FORMAT_BC1_UNORM:
      case DXGI_FORMAT_BC1_UNORM_SRGB:
      case DXGI_FORMAT_BC4_SNORM:
      case DXGI_FORMAT_BC4_TYPELESS:
      case DXGI_FORMAT_BC4_UNORM:
      case DXGI_FORMAT_BC2_TYPELESS:
      case DXGI_FORMAT_BC2_UNORM:
      case DXGI_FORMAT_BC2_UNORM_SRGB:
      case DXGI_FORMAT_BC3_TYPELESS:
      case DXGI_FORMAT_BC3_UNORM:
      case DXGI_FORMAT_BC3_UNORM_SRGB:
      case DXGI_FORMAT_BC5_SNORM:
      case DXGI_FORMAT_BC5_TYPELESS:
      case DXGI_FORMAT_BC5_UNORM:
      case DXGI_FORMAT_BC6H_SF16:
      case DXGI_FORMAT_BC6H_TYPELESS:
      case DXGI_FORMAT_BC6H_UF16:
      case DXGI_FORMAT_BC7_TYPELESS:
      case DXGI_FORMAT_BC7_UNORM:
      case DXGI_FORMAT_BC7_UNORM_SRGB:
        return 4;
      case DXGI_FORMAT_UNKNOWN:
        UNREACHABLE();
        return 1;
      default:
        return 1;
    }
}

bool IsDepthStencilFormat(DXGI_FORMAT format)
{
    switch (format)
    {
      case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
      case DXGI_FORMAT_D32_FLOAT:
      case DXGI_FORMAT_D24_UNORM_S8_UINT:
      case DXGI_FORMAT_D16_UNORM:
        return true;
      default:
        return false;
    }
}

DXGI_FORMAT GetDepthTextureFormat(DXGI_FORMAT format)
{
    switch (format)
    {
      case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return DXGI_FORMAT_R32G8X24_TYPELESS;
      case DXGI_FORMAT_D32_FLOAT:            return DXGI_FORMAT_R32_TYPELESS;
      case DXGI_FORMAT_D24_UNORM_S8_UINT:    return DXGI_FORMAT_R24G8_TYPELESS;
      case DXGI_FORMAT_D16_UNORM:            return DXGI_FORMAT_R16_TYPELESS;
      default: UNREACHABLE();                return DXGI_FORMAT_UNKNOWN;
    }
}

DXGI_FORMAT GetDepthShaderResourceFormat(DXGI_FORMAT format)
{
    switch (format)
    {
      case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
      case DXGI_FORMAT_D32_FLOAT:            return DXGI_FORMAT_R32_UINT;
      case DXGI_FORMAT_D24_UNORM_S8_UINT:    return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
      case DXGI_FORMAT_D16_UNORM:            return DXGI_FORMAT_R16_UNORM;
      default: UNREACHABLE();                return DXGI_FORMAT_UNKNOWN;
    }
}

HRESULT SetDebugName(ID3D11DeviceChild *resource, const char *name)
{
#if defined(_DEBUG)
    return resource->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(name), name);
#else
    return S_OK;
#endif
}

}
