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
#include "qdebug.h"
#include "qfile.h"
#include "qfsfileengine_p.h"
#include "qtemporaryfile.h"
#include "qlist.h"
#include "qfileinfo.h"
#include "private/qiodevice_p.h"
#include "private/qfile_p.h"
#include "private/qfilesystemengine_p.h"
#include "private/qsystemerror_p.h"
#if defined(QT_BUILD_CORE_LIB)
# include "qcoreapplication.h"
#endif

#ifdef QT_NO_QOBJECT
#define tr(X) QString::fromLatin1(X)
#endif

QT_BEGIN_NAMESPACE

//************* QFilePrivate
QFilePrivate::QFilePrivate()
{
}

QFilePrivate::~QFilePrivate()
{
}

bool
QFilePrivate::openExternalFile(int flags, int fd, QFile::FileHandleFlags handleFlags)
{
#ifdef QT_NO_FSFILEENGINE
    Q_UNUSED(flags);
    Q_UNUSED(fd);
    return false;
#else
    delete fileEngine;
    fileEngine = 0;
    QFSFileEngine *fe = new QFSFileEngine;
    fileEngine = fe;
    return fe->open(QIODevice::OpenMode(flags), fd, handleFlags);
#endif
}

bool
QFilePrivate::openExternalFile(int flags, FILE *fh, QFile::FileHandleFlags handleFlags)
{
#ifdef QT_NO_FSFILEENGINE
    Q_UNUSED(flags);
    Q_UNUSED(fh);
    return false;
#else
    delete fileEngine;
    fileEngine = 0;
    QFSFileEngine *fe = new QFSFileEngine;
    fileEngine = fe;
    return fe->open(QIODevice::OpenMode(flags), fh, handleFlags);
#endif
}

QAbstractFileEngine *QFilePrivate::engine() const
{
    if (!fileEngine)
        fileEngine = QAbstractFileEngine::create(fileName);
    return fileEngine;
}

//************* QFile

/*!
    \class QFile
    \inmodule QtCore
    \brief The QFile class provides an interface for reading from and writing to files.

    \ingroup io

    \reentrant

    QFile is an I/O device for reading and writing text and binary
    files and \l{The Qt Resource System}{resources}. A QFile may be
    used by itself or, more conveniently, with a QTextStream or
    QDataStream.

    The file name is usually passed in the constructor, but it can be
    set at any time using setFileName(). QFile expects the file
    separator to be '/' regardless of operating system. The use of
    other separators (e.g., '\\') is not supported.

    You can check for a file's existence using exists(), and remove a
    file using remove(). (More advanced file system related operations
    are provided by QFileInfo and QDir.)

    The file is opened with open(), closed with close(), and flushed
    with flush(). Data is usually read and written using QDataStream
    or QTextStream, but you can also call the QIODevice-inherited
    functions read(), readLine(), readAll(), write(). QFile also
    inherits getChar(), putChar(), and ungetChar(), which work one
    character at a time.

    The size of the file is returned by size(). You can get the
    current file position using pos(), or move to a new file position
    using seek(). If you've reached the end of the file, atEnd()
    returns \c true.

    \section1 Reading Files Directly

    The following example reads a text file line by line:

    \snippet file/file.cpp 0

    The QIODevice::Text flag passed to open() tells Qt to convert
    Windows-style line terminators ("\\r\\n") into C++-style
    terminators ("\\n"). By default, QFile assumes binary, i.e. it
    doesn't perform any conversion on the bytes stored in the file.

    \section1 Using Streams to Read Files

    The next example uses QTextStream to read a text file
    line by line:

    \snippet file/file.cpp 1

    QTextStream takes care of converting the 8-bit data stored on
    disk into a 16-bit Unicode QString. By default, it assumes that
    the user system's local 8-bit encoding is used (e.g., UTF-8
    on most unix based operating systems; see QTextCodec::codecForLocale() for
    details). This can be changed using setCodec().

    To write text, we can use operator<<(), which is overloaded to
    take a QTextStream on the left and various data types (including
    QString) on the right:

    \snippet file/file.cpp 2

    QDataStream is similar, in that you can use operator<<() to write
    data and operator>>() to read it back. See the class
    documentation for details.

    When you use QFile, QFileInfo, and QDir to access the file system
    with Qt, you can use Unicode file names. On Unix, these file
    names are converted to an 8-bit encoding. If you want to use
    standard C++ APIs (\c <cstdio> or \c <iostream>) or
    platform-specific APIs to access files instead of QFile, you can
    use the encodeName() and decodeName() functions to convert
    between Unicode file names and 8-bit file names.

    On Unix, there are some special system files (e.g. in \c /proc) for which
    size() will always return 0, yet you may still be able to read more data
    from such a file; the data is generated in direct response to you calling
    read(). In this case, however, you cannot use atEnd() to determine if
    there is more data to read (since atEnd() will return true for a file that
    claims to have size 0). Instead, you should either call readAll(), or call
    read() or readLine() repeatedly until no more data can be read. The next
    example uses QTextStream to read \c /proc/modules line by line:

    \snippet file/file.cpp 3

    \section1 Signals

    Unlike other QIODevice implementations, such as QTcpSocket, QFile does not
    emit the aboutToClose(), bytesWritten(), or readyRead() signals. This
    implementation detail means that QFile is not suitable for reading and
    writing certain types of files, such as device files on Unix platforms.

    \section1 Platform Specific Issues

    File permissions are handled differently on Linux/Mac OS X and
    Windows.  In a non \l{QIODevice::isWritable()}{writable}
    directory on Linux, files cannot be created. This is not always
    the case on Windows, where, for instance, the 'My Documents'
    directory usually is not writable, but it is still possible to
    create files in it.

    \sa QTextStream, QDataStream, QFileInfo, QDir, {The Qt Resource System}
*/

#ifdef QT_NO_QOBJECT
QFile::QFile()
    : QFileDevice(*new QFilePrivate)
{
}
QFile::QFile(const QString &name)
    : QFileDevice(*new QFilePrivate)
{
    d_func()->fileName = name;
}
QFile::QFile(QFilePrivate &dd)
    : QFileDevice(dd)
{
}
#else
/*!
    \internal
*/
QFile::QFile()
    : QFileDevice(*new QFilePrivate, 0)
{
}
/*!
    Constructs a new file object with the given \a parent.
*/
QFile::QFile(QObject *parent)
    : QFileDevice(*new QFilePrivate, parent)
{
}
/*!
    Constructs a new file object to represent the file with the given \a name.
*/
QFile::QFile(const QString &name)
    : QFileDevice(*new QFilePrivate, 0)
{
    Q_D(QFile);
    d->fileName = name;
}
/*!
    Constructs a new file object with the given \a parent to represent the
    file with the specified \a name.
*/
QFile::QFile(const QString &name, QObject *parent)
    : QFileDevice(*new QFilePrivate, parent)
{
    Q_D(QFile);
    d->fileName = name;
}
/*!
    \internal
*/
QFile::QFile(QFilePrivate &dd, QObject *parent)
    : QFileDevice(dd, parent)
{
}
#endif

/*!
    Destroys the file object, closing it if necessary.
*/
QFile::~QFile()
{
}

/*!
    Returns the name set by setFileName() or to the QFile
    constructors.

    \sa setFileName(), QFileInfo::fileName()
*/
QString QFile::fileName() const
{
    Q_D(const QFile);
    return d->engine()->fileName(QAbstractFileEngine::DefaultName);
}

/*!
    Sets the \a name of the file. The name can have no path, a
    relative path, or an absolute path.

    Do not call this function if the file has already been opened.

    If the file name has no path or a relative path, the path used
    will be the application's current directory path
    \e{at the time of the open()} call.

    Example:
    \snippet code/src_corelib_io_qfile.cpp 0

    Note that the directory separator "/" works for all operating
    systems supported by Qt.

    \sa fileName(), QFileInfo, QDir
*/
void
QFile::setFileName(const QString &name)
{
    Q_D(QFile);
    if (isOpen()) {
        qWarning("QFile::setFileName: File (%s) is already opened",
                 qPrintable(fileName()));
        close();
    }
    if(d->fileEngine) { //get a new file engine later
        delete d->fileEngine;
        d->fileEngine = 0;
    }
    d->fileName = name;
}

/*!
    \fn QString QFile::decodeName(const char *localFileName)

    \overload

    Returns the Unicode version of the given \a localFileName. See
    encodeName() for details.
*/

/*!
    \fn QByteArray QFile::encodeName(const QString &fileName)

    Converts \a fileName to the local 8-bit
    encoding determined by the user's locale. This is sufficient for
    file names that the user chooses. File names hard-coded into the
    application should only use 7-bit ASCII filename characters.

    \sa decodeName()
*/

/*!
    \typedef QFile::EncoderFn
    \obsolete

    This is a typedef for a pointer to a function with the following
    signature:

    \snippet code/src_corelib_io_qfile.cpp 1

    \sa setEncodingFunction(), encodeName()
*/

/*!
    \fn QString QFile::decodeName(const QByteArray &localFileName)

    This does the reverse of QFile::encodeName() using \a localFileName.

    \sa encodeName()
*/

/*!
    \fn void QFile::setEncodingFunction(EncoderFn function)
    \obsolete

    This function does nothing. It is provided for compatibility with Qt 4 code
    that attempted to set a different encoding function for file names. That
    feature is flawed and no longer supported in Qt 5.

    \sa encodeName(), setDecodingFunction()
*/

/*!
    \typedef QFile::DecoderFn

    This is a typedef for a pointer to a function with the following
    signature:

    \snippet code/src_corelib_io_qfile.cpp 2

    \sa setDecodingFunction()
*/

/*!
    \fn void QFile::setDecodingFunction(DecoderFn function)
    \obsolete

    This function does nothing. It is provided for compatibility with Qt 4 code
    that attempted to set a different decoding function for file names. That
    feature is flawed and no longer supported in Qt 5.

    \sa setEncodingFunction(), decodeName()
*/

/*!
    \overload

    Returns \c true if the file specified by fileName() exists; otherwise
    returns \c false.

    \sa fileName(), setFileName()
*/

bool
QFile::exists() const
{
    Q_D(const QFile);
    // 0x1000000 = QAbstractFileEngine::Refresh, forcing an update
    return (d->engine()->fileFlags(QAbstractFileEngine::FlagsMask
                                    | QAbstractFileEngine::FileFlag(0x1000000)) & QAbstractFileEngine::ExistsFlag);
}

/*!
    Returns \c true if the file specified by \a fileName exists; otherwise
    returns \c false.

    \note If \a fileName is a symlink that points to a non-existing
    file, false is returned.
*/

bool
QFile::exists(const QString &fileName)
{
    return QFileInfo::exists(fileName);
}

/*!
    \fn QString QFile::symLinkTarget() const
    \since 4.2
    \overload

    Returns the absolute path of the file or directory a symlink (or shortcut
    on Windows) points to, or a an empty string if the object isn't a symbolic
    link.

    This name may not represent an existing file; it is only a string.
    QFile::exists() returns \c true if the symlink points to an existing file.

    \sa fileName(), setFileName()
*/

/*!
    \obsolete

    Use symLinkTarget() instead.
*/
QString
QFile::readLink() const
{
    Q_D(const QFile);
    return d->engine()->fileName(QAbstractFileEngine::LinkName);
}

/*!
    \fn static QString QFile::symLinkTarget(const QString &fileName)
    \since 4.2

    Returns the absolute path of the file or directory referred to by the
    symlink (or shortcut on Windows) specified by \a fileName, or returns an
    empty string if the \a fileName does not correspond to a symbolic link.

    This name may not represent an existing file; it is only a string.
    QFile::exists() returns \c true if the symlink points to an existing file.
*/

/*!
    \obsolete

    Use symLinkTarget() instead.
*/
QString
QFile::readLink(const QString &fileName)
{
    return QFileInfo(fileName).readLink();
}

/*!
    Removes the file specified by fileName(). Returns \c true if successful;
    otherwise returns \c false.

    The file is closed before it is removed.

    \sa setFileName()
*/

bool
QFile::remove()
{
    Q_D(QFile);
    if (d->fileName.isEmpty()) {
        qWarning("QFile::remove: Empty or null file name");
        return false;
    }
    unsetError();
    close();
    if(error() == QFile::NoError) {
        if (d->engine()->remove()) {
            unsetError();
            return true;
        }
        d->setError(QFile::RemoveError, d->fileEngine->errorString());
    }
    return false;
}

/*!
    \overload

    Removes the file specified by the \a fileName given.

    Returns \c true if successful; otherwise returns \c false.

    \sa remove()
*/

bool
QFile::remove(const QString &fileName)
{
    return QFile(fileName).remove();
}

/*!
    Renames the file currently specified by fileName() to \a newName.
    Returns \c true if successful; otherwise returns \c false.

    If a file with the name \a newName already exists, rename() returns \c false
    (i.e., QFile will not overwrite it).

    The file is closed before it is renamed.

    If the rename operation fails, Qt will attempt to copy this file's
    contents to \a newName, and then remove this file, keeping only
    \a newName. If that copy operation fails or this file can't be removed,
    the destination file \a newName is removed to restore the old state.

    \sa setFileName()
*/

bool
QFile::rename(const QString &newName)
{
    Q_D(QFile);
    if (d->fileName.isEmpty()) {
        qWarning("QFile::rename: Empty or null file name");
        return false;
    }
    if (d->fileName == newName) {
        d->setError(QFile::RenameError, tr("Destination file is the same file."));
        return false;
    }
    if (!exists()) {
        d->setError(QFile::RenameError, tr("Source file does not exist."));
        return false;
    }
    // If the file exists and it is a case-changing rename ("foo" -> "Foo"),
    // compare Ids to make sure it really is a different file.
    if (QFile::exists(newName)) {
        if (d->fileName.compare(newName, Qt::CaseInsensitive)
            || QFileSystemEngine::id(QFileSystemEntry(d->fileName)) != QFileSystemEngine::id(QFileSystemEntry(newName))) {
            // ### Race condition. If a file is moved in after this, it /will/ be
            // overwritten. On Unix, the proper solution is to use hardlinks:
            // return ::link(old, new) && ::remove(old);
            d->setError(QFile::RenameError, tr("Destination file exists"));
            return false;
        }
#ifndef QT_NO_TEMPORARYFILE
        // This #ifndef disables the workaround it encloses. Therefore, this configuration is not recommended.
#ifdef Q_OS_LINUX
        // rename() on Linux simply does nothing when renaming "foo" to "Foo" on a case-insensitive
        // FS, such as FAT32. Move the file away and rename in 2 steps to work around.
        QTemporaryFile tempFile(d->fileName + QStringLiteral(".XXXXXX"));
        tempFile.setAutoRemove(false);
        if (!tempFile.open(QIODevice::ReadWrite)) {
            d->setError(QFile::RenameError, tempFile.errorString());
            return false;
        }
        tempFile.close();
        if (!d->engine()->rename(tempFile.fileName())) {
            d->setError(QFile::RenameError, tr("Error while renaming."));
            return false;
        }
        if (tempFile.rename(newName)) {
            d->fileEngine->setFileName(newName);
            d->fileName = newName;
            return true;
        }
        d->setError(QFile::RenameError, tempFile.errorString());
        // We need to restore the original file.
        if (!tempFile.rename(d->fileName)) {
            d->setError(QFile::RenameError, errorString() + QLatin1Char('\n')
                        + tr("Unable to restore from %1: %2").
                        arg(QDir::toNativeSeparators(tempFile.fileName()), tempFile.errorString()));
        }
        return false;
#endif // Q_OS_LINUX
#endif // QT_NO_TEMPORARYFILE
    }
    unsetError();
    close();
    if(error() == QFile::NoError) {
        if (d->engine()->rename(newName)) {
            unsetError();
            // engine was able to handle the new name so we just reset it
            d->fileEngine->setFileName(newName);
            d->fileName = newName;
            return true;
        }

        if (isSequential()) {
            d->setError(QFile::RenameError, tr("Will not rename sequential file using block copy"));
            return false;
        }

        QFile out(newName);
        if (open(QIODevice::ReadOnly)) {
            if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                bool error = false;
                char block[4096];
                qint64 bytes;
                while ((bytes = read(block, sizeof(block))) > 0) {
                    if (bytes != out.write(block, bytes)) {
                        d->setError(QFile::RenameError, out.errorString());
                        error = true;
                        break;
                    }
                }
                if (bytes == -1) {
                    d->setError(QFile::RenameError, errorString());
                    error = true;
                }
                if(!error) {
                    if (!remove()) {
                        d->setError(QFile::RenameError, tr("Cannot remove source file"));
                        error = true;
                    }
                }
                if (error) {
                    out.remove();
                } else {
                    d->fileEngine->setFileName(newName);
                    setPermissions(permissions());
                    unsetError();
                    setFileName(newName);
                }
                close();
                return !error;
            }
            close();
        }
        d->setError(QFile::RenameError, out.isOpen() ? errorString() : out.errorString());
    }
    return false;
}

/*!
    \overload

    Renames the file \a oldName to \a newName. Returns \c true if
    successful; otherwise returns \c false.

    If a file with the name \a newName already exists, rename() returns \c false
    (i.e., QFile will not overwrite it).

    \sa rename()
*/

bool
QFile::rename(const QString &oldName, const QString &newName)
{
    return QFile(oldName).rename(newName);
}

/*!

    Creates a link named \a linkName that points to the file currently specified by
    fileName().  What a link is depends on the underlying filesystem (be it a
    shortcut on Windows or a symbolic link on Unix). Returns \c true if successful;
    otherwise returns \c false.

    This function will not overwrite an already existing entity in the file system;
    in this case, \c link() will return false and set \l{QFile::}{error()} to
    return \l{QFile::}{RenameError}.

    \note To create a valid link on Windows, \a linkName must have a \c{.lnk} file extension.

    \sa setFileName()
*/

bool
QFile::link(const QString &linkName)
{
    Q_D(QFile);
    if (d->fileName.isEmpty()) {
        qWarning("QFile::link: Empty or null file name");
        return false;
    }
    QFileInfo fi(linkName);
    if (d->engine()->link(fi.absoluteFilePath())) {
        unsetError();
        return true;
    }
    d->setError(QFile::RenameError, d->fileEngine->errorString());
    return false;
}

/*!
    \overload

    Creates a link named \a linkName that points to the file \a fileName. What a link is
    depends on the underlying filesystem (be it a shortcut on Windows
    or a symbolic link on Unix). Returns \c true if successful; otherwise
    returns \c false.

    \sa link()
*/

bool
QFile::link(const QString &fileName, const QString &linkName)
{
    return QFile(fileName).link(linkName);
}

/*!
    Copies the file currently specified by fileName() to a file called
    \a newName.  Returns \c true if successful; otherwise returns \c false.

    Note that if a file with the name \a newName already exists,
    copy() returns \c false (i.e. QFile will not overwrite it).

    The source file is closed before it is copied.

    \sa setFileName()
*/

bool
QFile::copy(const QString &newName)
{
    Q_D(QFile);
    if (d->fileName.isEmpty()) {
        qWarning("QFile::copy: Empty or null file name");
        return false;
    }
    if (QFile(newName).exists()) {
        // ### Race condition. If a file is moved in after this, it /will/ be
        // overwritten. On Unix, the proper solution is to use hardlinks:
        // return ::link(old, new) && ::remove(old); See also rename().
        d->setError(QFile::CopyError, tr("Destination file exists"));
        return false;
    }
    unsetError();
    close();
    if(error() == QFile::NoError) {
        if (d->engine()->copy(newName)) {
            unsetError();
            return true;
        } else {
            bool error = false;
            if(!open(QFile::ReadOnly)) {
                error = true;
                d->setError(QFile::CopyError, tr("Cannot open %1 for input").arg(d->fileName));
            } else {
                QString fileTemplate = QLatin1String("%1/qt_temp.XXXXXX");
#ifdef QT_NO_TEMPORARYFILE
                QFile out(fileTemplate.arg(QFileInfo(newName).path()));
                if (!out.open(QIODevice::ReadWrite))
                    error = true;
#else
                QTemporaryFile out(fileTemplate.arg(QFileInfo(newName).path()));
                if (!out.open()) {
                    out.setFileTemplate(fileTemplate.arg(QDir::tempPath()));
                    if (!out.open())
                        error = true;
                }
#endif
                if (error) {
                    out.close();
                    close();
                    d->setError(QFile::CopyError, tr("Cannot open for output"));
                } else {
                    char block[4096];
                    qint64 totalRead = 0;
                    while(!atEnd()) {
                        qint64 in = read(block, sizeof(block));
                        if (in <= 0)
                            break;
                        totalRead += in;
                        if(in != out.write(block, in)) {
                            close();
                            d->setError(QFile::CopyError, tr("Failure to write block"));
                            error = true;
                            break;
                        }
                    }

                    if (totalRead != size()) {
                        // Unable to read from the source. The error string is
                        // already set from read().
                        error = true;
                    }
                    if (!error && !out.rename(newName)) {
                        error = true;
                        close();
                        d->setError(QFile::CopyError, tr("Cannot create %1 for output").arg(newName));
                    }
#ifdef QT_NO_TEMPORARYFILE
                    if (error)
                        out.remove();
#else
                    if (!error)
                        out.setAutoRemove(false);
#endif
                }
            }
            if(!error) {
                QFile::setPermissions(newName, permissions());
                close();
                unsetError();
                return true;
            }
        }
    }
    return false;
}

/*!
    \overload

    Copies the file \a fileName to \a newName. Returns \c true if successful;
    otherwise returns \c false.

    If a file with the name \a newName already exists, copy() returns \c false
    (i.e., QFile will not overwrite it).

    \sa rename()
*/

bool
QFile::copy(const QString &fileName, const QString &newName)
{
    return QFile(fileName).copy(newName);
}

/*!
    Opens the file using OpenMode \a mode, returning true if successful;
    otherwise false.

    The \a mode must be QIODevice::ReadOnly, QIODevice::WriteOnly, or
    QIODevice::ReadWrite. It may also have additional flags, such as
    QIODevice::Text and QIODevice::Unbuffered.

    \note In \l{QIODevice::}{WriteOnly} or \l{QIODevice::}{ReadWrite}
    mode, if the relevant file does not already exist, this function
    will try to create a new file before opening it.

    \sa QIODevice::OpenMode, setFileName()
*/
bool QFile::open(OpenMode mode)
{
    Q_D(QFile);
    if (isOpen()) {
        qWarning("QFile::open: File (%s) already open", qPrintable(fileName()));
        return false;
    }
    if (mode & Append)
        mode |= WriteOnly;

    unsetError();
    if ((mode & (ReadOnly | WriteOnly)) == 0) {
        qWarning("QIODevice::open: File access not specified");
        return false;
    }

    // QIODevice provides the buffering, so there's no need to request it from the file engine.
    if (d->engine()->open(mode | QIODevice::Unbuffered)) {
        QIODevice::open(mode);
        if (mode & Append)
            seek(size());
        return true;
    }
    QFile::FileError err = d->fileEngine->error();
    if(err == QFile::UnspecifiedError)
        err = QFile::OpenError;
    d->setError(err, d->fileEngine->errorString());
    return false;
}

/*!
    \overload

    Opens the existing file handle \a fh in the given \a mode.
    \a handleFlags may be used to specify additional options.
    Returns \c true if successful; otherwise returns \c false.

    Example:
    \snippet code/src_corelib_io_qfile.cpp 3

    When a QFile is opened using this function, behaviour of close() is
    controlled by the AutoCloseHandle flag.
    If AutoCloseHandle is specified, and this function succeeds,
    then calling close() closes the adopted handle.
    Otherwise, close() does not actually close the file, but only flushes it.

    \b{Warning:}
    \list 1
        \li If \a fh does not refer to a regular file, e.g., it is \c stdin,
           \c stdout, or \c stderr, you may not be able to seek(). size()
           returns \c 0 in those cases. See QIODevice::isSequential() for
           more information.
        \li Since this function opens the file without specifying the file name,
           you cannot use this QFile with a QFileInfo.
    \endlist

    \note For Windows CE you may not be able to call resize().

    \sa close()

    \b{Note for the Windows Platform}

    \a fh must be opened in binary mode (i.e., the mode string must contain
    'b', as in "rb" or "wb") when accessing files and other random-access
    devices. Qt will translate the end-of-line characters if you pass
    QIODevice::Text to \a mode. Sequential devices, such as stdin and stdout,
    are unaffected by this limitation.

    You need to enable support for console applications in order to use the
    stdin, stdout and stderr streams at the console. To do this, add the
    following declaration to your application's project file:

    \snippet code/src_corelib_io_qfile.cpp 4
*/
bool QFile::open(FILE *fh, OpenMode mode, FileHandleFlags handleFlags)
{
    Q_D(QFile);
    if (isOpen()) {
        qWarning("QFile::open: File (%s) already open", qPrintable(fileName()));
        return false;
    }
    if (mode & Append)
        mode |= WriteOnly;
    unsetError();
    if ((mode & (ReadOnly | WriteOnly)) == 0) {
        qWarning("QFile::open: File access not specified");
        return false;
    }
    if (d->openExternalFile(mode, fh, handleFlags)) {
        QIODevice::open(mode);
        if (!(mode & Append) && !isSequential()) {
            qint64 pos = (qint64)QT_FTELL(fh);
            if (pos != -1) {
                // Skip redundant checks in QFileDevice::seek().
                QIODevice::seek(pos);
            }
        }
        return true;
    }
    return false;
}

/*!
    \overload

    Opens the existing file descriptor \a fd in the given \a mode.
    \a handleFlags may be used to specify additional options.
    Returns \c true if successful; otherwise returns \c false.

    When a QFile is opened using this function, behaviour of close() is
    controlled by the AutoCloseHandle flag.
    If AutoCloseHandle is specified, and this function succeeds,
    then calling close() closes the adopted handle.
    Otherwise, close() does not actually close the file, but only flushes it.

    The QFile that is opened using this function is automatically set
    to be in raw mode; this means that the file input/output functions
    are slow. If you run into performance issues, you should try to
    use one of the other open functions.

    \warning If \a fd is not a regular file, e.g, it is 0 (\c stdin),
    1 (\c stdout), or 2 (\c stderr), you may not be able to seek(). In
    those cases, size() returns \c 0.  See QIODevice::isSequential()
    for more information.

    \warning For Windows CE you may not be able to call seek(), setSize(),
    fileTime(). size() returns \c 0.

    \warning Since this function opens the file without specifying the file name,
             you cannot use this QFile with a QFileInfo.

    \sa close()
*/
bool QFile::open(int fd, OpenMode mode, FileHandleFlags handleFlags)
{
    Q_D(QFile);
    if (isOpen()) {
        qWarning("QFile::open: File (%s) already open", qPrintable(fileName()));
        return false;
    }
    if (mode & Append)
        mode |= WriteOnly;
    unsetError();
    if ((mode & (ReadOnly | WriteOnly)) == 0) {
        qWarning("QFile::open: File access not specified");
        return false;
    }
    if (d->openExternalFile(mode, fd, handleFlags)) {
        QIODevice::open(mode);
        if (!(mode & Append) && !isSequential()) {
            qint64 pos = (qint64)QT_LSEEK(fd, QT_OFF_T(0), SEEK_CUR);
            if (pos != -1) {
                // Skip redundant checks in QFileDevice::seek().
                QIODevice::seek(pos);
            }
        }
        return true;
    }
    return false;
}

/*!
    \reimp
*/
bool QFile::resize(qint64 sz)
{
    return QFileDevice::resize(sz); // for now
}

/*!
    \overload

    Sets \a fileName to size (in bytes) \a sz. Returns \c true if the file if
    the resize succeeds; false otherwise. If \a sz is larger than \a
    fileName currently is the new bytes will be set to 0, if \a sz is
    smaller the file is simply truncated.

    \sa resize()
*/

bool
QFile::resize(const QString &fileName, qint64 sz)
{
    return QFile(fileName).resize(sz);
}

/*!
    \reimp
*/
QFile::Permissions QFile::permissions() const
{
    return QFileDevice::permissions(); // for now
}

/*!
    \overload

    Returns the complete OR-ed together combination of
    QFile::Permission for \a fileName.
*/

QFile::Permissions
QFile::permissions(const QString &fileName)
{
    return QFile(fileName).permissions();
}

/*!
    Sets the permissions for the file to the \a permissions specified.
    Returns \c true if successful, or false if the permissions cannot be
    modified.

    \sa permissions(), setFileName()
*/

bool QFile::setPermissions(Permissions permissions)
{
    return QFileDevice::setPermissions(permissions); // for now
}

/*!
    \overload

    Sets the permissions for \a fileName file to \a permissions.
*/

bool
QFile::setPermissions(const QString &fileName, Permissions permissions)
{
    return QFile(fileName).setPermissions(permissions);
}

/*!
  \reimp
*/
qint64 QFile::size() const
{
    return QFileDevice::size(); // for now
}

QT_END_NAMESPACE
