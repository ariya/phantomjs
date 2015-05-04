//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderbuffer.h: Defines the renderer-agnostic container class gl::Renderbuffer.
// Implements GL renderbuffer objects and related functionality.
// [OpenGL ES 2.0.24] section 4.4.3 page 108.

#ifndef LIBGLESV2_RENDERBUFFER_H_
#define LIBGLESV2_RENDERBUFFER_H_

#include "angle_gl.h"

#include "libGLESv2/Error.h"

#include "common/angleutils.h"
#include "common/RefCountObject.h"

namespace rx
{
class RenderbufferImpl;
}

namespace gl
{
class FramebufferAttachment;

// A GL renderbuffer object is usually used as a depth or stencil buffer attachment
// for a framebuffer object. The renderbuffer itself is a distinct GL object, see
// FramebufferAttachment and Framebuffer for how they are applied to an FBO via an
// attachment point.

class Renderbuffer : public RefCountObject
{
  public:
    Renderbuffer(rx::RenderbufferImpl *impl, GLuint id);
    virtual ~Renderbuffer();

    Error setStorage(GLsizei width, GLsizei height, GLenum internalformat, GLsizei samples);

    rx::RenderbufferImpl *getImplementation();

    GLsizei getWidth() const;
    GLsizei getHeight() const;
    GLenum getInternalFormat() const;
    GLenum getActualFormat() const;
    GLsizei getSamples() const;
    GLuint getRedSize() const;
    GLuint getGreenSize() const;
    GLuint getBlueSize() const;
    GLuint getAlphaSize() const;
    GLuint getDepthSize() const;
    GLuint getStencilSize() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Renderbuffer);

    rx::RenderbufferImpl *mRenderbuffer;

    GLsizei mWidth;
    GLsizei mHeight;
    GLenum mInternalFormat;
    GLenum mActualFormat;
    GLsizei mSamples;
};

}

#endif   // LIBGLESV2_RENDERBUFFER_H_
