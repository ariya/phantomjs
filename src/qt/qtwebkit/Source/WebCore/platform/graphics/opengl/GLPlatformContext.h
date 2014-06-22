/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GLPlatformContext_h
#define GLPlatformContext_h

#if USE(ACCELERATED_COMPOSITING)

#include "GLDefs.h"
#include "GLPlatformSurface.h"
#include "GraphicsContext3D.h"
#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>

// Encapsulates an OpenGL context, hiding platform specific management.
namespace WebCore {

class GLPlatformContext {
    WTF_MAKE_NONCOPYABLE(GLPlatformContext);

public:
    // From http://www.khronos.org/registry/gles/extensions/EXT/EXT_robustness.txt
    enum PlatformContextReset {
        PLATFORMCONTEXT_NO_ERROR = 0x0000,
        PLATFORMCONTEXT_GUILTY_CONTEXT_RESET = 0x8253,
        PLATFORMCONTEXT_INNOCENT_CONTEXT_RESET = 0x8254,
        PLATFORMCONTEXT_UNKNOWN_CONTEXT_RESET = 0x8255,
    };

    static PassOwnPtr<GLPlatformContext> createContext(GraphicsContext3D::RenderStyle);

    static bool supportsGLExtension(const String&);

#if USE(EGL)
    static bool supportsEGLExtension(EGLDisplay, const String&);
#endif

#if USE(GLX)
    static bool supportsGLXExtension(Display*, const String&);
#endif

    virtual ~GLPlatformContext();

    virtual bool initialize(GLPlatformSurface*, PlatformContext = 0);

    // Makes this and surface as current context and drawable.
    // Calling this function with no surface is same as calling releaseCurrent.
    // Does nothing if this is already current Context.
    bool makeCurrent(GLPlatformSurface* = 0);

    // Sets Current Context and Drawable as Null.
    // Doesn't have any effect if this is not the current Context.
    void releaseCurrent();

    virtual PlatformContext handle() const;

    virtual bool isCurrentContext() const;

    bool isValid() const;

    // Destroys any GL resources associated with this context.
    virtual void destroy();

    static GLPlatformContext* getCurrent();

protected:
    GLPlatformContext();
    virtual bool platformMakeCurrent(GLPlatformSurface*);
    virtual void platformReleaseCurrent();
    PlatformContext m_contextHandle;
    bool m_resetLostContext;
    bool m_contextLost;
};

} // namespace WebCore

#endif

#endif // GLNativeContext_H
