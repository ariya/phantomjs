//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Fence.cpp: Implements the gl::FenceNV and gl::FenceSync classes, which support the GL_NV_fence
// extension and GLES3 sync objects.

#include "libGLESv2/Fence.h"
#include "libGLESv2/renderer/FenceImpl.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/main.h"
#include "common/utilities.h"

#include "angle_gl.h"

namespace gl
{

FenceNV::FenceNV(rx::FenceNVImpl *impl)
    : mFence(impl),
      mIsSet(false),
      mStatus(GL_FALSE),
      mCondition(GL_NONE)
{
}

FenceNV::~FenceNV()
{
    SafeDelete(mFence);
}

GLboolean FenceNV::isFence() const
{
    // GL_NV_fence spec:
    // A name returned by GenFencesNV, but not yet set via SetFenceNV, is not the name of an existing fence.
    return (mIsSet ? GL_TRUE : GL_FALSE);
}

Error FenceNV::setFence(GLenum condition)
{
    Error error = mFence->set();
    if (error.isError())
    {
        return error;
    }

    mCondition = condition;
    mStatus = GL_FALSE;
    mIsSet = true;

    return Error(GL_NO_ERROR);
}

Error FenceNV::testFence(GLboolean *outResult)
{
    // Flush the command buffer by default
    Error error = mFence->test(true, &mStatus);
    if (error.isError())
    {
        return error;
    }

    *outResult = mStatus;
    return Error(GL_NO_ERROR);
}

Error FenceNV::finishFence()
{
    ASSERT(mIsSet);

    return mFence->finishFence(&mStatus);
}

FenceSync::FenceSync(rx::FenceSyncImpl *impl, GLuint id)
    : RefCountObject(id),
      mFence(impl),
      mCondition(GL_NONE)
{
}

FenceSync::~FenceSync()
{
    SafeDelete(mFence);
}

Error FenceSync::set(GLenum condition)
{
    Error error = mFence->set();
    if (error.isError())
    {
        return error;
    }

    mCondition = condition;
    return Error(GL_NO_ERROR);
}

Error FenceSync::clientWait(GLbitfield flags, GLuint64 timeout, GLenum *outResult)
{
    ASSERT(mCondition != GL_NONE);
    return mFence->clientWait(flags, timeout, outResult);
}

Error FenceSync::serverWait(GLbitfield flags, GLuint64 timeout)
{
    return mFence->serverWait(flags, timeout);
}

Error FenceSync::getStatus(GLint *outResult) const
{
    return mFence->getStatus(outResult);
}

}
