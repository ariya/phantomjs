//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderTarget.h: Defines an abstract wrapper class to manage IDirect3DSurface9
// and ID3D11View objects belonging to renderbuffers.

#ifndef LIBGLESV2_RENDERER_RENDERTARGET_H_
#define LIBGLESV2_RENDERER_RENDERTARGET_H_

#include "common/angleutils.h"
#include "libGLESv2/angletypes.h"

namespace rx
{
class RenderTarget
{
  public:
    RenderTarget();
    virtual ~RenderTarget();

    virtual GLsizei getWidth() const = 0;
    virtual GLsizei getHeight() const = 0;
    virtual GLsizei getDepth() const = 0;
    virtual GLenum getInternalFormat() const = 0;
    virtual GLenum getActualFormat() const = 0;
    virtual GLsizei getSamples() const = 0;
    gl::Extents getExtents() const { return gl::Extents(getWidth(), getHeight(), getDepth()); }

    virtual void invalidate(GLint x, GLint y, GLsizei width, GLsizei height) = 0;

    virtual unsigned int getSerial() const;
    static unsigned int issueSerials(unsigned int count);

    struct Desc {
        GLsizei width;
        GLsizei height;
        GLsizei depth;
        GLenum  format;
    };

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderTarget);

    const unsigned int mSerial;
    static unsigned int mCurrentSerial;
};

}

#endif // LIBGLESV2_RENDERTARGET_H_
