//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FenceImpl.h: Defines the rx::FenceImpl class.

#ifndef LIBGLESV2_RENDERER_FENCEIMPL_H_
#define LIBGLESV2_RENDERER_FENCEIMPL_H_

#include "common/angleutils.h"

namespace rx
{

class FenceImpl
{
  public:
    FenceImpl() : mStatus(GL_FALSE), mCondition(GL_NONE) { };
    virtual ~FenceImpl() { };

    virtual GLboolean isFence() = 0;
    virtual void setFence(GLenum condition) = 0;
    virtual GLboolean testFence() = 0;
    virtual void finishFence() = 0;
    virtual void getFenceiv(GLenum pname, GLint *params) = 0;

  protected:
    void setStatus(GLboolean status) { mStatus = status; }
    GLboolean getStatus() const { return mStatus; }

    void setCondition(GLuint condition) { mCondition = condition; }
    GLuint getCondition() const { return mCondition; }

  private:
    DISALLOW_COPY_AND_ASSIGN(FenceImpl);

    GLboolean mStatus;
    GLenum mCondition;
};

}

#endif // LIBGLESV2_RENDERER_FENCEIMPL_H_
