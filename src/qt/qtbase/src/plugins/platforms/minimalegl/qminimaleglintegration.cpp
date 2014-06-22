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

#include "qminimaleglintegration.h"

#include "qminimaleglwindow.h"
#include "qminimaleglbackingstore.h"

#include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>
#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>

#include <qpa/qplatformwindow.h>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>

#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

QMinimalEglIntegration::QMinimalEglIntegration()
    : mFontDb(new QGenericUnixFontDatabase()), mScreen(new QMinimalEglScreen(EGL_DEFAULT_DISPLAY))
{
    screenAdded(mScreen);

#ifdef QEGL_EXTRA_DEBUG
    qWarning("QMinimalEglIntegration\n");
#endif
}

QMinimalEglIntegration::~QMinimalEglIntegration()
{
    delete mScreen;
}

bool QMinimalEglIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case OpenGL: return true;
    case ThreadedOpenGL: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QMinimalEglIntegration::createPlatformWindow(QWindow *window) const
{
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QMinimalEglIntegration::createPlatformWindow %p\n",window);
#endif
    QPlatformWindow *w = new QMinimalEglWindow(window);
    w->requestActivateWindow();
    return w;
}


QPlatformBackingStore *QMinimalEglIntegration::createPlatformBackingStore(QWindow *window) const
{
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QMinimalEglIntegration::createWindowSurface %p\n", window);
#endif
    return new QMinimalEglBackingStore(window);
}

QPlatformOpenGLContext *QMinimalEglIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    return static_cast<QMinimalEglScreen *>(context->screen()->handle())->platformContext();
}

QPlatformFontDatabase *QMinimalEglIntegration::fontDatabase() const
{
    return mFontDb;
}

QAbstractEventDispatcher *QMinimalEglIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QVariant QMinimalEglIntegration::styleHint(QPlatformIntegration::StyleHint hint) const
{
    if (hint == QPlatformIntegration::ShowIsFullScreen)
        return true;

    return QPlatformIntegration::styleHint(hint);
}

QT_END_NAMESPACE
