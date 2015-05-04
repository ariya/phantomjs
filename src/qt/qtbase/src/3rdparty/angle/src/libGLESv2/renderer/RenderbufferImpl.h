//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderbufferImpl.h: Defines the abstract class gl::RenderbufferImpl

#ifndef LIBGLESV2_RENDERER_RENDERBUFFERIMPL_H_
#define LIBGLESV2_RENDERER_RENDERBUFFERIMPL_H_

#include "angle_gl.h"

#include "libGLESv2/Error.h"

#include "common/angleutils.h"

namespace rx
{

class RenderbufferImpl
{
  public:
    RenderbufferImpl();
    virtual ~RenderbufferImpl() = 0;

    virtual gl::Error setStorage(GLsizei width, GLsizei height, GLenum internalformat, GLsizei samples) = 0;

    virtual GLsizei getWidth() const = 0;
    virtual GLsizei getHeight() const = 0;
    virtual GLenum getInternalFormat() const = 0;
    virtual GLenum getActualFormat() const = 0;
    virtual GLsizei getSamples() const = 0;

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderbufferImpl);
};

}

#endif   // LIBGLESV2_RENDERER_RENDERBUFFERIMPL_H_
