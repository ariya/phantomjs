/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qandroidplatformforeignwindow.h"
#include "androidjnimain.h"
#include <QtCore/qvariant.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/private/qjnihelpers_p.h>

QT_BEGIN_NAMESPACE

QAndroidPlatformForeignWindow::QAndroidPlatformForeignWindow(QWindow *window)
    : QAndroidPlatformWindow(window),
      m_surfaceId(-1)
{
    const WId wId = window->property("_q_foreignWinId").value<WId>();
    m_view = reinterpret_cast<jobject>(wId);
}

QAndroidPlatformForeignWindow::~QAndroidPlatformForeignWindow()
{
    if (m_surfaceId != -1)
        QtAndroid::destroySurface(m_surfaceId);
}

void QAndroidPlatformForeignWindow::lower()
{
    if (m_surfaceId == -1)
        return;

    QAndroidPlatformWindow::lower();
    QtAndroid::bringChildToBack(m_surfaceId);
}

void QAndroidPlatformForeignWindow::raise()
{
    if (m_surfaceId == -1)
        return;

    QAndroidPlatformWindow::raise();
    QtAndroid::bringChildToFront(m_surfaceId);
}

void QAndroidPlatformForeignWindow::setGeometry(const QRect &rect)
{
    QWindow *parent = window()->parent();
    QRect newGeometry = rect;

    if (parent != 0)
        newGeometry.moveTo(parent->mapToGlobal(rect.topLeft()));

    if (newGeometry == geometry())
        return;

    QAndroidPlatformWindow::setGeometry(newGeometry);

    if (m_surfaceId != -1)
        QtAndroid::setSurfaceGeometry(m_surfaceId, newGeometry);
}

void QAndroidPlatformForeignWindow::setVisible(bool visible)
{
    if (!m_view.isValid())
        return;

    QAndroidPlatformWindow::setVisible(visible);

    if (!visible && m_surfaceId != -1) {
        QtAndroid::destroySurface(m_surfaceId);
        m_surfaceId = -1;
    } else if (m_surfaceId == -1) {
        m_surfaceId = QtAndroid::insertNativeView(m_view.object(), geometry());
    }
}

void QAndroidPlatformForeignWindow::applicationStateChanged(Qt::ApplicationState state)
{
    if (state <= Qt::ApplicationHidden
            && QtAndroid::blockEventLoopsWhenSuspended()
            && m_surfaceId != -1) {
        QtAndroid::destroySurface(m_surfaceId);
        m_surfaceId = -1;
    } else if (m_view.isValid() && m_surfaceId == -1){
        m_surfaceId = QtAndroid::insertNativeView(m_view.object(), geometry());
    }

    QAndroidPlatformWindow::applicationStateChanged(state);
}

void QAndroidPlatformForeignWindow::setParent(const QPlatformWindow *window)
{
    QRect newGeometry = geometry();

    if (window != 0)
        newGeometry.moveTo(window->mapToGlobal(geometry().topLeft()));

    if (newGeometry != geometry())
        QAndroidPlatformWindow::setGeometry(newGeometry);

    if (m_surfaceId == -1)
        return;

    QtAndroid::setSurfaceGeometry(m_surfaceId, newGeometry);
}

QT_END_NAMESPACE
