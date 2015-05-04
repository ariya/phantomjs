//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// renderer9_utils.h: Conversion functions and other utility routines
// specific to the D3D9 renderer

#ifndef LIBGLESV2_RENDERER_RENDERER9_UTILS_H
#define LIBGLESV2_RENDERER_RENDERER9_UTILS_H

#include "libGLESv2/angletypes.h"
#include "libGLESv2/Caps.h"
#include "libGLESv2/Error.h"

namespace gl
{
class FramebufferAttachment;
}

namespace rx
{
class RenderTarget9;
struct Workarounds;

namespace gl_d3d9
{

D3DCMPFUNC ConvertComparison(GLenum comparison);
D3DCOLOR ConvertColor(gl::ColorF color);
D3DBLEND ConvertBlendFunc(GLenum blend);
D3DBLENDOP ConvertBlendOp(GLenum blendOp);
D3DSTENCILOP ConvertStencilOp(GLenum stencilOp);
D3DTEXTUREADDRESS ConvertTextureWrap(GLenum wrap);
D3DCULL ConvertCullMode(GLenum cullFace, GLenum frontFace);
D3DCUBEMAP_FACES ConvertCubeFace(GLenum cubeFace);
DWORD ConvertColorMask(bool red, bool green, bool blue, bool alpha);
D3DTEXTUREFILTERTYPE ConvertMagFilter(GLenum magFilter, float maxAnisotropy);
void ConvertMinFilter(GLenum minFilter, D3DTEXTUREFILTERTYPE *d3dMinFilter, D3DTEXTUREFILTERTYPE *d3dMipFilter, float maxAnisotropy);

D3DMULTISAMPLE_TYPE GetMultisampleType(GLuint samples);

}

namespace d3d9_gl
{

GLsizei GetSamplesCount(D3DMULTISAMPLE_TYPE type);

bool IsFormatChannelEquivalent(D3DFORMAT d3dformat, GLenum format);

void GenerateCaps(IDirect3D9 *d3d9, IDirect3DDevice9 *device, D3DDEVTYPE deviceType, UINT adapter, gl::Caps *caps,
                  gl::TextureCapsMap *textureCapsMap, gl::Extensions *extensions);

}

namespace d3d9
{

GLuint ComputeBlockSize(D3DFORMAT format, GLuint width, GLuint height);

void MakeValidSize(bool isImage, D3DFORMAT format, GLsizei *requestWidth, GLsizei *requestHeight, int *levelOffset);

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

gl::Error GetAttachmentRenderTarget(gl::FramebufferAttachment *attachment, RenderTarget9 **outRT);
Workarounds GenerateWorkarounds();

}

}

#endif // LIBGLESV2_RENDERER_RENDERER9_UTILS_H
