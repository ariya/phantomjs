/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QFILEINFOGATHERER_H
#define QFILEINFOGATHERER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qthread.h>
#include <qmutex.h>
#include <qwaitcondition.h>
#include <qfilesystemwatcher.h>
#include <qfileiconprovider.h>
#include <qfsfileengine.h>
#include <qpair.h>
#include <qstack.h>
#include <qdatetime.h>
#include <qdir.h>
#include <qelapsedtimer.h>

QT_BEGIN_NAMESPACE

class QExtendedInformation {
public:
    enum Type { Dir, File, System };

    QExtendedInformation() {}
    QExtendedInformation(const QFileInfo &info) : mFileInfo(info) {}

    inline bool isDir() { return type() == Dir; }
    inline bool isFile() { return type() == File; }
    inline bool isSystem() { return type() == System; }

    bool operator ==(const QExtendedInformation &fileInfo) const {
       return mFileInfo == fileInfo.mFileInfo
       && displayType == fileInfo.displayType
       && permissions() == fileInfo.permissions();
    }

#ifndef QT_NO_FSFILEENGINE
    bool isCaseSensitive() const {
        QFSFileEngine fe(mFileInfo.absoluteFilePath());
        return fe.caseSensitive();
    }
#endif

    QFile::Permissions permissions() const {
        return mFileInfo.permissions();
    }

    Type type() const {
        if (mFileInfo.isDir()) {
            return QExtendedInformation::Dir;
        }
        if (mFileInfo.isFile()) {
            return QExtendedInformation::File;
        }
        if (!mFileInfo.exists() && mFileInfo.isSymLink()) {
            return QExtendedInformation::System;
        }
        return QExtendedInformation::System;
    }

    bool isSymLink() const {
        return mFileInfo.isSymLink();
    }

    bool isHidden() const {
        return mFileInfo.isHidden();
    }

    QFileInfo fileInfo() const {
        return mFileInfo;
    }

    QDateTime lastModified() const {
        return mFileInfo.lastModified();
    }

    qint64 size() const {
        qint64 size = -1;
        if (type() == QExtendedInformation::Dir)
            size = 0;
        if (type() == QExtendedInformation::File)
            size = mFileInfo.size();
        if (!mFileInfo.exists() && !mFileInfo.isSymLink())
            size = -1;
        return size;
    }

    QString displayType;
    QIcon icon;

private :
    QFileInfo mFileInfo;
};

class QFileIconProvider;

#ifndef QT_NO_FILESYSTEMMODEL

class Q_AUTOTEST_EXPORT QFileInfoGatherer : public QThread
{
Q_OBJECT

Q_SIGNALS:
    void updates(const QString &directory, const QList<QPair<QString, QFileInfo> > &updates);
    void newListOfFiles(const QString &directory, const QStringList &listOfFiles) const;
    void nameResolved(const QString &fileName, const QString &resolvedName) const;
    void directoryLoaded(const QString &path);

public:
    QFileInfoGatherer(QObject *parent = 0);
    ~QFileInfoGatherer();

    void clear();
    void removePath(const QString &path);
    QExtendedInformation getInfo(const QFileInfo &info) const;

public Q_SLOTS:
    void list(const QString &directoryPath);
    void fetchExtendedInformation(const QString &path, const QStringList &files);
    void updateFile(const QString &path);
    void setResolveSymlinks(bool enable);
    bool resolveSymlinks() const;
    void setIconProvider(QFileIconProvider *provider);
    QFileIconProvider *iconProvider() const;

protected:
    void run();
    void getFileInfos(const QString &path, const QStringList &files);

private:
    void fetch(const QFileInfo &info, QElapsedTimer &base, bool &firstTime, QList<QPair<QString, QFileInfo> > &updatedFiles, const QString &path);
    QString translateDriveName(const QFileInfo &drive) const;

    QMutex mutex;
    QWaitCondition condition;
    volatile bool abort;

    QStack<QString> path;
    QStack<QStringList> files;

#ifndef QT_NO_FILESYSTEMWATCHER
    QFileSystemWatcher *watcher;
#endif
    bool m_resolveSymlinks;
    QFileIconProvider *m_iconProvider;
    QFileIconProvider defaultProvider;
#ifndef Q_OS_WIN
    uint userId;
    uint groupId;
#endif
};
#endif // QT_NO_FILESYSTEMMODEL


QT_END_NAMESPACE
#endif // QFILEINFOGATHERER_H

