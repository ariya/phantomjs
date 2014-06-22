/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef WebGLContextObject_h
#define WebGLContextObject_h

#include "WebGLObject.h"

namespace WebCore {

class GraphicsContext3D;
class WebGLRenderingContext;

// WebGLContextObject the base class for objects that are owned by a specific
// WebGLRenderingContext.
class WebGLContextObject : public WebGLObject {
public:
    virtual ~WebGLContextObject();

    WebGLRenderingContext* context() const { return m_context; }

    virtual bool validate(const WebGLContextGroup*, const WebGLRenderingContext* context) const
    {
        return context == m_context;
    }

    void detachContext();

protected:
    WebGLContextObject(WebGLRenderingContext*);

    virtual bool hasGroupOrContext() const
    {
        return m_context;
    }

    virtual GraphicsContext3D* getAGraphicsContext3D() const;

private:
    WebGLRenderingContext* m_context;
};

} // namespace WebCore

#endif // WebGLContextObject_h
