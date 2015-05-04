/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qsystemtrayicon_p.h"

#include <QtGui/qpa/qplatformsystemtrayicon.h>
#include <qpa/qplatformtheme.h>
#include <private/qguiapplication_p.h>

#include <QApplication>
#include <QStyle>

#ifndef QT_NO_SYSTEMTRAYICON

QT_BEGIN_NAMESPACE

QSystemTrayIconPrivate::QSystemTrayIconPrivate()
    : qpa_sys(QGuiApplicationPrivate::platformTheme()->createPlatformSystemTrayIcon())
    , visible(false)
{
}

QSystemTrayIconPrivate::~QSystemTrayIconPrivate()
{
    delete qpa_sys;
}

void QSystemTrayIconPrivate::install_sys()
{
    if (qpa_sys)
        install_sys_qpa();
}

void QSystemTrayIconPrivate::remove_sys()
{
    if (qpa_sys)
        remove_sys_qpa();
}

QRect QSystemTrayIconPrivate::geometry_sys() const
{
    if (qpa_sys)
        return geometry_sys_qpa();
    else
        return QRect();
}

void QSystemTrayIconPrivate::updateIcon_sys()
{
    if (qpa_sys)
        updateIcon_sys_qpa();
}

void QSystemTrayIconPrivate::updateMenu_sys()
{
    if (qpa_sys)
        updateMenu_sys_qpa();
}

void QSystemTrayIconPrivate::updateToolTip_sys()
{
    if (qpa_sys)
        updateToolTip_sys_qpa();
}

bool QSystemTrayIconPrivate::isSystemTrayAvailable_sys()
{
    QScopedPointer<QPlatformSystemTrayIcon> sys(QGuiApplicationPrivate::platformTheme()->createPlatformSystemTrayIcon());
    if (sys)
        return sys->isSystemTrayAvailable();
    else
        return false;
}

bool QSystemTrayIconPrivate::supportsMessages_sys()
{
    QScopedPointer<QPlatformSystemTrayIcon> sys(QGuiApplicationPrivate::platformTheme()->createPlatformSystemTrayIcon());
    if (sys)
        return sys->supportsMessages();
    else
        return false;
}

void QSystemTrayIconPrivate::showMessage_sys(const QString &message,
                                             const QString &title,
                                             QSystemTrayIcon::MessageIcon icon,
                                             int msecs)
{
    if (qpa_sys)
        showMessage_sys_qpa(message, title, icon, msecs);
}

QT_END_NAMESPACE

#endif // QT_NO_SYSTEMTRAYICON
