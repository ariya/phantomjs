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

#include <QByteArray>
#include <QOpenGLContext>

#ifdef Q_OS_LINUX
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <private/qmath_p.h>
#endif

#include "qeglconvenience_p.h"

#ifndef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT_KHR 0x0040
#endif

QT_BEGIN_NAMESPACE

QVector<EGLint> q_createConfigAttributesFromFormat(const QSurfaceFormat &format)
{
    int redSize     = format.redBufferSize();
    int greenSize   = format.greenBufferSize();
    int blueSize    = format.blueBufferSize();
    int alphaSize   = format.alphaBufferSize();
    int depthSize   = format.depthBufferSize();
    int stencilSize = format.stencilBufferSize();
    int sampleCount = format.samples();

    // We want to make sure 16-bit configs are chosen over 32-bit configs as they will provide
    // the best performance. The EGL config selection algorithm is a bit stange in this regard:
    // The selection criteria for EGL_BUFFER_SIZE is "AtLeast", so we can't use it to discard
    // 32-bit configs completely from the selection. So it then comes to the sorting algorithm.
    // The red/green/blue sizes have a sort priority of 3, so they are sorted by first. The sort
    // order is special and described as "by larger _total_ number of color bits.". So EGL will
    // put 32-bit configs in the list before the 16-bit configs. However, the spec also goes on
    // to say "If the requested number of bits in attrib_list for a particular component is 0,
    // then the number of bits for that component is not considered". This part of the spec also
    // seems to imply that setting the red/green/blue bits to zero means none of the components
    // are considered and EGL disregards the entire sorting rule. It then looks to the next
    // highest priority rule, which is EGL_BUFFER_SIZE. Despite the selection criteria being
    // "AtLeast" for EGL_BUFFER_SIZE, it's sort order is "smaller" meaning 16-bit configs are
    // put in the list before 32-bit configs. So, to make sure 16-bit is preffered over 32-bit,
    // we must set the red/green/blue sizes to zero. This has an unfortunate consequence that
    // if the application sets the red/green/blue size to 5/6/5 on the QSurfaceFormat,
    // they might still get a 32-bit config, even when there's an RGB565 config available.

    QVector<EGLint> configAttributes;

    configAttributes.append(EGL_RED_SIZE);
    configAttributes.append(redSize > 0 ? redSize : 0);

    configAttributes.append(EGL_GREEN_SIZE);
    configAttributes.append(greenSize > 0 ? greenSize : 0);

    configAttributes.append(EGL_BLUE_SIZE);
    configAttributes.append(blueSize > 0 ? blueSize : 0);

    configAttributes.append(EGL_ALPHA_SIZE);
    configAttributes.append(alphaSize > 0 ? alphaSize : 0);

    configAttributes.append(EGL_DEPTH_SIZE);
    configAttributes.append(depthSize > 0 ? depthSize : 0);

    configAttributes.append(EGL_STENCIL_SIZE);
    configAttributes.append(stencilSize > 0 ? stencilSize : 0);

    configAttributes.append(EGL_SAMPLES);
    configAttributes.append(sampleCount > 0 ? sampleCount : 0);

    configAttributes.append(EGL_SAMPLE_BUFFERS);
    configAttributes.append(sampleCount > 0);

    return configAttributes;
}

bool q_reduceConfigAttributes(QVector<EGLint> *configAttributes)
{
    int i = -1;
    // Reduce the complexity of a configuration request to ask for less
    // because the previous request did not result in success.  Returns
    // true if the complexity was reduced, or false if no further
    // reductions in complexity are possible.

    i = configAttributes->indexOf(EGL_SWAP_BEHAVIOR);
    if (i >= 0) {
        configAttributes->remove(i,2);
    }

#ifdef EGL_VG_ALPHA_FORMAT_PRE_BIT
    // For OpenVG, we sometimes try to create a surface using a pre-multiplied format. If we can't
    // find a config which supports pre-multiplied formats, remove the flag on the surface type:

    i = configAttributes->indexOf(EGL_SURFACE_TYPE);
    if (i >= 0) {
        EGLint surfaceType = configAttributes->at(i +1);
        if (surfaceType & EGL_VG_ALPHA_FORMAT_PRE_BIT) {
            surfaceType ^= EGL_VG_ALPHA_FORMAT_PRE_BIT;
            configAttributes->replace(i+1,surfaceType);
            return true;
        }
    }
#endif

    // EGL chooses configs with the highest color depth over
    // those with smaller (but faster) lower color depths. One
    // way around this is to set EGL_BUFFER_SIZE to 16, which
    // trumps the others. Of course, there may not be a 16-bit
    // config available, so it's the first restraint we remove.
    i = configAttributes->indexOf(EGL_BUFFER_SIZE);
    if (i >= 0) {
        if (configAttributes->at(i+1) == 16) {
            configAttributes->remove(i,2);
            return true;
        }
    }

    i = configAttributes->indexOf(EGL_SAMPLES);
    if (i >= 0) {
        EGLint value = configAttributes->value(i+1, 0);
        if (value > 1)
            configAttributes->replace(i+1, qMin(EGLint(16), value / 2));
        else
            configAttributes->remove(i, 2);
        return true;
    }

    i = configAttributes->indexOf(EGL_SAMPLE_BUFFERS);
    if (i >= 0) {
        configAttributes->remove(i,2);
        return true;
    }

    i = configAttributes->indexOf(EGL_ALPHA_SIZE);
    if (i >= 0) {
        configAttributes->remove(i,2);
#if defined(EGL_BIND_TO_TEXTURE_RGBA) && defined(EGL_BIND_TO_TEXTURE_RGB)
        i = configAttributes->indexOf(EGL_BIND_TO_TEXTURE_RGBA);
        if (i >= 0) {
            configAttributes->replace(i,EGL_BIND_TO_TEXTURE_RGB);
            configAttributes->replace(i+1,true);

        }
#endif
        return true;
    }

    i = configAttributes->indexOf(EGL_STENCIL_SIZE);
    if (i >= 0) {
        if (configAttributes->at(i + 1) > 1)
            configAttributes->replace(i + 1, 1);
        else
            configAttributes->remove(i, 2);
        return true;
    }

    i = configAttributes->indexOf(EGL_DEPTH_SIZE);
    if (i >= 0) {
        if (configAttributes->at(i + 1) > 1)
            configAttributes->replace(i + 1, 1);
        else
            configAttributes->remove(i, 2);
        return true;
    }
#ifdef EGL_BIND_TO_TEXTURE_RGB
    i = configAttributes->indexOf(EGL_BIND_TO_TEXTURE_RGB);
    if (i >= 0) {
        configAttributes->remove(i,2);
        return true;
    }
#endif

    return false;
}

QEglConfigChooser::QEglConfigChooser(EGLDisplay display)
    : m_display(display)
    , m_surfaceType(EGL_WINDOW_BIT)
    , m_ignore(false)
    , m_confAttrRed(0)
    , m_confAttrGreen(0)
    , m_confAttrBlue(0)
    , m_confAttrAlpha(0)
{
}

QEglConfigChooser::~QEglConfigChooser()
{
}

EGLConfig QEglConfigChooser::chooseConfig()
{
    QVector<EGLint> configureAttributes = q_createConfigAttributesFromFormat(m_format);
    configureAttributes.append(EGL_SURFACE_TYPE);
    configureAttributes.append(surfaceType());

    configureAttributes.append(EGL_RENDERABLE_TYPE);
    bool needsES2Plus = false;
    switch (m_format.renderableType()) {
    case QSurfaceFormat::OpenVG:
        configureAttributes.append(EGL_OPENVG_BIT);
        break;
#ifdef EGL_VERSION_1_4
    case QSurfaceFormat::DefaultRenderableType:
#ifndef QT_NO_OPENGL
        if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL)
            configureAttributes.append(EGL_OPENGL_BIT);
        else
#endif // QT_NO_OPENGL
            needsES2Plus = true;
        break;
    case QSurfaceFormat::OpenGL:
         configureAttributes.append(EGL_OPENGL_BIT);
         break;
#endif
    case QSurfaceFormat::OpenGLES:
        if (m_format.majorVersion() == 1) {
            configureAttributes.append(EGL_OPENGL_ES_BIT);
            break;
        }
        // fall through
    default:
        needsES2Plus = true;
        break;
    }
    if (needsES2Plus) {
        if (m_format.majorVersion() >= 3 && q_hasEglExtension(display(), "EGL_KHR_create_context"))
            configureAttributes.append(EGL_OPENGL_ES3_BIT_KHR);
        else
            configureAttributes.append(EGL_OPENGL_ES2_BIT);
    }
    configureAttributes.append(EGL_NONE);

    EGLConfig cfg = 0;
    do {
        // Get the number of matching configurations for this set of properties.
        EGLint matching = 0;
        if (!eglChooseConfig(display(), configureAttributes.constData(), 0, 0, &matching) || !matching)
            continue;

        // Fetch all of the matching configurations and find the
        // first that matches the pixel format we wanted.
        int i = configureAttributes.indexOf(EGL_RED_SIZE);
        m_confAttrRed = configureAttributes.at(i+1);
        i = configureAttributes.indexOf(EGL_GREEN_SIZE);
        m_confAttrGreen = configureAttributes.at(i+1);
        i = configureAttributes.indexOf(EGL_BLUE_SIZE);
        m_confAttrBlue = configureAttributes.at(i+1);
        i = configureAttributes.indexOf(EGL_ALPHA_SIZE);
        m_confAttrAlpha = i == -1 ? 0 : configureAttributes.at(i+1);

        QVector<EGLConfig> configs(matching);
        eglChooseConfig(display(), configureAttributes.constData(), configs.data(), configs.size(), &matching);
        if (!cfg && matching > 0)
            cfg = configs.first();

        for (int i = 0; i < configs.size(); ++i) {
            if (filterConfig(configs[i]))
                return configs.at(i);
        }
    } while (q_reduceConfigAttributes(&configureAttributes));

    if (!cfg)
        qWarning("Cant find EGLConfig, returning null config");
    return cfg;
}

bool QEglConfigChooser::filterConfig(EGLConfig config) const
{
    if (m_ignore)
        return true;

    EGLint red = 0;
    EGLint green = 0;
    EGLint blue = 0;
    EGLint alpha = 0;

    if (m_confAttrRed)
        eglGetConfigAttrib(display(), config, EGL_RED_SIZE, &red);
    if (m_confAttrGreen)
        eglGetConfigAttrib(display(), config, EGL_GREEN_SIZE, &green);
    if (m_confAttrBlue)
        eglGetConfigAttrib(display(), config, EGL_BLUE_SIZE, &blue);
    if (m_confAttrAlpha)
        eglGetConfigAttrib(display(), config, EGL_ALPHA_SIZE, &alpha);

    return red == m_confAttrRed && green == m_confAttrGreen
           && blue == m_confAttrBlue && alpha == m_confAttrAlpha;
}

EGLConfig q_configFromGLFormat(EGLDisplay display, const QSurfaceFormat &format, bool highestPixelFormat, int surfaceType)
{
    QEglConfigChooser chooser(display);
    chooser.setSurfaceFormat(format);
    chooser.setSurfaceType(surfaceType);
    chooser.setIgnoreColorChannels(highestPixelFormat);

    return chooser.chooseConfig();
}

QSurfaceFormat q_glFormatFromConfig(EGLDisplay display, const EGLConfig config, const QSurfaceFormat &referenceFormat)
{
    QSurfaceFormat format;
    EGLint redSize     = 0;
    EGLint greenSize   = 0;
    EGLint blueSize    = 0;
    EGLint alphaSize   = 0;
    EGLint depthSize   = 0;
    EGLint stencilSize = 0;
    EGLint sampleCount = 0;
    EGLint renderableType = 0;

    eglGetConfigAttrib(display, config, EGL_RED_SIZE,     &redSize);
    eglGetConfigAttrib(display, config, EGL_GREEN_SIZE,   &greenSize);
    eglGetConfigAttrib(display, config, EGL_BLUE_SIZE,    &blueSize);
    eglGetConfigAttrib(display, config, EGL_ALPHA_SIZE,   &alphaSize);
    eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE,   &depthSize);
    eglGetConfigAttrib(display, config, EGL_STENCIL_SIZE, &stencilSize);
    eglGetConfigAttrib(display, config, EGL_SAMPLES,      &sampleCount);
    eglGetConfigAttrib(display, config, EGL_RENDERABLE_TYPE, &renderableType);

    if (referenceFormat.renderableType() == QSurfaceFormat::OpenVG && (renderableType & EGL_OPENVG_BIT))
        format.setRenderableType(QSurfaceFormat::OpenVG);
#ifdef EGL_VERSION_1_4
    else if (referenceFormat.renderableType() == QSurfaceFormat::OpenGL
             && (renderableType & EGL_OPENGL_BIT))
        format.setRenderableType(QSurfaceFormat::OpenGL);
    else if (referenceFormat.renderableType() == QSurfaceFormat::DefaultRenderableType
#ifndef QT_NO_OPENGL
             && QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL
#endif
             && (renderableType & EGL_OPENGL_BIT))
        format.setRenderableType(QSurfaceFormat::OpenGL);
#endif
    else
        format.setRenderableType(QSurfaceFormat::OpenGLES);

    format.setRedBufferSize(redSize);
    format.setGreenBufferSize(greenSize);
    format.setBlueBufferSize(blueSize);
    format.setAlphaBufferSize(alphaSize);
    format.setDepthBufferSize(depthSize);
    format.setStencilBufferSize(stencilSize);
    format.setSamples(sampleCount);
    format.setStereo(false);         // EGL doesn't support stereo buffers
    format.setSwapInterval(referenceFormat.swapInterval());

    // Clear the EGL error state because some of the above may
    // have errored out because the attribute is not applicable
    // to the surface type.  Such errors don't matter.
    eglGetError();

    return format;
}

bool q_hasEglExtension(EGLDisplay display, const char* extensionName)
{
    QList<QByteArray> extensions =
        QByteArray(reinterpret_cast<const char *>
            (eglQueryString(display, EGL_EXTENSIONS))).split(' ');
    return extensions.contains(extensionName);
}

struct AttrInfo { EGLint attr; const char *name; };
static struct AttrInfo attrs[] = {
    {EGL_BUFFER_SIZE, "EGL_BUFFER_SIZE"},
    {EGL_ALPHA_SIZE, "EGL_ALPHA_SIZE"},
    {EGL_BLUE_SIZE, "EGL_BLUE_SIZE"},
    {EGL_GREEN_SIZE, "EGL_GREEN_SIZE"},
    {EGL_RED_SIZE, "EGL_RED_SIZE"},
    {EGL_DEPTH_SIZE, "EGL_DEPTH_SIZE"},
    {EGL_STENCIL_SIZE, "EGL_STENCIL_SIZE"},
    {EGL_CONFIG_CAVEAT, "EGL_CONFIG_CAVEAT"},
    {EGL_CONFIG_ID, "EGL_CONFIG_ID"},
    {EGL_LEVEL, "EGL_LEVEL"},
    {EGL_MAX_PBUFFER_HEIGHT, "EGL_MAX_PBUFFER_HEIGHT"},
    {EGL_MAX_PBUFFER_PIXELS, "EGL_MAX_PBUFFER_PIXELS"},
    {EGL_MAX_PBUFFER_WIDTH, "EGL_MAX_PBUFFER_WIDTH"},
    {EGL_NATIVE_RENDERABLE, "EGL_NATIVE_RENDERABLE"},
    {EGL_NATIVE_VISUAL_ID, "EGL_NATIVE_VISUAL_ID"},
    {EGL_NATIVE_VISUAL_TYPE, "EGL_NATIVE_VISUAL_TYPE"},
    {EGL_SAMPLES, "EGL_SAMPLES"},
    {EGL_SAMPLE_BUFFERS, "EGL_SAMPLE_BUFFERS"},
    {EGL_SURFACE_TYPE, "EGL_SURFACE_TYPE"},
    {EGL_TRANSPARENT_TYPE, "EGL_TRANSPARENT_TYPE"},
    {EGL_TRANSPARENT_BLUE_VALUE, "EGL_TRANSPARENT_BLUE_VALUE"},
    {EGL_TRANSPARENT_GREEN_VALUE, "EGL_TRANSPARENT_GREEN_VALUE"},
    {EGL_TRANSPARENT_RED_VALUE, "EGL_TRANSPARENT_RED_VALUE"},
    {EGL_BIND_TO_TEXTURE_RGB, "EGL_BIND_TO_TEXTURE_RGB"},
    {EGL_BIND_TO_TEXTURE_RGBA, "EGL_BIND_TO_TEXTURE_RGBA"},
    {EGL_MIN_SWAP_INTERVAL, "EGL_MIN_SWAP_INTERVAL"},
    {EGL_MAX_SWAP_INTERVAL, "EGL_MAX_SWAP_INTERVAL"},
    {-1, 0}};

void q_printEglConfig(EGLDisplay display, EGLConfig config)
{
    EGLint index;
    for (index = 0; attrs[index].attr != -1; ++index) {
        EGLint value;
        if (eglGetConfigAttrib(display, config, attrs[index].attr, &value)) {
            qDebug("\t%s: %d", attrs[index].name, (int)value);
        }
    }
}

#ifdef Q_OS_LINUX

QSizeF q_physicalScreenSizeFromFb(int framebufferDevice, const QSize &screenSize)
{
    const int defaultPhysicalDpi = 100;
    static QSizeF size;

    if (size.isEmpty()) {
        // Note: in millimeters
        int width = qgetenv("QT_QPA_EGLFS_PHYSICAL_WIDTH").toInt();
        int height = qgetenv("QT_QPA_EGLFS_PHYSICAL_HEIGHT").toInt();

        if (width && height) {
            size.setWidth(width);
            size.setHeight(height);
            return size;
        }

        struct fb_var_screeninfo vinfo;
        int w = -1;
        int h = -1;
        QSize screenResolution;

        if (framebufferDevice != -1) {
            if (ioctl(framebufferDevice, FBIOGET_VSCREENINFO, &vinfo) == -1) {
                qWarning("eglconvenience: Could not query screen info");
            } else {
                w = vinfo.width;
                h = vinfo.height;
                screenResolution = QSize(vinfo.xres, vinfo.yres);
            }
        } else {
            // Use the provided screen size, when available, since some platforms may have their own
            // specific way to query it. Otherwise try querying it from the framebuffer.
            screenResolution = screenSize.isEmpty() ? q_screenSizeFromFb(framebufferDevice) : screenSize;
        }

        size.setWidth(w <= 0 ? screenResolution.width() * Q_MM_PER_INCH / defaultPhysicalDpi : qreal(w));
        size.setHeight(h <= 0 ? screenResolution.height() * Q_MM_PER_INCH / defaultPhysicalDpi : qreal(h));
    }

    return size;
}

QSize q_screenSizeFromFb(int framebufferDevice)
{
    const int defaultWidth = 800;
    const int defaultHeight = 600;
    static QSize size;

    if (size.isEmpty()) {
        int width = qgetenv("QT_QPA_EGLFS_WIDTH").toInt();
        int height = qgetenv("QT_QPA_EGLFS_HEIGHT").toInt();

        if (width && height) {
            size.setWidth(width);
            size.setHeight(height);
            return size;
        }

        struct fb_var_screeninfo vinfo;
        int xres = -1;
        int yres = -1;

        if (framebufferDevice != -1) {
            if (ioctl(framebufferDevice, FBIOGET_VSCREENINFO, &vinfo) == -1) {
                qWarning("eglconvenience: Could not read screen info");
            } else {
                xres = vinfo.xres;
                yres = vinfo.yres;
            }
        }

        size.setWidth(xres <= 0 ? defaultWidth : xres);
        size.setHeight(yres <= 0 ? defaultHeight : yres);
    }

    return size;
}

int q_screenDepthFromFb(int framebufferDevice)
{
    const int defaultDepth = 32;
    static int depth = qgetenv("QT_QPA_EGLFS_DEPTH").toInt();

    if (depth == 0) {
        struct fb_var_screeninfo vinfo;

        if (framebufferDevice != -1) {
            if (ioctl(framebufferDevice, FBIOGET_VSCREENINFO, &vinfo) == -1)
                qWarning("eglconvenience: Could not query screen info");
            else
                depth = vinfo.bits_per_pixel;
        }

        if (depth <= 0)
            depth = defaultDepth;
    }

    return depth;
}

#endif // Q_OS_LINUX

QT_END_NAMESPACE
