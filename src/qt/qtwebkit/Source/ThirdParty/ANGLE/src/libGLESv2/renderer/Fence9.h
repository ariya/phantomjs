//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Fence9.h: Defines the rx::Fence9 class which implements rx::FenceImpl.

#ifndef LIBGLESV2_RENDERER_FENCE9_H_
#define LIBGLESV2_RENDERER_FENCE9_H_

#include "libGLESv2/renderer/FenceImpl.h"

namespace rx
{
class Renderer9;

class Fence9 : public FenceImpl
{
  public:
    explicit Fence9(rx::Renderer9 *renderer);
    virtual ~Fence9();

    GLboolean isFence();
    void setFence(GLenum condition);
    GLboolean testFence();
    void finishFence();
    void getFenceiv(GLenum pname, GLint *params);

  private:
    DISALLOW_COPY_AND_ASSIGN(Fence9);

    rx::Renderer9 *mRenderer;
    IDirect3DQuery9 *mQuery;
};

}

#endif // LIBGLESV2_RENDERER_FENCE9_H_
