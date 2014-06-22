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

#include "config.h"
#include "GLPlatformContext.h"

#if USE(ACCELERATED_COMPOSITING)

#if USE(GLX)
#include "GLXContext.h"
#elif USE(EGL)
#include "EGLContext.h"
#endif

#include "NotImplemented.h"

namespace WebCore {

#if USE(OPENGL_ES_2)
static PFNGLGETGRAPHICSRESETSTATUSEXTPROC glGetGraphicsResetStatus = 0;
#else
static PFNGLGETGRAPHICSRESETSTATUSARBPROC glGetGraphicsResetStatus = 0;
#endif
static GLPlatformContext* m_currentContext = 0;

class GLCurrentContextWrapper : public GLPlatformContext {

public:
    GLCurrentContextWrapper()
        : GLPlatformContext()
    {
#if USE(GLX)
        m_contextHandle = glXGetCurrentContext();
#elif USE(EGL)
        m_contextHandle = eglGetCurrentContext();
#endif
        if (m_contextHandle)
            m_currentContext = this;
    }

    virtual ~GLCurrentContextWrapper() { }
};

static PassOwnPtr<GLPlatformContext> createOffScreenContext()
{
#if USE(GLX)
    return adoptPtr(new GLXOffScreenContext());
#elif USE(EGL)
    return adoptPtr(new EGLOffScreenContext());
#else
    return nullptr;
#endif
}

static HashSet<String> parseExtensions(const String& extensionsString)
{
    Vector<String> extNames;
    extensionsString.split(" ", extNames);
    HashSet<String> splitExtNames;
    unsigned size = extNames.size();
    for (unsigned i = 0; i < size; ++i)
        splitExtNames.add(extNames[i]);
    extNames.clear();

    return splitExtNames;
}

static void resolveResetStatusExtension()
{
    static bool resolvedRobustnessExtension = false;
    if (!resolvedRobustnessExtension) {
        resolvedRobustnessExtension = true;
#if USE(OPENGL_ES_2)
        glGetGraphicsResetStatus = reinterpret_cast<PFNGLGETGRAPHICSRESETSTATUSEXTPROC>(eglGetProcAddress("glGetGraphicsResetStatusEXT"));
#elif USE(EGL)
        glGetGraphicsResetStatus = reinterpret_cast<PFNGLGETGRAPHICSRESETSTATUSARBPROC>(eglGetProcAddress("glGetGraphicsResetStatusARB"));
#elif USE(GLX)
        glGetGraphicsResetStatus = reinterpret_cast<PFNGLGETGRAPHICSRESETSTATUSARBPROC>(glXGetProcAddressARB(reinterpret_cast<const GLubyte*>("glGetGraphicsResetStatusARB")));
#endif
    }
}

PassOwnPtr<GLPlatformContext> GLPlatformContext::createContext(GraphicsContext3D::RenderStyle renderStyle)
{
#if !USE(OPENGL_ES_2)
    if (!initializeOpenGLShims())
        return nullptr;
#endif

    switch (renderStyle) {
    case GraphicsContext3D::RenderOffscreen:
        if (OwnPtr<GLPlatformContext> context = createOffScreenContext())
            return context.release();
        break;
    case GraphicsContext3D::RenderToCurrentGLContext:
        if (OwnPtr<GLPlatformContext> context = adoptPtr(new GLCurrentContextWrapper()))
            return context.release();
        break;
    case GraphicsContext3D::RenderDirectlyToHostWindow:
        ASSERT_NOT_REACHED();
        break;
    }

    return nullptr;
}

bool GLPlatformContext::supportsGLExtension(const String& name)
{
    static HashSet<String> supportedExtensions;

    if (!supportedExtensions.size()) {
        String rawExtensions = reinterpret_cast<const char*>(::glGetString(GL_EXTENSIONS));
        supportedExtensions = parseExtensions(rawExtensions);
    }

    if (supportedExtensions.contains(name))
        return true;

    return false;
}

#if USE(EGL)
bool GLPlatformContext::supportsEGLExtension(EGLDisplay display, const String& name)
{
    static HashSet<String> supportedExtensions;

    if (!supportedExtensions.size()) {
        if (display == EGL_NO_DISPLAY)
            return false;

        String rawExtensions = reinterpret_cast<const char*>(eglQueryString(display, EGL_EXTENSIONS));
        supportedExtensions = parseExtensions(rawExtensions);
    }

    if (supportedExtensions.contains(name))
        return true;

    return false;
}
#endif

#if USE(GLX)
bool GLPlatformContext::supportsGLXExtension(Display* display, const String& name)
{
    static HashSet<String> supportedExtensions;

    if (!supportedExtensions.size()) {
        if (!display)
            return false;

        String rawExtensions = glXQueryExtensionsString(display, DefaultScreen(display));
        supportedExtensions = parseExtensions(rawExtensions);
    }

    if (supportedExtensions.contains(name))
        return true;

    return false;
}
#endif

GLPlatformContext::GLPlatformContext()
    : m_contextHandle(0)
    , m_resetLostContext(false)
{
}

GLPlatformContext::~GLPlatformContext()
{
    if (this == m_currentContext)
        m_currentContext = 0;
}

bool GLPlatformContext::makeCurrent(GLPlatformSurface* surface)
{
    m_contextLost = false;

    if (m_currentContext == this && (!surface || surface->isCurrentDrawable()))
        return true;

    m_currentContext = 0;

    if (!surface || (surface && !surface->drawable()))
        platformReleaseCurrent();
    else if (platformMakeCurrent(surface)) {
        m_currentContext = this;
        surface->onMakeCurrent();
    }

    if (m_resetLostContext) {
        resolveResetStatusExtension();

        if (glGetGraphicsResetStatus) {
            GLenum status = glGetGraphicsResetStatus();

            switch (status) {
            case PLATFORMCONTEXT_NO_ERROR:
                break;
            case PLATFORMCONTEXT_GUILTY_CONTEXT_RESET:
                m_contextLost = true;
                break;
            case PLATFORMCONTEXT_INNOCENT_CONTEXT_RESET:
                break;
            case PLATFORMCONTEXT_UNKNOWN_CONTEXT_RESET:
                m_contextLost = true;
                break;
            default:
                break;
            }
        }
    }

    return m_currentContext;
}

bool GLPlatformContext::isValid() const
{
    return !m_contextLost;
}

void GLPlatformContext::releaseCurrent()
{
    if (this == m_currentContext) {
        m_currentContext = 0;
        platformReleaseCurrent();
    }
}

PlatformContext GLPlatformContext::handle() const
{
    return m_contextHandle;
}

bool GLPlatformContext::isCurrentContext() const
{
    return true;
}

bool GLPlatformContext::initialize(GLPlatformSurface*, PlatformContext)
{
    return true;
}

GLPlatformContext* GLPlatformContext::getCurrent()
{
    return m_currentContext;
}

bool GLPlatformContext::platformMakeCurrent(GLPlatformSurface*)
{
    return true;
}

void GLPlatformContext::platformReleaseCurrent()
{
    notImplemented();
}

void GLPlatformContext::destroy()
{
    m_contextHandle = 0;
    m_resetLostContext = false;
}

} // namespace WebCore

#endif
