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

#include "qplatformdefs.h"
#include "qdebug.h"
#include "qfile.h"
#include "qfsfileengine.h"
#include "qtemporaryfile.h"
#include "qlist.h"
#include "qfileinfo.h"
#include "private/qiodevice_p.h"
#include "private/qfile_p.h"
#include "private/qsystemerror_p.h"
#if defined(QT_BUILD_CORE_LIB)
# include "qcoreapplication.h"
#endif

#ifdef QT_NO_QOBJECT
#define tr(X) QString::fromLatin1(X)
#endif

QT_BEGIN_NAMESPACE

static const int QFILE_WRITEBUFFER_SIZE = 16384;

static QByteArray locale_encode(const QString &f)
{
#if defined(Q_OS_DARWIN)
    // Mac always expects UTF-8... and decomposed...
    return f.normalized(QString::NormalizationForm_D).toUtf8();
#elif defined(Q_OS_SYMBIAN)
    return f.toUtf8();
#else
    return f.toLocal8Bit();
#endif
}

static QString locale_decode(const QByteArray &f)
{
#if defined(Q_OS_DARWIN)
    // Mac always gives us UTF-8 and decomposed, we want that composed...
    return QString::fromUtf8(f).normalized(QString::NormalizationForm_C);
#elif defined(Q_OS_SYMBIAN)
    return QString::fromUtf8(f);
#else
    return QString::fromLocal8Bit(f);
#endif
}

//************* QFilePrivate
QFile::EncoderFn QFilePrivate::encoder = locale_encode;
QFile::DecoderFn QFilePrivate::decoder = locale_decode;

QFilePrivate::QFilePrivate()
    : fileEngine(0), lastWasWrite(false),
      writeBuffer(QFILE_WRITEBUFFER_SIZE), error(QFile::NoError),
      cachedSize(0)
{
}

QFilePrivate::~QFilePrivate()
{
    delete fileEngine;
    fileEngine = 0;
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

#ifdef Q_OS_SYMBIAN
bool QFilePrivate::openExternalFile(int flags, const RFile &f, QFile::FileHandleFlags handleFlags)
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
    return fe->open(QIODevice::OpenMode(flags), f, handleFlags);
#endif
}
#endif

inline bool QFilePrivate::ensureFlushed() const
{
    // This function ensures that the write buffer has been flushed (const
    // because certain const functions need to call it.
    if (lastWasWrite) {
        const_cast<QFilePrivate *>(this)->lastWasWrite = false;
        if (!const_cast<QFile *>(q_func())->flush())
            return false;
    }
    return true;
}

void
QFilePrivate::setError(QFile::FileError err)
{
    error = err;
    errorString.clear();
}

void
QFilePrivate::setError(QFile::FileError err, const QString &errStr)
{
    error = err;
    errorString = errStr;
}

void
QFilePrivate::setError(QFile::FileError err, int errNum)
{
    error = err;
    errorString = qt_error_string(errNum);
}

//************* QFile

/*!
    \class QFile
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
    returns true.

    \section1 Reading Files Directly

    The following example reads a text file line by line:

    \snippet doc/src/snippets/file/file.cpp 0

    The QIODevice::Text flag passed to open() tells Qt to convert
    Windows-style line terminators ("\\r\\n") into C++-style
    terminators ("\\n"). By default, QFile assumes binary, i.e. it
    doesn't perform any conversion on the bytes stored in the file.

    \section1 Using Streams to Read Files

    The next example uses QTextStream to read a text file
    line by line:

    \snippet doc/src/snippets/file/file.cpp 1

    QTextStream takes care of converting the 8-bit data stored on
    disk into a 16-bit Unicode QString. By default, it assumes that
    the user system's local 8-bit encoding is used (e.g., ISO 8859-1
    for most of Europe; see QTextCodec::codecForLocale() for
    details). This can be changed using setCodec().

    To write text, we can use operator<<(), which is overloaded to
    take a QTextStream on the left and various data types (including
    QString) on the right:

    \snippet doc/src/snippets/file/file.cpp 2

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

    \snippet doc/src/snippets/file/file.cpp 3

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

/*!
    \enum QFile::FileError

    This enum describes the errors that may be returned by the error()
    function.

    \value NoError          No error occurred.
    \value ReadError        An error occurred when reading from the file.
    \value WriteError       An error occurred when writing to the file.
    \value FatalError       A fatal error occurred.
    \value ResourceError
    \value OpenError        The file could not be opened.
    \value AbortError       The operation was aborted.
    \value TimeOutError     A timeout occurred.
    \value UnspecifiedError An unspecified error occurred.
    \value RemoveError      The file could not be removed.
    \value RenameError      The file could not be renamed.
    \value PositionError    The position in the file could not be changed.
    \value ResizeError      The file could not be resized.
    \value PermissionsError The file could not be accessed.
    \value CopyError        The file could not be copied.

    \omitvalue ConnectError
*/

/*!
    \enum QFile::Permission

    This enum is used by the permission() function to report the
    permissions and ownership of a file. The values may be OR-ed
    together to test multiple permissions and ownership values.

    \value ReadOwner The file is readable by the owner of the file.
    \value WriteOwner The file is writable by the owner of the file.
    \value ExeOwner The file is executable by the owner of the file.
    \value ReadUser The file is readable by the user.
    \value WriteUser The file is writable by the user.
    \value ExeUser The file is executable by the user.
    \value ReadGroup The file is readable by the group.
    \value WriteGroup The file is writable by the group.
    \value ExeGroup The file is executable by the group.
    \value ReadOther The file is readable by anyone.
    \value WriteOther The file is writable by anyone.
    \value ExeOther The file is executable by anyone.

    \warning Because of differences in the platforms supported by Qt,
    the semantics of ReadUser, WriteUser and ExeUser are
    platform-dependent: On Unix, the rights of the owner of the file
    are returned and on Windows the rights of the current user are
    returned. This behavior might change in a future Qt version.

    Note that Qt does not by default check for permissions on NTFS
    file systems, as this may decrease the performance of file
    handling considerably. It is possible to force permission checking
    on NTFS by including the following code in your source:

    \snippet doc/src/snippets/ntfsp.cpp 0

    Permission checking is then turned on and off by incrementing and
    decrementing \c qt_ntfs_permission_lookup by 1.

    \snippet doc/src/snippets/ntfsp.cpp 1
*/

/*!
    \enum QFile::FileHandleFlag
    \since 4.8

    This enum is used when opening a file to specify additional
    options which only apply to files and not to a generic
    QIODevice.

    \value AutoCloseHandle The file handle passed into open() should be
    closed by close(), the default behaviour is that close just flushes
    the file and the application is responsible for closing the file handle.
    When opening a file by name, this flag is ignored as Qt always "owns" the
    file handle and must close it.
    \value DontCloseHandle The file handle passed into open() will not be
    closed by Qt. The application must ensure that close() is called.
 */

#ifdef QT3_SUPPORT
/*!
    \typedef QFile::PermissionSpec

    Use QFile::Permission instead.
*/
#endif

#ifdef QT_NO_QOBJECT
QFile::QFile()
    : QIODevice(*new QFilePrivate)
{
}
QFile::QFile(const QString &name)
    : QIODevice(*new QFilePrivate)
{
    d_func()->fileName = name;
}
QFile::QFile(QFilePrivate &dd)
    : QIODevice(dd)
{
}
#else
/*!
    \internal
*/
QFile::QFile()
    : QIODevice(*new QFilePrivate, 0)
{
}
/*!
    Constructs a new file object with the given \a parent.
*/
QFile::QFile(QObject *parent)
    : QIODevice(*new QFilePrivate, parent)
{
}
/*!
    Constructs a new file object to represent the file with the given \a name.
*/
QFile::QFile(const QString &name)
    : QIODevice(*new QFilePrivate, 0)
{
    Q_D(QFile);
    d->fileName = name;
}
/*!
    Constructs a new file object with the given \a parent to represent the
    file with the specified \a name.
*/
QFile::QFile(const QString &name, QObject *parent)
    : QIODevice(*new QFilePrivate, parent)
{
    Q_D(QFile);
    d->fileName = name;
}
/*!
    \internal
*/
QFile::QFile(QFilePrivate &dd, QObject *parent)
    : QIODevice(dd, parent)
{
}
#endif

/*!
    Destroys the file object, closing it if necessary.
*/
QFile::~QFile()
{
    close();
}

/*!
    Returns the name set by setFileName() or to the QFile
    constructors.

    \sa setFileName(), QFileInfo::fileName()
*/
QString QFile::fileName() const
{
    return fileEngine()->fileName(QAbstractFileEngine::DefaultName);
}

/*!
    Sets the \a name of the file. The name can have no path, a
    relative path, or an absolute path.

    Do not call this function if the file has already been opened.

    If the file name has no path or a relative path, the path used
    will be the application's current directory path
    \e{at the time of the open()} call.

    Example:
    \snippet doc/src/snippets/code/src_corelib_io_qfile.cpp 0

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
    By default, this function converts \a fileName to the local 8-bit
    encoding determined by the user's locale. This is sufficient for
    file names that the user chooses. File names hard-coded into the
    application should only use 7-bit ASCII filename characters.

    \sa decodeName() setEncodingFunction()
*/

QByteArray
QFile::encodeName(const QString &fileName)
{
    return (*QFilePrivate::encoder)(fileName);
}

/*!
    \typedef QFile::EncoderFn

    This is a typedef for a pointer to a function with the following
    signature:

    \snippet doc/src/snippets/code/src_corelib_io_qfile.cpp 1

    \sa setEncodingFunction(), encodeName()
*/

/*!
    This does the reverse of QFile::encodeName() using \a localFileName.

    \sa setDecodingFunction(), encodeName()
*/

QString
QFile::decodeName(const QByteArray &localFileName)
{
    return (*QFilePrivate::decoder)(localFileName);
}

/*!
    \fn void QFile::setEncodingFunction(EncoderFn function)

    \nonreentrant

    Sets the \a function for encoding Unicode file names. The
    default encodes in the locale-specific 8-bit encoding.

    \sa encodeName(), setDecodingFunction()
*/

void
QFile::setEncodingFunction(EncoderFn f)
{
    if (!f)
        f = locale_encode;
    QFilePrivate::encoder = f;
}

/*!
    \typedef QFile::DecoderFn

    This is a typedef for a pointer to a function with the following
    signature:

    \snippet doc/src/snippets/code/src_corelib_io_qfile.cpp 2

    \sa setDecodingFunction()
*/

/*!
    \fn void QFile::setDecodingFunction(DecoderFn function)

    \nonreentrant

    Sets the \a function for decoding 8-bit file names. The
    default uses the locale-specific 8-bit encoding.

    \sa setEncodingFunction(), decodeName()
*/

void
QFile::setDecodingFunction(DecoderFn f)
{
    if (!f)
        f = locale_decode;
    QFilePrivate::decoder = f;
}

/*!
    \overload

    Returns true if the file specified by fileName() exists; otherwise
    returns false.

    \sa fileName(), setFileName()
*/

bool
QFile::exists() const
{
    // 0x1000000 = QAbstractFileEngine::Refresh, forcing an update
    return (fileEngine()->fileFlags(QAbstractFileEngine::FlagsMask
                                    | QAbstractFileEngine::FileFlag(0x1000000)) & QAbstractFileEngine::ExistsFlag);
}

/*!
    Returns true if the file specified by \a fileName exists; otherwise
    returns false.
*/

bool
QFile::exists(const QString &fileName)
{
    return QFileInfo(fileName).exists();
}

/*!
    \fn QString QFile::symLinkTarget() const
    \since 4.2
    \overload

    Returns the absolute path of the file or directory a symlink (or shortcut
    on Windows) points to, or a an empty string if the object isn't a symbolic
    link.

    This name may not represent an existing file; it is only a string.
    QFile::exists() returns true if the symlink points to an existing file.

    \sa fileName() setFileName()
*/

/*!
    \obsolete

    Use symLinkTarget() instead.
*/
QString
QFile::readLink() const
{
    return fileEngine()->fileName(QAbstractFileEngine::LinkName);
}

/*!
    \fn static QString QFile::symLinkTarget(const QString &fileName)
    \since 4.2

    Returns the absolute path of the file or directory referred to by the
    symlink (or shortcut on Windows) specified by \a fileName, or returns an
    empty string if the \a fileName does not correspond to a symbolic link.

    This name may not represent an existing file; it is only a string.
    QFile::exists() returns true if the symlink points to an existing file.
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
    Removes the file specified by fileName(). Returns true if successful;
    otherwise returns false.

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
        if(fileEngine()->remove()) {
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

    Returns true if successful; otherwise returns false.

    \sa remove()
*/

bool
QFile::remove(const QString &fileName)
{
    return QFile(fileName).remove();
}

/*!
    Renames the file currently specified by fileName() to \a newName.
    Returns true if successful; otherwise returns false.

    If a file with the name \a newName already exists, rename() returns false
    (i.e., QFile will not overwrite it).

    The file is closed before it is renamed.

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
    if (QFile(newName).exists()) {
        // ### Race condition. If a file is moved in after this, it /will/ be
        // overwritten. On Unix, the proper solution is to use hardlinks:
        // return ::link(old, new) && ::remove(old);
        d->setError(QFile::RenameError, tr("Destination file exists"));
        return false;
    }
    unsetError();
    close();
    if(error() == QFile::NoError) {
        if (fileEngine()->rename(newName)) {
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

    Renames the file \a oldName to \a newName. Returns true if
    successful; otherwise returns false.

    If a file with the name \a newName already exists, rename() returns false
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
    shortcut on Windows or a symbolic link on Unix). Returns true if successful;
    otherwise returns false.

    This function will not overwrite an already existing entity in the file system;
    in this case, \c link() will return false and set \l{QFile::}{error()} to
    return \l{QFile::}{RenameError}.

    \note To create a valid link on Windows, \a linkName must have a \c{.lnk} file extension.

    \note Symbian filesystem does not support links.

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
    if(fileEngine()->link(fi.absoluteFilePath())) {
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
    or a symbolic link on Unix). Returns true if successful; otherwise
    returns false.

    \sa link()
*/

bool
QFile::link(const QString &fileName, const QString &linkName)
{
    return QFile(fileName).link(linkName);
}

/*!
    Copies the file currently specified by fileName() to a file called
    \a newName.  Returns true if successful; otherwise returns false.

    Note that if a file with the name \a newName already exists,
    copy() returns false (i.e. QFile will not overwrite it).

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
        if(fileEngine()->copy(newName)) {
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

    Copies the file \a fileName to \a newName. Returns true if successful;
    otherwise returns false.

    If a file with the name \a newName already exists, copy() returns false
    (i.e., QFile will not overwrite it).

    \sa rename()
*/

bool
QFile::copy(const QString &fileName, const QString &newName)
{
    return QFile(fileName).copy(newName);
}

/*!
    Returns true if the file can only be manipulated sequentially;
    otherwise returns false.

    Most files support random-access, but some special files may not.

    \sa QIODevice::isSequential()
*/
bool QFile::isSequential() const
{
    Q_D(const QFile);
    return d->fileEngine && d->fileEngine->isSequential();
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

#ifdef Q_OS_SYMBIAN
    // For symbian, the unbuffered flag is used to control write-behind cache behaviour
    if (fileEngine()->open(mode))
#else
    // QIODevice provides the buffering, so there's no need to request it from the file engine.
    if (fileEngine()->open(mode | QIODevice::Unbuffered))
#endif
    {
        QIODevice::open(mode);
        if (mode & Append) {
            //The file engine should have done this in open(),
            //this is a workaround for backward compatibility
            fileEngine()->seek(size());
        }
        return true;
    }
    QFile::FileError err = d->fileEngine->error();
    if(err == QFile::UnspecifiedError)
        err = QFile::OpenError;
    d->setError(err, d->fileEngine->errorString());
    return false;
}

/*! \fn QFile::open(OpenMode, FILE*)

    Use open(FILE *, OpenMode) instead.
*/

/*!
    \overload

    Opens the existing file handle \a fh in the given \a mode.
    Returns true if successful; otherwise returns false.

    Example:
    \snippet doc/src/snippets/code/src_corelib_io_qfile.cpp 3

    When a QFile is opened using this function, close() does not actually
    close the file, but only flushes it.

    \bold{Warning:}
    \list 1
        \o If \a fh does not refer to a regular file, e.g., it is \c stdin,
           \c stdout, or \c stderr, you may not be able to seek(). size()
           returns \c 0 in those cases. See QIODevice::isSequential() for
           more information.
        \o Since this function opens the file without specifying the file name,
           you cannot use this QFile with a QFileInfo.
    \endlist

    \note For Windows CE you may not be able to call resize().

    \sa close(), {qmake Variable Reference#CONFIG}{qmake Variable Reference}

    \bold{Note for the Windows Platform}

    \a fh must be opened in binary mode (i.e., the mode string must contain
    'b', as in "rb" or "wb") when accessing files and other random-access
    devices. Qt will translate the end-of-line characters if you pass
    QIODevice::Text to \a mode. Sequential devices, such as stdin and stdout,
    are unaffected by this limitation.

    You need to enable support for console applications in order to use the
    stdin, stdout and stderr streams at the console. To do this, add the
    following declaration to your application's project file:

    \snippet doc/src/snippets/code/src_corelib_io_qfile.cpp 4
*/
// ### Qt5: merge this into new overload with a default parameter
bool QFile::open(FILE *fh, OpenMode mode)
{
    return open(fh, mode, DontCloseHandle);
}

/*!
    \overload

    Opens the existing file handle \a fh in the given \a mode.
    Returns true if successful; otherwise returns false.

    Example:
    \snippet doc/src/snippets/code/src_corelib_io_qfile.cpp 3

    When a QFile is opened using this function, behaviour of close() is
    controlled by the AutoCloseHandle flag.
    If AutoCloseHandle is specified, and this function succeeds,
    then calling close() closes the adopted handle.
    Otherwise, close() does not actually close the file, but only flushes it.

    \bold{Warning:}
    \list 1
        \o If \a fh does not refer to a regular file, e.g., it is \c stdin,
           \c stdout, or \c stderr, you may not be able to seek(). size()
           returns \c 0 in those cases. See QIODevice::isSequential() for
           more information.
        \o Since this function opens the file without specifying the file name,
           you cannot use this QFile with a QFileInfo.
    \endlist

    \note For Windows CE you may not be able to call resize().

    \sa close(), {qmake Variable Reference#CONFIG}{qmake Variable Reference}

    \bold{Note for the Windows Platform}

    \a fh must be opened in binary mode (i.e., the mode string must contain
    'b', as in "rb" or "wb") when accessing files and other random-access
    devices. Qt will translate the end-of-line characters if you pass
    QIODevice::Text to \a mode. Sequential devices, such as stdin and stdout,
    are unaffected by this limitation.

    You need to enable support for console applications in order to use the
    stdin, stdout and stderr streams at the console. To do this, add the
    following declaration to your application's project file:

    \snippet doc/src/snippets/code/src_corelib_io_qfile.cpp 4
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
        if (mode & Append) {
            seek(size());
        } else {
            qint64 pos = (qint64)QT_FTELL(fh);
            if (pos != -1)
                seek(pos);
        }
        return true;
    }
    return false;
}

/*! \fn QFile::open(OpenMode, int)

    Use open(int, OpenMode) instead.
*/

/*!
    \overload

    Opens the existing file descriptor \a fd in the given \a mode.
    Returns true if successful; otherwise returns false.

    When a QFile is opened using this function, close() does not
    actually close the file.

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
// ### Qt5: merge this into new overload with a default parameter
bool QFile::open(int fd, OpenMode mode)
{
    return open(fd, mode, DontCloseHandle);
}

/*!
    \overload

    Opens the existing file descriptor \a fd in the given \a mode.
    Returns true if successful; otherwise returns false.

    When a QFile is opened using this function, behaviour of close() is
    controlled by the \a handleFlags argument.
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
        if (mode & Append) {
            seek(size());
        } else {
            qint64 pos = (qint64)QT_LSEEK(fd, QT_OFF_T(0), SEEK_CUR);
            if (pos != -1)
                seek(pos);
        }
        return true;
    }
    return false;
}

#ifdef Q_OS_SYMBIAN
/*!
    \overload

    Opens the existing file object \a f in the given \a mode.
    Returns true if successful; otherwise returns false.

    When a QFile is opened using this function, behaviour of close() is
    controlled by the \a handleFlags argument.
    If AutoCloseHandle is specified, and this function succeeds,
    then calling close() closes the adopted handle.
    Otherwise, close() does not actually close the file, but only flushes it.

    \warning If the file handle is adopted from another process,
             you may not be able to use this QFile with a QFileInfo.

    \sa close()
*/
bool QFile::open(const RFile &f, OpenMode mode, FileHandleFlags handleFlags)
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
    if (d->openExternalFile(mode, f, handleFlags)) {
        bool ok = QIODevice::open(mode);
        if (ok) {
            if (mode & Append) {
                ok = seek(size());
            } else {
                qint64 pos = 0;
                TInt err;
#ifdef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
                err = static_cast<const RFile64&>(f).Seek(ESeekCurrent, pos);
#else
                TInt pos32 = 0;
                err = f.Seek(ESeekCurrent, pos32);
                pos = pos32;
#endif
                ok = ok && (err == KErrNone);
                ok = ok && seek(pos);
            }
        }
        return ok;
    }
    return false;
}
#endif

/*!
  Returns the file handle of the file.

  This is a small positive integer, suitable for use with C library
  functions such as fdopen() and fcntl(). On systems that use file
  descriptors for sockets (i.e. Unix systems, but not Windows) the handle
  can be used with QSocketNotifier as well.

  If the file is not open, or there is an error, handle() returns -1.

  This function is not supported on Windows CE.

  On Symbian, this function returns -1 if the file was opened normally,
  as Symbian OS native file handles do not fit in an int, and are
  incompatible with C library functions that the handle would be used for.
  If the file was opened using the overloads that take an open C library
  file handle / file descriptor, then this function returns that same
  handle.

  \sa QSocketNotifier
*/

int
QFile::handle() const
{
    Q_D(const QFile);
    if (!isOpen() || !d->fileEngine)
        return -1;

    return d->fileEngine->handle();
}

/*!
    \enum QFile::MemoryMapFlags
    \since 4.4

    This enum describes special options that may be used by the map()
    function.

    \value NoOptions        No options.
*/

/*!
    \since 4.4
    Maps \a size bytes of the file into memory starting at \a offset.  A file
    should be open for a map to succeed but the file does not need to stay
    open after the memory has been mapped.  When the QFile is destroyed
    or a new file is opened with this object, any maps that have not been
    unmapped will automatically be unmapped.

    Any mapping options can be passed through \a flags.

    Returns a pointer to the memory or 0 if there is an error.

    \note On Windows CE 5.0 the file will be closed before mapping occurs.

    \sa unmap(), QAbstractFileEngine::supportsExtension()
 */
uchar *QFile::map(qint64 offset, qint64 size, MemoryMapFlags flags)
{
    Q_D(QFile);
    if (fileEngine()
            && d->fileEngine->supportsExtension(QAbstractFileEngine::MapExtension)) {
        unsetError();
        uchar *address = d->fileEngine->map(offset, size, flags);
        if (address == 0)
            d->setError(d->fileEngine->error(), d->fileEngine->errorString());
        return address;
    }
    return 0;
}

/*!
    \since 4.4
    Unmaps the memory \a address.

    Returns true if the unmap succeeds; false otherwise.

    \sa map(), QAbstractFileEngine::supportsExtension()
 */
bool QFile::unmap(uchar *address)
{
    Q_D(QFile);
    if (fileEngine()
        && d->fileEngine->supportsExtension(QAbstractFileEngine::UnMapExtension)) {
        unsetError();
        bool success = d->fileEngine->unmap(address);
        if (!success)
            d->setError(d->fileEngine->error(), d->fileEngine->errorString());
        return success;
    }
    d->setError(PermissionsError, tr("No file engine available or engine does not support UnMapExtension"));
    return false;
}

/*!
    \fn QString QFile::name() const

    Use fileName() instead.
*/

/*!
    \fn void QFile::setName(const QString &name)

    Use setFileName() instead.
*/

/*!
    Sets the file size (in bytes) \a sz. Returns true if the file if the
    resize succeeds; false otherwise. If \a sz is larger than the file
    currently is the new bytes will be set to 0, if \a sz is smaller the
    file is simply truncated.

    \sa size(), setFileName()
*/

bool
QFile::resize(qint64 sz)
{
    Q_D(QFile);
    if (!d->ensureFlushed())
        return false;
    fileEngine();
    if (isOpen() && d->fileEngine->pos() > sz)
        seek(sz);
    if(d->fileEngine->setSize(sz)) {
        unsetError();
        d->cachedSize = sz;
        return true;
    }
    d->cachedSize = 0;
    d->setError(QFile::ResizeError, d->fileEngine->errorString());
    return false;
}

/*!
    \overload

    Sets \a fileName to size (in bytes) \a sz. Returns true if the file if
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
    Returns the complete OR-ed together combination of
    QFile::Permission for the file.

    \sa setPermissions(), setFileName()
*/

QFile::Permissions
QFile::permissions() const
{
    QAbstractFileEngine::FileFlags perms = fileEngine()->fileFlags(QAbstractFileEngine::PermsMask) & QAbstractFileEngine::PermsMask;
    return QFile::Permissions((int)perms); //ewww
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
    Returns true if successful, or false if the permissions cannot be
    modified.

    \sa permissions(), setFileName()
*/

bool
QFile::setPermissions(Permissions permissions)
{
    Q_D(QFile);
    if(fileEngine()->setPermissions(permissions)) {
        unsetError();
        return true;
    }
    d->setError(QFile::PermissionsError, d->fileEngine->errorString());
    return false;
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

static inline qint64 _qfile_writeData(QAbstractFileEngine *engine, QRingBuffer *buffer)
{
    qint64 ret = engine->write(buffer->readPointer(), buffer->nextDataBlockSize());
    if (ret > 0)
        buffer->free(ret);
    return ret;
}

/*!
    Flushes any buffered data to the file. Returns true if successful;
    otherwise returns false.
*/

bool
QFile::flush()
{
    Q_D(QFile);
    if (!d->fileEngine) {
        qWarning("QFile::flush: No file engine. Is IODevice open?");
        return false;
    }

    if (!d->writeBuffer.isEmpty()) {
        qint64 size = d->writeBuffer.size();
        if (_qfile_writeData(d->fileEngine, &d->writeBuffer) != size) {
            QFile::FileError err = d->fileEngine->error();
            if(err == QFile::UnspecifiedError)
                err = QFile::WriteError;
            d->setError(err, d->fileEngine->errorString());
            return false;
        }
    }

    if (!d->fileEngine->flush()) {
        QFile::FileError err = d->fileEngine->error();
        if(err == QFile::UnspecifiedError)
            err = QFile::WriteError;
        d->setError(err, d->fileEngine->errorString());
        return false;
    }
    return true;
}

/*!
  Calls QFile::flush() and closes the file. Errors from flush are ignored.

  \sa QIODevice::close()
*/
void
QFile::close()
{
    Q_D(QFile);
    if(!isOpen())
        return;
    bool flushed = flush();
    QIODevice::close();

    // reset write buffer
    d->lastWasWrite = false;
    d->writeBuffer.clear();

    // keep earlier error from flush
    if (d->fileEngine->close() && flushed)
        unsetError();
    else if (flushed)
        d->setError(d->fileEngine->error(), d->fileEngine->errorString());
}

/*!
  Returns the size of the file.

  For regular empty files on Unix (e.g. those in \c /proc), this function
  returns 0; the contents of such a file are generated on demand in response
  to you calling read().
*/

qint64 QFile::size() const
{
    Q_D(const QFile);
    if (!d->ensureFlushed())
        return 0;
    d->cachedSize = fileEngine()->size();
    return d->cachedSize;
}

/*!
  \reimp
*/

qint64 QFile::pos() const
{
    return QIODevice::pos();
}

/*!
  Returns true if the end of the file has been reached; otherwise returns
  false.

  For regular empty files on Unix (e.g. those in \c /proc), this function
  returns true, since the file system reports that the size of such a file is
  0. Therefore, you should not depend on atEnd() when reading data from such a
  file, but rather call read() until no more data can be read.
*/

bool QFile::atEnd() const
{
    Q_D(const QFile);

    // If there's buffered data left, we're not at the end.
    if (!d->buffer.isEmpty())
        return false;

    if (!isOpen())
        return true;

    if (!d->ensureFlushed())
        return false;

    // If the file engine knows best, say what it says.
    if (d->fileEngine->supportsExtension(QAbstractFileEngine::AtEndExtension)) {
        // Check if the file engine supports AtEndExtension, and if it does,
        // check if the file engine claims to be at the end.
        return d->fileEngine->atEnd();
    }

    // if it looks like we are at the end, or if size is not cached,
    // fall through to bytesAvailable() to make sure.
    if (pos() < d->cachedSize)
        return false;

    // Fall back to checking how much is available (will stat files).
    return bytesAvailable() == 0;
}

/*!
    \fn bool QFile::seek(qint64 pos)

    For random-access devices, this function sets the current position
    to \a pos, returning true on success, or false if an error occurred.
    For sequential devices, the default behavior is to do nothing and
    return false.

    Seeking beyond the end of a file:
    If the position is beyond the end of a file, then seek() shall not
    immediately extend the file. If a write is performed at this position,
    then the file shall be extended. The content of the file between the
    previous end of file and the newly written data is UNDEFINED and
    varies between platforms and file systems.
*/

bool QFile::seek(qint64 off)
{
    Q_D(QFile);
    if (!isOpen()) {
        qWarning("QFile::seek: IODevice is not open");
        return false;
    }

    if (off == d->pos && off == d->devicePos)
        return true; //avoid expensive flush for NOP seek to current position

    if (!d->ensureFlushed())
        return false;

    if (!d->fileEngine->seek(off) || !QIODevice::seek(off)) {
        QFile::FileError err = d->fileEngine->error();
        if(err == QFile::UnspecifiedError)
            err = QFile::PositionError;
        d->setError(err, d->fileEngine->errorString());
        return false;
    }
    unsetError();
    return true;
}

/*!
  \reimp
*/
qint64 QFile::readLineData(char *data, qint64 maxlen)
{
    Q_D(QFile);
    if (!d->ensureFlushed())
        return -1;

    qint64 read;
    if (d->fileEngine->supportsExtension(QAbstractFileEngine::FastReadLineExtension)) {
        read = d->fileEngine->readLine(data, maxlen);
    } else {
        // Fall back to QIODevice's readLine implementation if the engine
        // cannot do it faster.
        read = QIODevice::readLineData(data, maxlen);
    }

    if (read < maxlen) {
        // failed to read all requested, may be at the end of file, stop caching size so that it's rechecked
        d->cachedSize = 0;
    }

    return read;
}

/*!
  \reimp
*/

qint64 QFile::readData(char *data, qint64 len)
{
    Q_D(QFile);
    unsetError();
    if (!d->ensureFlushed())
        return -1;

    qint64 read = d->fileEngine->read(data, len);
    if(read < 0) {
        QFile::FileError err = d->fileEngine->error();
        if(err == QFile::UnspecifiedError)
            err = QFile::ReadError;
        d->setError(err, d->fileEngine->errorString());
    }

    if (read < len) {
        // failed to read all requested, may be at the end of file, stop caching size so that it's rechecked
        d->cachedSize = 0;
    }

    return read;
}

/*!
    \internal
*/
bool QFilePrivate::putCharHelper(char c)
{
#ifdef QT_NO_QOBJECT
    return QIODevicePrivate::putCharHelper(c);
#else

    // Cutoff for code that doesn't only touch the buffer.
    int writeBufferSize = writeBuffer.size();
    if ((openMode & QIODevice::Unbuffered) || writeBufferSize + 1 >= QFILE_WRITEBUFFER_SIZE
#ifdef Q_OS_WIN
        || ((openMode & QIODevice::Text) && c == '\n' && writeBufferSize + 2 >= QFILE_WRITEBUFFER_SIZE)
#endif
        ) {
        return QIODevicePrivate::putCharHelper(c);
    }

    if (!(openMode & QIODevice::WriteOnly)) {
        if (openMode == QIODevice::NotOpen)
            qWarning("QIODevice::putChar: Closed device");
        else
            qWarning("QIODevice::putChar: ReadOnly device");
        return false;
    }

    // Make sure the device is positioned correctly.
    const bool sequential = isSequential();
    if (pos != devicePos && !sequential && !q_func()->seek(pos))
        return false;

    lastWasWrite = true;

    int len = 1;
#ifdef Q_OS_WIN
    if ((openMode & QIODevice::Text) && c == '\n') {
        ++len;
        *writeBuffer.reserve(1) = '\r';
    }
#endif

    // Write to buffer.
    *writeBuffer.reserve(1) = c;

    if (!sequential) {
        pos += len;
        devicePos += len;
        if (!buffer.isEmpty())
            buffer.skip(len);
    }

    return true;
#endif
}

/*!
  \reimp
*/

qint64
QFile::writeData(const char *data, qint64 len)
{
    Q_D(QFile);
    unsetError();
    d->lastWasWrite = true;
    bool buffered = !(d->openMode & Unbuffered);

    // Flush buffered data if this read will overflow.
    if (buffered && (d->writeBuffer.size() + len) > QFILE_WRITEBUFFER_SIZE) {
        if (!flush())
            return -1;
    }

    // Write directly to the engine if the block size is larger than
    // the write buffer size.
    if (!buffered || len > QFILE_WRITEBUFFER_SIZE) {
        qint64 ret = d->fileEngine->write(data, len);
        if(ret < 0) {
            QFile::FileError err = d->fileEngine->error();
            if(err == QFile::UnspecifiedError)
                err = QFile::WriteError;
            d->setError(err, d->fileEngine->errorString());
        }
        return ret;
    }

    // Write to the buffer.
    char *writePointer = d->writeBuffer.reserve(len);
    if (len == 1)
        *writePointer = *data;
    else
        ::memcpy(writePointer, data, len);
    return len;
}

/*!
    \internal
    Returns the QIOEngine for this QFile object.
*/
QAbstractFileEngine *QFile::fileEngine() const
{
    Q_D(const QFile);
    if(!d->fileEngine)
        d->fileEngine = QAbstractFileEngine::create(d->fileName);
    return d->fileEngine;
}

/*!
    Returns the file error status.

    The I/O device status returns an error code. For example, if open()
    returns false, or a read/write operation returns -1, this function can
    be called to find out the reason why the operation failed.

    \sa unsetError()
*/

QFile::FileError
QFile::error() const
{
    Q_D(const QFile);
    return d->error;
}

/*!
    Sets the file's error to QFile::NoError.

    \sa error()
*/
void
QFile::unsetError()
{
    Q_D(QFile);
    d->setError(QFile::NoError);
}

QT_END_NAMESPACE
