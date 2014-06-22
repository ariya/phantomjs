//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// renderer9_utils.h: Conversion functions and other utility routines 
// specific to the D3D9 renderer

#ifndef LIBGLESV2_RENDERER_RENDERER9_UTILS_H
#define LIBGLESV2_RENDERER_RENDERER9_UTILS_H

#include "libGLESv2/utilities.h"

const D3DFORMAT D3DFMT_INTZ = ((D3DFORMAT)(MAKEFOURCC('I','N','T','Z')));
const D3DFORMAT D3DFMT_NULL = ((D3DFORMAT)(MAKEFOURCC('N','U','L','L')));

namespace gl_d3d9
{

D3DCMPFUNC ConvertComparison(GLenum comparison);
D3DCOLOR ConvertColor(gl::Color color);
D3DBLEND ConvertBlendFunc(GLenum blend);
D3DBLENDOP ConvertBlendOp(GLenum blendOp);
D3DSTENCILOP ConvertStencilOp(GLenum stencilOp);
D3DTEXTUREADDRESS ConvertTextureWrap(GLenum wrap);
D3DCULL ConvertCullMode(GLenum cullFace, GLenum frontFace);
D3DCUBEMAP_FACES ConvertCubeFace(GLenum cubeFace);
DWORD ConvertColorMask(bool red, bool green, bool blue, bool alpha);
D3DTEXTUREFILTERTYPE ConvertMagFilter(GLenum magFilter, float maxAnisotropy);
void ConvertMinFilter(GLenum minFilter, D3DTEXTUREFILTERTYPE *d3dMinFilter, D3DTEXTUREFILTERTYPE *d3dMipFilter, float maxAnisotropy);
D3DFORMAT ConvertRenderbufferFormat(GLenum format);
D3DMULTISAMPLE_TYPE GetMultisampleTypeFromSamples(GLsizei samples);

}

namespace d3d9_gl
{

GLuint GetAlphaSize(D3DFORMAT colorFormat);
GLuint GetStencilSize(D3DFORMAT stencilFormat);

GLsizei GetSamplesFromMultisampleType(D3DMULTISAMPLE_TYPE type);

bool IsFormatChannelEquivalent(D3DFORMAT d3dformat, GLenum format);
GLenum ConvertBackBufferFormat(D3DFORMAT format);
GLenum ConvertDepthStencilFormat(D3DFORMAT format);
GLenum ConvertRenderTargetFormat(D3DFORMAT format);
GLenum GetEquivalentFormat(D3DFORMAT format);

}

namespace d3d9
{
bool IsCompressedFormat(D3DFORMAT format);
size_t ComputeRowSize(D3DFORMAT format, unsigned int width);

inline bool isDeviceLostError(HRESULT errorCode)
{
    switch (errorCode)
    {
      case D3DERR_DRIVERINTERNALERROR:
      case D3DERR_DEVICELOST:
      case D3DERR_DEVICEHUNG:
      case D3DERR_DEVICEREMOVED:
        return true;
      default:
        return false;
    }
}

}

#endif // LIBGLESV2_RENDERER_RENDERER9_UTILS_H
