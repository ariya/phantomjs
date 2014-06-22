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

#include <qpa/qwindowsysteminterface.h>

#include "qeglplatformwindow_p.h"
#include "qeglplatformbackingstore_p.h"
#include "qeglplatformscreen_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QEGLPlatformWindow
    \brief Base class for EGL-based platform window implementations.
    \since 5.2
    \internal
    \ingroup qpa

    Lightweight class providing some basic platform window operations
    and interfacing with QEGLPlatformBackingStore.

    Almost no QPlatformWindow functions are implemented here. This is
    intentional because different platform plugins may use different
    strategies for their window management (some may force fullscreen
    windows, some may not, some may share the underlying native
    surface, some may not, etc.) and therefore it is not sensible to
    enforce anything for these functions.

    \note Subclasses are responsible for invoking this class'
    implementation of create(). When using QEGLPlatformScreen, the
    subclasses of this class are expected to utilize the window stack
    management functions (addWindow() etc.) provided there.
 */

QEGLPlatformWindow::QEGLPlatformWindow(QWindow *w)
    : QPlatformWindow(w),
      m_winId(0)
{
}

static WId newWId()
{
    static WId id = 0;

    if (id == std::numeric_limits<WId>::max())
        qWarning("QEGLPlatformWindow: Out of window IDs");

    return ++id;
}

void QEGLPlatformWindow::create()
{
    m_winId = newWId();

    // Save the original surface type before changing to OpenGLSurface.
    m_raster = (window()->surfaceType() == QSurface::RasterSurface);
    if (m_raster) // change to OpenGL, but not for RasterGLSurface
        window()->setSurfaceType(QSurface::OpenGLSurface);

    if (window()->type() == Qt::Desktop) {
        QRect fullscreenRect(QPoint(), screen()->availableGeometry().size());
        QPlatformWindow::setGeometry(fullscreenRect);
        QWindowSystemInterface::handleGeometryChange(window(), fullscreenRect);
        return;
    }
}

bool QEGLPlatformWindow::isRaster() const
{
    return m_raster || window()->surfaceType() == QSurface::RasterGLSurface;
}

const QPlatformTextureList *QEGLPlatformWindow::textures() const
{
    if (m_backingStore)
        return m_backingStore->textures();

    return 0;
}

void QEGLPlatformWindow::composited()
{
    if (m_backingStore)
        m_backingStore->composited();
}

WId QEGLPlatformWindow::winId() const
{
    return m_winId;
}

QT_END_NAMESPACE
