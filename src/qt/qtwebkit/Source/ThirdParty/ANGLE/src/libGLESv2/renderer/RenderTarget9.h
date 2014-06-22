//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderTarget9.h: Defines a D3D9-specific wrapper for IDirect3DSurface9 pointers
// retained by Renderbuffers.

#ifndef LIBGLESV2_RENDERER_RENDERTARGET9_H_
#define LIBGLESV2_RENDERER_RENDERTARGET9_H_

#include "libGLESv2/renderer/RenderTarget.h"

namespace rx
{
class Renderer;
class Renderer9;

class RenderTarget9 : public RenderTarget
{
  public:
    RenderTarget9(Renderer *renderer, IDirect3DSurface9 *surface);
    RenderTarget9(Renderer *renderer, GLsizei width, GLsizei height, GLenum format, GLsizei samples);
    virtual ~RenderTarget9();

    static RenderTarget9 *makeRenderTarget9(RenderTarget *renderTarget);
    IDirect3DSurface9 *getSurface();

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderTarget9);

    IDirect3DSurface9 *mRenderTarget;

    Renderer9 *mRenderer;
};

}

#endif // LIBGLESV2_RENDERER_RENDERTARGET9_H_
