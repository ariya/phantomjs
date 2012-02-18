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
#include "EGLDisplayOpenVG.h"

#include "EGLUtils.h"
#include "IntSize.h"
#include "SurfaceOpenVG.h"

#include <wtf/Assertions.h>
#include <wtf/StdLibExtras.h>

namespace WebCore {

// Need to typedef this, otherwise DEFINE_STATIC_LOCAL() doesn't swallow it.
typedef HashMap<EGLDisplay, EGLDisplayOpenVG*> EGLDisplayManagerMap;

// File-static variables.
static EGLDisplayManagerMap& displayManagers()
{
    DEFINE_STATIC_LOCAL(EGLDisplayManagerMap, managers, ());
    return managers;
}

static EGLDisplayOpenVG* s_current = 0;

// Static class members.

SurfaceOpenVG* EGLDisplayOpenVG::currentSurface()
{
    EGLDisplayManagerMap& managers = displayManagers();
    EGLDisplay currentDisplay = eglGetCurrentDisplay();

    if (currentDisplay == EGL_NO_DISPLAY || !managers.contains(currentDisplay))
        return 0;

    EGLDisplayOpenVG* displayManager = managers.get(currentDisplay);
    EGLSurface currentSurface = eglGetCurrentSurface(EGL_DRAW);

    if (currentSurface == EGL_NO_SURFACE || !displayManager->m_platformSurfaces.contains(currentSurface))
        return 0;

    return displayManager->m_platformSurfaces.get(currentSurface);
}

void EGLDisplayOpenVG::registerPlatformSurface(SurfaceOpenVG* platformSurface)
{
    EGLDisplayOpenVG* displayManager = EGLDisplayOpenVG::forDisplay(platformSurface->eglDisplay());
    displayManager->m_platformSurfaces.set(platformSurface->eglSurface(), platformSurface);
}

void EGLDisplayOpenVG::unregisterPlatformSurface(SurfaceOpenVG* platformSurface)
{
    EGLDisplayOpenVG* displayManager = EGLDisplayOpenVG::forDisplay(platformSurface->eglDisplay());
    displayManager->m_platformSurfaces.remove(platformSurface->eglSurface());
}

void EGLDisplayOpenVG::setCurrentDisplay(const EGLDisplay& display)
{
    s_current = EGLDisplayOpenVG::forDisplay(display);
}

EGLDisplayOpenVG* EGLDisplayOpenVG::current()
{
    if (!s_current) {
        EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        eglInitialize(display, 0, 0);
        ASSERT_EGL_NO_ERROR();

        s_current = EGLDisplayOpenVG::forDisplay(display);
    }
    return s_current;
}

EGLDisplayOpenVG* EGLDisplayOpenVG::forDisplay(const EGLDisplay& display)
{
    EGLDisplayManagerMap& managers = displayManagers();

    if (!managers.contains(display))
        managers.set(display, new EGLDisplayOpenVG(display));

    return managers.get(display);
}


// Object/instance members.

EGLDisplayOpenVG::EGLDisplayOpenVG(const EGLDisplay& display)
    : m_display(display)
    , m_sharedPlatformSurface(0)
    , m_pbufferConfigId(0)
    , m_windowConfigId(0)
{
    eglBindAPI(EGL_OPENVG_API);
    ASSERT_EGL_NO_ERROR();
}

EGLDisplayOpenVG::~EGLDisplayOpenVG()
{
    eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    ASSERT_EGL_NO_ERROR();

    delete m_sharedPlatformSurface;

    HashMap<EGLSurface, EGLint>::const_iterator end = m_surfaceConfigIds.end();
    for (HashMap<EGLSurface, EGLint>::const_iterator it = m_surfaceConfigIds.begin(); it != end; ++it)
        destroySurface((*it).first);

    eglTerminate(m_display);
    ASSERT_EGL_NO_ERROR();
}

void EGLDisplayOpenVG::setDefaultPbufferConfig(const EGLConfig& config)
{
    EGLint configId;
    EGLBoolean success = eglGetConfigAttrib(m_display, config, EGL_CONFIG_ID, &configId);
    ASSERT(success == EGL_TRUE);
    ASSERT(configId != EGL_BAD_ATTRIBUTE);

    m_pbufferConfigId = configId;
}

EGLConfig EGLDisplayOpenVG::defaultPbufferConfig()
{
    EGLConfig config;
    EGLint numConfigs;

    // Hopefully the client will have set the pbuffer config of its choice
    // by now - if not, use a 32-bit generic one as default.
    if (!m_pbufferConfigId) {
        static const EGLint configAttribs[] = {
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_ALPHA_MASK_SIZE, 1,
            EGL_LUMINANCE_SIZE, EGL_DONT_CARE,
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENVG_BIT,
            EGL_NONE
        };
        eglChooseConfig(m_display, configAttribs, &config, 1, &numConfigs);
    } else {
        const EGLint configAttribs[] = {
            EGL_CONFIG_ID, m_pbufferConfigId,
            EGL_NONE
        };
        eglChooseConfig(m_display, configAttribs, &config, 1, &numConfigs);
    }

    ASSERT_EGL_NO_ERROR();
    ASSERT(numConfigs == 1);
    return config;
}

void EGLDisplayOpenVG::setDefaultWindowConfig(const EGLConfig& config)
{
    EGLint configId;
    EGLBoolean success = eglGetConfigAttrib(m_display, config, EGL_CONFIG_ID, &configId);
    ASSERT(success == EGL_TRUE);
    ASSERT(configId != EGL_BAD_ATTRIBUTE);

    m_windowConfigId = configId;
}

EGLConfig EGLDisplayOpenVG::defaultWindowConfig()
{
    EGLConfig config;
    EGLint numConfigs;

    // Hopefully the client will have set the window config of its choice
    // by now - if not, use a 32-bit generic one as default.
    if (!m_windowConfigId) {
        static const EGLint configAttribs[] = {
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_ALPHA_MASK_SIZE, 1,
            EGL_LUMINANCE_SIZE, EGL_DONT_CARE,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENVG_BIT,
            EGL_NONE
        };
        eglChooseConfig(m_display, configAttribs, &config, 1, &numConfigs);
    } else {
        const EGLint configAttribs[] = {
            EGL_CONFIG_ID, m_windowConfigId,
            EGL_NONE
        };
        eglChooseConfig(m_display, configAttribs, &config, 1, &numConfigs);
    }

    ASSERT_EGL_NO_ERROR();
    ASSERT(numConfigs == 1);
    return config;
}

SurfaceOpenVG* EGLDisplayOpenVG::sharedPlatformSurface()
{
    if (!m_sharedPlatformSurface) {
        // The shared surface doesn't need to be drawn on, it just exists so
        // that we can always make the shared context current (which in turn is
        // the owner of long-living resources such as images, paths and fonts).
        // We'll just make the shared surface as small as possible: 1x1 pixel.
        EGLConfig config = defaultPbufferConfig();
        EGLSurface surface = createPbufferSurface(IntSize(1, 1), config);

        EGLContext context = eglCreateContext(m_display, config, EGL_NO_CONTEXT, 0);
        ASSERT_EGL_NO_ERROR();
        m_contexts.set(m_surfaceConfigIds.get(surface), context);

        m_sharedPlatformSurface = new SurfaceOpenVG;
        m_sharedPlatformSurface->m_eglDisplay = m_display;
        m_sharedPlatformSurface->m_eglSurface = surface;
        m_sharedPlatformSurface->m_eglContext = context;
        m_platformSurfaces.set(surface, m_sharedPlatformSurface); // a.k.a. registerPlatformSurface()
    }
    return m_sharedPlatformSurface;
}

EGLSurface EGLDisplayOpenVG::createPbufferSurface(const IntSize& size, const EGLConfig& config, EGLint* errorCode)
{
    const EGLint attribList[] = {
        EGL_WIDTH, size.width(),
        EGL_HEIGHT, size.height(),
        EGL_NONE
    };
    EGLSurface surface = eglCreatePbufferSurface(m_display, config, attribList);

    if (errorCode)
        *errorCode = eglGetError();
    else
        ASSERT_EGL_NO_ERROR();

    if (surface == EGL_NO_SURFACE)
        return EGL_NO_SURFACE;

    EGLint surfaceConfigId;
    EGLBoolean success = eglGetConfigAttrib(m_display, config, EGL_CONFIG_ID, &surfaceConfigId);
    ASSERT(success == EGL_TRUE);
    ASSERT(surfaceConfigId != EGL_BAD_ATTRIBUTE);

    ASSERT(!m_surfaceConfigIds.contains(surface));
    m_surfaceConfigIds.set(surface, surfaceConfigId);
    return surface;
}

EGLSurface EGLDisplayOpenVG::createPbufferFromClientBuffer(
    EGLClientBuffer clientBuffer, EGLenum bufferType, const EGLConfig& config, EGLint* errorCode)
{
    EGLSurface surface = eglCreatePbufferFromClientBuffer(m_display,
        bufferType, clientBuffer, config, 0 /* attribList */);

    if (errorCode)
        *errorCode = eglGetError();
    else
        ASSERT_EGL_NO_ERROR();

    if (surface == EGL_NO_SURFACE)
        return EGL_NO_SURFACE;

    EGLint surfaceConfigId;
    EGLBoolean success = eglGetConfigAttrib(m_display, config, EGL_CONFIG_ID, &surfaceConfigId);
    ASSERT(success == EGL_TRUE);
    ASSERT(surfaceConfigId != EGL_BAD_ATTRIBUTE);

    ASSERT(!m_surfaceConfigIds.contains(surface));
    m_surfaceConfigIds.set(surface, surfaceConfigId);
    return surface;
}

EGLSurface EGLDisplayOpenVG::surfaceForWindow(EGLNativeWindowType wId, const EGLConfig& config)
{
    if (m_windowSurfaces.contains(wId))
        return m_windowSurfaces.get(wId);

    EGLSurface surface = eglCreateWindowSurface(m_display, config, wId, 0);
    ASSERT_EGL_NO_ERROR();

    EGLint surfaceConfigId;
    EGLBoolean success = eglGetConfigAttrib(m_display, config, EGL_CONFIG_ID, &surfaceConfigId);
    ASSERT(success == EGL_TRUE);
    ASSERT(surfaceConfigId != EGL_BAD_ATTRIBUTE);

    ASSERT(!m_surfaceConfigIds.contains(surface));
    m_surfaceConfigIds.set(surface, surfaceConfigId);
    return surface;
}

bool EGLDisplayOpenVG::surfacesCompatible(const EGLSurface& surface, const EGLSurface& otherSurface)
{
    if (surface == EGL_NO_SURFACE || otherSurface == EGL_NO_SURFACE)
        return false;

    // Currently, we assume that all surfaces known to this object are
    // context-compatible to each other (which is reasonable to assume,
    // otherwise eglCreateContext() would fail with EGL_BAD_MATCH for shared
    // context compatibility anyways.
    return m_surfaceConfigIds.contains(surface) && m_surfaceConfigIds.contains(otherSurface);
}

void EGLDisplayOpenVG::destroySurface(const EGLSurface& surface)
{
    ASSERT(surface != EGL_NO_SURFACE);

    if (eglGetCurrentSurface(EGL_DRAW) == surface) {
        eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        ASSERT_EGL_NO_ERROR();
    }

    // Destroy the context associated to the surface, if we already created one.
    if (m_surfaceConfigIds.contains(surface)) {
        EGLint surfaceConfigId = m_surfaceConfigIds.take(surface); // take = get and remove
        bool isContextReferenced = false;

        if (m_compatibleConfigIds.contains(surfaceConfigId))
            surfaceConfigId = m_compatibleConfigIds.get(surfaceConfigId);

        HashMap<EGLSurface, EGLint>::iterator end = m_surfaceConfigIds.end();

        // ...but only if there's no other surfaces associated to that context.
        for (HashMap<EGLSurface, EGLint>::iterator it = m_surfaceConfigIds.begin(); it != end; ++it) {
            if ((*it).second == surfaceConfigId) {
                isContextReferenced = true;
                break;
            }
        }
        if (!isContextReferenced && m_contexts.contains(surfaceConfigId)) {
            EGLContext context = m_contexts.take(surfaceConfigId);
            eglDestroyContext(m_display, context);
            ASSERT_EGL_NO_ERROR();
        }
    }

    m_platformSurfaces.remove(surface);

    HashMap<EGLNativeWindowType, EGLSurface>::iterator end = m_windowSurfaces.end();
    for (HashMap<EGLNativeWindowType, EGLSurface>::iterator it = m_windowSurfaces.begin(); it != end; ++it) {
        if ((*it).second == surface) {
            m_windowSurfaces.remove(it);
            break;
        }
    }

    eglDestroySurface(m_display, surface);
    ASSERT_EGL_NO_ERROR();
}

EGLContext EGLDisplayOpenVG::contextForSurface(const EGLSurface& surface)
{
    ASSERT(surface != EGL_NO_SURFACE);

    if (m_platformSurfaces.contains(surface))
        return m_platformSurfaces.get(surface)->eglContext();

    eglBindAPI(EGL_OPENVG_API);
    ASSERT_EGL_NO_ERROR();

    EGLint surfaceConfigId;

    if (m_surfaceConfigIds.contains(surface))
        surfaceConfigId = m_surfaceConfigIds.get(surface);
    else {
        // Retrieve the same EGL config for context creation that was used to
        // create the the EGL surface.
        EGLBoolean success = eglQuerySurface(m_display, surface, EGL_CONFIG_ID, &surfaceConfigId);
        ASSERT(success == EGL_TRUE);
        ASSERT(surfaceConfigId != EGL_BAD_ATTRIBUTE);

        m_surfaceConfigIds.set(surface, surfaceConfigId);
    }

    if (m_compatibleConfigIds.contains(surfaceConfigId))
        surfaceConfigId = m_compatibleConfigIds.get(surfaceConfigId);

    if (m_contexts.contains(surfaceConfigId))
        return m_contexts.get(surfaceConfigId);

    if (!m_sharedPlatformSurface) // shared context has not been created yet
        sharedPlatformSurface(); // creates the shared surface & context

    EGLDisplay currentDisplay = eglGetCurrentDisplay();
    EGLSurface currentReadSurface = eglGetCurrentSurface(EGL_READ);
    EGLSurface currentDrawSurface = eglGetCurrentSurface(EGL_DRAW);
    EGLContext currentContext = eglGetCurrentContext();

    // Before creating a new context, let's try whether an existing one
    // is compatible with the surface. EGL doesn't give us a different way
    // to check context/surface compatibility than trying it out, so let's
    // do just that.
    HashMap<EGLint, EGLContext>::iterator end = m_contexts.end();

    for (HashMap<EGLint, EGLContext>::iterator it = m_contexts.begin(); it != end; ++it) {
        eglMakeCurrent(m_display, surface, surface, (*it).second);
        if (eglGetError() == EGL_SUCCESS) {
            // Restore previous surface/context.
            if (currentContext != EGL_NO_CONTEXT) {
                eglMakeCurrent(currentDisplay, currentReadSurface, currentDrawSurface, currentContext);
                ASSERT_EGL_NO_ERROR();
            }
            // Cool, surface is compatible to one of our existing contexts.
            m_compatibleConfigIds.set(surfaceConfigId, (*it).first);
            return (*it).second;
        }
    }
    // Restore previous surface/context.
    if (currentContext != EGL_NO_CONTEXT) {
        eglMakeCurrent(currentDisplay, currentReadSurface, currentDrawSurface, currentContext);
        ASSERT_EGL_NO_ERROR();
    }

    EGLConfig config;
    EGLint numConfigs;

    const EGLint configAttribs[] = {
        EGL_CONFIG_ID, surfaceConfigId,
        EGL_NONE
    };

    eglChooseConfig(m_display, configAttribs, &config, 1, &numConfigs);
    ASSERT_EGL_NO_ERROR();
    ASSERT(numConfigs == 1);

    // We share all of the images and paths amongst the different contexts,
    // so that they can be used in all of them. Resources that are created
    // while m_sharedPlatformSurface->context() is current will be
    // accessible from all other contexts, but are not restricted to the
    // lifetime of those contexts.
    EGLContext context = eglCreateContext(m_display, config, m_sharedPlatformSurface->eglContext(), 0);
    ASSERT_EGL_NO_ERROR();

    ASSERT(!m_contexts.contains(surfaceConfigId));
    m_contexts.set(surfaceConfigId, context);
    return context;
}

}
