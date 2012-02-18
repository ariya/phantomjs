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

#ifndef EGLDisplayOpenVG_h
#define EGLDisplayOpenVG_h

#include <egl.h>
#include <wtf/HashMap.h>

namespace WebCore {

class IntSize;
class SurfaceOpenVG;

class EGLDisplayOpenVG {
public:
    friend class SurfaceOpenVG;

    static SurfaceOpenVG* currentSurface();
    static void setCurrentDisplay(const EGLDisplay&);
    static EGLDisplayOpenVG* current();
    static EGLDisplayOpenVG* forDisplay(const EGLDisplay&);

    void setDefaultPbufferConfig(const EGLConfig&);
    EGLConfig defaultPbufferConfig();
    void setDefaultWindowConfig(const EGLConfig&);
    EGLConfig defaultWindowConfig();

    EGLDisplay display() const { return m_display; }
    SurfaceOpenVG* sharedPlatformSurface();

    /** Creates a pbuffer surface using the given config. If no surface
     * could be created, EGL_NO_SURFACE is returned and errors can be
     * checked with the value that is written to the errorCode parameter
     * If no surface could be created and errorCode is zero, this method
     * will trigger an assertion by itself. */
    EGLSurface createPbufferSurface(const IntSize&, const EGLConfig&, EGLint* errorCode = 0);
    EGLSurface createPbufferFromClientBuffer(EGLClientBuffer, EGLenum bufferType, const EGLConfig&, EGLint* errorCode = 0);

    EGLSurface surfaceForWindow(EGLNativeWindowType, const EGLConfig&);

    bool surfacesCompatible(const EGLSurface&, const EGLSurface&);

    /** Destroy the surface and its corresponding context (unless another
     * surface is still using the same context, in which case the context
     * is not destroyed). */
    void destroySurface(const EGLSurface&);

    /** Return the context corresponding to the surface.
     * If no corresponding context exists, one is created automatically. */
    EGLContext contextForSurface(const EGLSurface&);

private:
    static void registerPlatformSurface(SurfaceOpenVG*);
    static void unregisterPlatformSurface(SurfaceOpenVG*);

    EGLDisplayOpenVG(const EGLDisplay& display);
    ~EGLDisplayOpenVG();

    EGLDisplay m_display;
    SurfaceOpenVG* m_sharedPlatformSurface;
    EGLint m_pbufferConfigId;
    EGLint m_windowConfigId;

    HashMap<EGLSurface, SurfaceOpenVG*> m_platformSurfaces;
    HashMap<EGLNativeWindowType, EGLSurface> m_windowSurfaces;
    HashMap<EGLSurface, EGLint> m_surfaceConfigIds;
    HashMap<EGLint, EGLint> m_compatibleConfigIds;
    HashMap<EGLint, EGLContext> m_contexts;
};

}

#endif
