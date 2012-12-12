/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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


#ifndef FILEWATCHER_FSEVENTS_P_H
#define FILEWATCHER_FSEVENTS_P_H

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

#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>
#include <QtCore/qthread.h>
#include <QtCore/qhash.h>
#include <QtCore/qlinkedlist.h>
#include <private/qcore_mac_p.h>
#include <sys/stat.h>

typedef struct __FSEventStream *FSEventStreamRef;
typedef const struct __FSEventStream *ConstFSEventStreamRef;
typedef const struct __CFArray *CFArrayRef;
typedef UInt32 FSEventStreamEventFlags;
typedef uint64_t FSEventStreamEventId;

QT_BEGIN_NAMESPACE

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5 && !defined(Q_OS_IOS)
// Yes, I use a stat64 element here. QFileInfo requires too much knowledge about implementation
// details to be used as a long-standing record. Since I'm going to have to store this information, I can
// do the stat myself too.
struct PathInfo {
    PathInfo(const QString &path, const QByteArray &absPath)
            : originalPath(path), absolutePath(absPath) {}
    QString originalPath; // The path we need to emit
    QByteArray absolutePath; // The path we need to stat.
    struct ::stat64 savedInfo;  // All the info for the path so we can compare it.
};
typedef QLinkedList<PathInfo> PathInfoList;
typedef QHash<QString, PathInfoList> PathHash;
#endif

class QFSEventsFileSystemWatcherEngine : public QFileSystemWatcherEngine
{
    Q_OBJECT
public:
    ~QFSEventsFileSystemWatcherEngine();

    static QFSEventsFileSystemWatcherEngine *create();

    QStringList addPaths(const QStringList &paths, QStringList *files, QStringList *directories);
    QStringList removePaths(const QStringList &paths, QStringList *files, QStringList *directories);

    void stop();

private:
    QFSEventsFileSystemWatcherEngine();
    void warmUpFSEvents();
    void updateFiles();

    static void fseventsCallback(ConstFSEventStreamRef streamRef, void *clientCallBackInfo, size_t numEvents,
                                  void *eventPaths, const FSEventStreamEventFlags eventFlags[],
                                  const FSEventStreamEventId eventIds[]);
    void run();
    FSEventStreamRef fsStream;
    CFArrayRef pathsToWatch;
    CFRunLoopRef threadsRunLoop;
    QMutex mutex;
    QWaitCondition waitCondition;
    QWaitCondition waitForStop;
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5 && !defined(Q_OS_IOS)
    PathHash filePathInfoHash;
    PathHash dirPathInfoHash;
    void updateHash(PathHash &pathHash);
    void updateList(PathInfoList &list, bool directory, bool emitSignals);
#endif
};

#endif //QT_NO_FILESYSTEMWATCHER

#endif

QT_END_NAMESPACE
