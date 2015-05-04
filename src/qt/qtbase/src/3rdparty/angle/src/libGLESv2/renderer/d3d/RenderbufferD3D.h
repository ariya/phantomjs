//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderbufferD3d.h: Defines the RenderbufferD3D class which implements RenderbufferImpl.

#ifndef LIBGLESV2_RENDERER_RENDERBUFFERD3D_H_
#define LIBGLESV2_RENDERER_RENDERBUFFERD3D_H_

#include "angle_gl.h"

#include "common/angleutils.h"
#include "libGLESv2/renderer/RenderbufferImpl.h"

namespace rx
{
class RendererD3D;
class RenderTarget;
class SwapChain;

class RenderbufferD3D : public RenderbufferImpl
{
  public:
    RenderbufferD3D(RendererD3D *renderer);
    virtual ~RenderbufferD3D();

    static RenderbufferD3D *makeRenderbufferD3D(RenderbufferImpl *renderbuffer);

    virtual gl::Error setStorage(GLsizei width, GLsizei height, GLenum internalformat, GLsizei samples) override;
    gl::Error setStorage(SwapChain *swapChain, bool depth);

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    RenderTarget *getRenderTarget();
    unsigned int getRenderTargetSerial() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderbufferD3D);

    RendererD3D *mRenderer;
    RenderTarget *mRenderTarget;
};
}

#endif // LIBGLESV2_RENDERER_RENDERBUFFERD3D_H_
