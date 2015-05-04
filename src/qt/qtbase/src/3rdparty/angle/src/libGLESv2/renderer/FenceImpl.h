//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FenceImpl.h: Defines the rx::FenceNVImpl and rx::FenceSyncImpl classes.

#ifndef LIBGLESV2_RENDERER_FENCEIMPL_H_
#define LIBGLESV2_RENDERER_FENCEIMPL_H_

#include "libGLESv2/Error.h"

#include "common/angleutils.h"

#include "angle_gl.h"

namespace rx
{

class FenceNVImpl
{
  public:
    FenceNVImpl() { };
    virtual ~FenceNVImpl() { };

    virtual gl::Error set() = 0;
    virtual gl::Error test(bool flushCommandBuffer, GLboolean *outFinished) = 0;
    virtual gl::Error finishFence(GLboolean *outFinished) = 0;

  private:
    DISALLOW_COPY_AND_ASSIGN(FenceNVImpl);
};

class FenceSyncImpl
{
  public:
    FenceSyncImpl() { };
    virtual ~FenceSyncImpl() { };

    virtual gl::Error set() = 0;
    virtual gl::Error clientWait(GLbitfield flags, GLuint64 timeout, GLenum *outResult) = 0;
    virtual gl::Error serverWait(GLbitfield flags, GLuint64 timeout) = 0;
    virtual gl::Error getStatus(GLint *outResult) = 0;

  private:
    DISALLOW_COPY_AND_ASSIGN(FenceSyncImpl);
};

}

#endif // LIBGLESV2_RENDERER_FENCEIMPL_H_
