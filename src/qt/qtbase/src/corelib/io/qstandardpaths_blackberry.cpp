/***************************************************************************
**
** Copyright (C) 2012 Research In Motion
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qstandardpaths.h"
#include <qdir.h>

#ifndef QT_NO_STANDARDPATHS

#include <qstring.h>

QT_BEGIN_NAMESPACE

static QString testModeInsert() {
    if (QStandardPaths::isTestModeEnabled())
        return QStringLiteral("/.qttest");
    else
        return QStringLiteral("");
}

QString QStandardPaths::writableLocation(StandardLocation type)
{
    QDir sharedDir = QDir::home();
    sharedDir.cd(QLatin1String("../shared"));

    const QString sharedRoot = sharedDir.absolutePath();

    switch (type) {
    case DataLocation:
        return QDir::homePath() + testModeInsert();
    case DesktopLocation:
    case HomeLocation:
        return QDir::homePath();
    case RuntimeLocation:
    case TempLocation:
        return QDir::tempPath();
    case CacheLocation:
    case GenericCacheLocation:
        return QDir::homePath() + testModeInsert() + QLatin1String("/Cache");
    case ConfigLocation:
    case GenericConfigLocation:
        return QDir::homePath() + testModeInsert() + QLatin1String("/Settings");
    case GenericDataLocation:
        return sharedRoot + testModeInsert() + QLatin1String("/misc");
    case DocumentsLocation:
        return sharedRoot + QLatin1String("/documents");
    case PicturesLocation:
        return sharedRoot + QLatin1String("/photos");
    case FontsLocation:
        // this is not a writable location
        return QString();
    case MusicLocation:
        return sharedRoot + QLatin1String("/music");
    case MoviesLocation:
        return sharedRoot + QLatin1String("/videos");
    case DownloadLocation:
        return sharedRoot + QLatin1String("/downloads");
    case ApplicationsLocation:
        return QString();
    default:
        break;
    }

    return QString();
}

QStringList QStandardPaths::standardLocations(StandardLocation type)
{
    QStringList dirs;

    if (type == FontsLocation)
        return QStringList(QLatin1String("/base/usr/fonts"));

    if (type == DataLocation)
        dirs.append(QDir::homePath() + testModeInsert() + QLatin1String("native/assets"));

    const QString localDir = writableLocation(type);
    dirs.prepend(localDir);
    return dirs;
}

QT_END_NAMESPACE

#endif // QT_NO_STANDARDPATHS
