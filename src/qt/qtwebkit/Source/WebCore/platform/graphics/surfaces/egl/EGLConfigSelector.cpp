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
#include "EGLConfigSelector.h"

#if USE(EGL)

#include "EGLHelper.h"

namespace WebCore {

EGLConfigSelector::EGLConfigSelector(GLPlatformSurface::SurfaceAttributes attributes)
    : m_pixmapFBConfig(0)
    , m_surfaceContextFBConfig(0)
    , m_attributes(attributes)
{
    m_sharedDisplay = EGLHelper::eglDisplay();
}

EGLConfigSelector::~EGLConfigSelector()
{
}

EGLConfig EGLConfigSelector::pixmapContextConfig()
{
    if (!m_pixmapFBConfig)
        m_pixmapFBConfig = createConfig(EGL_PIXMAP_BIT);

    return m_pixmapFBConfig;
}

EGLConfig EGLConfigSelector::surfaceContextConfig()
{
    if (!m_surfaceContextFBConfig)
        m_surfaceContextFBConfig = createConfig(EGL_WINDOW_BIT);

    return m_surfaceContextFBConfig;
}

EGLint EGLConfigSelector::nativeVisualId(const EGLConfig& config) const
{
    if (m_sharedDisplay == EGL_NO_DISPLAY)
        return -1;

    EGLint eglValue = 0;
    eglGetConfigAttrib(m_sharedDisplay, config, EGL_NATIVE_VISUAL_ID, &eglValue);

    return eglValue;
}

GLPlatformSurface::SurfaceAttributes EGLConfigSelector::attributes() const
{
    return m_attributes;
}

void EGLConfigSelector::reset()
{
    m_surfaceContextFBConfig = 0;
    m_pixmapFBConfig = 0;
}

EGLConfig EGLConfigSelector::createConfig(EGLint expectedSurfaceType)
{
    if (m_sharedDisplay == EGL_NO_DISPLAY)
        return 0;

    EGLint numConfigs;

    eglGetConfigs(m_sharedDisplay, 0, 0, &numConfigs);

    if (!numConfigs) {
        LOG_ERROR("Failed to retrieve number of EGL configs.");
        return 0;
    }

    EGLConfig configs[numConfigs];
    eglGetConfigs(m_sharedDisplay, configs, numConfigs, &numConfigs);

    if (!numConfigs) {
        LOG_ERROR("Failed to retrieve any EGL configs.");
        return 0;
    }

    EGLConfig config = 0;
    EGLint alpha, surface, red, green, blue, renderType;
    EGLint expectedAlpha = m_attributes & GLPlatformSurface::SupportAlpha ? 8 : 0;
    EGLint expectedRed = 8;
    EGLint expectedBlue = 8;
    EGLint expectedGreen = 8;
#if USE(OPENGL_ES_2)
    EGLint expectedRenderType = EGL_OPENGL_ES2_BIT;
#else
    EGLint expectedRenderType = EGL_OPENGL_BIT,
#endif

    for (int i = 0; i < numConfigs; i++) {
        EGLConfig tempConfig = configs[i];
        eglGetConfigAttrib(m_sharedDisplay, tempConfig, EGL_RENDERABLE_TYPE, &renderType);

        if (!(renderType & expectedRenderType))
            continue;

        eglGetConfigAttrib(m_sharedDisplay, tempConfig, EGL_ALPHA_SIZE, &alpha);

        if (alpha != expectedAlpha)
            continue;

        eglGetConfigAttrib(m_sharedDisplay, tempConfig, EGL_RED_SIZE, &red);

        if (red != expectedRed)
            continue;

        eglGetConfigAttrib(m_sharedDisplay, tempConfig, EGL_GREEN_SIZE, &green);

        if (green != expectedGreen)
            continue;

        eglGetConfigAttrib(m_sharedDisplay, tempConfig, EGL_BLUE_SIZE, &blue);

        if (blue != expectedBlue)
            continue;

        eglGetConfigAttrib(m_sharedDisplay, tempConfig, EGL_SURFACE_TYPE, &surface);

        if (surface & expectedSurfaceType) {
            config = configs[i];
            break;
        }
    }

    if ((m_attributes & GLPlatformSurface::SupportAlpha) && !config) {
        LOG_ERROR("Failed to retrieve EGL Configuration with alpha. Trying to find one without alpha support.");
        m_attributes &= ~GLPlatformSurface::SupportAlpha;
        config = createConfig(expectedSurfaceType);
    }

    if (!config)
        LOG_ERROR("Failed to find a valid EGL Configuration.");

    return config;
}

#if PLATFORM(X11)
EGLConfig EGLConfigSelector::surfaceClientConfig(NativeVisualId id)
{
    EGLConfig config = findMatchingConfigWithVisualId(id);

    if (!config)
        config = createConfig(EGL_PIXMAP_BIT);

    return config;
}

EGLConfig EGLConfigSelector::findMatchingConfigWithVisualId(NativeVisualId id)
{
    if (m_sharedDisplay == EGL_NO_DISPLAY)
        return 0;

    EGLint numConfigs;
    EGLint i;
    EGLint visualId;
    EGLConfig config = 0;

    eglGetConfigs(m_sharedDisplay, 0, 0, &numConfigs);

    if (!numConfigs) {
        LOG_ERROR("Failed to retrieve any EGL configs.");
        return 0;
    }

    EGLConfig configs[numConfigs];
    eglGetConfigs(m_sharedDisplay, configs, numConfigs, &numConfigs);

    if (!numConfigs) {
        LOG_ERROR("Failed to retrieve any EGL configs.");
        return 0;
    }

    int alphaChannelRequired = m_attributes & GLPlatformSurface::SupportAlpha ? 8 : 0;
    for (i = 0; i < numConfigs; i++) {
        EGLint alpha, surfaces;
        EGLConfig tempConfig = configs[i];
        eglGetConfigAttrib(m_sharedDisplay, tempConfig, EGL_NATIVE_VISUAL_ID, &visualId);

        if (static_cast<NativeVisualId>(visualId) != id)
            continue;

        eglGetConfigAttrib(m_sharedDisplay, tempConfig, EGL_ALPHA_SIZE, &alpha);

        if (alphaChannelRequired != alpha)
            continue;

        eglGetConfigAttrib(m_sharedDisplay, tempConfig, EGL_SURFACE_TYPE, &surfaces);

        if (surfaces & EGL_PIXMAP_BIT) {
            config = tempConfig;
            break;
        }
    }

    return config;
}
#endif

}

#endif
