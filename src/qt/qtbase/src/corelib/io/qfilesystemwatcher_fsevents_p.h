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

#ifndef QFILESYSTEMWATCHER_FSEVENTS_P_H
#define QFILESYSTEMWATCHER_FSEVENTS_P_H

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

#include <QtCore/qmutex.h>
#include <QtCore/qhash.h>
#include <QtCore/qthread.h>
#include <QtCore/qvector.h>
#include <QtCore/qsocketnotifier.h>

#include <dispatch/dispatch.h>
#include <CoreServices/CoreServices.h>

#ifndef QT_NO_FILESYSTEMWATCHER

QT_BEGIN_NAMESPACE

class QFseventsFileSystemWatcherEngine : public QFileSystemWatcherEngine
{
    Q_OBJECT
public:
    ~QFseventsFileSystemWatcherEngine();

    static QFseventsFileSystemWatcherEngine *create(QObject *parent);

    QStringList addPaths(const QStringList &paths, QStringList *files, QStringList *directories);
    QStringList removePaths(const QStringList &paths, QStringList *files, QStringList *directories);

    void processEvent(ConstFSEventStreamRef streamRef, size_t numEvents, char **eventPaths, const FSEventStreamEventFlags eventFlags[], const FSEventStreamEventId eventIds[]);

Q_SIGNALS:
    void emitFileChanged(const QString path, bool removed);
    void emitDirectoryChanged(const QString path, bool removed);
    void scheduleStreamRestart();

private slots:
    void doEmitFileChanged(const QString path, bool removed);
    void doEmitDirectoryChanged(const QString path, bool removed);
    void restartStream();

private:
    struct Info {
        QString origPath;
        timespec ctime;
        mode_t mode;
        QString watchedPath;

        Info(): mode(0)
        {
            ctime.tv_sec = 0;
            ctime.tv_nsec = 0;
        }

        Info(const QString &origPath, const timespec &ctime, mode_t mode, const QString &watchedPath)
            : origPath(origPath)
            , ctime(ctime)
            , mode(mode)
            , watchedPath(watchedPath)
        {}
    };
    typedef QHash<QString, Info> InfoByName;
    typedef QHash<QString, InfoByName> FilesByPath;
    struct DirInfo {
        Info dirInfo;
        InfoByName entries;
    };
    typedef QHash<QString, DirInfo> DirsByName;
    typedef QHash<QString, qint64> PathRefCounts;

    QFseventsFileSystemWatcherEngine(QObject *parent);
    bool startStream();
    void stopStream(bool isStopped = false);
    InfoByName scanForDirEntries(const QString &path);
    bool derefPath(const QString &watchedPath);
    bool checkDir(DirsByName::iterator &it);
    bool rescanDirs(const QString &path);
    bool rescanFiles(InfoByName &filesInPath);
    bool rescanFiles(const QString &path);

    QMutex lock;
    dispatch_queue_t queue;
    FSEventStreamRef stream;
    FilesByPath watchedFiles;
    DirsByName watchedDirectories;
    PathRefCounts watchedPaths;
    FSEventStreamEventId lastReceivedEvent;
};

QT_END_NAMESPACE

#endif //QT_NO_FILESYSTEMWATCHER
#endif // QFILESYSTEMWATCHER_FSEVENTS_P_H
