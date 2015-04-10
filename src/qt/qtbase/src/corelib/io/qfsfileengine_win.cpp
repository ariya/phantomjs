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

#include "qplatformdefs.h"
#include "private/qabstractfileengine_p.h"
#include "private/qfsfileengine_p.h"
#include "qfilesystemengine_p.h"
#include <qdebug.h>

#include "qfile.h"
#include "qdir.h"
#include "private/qmutexpool_p.h"
#include "qvarlengtharray.h"
#include "qdatetime.h"
#include "qt_windows.h"

#if !defined(Q_OS_WINCE)
#  include <sys/types.h>
#  include <direct.h>
#  include <winioctl.h>
#else
#  include <types.h>
#endif
#include <objbase.h>
#ifndef Q_OS_WINRT
#  include <shlobj.h>
#  include <accctrl.h>
#endif
#include <initguid.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#ifndef Q_OS_WINRT
#  define SECURITY_WIN32
#  include <security.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX FILENAME_MAX
#endif

QT_BEGIN_NAMESPACE

#if !defined(Q_OS_WINCE)
static inline bool isUncPath(const QString &path)
{
    // Starts with \\, but not \\.
    return (path.startsWith(QLatin1String("\\\\"))
            && path.size() > 2 && path.at(2) != QLatin1Char('.'));
}
#endif

/*!
    \internal
*/
QString QFSFileEnginePrivate::longFileName(const QString &path)
{
    if (path.startsWith(QLatin1String("\\\\.\\")))
        return path;

    QString absPath = QFileSystemEngine::nativeAbsoluteFilePath(path);
#if !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
    QString prefix = QLatin1String("\\\\?\\");
    if (isUncPath(absPath)) {
        prefix.append(QLatin1String("UNC\\")); // "\\\\?\\UNC\\"
        absPath.remove(0, 2);
    }
    return prefix + absPath;
#else
    return absPath;
#endif
}

/*
    \internal
*/
bool QFSFileEnginePrivate::nativeOpen(QIODevice::OpenMode openMode)
{
    Q_Q(QFSFileEngine);

    // All files are opened in share mode (both read and write).
    DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;

    int accessRights = 0;
    if (openMode & QIODevice::ReadOnly)
        accessRights |= GENERIC_READ;
    if (openMode & QIODevice::WriteOnly)
        accessRights |= GENERIC_WRITE;


    // WriteOnly can create files, ReadOnly cannot.
    DWORD creationDisp = (openMode & QIODevice::WriteOnly) ? OPEN_ALWAYS : OPEN_EXISTING;
    // Create the file handle.
#ifndef Q_OS_WINRT
    SECURITY_ATTRIBUTES securityAtts = { sizeof(SECURITY_ATTRIBUTES), NULL, FALSE };
    fileHandle = CreateFile((const wchar_t*)fileEntry.nativeFilePath().utf16(),
                            accessRights,
                            shareMode,
                            &securityAtts,
                            creationDisp,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);
#else // !Q_OS_WINRT
    fileHandle = CreateFile2((const wchar_t*)fileEntry.nativeFilePath().utf16(),
                             accessRights,
                             shareMode,
                             creationDisp,
                             NULL);
#endif // Q_OS_WINRT

    // Bail out on error.
    if (fileHandle == INVALID_HANDLE_VALUE) {
        q->setError(QFile::OpenError, qt_error_string());
        return false;
    }

    // Truncate the file after successfully opening it if Truncate is passed.
    if (openMode & QIODevice::Truncate)
        q->setSize(0);

    return true;
}

/*
    \internal
*/
bool QFSFileEnginePrivate::nativeClose()
{
    Q_Q(QFSFileEngine);
    if (fh || fd != -1) {
        // stdlib / stdio mode.
        return closeFdFh();
    }

    // Windows native mode.
    bool ok = true;

#ifndef Q_OS_WINCE
    if (cachedFd != -1) {
        if (::_close(cachedFd) && !::CloseHandle(fileHandle)) {
            q->setError(QFile::UnspecifiedError, qt_error_string());
            ok = false;
        }

        // System handle is closed with associated file descriptor.
        fileHandle = INVALID_HANDLE_VALUE;
        cachedFd = -1;

        return ok;
    }
#endif

    if ((fileHandle == INVALID_HANDLE_VALUE || !::CloseHandle(fileHandle))) {
        q->setError(QFile::UnspecifiedError, qt_error_string());
        ok = false;
    }
    fileHandle = INVALID_HANDLE_VALUE;
    return ok;
}

/*
    \internal
*/
bool QFSFileEnginePrivate::nativeFlush()
{
    if (fh) {
        // Buffered stdlib mode.
        return flushFh();
    }
    if (fd != -1) {
        // Unbuffered stdio mode; always succeeds (no buffer).
        return true;
    }

    // Windows native mode; flushing is unnecessary.
    return true;
}

/*
    \internal
    \since 5.1
*/
bool QFSFileEnginePrivate::nativeSyncToDisk()
{
    if (fh || fd != -1) {
        // stdlib / stdio mode. No API available.
        return false;
    }
    return FlushFileBuffers(fileHandle);
}

/*
    \internal
*/
qint64 QFSFileEnginePrivate::nativeSize() const
{
    Q_Q(const QFSFileEngine);
    QFSFileEngine *thatQ = const_cast<QFSFileEngine *>(q);

    // ### Don't flush; for buffered files, we should get away with ftell.
    thatQ->flush();

    // Always retrive the current information
    metaData.clearFlags(QFileSystemMetaData::SizeAttribute);
#if defined(Q_OS_WINCE)
    // Buffered stdlib mode.
    if (fh) {
        QT_OFF_T oldPos = QT_FTELL(fh);
        QT_FSEEK(fh, 0, SEEK_END);
        qint64 fileSize = (qint64)QT_FTELL(fh);
        QT_FSEEK(fh, oldPos, SEEK_SET);
        if (fileSize == -1) {
            fileSize = 0;
            thatQ->setError(QFile::UnspecifiedError, qt_error_string(errno));
        }
        return fileSize;
    }
    if (fd != -1) {
        thatQ->setError(QFile::UnspecifiedError, QLatin1String("Not implemented!"));
        return 0;
    }
#endif
    bool filled = false;
    if (fileHandle != INVALID_HANDLE_VALUE && openMode != QIODevice::NotOpen )
        filled = QFileSystemEngine::fillMetaData(fileHandle, metaData,
                                                 QFileSystemMetaData::SizeAttribute);
    else
        filled = doStat(QFileSystemMetaData::SizeAttribute);

    if (!filled) {
        thatQ->setError(QFile::UnspecifiedError, qt_error_string(errno));
        return 0;
    }
    return metaData.size();
}

/*
    \internal
*/
qint64 QFSFileEnginePrivate::nativePos() const
{
    Q_Q(const QFSFileEngine);
    QFSFileEngine *thatQ = const_cast<QFSFileEngine *>(q);

    if (fh || fd != -1) {
        // stdlib / stido mode.
        return posFdFh();
    }

    // Windows native mode.
    if (fileHandle == INVALID_HANDLE_VALUE)
        return 0;

#if !defined(Q_OS_WINCE)
    LARGE_INTEGER currentFilePos;
    LARGE_INTEGER offset;
    offset.QuadPart = 0;
    if (!::SetFilePointerEx(fileHandle, offset, &currentFilePos, FILE_CURRENT)) {
        thatQ->setError(QFile::UnspecifiedError, qt_error_string());
        return 0;
    }

    return qint64(currentFilePos.QuadPart);
#else
    LARGE_INTEGER filepos;
    filepos.HighPart = 0;
    DWORD newFilePointer = SetFilePointer(fileHandle, 0, &filepos.HighPart, FILE_CURRENT);
    if (newFilePointer == 0xFFFFFFFF && GetLastError() != NO_ERROR) {
        thatQ->setError(QFile::UnspecifiedError, qt_error_string());
        return 0;
    }

    filepos.LowPart = newFilePointer;
    return filepos.QuadPart;
#endif
}

/*
    \internal
*/
bool QFSFileEnginePrivate::nativeSeek(qint64 pos)
{
    Q_Q(QFSFileEngine);

    if (fh || fd != -1) {
        // stdlib / stdio mode.
        return seekFdFh(pos);
    }

#if !defined(Q_OS_WINCE)
    LARGE_INTEGER currentFilePos;
    LARGE_INTEGER offset;
    offset.QuadPart = pos;
    if (!::SetFilePointerEx(fileHandle, offset, &currentFilePos, FILE_BEGIN)) {
        q->setError(QFile::UnspecifiedError, qt_error_string());
        return false;
    }

    return true;
#else
    DWORD newFilePointer;
    LARGE_INTEGER *li = reinterpret_cast<LARGE_INTEGER*>(&pos);
    newFilePointer = SetFilePointer(fileHandle, li->LowPart, &li->HighPart, FILE_BEGIN);
    if (newFilePointer == 0xFFFFFFFF && GetLastError() != NO_ERROR) {
        q->setError(QFile::PositionError, qt_error_string());
        return false;
    }

    return true;
#endif
}

/*
    \internal
*/
qint64 QFSFileEnginePrivate::nativeRead(char *data, qint64 maxlen)
{
    Q_Q(QFSFileEngine);

    if (fh || fd != -1) {
        // stdio / stdlib mode.
        if (fh && nativeIsSequential() && feof(fh)) {
            q->setError(QFile::ReadError, qt_error_string(int(errno)));
            return -1;
        }

        return readFdFh(data, maxlen);
    }

    // Windows native mode.
    if (fileHandle == INVALID_HANDLE_VALUE)
        return -1;

    DWORD bytesToRead = DWORD(maxlen); // <- lossy

    // Reading on Windows fails with ERROR_NO_SYSTEM_RESOURCES when
    // the chunks are too large, so we limit the block size to 32MB.
    static const DWORD maxBlockSize = 32 * 1024 * 1024;

    qint64 totalRead = 0;
    do {
        DWORD blockSize = qMin<DWORD>(bytesToRead, maxBlockSize);
        DWORD bytesRead;
        if (!ReadFile(fileHandle, data + totalRead, blockSize, &bytesRead, NULL)) {
            if (totalRead == 0) {
                // Note: only return failure if the first ReadFile fails.
                q->setError(QFile::ReadError, qt_error_string());
                return -1;
            }
            break;
        }
        if (bytesRead == 0)
            break;
        totalRead += bytesRead;
        bytesToRead -= bytesRead;
    } while (totalRead < maxlen);
    return qint64(totalRead);
}

/*
    \internal
*/
qint64 QFSFileEnginePrivate::nativeReadLine(char *data, qint64 maxlen)
{
    Q_Q(QFSFileEngine);

    if (fh || fd != -1) {
        // stdio / stdlib mode.
        return readLineFdFh(data, maxlen);
    }

    // Windows native mode.
    if (fileHandle == INVALID_HANDLE_VALUE)
        return -1;

    // ### No equivalent in Win32?
    return q->QAbstractFileEngine::readLine(data, maxlen);
}

/*
    \internal
*/
qint64 QFSFileEnginePrivate::nativeWrite(const char *data, qint64 len)
{
    Q_Q(QFSFileEngine);

    if (fh || fd != -1) {
        // stdio / stdlib mode.
        return writeFdFh(data, len);
    }

    // Windows native mode.
    if (fileHandle == INVALID_HANDLE_VALUE)
        return -1;

    qint64 bytesToWrite = DWORD(len); // <- lossy

    // Writing on Windows fails with ERROR_NO_SYSTEM_RESOURCES when
    // the chunks are too large, so we limit the block size to 32MB.
    static const DWORD maxBlockSize = 32 * 1024 * 1024;

    qint64 totalWritten = 0;
    do {
        DWORD blockSize = qMin<DWORD>(bytesToWrite, maxBlockSize);
        DWORD bytesWritten;
        if (!WriteFile(fileHandle, data + totalWritten, blockSize, &bytesWritten, NULL)) {
            if (totalWritten == 0) {
                // Note: Only return error if the first WriteFile failed.
                q->setError(QFile::WriteError, qt_error_string());
                return -1;
            }
            break;
        }
        if (bytesWritten == 0)
            break;
        totalWritten += bytesWritten;
        bytesToWrite -= bytesWritten;
    } while (totalWritten < len);
    return qint64(totalWritten);
}

/*
    \internal
*/
int QFSFileEnginePrivate::nativeHandle() const
{
    if (fh || fd != -1)
        return fh ? QT_FILENO(fh) : fd;
#ifndef Q_OS_WINCE
    if (cachedFd != -1)
        return cachedFd;

    int flags = 0;
    if (openMode & QIODevice::Append)
        flags |= _O_APPEND;
    if (!(openMode & QIODevice::WriteOnly))
        flags |= _O_RDONLY;
    cachedFd = _open_osfhandle((intptr_t) fileHandle, flags);
    return cachedFd;
#else
    return -1;
#endif
}

/*
    \internal
*/
bool QFSFileEnginePrivate::nativeIsSequential() const
{
#if !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
    HANDLE handle = fileHandle;
    if (fh || fd != -1)
        handle = (HANDLE)_get_osfhandle(fh ? QT_FILENO(fh) : fd);
    if (handle == INVALID_HANDLE_VALUE)
        return false;

    DWORD fileType = GetFileType(handle);
    return (fileType == FILE_TYPE_CHAR)
            || (fileType == FILE_TYPE_PIPE);
#else
    return false;
#endif
}

bool QFSFileEngine::remove()
{
    Q_D(QFSFileEngine);
    QSystemError error;
    bool ret = QFileSystemEngine::removeFile(d->fileEntry, error);
    if (!ret)
        setError(QFile::RemoveError, error.toString());
    return ret;
}

bool QFSFileEngine::copy(const QString &copyName)
{
    Q_D(QFSFileEngine);
    QSystemError error;
    bool ret = QFileSystemEngine::copyFile(d->fileEntry, QFileSystemEntry(copyName), error);
    if (!ret)
        setError(QFile::CopyError, error.toString());
    return ret;
}

bool QFSFileEngine::rename(const QString &newName)
{
    Q_D(QFSFileEngine);
    QSystemError error;
    bool ret = QFileSystemEngine::renameFile(d->fileEntry, QFileSystemEntry(newName), error);
    if (!ret)
        setError(QFile::RenameError, error.toString());
    return ret;
}

bool QFSFileEngine::renameOverwrite(const QString &newName)
{
    Q_D(QFSFileEngine);
#if defined(Q_OS_WINCE)
    // Windows Embedded Compact 7 does not have MoveFileEx, simulate it with  the following sequence:
    //   1. DeleteAndRenameFile  (Should work on RAM FS when both files exist)
    //   2. DeleteFile/MoveFile  (Should work on all file systems)
    //
    // DeleteFile/MoveFile fallback implementation violates atomicity, but it is more acceptable than
    // alternative CopyFile/DeleteFile sequence for the following reasons:
    //
    //  1. DeleteFile/MoveFile is way faster than CopyFile/DeleteFile and thus more atomic.
    //  2. Given the intended use case of this function in QSaveFile, DeleteFile/MoveFile sequence will
    //     delete the old content, but leave a file "filename.ext.XXXXXX" in the same directory if MoveFile fails.
    //     With CopyFile/DeleteFile sequence, it can happen that new data is partially copied to target file
    //     (because CopyFile is not atomic either), thus leaving *some* content to target file.
    //     This makes the need for application level recovery harder to detect than in DeleteFile/MoveFile
    //     sequence where target file simply does not exist.
    //
    bool ret = ::DeleteAndRenameFile((wchar_t*)QFileSystemEntry(newName).nativeFilePath().utf16(),
                                     (wchar_t*)d->fileEntry.nativeFilePath().utf16()) != 0;
    if (!ret) {
        ret = ::DeleteFile((wchar_t*)d->fileEntry.nativeFilePath().utf16()) != 0;
        if (ret)
            ret = ::MoveFile((wchar_t*)d->fileEntry.nativeFilePath().utf16(),
                             (wchar_t*)QFileSystemEntry(newName).nativeFilePath().utf16()) != 0;
    }
#else
    bool ret = ::MoveFileEx((wchar_t*)d->fileEntry.nativeFilePath().utf16(),
                            (wchar_t*)QFileSystemEntry(newName).nativeFilePath().utf16(),
                            MOVEFILE_REPLACE_EXISTING) != 0;
#endif
    if (!ret)
        setError(QFile::RenameError, QSystemError(::GetLastError(), QSystemError::NativeError).toString());
    return ret;
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
    return false;
}

bool QFSFileEngine::setCurrentPath(const QString &path)
{
    return QFileSystemEngine::setCurrentPath(QFileSystemEntry(path));
}

QString QFSFileEngine::currentPath(const QString &fileName)
{
#if !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
    QString ret;
    //if filename is a drive: then get the pwd of that drive
    if (fileName.length() >= 2 &&
        fileName.at(0).isLetter() && fileName.at(1) == QLatin1Char(':')) {
        int drv = fileName.toUpper().at(0).toLatin1() - 'A' + 1;
        if (_getdrive() != drv) {
            wchar_t buf[PATH_MAX];
            ::_wgetdcwd(drv, buf, PATH_MAX);
            ret = QString::fromWCharArray(buf);
        }
    }
    if (ret.isEmpty()) {
        //just the pwd
        ret = QFileSystemEngine::currentPath().filePath();
    }
    if (ret.length() >= 2 && ret[1] == QLatin1Char(':'))
        ret[0] = ret.at(0).toUpper(); // Force uppercase drive letters.
    return ret;
#else // !Q_OS_WINCE && !Q_OS_WINRT
    Q_UNUSED(fileName);
    return QFileSystemEngine::currentPath().filePath();
#endif // Q_OS_WINCE || Q_OS_WINRT
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
#if !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
#if defined(Q_OS_WIN32)
    quint32 driveBits = (quint32) GetLogicalDrives() & 0x3ffffff;
#endif
    char driveName[] = "A:/";

    while (driveBits) {
        if (driveBits & 1)
            ret.append(QFileInfo(QLatin1String(driveName)));
        driveName[0]++;
        driveBits = driveBits >> 1;
    }
    return ret;
#else // !Q_OS_WINCE && !Q_OS_WINRT
    ret.append(QFileInfo(QLatin1String("/")));
    return ret;
#endif // Q_OS_WINCE || Q_OS_WINRT
}

bool QFSFileEnginePrivate::doStat(QFileSystemMetaData::MetaDataFlags flags) const
{
    if (!tried_stat || !metaData.hasFlags(flags)) {
        tried_stat = true;

#if !defined(Q_OS_WINCE)
        int localFd = fd;
        if (fh && fileEntry.isEmpty())
            localFd = QT_FILENO(fh);
        if (localFd != -1)
            QFileSystemEngine::fillMetaData(localFd, metaData, flags);
#endif
        if (metaData.missingFlags(flags) && !fileEntry.isEmpty())
            QFileSystemEngine::fillMetaData(fileEntry, metaData, metaData.missingFlags(flags));
    }

    return metaData.exists();
}


bool QFSFileEngine::link(const QString &newName)
{
#if !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
#if !defined(QT_NO_LIBRARY)
    bool ret = false;

    QString linkName = newName;
    //### assume that they add .lnk

    IShellLink *psl;
    bool neededCoInit = false;

    HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void **)&psl);

    if (hres == CO_E_NOTINITIALIZED) { // COM was not initialized
        neededCoInit = true;
        CoInitialize(NULL);
        hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void **)&psl);
    }

    if (SUCCEEDED(hres)) {
        hres = psl->SetPath((wchar_t *)fileName(AbsoluteName).replace(QLatin1Char('/'), QLatin1Char('\\')).utf16());
        if (SUCCEEDED(hres)) {
            hres = psl->SetWorkingDirectory((wchar_t *)fileName(AbsolutePathName).replace(QLatin1Char('/'), QLatin1Char('\\')).utf16());
            if (SUCCEEDED(hres)) {
                IPersistFile *ppf;
                hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
                if (SUCCEEDED(hres)) {
                    hres = ppf->Save((wchar_t*)linkName.utf16(), TRUE);
                    if (SUCCEEDED(hres))
                         ret = true;
                    ppf->Release();
                }
            }
        }
        psl->Release();
    }
    if (!ret)
        setError(QFile::RenameError, qt_error_string());

    if (neededCoInit)
        CoUninitialize();

    return ret;
#else
    Q_UNUSED(newName);
    return false;
#endif // QT_NO_LIBRARY
#elif defined(Q_OS_WINCE)
    QString linkName = newName;
    linkName.replace(QLatin1Char('/'), QLatin1Char('\\'));
    if (!linkName.endsWith(QLatin1String(".lnk")))
        linkName += QLatin1String(".lnk");
    QString orgName = fileName(AbsoluteName).replace(QLatin1Char('/'), QLatin1Char('\\'));
    // Need to append on our own
    orgName.prepend(QLatin1Char('"'));
    orgName.append(QLatin1Char('"'));
    bool ret = SUCCEEDED(SHCreateShortcut((wchar_t*)linkName.utf16(), (wchar_t*)orgName.utf16()));
    if (!ret)
        setError(QFile::RenameError, qt_error_string());
    return ret;
#else // Q_OS_WINCE
    Q_UNUSED(newName);
    Q_UNIMPLEMENTED();
    return false;
#endif // Q_OS_WINRT
}

/*!
    \reimp
*/
QAbstractFileEngine::FileFlags QFSFileEngine::fileFlags(QAbstractFileEngine::FileFlags type) const
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

        // AliasType and BundleType are 0x0
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

    if (exists && (type & PermsMask))
        ret |= FileFlags(uint(d->metaData.permissions()));

    if (type & TypesMask) {
        if ((type & LinkType) && d->metaData.isLegacyLink())
            ret |= LinkType;
        if (d->metaData.isDirectory()) {
            ret |= DirectoryType;
        } else {
            ret |= FileType;
        }
    }
    if (type & FlagsMask) {
        if (d->metaData.exists()) {
            ret |= ExistsFlag;
            if (d->fileEntry.isRoot())
                ret |= RootFlag;
            else if (d->metaData.isHidden())
                ret |= HiddenFlag;
        }
    }
    return ret;
}

QString QFSFileEngine::fileName(FileName file) const
{
    Q_D(const QFSFileEngine);
    if (file == BaseName) {
        return d->fileEntry.fileName();
    } else if (file == PathName) {
        return d->fileEntry.path();
    } else if (file == AbsoluteName || file == AbsolutePathName) {
        QString ret;

        if (!isRelativePath()) {
#if !defined(Q_OS_WINCE)
            if (d->fileEntry.filePath().startsWith(QLatin1Char('/')) || // It's a absolute path to the current drive, so \a.txt -> Z:\a.txt
                d->fileEntry.filePath().size() == 2 ||                  // It's a drive letter that needs to get a working dir appended
                (d->fileEntry.filePath().size() > 2 && d->fileEntry.filePath().at(2) != QLatin1Char('/')) || // It's a drive-relative path, so Z:a.txt -> Z:\currentpath\a.txt
                d->fileEntry.filePath().contains(QLatin1String("/../")) || d->fileEntry.filePath().contains(QLatin1String("/./")) ||
                d->fileEntry.filePath().endsWith(QLatin1String("/..")) || d->fileEntry.filePath().endsWith(QLatin1String("/.")))
            {
                ret = QDir::fromNativeSeparators(QFileSystemEngine::nativeAbsoluteFilePath(d->fileEntry.filePath()));
            } else
#endif
            {
                ret = d->fileEntry.filePath();
            }
        } else {
            ret = QDir::cleanPath(QDir::currentPath() + QLatin1Char('/') + d->fileEntry.filePath());
        }

        // The path should be absolute at this point.
        // From the docs :
        // Absolute paths begin with the directory separator "/"
        // (optionally preceded by a drive specification under Windows).
        if (ret.at(0) != QLatin1Char('/')) {
            Q_ASSERT(ret.length() >= 2);
            Q_ASSERT(ret.at(0).isLetter());
            Q_ASSERT(ret.at(1) == QLatin1Char(':'));

            // Force uppercase drive letters.
            ret[0] = ret.at(0).toUpper();
        }

        if (file == AbsolutePathName) {
            int slash = ret.lastIndexOf(QLatin1Char('/'));
            if (slash < 0)
                return ret;
            else if (ret.at(0) != QLatin1Char('/') && slash == 2)
                return ret.left(3);      // include the slash
            else
                return ret.left(slash > 0 ? slash : 1);
        }
        return ret;
    } else if (file == CanonicalName || file == CanonicalPathName) {
        if (!(fileFlags(ExistsFlag) & ExistsFlag))
            return QString();
        QFileSystemEntry entry(QFileSystemEngine::canonicalName(QFileSystemEntry(fileName(AbsoluteName)), d->metaData));

        if (file == CanonicalPathName)
            return entry.path();
        return entry.filePath();
    } else if (file == LinkName) {
        return QFileSystemEngine::getLinkTarget(d->fileEntry, d->metaData).filePath();
    } else if (file == BundleName) {
        return QString();
    }
    return d->fileEntry.filePath();
}

bool QFSFileEngine::isRelativePath() const
{
    Q_D(const QFSFileEngine);
    // drive, e.g. "a:", or UNC root, e.q. "//"
    return d->fileEntry.isRelative();
}

uint QFSFileEngine::ownerId(FileOwner /*own*/) const
{
    static const uint nobodyID = (uint) -2;
    return nobodyID;
}

QString QFSFileEngine::owner(FileOwner own) const
{
    Q_D(const QFSFileEngine);
    return QFileSystemEngine::owner(d->fileEntry, own);
}

bool QFSFileEngine::setPermissions(uint perms)
{
    Q_D(QFSFileEngine);
    QSystemError error;
    bool ret = QFileSystemEngine::setPermissions(d->fileEntry, QFile::Permissions(perms), error);
    if (!ret)
        setError(QFile::PermissionsError, error.toString());
    return ret;
}

bool QFSFileEngine::setSize(qint64 size)
{
    Q_D(QFSFileEngine);

    if (d->fileHandle != INVALID_HANDLE_VALUE || d->fd != -1 || d->fh) {
        // resize open file
        HANDLE fh = d->fileHandle;
#if !defined(Q_OS_WINCE)
        if (fh == INVALID_HANDLE_VALUE) {
            if (d->fh)
                fh = (HANDLE)_get_osfhandle(QT_FILENO(d->fh));
            else
                fh = (HANDLE)_get_osfhandle(d->fd);
        }
#endif
        if (fh == INVALID_HANDLE_VALUE)
            return false;
        qint64 currentPos = pos();

        if (seek(size) && SetEndOfFile(fh)) {
            seek(qMin(currentPos, size));
            return true;
        }

        seek(currentPos);
        return false;
    }

    if (!d->fileEntry.isEmpty()) {
        // resize file on disk
        QFile file(d->fileEntry.filePath());
        if (file.open(QFile::ReadWrite)) {
            bool ret = file.resize(size);
            if (!ret)
                setError(QFile::ResizeError, file.errorString());
            return ret;
        }
    }
    return false;
}


QDateTime QFSFileEngine::fileTime(FileTime time) const
{
    Q_D(const QFSFileEngine);

    if (d->doStat(QFileSystemMetaData::Times))
        return d->metaData.fileTime(time);

    return QDateTime();
}

uchar *QFSFileEnginePrivate::map(qint64 offset, qint64 size,
                                 QFile::MemoryMapFlags flags)
{
#ifndef Q_OS_WINPHONE
    Q_Q(QFSFileEngine);
    Q_UNUSED(flags);
    if (openMode == QFile::NotOpen) {
        q->setError(QFile::PermissionsError, qt_error_string(ERROR_ACCESS_DENIED));
        return 0;
    }
    if (offset == 0 && size == 0) {
        q->setError(QFile::UnspecifiedError, qt_error_string(ERROR_INVALID_PARAMETER));
        return 0;
    }

    if (mapHandle == NULL) {
        // get handle to the file
        HANDLE handle = fileHandle;

#ifndef Q_OS_WINCE
        if (handle == INVALID_HANDLE_VALUE && fh)
            handle = (HANDLE)::_get_osfhandle(QT_FILENO(fh));
#endif

#ifdef Q_USE_DEPRECATED_MAP_API
        nativeClose();
        // handle automatically closed by kernel with mapHandle (below).
        handle = ::CreateFileForMapping((const wchar_t*)fileEntry.nativeFilePath().utf16(),
                GENERIC_READ | (openMode & QIODevice::WriteOnly ? GENERIC_WRITE : 0),
                0,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
        // Since this is a special case, we check if the return value was NULL and if so
        // we change it to INVALID_HANDLE_VALUE to follow the logic inside this function.
        if(0 == handle)
            handle = INVALID_HANDLE_VALUE;
#endif

        if (handle == INVALID_HANDLE_VALUE) {
            q->setError(QFile::PermissionsError, qt_error_string(ERROR_ACCESS_DENIED));
            return 0;
        }

        // first create the file mapping handle
        DWORD protection = (openMode & QIODevice::WriteOnly) ? PAGE_READWRITE : PAGE_READONLY;
#ifndef Q_OS_WINRT
        mapHandle = ::CreateFileMapping(handle, 0, protection, 0, 0, 0);
#else
        mapHandle = ::CreateFileMappingFromApp(handle, 0, protection, 0, 0);
#endif
        if (mapHandle == NULL) {
            q->setError(QFile::PermissionsError, qt_error_string());
#ifdef Q_USE_DEPRECATED_MAP_API
            ::CloseHandle(handle);
#endif
            return 0;
        }
    }

    // setup args to map
    DWORD access = 0;
    if (openMode & QIODevice::ReadOnly) access = FILE_MAP_READ;
    if (openMode & QIODevice::WriteOnly) access = FILE_MAP_WRITE;

    DWORD offsetHi = offset >> 32;
    DWORD offsetLo = offset & Q_UINT64_C(0xffffffff);
    SYSTEM_INFO sysinfo;
#ifndef Q_OS_WINRT
    ::GetSystemInfo(&sysinfo);
#else
    ::GetNativeSystemInfo(&sysinfo);
#endif
    DWORD mask = sysinfo.dwAllocationGranularity - 1;
    DWORD extra = offset & mask;
    if (extra)
        offsetLo &= ~mask;

    // attempt to create the map
#ifndef Q_OS_WINRT
    LPVOID mapAddress = ::MapViewOfFile(mapHandle, access,
                                      offsetHi, offsetLo, size + extra);
#else
    LPVOID mapAddress = ::MapViewOfFileFromApp(mapHandle, access,
                                               (ULONG64(offsetHi) << 32) + offsetLo, size);
#endif
    if (mapAddress) {
        uchar *address = extra + static_cast<uchar*>(mapAddress);
        maps[address] = extra;
        return address;
    }

    switch(GetLastError()) {
    case ERROR_ACCESS_DENIED:
        q->setError(QFile::PermissionsError, qt_error_string());
        break;
    case ERROR_INVALID_PARAMETER:
        // size are out of bounds
    default:
        q->setError(QFile::UnspecifiedError, qt_error_string());
    }

    ::CloseHandle(mapHandle);
    mapHandle = NULL;
#else // !Q_OS_WINPHONE
    Q_UNUSED(offset);
    Q_UNUSED(size);
    Q_UNUSED(flags);
    Q_UNIMPLEMENTED();
#endif // Q_OS_WINPHONE
    return 0;
}

bool QFSFileEnginePrivate::unmap(uchar *ptr)
{
#ifndef Q_OS_WINPHONE
    Q_Q(QFSFileEngine);
    if (!maps.contains(ptr)) {
        q->setError(QFile::PermissionsError, qt_error_string(ERROR_ACCESS_DENIED));
        return false;
    }
    uchar *start = ptr - maps[ptr];
    if (!UnmapViewOfFile(start)) {
        q->setError(QFile::PermissionsError, qt_error_string());
        return false;
    }

    maps.remove(ptr);
    if (maps.isEmpty()) {
        ::CloseHandle(mapHandle);
        mapHandle = NULL;
    }

    return true;
#else // !Q_OS_WINPHONE
    Q_UNUSED(ptr);
    Q_UNIMPLEMENTED();
    return false;
#endif // Q_OS_WINPHONE
}

QT_END_NAMESPACE
