#include "precompiled.h"
//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Framebuffer.cpp: Implements the gl::Framebuffer class. Implements GL framebuffer
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4 page 105.

#include "libGLESv2/Framebuffer.h"

#include "libGLESv2/main.h"
#include "libGLESv2/utilities.h"
#include "libGLESv2/Texture.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/Renderbuffer.h"

namespace gl
{

Framebuffer::Framebuffer(rx::Renderer *renderer)
    : mRenderer(renderer)
{
    for (unsigned int colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        mColorbufferTypes[colorAttachment] = GL_NONE;
        mDrawBufferStates[colorAttachment] = GL_NONE;
    }
    mDrawBufferStates[0] = GL_COLOR_ATTACHMENT0_EXT;
    mReadBufferState = GL_COLOR_ATTACHMENT0_EXT;

    mDepthbufferType = GL_NONE;
    mStencilbufferType = GL_NONE;
}

Framebuffer::~Framebuffer()
{
    for (unsigned int colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        mColorbufferPointers[colorAttachment].set(NULL);
    }
    mDepthbufferPointer.set(NULL);
    mStencilbufferPointer.set(NULL);
}

Renderbuffer *Framebuffer::lookupRenderbuffer(GLenum type, GLuint handle) const
{
    gl::Context *context = gl::getContext();
    Renderbuffer *buffer = NULL;

    if (type == GL_NONE)
    {
        buffer = NULL;
    }
    else if (type == GL_RENDERBUFFER)
    {
        buffer = context->getRenderbuffer(handle);
    }
    else if (IsInternalTextureTarget(type))
    {
        buffer = context->getTexture(handle)->getRenderbuffer(type);
    }
    else
    {
        UNREACHABLE();
    }

    return buffer;
}

void Framebuffer::setColorbuffer(unsigned int colorAttachment, GLenum type, GLuint colorbuffer)
{
    ASSERT(colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS);
    mColorbufferTypes[colorAttachment] = (colorbuffer != 0) ? type : GL_NONE;
    mColorbufferPointers[colorAttachment].set(lookupRenderbuffer(type, colorbuffer));
}

void Framebuffer::setDepthbuffer(GLenum type, GLuint depthbuffer)
{
    mDepthbufferType = (depthbuffer != 0) ? type : GL_NONE;
    mDepthbufferPointer.set(lookupRenderbuffer(type, depthbuffer));
}

void Framebuffer::setStencilbuffer(GLenum type, GLuint stencilbuffer)
{
    mStencilbufferType = (stencilbuffer != 0) ? type : GL_NONE;
    mStencilbufferPointer.set(lookupRenderbuffer(type, stencilbuffer));
}

void Framebuffer::detachTexture(GLuint texture)
{
    for (unsigned int colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        if (mColorbufferPointers[colorAttachment].id() == texture && IsInternalTextureTarget(mColorbufferTypes[colorAttachment]))
        {
            mColorbufferTypes[colorAttachment] = GL_NONE;
            mColorbufferPointers[colorAttachment].set(NULL);
        }
    }

    if (mDepthbufferPointer.id() == texture && IsInternalTextureTarget(mDepthbufferType))
    {
        mDepthbufferType = GL_NONE;
        mDepthbufferPointer.set(NULL);
    }

    if (mStencilbufferPointer.id() == texture && IsInternalTextureTarget(mStencilbufferType))
    {
        mStencilbufferType = GL_NONE;
        mStencilbufferPointer.set(NULL);
    }
}

void Framebuffer::detachRenderbuffer(GLuint renderbuffer)
{
    for (unsigned int colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        if (mColorbufferPointers[colorAttachment].id() == renderbuffer && mColorbufferTypes[colorAttachment] == GL_RENDERBUFFER)
        {
            mColorbufferTypes[colorAttachment] = GL_NONE;
            mColorbufferPointers[colorAttachment].set(NULL);
        }
    }

    if (mDepthbufferPointer.id() == renderbuffer && mDepthbufferType == GL_RENDERBUFFER)
    {
        mDepthbufferType = GL_NONE;
        mDepthbufferPointer.set(NULL);
    }

    if (mStencilbufferPointer.id() == renderbuffer && mStencilbufferType == GL_RENDERBUFFER)
    {
        mStencilbufferType = GL_NONE;
        mStencilbufferPointer.set(NULL);
    }
}

unsigned int Framebuffer::getRenderTargetSerial(unsigned int colorAttachment) const
{
    ASSERT(colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS);

    Renderbuffer *colorbuffer = mColorbufferPointers[colorAttachment].get();

    if (colorbuffer)
    {
        return colorbuffer->getSerial();
    }

    return 0;
}

unsigned int Framebuffer::getDepthbufferSerial() const
{
    Renderbuffer *depthbuffer = mDepthbufferPointer.get();

    if (depthbuffer)
    {
        return depthbuffer->getSerial();
    }

    return 0;
}

unsigned int Framebuffer::getStencilbufferSerial() const
{
    Renderbuffer *stencilbuffer = mStencilbufferPointer.get();

    if (stencilbuffer)
    {
        return stencilbuffer->getSerial();
    }

    return 0;
}

Renderbuffer *Framebuffer::getColorbuffer(unsigned int colorAttachment) const
{
    ASSERT(colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS);
    return mColorbufferPointers[colorAttachment].get();
}

Renderbuffer *Framebuffer::getDepthbuffer() const
{
    return mDepthbufferPointer.get();
}

Renderbuffer *Framebuffer::getStencilbuffer() const
{
    return mStencilbufferPointer.get();
}

Renderbuffer *Framebuffer::getDepthOrStencilbuffer() const
{
    Renderbuffer *depthstencilbuffer = mDepthbufferPointer.get();
    
    if (!depthstencilbuffer)
    {
        depthstencilbuffer = mStencilbufferPointer.get();
    }

    return depthstencilbuffer;
}

Renderbuffer *Framebuffer::getReadColorbuffer() const
{
    // Will require more logic if glReadBuffers is supported
    return mColorbufferPointers[0].get();
}

GLenum Framebuffer::getReadColorbufferType() const
{
    // Will require more logic if glReadBuffers is supported
    return mColorbufferTypes[0];
}

Renderbuffer *Framebuffer::getFirstColorbuffer() const
{
    for (unsigned int colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        if (mColorbufferTypes[colorAttachment] != GL_NONE)
        {
            return mColorbufferPointers[colorAttachment].get();
        }
    }

    return NULL;
}

GLenum Framebuffer::getColorbufferType(unsigned int colorAttachment) const
{
    ASSERT(colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS);
    return mColorbufferTypes[colorAttachment];
}

GLenum Framebuffer::getDepthbufferType() const
{
    return mDepthbufferType;
}

GLenum Framebuffer::getStencilbufferType() const
{
    return mStencilbufferType;
}

GLuint Framebuffer::getColorbufferHandle(unsigned int colorAttachment) const
{
    ASSERT(colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS);
    return mColorbufferPointers[colorAttachment].id();
}

GLuint Framebuffer::getDepthbufferHandle() const
{
    return mDepthbufferPointer.id();
}

GLuint Framebuffer::getStencilbufferHandle() const
{
    return mStencilbufferPointer.id();
}

GLenum Framebuffer::getDrawBufferState(unsigned int colorAttachment) const
{
    return mDrawBufferStates[colorAttachment];
}

void Framebuffer::setDrawBufferState(unsigned int colorAttachment, GLenum drawBuffer)
{
    mDrawBufferStates[colorAttachment] = drawBuffer;
}

bool Framebuffer::isEnabledColorAttachment(unsigned int colorAttachment) const
{
    return (mColorbufferTypes[colorAttachment] != GL_NONE && mDrawBufferStates[colorAttachment] != GL_NONE);
}

bool Framebuffer::hasEnabledColorAttachment() const
{
    for (unsigned int colorAttachment = 0; colorAttachment < gl::IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        if (isEnabledColorAttachment(colorAttachment))
        {
            return true;
        }
    }

    return false;
}

bool Framebuffer::hasStencil() const
{
    if (mStencilbufferType != GL_NONE)
    {
        const Renderbuffer *stencilbufferObject = getStencilbuffer();

        if (stencilbufferObject)
        {
            return stencilbufferObject->getStencilSize() > 0;
        }
    }

    return false;
}

bool Framebuffer::usingExtendedDrawBuffers() const
{
    for (unsigned int colorAttachment = 1; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        if (isEnabledColorAttachment(colorAttachment))
        {
            return true;
        }
    }

    return false;
}

GLenum Framebuffer::completeness() const
{
    int width = 0;
    int height = 0;
    int colorbufferSize = 0;
    int samples = -1;
    bool missingAttachment = true;

    for (unsigned int colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        if (mColorbufferTypes[colorAttachment] != GL_NONE)
        {
            const Renderbuffer *colorbuffer = getColorbuffer(colorAttachment);

            if (!colorbuffer)
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }

            if (colorbuffer->getWidth() == 0 || colorbuffer->getHeight() == 0)
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }

            if (mColorbufferTypes[colorAttachment] == GL_RENDERBUFFER)
            {
                if (!gl::IsColorRenderable(colorbuffer->getInternalFormat()))
                {
                    return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
                }
            }
            else if (IsInternalTextureTarget(mColorbufferTypes[colorAttachment]))
            {
                GLint internalformat = colorbuffer->getInternalFormat();
                GLenum format = gl::ExtractFormat(internalformat);

                if (IsCompressed(format) ||
                    format == GL_ALPHA ||
                    format == GL_LUMINANCE ||
                    format == GL_LUMINANCE_ALPHA)
                {
                    return GL_FRAMEBUFFER_UNSUPPORTED;
                }

                bool filtering, renderable;

                if ((gl::IsFloat32Format(internalformat) && !mRenderer->getFloat32TextureSupport(&filtering, &renderable)) ||
                    (gl::IsFloat16Format(internalformat) && !mRenderer->getFloat16TextureSupport(&filtering, &renderable)))
                {
                    return GL_FRAMEBUFFER_UNSUPPORTED;
                }

                if (gl::IsDepthTexture(internalformat) || gl::IsStencilTexture(internalformat))
                {
                    return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
                }
            }
            else
            {
                UNREACHABLE();
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }

            if (!missingAttachment)
            {
                // all color attachments must have the same width and height
                if (colorbuffer->getWidth() != width || colorbuffer->getHeight() != height)
                {
                    return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
                }

                // APPLE_framebuffer_multisample, which EXT_draw_buffers refers to, requires that
                // all color attachments have the same number of samples for the FBO to be complete.
                if (colorbuffer->getSamples() != samples)
                {
                    return GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT;
                }

                // all color attachments attachments must have the same number of bitplanes
                if (gl::ComputePixelSize(colorbuffer->getInternalFormat()) != colorbufferSize)
                {
                    return GL_FRAMEBUFFER_UNSUPPORTED;
                }

                // D3D11 does not allow for overlapping RenderTargetViews, so ensure uniqueness
                for (unsigned int previousColorAttachment = 0; previousColorAttachment < colorAttachment; previousColorAttachment++)
                {
                    if (mColorbufferPointers[colorAttachment].get() == mColorbufferPointers[previousColorAttachment].get())
                    {
                        return GL_FRAMEBUFFER_UNSUPPORTED;
                    }
                }
            }
            else
            {
                width = colorbuffer->getWidth();
                height = colorbuffer->getHeight();
                samples = colorbuffer->getSamples();
                colorbufferSize = gl::ComputePixelSize(colorbuffer->getInternalFormat());
                missingAttachment = false;
            }
        }
    }

    const Renderbuffer *depthbuffer = NULL;
    const Renderbuffer *stencilbuffer = NULL;

    if (mDepthbufferType != GL_NONE)
    {
        depthbuffer = getDepthbuffer();

        if (!depthbuffer)
        {
            return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        }

        if (depthbuffer->getWidth() == 0 || depthbuffer->getHeight() == 0)
        {
            return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        }

        if (mDepthbufferType == GL_RENDERBUFFER)
        {
            if (!gl::IsDepthRenderable(depthbuffer->getInternalFormat()))
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
        }
        else if (IsInternalTextureTarget(mDepthbufferType))
        {
            GLint internalformat = depthbuffer->getInternalFormat();

            // depth texture attachments require OES/ANGLE_depth_texture
            if (!mRenderer->getDepthTextureSupport())
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }

            if (!gl::IsDepthTexture(internalformat))
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
        }
        else
        {
            UNREACHABLE();
            return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        }

        if (missingAttachment)
        {
            width = depthbuffer->getWidth();
            height = depthbuffer->getHeight();
            samples = depthbuffer->getSamples();
            missingAttachment = false;
        }
        else if (width != depthbuffer->getWidth() || height != depthbuffer->getHeight())
        {
            return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
        }
        else if (samples != depthbuffer->getSamples())
        {
            return GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_ANGLE;
        }
    }

    if (mStencilbufferType != GL_NONE)
    {
        stencilbuffer = getStencilbuffer();

        if (!stencilbuffer)
        {
            return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        }

        if (stencilbuffer->getWidth() == 0 || stencilbuffer->getHeight() == 0)
        {
            return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        }

        if (mStencilbufferType == GL_RENDERBUFFER)
        {
            if (!gl::IsStencilRenderable(stencilbuffer->getInternalFormat()))
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
        }
        else if (IsInternalTextureTarget(mStencilbufferType))
        {
            GLint internalformat = stencilbuffer->getInternalFormat();

            // texture stencil attachments come along as part
            // of OES_packed_depth_stencil + OES/ANGLE_depth_texture
            if (!mRenderer->getDepthTextureSupport())
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }

            if (!gl::IsStencilTexture(internalformat))
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
        }
        else
        {
            UNREACHABLE();
            return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        }

        if (missingAttachment)
        {
            width = stencilbuffer->getWidth();
            height = stencilbuffer->getHeight();
            samples = stencilbuffer->getSamples();
            missingAttachment = false;
        }
        else if (width != stencilbuffer->getWidth() || height != stencilbuffer->getHeight())
        {
            return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
        }
        else if (samples != stencilbuffer->getSamples())
        {
            return GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_ANGLE;
        }
    }

    // if we have both a depth and stencil buffer, they must refer to the same object
    // since we only support packed_depth_stencil and not separate depth and stencil
    if (depthbuffer && stencilbuffer && (depthbuffer != stencilbuffer))
    {
        return GL_FRAMEBUFFER_UNSUPPORTED;
    }

    // we need to have at least one attachment to be complete
    if (missingAttachment)
    {
        return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
    }

    return GL_FRAMEBUFFER_COMPLETE;
}

DefaultFramebuffer::DefaultFramebuffer(rx::Renderer *renderer, Colorbuffer *colorbuffer, DepthStencilbuffer *depthStencil)
    : Framebuffer(renderer)
{
    mColorbufferPointers[0].set(new Renderbuffer(mRenderer, 0, colorbuffer));

    Renderbuffer *depthStencilRenderbuffer = new Renderbuffer(mRenderer, 0, depthStencil);
    mDepthbufferPointer.set(depthStencilRenderbuffer);
    mStencilbufferPointer.set(depthStencilRenderbuffer);

    mColorbufferTypes[0] = GL_RENDERBUFFER;
    mDepthbufferType = (depthStencilRenderbuffer->getDepthSize() != 0) ? GL_RENDERBUFFER : GL_NONE;
    mStencilbufferType = (depthStencilRenderbuffer->getStencilSize() != 0) ? GL_RENDERBUFFER : GL_NONE;

    mDrawBufferStates[0] = GL_BACK;
    mReadBufferState = GL_BACK;
}

int Framebuffer::getSamples() const
{
    if (completeness() == GL_FRAMEBUFFER_COMPLETE)
    {
        // for a complete framebuffer, all attachments must have the same sample count
        // in this case return the first nonzero sample size
        for (unsigned int colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
        {
            if (mColorbufferTypes[colorAttachment] != GL_NONE)
            {
                return getColorbuffer(colorAttachment)->getSamples();
            }
        }
    }

    return 0;
}

GLenum DefaultFramebuffer::completeness() const
{
    // The default framebuffer *must* always be complete, though it may not be
    // subject to the same rules as application FBOs. ie, it could have 0x0 size.
    return GL_FRAMEBUFFER_COMPLETE;
}

}
