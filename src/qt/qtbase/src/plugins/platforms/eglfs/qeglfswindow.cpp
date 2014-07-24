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

#include "qeglfswindow.h"
#include "qeglfshooks.h"
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>
#include <QtGui/QOpenGLContext>
#include <QtPlatformSupport/private/qeglplatformcursor_p.h>
#include <QtPlatformSupport/private/qeglconvenience_p.h>

#include <QtDebug>

QT_BEGIN_NAMESPACE

QEglFSWindow::QEglFSWindow(QWindow *w)
    : QEGLPlatformWindow(w)
    , m_surface(0)
    , m_window(0)
    , m_flags(0)
{
}

QEglFSWindow::~QEglFSWindow()
{
    destroy();
}

void QEglFSWindow::create()
{
    if (m_flags.testFlag(Created))
        return;

    QEGLPlatformWindow::create();

    m_flags = Created;

    if (window()->type() == Qt::Desktop)
        return;

    // Stop if there is already a window backed by a native window and surface. Additional
    // raster windows will not have their own native window, surface and context. Instead,
    // they will be composited onto the root window's surface.
    QEglFSScreen *screen = this->screen();
    if (screen->primarySurface() != EGL_NO_SURFACE) {
        if (isRaster() && screen->compositingWindow())
            return;

#if !defined(Q_OS_ANDROID) || defined(Q_OS_ANDROID_NO_SDK)
        // We can have either a single OpenGL window or multiple raster windows.
        // Other combinations cannot work.
        qFatal("EGLFS: OpenGL windows cannot be mixed with others.");
#endif

        return;
    }

    m_flags |= HasNativeWindow;
    setGeometry(QRect()); // will become fullscreen
    QWindowSystemInterface::handleExposeEvent(window(), geometry());

    EGLDisplay display = static_cast<QEglFSScreen *>(screen)->display();
    QSurfaceFormat platformFormat = QEglFSHooks::hooks()->surfaceFormatFor(window()->requestedFormat());
    m_config = QEglFSIntegration::chooseConfig(display, platformFormat);
    m_format = q_glFormatFromConfig(display, m_config, platformFormat);

    resetSurface();

    screen->setPrimarySurface(m_surface);

    if (isRaster()) {
        QOpenGLContext *context = new QOpenGLContext(QGuiApplication::instance());
        context->setFormat(window()->requestedFormat());
        context->setScreen(window()->screen());
        if (!context->create())
            qFatal("EGLFS: Failed to create compositing context");
        screen->setRootContext(context);
        screen->setRootWindow(this);
    }
}

void QEglFSWindow::destroy()
{
    QEglFSScreen *screen = this->screen();
    if (m_flags.testFlag(HasNativeWindow)) {
        QEGLPlatformCursor *cursor = static_cast<QEGLPlatformCursor *>(screen->cursor());
        if (cursor)
            cursor->resetResources();

        if (screen->primarySurface() == m_surface)
            screen->setPrimarySurface(EGL_NO_SURFACE);

        invalidateSurface();
    }

    m_flags = 0;
    screen->removeWindow(this);
}

// The virtual functions resetSurface and invalidateSurface may get overridden
// in derived classes, for example in the Android port, to perform the native
// window and surface creation differently.

void QEglFSWindow::invalidateSurface()
{
    if (m_surface != EGL_NO_SURFACE) {
        EGLDisplay display = static_cast<QEglFSScreen *>(screen())->display();
        eglDestroySurface(display, m_surface);
        m_surface = EGL_NO_SURFACE;
    }
    QEglFSHooks::hooks()->destroyNativeWindow(m_window);
    m_window = 0;
}

void QEglFSWindow::resetSurface()
{
    EGLDisplay display = static_cast<QEglFSScreen *>(screen())->display();
    m_window = QEglFSHooks::hooks()->createNativeWindow(this, QEglFSHooks::hooks()->screenSize(), m_format);
    m_surface = eglCreateWindowSurface(display, m_config, m_window, NULL);
    if (m_surface == EGL_NO_SURFACE) {
        EGLint error = eglGetError();
        eglTerminate(display);
        qFatal("EGL Error : Could not create the egl surface: error = 0x%x\n", error);
    }
}

void QEglFSWindow::setVisible(bool visible)
{
    QList<QEGLPlatformWindow *> windows = screen()->windows();

    if (window()->type() != Qt::Desktop) {
        if (visible) {
            screen()->addWindow(this);
        } else {
            screen()->removeWindow(this);
            windows = screen()->windows();
            if (windows.size())
                windows.last()->requestActivateWindow();
        }
    }

    QWindowSystemInterface::handleExposeEvent(window(), window()->geometry());

    if (visible)
        QWindowSystemInterface::flushWindowSystemEvents();
}

void QEglFSWindow::setGeometry(const QRect &r)
{
    QRect rect;
    bool forceFullscreen = m_flags.testFlag(HasNativeWindow);
    if (forceFullscreen)
        rect = screen()->availableGeometry();
    else
        rect = r;

    QPlatformWindow::setGeometry(rect);

    // if we corrected the size, trigger a resize event
    if (rect != r)
        QWindowSystemInterface::handleGeometryChange(window(), rect, r);
}

QRect QEglFSWindow::geometry() const
{
    // For yet-to-become-fullscreen windows report the geometry covering the entire
    // screen. This is particularly important for Quick where the root object may get
    // sized to some geometry queried before calling create().
    if (!m_flags.testFlag(Created) && screen()->primarySurface() == EGL_NO_SURFACE)
        return screen()->availableGeometry();

    return QPlatformWindow::geometry();
}

void QEglFSWindow::requestActivateWindow()
{
    if (window()->type() != Qt::Desktop)
        screen()->moveToTop(this);

    QWindowSystemInterface::handleWindowActivated(window());
    QWindowSystemInterface::handleExposeEvent(window(), window()->geometry());
}

void QEglFSWindow::raise()
{
    if (window()->type() != Qt::Desktop) {
        screen()->moveToTop(this);
        QWindowSystemInterface::handleExposeEvent(window(), window()->geometry());
    }
}

void QEglFSWindow::lower()
{
    QList<QEGLPlatformWindow *> windows = screen()->windows();
    if (window()->type() != Qt::Desktop && windows.count() > 1) {
        int idx = windows.indexOf(this);
        if (idx > 0) {
            screen()->changeWindowIndex(this, idx - 1);
            QWindowSystemInterface::handleExposeEvent(windows.last()->window(), windows.last()->geometry());
        }
    }
}

EGLSurface QEglFSWindow::surface() const
{
    return m_surface != EGL_NO_SURFACE ? m_surface : screen()->primarySurface();
}

QSurfaceFormat QEglFSWindow::format() const
{
    return m_format;
}

EGLNativeWindowType QEglFSWindow::eglWindow() const
{
    return m_window;
}

QEglFSScreen *QEglFSWindow::screen() const
{
    return static_cast<QEglFSScreen *>(QPlatformWindow::screen());
}

QT_END_NAMESPACE
