//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Fence9.h: Defines the rx::FenceNV9 class which implements rx::FenceNVImpl.

#ifndef LIBGLESV2_RENDERER_FENCE9_H_
#define LIBGLESV2_RENDERER_FENCE9_H_

#include "libGLESv2/renderer/FenceImpl.h"

namespace rx
{
class Renderer9;

class FenceNV9 : public FenceNVImpl
{
  public:
    explicit FenceNV9(Renderer9 *renderer);
    virtual ~FenceNV9();

    gl::Error set();
    gl::Error test(bool flushCommandBuffer, GLboolean *outFinished);
    gl::Error finishFence(GLboolean *outFinished);

  private:
    DISALLOW_COPY_AND_ASSIGN(FenceNV9);

    Renderer9 *mRenderer;
    IDirect3DQuery9 *mQuery;
};

}

#endif // LIBGLESV2_RENDERER_FENCE9_H_
