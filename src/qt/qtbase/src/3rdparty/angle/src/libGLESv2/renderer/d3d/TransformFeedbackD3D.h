//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TransformFeedbackD3D.h: Implements the abstract rx::TransformFeedbackImpl class.

#ifndef LIBGLESV2_RENDERER_D3D_TRANSFORMFEEDBACKD3D_H_
#define LIBGLESV2_RENDERER_D3D_TRANSFORMFEEDBACKD3D_H_

#include "libGLESv2/renderer/TransformFeedbackImpl.h"
#include "libGLESv2/angletypes.h"

namespace rx
{

class TransformFeedbackD3D : public TransformFeedbackImpl
{
  public:
    TransformFeedbackD3D();
    virtual ~TransformFeedbackD3D();

    virtual void begin(GLenum primitiveMode);
    virtual void end();
    virtual void pause();
    virtual void resume();
};

}

#endif // LIBGLESV2_RENDERER_D3D_TRANSFORMFEEDBACKD3D_H_
