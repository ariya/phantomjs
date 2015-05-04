//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "libGLESv2/TransformFeedback.h"
#include "libGLESv2/renderer/TransformFeedbackImpl.h"

namespace gl
{

TransformFeedback::TransformFeedback(rx::TransformFeedbackImpl* impl, GLuint id)
    : RefCountObject(id),
      mTransformFeedback(impl),
      mStarted(GL_FALSE),
      mPrimitiveMode(GL_NONE),
      mPaused(GL_FALSE)
{
    ASSERT(impl != NULL);
}

TransformFeedback::~TransformFeedback()
{
    SafeDelete(mTransformFeedback);
}

void TransformFeedback::start(GLenum primitiveMode)
{
    mStarted = GL_TRUE;
    mPrimitiveMode = primitiveMode;
    mPaused = GL_FALSE;
    mTransformFeedback->begin(primitiveMode);
}

void TransformFeedback::stop()
{
    mStarted = GL_FALSE;
    mPrimitiveMode = GL_NONE;
    mPaused = GL_FALSE;
    mTransformFeedback->end();
}

GLboolean TransformFeedback::isStarted() const
{
    return mStarted;
}

GLenum TransformFeedback::getDrawMode() const
{
    return mPrimitiveMode;
}

void TransformFeedback::pause()
{
    mPaused = GL_TRUE;
    mTransformFeedback->pause();
}

void TransformFeedback::resume()
{
    mPaused = GL_FALSE;
    mTransformFeedback->resume();
}

GLboolean TransformFeedback::isPaused() const
{
    return mPaused;
}

}
