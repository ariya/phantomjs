//
// Copyright (c) 2012-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// renderer11_utils.h: Conversion functions and other utility routines
// specific to the D3D11 renderer.

#ifndef LIBGLESV2_RENDERER_RENDERER11_UTILS_H
#define LIBGLESV2_RENDERER_RENDERER11_UTILS_H

#include "libGLESv2/angletypes.h"
#include "libGLESv2/Caps.h"
#include "libGLESv2/Error.h"

#include <vector>

namespace gl
{
class FramebufferAttachment;
}

namespace rx
{
class RenderTarget11;
struct Workarounds;

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

D3D11_FILTER ConvertFilter(GLenum minFilter, GLenum magFilter, float maxAnisotropy, GLenum comparisonMode);
D3D11_TEXTURE_ADDRESS_MODE ConvertTextureWrap(GLenum wrap);

D3D11_QUERY ConvertQueryType(GLenum queryType);

}

namespace d3d11_gl
{

void GenerateCaps(ID3D11Device *device, gl::Caps *caps, gl::TextureCapsMap *textureCapsMap, gl::Extensions *extensions);

}

namespace d3d11
{

void MakeValidSize(bool isImage, DXGI_FORMAT format, GLsizei *requestWidth, GLsizei *requestHeight, int *levelOffset);

void GenerateInitialTextureData(GLint internalFormat, GLuint width, GLuint height, GLuint depth,
                                GLuint mipLevels, std::vector<D3D11_SUBRESOURCE_DATA> *outSubresourceData,
                                std::vector< std::vector<BYTE> > *outData);

struct PositionTexCoordVertex
{
    float x, y;
    float u, v;
};
void SetPositionTexCoordVertex(PositionTexCoordVertex* vertex, float x, float y, float u, float v);

struct PositionLayerTexCoord3DVertex
{
    float x, y;
    unsigned int l;
    float u, v, s;
};
void SetPositionLayerTexCoord3DVertex(PositionLayerTexCoord3DVertex* vertex, float x, float y,
                                      unsigned int layer, float u, float v, float s);

template <typename T>
struct PositionDepthColorVertex
{
    float x, y, z;
    T r, g, b, a;
};

template <typename T>
void SetPositionDepthColorVertex(PositionDepthColorVertex<T>* vertex, float x, float y, float z,
                                 const gl::Color<T> &color)
{
    vertex->x = x;
    vertex->y = y;
    vertex->z = z;
    vertex->r = color.red;
    vertex->g = color.green;
    vertex->b = color.blue;
    vertex->a = color.alpha;
}

HRESULT SetDebugName(ID3D11DeviceChild *resource, const char *name);

template <typename outType>
outType* DynamicCastComObject(IUnknown* object)
{
    outType *outObject = NULL;
    HRESULT result = object->QueryInterface(__uuidof(outType), reinterpret_cast<void**>(&outObject));
    if (SUCCEEDED(result))
    {
        return outObject;
    }
    else
    {
        SafeRelease(outObject);
        return NULL;
    }
}

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

template <unsigned int N>
inline ID3D11VertexShader *CompileVS(ID3D11Device *device, const BYTE (&byteCode)[N], const char *name)
{
    ID3D11VertexShader *vs = NULL;
    HRESULT result = device->CreateVertexShader(byteCode, N, NULL, &vs);
    UNUSED_ASSERTION_VARIABLE(result);
    ASSERT(SUCCEEDED(result));
    SetDebugName(vs, name);
    return vs;
}

template <unsigned int N>
inline ID3D11GeometryShader *CompileGS(ID3D11Device *device, const BYTE (&byteCode)[N], const char *name)
{
    ID3D11GeometryShader *gs = NULL;
    HRESULT result = device->CreateGeometryShader(byteCode, N, NULL, &gs);
    UNUSED_ASSERTION_VARIABLE(result);
    ASSERT(SUCCEEDED(result));
    SetDebugName(gs, name);
    return gs;
}

template <unsigned int N>
inline ID3D11PixelShader *CompilePS(ID3D11Device *device, const BYTE (&byteCode)[N], const char *name)
{
    ID3D11PixelShader *ps = NULL;
    HRESULT result = device->CreatePixelShader(byteCode, N, NULL, &ps);
    UNUSED_ASSERTION_VARIABLE(result);
    ASSERT(SUCCEEDED(result));
    SetDebugName(ps, name);
    return ps;
}

// Copy data to small D3D11 buffers, such as for small constant buffers, which use one struct to
// represent an entire buffer.
template <class T>
inline void SetBufferData(ID3D11DeviceContext *context, ID3D11Buffer *constantBuffer, const T &value)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

    memcpy(mappedResource.pData, &value, sizeof(T));

    context->Unmap(constantBuffer, 0);
}

gl::Error GetAttachmentRenderTarget(gl::FramebufferAttachment *attachment, RenderTarget11 **outRT);

Workarounds GenerateWorkarounds();

}

}

#endif // LIBGLESV2_RENDERER_RENDERER11_UTILS_H
