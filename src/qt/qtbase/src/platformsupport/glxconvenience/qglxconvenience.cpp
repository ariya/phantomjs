/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

// We have to include this before the X11 headers dragged in by
// qglxconvenience_p.h.
#include <QtCore/QByteArray>

#include "qglxconvenience_p.h"

#include <QtCore/QVector>
#include <QtCore/QVarLengthArray>

#ifndef QT_NO_XRENDER
#include <X11/extensions/Xrender.h>
#endif

#include <GL/glxext.h>

enum {
    XFocusOut = FocusOut,
    XFocusIn = FocusIn,
    XKeyPress = KeyPress,
    XKeyRelease = KeyRelease,
    XNone = None,
    XRevertToParent = RevertToParent,
    XGrayScale = GrayScale,
    XCursorShape = CursorShape
};
#undef FocusOut
#undef FocusIn
#undef KeyPress
#undef KeyRelease
#undef None
#undef RevertToParent
#undef GrayScale
#undef CursorShape

#ifdef FontChange
#undef FontChange
#endif

QVector<int> qglx_buildSpec(const QSurfaceFormat &format, int drawableBit)
{
    QVector<int> spec(48);
    int i = 0;

    spec[i++] = GLX_LEVEL;
    spec[i++] = 0;
    spec[i++] = GLX_DRAWABLE_TYPE; spec[i++] = drawableBit;

    spec[i++] = GLX_RENDER_TYPE; spec[i++] = GLX_RGBA_BIT;

    spec[i++] = GLX_RED_SIZE; spec[i++] = (format.redBufferSize() == -1) ? 1 : format.redBufferSize();
    spec[i++] = GLX_GREEN_SIZE; spec[i++] =  (format.greenBufferSize() == -1) ? 1 : format.greenBufferSize();
    spec[i++] = GLX_BLUE_SIZE; spec[i++] = (format.blueBufferSize() == -1) ? 1 : format.blueBufferSize();
    if (format.hasAlpha()) {
        spec[i++] = GLX_ALPHA_SIZE; spec[i++] = format.alphaBufferSize();
    }

    spec[i++] = GLX_DOUBLEBUFFER; spec[i++] = format.swapBehavior() != QSurfaceFormat::SingleBuffer ? True : False;

    spec[i++] = GLX_STEREO; spec[i++] =  format.stereo() ? True : False;

    if (format.depthBufferSize() > 0) {
        spec[i++] = GLX_DEPTH_SIZE; spec[i++] = format.depthBufferSize();
    }

    if (format.stencilBufferSize() > 0) {
        spec[i++] = GLX_STENCIL_SIZE; spec[i++] =  (format.stencilBufferSize() == -1) ? 1 : format.stencilBufferSize();
    }

    if (format.samples() > 1) {
        spec[i++] = GLX_SAMPLE_BUFFERS_ARB;
        spec[i++] = 1;
        spec[i++] = GLX_SAMPLES_ARB;
        spec[i++] = format.samples();
    }

    spec[i++] = XNone;
    return spec;
}

GLXFBConfig qglx_findConfig(Display *display, int screen , const QSurfaceFormat &format, int drawableBit)
{
    // Allow forcing LIBGL_ALWAYS_SOFTWARE for Qt 5 applications only.
    // This is most useful with drivers that only support OpenGL 1.
    // We need OpenGL 2, but the user probably doesn't want
    // LIBGL_ALWAYS_SOFTWARE in OpenGL 1 apps.
    static bool checkedForceSoftwareOpenGL = false;
    static bool forceSoftwareOpenGL = false;
    if (!checkedForceSoftwareOpenGL) {
        // If LIBGL_ALWAYS_SOFTWARE is already set, don't mess with it.
        // We want to unset LIBGL_ALWAYS_SOFTWARE at the end so it does not
        // get inherited by other processes, of course only if it wasn't
        // already set before.
        if (!qEnvironmentVariableIsEmpty("QT_XCB_FORCE_SOFTWARE_OPENGL")
            && !qEnvironmentVariableIsSet("LIBGL_ALWAYS_SOFTWARE"))
            forceSoftwareOpenGL = true;

        checkedForceSoftwareOpenGL = true;
    }

    if (forceSoftwareOpenGL)
        qputenv("LIBGL_ALWAYS_SOFTWARE", QByteArrayLiteral("1"));

    bool reduced = true;
    GLXFBConfig chosenConfig = 0;
    QSurfaceFormat reducedFormat = format;
    while (!chosenConfig && reduced) {
        QVector<int> spec = qglx_buildSpec(reducedFormat, drawableBit);
        int confcount = 0;
        GLXFBConfig *configs;
        configs = glXChooseFBConfig(display, screen,spec.constData(),&confcount);
        if (confcount)
        {
            for (int i = 0; i < confcount; i++) {
                chosenConfig = configs[i];
                // Make sure we try to get an ARGB visual if the format asked for an alpha:
                if (reducedFormat.hasAlpha()) {
                    int alphaSize;
                    glXGetFBConfigAttrib(display,configs[i],GLX_ALPHA_SIZE,&alphaSize);
                    if (alphaSize > 0) {
                        XVisualInfo *visual = glXGetVisualFromFBConfig(display, chosenConfig);
                        bool hasAlpha = false;

#if !defined(QT_NO_XRENDER)
                        XRenderPictFormat *pictFormat = XRenderFindVisualFormat(display, visual->visual);
                        hasAlpha = pictFormat->direct.alphaMask > 0;
#else
                        hasAlpha = visual->depth == 32;
#endif

                        XFree(visual);

                        if (hasAlpha)
                            break;
                    }
                } else {
                    break; // Just choose the first in the list if there's no alpha requested
                }
            }

            XFree(configs);
        }
        if (!chosenConfig)
            reducedFormat = qglx_reduceSurfaceFormat(reducedFormat,&reduced);
    }

    // unset LIBGL_ALWAYS_SOFTWARE now so other processes don't inherit it
    if (forceSoftwareOpenGL)
        qunsetenv("LIBGL_ALWAYS_SOFTWARE");

    return chosenConfig;
}

XVisualInfo *qglx_findVisualInfo(Display *display, int screen, QSurfaceFormat *format)
{
    Q_ASSERT(format);

    XVisualInfo *visualInfo = 0;

    GLXFBConfig config = qglx_findConfig(display,screen,*format);
    if (config) {
        visualInfo = glXGetVisualFromFBConfig(display, config);
        qglx_surfaceFormatFromGLXFBConfig(format, display, config);
    }

    // attempt to fall back to glXChooseVisual
    bool reduced = true;
    QSurfaceFormat reducedFormat = *format;
    while (!visualInfo && reduced) {
        QVarLengthArray<int, 13> attribs;
        attribs.append(GLX_RGBA);

        if (reducedFormat.redBufferSize() > 0) {
            attribs.append(GLX_RED_SIZE);
            attribs.append(reducedFormat.redBufferSize());
        }

        if (reducedFormat.greenBufferSize() > 0) {
            attribs.append(GLX_GREEN_SIZE);
            attribs.append(reducedFormat.greenBufferSize());
        }

        if (reducedFormat.blueBufferSize() > 0) {
            attribs.append(GLX_BLUE_SIZE);
            attribs.append(reducedFormat.blueBufferSize());
        }

        if (reducedFormat.stencilBufferSize() > 0) {
            attribs.append(GLX_STENCIL_SIZE);
            attribs.append(reducedFormat.stencilBufferSize());
        }

        if (reducedFormat.depthBufferSize() > 0) {
            attribs.append(GLX_DEPTH_SIZE);
            attribs.append(reducedFormat.depthBufferSize());
        }

        if (reducedFormat.swapBehavior() != QSurfaceFormat::SingleBuffer)
            attribs.append(GLX_DOUBLEBUFFER);

        attribs.append(XNone);

        visualInfo = glXChooseVisual(display, screen, attribs.data());
        if (visualInfo)
            *format = reducedFormat;

        reducedFormat = qglx_reduceSurfaceFormat(reducedFormat, &reduced);
    }

    return visualInfo;
}

void qglx_surfaceFormatFromGLXFBConfig(QSurfaceFormat *format, Display *display, GLXFBConfig config, GLXContext)
{
    int redSize     = 0;
    int greenSize   = 0;
    int blueSize    = 0;
    int alphaSize   = 0;
    int depthSize   = 0;
    int stencilSize = 0;
    int sampleBuffers = 0;
    int sampleCount = 0;
    int stereo      = 0;

    XVisualInfo *vi = glXGetVisualFromFBConfig(display,config);
    XFree(vi);
    glXGetFBConfigAttrib(display, config, GLX_RED_SIZE,     &redSize);
    glXGetFBConfigAttrib(display, config, GLX_GREEN_SIZE,   &greenSize);
    glXGetFBConfigAttrib(display, config, GLX_BLUE_SIZE,    &blueSize);
    glXGetFBConfigAttrib(display, config, GLX_ALPHA_SIZE,   &alphaSize);
    glXGetFBConfigAttrib(display, config, GLX_DEPTH_SIZE,   &depthSize);
    glXGetFBConfigAttrib(display, config, GLX_STENCIL_SIZE, &stencilSize);
    glXGetFBConfigAttrib(display, config, GLX_SAMPLES_ARB,  &sampleBuffers);
    glXGetFBConfigAttrib(display, config, GLX_STEREO,       &stereo);

    format->setRedBufferSize(redSize);
    format->setGreenBufferSize(greenSize);
    format->setBlueBufferSize(blueSize);
    format->setAlphaBufferSize(alphaSize);
    format->setDepthBufferSize(depthSize);
    format->setStencilBufferSize(stencilSize);
    if (sampleBuffers) {
        glXGetFBConfigAttrib(display, config, GLX_SAMPLES_ARB, &sampleCount);
        format->setSamples(sampleCount);
    }

    format->setStereo(stereo);
}

QSurfaceFormat qglx_reduceSurfaceFormat(const QSurfaceFormat &format, bool *reduced)
{
    QSurfaceFormat retFormat = format;
    *reduced = true;

    if (retFormat.redBufferSize() > 1) {
        retFormat.setRedBufferSize(1);
    } else if (retFormat.greenBufferSize() > 1) {
        retFormat.setGreenBufferSize(1);
    } else if (retFormat.blueBufferSize() > 1) {
        retFormat.setBlueBufferSize(1);
    } else if (retFormat.samples() > 1) {
        retFormat.setSamples(qMin(retFormat.samples() / 2, 16));
    } else if (retFormat.stereo()) {
        retFormat.setStereo(false);
    }else if (retFormat.stencilBufferSize() > 0) {
        retFormat.setStencilBufferSize(0);
    }else if (retFormat.hasAlpha()) {
        retFormat.setAlphaBufferSize(0);
    }else if (retFormat.depthBufferSize() > 0) {
        retFormat.setDepthBufferSize(0);
    }else if (retFormat.swapBehavior() != QSurfaceFormat::SingleBuffer) {
        retFormat.setSwapBehavior(QSurfaceFormat::SingleBuffer);
    }else{
        *reduced = false;
    }
    return retFormat;
}
