/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qplatformservices.h"

#include <QtCore/QUrl>
#include <QtCore/QString>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformServices
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformServices provides the backend for desktop-related functionality.
*/

bool QPlatformServices::openUrl(const QUrl &url)
{
    qWarning("This plugin does not support QPlatformServices::openUrl() for '%s'.",
             qPrintable(url.toString()));
    return false;
}

bool QPlatformServices::openDocument(const QUrl &url)
{
    qWarning("This plugin does not support QPlatformServices::openDocument() for '%s'.",
             qPrintable(url.toString()));
    return false;
}

/*!
 * \brief QPlatformServices::desktopEnvironment returns the active desktop environment.
 *
 * On Unix this function returns the uppercase desktop environment name, such as
 * KDE, GNOME, UNITY, XFCE, LXDE etc. or UNKNOWN if none was detected.
 * The primary way to detect the desktop environment is the environment variable
 * XDG_CURRENT_DESKTOP.
 */
QByteArray QPlatformServices::desktopEnvironment() const
{
    return QByteArray("UNKNOWN");
}


QT_END_NAMESPACE
