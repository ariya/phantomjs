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

#ifndef QFILESYSTEMITERATOR_P_H_INCLUDED
#define QFILESYSTEMITERATOR_P_H_INCLUDED

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

#include <QtCore/qglobal.h>

#ifndef QT_NO_FILESYSTEMITERATOR

#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qstringlist.h>

#include <QtCore/private/qfilesystementry_p.h>
#include <QtCore/private/qfilesystemmetadata_p.h>

// Platform-specific headers
#if defined(Q_OS_WIN)
#elif defined (Q_OS_SYMBIAN)
#include <f32file.h>
#else
#include <QtCore/qscopedpointer.h>
#endif

QT_BEGIN_NAMESPACE

class QFileSystemIterator
{
public:
    QFileSystemIterator(const QFileSystemEntry &entry, QDir::Filters filters,
            const QStringList &nameFilters, QDirIterator::IteratorFlags flags
                = QDirIterator::FollowSymlinks | QDirIterator::Subdirectories);
    ~QFileSystemIterator();

    bool advance(QFileSystemEntry &fileEntry, QFileSystemMetaData &metaData);

private:
    QFileSystemEntry::NativePath nativePath;

    // Platform-specific data
#if defined(Q_OS_WIN)
    QFileSystemEntry::NativePath dirPath;
    HANDLE findFileHandle;
    QStringList uncShares;
    bool uncFallback;
    int uncShareIndex;
    bool onlyDirs;
#elif defined (Q_OS_SYMBIAN)
    RDir dirHandle;
    TEntryArray entries;
    TInt lastError;
    TInt entryIndex;
#else
    QT_DIR *dir;
    QT_DIRENT *dirEntry;
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(Q_OS_CYGWIN)
    // for readdir_r
    QScopedPointer<QT_DIRENT, QScopedPointerPodDeleter> mt_file;
#endif
    int lastError;
#endif

    Q_DISABLE_COPY(QFileSystemIterator)
};

QT_END_NAMESPACE

#endif // QT_NO_FILESYSTEMITERATOR

#endif // include guard
