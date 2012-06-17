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

#include "qplatformdefs.h"
#include "qfileinfo.h"
#include "qglobal.h"
#include "qdir.h"
#include "qfileinfo_p.h"

QT_BEGIN_NAMESPACE

QString QFileInfoPrivate::getFileName(QAbstractFileEngine::FileName name) const
{
    if (cache_enabled && !fileNames[(int)name].isNull())
        return fileNames[(int)name];

    QString ret;
    if (fileEngine == 0) { // local file; use the QFileSystemEngine directly
        switch (name) {
            case QAbstractFileEngine::CanonicalName:
            case QAbstractFileEngine::CanonicalPathName: {
                QFileSystemEntry entry = QFileSystemEngine::canonicalName(fileEntry, metaData);
                if (cache_enabled) { // be smart and store both
                    fileNames[QAbstractFileEngine::CanonicalName] = entry.filePath();
                    fileNames[QAbstractFileEngine::CanonicalPathName] = entry.path();
                }
                if (name == QAbstractFileEngine::CanonicalName)
                    ret = entry.filePath();
                else
                    ret = entry.path();
                break;
            }
            case QAbstractFileEngine::LinkName:
                ret = QFileSystemEngine::getLinkTarget(fileEntry, metaData).filePath();
                break;
            case QAbstractFileEngine::BundleName:
                ret = QFileSystemEngine::bundleName(fileEntry);
                break;
            case QAbstractFileEngine::AbsoluteName:
            case QAbstractFileEngine::AbsolutePathName: {
                QFileSystemEntry entry = QFileSystemEngine::absoluteName(fileEntry);
                if (cache_enabled) { // be smart and store both
                    fileNames[QAbstractFileEngine::AbsoluteName] = entry.filePath();
                    fileNames[QAbstractFileEngine::AbsolutePathName] = entry.path();
                }
                if (name == QAbstractFileEngine::AbsoluteName)
                    ret = entry.filePath();
                else
                    ret = entry.path();
                break;
            }
            default: break;
        }
    } else {
        ret = fileEngine->fileName(name);
    }
    if (ret.isNull())
        ret = QLatin1String("");
    if (cache_enabled)
        fileNames[(int)name] = ret;
    return ret;
}

QString QFileInfoPrivate::getFileOwner(QAbstractFileEngine::FileOwner own) const
{
    if (cache_enabled && !fileOwners[(int)own].isNull())
        return fileOwners[(int)own];
    QString ret;
    if (fileEngine == 0) {
        switch (own) {
        case QAbstractFileEngine::OwnerUser:
            ret = QFileSystemEngine::resolveUserName(fileEntry, metaData);
            break;
        case QAbstractFileEngine::OwnerGroup:
            ret = QFileSystemEngine::resolveGroupName(fileEntry, metaData);
            break;
        }
     } else {
        ret = fileEngine->owner(own);
    }
    if (ret.isNull())
        ret = QLatin1String("");
    if (cache_enabled)
        fileOwners[(int)own] = ret;
    return ret;
}

uint QFileInfoPrivate::getFileFlags(QAbstractFileEngine::FileFlags request) const
{
    Q_ASSERT(fileEngine); // should never be called when using the native FS
    // We split the testing into tests for for LinkType, BundleType, PermsMask
    // and the rest.
    // Tests for file permissions on Windows can be slow, expecially on network
    // paths and NTFS drives.
    // In order to determine if a file is a symlink or not, we have to lstat().
    // If we're not interested in that information, we might as well avoid one
    // extra syscall. Bundle detecton on Mac can be slow, expecially on network
    // paths, so we separate out that as well.

    QAbstractFileEngine::FileFlags req = 0;
    uint cachedFlags = 0;

    if (request & (QAbstractFileEngine::FlagsMask | QAbstractFileEngine::TypesMask)) {
        if (!getCachedFlag(CachedFileFlags)) {
            req |= QAbstractFileEngine::FlagsMask;
            req |= QAbstractFileEngine::TypesMask;
            req &= (~QAbstractFileEngine::LinkType);
            req &= (~QAbstractFileEngine::BundleType);

            cachedFlags |= CachedFileFlags;
        }

        if (request & QAbstractFileEngine::LinkType) {
            if (!getCachedFlag(CachedLinkTypeFlag)) {
                req |= QAbstractFileEngine::LinkType;
                cachedFlags |= CachedLinkTypeFlag;
            }
        }

        if (request & QAbstractFileEngine::BundleType) {
            if (!getCachedFlag(CachedBundleTypeFlag)) {
                req |= QAbstractFileEngine::BundleType;
                cachedFlags |= CachedBundleTypeFlag;
            }
        }
    }

    if (request & QAbstractFileEngine::PermsMask) {
        if (!getCachedFlag(CachedPerms)) {
            req |= QAbstractFileEngine::PermsMask;
            cachedFlags |= CachedPerms;
        }
    }

    if (req) {
        if (cache_enabled)
            req &= (~QAbstractFileEngine::Refresh);
        else
            req |= QAbstractFileEngine::Refresh;

        QAbstractFileEngine::FileFlags flags = fileEngine->fileFlags(req);
        fileFlags |= uint(flags);
        setCachedFlag(cachedFlags);
    }

    return fileFlags & request;
}

QDateTime &QFileInfoPrivate::getFileTime(QAbstractFileEngine::FileTime request) const
{
    Q_ASSERT(fileEngine); // should never be called when using the native FS
    if (!cache_enabled)
        clearFlags();
    uint cf;
    if (request == QAbstractFileEngine::CreationTime)
        cf = CachedCTime;
    else if (request == QAbstractFileEngine::ModificationTime)
        cf = CachedMTime;
    else
        cf = CachedATime;
    if (!getCachedFlag(cf)) {
        fileTimes[request] = fileEngine->fileTime(request);
        setCachedFlag(cf);
    }
    return fileTimes[request];
}

//************* QFileInfo

/*!
    \class QFileInfo
    \reentrant
    \brief The QFileInfo class provides system-independent file information.

    \ingroup io
    \ingroup shared

    QFileInfo provides information about a file's name and position
    (path) in the file system, its access rights and whether it is a
    directory or symbolic link, etc. The file's size and last
    modified/read times are also available. QFileInfo can also be
    used to obtain information about a Qt \l{resource
    system}{resource}.

    A QFileInfo can point to a file with either a relative or an
    absolute file path. Absolute file paths begin with the directory
    separator "/" (or with a drive specification on Windows). Relative
    file names begin with a directory name or a file name and specify
    a path relative to the current working directory. An example of an
    absolute path is the string "/tmp/quartz". A relative path might
    look like "src/fatlib". You can use the function isRelative() to
    check whether a QFileInfo is using a relative or an absolute file
    path. You can call the function makeAbsolute() to convert a
    relative QFileInfo's path to an absolute path.

    The file that the QFileInfo works on is set in the constructor or
    later with setFile(). Use exists() to see if the file exists and
    size() to get its size.

    The file's type is obtained with isFile(), isDir() and
    isSymLink(). The symLinkTarget() function provides the name of the file
    the symlink points to.

    On Unix (including Mac OS X), the symlink has the same size() has
    the file it points to, because Unix handles symlinks
    transparently; similarly, opening a symlink using QFile
    effectively opens the link's target. For example:

    \snippet doc/src/snippets/code/src_corelib_io_qfileinfo.cpp 0

    On Windows, symlinks (shortcuts) are \c .lnk files. The reported
    size() is that of the symlink (not the link's target), and
    opening a symlink using QFile opens the \c .lnk file. For
    example:

    \snippet doc/src/snippets/code/src_corelib_io_qfileinfo.cpp 1

    Elements of the file's name can be extracted with path() and
    fileName(). The fileName()'s parts can be extracted with
    baseName(), suffix() or completeSuffix(). QFileInfo objects to
    directories created by Qt classes will not have a trailing file
    separator. If you wish to use trailing separators in your own file
    info objects, just append one to the file name given to the constructors
    or setFile().

    The file's dates are returned by created(), lastModified() and
    lastRead(). Information about the file's access permissions is
    obtained with isReadable(), isWritable() and isExecutable(). The
    file's ownership is available from owner(), ownerId(), group() and
    groupId(). You can examine a file's permissions and ownership in a
    single statement using the permission() function.

    \section1 Performance Issues

    Some of QFileInfo's functions query the file system, but for
    performance reasons, some functions only operate on the
    file name itself. For example: To return the absolute path of
    a relative file name, absolutePath() has to query the file system.
    The path() function, however, can work on the file name directly,
    and so it is faster.

    \note To speed up performance, QFileInfo caches information about
    the file.

    To speed up performance, QFileInfo caches information about the
    file. Because files can be changed by other users or programs, or
    even by other parts of the same program, there is a function that
    refreshes the file information: refresh(). If you want to switch
    off a QFileInfo's caching and force it to access the file system
    every time you request information from it call setCaching(false).

    \sa QDir, QFile
*/

/*!
    \internal
*/
QFileInfo::QFileInfo(QFileInfoPrivate *p) : d_ptr(p)
{
}

/*!
    Constructs an empty QFileInfo object.

    Note that an empty QFileInfo object contain no file reference.

    \sa setFile()
*/
QFileInfo::QFileInfo() : d_ptr(new QFileInfoPrivate())
{
}

/*!
    Constructs a new QFileInfo that gives information about the given
    file. The \a file can also include an absolute or relative path.

    \sa setFile(), isRelative(), QDir::setCurrent(), QDir::isRelativePath()
*/
QFileInfo::QFileInfo(const QString &file) : d_ptr(new QFileInfoPrivate(file))
{
}

/*!
    Constructs a new QFileInfo that gives information about file \a
    file.

    If the \a file has a relative path, the QFileInfo will also have a
    relative path.

    \sa isRelative()
*/
QFileInfo::QFileInfo(const QFile &file) : d_ptr(new QFileInfoPrivate(file.fileName()))
{
}

/*!
    Constructs a new QFileInfo that gives information about the given
    \a file in the directory \a dir.

    If \a dir has a relative path, the QFileInfo will also have a
    relative path.

    If \a file is an absolute path, then the directory specified
    by \a dir will be disregarded.

    \sa isRelative()
*/
QFileInfo::QFileInfo(const QDir &dir, const QString &file)
    : d_ptr(new QFileInfoPrivate(dir.filePath(file)))
{
}

/*!
    Constructs a new QFileInfo that is a copy of the given \a fileinfo.
*/
QFileInfo::QFileInfo(const QFileInfo &fileinfo)
    : d_ptr(fileinfo.d_ptr)
{

}

/*!
    Destroys the QFileInfo and frees its resources.
*/

QFileInfo::~QFileInfo()
{
}

/*!
    \fn bool QFileInfo::operator!=(const QFileInfo &fileinfo)

    Returns true if this QFileInfo object refers to a different file
    than the one specified by \a fileinfo; otherwise returns false.

    \sa operator==()
*/

/*!
    \overload
    \fn bool QFileInfo::operator!=(const QFileInfo &fileinfo) const
*/

/*!
    \overload
*/
bool QFileInfo::operator==(const QFileInfo &fileinfo) const
{
    Q_D(const QFileInfo);
    // ### Qt 5: understand long and short file names on Windows
    // ### (GetFullPathName()).
    if (fileinfo.d_ptr == d_ptr)
        return true;
    if (d->isDefaultConstructed || fileinfo.d_ptr->isDefaultConstructed)
        return false;

    // Assume files are the same if path is the same
    if (d->fileEntry.filePath() == fileinfo.d_ptr->fileEntry.filePath())
        return true;

    Qt::CaseSensitivity sensitive;
    if (d->fileEngine == 0 || fileinfo.d_ptr->fileEngine == 0) {
        if (d->fileEngine != fileinfo.d_ptr->fileEngine) // one is native, the other is a custom file-engine
            return false;

        sensitive = QFileSystemEngine::isCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    } else {
        if (d->fileEngine->caseSensitive() != fileinfo.d_ptr->fileEngine->caseSensitive())
            return false;
        sensitive = d->fileEngine->caseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    }

    if (fileinfo.size() != size()) //if the size isn't the same...
        return false;

   // Fallback to expensive canonical path computation
   return canonicalFilePath().compare(fileinfo.canonicalFilePath(), sensitive) == 0;
}

/*!
    Returns true if this QFileInfo object refers to a file in the same
    location as \a fileinfo; otherwise returns false.

    Note that the result of comparing two empty QFileInfo objects,
    containing no file references, is undefined.

    \warning This will not compare two different symbolic links
    pointing to the same file.

    \warning Long and short file names that refer to the same file on Windows
    are treated as if they referred to different files.

    \sa operator!=()
*/
bool QFileInfo::operator==(const QFileInfo &fileinfo)
{
    return const_cast<const QFileInfo *>(this)->operator==(fileinfo);
}

/*!
    Makes a copy of the given \a fileinfo and assigns it to this QFileInfo.
*/
QFileInfo &QFileInfo::operator=(const QFileInfo &fileinfo)
{
    d_ptr = fileinfo.d_ptr;
    return *this;
}

/*!
    Sets the file that the QFileInfo provides information about to \a
    file.

    The \a file can also include an absolute or relative file path.
    Absolute paths begin with the directory separator (e.g. "/" under
    Unix) or a drive specification (under Windows). Relative file
    names begin with a directory name or a file name and specify a
    path relative to the current directory.

    Example:
    \snippet doc/src/snippets/code/src_corelib_io_qfileinfo.cpp 2

    \sa isRelative(), QDir::setCurrent(), QDir::isRelativePath()
*/
void QFileInfo::setFile(const QString &file)
{
    bool caching = d_ptr.constData()->cache_enabled;
    *this = QFileInfo(file);
    d_ptr->cache_enabled = caching;
}

/*!
    \overload

    Sets the file that the QFileInfo provides information about to \a
    file.

    If \a file includes a relative path, the QFileInfo will also have
    a relative path.

    \sa isRelative()
*/
void QFileInfo::setFile(const QFile &file)
{
    setFile(file.fileName());
}

/*!
    \overload

    Sets the file that the QFileInfo provides information about to \a
    file in directory \a dir.

    If \a file includes a relative path, the QFileInfo will also
    have a relative path.

    \sa isRelative()
*/
void QFileInfo::setFile(const QDir &dir, const QString &file)
{
    setFile(dir.filePath(file));
}

/*!
    Returns an absolute path including the file name.

    The absolute path name consists of the full path and the file
    name. On Unix this will always begin with the root, '/',
    directory. On Windows this will always begin 'D:/' where D is a
    drive letter, except for network shares that are not mapped to a
    drive letter, in which case the path will begin '//sharename/'.
    QFileInfo will uppercase drive letters. Note that QDir does not do
    this. The code snippet below shows this.

    \snippet doc/src/snippets/code/src_corelib_io_qfileinfo.cpp newstuff

    This function returns the same as filePath(), unless isRelative()
    is true. In contrast to canonicalFilePath(), symbolic links or
    redundant "." or ".." elements are not necessarily removed.

    If the QFileInfo is empty it returns QDir::currentPath().

    \sa filePath(), canonicalFilePath(), isRelative()
*/
QString QFileInfo::absoluteFilePath() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->getFileName(QAbstractFileEngine::AbsoluteName);
}

/*!
    Returns the canonical path including the file name, i.e. an absolute
    path without symbolic links or redundant "." or ".." elements.

    If the file does not exist, canonicalFilePath() returns an empty
    string.

    \sa filePath(), absoluteFilePath(), dir()
*/
QString QFileInfo::canonicalFilePath() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->getFileName(QAbstractFileEngine::CanonicalName);
}


/*!
    Returns a file's path absolute path. This doesn't include the
    file name.

    On Unix the absolute path will always begin with the root, '/',
    directory. On Windows this will always begin 'D:/' where D is a
    drive letter, except for network shares that are not mapped to a
    drive letter, in which case the path will begin '//sharename/'.

    In contrast to canonicalPath() symbolic links or redundant "." or
    ".." elements are not necessarily removed.

    \warning If the QFileInfo object was created with an empty QString,
              the behavior of this function is undefined.

    \sa absoluteFilePath(), path(), canonicalPath(), fileName(), isRelative()
*/
QString QFileInfo::absolutePath() const
{
    Q_D(const QFileInfo);

    if (d->isDefaultConstructed) {
        return QLatin1String("");
    } else if (d->fileEntry.isEmpty()) {
        qWarning("QFileInfo::absolutePath: Constructed with empty filename");
        return QLatin1String("");
    }
    return d->getFileName(QAbstractFileEngine::AbsolutePathName);
}

/*!
    Returns the file's path canonical path (excluding the file name),
    i.e. an absolute path without symbolic links or redundant "." or ".." elements.

    If the file does not exist, canonicalPath() returns an empty string.

    \sa path(), absolutePath()
*/
QString QFileInfo::canonicalPath() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->getFileName(QAbstractFileEngine::CanonicalPathName);
}

/*!
    Returns the file's path. This doesn't include the file name.

    Note that, if this QFileInfo object is given a path ending in a
    slash, the name of the file is considered empty and this function
    will return the entire path.

    \sa filePath(), absolutePath(), canonicalPath(), dir(), fileName(), isRelative()
*/
QString QFileInfo::path() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->fileEntry.path();
}

/*!
    \fn bool QFileInfo::isAbsolute() const

    Returns true if the file path name is absolute, otherwise returns
    false if the path is relative.

    \sa isRelative()
*/

/*!
    Returns true if the file path name is relative, otherwise returns
    false if the path is absolute (e.g. under Unix a path is absolute
    if it begins with a "/").

    \sa isAbsolute()
*/
bool QFileInfo::isRelative() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return true;
    if (d->fileEngine == 0)
        return d->fileEntry.isRelative();
    return d->fileEngine->isRelativePath();
}

/*!
    Converts the file's path to an absolute path if it is not already in that form.
    Returns true to indicate that the path was converted; otherwise returns false
    to indicate that the path was already absolute.

    \sa filePath(), isRelative()
*/
bool QFileInfo::makeAbsolute()
{
    if (d_ptr.constData()->isDefaultConstructed
            || !d_ptr.constData()->fileEntry.isRelative())
        return false;

    setFile(absoluteFilePath());
    return true;
}

/*!
    Returns true if the file exists; otherwise returns false.

    \note If the file is a symlink that points to a non existing
     file, false is returned.
*/
bool QFileInfo::exists() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return false;
    if (d->fileEngine == 0) {
        if (!d->cache_enabled || !d->metaData.hasFlags(QFileSystemMetaData::ExistsAttribute))
            QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::ExistsAttribute);
        return d->metaData.exists();
    }
    return d->getFileFlags(QAbstractFileEngine::ExistsFlag);
}

/*!
    Refreshes the information about the file, i.e. reads in information
    from the file system the next time a cached property is fetched.

   \note On Windows CE, there might be a delay for the file system driver
    to detect changes on the file.
*/
void QFileInfo::refresh()
{
    Q_D(QFileInfo);
    d->clear();
}

/*!
    Returns the file name, including the path (which may be absolute
    or relative).

    \sa absoluteFilePath(), canonicalFilePath(), isRelative()
*/
QString QFileInfo::filePath() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->fileEntry.filePath();
}

/*!
    Returns the name of the file, excluding the path.

    Example:
    \snippet doc/src/snippets/code/src_corelib_io_qfileinfo.cpp 3

    Note that, if this QFileInfo object is given a path ending in a
    slash, the name of the file is considered empty.

    \sa isRelative(), filePath(), baseName(), extension()
*/
QString QFileInfo::fileName() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->fileEntry.fileName();
}

/*!
    \since 4.3
    Returns the name of the bundle.

    On Mac OS X this returns the proper localized name for a bundle if the
    path isBundle(). On all other platforms an empty QString is returned.

    Example:
    \snippet doc/src/snippets/code/src_corelib_io_qfileinfo.cpp 4

    \sa isBundle(), filePath(), baseName(), extension()
*/
QString QFileInfo::bundleName() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->getFileName(QAbstractFileEngine::BundleName);
}

/*!
    Returns the base name of the file without the path.

    The base name consists of all characters in the file up to (but
    not including) the \e first '.' character.

    Example:
    \snippet doc/src/snippets/code/src_corelib_io_qfileinfo.cpp 5


    The base name of a file is computed equally on all platforms, independent
    of file naming conventions (e.g., ".bashrc" on Unix has an empty base
    name, and the suffix is "bashrc").

    \sa fileName(), suffix(), completeSuffix(), completeBaseName()
*/
QString QFileInfo::baseName() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->fileEntry.baseName();
}

/*!
    Returns the complete base name of the file without the path.

    The complete base name consists of all characters in the file up
    to (but not including) the \e last '.' character.

    Example:
    \snippet doc/src/snippets/code/src_corelib_io_qfileinfo.cpp 6

    \sa fileName(), suffix(), completeSuffix(), baseName()
*/
QString QFileInfo::completeBaseName() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->fileEntry.completeBaseName();
}

/*!
    Returns the complete suffix of the file.

    The complete suffix consists of all characters in the file after
    (but not including) the first '.'.

    Example:
    \snippet doc/src/snippets/code/src_corelib_io_qfileinfo.cpp 7

    \sa fileName(), suffix(), baseName(), completeBaseName()
*/
QString QFileInfo::completeSuffix() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->fileEntry.completeSuffix();
}

/*!
    Returns the suffix of the file.

    The suffix consists of all characters in the file after (but not
    including) the last '.'.

    Example:
    \snippet doc/src/snippets/code/src_corelib_io_qfileinfo.cpp 8

    The suffix of a file is computed equally on all platforms, independent of
    file naming conventions (e.g., ".bashrc" on Unix has an empty base name,
    and the suffix is "bashrc").

    \sa fileName(), completeSuffix(), baseName(), completeBaseName()
*/
QString QFileInfo::suffix() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->fileEntry.suffix();
}


/*!
    Returns the path of the object's parent directory as a QDir object.

    \bold{Note:} The QDir returned always corresponds to the object's
    parent directory, even if the QFileInfo represents a directory.

    For each of the following, dir() returns a QDir for
    \c{"~/examples/191697"}.

    \snippet doc/src/snippets/fileinfo/main.cpp 0

    For each of the following, dir() returns a QDir for
    \c{"."}.

    \snippet doc/src/snippets/fileinfo/main.cpp 1

    \sa absolutePath(), filePath(), fileName(), isRelative(), absoluteDir()
*/
QDir QFileInfo::dir() const
{
    Q_D(const QFileInfo);
    // ### Qt5: Maybe rename this to parentDirectory(), considering what it actually do?
    return QDir(d->fileEntry.path());
}

/*!
    Returns the file's absolute path as a QDir object.

    \sa dir(), filePath(), fileName(), isRelative()
*/
QDir QFileInfo::absoluteDir() const
{
    return QDir(absolutePath());
}

#ifdef QT3_SUPPORT
/*!
    Use absoluteDir() or the dir() overload that takes no parameters
    instead.
*/
QDir QFileInfo::dir(bool absPath) const
{
    if (absPath)
        return absoluteDir();
    return dir();
}
#endif //QT3_SUPPORT

/*!
    Returns true if the user can read the file; otherwise returns false.

    \sa isWritable(), isExecutable(), permission()
*/
bool QFileInfo::isReadable() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return false;
    if (d->fileEngine == 0) {
        if (!d->cache_enabled || !d->metaData.hasFlags(QFileSystemMetaData::UserReadPermission))
            QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::UserReadPermission);
        return (d->metaData.permissions() & QFile::ReadUser) != 0;
    }
    return d->getFileFlags(QAbstractFileEngine::ReadUserPerm);
}

/*!
    Returns true if the user can write to the file; otherwise returns false.

    \sa isReadable(), isExecutable(), permission()
*/
bool QFileInfo::isWritable() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return false;
    if (d->fileEngine == 0) {
        if (!d->cache_enabled || !d->metaData.hasFlags(QFileSystemMetaData::UserWritePermission))
            QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::UserWritePermission);
        return (d->metaData.permissions() & QFile::WriteUser) != 0;
    }
    return d->getFileFlags(QAbstractFileEngine::WriteUserPerm);
}

/*!
    Returns true if the file is executable; otherwise returns false.

    \sa isReadable(), isWritable(), permission()
*/
bool QFileInfo::isExecutable() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return false;
    if (d->fileEngine == 0) {
        if (!d->cache_enabled || !d->metaData.hasFlags(QFileSystemMetaData::UserExecutePermission))
            QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::UserExecutePermission);
        return (d->metaData.permissions() & QFile::ExeUser) != 0;
    }
    return d->getFileFlags(QAbstractFileEngine::ExeUserPerm);
}

/*!
    Returns true if this is a `hidden' file; otherwise returns false.

    \bold{Note:} This function returns true for the special entries
    "." and ".." on Unix, even though QDir::entryList threats them as shown.
*/
bool QFileInfo::isHidden() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return false;
    if (d->fileEngine == 0) {
        if (!d->cache_enabled || !d->metaData.hasFlags(QFileSystemMetaData::HiddenAttribute))
            QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::HiddenAttribute);
        return d->metaData.isHidden();
    }
    return d->getFileFlags(QAbstractFileEngine::HiddenFlag);
}

/*!
    Returns true if this object points to a file or to a symbolic
    link to a file. Returns false if the
    object points to something which isn't a file, such as a directory.

    \sa isDir(), isSymLink(), isBundle()
*/
bool QFileInfo::isFile() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return false;
    if (d->fileEngine == 0) {
        if (!d->cache_enabled || !d->metaData.hasFlags(QFileSystemMetaData::FileType))
            QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::FileType);
        return d->metaData.isFile();
    }
    return d->getFileFlags(QAbstractFileEngine::FileType);
}

/*!
    Returns true if this object points to a directory or to a symbolic
    link to a directory; otherwise returns false.

    \sa isFile(), isSymLink(), isBundle()
*/
bool QFileInfo::isDir() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return false;
    if (d->fileEngine == 0) {
        if (!d->cache_enabled || !d->metaData.hasFlags(QFileSystemMetaData::DirectoryType))
            QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::DirectoryType);
        return d->metaData.isDirectory();
    }
    return d->getFileFlags(QAbstractFileEngine::DirectoryType);
}


/*!
    \since 4.3
    Returns true if this object points to a bundle or to a symbolic
    link to a bundle on Mac OS X; otherwise returns false.

    \sa isDir(), isSymLink(), isFile()
*/
bool QFileInfo::isBundle() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return false;
    if (d->fileEngine == 0) {
        if (!d->cache_enabled || !d->metaData.hasFlags(QFileSystemMetaData::BundleType))
            QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::BundleType);
        return d->metaData.isBundle();
    }
    return d->getFileFlags(QAbstractFileEngine::BundleType);
}

/*!
    Returns true if this object points to a symbolic link (or to a
    shortcut on Windows); otherwise returns false.

    On Unix (including Mac OS X), opening a symlink effectively opens
    the \l{symLinkTarget()}{link's target}. On Windows, it opens the \c
    .lnk file itself.

    Example:

    \snippet doc/src/snippets/code/src_corelib_io_qfileinfo.cpp 9

    \note If the symlink points to a non existing file, exists() returns
     false.

    \sa isFile(), isDir(), symLinkTarget()
*/
bool QFileInfo::isSymLink() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return false;
    if (d->fileEngine == 0) {
        if (!d->cache_enabled || !d->metaData.hasFlags(QFileSystemMetaData::LegacyLinkType))
            QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::LegacyLinkType);
        return d->metaData.isLegacyLink();
    }
    return d->getFileFlags(QAbstractFileEngine::LinkType);
}

/*!
    Returns true if the object points to a directory or to a symbolic
    link to a directory, and that directory is the root directory; otherwise
    returns false.
*/
bool QFileInfo::isRoot() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return true;
    if (d->fileEngine == 0) {
        if (d->fileEntry.isRoot()) {
#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN) 
            //the path is a drive root, but the drive may not exist
            //for backward compatibility, return true only if the drive exists
            if (!d->cache_enabled || !d->metaData.hasFlags(QFileSystemMetaData::ExistsAttribute))
                QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::ExistsAttribute);
            return d->metaData.exists();
#else
            return true;
#endif
        }
        return false;
    }
    return d->getFileFlags(QAbstractFileEngine::RootFlag);
}

/*!
    \fn QString QFileInfo::symLinkTarget() const
    \since 4.2

    Returns the absolute path to the file or directory a symlink (or shortcut
    on Windows) points to, or a an empty string if the object isn't a symbolic
    link.

    This name may not represent an existing file; it is only a string.
    QFileInfo::exists() returns true if the symlink points to an
    existing file.

    \sa exists(), isSymLink(), isDir(), isFile()
*/

/*!
    \obsolete

    Use symLinkTarget() instead.
*/
QString QFileInfo::readLink() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->getFileName(QAbstractFileEngine::LinkName);
}

/*!
    Returns the owner of the file. On systems where files
    do not have owners, or if an error occurs, an empty string is
    returned.

    This function can be time consuming under Unix (in the order of
    milliseconds).

    \sa ownerId(), group(), groupId()
*/
QString QFileInfo::owner() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->getFileOwner(QAbstractFileEngine::OwnerUser);
}

/*!
    Returns the id of the owner of the file.

    On Windows and on systems where files do not have owners this
    function returns ((uint) -2).

    \sa owner(), group(), groupId()
*/
uint QFileInfo::ownerId() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return 0;
    if (d->fileEngine == 0) {
        if (!d->cache_enabled || !d->metaData.hasFlags(QFileSystemMetaData::UserId))
            QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::UserId);
        return d->metaData.userId();
    }
    return d->fileEngine->ownerId(QAbstractFileEngine::OwnerUser);
}

/*!
    Returns the group of the file. On Windows, on systems where files
    do not have groups, or if an error occurs, an empty string is
    returned.

    This function can be time consuming under Unix (in the order of
    milliseconds).

    \sa groupId(), owner(), ownerId()
*/
QString QFileInfo::group() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->getFileOwner(QAbstractFileEngine::OwnerGroup);
}

/*!
    Returns the id of the group the file belongs to.

    On Windows and on systems where files do not have groups this
    function always returns (uint) -2.

    \sa group(), owner(), ownerId()
*/
uint QFileInfo::groupId() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return 0;
    if (d->fileEngine == 0) {
        if (!d->cache_enabled || !d->metaData.hasFlags(QFileSystemMetaData::GroupId))
            QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::GroupId);
        return d->metaData.groupId();
    }
    return d->fileEngine->ownerId(QAbstractFileEngine::OwnerGroup);
}

/*!
    Tests for file permissions. The \a permissions argument can be
    several flags of type QFile::Permissions OR-ed together to check
    for permission combinations.

    On systems where files do not have permissions this function
    always returns true.

    Example:
    \snippet doc/src/snippets/code/src_corelib_io_qfileinfo.cpp 10

    \sa isReadable(), isWritable(), isExecutable()
*/
bool QFileInfo::permission(QFile::Permissions permissions) const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return false;
    if (d->fileEngine == 0) {
        // the QFileSystemMetaData::MetaDataFlag and QFile::Permissions overlap, so just static cast.
        QFileSystemMetaData::MetaDataFlag permissionFlags = static_cast<QFileSystemMetaData::MetaDataFlag>((int)permissions);
        if (!d->cache_enabled || !d->metaData.hasFlags(permissionFlags))
            QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, permissionFlags);
        return (d->metaData.permissions() & permissions) == permissions;
    }
    return d->getFileFlags(QAbstractFileEngine::FileFlags((int)permissions)) == (uint)permissions;
}

/*!
    Returns the complete OR-ed together combination of
    QFile::Permissions for the file.
*/
QFile::Permissions QFileInfo::permissions() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return 0;
    if (d->fileEngine == 0) {
        if (!d->cache_enabled || !d->metaData.hasFlags(QFileSystemMetaData::Permissions))
            QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::Permissions);
        return d->metaData.permissions();
    }
    return QFile::Permissions(d->getFileFlags(QAbstractFileEngine::PermsMask) & QAbstractFileEngine::PermsMask);
}


/*!
    Returns the file size in bytes. If the file does not exist or cannot be
    fetched, 0 is returned.

    \sa exists()
*/
qint64 QFileInfo::size() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return 0;
    if (d->fileEngine == 0) {
        if (!d->cache_enabled || !d->metaData.hasFlags(QFileSystemMetaData::SizeAttribute))
            QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::SizeAttribute);
        return d->metaData.size();
    }
    if (!d->getCachedFlag(QFileInfoPrivate::CachedSize)) {
        d->setCachedFlag(QFileInfoPrivate::CachedSize);
        d->fileSize = d->fileEngine->size();
    }
    return d->fileSize;
}

/*!
    Returns the date and time when the file was created.

    On most Unix systems, this function returns the time of the last
    status change. A status change occurs when the file is created,
    but it also occurs whenever the user writes or sets inode
    information (for example, changing the file permissions).

    If neither creation time nor "last status change" time are not
    available, returns the same as lastModified().

    \sa lastModified() lastRead()
*/
QDateTime QFileInfo::created() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QDateTime();
    if (d->fileEngine == 0) {
        if (!d->cache_enabled || !d->metaData.hasFlags(QFileSystemMetaData::CreationTime))
            QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::CreationTime);
        return d->metaData.creationTime();
    }
    return d->getFileTime(QAbstractFileEngine::CreationTime);
}

/*!
    Returns the date and time when the file was last modified.

    \sa created() lastRead()
*/
QDateTime QFileInfo::lastModified() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QDateTime();
    if (d->fileEngine == 0) {
        if (!d->cache_enabled || !d->metaData.hasFlags(QFileSystemMetaData::ModificationTime))
            QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::ModificationTime);
        return d->metaData.modificationTime();
    }
    return d->getFileTime(QAbstractFileEngine::ModificationTime);
}

/*!
    Returns the date and time when the file was last read (accessed).

    On platforms where this information is not available, returns the
    same as lastModified().

    \sa created() lastModified()
*/
QDateTime QFileInfo::lastRead() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QDateTime();
    if (d->fileEngine == 0) {
        if (!d->cache_enabled || !d->metaData.hasFlags(QFileSystemMetaData::AccessTime))
            QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::AccessTime);
        return d->metaData.accessTime();
    }
    return d->getFileTime(QAbstractFileEngine::AccessTime);
}

/*! \internal
    Detaches all internal data.
*/
void QFileInfo::detach()
{
    d_ptr.detach();
}

/*!
    Returns true if caching is enabled; otherwise returns false.

    \sa setCaching(), refresh()
*/
bool QFileInfo::caching() const
{
    Q_D(const QFileInfo);
    return d->cache_enabled;
}

/*!
    If \a enable is true, enables caching of file information. If \a
    enable is false caching is disabled.

    When caching is enabled, QFileInfo reads the file information from
    the file system the first time it's needed, but generally not
    later.

    Caching is enabled by default.

    \sa refresh(), caching()
*/
void QFileInfo::setCaching(bool enable)
{
    Q_D(QFileInfo);
    d->cache_enabled = enable;
}

/*!
    \fn QString QFileInfo::baseName(bool complete)

    Use completeBaseName() or the baseName() overload that takes no
    parameters instead.
*/

/*!
    \fn QString QFileInfo::extension(bool complete = true) const

    Use completeSuffix() or suffix() instead.
*/

/*!
    \fn QString QFileInfo::absFilePath() const

    Use absoluteFilePath() instead.
*/

/*!
    \fn QString QFileInfo::dirPath(bool absPath) const

    Use absolutePath() if the absolute path is wanted (\a absPath
    is true) or path() if it's not necessary (\a absPath is false).
*/

/*!
    \fn bool QFileInfo::convertToAbs()

    Use makeAbsolute() instead.
*/

/*!
    \enum QFileInfo::Permission

    \compat

    \value ReadOwner
    \value WriteOwner
    \value ExeOwner
    \value ReadUser
    \value WriteUser
    \value ExeUser
    \value ReadGroup
    \value WriteGroup
    \value ExeGroup
    \value ReadOther
    \value WriteOther
    \value ExeOther
*/

/*!
    \fn bool QFileInfo::permission(PermissionSpec permissions) const
    \compat

    Use permission() instead.
*/

/*!
    \typedef QFileInfoList
    \relates QFileInfo

    Synonym for QList<QFileInfo>.
*/

QT_END_NAMESPACE
