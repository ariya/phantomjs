/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QFILESYSTEMWATCHER_WIN_P_H
#define QFILESYSTEMWATCHER_WIN_P_H

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

#include "qfilesystemwatcher_p.h"

#ifndef QT_NO_FILESYSTEMWATCHER

#include <QtCore/qdatetime.h>
#include <QtCore/qthread.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qhash.h>
#include <QtCore/qmutex.h>
#include <QtCore/qvector.h>

QT_BEGIN_NAMESPACE

class QWindowsFileSystemWatcherEngineThread;

// Even though QWindowsFileSystemWatcherEngine is derived of QThread
// via QFileSystemWatcher, it does not start a thread.
// Instead QWindowsFileSystemWatcher creates QWindowsFileSystemWatcherEngineThreads
// to do the actually watching.
class QWindowsFileSystemWatcherEngine : public QFileSystemWatcherEngine
{
    Q_OBJECT
public:
    inline QWindowsFileSystemWatcherEngine(QObject *parent)
        : QFileSystemWatcherEngine(parent)
    { }
    ~QWindowsFileSystemWatcherEngine();

    QStringList addPaths(const QStringList &paths, QStringList *files, QStringList *directories);
    QStringList removePaths(const QStringList &paths, QStringList *files, QStringList *directories);

    class Handle
    {
    public:
        Qt::HANDLE handle;
        uint flags;

        Handle();
    };

    class PathInfo {
    public:
        QString absolutePath;
        QString path;
        bool isDir;

        // fileinfo bits
        uint ownerId;
        uint groupId;
        QFile::Permissions permissions;
        QDateTime lastModified;

        PathInfo &operator=(const QFileInfo &fileInfo)
                           {
            ownerId = fileInfo.ownerId();
            groupId = fileInfo.groupId();
            permissions = fileInfo.permissions();
            lastModified = fileInfo.lastModified();
            return *this;
        }

        bool operator!=(const QFileInfo &fileInfo) const
        {
            return (ownerId != fileInfo.ownerId()
                    || groupId != fileInfo.groupId()
                    || permissions != fileInfo.permissions()
                    || lastModified != fileInfo.lastModified());
        }
    };
private:
    QList<QWindowsFileSystemWatcherEngineThread *> threads;

};

class QFileSystemWatcherPathKey : public QString
{
public:
    QFileSystemWatcherPathKey() {}
    explicit QFileSystemWatcherPathKey(const QString &other) : QString(other) {}
    QFileSystemWatcherPathKey(const QFileSystemWatcherPathKey &other) : QString(other) {}
    bool operator==(const QFileSystemWatcherPathKey &other) const { return !compare(other, Qt::CaseInsensitive); }
};

Q_DECLARE_TYPEINFO(QFileSystemWatcherPathKey, Q_MOVABLE_TYPE);

inline uint qHash(const QFileSystemWatcherPathKey &key) { return qHash(key.toCaseFolded()); }

class QWindowsFileSystemWatcherEngineThread : public QThread
{
    Q_OBJECT

public:
    typedef QHash<QFileSystemWatcherPathKey, QWindowsFileSystemWatcherEngine::Handle> HandleForDirHash;
    typedef QHash<QFileSystemWatcherPathKey, QWindowsFileSystemWatcherEngine::PathInfo> PathInfoHash;

    QWindowsFileSystemWatcherEngineThread();
    ~QWindowsFileSystemWatcherEngineThread();
    void run();
    void stop();
    void wakeup();

    QMutex mutex;
    QVector<Qt::HANDLE> handles;
    int msg;

    HandleForDirHash handleForDir;

    QHash<Qt::HANDLE, PathInfoHash> pathInfoForHandle;

Q_SIGNALS:
    void fileChanged(const QString &path, bool removed);
    void directoryChanged(const QString &path, bool removed);
};

QT_END_NAMESPACE

#endif // QT_NO_FILESYSTEMWATCHER

#endif // QFILESYSTEMWATCHER_WIN_P_H
