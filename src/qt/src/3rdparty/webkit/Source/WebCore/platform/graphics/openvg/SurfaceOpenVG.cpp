/*
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "SurfaceOpenVG.h"

#include "IntSize.h"
#include "PainterOpenVG.h"

#if PLATFORM(EGL)
#include "EGLDisplayOpenVG.h"
#include "EGLUtils.h"
#endif

#include <wtf/Assertions.h>

namespace WebCore {

PainterOpenVG* SurfaceOpenVG::s_currentPainter = 0;

SurfaceOpenVG* SurfaceOpenVG::currentSurface()
{
#if PLATFORM(EGL)
    return EGLDisplayOpenVG::currentSurface();
#else
    ASSERT_NOT_REACHED();
    return 0;
#endif
}

#if PLATFORM(EGL)
SurfaceOpenVG::SurfaceOpenVG(const IntSize& size, const EGLDisplay& display, EGLConfig* confPtr, EGLint* errorCode)
    : m_activePainter(0)
    , m_eglDisplay(display)
    , m_eglSurface(EGL_NO_SURFACE)
    , m_eglContext(EGL_NO_CONTEXT)
{
    ASSERT(m_eglDisplay != EGL_NO_DISPLAY);

    EGLDisplayOpenVG* displayManager = EGLDisplayOpenVG::forDisplay(m_eglDisplay);
    EGLConfig config = confPtr ? (*confPtr) : displayManager->defaultPbufferConfig();
    m_eglSurface = displayManager->createPbufferSurface(size, config, errorCode);

    if (m_eglSurface == EGL_NO_SURFACE)
        return;

    m_eglContext = displayManager->contextForSurface(m_eglSurface);
    EGLDisplayOpenVG::registerPlatformSurface(this);
}

SurfaceOpenVG::SurfaceOpenVG(EGLClientBuffer buffer, EGLenum bufferType, const EGLDisplay& display, EGLConfig* confPtr, EGLint* errorCode)
    : m_activePainter(0)
    , m_eglDisplay(display)
    , m_eglSurface(EGL_NO_SURFACE)
    , m_eglContext(EGL_NO_CONTEXT)
{
    ASSERT(m_eglDisplay != EGL_NO_DISPLAY);

    EGLDisplayOpenVG* displayManager = EGLDisplayOpenVG::forDisplay(m_eglDisplay);
    EGLConfig config = confPtr ? (*confPtr) : displayManager->defaultPbufferConfig();
    m_eglSurface = displayManager->createPbufferFromClientBuffer(buffer, bufferType, config, errorCode);

    if (m_eglSurface == EGL_NO_SURFACE)
        return;

    m_eglContext = displayManager->contextForSurface(m_eglSurface);
    EGLDisplayOpenVG::registerPlatformSurface(this);
}

SurfaceOpenVG::SurfaceOpenVG(EGLNativeWindowType window, const EGLDisplay& display, EGLConfig* confPtr)
    : m_activePainter(0)
    , m_eglDisplay(display)
    , m_eglSurface(EGL_NO_SURFACE)
    , m_eglContext(EGL_NO_CONTEXT)
{
    ASSERT(m_eglDisplay != EGL_NO_DISPLAY);

    EGLDisplayOpenVG* displayManager = EGLDisplayOpenVG::forDisplay(m_eglDisplay);
    EGLConfig config = confPtr ? (*confPtr) : displayManager->defaultWindowConfig();
    m_eglSurface = displayManager->surfaceForWindow(window, config);
    ASSERT(m_eglSurface != EGL_NO_SURFACE);

    m_eglContext = displayManager->contextForSurface(m_eglSurface);
    EGLDisplayOpenVG::registerPlatformSurface(this);
}

// Constructor only accessible to EGLDisplayOpenVG for shared context
// initialization. The parameter types might define to void* like in the
// window surface constructor, so it can't be overloaded with all the required
// arguments and EGLDisplayOpenVG basically implements the constructor
// by itself.
SurfaceOpenVG::SurfaceOpenVG()
    : m_activePainter(0)
    , m_eglDisplay(EGL_NO_DISPLAY)
    , m_eglSurface(EGL_NO_SURFACE)
    , m_eglContext(EGL_NO_CONTEXT)
{
}
#endif

SurfaceOpenVG::~SurfaceOpenVG()
{
    if (!isValid())
        return;

    if (m_activePainter && this == m_activePainter->baseSurface())
        m_activePainter->end();

#if PLATFORM(EGL)
    EGLDisplayOpenVG::forDisplay(m_eglDisplay)->destroySurface(m_eglSurface);
    EGLDisplayOpenVG::unregisterPlatformSurface(this);
#else
    ASSERT_NOT_REACHED();
#endif
}

bool SurfaceOpenVG::isValid() const
{
#if PLATFORM(EGL)
    return (m_eglSurface != EGL_NO_SURFACE);
#else
    ASSERT_NOT_REACHED();
    return false;
#endif
}

int SurfaceOpenVG::width() const
{
#if PLATFORM(EGL)
    ASSERT(m_eglSurface != EGL_NO_SURFACE);

    EGLint width;
    eglQuerySurface(m_eglDisplay, m_eglSurface, EGL_WIDTH, &width);
    ASSERT_EGL_NO_ERROR();
    return width;
#else
    ASSERT_NOT_REACHED();
    return 0;
#endif
}

int SurfaceOpenVG::height() const
{
#if PLATFORM(EGL)
    ASSERT(m_eglSurface != EGL_NO_SURFACE);

    EGLint height;
    eglQuerySurface(m_eglDisplay, m_eglSurface, EGL_HEIGHT, &height);
    ASSERT_EGL_NO_ERROR();
    return height;
#else
    ASSERT_NOT_REACHED();
    return 0;
#endif
}

SurfaceOpenVG* SurfaceOpenVG::sharedSurface() const
{
#if PLATFORM(EGL)
    ASSERT(m_eglSurface != EGL_NO_SURFACE);
    return EGLDisplayOpenVG::forDisplay(m_eglDisplay)->sharedPlatformSurface();
#else
    ASSERT_NOT_REACHED();
    return 0;
#endif
}

void SurfaceOpenVG::makeCurrent(MakeCurrentMode mode)
{
#if PLATFORM(EGL)
    ASSERT(m_eglSurface != EGL_NO_SURFACE);

    eglBindAPI(EGL_OPENVG_API);
    ASSERT_EGL_NO_ERROR();
    EGLSurface currentSurface = eglGetCurrentSurface(EGL_DRAW);
    ASSERT_EGL_NO_ERROR();

    if (currentSurface != m_eglSurface) {
        // Save other context before switching over.
        if (s_currentPainter && mode != DontSaveOrApplyPainterState
            && s_currentPainter->surface()->m_eglSurface == currentSurface)
            s_currentPainter->save(PainterOpenVG::KeepCurrentState);

        eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);
        ASSERT_EGL_NO_ERROR();
        s_currentPainter = 0;
    }
#endif

    if (m_activePainter && mode == ApplyPainterStateOnSurfaceSwitch
        && s_currentPainter != m_activePainter) {
        m_activePainter->applyState();
        s_currentPainter = m_activePainter;
    }
}

void SurfaceOpenVG::makeCompatibleCurrent()
{
#if PLATFORM(EGL)
    ASSERT(m_eglSurface != EGL_NO_SURFACE);

    eglBindAPI(EGL_OPENVG_API);
    ASSERT_EGL_NO_ERROR();
    EGLSurface currentSurface = eglGetCurrentSurface(EGL_DRAW);
    ASSERT_EGL_NO_ERROR();

    if (currentSurface == m_eglSurface) {
        if (m_activePainter && s_currentPainter != m_activePainter) {
            m_activePainter->applyState();
            s_currentPainter = m_activePainter;
        }
    } else if (!EGLDisplayOpenVG::forDisplay(m_eglDisplay)->surfacesCompatible(currentSurface, m_eglSurface)) {
        // Save other context before switching over.
        if (s_currentPainter && s_currentPainter->surface()->m_eglSurface == currentSurface)
            s_currentPainter->save(PainterOpenVG::KeepCurrentState);

        eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);
        ASSERT_EGL_NO_ERROR();
        s_currentPainter = 0;
    }
    // else: surfaces compatible, no need to switch contexts
#endif
}

void SurfaceOpenVG::flush()
{
#if PLATFORM(EGL)
    ASSERT(m_eglSurface != EGL_NO_SURFACE);

    eglSwapBuffers(m_eglDisplay, m_eglSurface);
    ASSERT_EGL_NO_ERROR();
#endif
}

void SurfaceOpenVG::setActivePainter(PainterOpenVG* painter)
{
    ASSERT(isValid());

    // If painter is non-zero, we want to make sure there was no previous painter set.
    ASSERT(!painter || !m_activePainter);

    // Make sure a disabled painter isn't marked as global current painter anymore.
    if (!painter && s_currentPainter == m_activePainter)
        s_currentPainter = 0;

    m_activePainter = painter;
}

PainterOpenVG* SurfaceOpenVG::activePainter()
{
    ASSERT(isValid());
    return m_activePainter;
}

}
