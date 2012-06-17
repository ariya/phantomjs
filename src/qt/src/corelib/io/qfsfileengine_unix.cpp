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
#include "qabstractfileengine.h"
#include "private/qfsfileengine_p.h"
#include "private/qcore_unix_p.h"
#include "qfilesystementry_p.h"
#include "qfilesystemengine_p.h"

#ifndef QT_NO_FSFILEENGINE

#include "qfile.h"
#include "qdir.h"
#include "qdatetime.h"
#include "qvarlengtharray.h"

#include <sys/mman.h>
#include <stdlib.h>
#include <limits.h>
#if defined(Q_OS_SYMBIAN)
# include <sys/syslimits.h>
# include <f32file.h>
# include <pathinfo.h>
# include "private/qcore_symbian_p.h"
#endif
#include <errno.h>
#if !defined(QWS) && defined(Q_OS_MAC)
# include <private/qcore_mac_p.h>
#endif

QT_BEGIN_NAMESPACE

#if defined(Q_OS_SYMBIAN)
/*!
    \internal

    Returns true if supplied path is a relative path
*/
static bool isRelativePathSymbian(const QString& fileName)
{
    return !(fileName.startsWith(QLatin1Char('/'))
             || (fileName.length() >= 2
             && ((fileName.at(0).isLetter() && fileName.at(1) == QLatin1Char(':'))
             || (fileName.at(0) == QLatin1Char('/') && fileName.at(1) == QLatin1Char('/')))));
}

#endif

#ifndef Q_OS_SYMBIAN
/*!
    \internal

    Returns the stdlib open string corresponding to a QIODevice::OpenMode.
*/
static inline QByteArray openModeToFopenMode(QIODevice::OpenMode flags, const QFileSystemEntry &fileEntry,
        QFileSystemMetaData &metaData)
{
    QByteArray mode;
    if ((flags & QIODevice::ReadOnly) && !(flags & QIODevice::Truncate)) {
        mode = "rb";
        if (flags & QIODevice::WriteOnly) {
            metaData.clearFlags(QFileSystemMetaData::FileType);
            if (!fileEntry.isEmpty()
                    && QFileSystemEngine::fillMetaData(fileEntry, metaData, QFileSystemMetaData::FileType)
                    && metaData.isFile()) {
                mode += '+';
            } else {
                mode = "wb+";
            }
        }
    } else if (flags & QIODevice::WriteOnly) {
        mode = "wb";
        if (flags & QIODevice::ReadOnly)
            mode += '+';
    }
    if (flags & QIODevice::Append) {
        mode = "ab";
        if (flags & QIODevice::ReadOnly)
            mode += '+';
    }

#if defined(__GLIBC__) && (__GLIBC__ * 0x100 + __GLIBC_MINOR__) >= 0x0207
    // must be glibc >= 2.7
    mode += 'e';
#endif

    return mode;
}
#endif

/*!
    \internal

    Returns the stdio open flags corresponding to a QIODevice::OpenMode.
*/
static inline int openModeToOpenFlags(QIODevice::OpenMode mode)
{
    int oflags = QT_OPEN_RDONLY;
#ifdef QT_LARGEFILE_SUPPORT
    oflags |= QT_OPEN_LARGEFILE;
#endif

    if ((mode & QFile::ReadWrite) == QFile::ReadWrite) {
        oflags = QT_OPEN_RDWR | QT_OPEN_CREAT;
    } else if (mode & QFile::WriteOnly) {
        oflags = QT_OPEN_WRONLY | QT_OPEN_CREAT;
    }

    if (mode & QFile::Append) {
        oflags |= QT_OPEN_APPEND;
    } else if (mode & QFile::WriteOnly) {
        if ((mode & QFile::Truncate) || !(mode & QFile::ReadOnly))
            oflags |= QT_OPEN_TRUNC;
    }

    return oflags;
}

#ifndef Q_OS_SYMBIAN
/*!
    \internal

    Sets the file descriptor to close on exec. That is, the file
    descriptor is not inherited by child processes.
*/
static inline bool setCloseOnExec(int fd)
{
    return fd != -1 && fcntl(fd, F_SETFD, FD_CLOEXEC) != -1;
}
#endif

#ifdef Q_OS_SYMBIAN
/*!
    \internal
*/
bool QFSFileEnginePrivate::nativeOpen(QIODevice::OpenMode openMode)
{
    Q_Q(QFSFileEngine);
	
	fh = 0;
	fd = -1;

    QString fn(QFileSystemEngine::absoluteName(fileEntry).nativeFilePath());
    RFs& fs = qt_s60GetRFs();

    TUint symbianMode = 0;

    if(openMode & QIODevice::ReadOnly)
        symbianMode |= EFileRead;
    if(openMode & QIODevice::WriteOnly)
        symbianMode |= EFileWrite;
    if(openMode & QIODevice::Text)
        symbianMode |= EFileStreamText;

    if (openMode & QFile::Unbuffered) {
        if (openMode & QIODevice::WriteOnly)
            symbianMode |= 0x00001000; //EFileWriteDirectIO;
        // ### Unbuffered read is not used, because it prevents file open in /resource
        // ### and has no obvious benefits
    } else {
        if (openMode & QIODevice::WriteOnly)
            symbianMode |= 0x00000800; //EFileWriteBuffered;
        // use implementation defaults for read buffering
    }

    // Until Qt supports file sharing, we can't support EFileShareReadersOrWriters safely,
    // but Qt does this on other platforms and autotests rely on it.
    // The reason is that Unix locks are only advisory - the application needs to test the
    // lock after opening the file. Symbian and Windows locks are mandatory - opening a
    // locked file will fail.
    symbianMode |= EFileShareReadersOrWriters;

    TInt r;
    //note QIODevice::Truncate only has meaning for read/write access
    //write-only files are always truncated unless append is specified
    //reference openModeToOpenFlags in qfsfileengine_unix.cpp
    if ((openMode & QIODevice::Truncate) || (!(openMode & QIODevice::ReadOnly) && !(openMode & QIODevice::Append))) {
        r = symbianFile.Replace(fs, qt_QString2TPtrC(fn), symbianMode);
    } else {
        r = symbianFile.Open(fs, qt_QString2TPtrC(fn), symbianMode);
        if (r == KErrNotFound && (openMode & QIODevice::WriteOnly)) {
            r = symbianFile.Create(fs, qt_QString2TPtrC(fn), symbianMode);
        }
    }

    if (r == KErrNone) {
#ifdef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
        TInt64 size;
#else
        TInt size;
#endif
        r = symbianFile.Size(size);
        if (r==KErrNone) {
            if (openMode & QIODevice::Append)
                symbianFilePos = size;
            else
                symbianFilePos = 0;
            //TODO: port this (QFileSystemMetaData in open?)
            //cachedSize = size;
        }
    }

    if (r != KErrNone) {
        q->setError(QFile::OpenError, QSystemError(r, QSystemError::NativeError).toString());
        symbianFile.Close();
        return false;
    }

    closeFileHandle = true;
    return true;
}

/*!
    Opens the file descriptor specified by \a file in the mode given by
    \a openMode. Returns true on success; otherwise returns false.

    The \a handleFlags argument specifies whether the file handle will be
    closed by Qt. See the QFile::FileHandleFlags documentation for more
    information.
*/
bool QFSFileEngine::open(QIODevice::OpenMode openMode, const RFile &file, QFile::FileHandleFlags handleFlags)
{
    Q_D(QFSFileEngine);

    // Append implies WriteOnly.
    if (openMode & QFile::Append)
        openMode |= QFile::WriteOnly;

    // WriteOnly implies Truncate if neither ReadOnly nor Append are sent.
    if ((openMode & QFile::WriteOnly) && !(openMode & (QFile::ReadOnly | QFile::Append)))
        openMode |= QFile::Truncate;

    d->openMode = openMode;
    d->lastFlushFailed = false;
    d->closeFileHandle = (handleFlags & QFile::AutoCloseHandle);
    d->fileEntry.clear();
    d->fh = 0;
    d->fd = -1;
    d->tried_stat = 0;

#ifdef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
    //RFile64 adds only functions to RFile, no data members
    d->symbianFile = static_cast<const RFile64&>(file);
#else
    d->symbianFile = file;
#endif
    TInt ret;
    d->symbianFilePos = 0;
    if (openMode & QFile::Append) {
        // Seek to the end when in Append mode.
        ret = d->symbianFile.Size(d->symbianFilePos);
    } else {
        // Seek to current otherwise
        ret = d->symbianFile.Seek(ESeekCurrent, d->symbianFilePos);
    }

    if (ret != KErrNone) {
        setError(QFile::OpenError, QSystemError(ret, QSystemError::NativeError).toString());

        d->openMode = QIODevice::NotOpen;
#ifdef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
        d->symbianFile = RFile64();
#else
        d->symbianFile = RFile();
#endif
        return false;
    }

    // Extract filename (best effort)
    TFileName fn;
    TInt err = d->symbianFile.FullName(fn);
    if (err == KErrNone)
        d->fileEntry = QFileSystemEntry(qt_TDesC2QString(fn), QFileSystemEntry::FromNativePath());
    else
        d->fileEntry.clear();

    return true;
}
#else
/*!
    \internal
*/
bool QFSFileEnginePrivate::nativeOpen(QIODevice::OpenMode openMode)
{
    Q_Q(QFSFileEngine);

    if (openMode & QIODevice::Unbuffered) {
        int flags = openModeToOpenFlags(openMode);

        // Try to open the file in unbuffered mode.
        do {
            fd = QT_OPEN(fileEntry.nativeFilePath().constData(), flags, 0666);
        } while (fd == -1 && errno == EINTR);

        // On failure, return and report the error.
        if (fd == -1) {
            q->setError(errno == EMFILE ? QFile::ResourceError : QFile::OpenError,
                        qt_error_string(errno));
            return false;
        }

        if (!(openMode & QIODevice::WriteOnly)) {
            // we don't need this check if we tried to open for writing because then
            // we had received EISDIR anyway.
            if (QFileSystemEngine::fillMetaData(fd, metaData)
                    && metaData.isDirectory()) {
                q->setError(QFile::OpenError, QLatin1String("file to open is a directory"));
                QT_CLOSE(fd);
                return false;
            }
        }

        // Seek to the end when in Append mode.
        if (flags & QFile::Append) {
            int ret;
            do {
                ret = QT_LSEEK(fd, 0, SEEK_END);
            } while (ret == -1 && errno == EINTR);

            if (ret == -1) {
                q->setError(errno == EMFILE ? QFile::ResourceError : QFile::OpenError,
                            qt_error_string(int(errno)));
                return false;
            }
        }

        fh = 0;
    } else {
        QByteArray fopenMode = openModeToFopenMode(openMode, fileEntry, metaData);

        // Try to open the file in buffered mode.
        do {
            fh = QT_FOPEN(fileEntry.nativeFilePath().constData(), fopenMode.constData());
        } while (!fh && errno == EINTR);

        // On failure, return and report the error.
        if (!fh) {
            q->setError(errno == EMFILE ? QFile::ResourceError : QFile::OpenError,
                        qt_error_string(int(errno)));
            return false;
        }

        if (!(openMode & QIODevice::WriteOnly)) {
            // we don't need this check if we tried to open for writing because then
            // we had received EISDIR anyway.
            if (QFileSystemEngine::fillMetaData(QT_FILENO(fh), metaData)
                    && metaData.isDirectory()) {
                q->setError(QFile::OpenError, QLatin1String("file to open is a directory"));
                fclose(fh);
                return false;
            }
        }

        setCloseOnExec(fileno(fh)); // ignore failure

        // Seek to the end when in Append mode.
        if (openMode & QIODevice::Append) {
            int ret;
            do {
                ret = QT_FSEEK(fh, 0, SEEK_END);
            } while (ret == -1 && errno == EINTR);

            if (ret == -1) {
                q->setError(errno == EMFILE ? QFile::ResourceError : QFile::OpenError,
                            qt_error_string(int(errno)));
                return false;
            }
        }

        fd = -1;
    }

    closeFileHandle = true;
    return true;
}
#endif

/*!
    \internal
*/
bool QFSFileEnginePrivate::nativeClose()
{
    return closeFdFh();
}

/*!
    \internal

*/
bool QFSFileEnginePrivate::nativeFlush()
{
#ifdef Q_OS_SYMBIAN
    if (symbianFile.SubSessionHandle())
        return (KErrNone == symbianFile.Flush());
#endif
    return fh ? flushFh() : fd != -1;
}

/*!
    \internal
*/
qint64 QFSFileEnginePrivate::nativeRead(char *data, qint64 len)
{
    Q_Q(QFSFileEngine);

#ifdef Q_OS_SYMBIAN
    if (symbianFile.SubSessionHandle()) {
        if(len > KMaxTInt) {
            //this check is more likely to catch a corrupt length, since it isn't possible to allocate 2GB buffers (yet..)
            q->setError(QFile::ReadError, QLatin1String("Maximum 2GB in single read on this platform"));
            return -1;
        }
        TPtr8 ptr(reinterpret_cast<TUint8*>(data), static_cast<TInt>(len));
        TInt r = symbianFile.Read(symbianFilePos, ptr);
        if (r != KErrNone)
        {
            q->setError(QFile::ReadError, QSystemError(r, QSystemError::NativeError).toString());
            return -1;
        }
        symbianFilePos += ptr.Length();
        return qint64(ptr.Length());
    }
#endif
    if (fh && nativeIsSequential()) {
        size_t readBytes = 0;
        int oldFlags = fcntl(QT_FILENO(fh), F_GETFL);
        for (int i = 0; i < 2; ++i) {
            // Unix: Make the underlying file descriptor non-blocking
            if ((oldFlags & O_NONBLOCK) == 0)
                fcntl(QT_FILENO(fh), F_SETFL, oldFlags | O_NONBLOCK);

            // Cross platform stdlib read
            size_t read = 0;
            do {
                read = fread(data + readBytes, 1, size_t(len - readBytes), fh);
            } while (read == 0 && !feof(fh) && errno == EINTR);
            if (read > 0) {
                readBytes += read;
                break;
            } else {
                if (readBytes)
                    break;
                readBytes = read;
            }

            // Unix: Restore the blocking state of the underlying socket
            if ((oldFlags & O_NONBLOCK) == 0) {
                fcntl(QT_FILENO(fh), F_SETFL, oldFlags);
                if (readBytes == 0) {
                    int readByte = 0;
                    do {
                        readByte = fgetc(fh);
                    } while (readByte == -1 && errno == EINTR);
                    if (readByte != -1) {
                        *data = uchar(readByte);
                        readBytes += 1;
                    } else {
                        break;
                    }
                }
            }
        }
        // Unix: Restore the blocking state of the underlying socket
        if ((oldFlags & O_NONBLOCK) == 0) {
            fcntl(QT_FILENO(fh), F_SETFL, oldFlags);
        }
        if (readBytes == 0 && !feof(fh)) {
            // if we didn't read anything and we're not at EOF, it must be an error
            q->setError(QFile::ReadError, qt_error_string(int(errno)));
            return -1;
        }
        return readBytes;
    }

    return readFdFh(data, len);
}

/*!
    \internal
*/
qint64 QFSFileEnginePrivate::nativeReadLine(char *data, qint64 maxlen)
{
    return readLineFdFh(data, maxlen);
}

/*!
    \internal
*/
qint64 QFSFileEnginePrivate::nativeWrite(const char *data, qint64 len)
{
#ifdef Q_OS_SYMBIAN
    Q_Q(QFSFileEngine);
    if (symbianFile.SubSessionHandle()) {
        if(len > KMaxTInt) {
            //this check is more likely to catch a corrupt length, since it isn't possible to allocate 2GB buffers (yet..)
            q->setError(QFile::WriteError, QLatin1String("Maximum 2GB in single write on this platform"));
            return -1;
        }
        const TPtrC8 ptr(reinterpret_cast<const TUint8*>(data), static_cast<TInt>(len));
#ifdef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
        TInt64 eofpos = 0;
#else
        TInt eofpos = 0;
#endif
        //The end of file position is not cached because QFile is read/write sharable, therefore another
        //process may have altered the file size.
        TInt r = symbianFile.Seek(ESeekEnd, eofpos);
        if (r == KErrNone && symbianFilePos > eofpos) {
            //seek position is beyond end of file so file needs to be extended before write.
            //note that SetSize does not zero-initialise (c.f. posix lseek)
            r = symbianFile.SetSize(symbianFilePos);
        }
        if (r == KErrNone) {
            //write to specific position in the file (i.e. use our own cursor rather than calling seek)
            r = symbianFile.Write(symbianFilePos, ptr);
        }
        if (r != KErrNone) {
            q->setError(QFile::WriteError, QSystemError(r, QSystemError::NativeError).toString());
            return -1;
        }
        symbianFilePos += len;
        return len;
    }
#endif
    return writeFdFh(data, len);
}

/*!
    \internal
*/
qint64 QFSFileEnginePrivate::nativePos() const
{
#ifdef Q_OS_SYMBIAN
    const Q_Q(QFSFileEngine);
    if (symbianFile.SubSessionHandle()) {
        return symbianFilePos;
    }
#endif
    return posFdFh();
}

/*!
    \internal
*/
bool QFSFileEnginePrivate::nativeSeek(qint64 pos)
{
#ifdef Q_OS_SYMBIAN
    Q_Q(QFSFileEngine);
    if (symbianFile.SubSessionHandle()) {
#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
        if(pos > KMaxTInt) {
            q->setError(QFile::PositionError, QLatin1String("Maximum 2GB file position on this platform"));
            return false;
        }
#endif
        symbianFilePos = pos;
        return true;
    }
#endif
    return seekFdFh(pos);
}

/*!
    \internal
*/
int QFSFileEnginePrivate::nativeHandle() const
{
    return fh ? fileno(fh) : fd;
}

#if defined(Q_OS_SYMBIAN) && !defined(QT_SYMBIAN_USE_NATIVE_FILEMAP)
int QFSFileEnginePrivate::getMapHandle()
{
    if (symbianFile.SubSessionHandle()) {
        // Symbian file handle can't be used for open C mmap() so open the file with open C as well.
        if (fileHandleForMaps < 0) {
            int flags = openModeToOpenFlags(openMode);
            flags &= ~(O_CREAT | O_TRUNC);
            fileHandleForMaps = ::wopen((wchar_t*)(fileEntry.nativeFilePath().utf16()), flags, 0666);
        }
        return fileHandleForMaps;
    }
    return nativeHandle();
}
#endif

/*!
    \internal
*/
bool QFSFileEnginePrivate::nativeIsSequential() const
{
#ifdef Q_OS_SYMBIAN
    if (symbianFile.SubSessionHandle())
        return false;
#endif
    return isSequentialFdFh();
}

bool QFSFileEngine::remove()
{
    Q_D(QFSFileEngine);
    QSystemError error;
    bool ret = QFileSystemEngine::removeFile(d->fileEntry, error);
    d->metaData.clear();
    if (!ret) {
        setError(QFile::RemoveError, error.toString());
    }
    return ret;
}

bool QFSFileEngine::copy(const QString &newName)
{
    Q_D(QFSFileEngine);
    QSystemError error;
    bool ret = QFileSystemEngine::copyFile(d->fileEntry, QFileSystemEntry(newName), error);
    if (!ret) {
        setError(QFile::CopyError, error.toString());
    }
    return ret;
}

bool QFSFileEngine::rename(const QString &newName)
{
    Q_D(QFSFileEngine);
    QSystemError error;
    bool ret = QFileSystemEngine::renameFile(d->fileEntry, QFileSystemEntry(newName), error);

    if (!ret) {
        setError(QFile::RenameError, error.toString());
    }

    return ret;
}

bool QFSFileEngine::link(const QString &newName)
{
    Q_D(QFSFileEngine);
    QSystemError error;
    bool ret = QFileSystemEngine::createLink(d->fileEntry, QFileSystemEntry(newName), error);
    if (!ret) {
        setError(QFile::RenameError, error.toString());
    }
    return ret;
}

qint64 QFSFileEnginePrivate::nativeSize() const
{
#ifdef Q_OS_SYMBIAN
    const Q_Q(QFSFileEngine);
    if (symbianFile.SubSessionHandle()) {
#ifdef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
        qint64 size;
#else
        TInt size;
#endif
        TInt err = symbianFile.Size(size);
        if(err != KErrNone) {
            const_cast<QFSFileEngine*>(q)->setError(QFile::PositionError, QSystemError(err, QSystemError::NativeError).toString());
            return 0;
        }
        return size;
    }
#endif
    return sizeFdFh();
}

bool QFSFileEngine::mkdir(const QString &name, bool createParentDirectories) const
{
    return QFileSystemEngine::createDirectory(QFileSystemEntry(name), createParentDirectories);
}

bool QFSFileEngine::rmdir(const QString &name, bool recurseParentDirectories) const
{
    return QFileSystemEngine::removeDirectory(QFileSystemEntry(name), recurseParentDirectories);
}

bool QFSFileEngine::caseSensitive() const
{
#if defined(Q_OS_SYMBIAN)
    return false;
#else
    return true;
#endif
}

bool QFSFileEngine::setCurrentPath(const QString &path)
{
    return QFileSystemEngine::setCurrentPath(QFileSystemEntry(path));
}

QString QFSFileEngine::currentPath(const QString &)
{
    return QFileSystemEngine::currentPath().filePath();
}

QString QFSFileEngine::homePath()
{
    return QFileSystemEngine::homePath();
}

QString QFSFileEngine::rootPath()
{
    return QFileSystemEngine::rootPath();
}

QString QFSFileEngine::tempPath()
{
    return QFileSystemEngine::tempPath();
}

QFileInfoList QFSFileEngine::drives()
{
    QFileInfoList ret;
#if defined(Q_OS_SYMBIAN)
    TDriveList driveList;
    RFs rfs = qt_s60GetRFs();
    TInt err = rfs.DriveList(driveList);
    if (err == KErrNone) {
        char driveName[] = "A:/";

        for (char i = 0; i < KMaxDrives; i++) {
            if (driveList[i]) {
                driveName[0] = 'A' + i;
                ret.append(QFileInfo(QLatin1String(driveName)));
            }
        }
    } else {
        qWarning("QFSFileEngine::drives: Getting drives failed");
    }
#else
    ret.append(QFileInfo(rootPath()));
#endif
    return ret;
}

bool QFSFileEnginePrivate::doStat(QFileSystemMetaData::MetaDataFlags flags) const
{
    if (!tried_stat || !metaData.hasFlags(flags)) {
        tried_stat = 1;

        int localFd = fd;
        if (fh && fileEntry.isEmpty())
            localFd = QT_FILENO(fh);
        if (localFd != -1)
            QFileSystemEngine::fillMetaData(localFd, metaData);

        if (metaData.missingFlags(flags) && !fileEntry.isEmpty())
            QFileSystemEngine::fillMetaData(fileEntry, metaData, metaData.missingFlags(flags));
    }

    return metaData.exists();
}

bool QFSFileEnginePrivate::isSymlink() const
{
    if (!metaData.hasFlags(QFileSystemMetaData::LinkType))
        QFileSystemEngine::fillMetaData(fileEntry, metaData, QFileSystemMetaData::LinkType);

    return metaData.isLink();
}

/*!
    \reimp
*/
QAbstractFileEngine::FileFlags QFSFileEngine::fileFlags(FileFlags type) const
{
    Q_D(const QFSFileEngine);

    if (type & Refresh)
        d->metaData.clear();

    QAbstractFileEngine::FileFlags ret = 0;

    if (type & FlagsMask)
        ret |= LocalDiskFlag;

    bool exists;
    {
        QFileSystemMetaData::MetaDataFlags queryFlags = 0;

        queryFlags |= QFileSystemMetaData::MetaDataFlags(uint(type))
                & QFileSystemMetaData::Permissions;

        if (type & TypesMask)
            queryFlags |= QFileSystemMetaData::AliasType
                    | QFileSystemMetaData::LinkType
                    | QFileSystemMetaData::FileType
                    | QFileSystemMetaData::DirectoryType
                    | QFileSystemMetaData::BundleType;

        if (type & FlagsMask)
            queryFlags |= QFileSystemMetaData::HiddenAttribute
                    | QFileSystemMetaData::ExistsAttribute;

        queryFlags |= QFileSystemMetaData::LinkType;

        exists = d->doStat(queryFlags);
    }

    if (!exists && !d->metaData.isLink())
        return ret;

    if (exists && (type & PermsMask))
        ret |= FileFlags(uint(d->metaData.permissions()));

    if (type & TypesMask) {
        if (d->metaData.isAlias()) {
            ret |= LinkType;
        } else {
            if ((type & LinkType) && d->metaData.isLink())
                ret |= LinkType;
            if (exists) {
                if (d->metaData.isFile()) {
                    ret |= FileType;
                } else if (d->metaData.isDirectory()) {
                    ret |= DirectoryType;
                    if ((type & BundleType) && d->metaData.isBundle())
                        ret |= BundleType;
                }
            }
        }
    }

    if (type & FlagsMask) {
        if (exists)
            ret |= ExistsFlag;
        if (d->fileEntry.isRoot())
            ret |= RootFlag;
        else if (d->metaData.isHidden())
            ret |= HiddenFlag;
    }

    return ret;
}

QString QFSFileEngine::fileName(FileName file) const
{
    Q_D(const QFSFileEngine);
    if (file == BundleName) {
        return QFileSystemEngine::bundleName(d->fileEntry);
    } else if (file == BaseName) {
        return d->fileEntry.fileName();
    } else if (file == PathName) {
        return d->fileEntry.path();
    } else if (file == AbsoluteName || file == AbsolutePathName) {
        QFileSystemEntry entry(QFileSystemEngine::absoluteName(d->fileEntry));
        if (file == AbsolutePathName) {
            return entry.path();
        }
        return entry.filePath();
    } else if (file == CanonicalName || file == CanonicalPathName) {
        QFileSystemEntry entry(QFileSystemEngine::canonicalName(d->fileEntry, d->metaData));
        if (file == CanonicalPathName)
            return entry.path();
        return entry.filePath();
    } else if (file == LinkName) {
        if (d->isSymlink()) {
            QFileSystemEntry entry = QFileSystemEngine::getLinkTarget(d->fileEntry, d->metaData);
            return entry.filePath();
        }
        return QString();
    }
    return d->fileEntry.filePath();
}

bool QFSFileEngine::isRelativePath() const
{
    Q_D(const QFSFileEngine);
#if defined(Q_OS_SYMBIAN)
    return isRelativePathSymbian(d->fileEntry.filePath());
#else
    return d->fileEntry.filePath().length() ? d->fileEntry.filePath()[0] != QLatin1Char('/') : true;
#endif
}

uint QFSFileEngine::ownerId(FileOwner own) const
{
    Q_D(const QFSFileEngine);
    static const uint nobodyID = (uint) -2;

    if (d->doStat(QFileSystemMetaData::OwnerIds))
        return d->metaData.ownerId(own);

    return nobodyID;
}

QString QFSFileEngine::owner(FileOwner own) const
{
#ifndef Q_OS_SYMBIAN
    if (own == OwnerUser)
        return QFileSystemEngine::resolveUserName(ownerId(own));
    return QFileSystemEngine::resolveGroupName(ownerId(own));
#else
    Q_UNUSED(own)
    return QString();
#endif
}

bool QFSFileEngine::setPermissions(uint perms)
{
    Q_D(QFSFileEngine);
    QSystemError error;
    if (!QFileSystemEngine::setPermissions(d->fileEntry, QFile::Permissions(perms), error, 0)) {
        setError(QFile::PermissionsError, error.toString());
        return false;
    }
    return true;
}

#ifdef Q_OS_SYMBIAN
bool QFSFileEngine::setSize(qint64 size)
{
    Q_D(QFSFileEngine);
    bool ret = false;
    TInt err = KErrNone;
    if (d->symbianFile.SubSessionHandle()) {
        TInt err = d->symbianFile.SetSize(size);
        ret = (err == KErrNone);
        if (ret && d->symbianFilePos > size)
            d->symbianFilePos = size;
    }
    else if (d->fd != -1)
        ret = QT_FTRUNCATE(d->fd, size) == 0;
    else if (d->fh)
        ret = QT_FTRUNCATE(QT_FILENO(d->fh), size) == 0;
    else {
        RFile tmp;
        QString symbianFilename(d->fileEntry.nativeFilePath());
        err = tmp.Open(qt_s60GetRFs(), qt_QString2TPtrC(symbianFilename), EFileWrite);
        if (err == KErrNone)
        {
            err = tmp.SetSize(size);
            tmp.Close();
        }
        ret = (err == KErrNone);
    }
    if (!ret) {
        QSystemError error;
        if (err)
            error = QSystemError(err, QSystemError::NativeError);
        else
            error = QSystemError(errno, QSystemError::StandardLibraryError);
        setError(QFile::ResizeError, error.toString());
    }
    return ret;
}
#else
bool QFSFileEngine::setSize(qint64 size)
{
    Q_D(QFSFileEngine);
    bool ret = false;
    if (d->fd != -1)
        ret = QT_FTRUNCATE(d->fd, size) == 0;
    else if (d->fh)
        ret = QT_FTRUNCATE(QT_FILENO(d->fh), size) == 0;
    else
        ret = QT_TRUNCATE(d->fileEntry.nativeFilePath().constData(), size) == 0;
    if (!ret)
        setError(QFile::ResizeError, qt_error_string(errno));
    return ret;
}
#endif

QDateTime QFSFileEngine::fileTime(FileTime time) const
{
    Q_D(const QFSFileEngine);

    if (d->doStat(QFileSystemMetaData::Times))
        return d->metaData.fileTime(time);

    return QDateTime();
}

uchar *QFSFileEnginePrivate::map(qint64 offset, qint64 size, QFile::MemoryMapFlags flags)
{
    Q_Q(QFSFileEngine);
    Q_UNUSED(flags);
    if (openMode == QIODevice::NotOpen) {
        q->setError(QFile::PermissionsError, qt_error_string(int(EACCES)));
        return 0;
    }

    if (offset < 0 || offset != qint64(QT_OFF_T(offset))
            || size < 0 || quint64(size) > quint64(size_t(-1))) {
        q->setError(QFile::UnspecifiedError, qt_error_string(int(EINVAL)));
        return 0;
    }

    // If we know the mapping will extend beyond EOF, fail early to avoid
    // undefined behavior. Otherwise, let mmap have its say.
    if (doStat(QFileSystemMetaData::SizeAttribute)
            && (QT_OFF_T(size) > metaData.size() - QT_OFF_T(offset)))
        qWarning("QFSFileEngine::map: Mapping a file beyond its size is not portable");

    int access = 0;
    if (openMode & QIODevice::ReadOnly) access |= PROT_READ;
    if (openMode & QIODevice::WriteOnly) access |= PROT_WRITE;

#if defined(Q_OS_INTEGRITY)
    int pageSize = sysconf(_SC_PAGESIZE);
#else
    int pageSize = getpagesize();
#endif
    int extra = offset % pageSize;

    if (quint64(size + extra) > quint64((size_t)-1)) {
        q->setError(QFile::UnspecifiedError, qt_error_string(int(EINVAL)));
        return 0;
    }

    size_t realSize = (size_t)size + extra;
    QT_OFF_T realOffset = QT_OFF_T(offset);
    realOffset &= ~(QT_OFF_T(pageSize - 1));

#ifdef QT_SYMBIAN_USE_NATIVE_FILEMAP
    TInt nativeMapError = KErrNone;
    RFileMap mapping;
    TUint mode(EFileMapRemovableMedia);
    TUint64 nativeOffset = offset & ~(mapping.PageSizeInBytes() - 1);

    //If the file was opened for write or read/write, then open the map for read/write
    if (openMode & QIODevice::WriteOnly)
        mode |= EFileMapWrite;
    if (symbianFile.SubSessionHandle()) {
        nativeMapError = mapping.Open(symbianFile, nativeOffset, size, mode);
    } else {
        //map file by name if we don't have a native handle
        QString fn = QFileSystemEngine::absoluteName(fileEntry).nativeFilePath();
        TUint filemode = EFileShareReadersOrWriters | EFileRead;
        if (openMode & QIODevice::WriteOnly)
            filemode |= EFileWrite;
        nativeMapError = mapping.Open(qt_s60GetRFs(), qt_QString2TPtrC(fn), filemode, nativeOffset, size, mode);
    }
    if (nativeMapError == KErrNone) {
        QScopedResource<RFileMap> ptr(mapping); //will call Close if adding to mapping throws an exception
        uchar *address = mapping.Base() + (offset - nativeOffset);
        maps[address] = mapping;
        ptr.take();
        return address;
    }
    QFile::FileError reportedError = QFile::UnspecifiedError;
    switch (nativeMapError) {
    case KErrAccessDenied:
    case KErrPermissionDenied:
        reportedError = QFile::PermissionsError;
        break;
    case KErrNoMemory:
        reportedError = QFile::ResourceError;
        break;
    }
    q->setError(reportedError, QSystemError(nativeMapError, QSystemError::NativeError).toString());
    return 0;
#else
#ifdef Q_OS_SYMBIAN
    //older phones & emulator don't support native mapping, so need to keep the open C way around for those.
    void *mapAddress;
    TRAPD(err, mapAddress = QT_MMAP((void*)0, realSize,
                   access, MAP_SHARED, getMapHandle(), realOffset));
    if (err != KErrNone) {
        qWarning("OpenC bug: leave from mmap %d", err);
        mapAddress = MAP_FAILED;
        errno = EINVAL;
    }
#else
    void *mapAddress = QT_MMAP((void*)0, realSize,
                   access, MAP_SHARED, nativeHandle(), realOffset);
#endif
    if (MAP_FAILED != mapAddress) {
        uchar *address = extra + static_cast<uchar*>(mapAddress);
        maps[address] = QPair<int,size_t>(extra, realSize);
        return address;
    }

    switch(errno) {
    case EBADF:
        q->setError(QFile::PermissionsError, qt_error_string(int(EACCES)));
        break;
    case ENFILE:
    case ENOMEM:
        q->setError(QFile::ResourceError, qt_error_string(int(errno)));
        break;
    case EINVAL:
        // size are out of bounds
    default:
        q->setError(QFile::UnspecifiedError, qt_error_string(int(errno)));
        break;
    }
    return 0;
#endif
}

bool QFSFileEnginePrivate::unmap(uchar *ptr)
{
#if !defined(Q_OS_INTEGRITY)
    Q_Q(QFSFileEngine);
    if (!maps.contains(ptr)) {
        q->setError(QFile::PermissionsError, qt_error_string(EACCES));
        return false;
    }

#ifdef QT_SYMBIAN_USE_NATIVE_FILEMAP
    RFileMap mapping = maps.value(ptr);
    TInt err = mapping.Flush();
    mapping.Close();
    maps.remove(ptr);
    if (err) {
        q->setError(QFile::WriteError, QSystemError(err, QSystemError::NativeError).toString());
        return false;
    }
    return true;
#else
    uchar *start = ptr - maps[ptr].first;
    size_t len = maps[ptr].second;
    if (-1 == munmap(start, len)) {
        q->setError(QFile::UnspecifiedError, qt_error_string(errno));
        return false;
    }
    maps.remove(ptr);
    return true;
#endif
#else
    return false;
#endif
}

QT_END_NAMESPACE

#endif // QT_NO_FSFILEENGINE
