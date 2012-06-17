/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QLIBRARY_P_H
#define QLIBRARY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#ifdef Q_WS_WIN
# include "QtCore/qt_windows.h"
#endif
#include "QtCore/qlibrary.h"
#include "QtCore/qpointer.h"
#include "QtCore/qstringlist.h"
#include "QtCore/qplugin.h"
#include "QtCore/qsharedpointer.h"

#ifndef QT_NO_LIBRARY

QT_BEGIN_NAMESPACE

bool qt_debug_component();

class QSettings;
class QLibraryPrivate
{
public:

#ifdef Q_WS_WIN
    HINSTANCE
#else
    void *
#endif
    pHnd;

    QString fileName, qualifiedFileName;
    QString fullVersion;

    bool load();
    bool loadPlugin(); // loads and resolves instance
    bool unload();
    void release();
    void *resolve(const char *);

    static QLibraryPrivate *findOrCreate(const QString &fileName, const QString &version = QString());

    QWeakPointer<QObject> inst;
    QtPluginInstanceFunction instance;
    uint qt_version;
    QString lastModified;

    QString errorString;
    QLibrary::LoadHints loadHints;

    bool isPlugin(QSettings *settings = 0);


private:
    explicit QLibraryPrivate(const QString &canonicalFileName, const QString &version);
    ~QLibraryPrivate();

    bool load_sys();
    bool unload_sys();
    void *resolve_sys(const char *);

    QAtomicInt libraryRefCount;
    QAtomicInt libraryUnloadCount;

    enum {IsAPlugin, IsNotAPlugin, MightBeAPlugin } pluginState;
    friend class QLibraryPrivateHasFriends;
};

QT_END_NAMESPACE

#endif // QT_NO_LIBRARY

#endif // QLIBRARY_P_H
