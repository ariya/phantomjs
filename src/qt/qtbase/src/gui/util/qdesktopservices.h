/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QDESKTOPSERVICES_H
#define QDESKTOPSERVICES_H

#include <QtCore/qstring.h>
#include <QtCore/qstandardpaths.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_DESKTOPSERVICES

class QStringList;
class QUrl;
class QObject;

class Q_GUI_EXPORT QDesktopServices
{
public:
    static bool openUrl(const QUrl &url);
    static void setUrlHandler(const QString &scheme, QObject *receiver, const char *method);
    static void unsetUrlHandler(const QString &scheme);

#if QT_DEPRECATED_SINCE(5, 0)
    //Must match QStandardPaths::StandardLocation
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
        CacheLocation
    };

    QT_DEPRECATED static QString storageLocation(StandardLocation type) {
        return storageLocationImpl(static_cast<QStandardPaths::StandardLocation>(type));
    }
    QT_DEPRECATED static QString displayName(StandardLocation type) {
        return QStandardPaths::displayName(static_cast<QStandardPaths::StandardLocation>(type));
    }
#endif
private:
    static QString storageLocationImpl(QStandardPaths::StandardLocation type);
};

#endif // QT_NO_DESKTOPSERVICES

QT_END_NAMESPACE

#endif // QDESKTOPSERVICES_H

