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

#include "qwindowseglcontext.h"
#include "qwindowscontext.h"
#include "qwindowswindow.h"

#include <QtCore/QDebug>
#include <QtGui/QOpenGLContext>

#include <QtPlatformSupport/private/qeglconvenience_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsEGLStaticContext
    \brief Static data for QWindowsEGLContext.

    Keeps the display. The class is shared via
    QSharedPointer in the windows, the contexts
    and in QWindowsIntegration. The display will
    be closed if the last instance is deleted.

    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsEGLStaticContext::QWindowsEGLStaticContext(EGLDisplay display, int version)
    : m_display(display), m_version(version)
{
}

QWindowsEGLStaticContext *QWindowsEGLStaticContext::create()
{
    const HDC dc = QWindowsContext::instance()->displayContext();
    if (!dc){
        qWarning("%s: No Display", Q_FUNC_INFO);
        return 0;
    }

    EGLDisplay display = eglGetDisplay((EGLNativeDisplayType)dc);
    if (!display) {
        qWarning("%s: Could not obtain EGL display", Q_FUNC_INFO);
        return 0;
    }

    EGLint major;
    EGLint minor;
    if (!eglInitialize(display, &major, &minor)) {
        qWarning("%s: Could not initialize egl display: error %d\n",
                 Q_FUNC_INFO, eglGetError());
        return 0;
    }

    qCDebug(lcQpaGl) << __FUNCTION__ << "Created EGL display" << display << 'v' <<major << '.' << minor;
    return new QWindowsEGLStaticContext(display, (major << 8) | minor);
}

QWindowsEGLStaticContext::~QWindowsEGLStaticContext()
{
    qCDebug(lcQpaGl) << __FUNCTION__ << "Releasing EGL display " << m_display;
    eglTerminate(m_display);
}

/*!
    \class QWindowsEGLContext
    \brief Open EGL context.

    \section1 Using QWindowsEGLContext for Desktop with ANGLE
    \section2 Build Instructions
    \list
    \o Install the Direct X SDK
    \o Checkout and build ANGLE (SVN repository) as explained here:
       \l{http://code.google.com/p/angleproject/wiki/DevSetup}{ANGLE-Project}.
       When building for 64bit, de-activate the "WarnAsError" option
       in every project file (as otherwise integer conversion
       warnings will break the build).
    \o Run configure.exe with the options "-opengl es2".
    \o Build qtbase and test some examples.
    \endlist

    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsEGLContext::QWindowsEGLContext(const QWindowsEGLStaticContextPtr &staticContext,
                                       const QSurfaceFormat &format,
                                       QPlatformOpenGLContext *share)
    : QEGLPlatformContext(format, share, staticContext->display())
    , m_staticContext(staticContext)
{
}

QWindowsEGLContext::~QWindowsEGLContext()
{
}

bool QWindowsEGLContext::hasThreadedOpenGLCapability()
{
    return false;
}

EGLSurface QWindowsEGLContext::eglSurfaceForPlatformSurface(QPlatformSurface *surface)
{
    const QWindowsWindow *window = static_cast<const QWindowsWindow *>(surface);
    return window->eglSurfaceHandle();
}

bool QWindowsEGLContext::makeCurrent(QPlatformSurface *surface)
{
    bool ok = false;
    QWindowsWindow *window = static_cast<QWindowsWindow *>(surface);
    if (EGLSurface eglSurface = window->ensureEglSurfaceHandle(m_staticContext, eglConfig())) {
        ok = eglMakeCurrent(eglDisplay(), eglSurface, eglSurface, eglContext());
        if (!ok)
            qWarning("%s: eglMakeCurrent() failed, eglError: 0x%x, this: %p \n",
                     Q_FUNC_INFO, eglGetError(), this);
    }
    return ok;
}

QT_END_NAMESPACE
