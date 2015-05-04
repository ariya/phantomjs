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

#ifndef QFILESYSTEMWATCHER_P_H
#define QFILESYSTEMWATCHER_P_H

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

#include "qfilesystemwatcher.h"

#ifndef QT_NO_FILESYSTEMWATCHER

#include <private/qobject_p.h>

#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class QFileSystemWatcherEngine : public QObject
{
    Q_OBJECT

protected:
    inline QFileSystemWatcherEngine(QObject *parent)
        : QObject(parent)
    {
    }

public:
    // fills \a files and \a directories with the \a paths it could
    // watch, and returns a list of paths this engine could not watch
    virtual QStringList addPaths(const QStringList &paths,
                                 QStringList *files,
                                 QStringList *directories) = 0;
    // removes \a paths from \a files and \a directories, and returns
    // a list of paths this engine does not know about (either addPath
    // failed or wasn't called)
    virtual QStringList removePaths(const QStringList &paths,
                                    QStringList *files,
                                    QStringList *directories) = 0;

Q_SIGNALS:
    void fileChanged(const QString &path, bool removed);
    void directoryChanged(const QString &path, bool removed);
};

class QFileSystemWatcherPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QFileSystemWatcher)

    static QFileSystemWatcherEngine *createNativeEngine(QObject *parent);

public:
    QFileSystemWatcherPrivate();
    void init();
    void initPollerEngine();

    QFileSystemWatcherEngine *native, *poller;
    QStringList files, directories;

    // private slots
    void _q_fileChanged(const QString &path, bool removed);
    void _q_directoryChanged(const QString &path, bool removed);
};


QT_END_NAMESPACE
#endif // QT_NO_FILESYSTEMWATCHER
#endif // QFILESYSTEMWATCHER_P_H
