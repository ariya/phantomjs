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

#ifndef QFILEINFO_P_H
#define QFILEINFO_P_H

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

#include "qfileinfo.h"
#include "qdatetime.h"
#include "qatomic.h"
#include "qshareddata.h"
#include "qfilesystemengine_p.h"
#include "qvector.h"

#include <QtCore/private/qabstractfileengine_p.h>
#include <QtCore/private/qfilesystementry_p.h>
#include <QtCore/private/qfilesystemmetadata_p.h>

QT_BEGIN_NAMESPACE

class QFileInfoPrivate : public QSharedData
{
public:
    enum { CachedFileFlags=0x01, CachedLinkTypeFlag=0x02, CachedBundleTypeFlag=0x04,
           CachedMTime=0x10, CachedCTime=0x20, CachedATime=0x40,
           CachedSize =0x08, CachedPerms=0x80 };

    inline QFileInfoPrivate()
        : QSharedData(), fileEngine(0),
        cachedFlags(0),
        isDefaultConstructed(true),
        cache_enabled(true), fileFlags(0), fileSize(0)
    {}
    inline QFileInfoPrivate(const QFileInfoPrivate &copy)
        : QSharedData(copy),
        fileEntry(copy.fileEntry),
        metaData(copy.metaData),
        fileEngine(QFileSystemEngine::resolveEntryAndCreateLegacyEngine(fileEntry, metaData)),
        cachedFlags(0),
#ifndef QT_NO_FSFILEENGINE
        isDefaultConstructed(false),
#else
        isDefaultConstructed(!fileEngine),
#endif
        cache_enabled(copy.cache_enabled), fileFlags(0), fileSize(0)
    {}
    inline QFileInfoPrivate(const QString &file)
        : fileEntry(QDir::fromNativeSeparators(file)),
        fileEngine(QFileSystemEngine::resolveEntryAndCreateLegacyEngine(fileEntry, metaData)),
        cachedFlags(0),
#ifndef QT_NO_FSFILEENGINE
        isDefaultConstructed(false),
#else
        isDefaultConstructed(!fileEngine),
#endif
        cache_enabled(true), fileFlags(0), fileSize(0)
    {
    }

    inline QFileInfoPrivate(const QFileSystemEntry &file, const QFileSystemMetaData &data)
        : QSharedData(),
        fileEntry(file),
        metaData(data),
        fileEngine(QFileSystemEngine::resolveEntryAndCreateLegacyEngine(fileEntry, metaData)),
        cachedFlags(0),
        isDefaultConstructed(false),
        cache_enabled(true), fileFlags(0), fileSize(0)
    {
        //If the file engine is not null, this maybe a "mount point" for a custom file engine
        //in which case we can't trust the metadata
        if (fileEngine)
            metaData = QFileSystemMetaData();
    }

    inline QFileInfoPrivate(const QFileSystemEntry &file, const QFileSystemMetaData &data, QAbstractFileEngine *engine)
        : fileEntry(file),
        metaData(data),
        fileEngine(engine),
        cachedFlags(0),
#ifndef QT_NO_FSFILEENGINE
        isDefaultConstructed(false),
#else
        isDefaultConstructed(!fileEngine),
#endif
        cache_enabled(true), fileFlags(0), fileSize(0)
    {
    }

    inline void clearFlags() const {
        fileFlags = 0;
        cachedFlags = 0;
        if (fileEngine)
            (void)fileEngine->fileFlags(QAbstractFileEngine::Refresh);
    }
    inline void clear() {
        metaData.clear();
        clearFlags();
        for (int i = QAbstractFileEngine::NFileNames - 1 ; i >= 0 ; --i)
            fileNames[i].clear();
        fileOwners[1].clear();
        fileOwners[0].clear();
    }

    uint getFileFlags(QAbstractFileEngine::FileFlags) const;
    QDateTime &getFileTime(QAbstractFileEngine::FileTime) const;
    QString getFileName(QAbstractFileEngine::FileName) const;
    QString getFileOwner(QAbstractFileEngine::FileOwner own) const;

    QFileSystemEntry fileEntry;
    mutable QFileSystemMetaData metaData;

    QScopedPointer<QAbstractFileEngine> const fileEngine;

    mutable QString fileNames[QAbstractFileEngine::NFileNames];
    mutable QString fileOwners[2];

    mutable uint cachedFlags : 30;
    bool const isDefaultConstructed : 1; // QFileInfo is a default constructed instance
    bool cache_enabled : 1;
    mutable uint fileFlags;
    mutable qint64 fileSize;
    // ### Qt6: FIXME: This vector is essentially a plain array
    // mutable QDateTime fileTimes[3], but the array is slower
    // to initialize than the QVector as QDateTime has a pimpl.
    // In Qt 6, QDateTime should inline its data members,
    // and this here can be an array again.
    mutable QVector<QDateTime> fileTimes;
    inline bool getCachedFlag(uint c) const
    { return cache_enabled ? (cachedFlags & c) : 0; }
    inline void setCachedFlag(uint c) const
    { if (cache_enabled) cachedFlags |= c; }

};

QT_END_NAMESPACE

#endif // QFILEINFO_P_H
