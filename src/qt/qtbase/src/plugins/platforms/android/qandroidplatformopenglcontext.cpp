/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
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

#include "qandroidplatformopenglcontext.h"
#include "qandroidplatformopenglwindow.h"
#include "qandroidplatformintegration.h"

#include <QSurface>
#include <QtGui/private/qopenglcontext_p.h>

QT_BEGIN_NAMESPACE

QAndroidPlatformOpenGLContext::QAndroidPlatformOpenGLContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display)
    :QEGLPlatformContext(format, share, display)
{
}

void QAndroidPlatformOpenGLContext::swapBuffers(QPlatformSurface *surface)
{
    QEGLPlatformContext::swapBuffers(surface);

    if (surface->surface()->surfaceClass() == QSurface::Window)
        static_cast<QAndroidPlatformOpenGLWindow *>(surface)->checkNativeSurface(eglConfig());
}

bool QAndroidPlatformOpenGLContext::needsFBOReadBackWorkaroud()
{
    static bool set = false;
    static bool needsWorkaround = false;

    if (!set) {
        const char *rendererString = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
        needsWorkaround =
                qstrcmp(rendererString, "Mali-400 MP") == 0
                || qstrcmp(rendererString, "Adreno (TM) 200") == 0
                || qstrcmp(rendererString, "GC1000 core") == 0;
        set = true;
    }

    return needsWorkaround;
}

bool QAndroidPlatformOpenGLContext::makeCurrent(QPlatformSurface *surface)
{
    bool ret = QEGLPlatformContext::makeCurrent(surface);
    QOpenGLContextPrivate *ctx_d = QOpenGLContextPrivate::get(context());

    const char *rendererString = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    if (rendererString != 0 && qstrncmp(rendererString, "Android Emulator", 16) == 0)
        ctx_d->workaround_missingPrecisionQualifiers = true;

    if (!ctx_d->workaround_brokenFBOReadBack && needsFBOReadBackWorkaroud())
        ctx_d->workaround_brokenFBOReadBack = true;

    return ret;
}

EGLSurface QAndroidPlatformOpenGLContext::eglSurfaceForPlatformSurface(QPlatformSurface *surface)
{
    if (surface->surface()->surfaceClass() == QSurface::Window)
        return static_cast<QAndroidPlatformOpenGLWindow *>(surface)->eglSurface(eglConfig());
    return EGL_NO_SURFACE;
}

QT_END_NAMESPACE
