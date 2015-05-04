//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderbufferD3d.cpp: Implements the RenderbufferD3D class, a specialization of RenderbufferImpl


#include "libGLESv2/renderer/d3d/RenderbufferD3D.h"

#include "libGLESv2/renderer/d3d/RendererD3D.h"
#include "libGLESv2/renderer/RenderTarget.h"

namespace rx
{
RenderbufferD3D::RenderbufferD3D(RendererD3D *renderer) : mRenderer(renderer)
{
    mRenderTarget = NULL;
}

RenderbufferD3D::~RenderbufferD3D()
{
    SafeDelete(mRenderTarget);
}

RenderbufferD3D *RenderbufferD3D::makeRenderbufferD3D(RenderbufferImpl *renderbuffer)
{
    ASSERT(HAS_DYNAMIC_TYPE(RenderbufferD3D*, renderbuffer));
    return static_cast<RenderbufferD3D*>(renderbuffer);
}

gl::Error RenderbufferD3D::setStorage(GLsizei width, GLsizei height, GLenum internalformat, GLsizei samples)
{
    // If the renderbuffer parameters are queried, the calling function
    // will expect one of the valid renderbuffer formats for use in
    // glRenderbufferStorage, but we should create depth and stencil buffers
    // as DEPTH24_STENCIL8
    GLenum creationFormat = internalformat;
    if (internalformat == GL_DEPTH_COMPONENT16 || internalformat == GL_STENCIL_INDEX8)
    {
        creationFormat = GL_DEPTH24_STENCIL8_OES;
    }

    RenderTarget *newRT = NULL;
    gl::Error error = mRenderer->createRenderTarget(width, height, creationFormat, samples, &newRT);
    if (error.isError())
    {
        return error;
    }

    SafeDelete(mRenderTarget);
    mRenderTarget = newRT;

    return gl::Error(GL_NO_ERROR);
}

gl::Error RenderbufferD3D::setStorage(SwapChain *swapChain, bool depth)
{
    RenderTarget *newRT = NULL;
    gl::Error error = mRenderer->createRenderTarget(swapChain, depth, &newRT);
    if (error.isError())
    {
        return error;
    }

    SafeDelete(mRenderTarget);
    mRenderTarget = newRT;

    return gl::Error(GL_NO_ERROR);
}

GLsizei RenderbufferD3D::getWidth() const
{
    return (mRenderTarget ? mRenderTarget->getWidth() : 0);
}

GLsizei RenderbufferD3D::getHeight() const
{
    return (mRenderTarget ? mRenderTarget->getHeight() : 0);
}

GLenum RenderbufferD3D::getInternalFormat() const
{
    return (mRenderTarget ? mRenderTarget->getInternalFormat() : GL_RGBA4);
}

GLenum RenderbufferD3D::getActualFormat() const
{
    return (mRenderTarget ? mRenderTarget->getActualFormat() : GL_RGBA4);
}

GLsizei RenderbufferD3D::getSamples() const
{
    return (mRenderTarget ? mRenderTarget->getSamples() : 0);
}

RenderTarget *RenderbufferD3D::getRenderTarget()
{
    return mRenderTarget;
}

unsigned int RenderbufferD3D::getRenderTargetSerial() const
{
    return (mRenderTarget ? mRenderTarget->getSerial() : 0);
}

}
