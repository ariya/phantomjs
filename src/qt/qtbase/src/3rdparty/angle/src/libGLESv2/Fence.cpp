#include "precompiled.h"
//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Fence.cpp: Implements the gl::Fence class, which supports the GL_NV_fence extension.

#include "libGLESv2/Fence.h"
#include "libGLESv2/renderer/FenceImpl.h"
#include "libGLESv2/renderer/Renderer.h"

namespace gl
{

Fence::Fence(rx::Renderer *renderer)
{
    mFence = renderer->createFence();
}

Fence::~Fence()
{
    delete mFence;
}

GLboolean Fence::isFence()
{
    return mFence->isFence();
}

void Fence::setFence(GLenum condition)
{
    mFence->setFence(condition);
}

GLboolean Fence::testFence()
{
    return mFence->testFence();
}

void Fence::finishFence()
{
    mFence->finishFence();
}

void Fence::getFenceiv(GLenum pname, GLint *params)
{
    mFence->getFenceiv(pname, params);
}

}
