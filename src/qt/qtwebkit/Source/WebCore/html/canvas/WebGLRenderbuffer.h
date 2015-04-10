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

#ifndef WebGLRenderbuffer_h
#define WebGLRenderbuffer_h

#include "WebGLSharedObject.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class WebGLRenderbuffer : public WebGLSharedObject {
public:
    virtual ~WebGLRenderbuffer();

    static PassRefPtr<WebGLRenderbuffer> create(WebGLRenderingContext*);

    void setInternalFormat(GC3Denum internalformat)
    {
        m_internalFormat = internalformat;
        m_initialized = false;
    }
    GC3Denum getInternalFormat() const { return m_internalFormat; }

    void setSize(GC3Dsizei width, GC3Dsizei height)
    {
        m_width = width;
        m_height = height;
    }
    GC3Dsizei getWidth() const { return m_width; }
    GC3Dsizei getHeight() const { return m_height; }

    void setIsValid(bool isValid) { m_isValid = isValid; }
    bool isValid() const { return m_isValid; }

    bool isInitialized() const { return m_initialized; }
    void setInitialized() { m_initialized = true; }

    bool hasEverBeenBound() const { return object() && m_hasEverBeenBound; }

    void setHasEverBeenBound() { m_hasEverBeenBound = true; }

protected:
    WebGLRenderbuffer(WebGLRenderingContext*);

    virtual void deleteObjectImpl(GraphicsContext3D*, Platform3DObject);

private:
    virtual bool isRenderbuffer() const { return true; }

    GC3Denum m_internalFormat;
    bool m_initialized;
    GC3Dsizei m_width, m_height;
    bool m_isValid; // This is only false if internalFormat is DEPTH_STENCIL and packed_depth_stencil is not supported.

    bool m_hasEverBeenBound;
};

} // namespace WebCore

#endif // WebGLRenderbuffer_h
