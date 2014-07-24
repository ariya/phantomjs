/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake spec of the Qt Toolkit.
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

#include "qeglfshooks.h"
#include <QtPlatformSupport/private/qeglplatformcursor_p.h>
#include <QtPlatformSupport/private/qeglconvenience_p.h>
#include <QtCore/QRegularExpression>

#if defined(Q_OS_LINUX)
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#endif

#include <private/qcore_unix_p.h>

QT_BEGIN_NAMESPACE

// file descriptor for the frame buffer
// this is a global static to keep the QEglFSHooks interface as clean as possible
static int framebuffer = -1;

QByteArray QEglFSHooks::fbDeviceName() const
{
    QByteArray fbDev = qgetenv("QT_QPA_EGLFS_FB");
    if (fbDev.isEmpty())
        fbDev = QByteArrayLiteral("/dev/fb0");

    return fbDev;
}

int QEglFSHooks::framebufferIndex() const
{
    int fbIndex = 0;
    QRegularExpression fbIndexRx(QLatin1String("fb(\\d+)"));
    QRegularExpressionMatch match = fbIndexRx.match(fbDeviceName());
    if (match.hasMatch())
        fbIndex = match.captured(1).toInt();

    return fbIndex;
}

void QEglFSHooks::platformInit()
{
    QByteArray fbDev = fbDeviceName();

    framebuffer = qt_safe_open(fbDev, O_RDONLY);

    if (framebuffer == -1)
        qWarning("EGLFS: Failed to open %s", qPrintable(fbDev));
}

void QEglFSHooks::platformDestroy()
{
    if (framebuffer != -1)
        close(framebuffer);
}

EGLNativeDisplayType QEglFSHooks::platformDisplay() const
{
    return EGL_DEFAULT_DISPLAY;
}

QSizeF QEglFSHooks::physicalScreenSize() const
{
    return q_physicalScreenSizeFromFb(framebuffer, screenSize());
}

QSize QEglFSHooks::screenSize() const
{
    return q_screenSizeFromFb(framebuffer);
}

QDpi QEglFSHooks::logicalDpi() const
{
    QSizeF ps = physicalScreenSize();
    QSize s = screenSize();

    return QDpi(25.4 * s.width() / ps.width(),
                25.4 * s.height() / ps.height());
}

Qt::ScreenOrientation QEglFSHooks::nativeOrientation() const
{
    return Qt::PrimaryOrientation;
}

Qt::ScreenOrientation QEglFSHooks::orientation() const
{
    return Qt::PrimaryOrientation;
}

int QEglFSHooks::screenDepth() const
{
    return q_screenDepthFromFb(framebuffer);
}

QImage::Format QEglFSHooks::screenFormat() const
{
    return screenDepth() == 16 ? QImage::Format_RGB16 : QImage::Format_RGB32;
}

QSurfaceFormat QEglFSHooks::surfaceFormatFor(const QSurfaceFormat &inputFormat) const
{
    QSurfaceFormat format = inputFormat;

    static const bool force888 = qgetenv("QT_QPA_EGLFS_FORCE888").toInt();
    if (force888) {
        format.setRedBufferSize(8);
        format.setGreenBufferSize(8);
        format.setBlueBufferSize(8);
    }

    return format;
}

bool QEglFSHooks::filterConfig(EGLDisplay, EGLConfig) const
{
    return true;
}

EGLNativeWindowType QEglFSHooks::createNativeWindow(QPlatformWindow *platformWindow,
                                                    const QSize &size,
                                                    const QSurfaceFormat &format)
{
    Q_UNUSED(platformWindow);
    Q_UNUSED(size);
    Q_UNUSED(format);
    return 0;
}

void QEglFSHooks::destroyNativeWindow(EGLNativeWindowType window)
{
    Q_UNUSED(window);
}

bool QEglFSHooks::hasCapability(QPlatformIntegration::Capability cap) const
{
    Q_UNUSED(cap);
    return false;
}

QEGLPlatformCursor *QEglFSHooks::createCursor(QPlatformScreen *screen) const
{
    return new QEGLPlatformCursor(screen);
}

void QEglFSHooks::waitForVSync() const
{
#if defined(FBIO_WAITFORVSYNC)
    static const bool forceSync = qgetenv("QT_QPA_EGLFS_FORCEVSYNC").toInt();
    if (forceSync && framebuffer != -1) {
        int arg = 0;
        if (ioctl(framebuffer, FBIO_WAITFORVSYNC, &arg) == -1)
            qWarning("Could not wait for vsync.");
    }
#endif
}

#ifndef EGLFS_PLATFORM_HOOKS
QEglFSHooks stubHooks;
#endif

QT_END_NAMESPACE
