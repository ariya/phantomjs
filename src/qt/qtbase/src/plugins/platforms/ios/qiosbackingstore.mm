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

#include "qiosbackingstore.h"
#include "qioswindow.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLPaintDevice>

#include <QtDebug>

QIOSBackingStore::QIOSBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
    , m_context(new QOpenGLContext)
    , m_device(0)
{
    QSurfaceFormat fmt = window->requestedFormat();
    fmt.setDepthBufferSize(16);
    fmt.setStencilBufferSize(8);

    // Needed to prevent QOpenGLContext::makeCurrent() from failing
    window->setSurfaceType(QSurface::OpenGLSurface);

    m_context->setFormat(fmt);
    m_context->setScreen(window->screen());
    m_context->create();
}

QIOSBackingStore::~QIOSBackingStore()
{
    delete m_context;
    delete m_device;
}

void QIOSBackingStore::beginPaint(const QRegion &)
{
    m_context->makeCurrent(window());

    QIOSWindow *iosWindow = static_cast<QIOSWindow *>(window()->handle());
    static_cast<QOpenGLPaintDevice *>(paintDevice())->setSize(window()->size() * iosWindow->devicePixelRatio());
}

QPaintDevice *QIOSBackingStore::paintDevice()
{
    if (!m_device) {
        QIOSWindow *iosWindow = static_cast<QIOSWindow *>(window()->handle());
        QOpenGLPaintDevice *openGLDevice = new QOpenGLPaintDevice(window()->size() * iosWindow->devicePixelRatio());
        openGLDevice->setDevicePixelRatio(iosWindow->devicePixelRatio());
        m_device = openGLDevice;
    }

    return m_device;
}

void QIOSBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region);
    Q_UNUSED(offset);

    if (window != this->window()) {
        // We skip flushing raster-based child windows, to avoid the extra cost of copying from the
        // parent FBO into the child FBO. Since the child is already drawn inside the parent FBO, it
        // will become visible when flushing the parent. The only case we end up not supporting is if
        // the child window overlaps a sibling window that's draws using a separate QOpenGLContext.
        return;
    }

    m_context->makeCurrent(window);
    m_context->swapBuffers(window);
}

void QIOSBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents);

    // Resizing the backing store would in our case mean resizing the QWindow,
    // as we cheat and use an QOpenGLPaintDevice that we target at the window.
    // That's probably not what the user intended, so we ignore resizes of the
    // backing store and always keep the paint device's size in sync with the
    // window size in beginPaint().

    if (size != window()->size() && !window()->inherits("QWidgetWindow"))
        qWarning() << "QIOSBackingStore needs to have the same size as its window";
}

QT_END_NAMESPACE
