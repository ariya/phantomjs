/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "qoffscreenintegration_x11.h"

#include <QByteArray>
#include <QOpenGLContext>

#include <X11/Xlib.h>
#include <GL/glx.h>

#include <QtPlatformSupport/private/qglxconvenience_p.h>

#include <qpa/qplatformsurface.h>
#include <qsurface.h>

QT_BEGIN_NAMESPACE

QOffscreenIntegration *QOffscreenIntegration::createOffscreenIntegration()
{
    return new QOffscreenX11Integration;
}

bool QOffscreenX11Integration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case OpenGL: return true;
    case ThreadedOpenGL: return true;
    default: return QOffscreenIntegration::hasCapability(cap);
    }
}

QPlatformOpenGLContext *QOffscreenX11Integration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    if (!m_connection)
        m_connection.reset(new QOffscreenX11Connection);

    return new QOffscreenX11GLXContext(m_connection->x11Info(), context);
}

QOffscreenX11Connection::QOffscreenX11Connection()
{
    XInitThreads();

    QByteArray displayName = qgetenv("DISPLAY");
    Display *display = XOpenDisplay(displayName.constData());
    m_display = display;
    m_screenNumber = DefaultScreen(display);
}

QOffscreenX11Connection::~QOffscreenX11Connection()
{
    XCloseDisplay((Display *)m_display);
}

class QOffscreenX11Info
{
public:
    QOffscreenX11Info(QOffscreenX11Connection *connection)
        : m_connection(connection)
    {
    }

    Display *display() const {
        return (Display *)m_connection->display();
    }

    Window root() const {
        return DefaultRootWindow(display());
    }

    int screenNumber() const {
        return m_connection->screenNumber();
    }

private:
    QOffscreenX11Connection *m_connection;
};

QOffscreenX11Info *QOffscreenX11Connection::x11Info()
{
    if (!m_x11Info)
        m_x11Info.reset(new QOffscreenX11Info(this));
    return m_x11Info.data();
}

class QOffscreenX11GLXContextData
{
public:
    QOffscreenX11Info *x11;
    QSurfaceFormat format;
    GLXContext context;
    GLXContext shareContext;
    Window window;
};

static Window createDummyWindow(QOffscreenX11Info *x11, XVisualInfo *visualInfo)
{
    Colormap cmap = XCreateColormap(x11->display(), x11->root(), visualInfo->visual, AllocNone);
    XSetWindowAttributes a;
    a.background_pixel = WhitePixel(x11->display(), x11->screenNumber());
    a.border_pixel = BlackPixel(x11->display(), x11->screenNumber());
    a.colormap = cmap;

    Window window = XCreateWindow(x11->display(), x11->root(),
                                  0, 0, 100, 100,
                                  0, visualInfo->depth, InputOutput, visualInfo->visual,
                                  CWBackPixel|CWBorderPixel|CWColormap, &a);
    XFreeColormap(x11->display(), cmap);
    return window;
}

static Window createDummyWindow(QOffscreenX11Info *x11, GLXFBConfig config)
{
    XVisualInfo *visualInfo = glXGetVisualFromFBConfig(x11->display(), config);
    if (!visualInfo)
        qFatal("Could not initialize GLX");
    Window window = createDummyWindow(x11, visualInfo);
    XFree(visualInfo);
    return window;
}

QOffscreenX11GLXContext::QOffscreenX11GLXContext(QOffscreenX11Info *x11, QOpenGLContext *context)
    : d(new QOffscreenX11GLXContextData)
{
    d->x11 = x11;
    d->format = context->format();

    d->shareContext = 0;
    if (context->shareHandle())
        d->shareContext = static_cast<QOffscreenX11GLXContext *>(context->shareHandle())->d->context;

    GLXFBConfig config = qglx_findConfig(x11->display(), x11->screenNumber(), d->format);
    if (config) {
        d->context = glXCreateNewContext(x11->display(), config, GLX_RGBA_TYPE, d->shareContext, true);
        if (!d->context && d->shareContext) {
            d->shareContext = 0;
            // re-try without a shared glx context
            d->context = glXCreateNewContext(x11->display(), config, GLX_RGBA_TYPE, 0, true);
        }

        // Get the basic surface format details
        if (d->context)
            qglx_surfaceFormatFromGLXFBConfig(&d->format, x11->display(), config, d->context);

        // Create a temporary window so that we can make the new context current
        d->window = createDummyWindow(x11, config);
    } else {
        XVisualInfo *visualInfo = qglx_findVisualInfo(x11->display(), 0, &d->format);
        if (!visualInfo)
            qFatal("Could not initialize GLX");
        d->context = glXCreateContext(x11->display(), visualInfo, d->shareContext, true);
        if (!d->context && d->shareContext) {
            // re-try without a shared glx context
            d->shareContext = 0;
            d->context = glXCreateContext(x11->display(), visualInfo, 0, true);
        }

        d->window = createDummyWindow(x11, visualInfo);
        XFree(visualInfo);
    }
}

QOffscreenX11GLXContext::~QOffscreenX11GLXContext()
{
    glXDestroyContext(d->x11->display(), d->context);
    XDestroyWindow(d->x11->display(), d->window);
}

bool QOffscreenX11GLXContext::makeCurrent(QPlatformSurface *surface)
{
    QSize size = surface->surface()->size();

    XResizeWindow(d->x11->display(), d->window, size.width(), size.height());
    XSync(d->x11->display(), true);

    if (glXMakeCurrent(d->x11->display(), d->window, d->context)) {
        glViewport(0, 0, size.width(), size.height());
        return true;
    }

    return false;
}

void QOffscreenX11GLXContext::doneCurrent()
{
    glXMakeCurrent(d->x11->display(), 0, 0);
}

void QOffscreenX11GLXContext::swapBuffers(QPlatformSurface *)
{
}

void (*QOffscreenX11GLXContext::getProcAddress(const QByteArray &procName)) ()
{
    return (void (*)())glXGetProcAddressARB(reinterpret_cast<const GLubyte *>(procName.constData()));
}

QSurfaceFormat QOffscreenX11GLXContext::format() const
{
    return d->format;
}

bool QOffscreenX11GLXContext::isSharing() const
{
    return d->shareContext;
}

bool QOffscreenX11GLXContext::isValid() const
{
    return d->context && d->window;
}

QT_END_NAMESPACE
