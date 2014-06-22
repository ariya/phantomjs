//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Fence.h: Defines the gl::Fence class, which supports the GL_NV_fence extension.

#ifndef LIBGLESV2_FENCE_H_
#define LIBGLESV2_FENCE_H_

#include "common/angleutils.h"

namespace rx
{
class Renderer;
class FenceImpl;
}

namespace gl
{

class Fence
{
  public:
    explicit Fence(rx::Renderer *renderer);
    virtual ~Fence();

    GLboolean isFence();
    void setFence(GLenum condition);
    GLboolean testFence();
    void finishFence();
    void getFenceiv(GLenum pname, GLint *params);

  private:
    DISALLOW_COPY_AND_ASSIGN(Fence);

    rx::FenceImpl *mFence;
};

}

#endif   // LIBGLESV2_FENCE_H_
