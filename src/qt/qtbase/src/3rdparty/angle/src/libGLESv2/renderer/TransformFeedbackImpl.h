//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TransformFeedbackImpl.h: Defines the abstract rx::TransformFeedbackImpl class.

#ifndef LIBGLESV2_RENDERER_TRANSFORMFEEDBACKIMPL_H_
#define LIBGLESV2_RENDERER_TRANSFORMFEEDBACKIMPL_H_

#include "common/angleutils.h"
#include "libGLESv2/TransformFeedback.h"

namespace rx
{

class TransformFeedbackImpl
{
  public:
    virtual ~TransformFeedbackImpl() { }

    virtual void begin(GLenum primitiveMode) = 0;
    virtual void end() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
};

}

#endif // LIBGLESV2_RENDERER_TRANSFORMFEEDBACKIMPL_H_
