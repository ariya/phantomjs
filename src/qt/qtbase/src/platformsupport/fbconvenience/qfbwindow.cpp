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

#include "qfbwindow_p.h"
#include "qfbscreen_p.h"

#include <QtGui/QScreen>
#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

QFbWindow::QFbWindow(QWindow *window)
    : QPlatformWindow(window), mBackingStore(0), mWindowState(Qt::WindowNoState)
{
    static QAtomicInt winIdGenerator(1);
    mWindowId = winIdGenerator.fetchAndAddRelaxed(1);
}

QFbWindow::~QFbWindow()
{
}

QFbScreen *QFbWindow::platformScreen() const
{
    return static_cast<QFbScreen *>(window()->screen()->handle());
}

void QFbWindow::setGeometry(const QRect &rect)
{
    // store previous geometry for screen update
    mOldGeometry = geometry();

    platformScreen()->invalidateRectCache();
    QWindowSystemInterface::handleGeometryChange(window(), rect);

    QPlatformWindow::setGeometry(rect);
}

void QFbWindow::setVisible(bool visible)
{
    if (visible) {
        if (mWindowState & Qt::WindowFullScreen)
            setGeometry(platformScreen()->geometry());
        else if (mWindowState & Qt::WindowMaximized)
            setGeometry(platformScreen()->availableGeometry());
    }
    QPlatformWindow::setVisible(visible);

    if (visible)
        platformScreen()->addWindow(this);
    else
        platformScreen()->removeWindow(this);
}


void QFbWindow::setWindowState(Qt::WindowState state)
{
    QPlatformWindow::setWindowState(state);
    mWindowState = state;
    platformScreen()->invalidateRectCache();
}


void QFbWindow::setWindowFlags(Qt::WindowFlags flags)
{
    mWindowFlags = flags;
    platformScreen()->invalidateRectCache();
}

Qt::WindowFlags QFbWindow::windowFlags() const
{
    return mWindowFlags;
}

void QFbWindow::raise()
{
    platformScreen()->raise(this);
}

void QFbWindow::lower()
{
    platformScreen()->lower(this);
}

void QFbWindow::repaint(const QRegion &region)
{
    QRect currentGeometry = geometry();

    QRect dirtyClient = region.boundingRect();
    QRect dirtyRegion(currentGeometry.left() + dirtyClient.left(),
                      currentGeometry.top() + dirtyClient.top(),
                      dirtyClient.width(),
                      dirtyClient.height());
    QRect mOldGeometryLocal = mOldGeometry;
    mOldGeometry = currentGeometry;
    // If this is a move, redraw the previous location
    if (mOldGeometryLocal != currentGeometry)
        platformScreen()->setDirty(mOldGeometryLocal);
    platformScreen()->setDirty(dirtyRegion);
}

QT_END_NAMESPACE

