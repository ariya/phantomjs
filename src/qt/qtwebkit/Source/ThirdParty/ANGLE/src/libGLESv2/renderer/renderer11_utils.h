//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// renderer11_utils.h: Conversion functions and other utility routines
// specific to the D3D11 renderer.

#ifndef LIBGLESV2_RENDERER_RENDERER11_UTILS_H
#define LIBGLESV2_RENDERER_RENDERER11_UTILS_H

#include "libGLESv2/angletypes.h"

namespace gl_d3d11
{

D3D11_BLEND ConvertBlendFunc(GLenum glBlend, bool isAlpha);
D3D11_BLEND_OP ConvertBlendOp(GLenum glBlendOp);
UINT8 ConvertColorMask(bool maskRed, bool maskGreen, bool maskBlue, bool maskAlpha);

D3D11_CULL_MODE ConvertCullMode(bool cullEnabled, GLenum cullMode);

D3D11_COMPARISON_FUNC ConvertComparison(GLenum comparison);
D3D11_DEPTH_WRITE_MASK ConvertDepthMask(bool depthWriteEnabled);
UINT8 ConvertStencilMask(GLuint stencilmask);
D3D11_STENCIL_OP ConvertStencilOp(GLenum stencilOp);

D3D11_FILTER ConvertFilter(GLenum minFilter, GLenum magFilter, float maxAnisotropy);
D3D11_TEXTURE_ADDRESS_MODE ConvertTextureWrap(GLenum wrap);
FLOAT ConvertMinLOD(GLenum minFilter, unsigned int lodOffset);
FLOAT ConvertMaxLOD(GLenum minFilter, unsigned int lodOffset);

DXGI_FORMAT ConvertRenderbufferFormat(GLenum format);
DXGI_FORMAT ConvertTextureFormat(GLenum format);
}

namespace d3d11_gl
{

GLenum ConvertBackBufferFormat(DXGI_FORMAT format);
GLenum ConvertDepthStencilFormat(DXGI_FORMAT format);
GLenum ConvertRenderbufferFormat(DXGI_FORMAT format);
GLenum ConvertTextureInternalFormat(DXGI_FORMAT format);

}

namespace d3d11
{

struct PositionTexCoordVertex
{
    float x, y;
    float u, v;
};
void SetPositionTexCoordVertex(PositionTexCoordVertex* vertex, float x, float y, float u, float v);

struct PositionDepthColorVertex
{
    float x, y, z;
    float r, g, b, a;
};
void SetPositionDepthColorVertex(PositionDepthColorVertex* vertex, float x, float y, float z,
                                 const gl::Color &color);

size_t ComputePixelSizeBits(DXGI_FORMAT format);
size_t ComputeBlockSizeBits(DXGI_FORMAT format);

bool IsCompressed(DXGI_FORMAT format);
unsigned int GetTextureFormatDimensionAlignment(DXGI_FORMAT format);

bool IsDepthStencilFormat(DXGI_FORMAT format);
DXGI_FORMAT GetDepthTextureFormat(DXGI_FORMAT format);
DXGI_FORMAT GetDepthShaderResourceFormat(DXGI_FORMAT format);

HRESULT SetDebugName(ID3D11DeviceChild *resource, const char *name);

inline bool isDeviceLostError(HRESULT errorCode)
{
    switch (errorCode)
    {
      case DXGI_ERROR_DEVICE_HUNG:
      case DXGI_ERROR_DEVICE_REMOVED:
      case DXGI_ERROR_DEVICE_RESET:
      case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
      case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
        return true;
      default:
        return false;
    }
}

}

#endif // LIBGLESV2_RENDERER_RENDERER11_UTILS_H
