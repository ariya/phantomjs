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

#include "qfilesystemengine_p.h"
#include <QtCore/qdir.h>
#include <QtCore/qset.h>
#include <QtCore/qstringbuilder.h>
#include <QtCore/private/qabstractfileengine_p.h>
#ifdef QT_BUILD_CORE_LIB
#include <QtCore/private/qresource_p.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \internal

    Returns the canonicalized form of \a path (i.e., with all symlinks
    resolved, and all redundant path elements removed.
*/
QString QFileSystemEngine::slowCanonicalized(const QString &path)
{
    if (path.isEmpty())
        return path;

    QFileInfo fi;
    const QChar slash(QLatin1Char('/'));
    QString tmpPath = path;
    int separatorPos = 0;
    QSet<QString> nonSymlinks;
    QSet<QString> known;

    known.insert(path);
    do {
#ifdef Q_OS_WIN
        if (separatorPos == 0) {
            if (tmpPath.size() >= 2 && tmpPath.at(0) == slash && tmpPath.at(1) == slash) {
                // UNC, skip past the first two elements
                separatorPos = tmpPath.indexOf(slash, 2);
            } else if (tmpPath.size() >= 3 && tmpPath.at(1) == QLatin1Char(':') && tmpPath.at(2) == slash) {
                // volume root, skip since it can not be a symlink
                separatorPos = 2;
            }
        }
        if (separatorPos != -1)
#endif
        separatorPos = tmpPath.indexOf(slash, separatorPos + 1);
        QString prefix = separatorPos == -1 ? tmpPath : tmpPath.left(separatorPos);
        if (!nonSymlinks.contains(prefix)) {
            fi.setFile(prefix);
            if (fi.isSymLink()) {
                QString target = fi.symLinkTarget();
                if(QFileInfo(target).isRelative())
                    target = fi.absolutePath() + slash + target;
                if (separatorPos != -1) {
                    if (fi.isDir() && !target.endsWith(slash))
                        target.append(slash);
                    target.append(tmpPath.mid(separatorPos));
                }
                tmpPath = QDir::cleanPath(target);
                separatorPos = 0;

                if (known.contains(tmpPath))
                    return QString();
                known.insert(tmpPath);
            } else {
                nonSymlinks.insert(prefix);
            }
        }
    } while (separatorPos != -1);

    return QDir::cleanPath(tmpPath);
}

static inline bool _q_checkEntry(QFileSystemEntry &entry, QFileSystemMetaData &data, bool resolvingEntry)
{
    if (resolvingEntry) {
        if (!QFileSystemEngine::fillMetaData(entry, data, QFileSystemMetaData::ExistsAttribute)
                || !data.exists()) {
            data.clear();
            return false;
        }
    }

    return true;
}

static inline bool _q_checkEntry(QAbstractFileEngine *&engine, bool resolvingEntry)
{
    if (resolvingEntry) {
        if (!(engine->fileFlags(QAbstractFileEngine::FlagsMask) & QAbstractFileEngine::ExistsFlag)) {
            delete engine;
            engine = 0;
            return false;
        }
    }

    return true;
}

static bool _q_resolveEntryAndCreateLegacyEngine_recursive(QFileSystemEntry &entry, QFileSystemMetaData &data,
        QAbstractFileEngine *&engine, bool resolvingEntry = false)
{
    QString const &filePath = entry.filePath();
    if ((engine = qt_custom_file_engine_handler_create(filePath)))
        return _q_checkEntry(engine, resolvingEntry);

#if defined(QT_BUILD_CORE_LIB)
    for (int prefixSeparator = 0; prefixSeparator < filePath.size(); ++prefixSeparator) {
        QChar const ch = filePath[prefixSeparator];
        if (ch == QLatin1Char('/'))
            break;

        if (ch == QLatin1Char(':')) {
            if (prefixSeparator == 0) {
                engine = new QResourceFileEngine(filePath);
                return _q_checkEntry(engine, resolvingEntry);
            }

            if (prefixSeparator == 1)
                break;

            const QStringList &paths = QDir::searchPaths(filePath.left(prefixSeparator));
            for (int i = 0; i < paths.count(); i++) {
                entry = QFileSystemEntry(QDir::cleanPath(paths.at(i) % QLatin1Char('/') % filePath.mid(prefixSeparator + 1)));
                // Recurse!
                if (_q_resolveEntryAndCreateLegacyEngine_recursive(entry, data, engine, true))
                    return true;
            }

            // entry may have been clobbered at this point.
            return false;
        }

        //  There's no need to fully validate the prefix here. Consulting the
        //  unicode tables could be expensive and validation is already
        //  performed in QDir::setSearchPaths.
        //
        //  if (!ch.isLetterOrNumber())
        //      break;
    }
#endif // defined(QT_BUILD_CORE_LIB)

    return _q_checkEntry(entry, data, resolvingEntry);
}

/*!
    \internal

    Resolves the \a entry (see QDir::searchPaths) and returns an engine for
    it, but never a QFSFileEngine.

    Returns a file engine that can be used to access the entry. Returns 0 if
    QFileSystemEngine API should be used to query and interact with the file
    system object.
*/
QAbstractFileEngine *QFileSystemEngine::resolveEntryAndCreateLegacyEngine(
        QFileSystemEntry &entry, QFileSystemMetaData &data) {
    QFileSystemEntry copy = entry;
    QAbstractFileEngine *engine = 0;

    if (_q_resolveEntryAndCreateLegacyEngine_recursive(copy, data, engine))
        // Reset entry to resolved copy.
        entry = copy;
    else
        data.clear();

    return engine;
}

//these unix functions are in this file, because they are shared by symbian port
//for open C file handles.
#ifdef Q_OS_UNIX
//static
bool QFileSystemEngine::fillMetaData(int fd, QFileSystemMetaData &data)
{
    data.entryFlags &= ~QFileSystemMetaData::PosixStatFlags;
    data.knownFlagsMask |= QFileSystemMetaData::PosixStatFlags;

    QT_STATBUF statBuffer;
    if (QT_FSTAT(fd, &statBuffer) == 0) {
        data.fillFromStatBuf(statBuffer);
        return true;
    }

    return false;
}

#if defined(Q_OS_QNX)
static void fillStat64fromStat32(struct stat64 *statBuf64, const struct stat &statBuf32)
{
    statBuf64->st_mode = statBuf32.st_mode;
    statBuf64->st_size = statBuf32.st_size;
    statBuf64->st_ctime = statBuf32.st_ctime;
    statBuf64->st_mtime = statBuf32.st_mtime;
    statBuf64->st_atime = statBuf32.st_atime;
    statBuf64->st_uid = statBuf32.st_uid;
    statBuf64->st_gid = statBuf32.st_gid;
}
#endif

void QFileSystemMetaData::fillFromStatBuf(const QT_STATBUF &statBuffer)
{
    // Permissions
    if (statBuffer.st_mode & S_IRUSR)
        entryFlags |= QFileSystemMetaData::OwnerReadPermission;
    if (statBuffer.st_mode & S_IWUSR)
        entryFlags |= QFileSystemMetaData::OwnerWritePermission;
    if (statBuffer.st_mode & S_IXUSR)
        entryFlags |= QFileSystemMetaData::OwnerExecutePermission;

    if (statBuffer.st_mode & S_IRGRP)
        entryFlags |= QFileSystemMetaData::GroupReadPermission;
    if (statBuffer.st_mode & S_IWGRP)
        entryFlags |= QFileSystemMetaData::GroupWritePermission;
    if (statBuffer.st_mode & S_IXGRP)
        entryFlags |= QFileSystemMetaData::GroupExecutePermission;

    if (statBuffer.st_mode & S_IROTH)
        entryFlags |= QFileSystemMetaData::OtherReadPermission;
    if (statBuffer.st_mode & S_IWOTH)
        entryFlags |= QFileSystemMetaData::OtherWritePermission;
    if (statBuffer.st_mode & S_IXOTH)
        entryFlags |= QFileSystemMetaData::OtherExecutePermission;

    // Type
    if ((statBuffer.st_mode & S_IFMT) == S_IFREG)
        entryFlags |= QFileSystemMetaData::FileType;
    else if ((statBuffer.st_mode & S_IFMT) == S_IFDIR)
        entryFlags |= QFileSystemMetaData::DirectoryType;
    else
        entryFlags |= QFileSystemMetaData::SequentialType;

    // Attributes
    entryFlags |= QFileSystemMetaData::ExistsAttribute;
    size_ = statBuffer.st_size;
#if defined(Q_OS_MACX)
    if (statBuffer.st_flags & UF_HIDDEN) {
        entryFlags |= QFileSystemMetaData::HiddenAttribute;
        knownFlagsMask |= QFileSystemMetaData::HiddenAttribute;
    }
#endif

    // Times
    creationTime_ = statBuffer.st_ctime ? statBuffer.st_ctime : statBuffer.st_mtime;
    modificationTime_ = statBuffer.st_mtime;
    accessTime_ = statBuffer.st_atime;
    userId_ = statBuffer.st_uid;
    groupId_ = statBuffer.st_gid;
}

void QFileSystemMetaData::fillFromDirEnt(const QT_DIRENT &entry)
{
#if defined(Q_OS_QNX)
    knownFlagsMask = 0;
    entryFlags = 0;
    for (dirent_extra *extra = _DEXTRA_FIRST(&entry); _DEXTRA_VALID(extra, &entry);
         extra = _DEXTRA_NEXT(extra)) {
        if (extra->d_type == _DTYPE_STAT || extra->d_type == _DTYPE_LSTAT) {

            const struct dirent_extra_stat * const extra_stat =
                    reinterpret_cast<struct dirent_extra_stat *>(extra);

            // Remember whether this was a link or not, this saves an lstat() call later.
            if (extra->d_type == _DTYPE_LSTAT) {
                knownFlagsMask |= QFileSystemMetaData::LinkType;
                if (S_ISLNK(extra_stat->d_stat.st_mode))
                    entryFlags |= QFileSystemMetaData::LinkType;
            }

            // For symlinks, the extra type _DTYPE_LSTAT doesn't work for filling out the meta data,
            // as we need the stat() information there, not the lstat() information.
            // In this case, don't use the extra information.
            // Unfortunately, readdir() never seems to return extra info of type _DTYPE_STAT, so for
            // symlinks, we always incur the cost of an extra stat() call later.
            if (S_ISLNK(extra_stat->d_stat.st_mode) && extra->d_type == _DTYPE_LSTAT)
                continue;

#if defined(QT_USE_XOPEN_LFS_EXTENSIONS) && defined(QT_LARGEFILE_SUPPORT)
            // Even with large file support, d_stat is always of type struct stat, not struct stat64,
            // so it needs to be converted
            struct stat64 statBuf;
            fillStat64fromStat32(&statBuf, extra_stat->d_stat);
            fillFromStatBuf(statBuf);
#else
            fillFromStatBuf(extra_stat->d_stat);
#endif
            knownFlagsMask |= QFileSystemMetaData::PosixStatFlags;
            if (!S_ISLNK(extra_stat->d_stat.st_mode)) {
                knownFlagsMask |= QFileSystemMetaData::ExistsAttribute;
                entryFlags |= QFileSystemMetaData::ExistsAttribute;
            }
        }
    }
#elif defined(_DIRENT_HAVE_D_TYPE) || defined(Q_OS_BSD4)
    // BSD4 includes Mac OS X

    // ### This will clear all entry flags and knownFlagsMask
    switch (entry.d_type)
    {
    case DT_DIR:
        knownFlagsMask = QFileSystemMetaData::LinkType
            | QFileSystemMetaData::FileType
            | QFileSystemMetaData::DirectoryType
            | QFileSystemMetaData::SequentialType
            | QFileSystemMetaData::ExistsAttribute;

        entryFlags = QFileSystemMetaData::DirectoryType
            | QFileSystemMetaData::ExistsAttribute;

        break;

    case DT_BLK:
    case DT_CHR:
    case DT_FIFO:
    case DT_SOCK:
        // ### System attribute
        knownFlagsMask = QFileSystemMetaData::LinkType
            | QFileSystemMetaData::FileType
            | QFileSystemMetaData::DirectoryType
            | QFileSystemMetaData::BundleType
            | QFileSystemMetaData::AliasType
            | QFileSystemMetaData::SequentialType
            | QFileSystemMetaData::ExistsAttribute;

        entryFlags = QFileSystemMetaData::SequentialType
            | QFileSystemMetaData::ExistsAttribute;

        break;

    case DT_LNK:
        knownFlagsMask = QFileSystemMetaData::LinkType;
        entryFlags = QFileSystemMetaData::LinkType;
        break;

    case DT_REG:
        knownFlagsMask = QFileSystemMetaData::LinkType
            | QFileSystemMetaData::FileType
            | QFileSystemMetaData::DirectoryType
            | QFileSystemMetaData::BundleType
            | QFileSystemMetaData::SequentialType
            | QFileSystemMetaData::ExistsAttribute;

        entryFlags = QFileSystemMetaData::FileType
            | QFileSystemMetaData::ExistsAttribute;

        break;

    case DT_UNKNOWN:
    default:
        clear();
    }
#else
    Q_UNUSED(entry)
#endif
}

#endif

//static
QString QFileSystemEngine::resolveUserName(const QFileSystemEntry &entry, QFileSystemMetaData &metaData)
{
#if defined(Q_OS_WIN)
    Q_UNUSED(metaData);
    return QFileSystemEngine::owner(entry, QAbstractFileEngine::OwnerUser);
#else //(Q_OS_UNIX)
    if (!metaData.hasFlags(QFileSystemMetaData::UserId))
        QFileSystemEngine::fillMetaData(entry, metaData, QFileSystemMetaData::UserId);
    return resolveUserName(metaData.userId());
#endif
}

//static
QString QFileSystemEngine::resolveGroupName(const QFileSystemEntry &entry, QFileSystemMetaData &metaData)
{
#if defined(Q_OS_WIN)
    Q_UNUSED(metaData);
    return QFileSystemEngine::owner(entry, QAbstractFileEngine::OwnerGroup);
#else //(Q_OS_UNIX)
    if (!metaData.hasFlags(QFileSystemMetaData::GroupId))
        QFileSystemEngine::fillMetaData(entry, metaData, QFileSystemMetaData::GroupId);
    return resolveGroupName(metaData.groupId());
#endif
}

QT_END_NAMESPACE
