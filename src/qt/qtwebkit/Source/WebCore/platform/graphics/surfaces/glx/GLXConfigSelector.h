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

#ifndef GLXConfigSelector_h
#define GLXConfigSelector_h

#if USE(ACCELERATED_COMPOSITING) && USE(GLX)

#include "X11Helper.h"
#include <opengl/GLDefs.h>
#include <opengl/GLPlatformSurface.h>

namespace WebCore {

static int clientAttributes[] = {
    // The specification is a set key value pairs stored in a simple array.
    GLX_LEVEL,                          0,
    static_cast<int>(GLX_VISUAL_ID),    0,
    GLX_DRAWABLE_TYPE,                  GLX_PIXMAP_BIT,
    GLX_BIND_TO_TEXTURE_TARGETS_EXT,    GLX_TEXTURE_2D_BIT_EXT,
    GLX_BIND_TO_TEXTURE_RGBA_EXT,       TRUE,
    0
};

static int glxSurfaceAttributes[] = {
    GLX_LEVEL, 0,
    GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT,
    GLX_RENDER_TYPE,   0,
    GLX_RED_SIZE,      1,
    GLX_GREEN_SIZE,    1,
    GLX_BLUE_SIZE,     1,
    GLX_ALPHA_SIZE,    0,
    GLX_DOUBLEBUFFER,  GL_FALSE,
    None
};

class GLXConfigSelector {
    WTF_MAKE_NONCOPYABLE(GLXConfigSelector);

public:
    GLXConfigSelector(GLPlatformSurface::SurfaceAttributes attr = GLPlatformSurface::Default)
        : m_surfaceContextFBConfig(0)
        , m_pixmapContextFBConfig(0)
        , m_attributes(attr)
    {
    }

    virtual ~GLXConfigSelector()
    {
    }

    XVisualInfo* visualInfo(const GLXFBConfig& config)
    {
        return glXGetVisualFromFBConfig(X11Helper::nativeDisplay(), config);
    }

    GLXFBConfig pixmapContextConfig()
    {
        if (!m_pixmapContextFBConfig) {
            validateAttributes();
            m_pixmapContextFBConfig = findMatchingConfig(glxSurfaceAttributes, m_attributes & GLPlatformSurface::SupportAlpha ? 32 : 24);
        }

        return m_pixmapContextFBConfig;
    }


    GLXFBConfig surfaceContextConfig()
    {
        if (!m_surfaceContextFBConfig) {
            glxSurfaceAttributes[3] = GLX_WINDOW_BIT;
            glxSurfaceAttributes[7] = 8;
            glxSurfaceAttributes[9] = 8;
            glxSurfaceAttributes[11] = 8;
            validateAttributes();
            m_surfaceContextFBConfig = findMatchingConfig(glxSurfaceAttributes, m_attributes & GLPlatformSurface::SupportAlpha ? 32 : 24);
        }

        return m_surfaceContextFBConfig;
    }

    GLXFBConfig surfaceClientConfig(int depth, VisualID id)
    {
        clientAttributes[3] = static_cast<int>(id);
        clientAttributes[8] = depth == 32 ? GLX_BIND_TO_TEXTURE_RGBA_EXT : GLX_BIND_TO_TEXTURE_RGB_EXT;
        // Prefer to match with Visual Id.
        GLXFBConfig config = findMatchingConfigWithVisualId(clientAttributes, depth, id);

        if (!config)
            config = findMatchingConfig(clientAttributes, depth);

        return config;
    }

    void reset()
    {
        m_surfaceContextFBConfig = 0;
        m_pixmapContextFBConfig = 0;
    }

    GLPlatformSurface::SurfaceAttributes attributes() const
    {
        return m_attributes;
    }

private:
    void validateAttributes()
    {
        if (m_attributes & GLPlatformSurface::SupportAlpha) {
            glxSurfaceAttributes[13] = 8;
            glxSurfaceAttributes[5] = GLX_RGBA_BIT;
        }

        if (m_attributes & GLPlatformSurface::DoubleBuffered)
            glxSurfaceAttributes[15] = GL_TRUE;
    }

    GLXFBConfig findMatchingConfig(const int attributes[], int depth = 32)
    {
        int numAvailableConfigs;
        OwnPtrX11<GLXFBConfig> temp(glXChooseFBConfig(X11Helper::nativeDisplay(), DefaultScreen(X11Helper::nativeDisplay()), attributes, &numAvailableConfigs));

        if (!numAvailableConfigs || !temp.get())
            return 0;

        OwnPtrX11<XVisualInfo> scopedVisualInfo;
        for (int i = 0; i < numAvailableConfigs; ++i) {
            scopedVisualInfo = glXGetVisualFromFBConfig(X11Helper::nativeDisplay(), temp[i]);
            if (!scopedVisualInfo.get())
                continue;

#if USE(GRAPHICS_SURFACE) && USE(GLX)
            if (X11Helper::isXRenderExtensionSupported()) {
                XRenderPictFormat* format = XRenderFindVisualFormat(X11Helper::nativeDisplay(), scopedVisualInfo->visual);

                if (format) {
                    if (m_attributes & GLPlatformSurface::SupportAlpha) {
                        if (scopedVisualInfo->depth == depth && format->direct.alphaMask > 0)
                            return temp[i];
                    } else if (!format->direct.alphaMask)
                        return temp[i];
                }
            }
#endif
            if (scopedVisualInfo->depth == depth)
                return temp[i];
        }

        // Did not find any visual supporting alpha, select the first available config.
        scopedVisualInfo = glXGetVisualFromFBConfig(X11Helper::nativeDisplay(), temp[0]);

        if ((m_attributes & GLPlatformSurface::SupportAlpha) && (scopedVisualInfo->depth != 32))
            m_attributes &= ~GLPlatformSurface::SupportAlpha;

        return temp[0];
    }

    GLXFBConfig findMatchingConfigWithVisualId(const int attributes[], int depth, VisualID id)
    {
        int numAvailableConfigs;
        OwnPtrX11<GLXFBConfig> temp(glXChooseFBConfig(X11Helper::nativeDisplay(), DefaultScreen(X11Helper::nativeDisplay()), attributes, &numAvailableConfigs));

        if (!numAvailableConfigs || !temp.get())
            return 0;

        OwnPtrX11<XVisualInfo> scopedVisualInfo;
        for (int i = 0; i < numAvailableConfigs; ++i) {
            scopedVisualInfo = glXGetVisualFromFBConfig(X11Helper::nativeDisplay(), temp[i]);
            if (!scopedVisualInfo.get())
                continue;

            if (id && scopedVisualInfo->depth == depth && scopedVisualInfo->visualid == id)
                return temp[i];
        }

        return 0;
    }

    GLXFBConfig m_surfaceContextFBConfig;
    GLXFBConfig m_pixmapContextFBConfig;
    GLPlatformSurface::SurfaceAttributes m_attributes : 3;
};

}

#endif

#endif

