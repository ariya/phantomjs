//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Blit9.cpp: Surface copy utility class.

#ifndef LIBGLESV2_BLIT9_H_
#define LIBGLESV2_BLIT9_H_

#include "common/angleutils.h"
#include "libGLESv2/Error.h"

#include <GLES2/gl2.h>

namespace gl
{
class Framebuffer;
}

namespace rx
{
class Renderer9;
class TextureStorage;

class Blit9
{
  public:
    explicit Blit9(Renderer9 *renderer);
    ~Blit9();

    gl::Error initialize();

    // Copy from source surface to dest surface.
    // sourceRect, xoffset, yoffset are in D3D coordinates (0,0 in upper-left)
    gl::Error copy2D(gl::Framebuffer *framebuffer, const RECT &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, TextureStorage *storage, GLint level);
    gl::Error copyCube(gl::Framebuffer *framebuffer, const RECT &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, TextureStorage *storage, GLenum target, GLint level);

    // Copy from source surface to dest surface.
    // sourceRect, xoffset, yoffset are in D3D coordinates (0,0 in upper-left)
    // source is interpreted as RGBA and destFormat specifies the desired result format. For example, if destFormat = GL_RGB, the alpha channel will be forced to 0.
    gl::Error formatConvert(IDirect3DSurface9 *source, const RECT &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, IDirect3DSurface9 *dest);

    // 2x2 box filter sample from source to dest.
    // Requires that source is RGB(A) and dest has the same format as source.
    gl::Error boxFilter(IDirect3DSurface9 *source, IDirect3DSurface9 *dest);

  private:
    Renderer9 *mRenderer;

    bool mGeometryLoaded;
    IDirect3DVertexBuffer9 *mQuadVertexBuffer;
    IDirect3DVertexDeclaration9 *mQuadVertexDeclaration;

    gl::Error setFormatConvertShaders(GLenum destFormat);

    gl::Error copy(IDirect3DSurface9 *source, const RECT &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, IDirect3DSurface9 *dest);
    gl::Error copySurfaceToTexture(IDirect3DSurface9 *surface, const RECT &sourceRect, IDirect3DTexture9 **outTexture);
    void setViewport(const RECT &sourceRect, GLint xoffset, GLint yoffset);
    void setCommonBlitState();
    RECT getSurfaceRect(IDirect3DSurface9 *surface) const;

    // This enum is used to index mCompiledShaders and mShaderSource.
    enum ShaderId
    {
        SHADER_VS_STANDARD,
        SHADER_VS_FLIPY,
        SHADER_PS_PASSTHROUGH,
        SHADER_PS_LUMINANCE,
        SHADER_PS_COMPONENTMASK,
        SHADER_COUNT
    };

    // This actually contains IDirect3DVertexShader9 or IDirect3DPixelShader9 casted to IUnknown.
    IUnknown *mCompiledShaders[SHADER_COUNT];

    template <class D3DShaderType>
    gl::Error setShader(ShaderId source, const char *profile,
                        gl::Error (Renderer9::*createShader)(const DWORD *, size_t length, D3DShaderType **outShader),
                        HRESULT (WINAPI IDirect3DDevice9::*setShader)(D3DShaderType*));

    gl::Error setVertexShader(ShaderId shader);
    gl::Error setPixelShader(ShaderId shader);
    void render();

    void saveState();
    void restoreState();
    IDirect3DStateBlock9 *mSavedStateBlock;
    IDirect3DSurface9 *mSavedRenderTarget;
    IDirect3DSurface9 *mSavedDepthStencil;

    DISALLOW_COPY_AND_ASSIGN(Blit9);
};
}

#endif   // LIBGLESV2_BLIT9_H_
