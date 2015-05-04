/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake spec of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglfshooks.h"
#include <EGL/fbdev_window.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include <private/qcore_unix_p.h>

QT_BEGIN_NAMESPACE

class QEglFSHiX5Hd2Hooks : public QEglFSHooks
{
private:
    void fbInit();
public:
    void platformInit() Q_DECL_OVERRIDE;
    EGLNativeWindowType createNativeWindow(QPlatformWindow *window, const QSize &size, const QSurfaceFormat &format) Q_DECL_OVERRIDE;
    void destroyNativeWindow(EGLNativeWindowType window) Q_DECL_OVERRIDE;
};

void QEglFSHiX5Hd2Hooks::fbInit()
{
    int fd = qt_safe_open("/dev/fb0", O_RDWR, 0);
    if (fd == -1)
        qWarning("Failed to open fb to detect screen resolution!");

    struct fb_var_screeninfo vinfo;
    memset(&vinfo, 0, sizeof(vinfo));
    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == -1)
        qWarning("Could not get variable screen info");

    vinfo.bits_per_pixel   = 32;
    vinfo.red.length       = 8;
    vinfo.green.length     = 8;
    vinfo.blue.length      = 8;
    vinfo.transp.length    = 8;
    vinfo.blue.offset      = 0;
    vinfo.green.offset     = 8;
    vinfo.red.offset       = 16;
    vinfo.transp.offset    = 24;
    vinfo.yres_virtual     = 2 * vinfo.yres;

    if (ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo) == -1)
        qErrnoWarning(errno, "Unable to set double buffer mode!");

    qt_safe_close(fd);
    return;
}

void QEglFSHiX5Hd2Hooks::platformInit()
{
    QEglFSHooks::platformInit();
    fbInit();
}

EGLNativeWindowType QEglFSHiX5Hd2Hooks::createNativeWindow(QPlatformWindow *window, const QSize &size, const QSurfaceFormat &format)
{
    fbdev_window *fbwin = reinterpret_cast<fbdev_window *>(malloc(sizeof(fbdev_window)));
    if (NULL == fbwin)
        return 0;

    fbwin->width = size.width();
    fbwin->height = size.height();
    return (EGLNativeWindowType)fbwin;
}

void QEglFSHiX5Hd2Hooks::destroyNativeWindow(EGLNativeWindowType window)
{
    free(window);
}

QEglFSHiX5Hd2Hooks eglFSHiX5Hd2Hooks;
QEglFSHooks *platformHooks = &eglFSHiX5Hd2Hooks;

QT_END_NAMESPACE
