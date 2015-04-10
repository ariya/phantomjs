//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Fence11.h: Defines the rx::Fence11 class which implements rx::FenceImpl.

#ifndef LIBGLESV2_RENDERER_Fence11_H_
#define LIBGLESV2_RENDERER_Fence11_H_

#include "libGLESv2/renderer/FenceImpl.h"

namespace rx
{
class Renderer11;

class Fence11 : public FenceImpl
{
  public:
    explicit Fence11(rx::Renderer11 *renderer);
    virtual ~Fence11();

    GLboolean isFence();
    void setFence(GLenum condition);
    GLboolean testFence();
    void finishFence();
    void getFenceiv(GLenum pname, GLint *params);

  private:
    DISALLOW_COPY_AND_ASSIGN(Fence11);

    rx::Renderer11 *mRenderer;
    ID3D11Query *mQuery;
};

}

#endif // LIBGLESV2_RENDERER_FENCE11_H_
