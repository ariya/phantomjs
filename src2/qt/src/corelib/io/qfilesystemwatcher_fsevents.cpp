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


#include <qplatformdefs.h>

#include "qfilesystemwatcher.h"
#include "qfilesystemwatcher_fsevents_p.h"

#ifndef QT_NO_FILESYSTEMWATCHER

#include <qdebug.h>
#include <qfile.h>
#include <qdatetime.h>
#include <qfileinfo.h>
#include <qvarlengtharray.h>

#include <mach/mach.h>
#include <sys/types.h>
#include <CoreFoundation/CFRunLoop.h>
#include <CoreFoundation/CFUUID.h>
#if !defined(Q_OS_IOS)
#include <CoreServices/CoreServices.h>
#endif
#include <AvailabilityMacros.h>
#include <private/qcore_mac_p.h>

QT_BEGIN_NAMESPACE

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5 && !defined(Q_OS_IOS)
// Static operator overloading so for the sake of some convieniece.
// They only live in this compilation unit to avoid polluting Qt in general.
static bool operator==(const struct ::timespec &left, const struct ::timespec &right)
{
    return left.tv_sec == right.tv_sec
            && left.tv_nsec == right.tv_nsec;
}

static bool operator==(const struct ::stat64 &left, const struct ::stat64 &right)
{
    return left.st_dev == right.st_dev
            && left.st_mode == right.st_mode
            && left.st_size == right.st_size
            && left.st_ino == right.st_ino
            && left.st_uid == right.st_uid
            && left.st_gid == right.st_gid
            && left.st_mtimespec == right.st_mtimespec
            && left.st_ctimespec == right.st_ctimespec
            && left.st_flags == right.st_flags;
}

static bool operator!=(const struct ::stat64 &left, const struct ::stat64 &right)
{
    return !(operator==(left, right));
}


static void addPathToHash(PathHash &pathHash, const QString &key, const QFileInfo &fileInfo,
                          const QString &path)
{
    PathInfoList &list = pathHash[key];
    list.push_back(PathInfo(path,
                            fileInfo.canonicalFilePath().normalized(QString::NormalizationForm_D).toUtf8()));
    pathHash.insert(key, list);
}

static void removePathFromHash(PathHash &pathHash, const QString &key, const QString &path)
{
    PathInfoList &list = pathHash[key];
    // We make the assumption that the list contains unique paths
    PathInfoList::iterator End = list.end();
    PathInfoList::iterator it = list.begin();
    while (it != End) {
        if (it->originalPath == path) {
            list.erase(it);
            break;
        }
        ++it;
    }
    if (list.isEmpty())
        pathHash.remove(key);
}

static void stopFSStream(FSEventStreamRef stream)
{
    if (stream) {
        FSEventStreamStop(stream);
        FSEventStreamInvalidate(stream);
    }
}

static QString createFSStreamPath(const QString &absolutePath)
{
    // The path returned has a trailing slash, so ensure that here.
    QString string = absolutePath;
    string.reserve(string.size() + 1);
    string.append(QLatin1Char('/'));
    return string;
}

static void cleanupFSStream(FSEventStreamRef stream)
{
    if (stream)
        FSEventStreamRelease(stream);
}

const FSEventStreamCreateFlags QtFSEventFlags = (kFSEventStreamCreateFlagUseCFTypes | kFSEventStreamCreateFlagNoDefer /* | kFSEventStreamCreateFlagWatchRoot*/);

const CFTimeInterval Latency = 0.033; // This will do updates 30 times a second which is probably more than you need.
#endif

QFSEventsFileSystemWatcherEngine::QFSEventsFileSystemWatcherEngine()
    : fsStream(0), pathsToWatch(0), threadsRunLoop(0)
{
}

QFSEventsFileSystemWatcherEngine::~QFSEventsFileSystemWatcherEngine()
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5 && !defined(Q_OS_IOS)
    // I assume that at this point, QFileSystemWatcher has already called stop
    // on me, so I don't need to invalidate or stop my stream, simply
    // release it.
    cleanupFSStream(fsStream);
    if (pathsToWatch)
        CFRelease(pathsToWatch);
#endif
}

QFSEventsFileSystemWatcherEngine *QFSEventsFileSystemWatcherEngine::create()
{
    return new QFSEventsFileSystemWatcherEngine();
}

QStringList QFSEventsFileSystemWatcherEngine::addPaths(const QStringList &paths,
                                                       QStringList *files,
                                                       QStringList *directories)
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5 && !defined(Q_OS_IOS)
    stop();
    wait();
    QMutexLocker locker(&mutex);
    QStringList failedToAdd;
    // if we have a running FSStreamEvent, we have to kill it, we'll re-add the stream soon.
    FSEventStreamEventId idToCheck;
    if (fsStream) {
        idToCheck = FSEventStreamGetLatestEventId(fsStream);
        cleanupFSStream(fsStream);
    } else {
        idToCheck = kFSEventStreamEventIdSinceNow;
    }

    // Brain-dead approach, but works. FSEvents actually can already read sub-trees, but since it's
    // work to figure out if we are doing a double register, we just register it twice as FSEvents
    // seems smart enough to only deliver one event. We also duplicate directory entries in here
    // (e.g., if you watch five files in the same directory, you get that directory included in the
    // array 5 times). This stupidity also makes remove work correctly though. I'll freely admit
    // that we could make this a bit smarter. If you do, check the auto-tests, they should catch at
    // least a couple of the issues.
    QCFType<CFMutableArrayRef> tmpArray = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
    for (int i = 0; i < paths.size(); ++i) {
        const QString &path = paths.at(i);

        QFileInfo fileInfo(path);
        if (!fileInfo.exists()) {
            failedToAdd.append(path);
            continue;
        }

        if (fileInfo.isDir()) {
            if (directories->contains(path)) {
                failedToAdd.append(path);
                continue;
            } else {
                directories->append(path);
                // Full file path for dirs.
                QCFString cfpath(createFSStreamPath(fileInfo.canonicalFilePath()));
                addPathToHash(dirPathInfoHash, cfpath, fileInfo, path);
                CFArrayAppendValue(tmpArray, cfpath);
            }
        } else {
            if (files->contains(path)) {
                failedToAdd.append(path);
                continue;
            } else {
                // Just the absolute path (minus it's filename) for files.
                QCFString cfpath(createFSStreamPath(fileInfo.canonicalPath()));
                files->append(path);
                addPathToHash(filePathInfoHash, cfpath, fileInfo, path);
                CFArrayAppendValue(tmpArray, cfpath);
            }
        }
    }

    if (!pathsToWatch && failedToAdd.size() == paths.size()) {
        return failedToAdd;
    }

    if (CFArrayGetCount(tmpArray) > 0) {
        if (pathsToWatch) {
            CFArrayAppendArray(tmpArray, pathsToWatch, CFRangeMake(0, CFArrayGetCount(pathsToWatch)));
            CFRelease(pathsToWatch);
        }
        pathsToWatch = CFArrayCreateCopy(kCFAllocatorDefault, tmpArray);
    }

    FSEventStreamContext context = { 0, this, 0, 0, 0 };
    fsStream = FSEventStreamCreate(kCFAllocatorDefault,
                                   QFSEventsFileSystemWatcherEngine::fseventsCallback,
                                   &context, pathsToWatch,
                                   idToCheck, Latency, QtFSEventFlags);
    warmUpFSEvents();

    return failedToAdd;
#else
    Q_UNUSED(paths);
    Q_UNUSED(files);
    Q_UNUSED(directories);
    return QStringList();
#endif
}

void QFSEventsFileSystemWatcherEngine::warmUpFSEvents()
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5 && !defined(Q_OS_IOS)
    // This function assumes that the mutex has already been grabbed before calling it.
    // It exits with the mutex still locked (Q_ASSERT(mutex.isLocked()) ;-).
    start();
    waitCondition.wait(&mutex);
#endif
}

QStringList QFSEventsFileSystemWatcherEngine::removePaths(const QStringList &paths,
                                                          QStringList *files,
                                                          QStringList *directories)
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5 && !defined(Q_OS_IOS)
    stop();
    wait();
    QMutexLocker locker(&mutex);
    // short circuit for smarties that call remove before add and we have nothing.
    if (pathsToWatch == 0)
        return paths;
    QStringList failedToRemove;
    // if we have a running FSStreamEvent, we have to stop it, we'll re-add the stream soon.
    FSEventStreamEventId idToCheck;
    if (fsStream) {
        idToCheck = FSEventStreamGetLatestEventId(fsStream);
        cleanupFSStream(fsStream);
        fsStream = 0;
    } else {
        idToCheck = kFSEventStreamEventIdSinceNow;
    }

    CFIndex itemCount = CFArrayGetCount(pathsToWatch);
    QCFType<CFMutableArrayRef> tmpArray = CFArrayCreateMutableCopy(kCFAllocatorDefault, itemCount,
                                                                   pathsToWatch);
    CFRelease(pathsToWatch);
    pathsToWatch = 0;
    for (int i = 0; i < paths.size(); ++i) {
        // Get the itemCount at the beginning to avoid any overruns during the iteration.
        itemCount = CFArrayGetCount(tmpArray);
        const QString &path = paths.at(i);
        QFileInfo fi(path);
        QCFString cfpath(createFSStreamPath(fi.canonicalPath()));

        CFIndex index = CFArrayGetFirstIndexOfValue(tmpArray, CFRangeMake(0, itemCount), cfpath);
        if (index != -1) {
            CFArrayRemoveValueAtIndex(tmpArray, index);
            files->removeAll(path);
            removePathFromHash(filePathInfoHash, cfpath, path);
        } else {
            // Could be a directory we are watching instead.
            QCFString cfdirpath(createFSStreamPath(fi.canonicalFilePath()));
            index = CFArrayGetFirstIndexOfValue(tmpArray, CFRangeMake(0, itemCount), cfdirpath);
            if (index != -1) {
                CFArrayRemoveValueAtIndex(tmpArray, index);
                directories->removeAll(path);
                removePathFromHash(dirPathInfoHash, cfpath, path);
            } else {
                failedToRemove.append(path);
            }
        }
    }
    itemCount = CFArrayGetCount(tmpArray);
    if (itemCount != 0) {
        pathsToWatch = CFArrayCreateCopy(kCFAllocatorDefault, tmpArray);

        FSEventStreamContext context = { 0, this, 0, 0, 0 };
        fsStream = FSEventStreamCreate(kCFAllocatorDefault,
                                       QFSEventsFileSystemWatcherEngine::fseventsCallback,
                                       &context, pathsToWatch, idToCheck, Latency, QtFSEventFlags);
        warmUpFSEvents();
    }
    return failedToRemove;
#else
    Q_UNUSED(paths);
    Q_UNUSED(files);
    Q_UNUSED(directories);
    return QStringList();
#endif
}

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5 && !defined(Q_OS_IOS)
void QFSEventsFileSystemWatcherEngine::updateList(PathInfoList &list, bool directory, bool emitSignals)
{
    PathInfoList::iterator End = list.end();
    PathInfoList::iterator it = list.begin();
    while (it != End) {
        struct ::stat64 newInfo;
        if (::stat64(it->absolutePath, &newInfo) == 0) {
            if (emitSignals) {
                if (newInfo != it->savedInfo) {
                    it->savedInfo = newInfo;
                    if (directory)
                        emit directoryChanged(it->originalPath, false);
                    else
                        emit fileChanged(it->originalPath, false);
                }
            } else {
                it->savedInfo = newInfo;
            }
        } else {
            if (errno == ENOENT) {
                if (emitSignals) {
                    if (directory)
                        emit directoryChanged(it->originalPath, true);
                    else
                        emit fileChanged(it->originalPath, true);
                }
                it = list.erase(it);
                continue;
            } else {
                qWarning("%s:%d:QFSEventsFileSystemWatcherEngine: stat error on %s:%s",
                         __FILE__, __LINE__, qPrintable(it->originalPath), strerror(errno));

            }
        }
        ++it;
    }
}

void QFSEventsFileSystemWatcherEngine::updateHash(PathHash &pathHash)
{
    PathHash::iterator HashEnd = pathHash.end();
    PathHash::iterator it = pathHash.begin();
    const bool IsDirectory = (&pathHash == &dirPathInfoHash);
    while (it != HashEnd) {
        updateList(it.value(), IsDirectory, false);
        if (it.value().isEmpty())
            it = pathHash.erase(it);
        else
            ++it;
    }
}
#endif

void QFSEventsFileSystemWatcherEngine::fseventsCallback(ConstFSEventStreamRef ,
                                                        void *clientCallBackInfo, size_t numEvents,
                                                        void *eventPaths,
                                                        const FSEventStreamEventFlags eventFlags[],
                                                        const FSEventStreamEventId [])
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5 && !defined(Q_OS_IOS)
    QFSEventsFileSystemWatcherEngine *watcher = static_cast<QFSEventsFileSystemWatcherEngine *>(clientCallBackInfo);
    QMutexLocker locker(&watcher->mutex);
    CFArrayRef paths = static_cast<CFArrayRef>(eventPaths);
    for (size_t i = 0; i < numEvents; ++i) {
        const QString path = QCFString::toQString(
                                    static_cast<CFStringRef>(CFArrayGetValueAtIndex(paths, i)));
        const FSEventStreamEventFlags pathFlags = eventFlags[i];
        // There are several flags that may be passed, but we really don't care about them ATM.
        // Here they are and why we don't care.
        // kFSEventStreamEventFlagHistoryDone--(very unlikely to be gotten, but even then, not much changes).
        // kFSEventStreamEventFlagMustScanSubDirs--Likely means the data is very much out of date, we
        //            aren't coalescing our directories, so again not so much of an issue
        // kFSEventStreamEventFlagRootChanged | kFSEventStreamEventFlagMount | kFSEventStreamEventFlagUnmount--
        // These three flags indicate something has changed, but the stat will likely show this, so
        // there's not really much to worry about.
        // (btw, FSEvents is not the correct way of checking for mounts/unmounts,
        //  there are real CarbonCore events for that.)
        Q_UNUSED(pathFlags);
        if (watcher->filePathInfoHash.contains(path))
            watcher->updateList(watcher->filePathInfoHash[path], false, true);

        if (watcher->dirPathInfoHash.contains(path))
            watcher->updateList(watcher->dirPathInfoHash[path], true, true);
    }
#else
    Q_UNUSED(clientCallBackInfo);
    Q_UNUSED(numEvents);
    Q_UNUSED(eventPaths);
    Q_UNUSED(eventFlags);
#endif
}

void QFSEventsFileSystemWatcherEngine::stop()
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5 && !defined(Q_OS_IOS)
    QMutexLocker locker(&mutex);
    stopFSStream(fsStream);
    if (threadsRunLoop) {
        CFRunLoopStop(threadsRunLoop);
        waitForStop.wait(&mutex);
    }
#endif
}

void QFSEventsFileSystemWatcherEngine::updateFiles()
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5 && !defined(Q_OS_IOS)
    QMutexLocker locker(&mutex);
    updateHash(filePathInfoHash);
    updateHash(dirPathInfoHash);
    if (filePathInfoHash.isEmpty() && dirPathInfoHash.isEmpty()) {
        // Everything disappeared before we got to start, don't bother.
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5 && !defined(Q_OS_IOS)
        // Code duplicated from stop(), with the exception that we
        // don't wait on waitForStop here. Doing this will lead to
        // a deadlock since this function is called from the worker
        // thread. (waitForStop.wakeAll() is only called from the
        // end of run()).
        stopFSStream(fsStream);
        if (threadsRunLoop)
            CFRunLoopStop(threadsRunLoop);
#endif
        cleanupFSStream(fsStream);
    }
    waitCondition.wakeAll();
#endif
}

void QFSEventsFileSystemWatcherEngine::run()
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5 && !defined(Q_OS_IOS)
    threadsRunLoop = CFRunLoopGetCurrent();
    FSEventStreamScheduleWithRunLoop(fsStream, threadsRunLoop, kCFRunLoopDefaultMode);
    bool startedOK = FSEventStreamStart(fsStream);
    // It's recommended by Apple that you only update the files after you've started
    // the stream, because otherwise you might miss an update in between starting it.
    updateFiles();
#ifdef QT_NO_DEBUG
    Q_UNUSED(startedOK);
#else
    Q_ASSERT(startedOK);
#endif
    // If for some reason we called stop up above (and invalidated our stream), this call will return
    // immediately.
    CFRunLoopRun();
    threadsRunLoop = 0;
    QMutexLocker locker(&mutex);
    waitForStop.wakeAll();
#endif
}

QT_END_NAMESPACE
#endif //QT_NO_FILESYSTEMWATCHER
