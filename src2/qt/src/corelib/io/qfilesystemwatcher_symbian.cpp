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

#include "qfilesystemwatcher.h"
#include "qfilesystemwatcher_symbian_p.h"
#include "qfileinfo.h"
#include "qdebug.h"
#include "private/qcore_symbian_p.h"
#include <QDir>

#ifndef QT_NO_FILESYSTEMWATCHER


QT_BEGIN_NAMESPACE

QNotifyChangeEvent::QNotifyChangeEvent(RFs &fs, const TDesC &file,
                                       QSymbianFileSystemWatcherInterface *e, bool aIsDir,
									   TInt aPriority) :
        CActive(aPriority),
        isDir(aIsDir),
        fsSession(fs),
        watchedPath(file),
        engine(e),
        failureCount(0)
{
    if (isDir) {
        fsSession.NotifyChange(ENotifyEntry, iStatus, watchedPath);
    } else {
        fsSession.NotifyChange(ENotifyAll, iStatus, watchedPath);
    }
    CActiveScheduler::Add(this);
    SetActive();
}

QNotifyChangeEvent::~QNotifyChangeEvent()
{
    Cancel();
}

void QNotifyChangeEvent::RunL()
{
    if(iStatus.Int() == KErrNone) {
        failureCount = 0;
    } else {
        qWarning("QNotifyChangeEvent::RunL() - Failed to order change notifications: %d", iStatus.Int());
        failureCount++;
    }

    // Re-request failed notification once, but if it won't start working,
    // we can't do much besides just not request any more notifications.
    if (failureCount < 2) {
        if (isDir) {
            fsSession.NotifyChange(ENotifyEntry, iStatus, watchedPath);
        } else {
            fsSession.NotifyChange(ENotifyAll, iStatus, watchedPath);
        }
        SetActive();

        if (!failureCount) {
            int err;
            QT_TRYCATCH_ERROR(err, engine->handlePathChanged(this));
            if (err != KErrNone)
                qWarning("QNotifyChangeEvent::RunL() - handlePathChanged threw exception (Converted error code: %d)", err);
        }
    }
}

void QNotifyChangeEvent::DoCancel()
{
    fsSession.NotifyChangeCancel(iStatus);
}

QSymbianFileSystemWatcherEngine::QSymbianFileSystemWatcherEngine() :
        watcherStarted(false)
{
    moveToThread(this);
}

QSymbianFileSystemWatcherEngine::~QSymbianFileSystemWatcherEngine()
{
    stop();
}

QStringList QSymbianFileSystemWatcherEngine::addPaths(const QStringList &paths, QStringList *files,
        QStringList *directories)
{
    QMutexLocker locker(&mutex);
    QStringList p = paths;

    startWatcher();

    QMutableListIterator<QString> it(p);
    while (it.hasNext()) {
        QString path = it.next();
        QFileInfo fi(path);
        if (!fi.exists())
            continue;

        bool isDir = fi.isDir();
        if (isDir) {
            if (directories->contains(path))
                continue;
        } else {
            if (files->contains(path))
                continue;
        }

        // Use absolute filepath as relative paths seem to have some issues.
        QString filePath = fi.absoluteFilePath();
        if (isDir && filePath.at(filePath.size() - 1) != QChar(L'/')) {
            filePath += QChar(L'/');
        }

        currentAddEvent = NULL;
        QMetaObject::invokeMethod(this,
                                  "addNativeListener",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, filePath));

        syncCondition.wait(&mutex);
        if (currentAddEvent) {
            currentAddEvent->isDir = isDir;

            activeObjectToPath.insert(currentAddEvent, path);
            it.remove();

            if (isDir)
                directories->append(path);
            else
                files->append(path);
        }
    }

    return p;
}

QStringList QSymbianFileSystemWatcherEngine::removePaths(const QStringList &paths,
                                                         QStringList *files,
                                                         QStringList *directories)
{
    QMutexLocker locker(&mutex);

    QStringList p = paths;
    QMutableListIterator<QString> it(p);
    while (it.hasNext()) {
        QString path = it.next();

        currentRemoveEvent = activeObjectToPath.key(path);
        if (!currentRemoveEvent)
            continue;
        activeObjectToPath.remove(currentRemoveEvent);

        QMetaObject::invokeMethod(this,
                                  "removeNativeListener",
                                  Qt::QueuedConnection);

        syncCondition.wait(&mutex);

        it.remove();

        files->removeAll(path);
        directories->removeAll(path);
    }

    return p;
}

void QSymbianFileSystemWatcherEngine::handlePathChanged(QNotifyChangeEvent *e)
{
    QMutexLocker locker(&mutex);

    QString path = activeObjectToPath.value(e);
    QFileInfo fi(path);

    if (e->isDir)
        emit directoryChanged(path, !fi.exists());
    else
        emit fileChanged(path, !fi.exists());
}

void QSymbianFileSystemWatcherEngine::stop()
{
    quit();
    wait();
}

// This method must be called inside mutex
void QSymbianFileSystemWatcherEngine::startWatcher()
{
    if (!watcherStarted) {
        setStackSize(0x5000);
        start();
        syncCondition.wait(&mutex);
        watcherStarted = true;
    }
}


void QSymbianFileSystemWatcherEngine::run()
{
    mutex.lock();
    syncCondition.wakeOne();
    mutex.unlock();

    exec();

    foreach(QNotifyChangeEvent *e, activeObjectToPath.keys()) {
        e->Cancel();
        delete e;
    }

    activeObjectToPath.clear();
}

void QSymbianFileSystemWatcherEngine::addNativeListener(const QString &directoryPath)
{
    QMutexLocker locker(&mutex);
    QString nativeDir(QDir::toNativeSeparators(directoryPath));
    TPtrC ptr(qt_QString2TPtrC(nativeDir));
    currentAddEvent = q_check_ptr(new QNotifyChangeEvent(qt_s60GetRFs(), ptr, this, directoryPath.endsWith(QChar(L'/'))));
    syncCondition.wakeOne();
}

void QSymbianFileSystemWatcherEngine::removeNativeListener()
{
    QMutexLocker locker(&mutex);
    currentRemoveEvent->Cancel();
    delete currentRemoveEvent;
    currentRemoveEvent = NULL;
    syncCondition.wakeOne();
}


QT_END_NAMESPACE
#endif // QT_NO_FILESYSTEMWATCHER
