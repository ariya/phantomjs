/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QSTANDARDPATHS_H
#define QSTANDARDPATHS_H

#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_STANDARDPATHS

class Q_CORE_EXPORT QStandardPaths
{
public:
    // Do not re-order, must match QDesktopServices
    enum StandardLocation {
        DesktopLocation,
        DocumentsLocation,
        FontsLocation,
        ApplicationsLocation,
        MusicLocation,
        MoviesLocation,
        PicturesLocation,
        TempLocation,
        HomeLocation,
        DataLocation,
        CacheLocation,
        GenericDataLocation,
        RuntimeLocation,
        ConfigLocation,
        DownloadLocation,
        GenericCacheLocation,
        GenericConfigLocation,
        AppDataLocation,
        AppLocalDataLocation = DataLocation
    };

    static QString writableLocation(StandardLocation type);
    static QStringList standardLocations(StandardLocation type);

    enum LocateOption {
        LocateFile = 0x0,
        LocateDirectory = 0x1
    };
    Q_DECLARE_FLAGS(LocateOptions, LocateOption)

    static QString locate(StandardLocation type, const QString &fileName, LocateOptions options = LocateFile);
    static QStringList locateAll(StandardLocation type, const QString &fileName, LocateOptions options = LocateFile);
#ifndef QT_BOOTSTRAPPED
    static QString displayName(StandardLocation type);
#endif

    static QString findExecutable(const QString &executableName, const QStringList &paths = QStringList());

#if QT_DEPRECATED_SINCE(5, 2)
    static QT_DEPRECATED void enableTestMode(bool testMode);
#endif
    static void setTestModeEnabled(bool testMode);
    static bool isTestModeEnabled();

private:
    // prevent construction
    QStandardPaths();
    ~QStandardPaths();
};

#endif // QT_NO_STANDARDPATHS

QT_END_NAMESPACE

#endif // QSTANDARDPATHS_H
