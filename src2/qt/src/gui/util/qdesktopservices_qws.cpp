/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include <qcoreapplication.h>
#include <qdir.h>

QT_BEGIN_NAMESPACE

static bool launchWebBrowser(const QUrl &url)
{
    Q_UNUSED(url);
    qWarning("QDesktopServices::launchWebBrowser not implemented");
    return false;
}

static bool openDocument(const QUrl &file)
{
    Q_UNUSED(file);
    qWarning("QDesktopServices::openDocument not implemented");
    return false;
}


QString QDesktopServices::storageLocation(StandardLocation type)
{
    if (type == QDesktopServices::HomeLocation)
        return QDir::homePath();
    if (type == QDesktopServices::TempLocation)
        return QDir::tempPath();

    if (type == DataLocation) {
        QString qwsDataHome = QLatin1String(qgetenv("QWS_DATA_HOME"));
        if (qwsDataHome.isEmpty())
            qwsDataHome = QDir::homePath() + QLatin1String("/.qws/share");
        qwsDataHome += QLatin1String("/data/")
                    + QCoreApplication::organizationName() + QLatin1Char('/')
                    + QCoreApplication::applicationName();
        return qwsDataHome;
    }
    if (type == QDesktopServices::CacheLocation) {
        QString qwsCacheHome = QLatin1String(qgetenv("QWS_CACHE_HOME"));
        if (qwsCacheHome.isEmpty())
            qwsCacheHome = QDir::homePath() + QLatin1String("/.qws/cache/");
        qwsCacheHome += QCoreApplication::organizationName() +  QLatin1Char('/')
                    + QCoreApplication::applicationName();
        return qwsCacheHome;
    }

    qWarning("QDesktopServices::storageLocation %d not implemented", type);
    return QString();
}

QString QDesktopServices::displayName(StandardLocation type)
{
    Q_UNUSED(type);
    qWarning("QDesktopServices::displayName not implemented");
    return QString();
}

QT_END_NAMESPACE
