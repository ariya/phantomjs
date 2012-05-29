/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef WebGLFramebuffer_h
#define WebGLFramebuffer_h

#include "WebGLObject.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class WebGLRenderbuffer;
class WebGLTexture;

class WebGLFramebuffer : public WebGLObject {
public:
    virtual ~WebGLFramebuffer() { deleteObject(); }

    static PassRefPtr<WebGLFramebuffer> create(WebGLRenderingContext*);

    void setAttachment(GC3Denum attachment, GC3Denum texTarget, WebGLTexture*, GC3Dint level);
    void setAttachment(GC3Denum attachment, WebGLRenderbuffer*);
    // If an object is attached to the framebuffer, remove it.
    void removeAttachment(WebGLObject*);
    WebGLObject* getAttachment(GC3Denum) const;

    GC3Denum getColorBufferFormat() const;
    GC3Dsizei getWidth() const;
    GC3Dsizei getHeight() const;

    // This should always be called before drawArray, drawElements, clear,
    // readPixels, copyTexImage2D, copyTexSubImage2D if this framebuffer is
    // currently bound.
    // Return false if the framebuffer is incomplete; otherwise initialize
    // the buffers if they haven't been initialized and
    // needToInitializeRenderbuffers is true.
    bool onAccess(bool needToInitializeRenderbuffers);

    // Return false does not mean COMPLETE, might still be INCOMPLETE.
    bool isIncomplete(bool checkInternalFormat) const;

    bool hasEverBeenBound() const { return object() && m_hasEverBeenBound; }

    void setHasEverBeenBound() { m_hasEverBeenBound = true; }

protected:
    WebGLFramebuffer(WebGLRenderingContext*);

    virtual void deleteObjectImpl(Platform3DObject);

private:
    virtual bool isFramebuffer() const { return true; }

    // Return false if framebuffer is incomplete.
    bool initializeRenderbuffers();

    bool isColorAttached() const { return (m_colorAttachment && m_colorAttachment->object()); }
    bool isDepthAttached() const { return (m_depthAttachment && m_depthAttachment->object()); }
    bool isStencilAttached() const { return (m_stencilAttachment && m_stencilAttachment->object()); }
    bool isDepthStencilAttached() const { return (m_depthStencilAttachment && m_depthStencilAttachment->object()); }

    RefPtr<WebGLObject> m_colorAttachment;
    RefPtr<WebGLObject> m_depthAttachment;
    RefPtr<WebGLObject> m_stencilAttachment;
    RefPtr<WebGLObject> m_depthStencilAttachment;

    bool m_hasEverBeenBound;

    GC3Denum m_texTarget;
    GC3Dint m_texLevel;
};

} // namespace WebCore

#endif // WebGLFramebuffer_h
