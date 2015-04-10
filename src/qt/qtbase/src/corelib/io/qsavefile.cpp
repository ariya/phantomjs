/****************************************************************************
**
** Copyright (C) 2012 David Faure <faure@kde.org>
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

#include "qsavefile.h"

#ifndef QT_NO_TEMPORARYFILE

#include "qplatformdefs.h"
#include "private/qsavefile_p.h"
#include "qfileinfo.h"
#include "qabstractfileengine_p.h"
#include "qdebug.h"
#include "qtemporaryfile.h"
#include "private/qiodevice_p.h"
#include "private/qtemporaryfile_p.h"
#ifdef Q_OS_UNIX
#include <errno.h>
#endif

QT_BEGIN_NAMESPACE

QSaveFilePrivate::QSaveFilePrivate()
    : writeError(QFileDevice::NoError),
      useTemporaryFile(true),
      directWriteFallback(false)
{
}

QSaveFilePrivate::~QSaveFilePrivate()
{
}

/*!
    \class QSaveFile
    \inmodule QtCore
    \brief The QSaveFile class provides an interface for safely writing to files.

    \ingroup io

    \reentrant

    \since 5.1

    QSaveFile is an I/O device for writing text and binary files, without losing
    existing data if the writing operation fails.

    While writing, the contents will be written to a temporary file, and if
    no error happened, commit() will move it to the final file. This ensures that
    no data at the final file is lost in case an error happens while writing,
    and no partially-written file is ever present at the final location. Always
    use QSaveFile when saving entire documents to disk.

    QSaveFile automatically detects errors while writing, such as the full partition
    situation, where write() cannot write all the bytes. It will remember that
    an error happened, and will discard the temporary file in commit().

    Much like with QFile, the file is opened with open(). Data is usually read
    and written using QDataStream or QTextStream, but you can also call the
    QIODevice-inherited functions read(), readLine(), readAll(), write().

    Unlike QFile, calling close() is not allowed. commit() replaces it. If commit()
    was not called and the QSaveFile instance is destroyed, the temporary file is
    discarded.

    To abort saving due to an application error, call cancelWriting(), so that
    even a call to commit() later on will not save.

    \sa QTextStream, QDataStream, QFileInfo, QDir, QFile, QTemporaryFile
*/

/*!
    Constructs a new file object with the given \a parent.
*/
QSaveFile::QSaveFile(QObject *parent)
    : QFileDevice(*new QSaveFilePrivate, parent)
{
}
/*!
    Constructs a new file object to represent the file with the given \a name.
*/
QSaveFile::QSaveFile(const QString &name)
    : QFileDevice(*new QSaveFilePrivate, 0)
{
    Q_D(QSaveFile);
    d->fileName = name;
}
/*!
    Constructs a new file object with the given \a parent to represent the
    file with the specified \a name.
*/
QSaveFile::QSaveFile(const QString &name, QObject *parent)
    : QFileDevice(*new QSaveFilePrivate, parent)
{
    Q_D(QSaveFile);
    d->fileName = name;
}

/*!
    Destroys the file object, discarding the saved contents unless commit() was called.
*/
QSaveFile::~QSaveFile()
{
    Q_D(QSaveFile);
    QFileDevice::close();
    if (d->fileEngine) {
        d->fileEngine->remove();
        delete d->fileEngine;
        d->fileEngine = 0;
    }
}

/*!
    Returns the name set by setFileName() or to the QSaveFile
    constructor.

    \sa setFileName()
*/
QString QSaveFile::fileName() const
{
    return d_func()->fileName;
}

/*!
    Sets the \a name of the file. The name can have no path, a
    relative path, or an absolute path.

    \sa QFile::setFileName(), fileName()
*/
void QSaveFile::setFileName(const QString &name)
{
    d_func()->fileName = name;
}

/*!
    Opens the file using OpenMode \a mode, returning true if successful;
    otherwise false.

    Important: the \a mode must include QIODevice::WriteOnly.
    It may also have additional flags, such as QIODevice::Text and QIODevice::Unbuffered.

    QIODevice::ReadWrite and QIODevice::Append are not supported at the moment.

    \sa QIODevice::OpenMode, setFileName()
*/
bool QSaveFile::open(OpenMode mode)
{
    Q_D(QSaveFile);
    if (isOpen()) {
        qWarning("QSaveFile::open: File (%s) already open", qPrintable(fileName()));
        return false;
    }
    unsetError();
    if ((mode & (ReadOnly | WriteOnly)) == 0) {
        qWarning("QSaveFile::open: Open mode not specified");
        return false;
    }
    // In the future we could implement ReadWrite by copying from the existing file to the temp file...
    if ((mode & ReadOnly) || (mode & Append)) {
        qWarning("QSaveFile::open: Unsupported open mode 0x%x", int(mode));
        return false;
    }

    // check if existing file is writable
    QFileInfo existingFile(d->fileName);
    if (existingFile.exists() && !existingFile.isWritable()) {
        d->setError(QFileDevice::WriteError, QSaveFile::tr("Existing file %1 is not writable").arg(d->fileName));
        d->writeError = QFileDevice::WriteError;
        return false;
    }
    d->fileEngine = new QTemporaryFileEngine(d->fileName);
    // Same as in QFile: QIODevice provides the buffering, so there's no need to request it from the file engine.
    if (!d->fileEngine->open(mode | QIODevice::Unbuffered)) {
        QFileDevice::FileError err = d->fileEngine->error();
#ifdef Q_OS_UNIX
        if (d->directWriteFallback && err == QFileDevice::OpenError && errno == EACCES) {
            delete d->fileEngine;
            d->fileEngine = QAbstractFileEngine::create(d->fileName);
            if (d->fileEngine->open(mode | QIODevice::Unbuffered)) {
                d->useTemporaryFile = false;
                QFileDevice::open(mode);
                return true;
            }
            err = d->fileEngine->error();
        }
#endif
        if (err == QFileDevice::UnspecifiedError)
            err = QFileDevice::OpenError;
        d->setError(err, d->fileEngine->errorString());
        delete d->fileEngine;
        d->fileEngine = 0;
        return false;
    }

    d->useTemporaryFile = true;
    QFileDevice::open(mode);
    if (existingFile.exists())
        setPermissions(existingFile.permissions());
    return true;
}

/*!
  \reimp
  This method has been made private so that it cannot be called, in order to prevent mistakes.
  In order to finish writing the file, call commit().
  If instead you want to abort writing, call cancelWriting().
*/
void QSaveFile::close()
{
    qFatal("QSaveFile::close called");
}

/*!
  Commits the changes to disk, if all previous writes were successful.

  It is mandatory to call this at the end of the saving operation, otherwise the file will be
  discarded.

  If an error happened during writing, deletes the temporary file and returns \c false.
  Otherwise, renames it to the final fileName and returns \c true on success.
  Finally, closes the device.

  \sa cancelWriting()
*/
bool QSaveFile::commit()
{
    Q_D(QSaveFile);
    if (!d->fileEngine)
        return false;

    if (!isOpen()) {
        qWarning("QSaveFile::commit: File (%s) is not open", qPrintable(fileName()));
        return false;
    }
    QFileDevice::close(); // calls flush()

    // Sync to disk if possible. Ignore errors (e.g. not supported).
    d->fileEngine->syncToDisk();

    if (d->useTemporaryFile) {
        if (d->writeError != QFileDevice::NoError) {
            d->fileEngine->remove();
            d->writeError = QFileDevice::NoError;
            delete d->fileEngine;
            d->fileEngine = 0;
            return false;
        }
        // atomically replace old file with new file
        // Can't use QFile::rename for that, must use the file engine directly
        Q_ASSERT(d->fileEngine);
        if (!d->fileEngine->renameOverwrite(d->fileName)) {
            d->setError(d->fileEngine->error(), d->fileEngine->errorString());
            d->fileEngine->remove();
            delete d->fileEngine;
            d->fileEngine = 0;
            return false;
        }
    }
    delete d->fileEngine;
    d->fileEngine = 0;
    return true;
}

/*!
  Cancels writing the new file.

  If the application changes its mind while saving, it can call cancelWriting(),
  which sets an error code so that commit() will discard the temporary file.

  Alternatively, it can simply make sure not to call commit().

  Further write operations are possible after calling this method, but none
  of it will have any effect, the written file will be discarded.

  This method has no effect when direct write fallback is used. This is the case
  when saving over an existing file in a readonly directory: no temporary file can
  be created, so the existing file is overwritten no matter what, and cancelWriting()
  cannot do anything about that, the contents of the existing file will be lost.

  \sa commit()
*/
void QSaveFile::cancelWriting()
{
    Q_D(QSaveFile);
    if (!isOpen())
        return;
    d->setError(QFileDevice::WriteError, QSaveFile::tr("Writing canceled by application"));
    d->writeError = QFileDevice::WriteError;
}

/*!
  \reimp
*/
qint64 QSaveFile::writeData(const char *data, qint64 len)
{
    Q_D(QSaveFile);
    if (d->writeError != QFileDevice::NoError)
        return -1;

    const qint64 ret = QFileDevice::writeData(data, len);

    if (d->error != QFileDevice::NoError)
        d->writeError = d->error;
    return ret;
}

/*!
  Allows writing over the existing file if necessary.

  QSaveFile creates a temporary file in the same directory as the final
  file and atomically renames it. However this is not possible if the
  directory permissions do not allow creating new files.
  In order to preserve atomicity guarantees, open() fails when it
  cannot create the temporary file.

  In order to allow users to edit files with write permissions in a
  directory with restricted permissions, call setDirectWriteFallback() with
  \a enabled set to true, and the following calls to open() will fallback to
  opening the existing file directly and writing into it, without the use of
  a temporary file.
  This does not have atomicity guarantees, i.e. an application crash or
  for instance a power failure could lead to a partially-written file on disk.
  It also means cancelWriting() has no effect, in such a case.

  Typically, to save documents edited by the user, call setDirectWriteFallback(true),
  and to save application internal files (configuration files, data files, ...), keep
  the default setting which ensures atomicity.

  \sa directWriteFallback()
*/
void QSaveFile::setDirectWriteFallback(bool enabled)
{
    Q_D(QSaveFile);
    d->directWriteFallback = enabled;
}

/*!
  Returns \c true if the fallback solution for saving files in read-only
  directories is enabled.

  \sa setDirectWriteFallback()
*/
bool QSaveFile::directWriteFallback() const
{
    Q_D(const QSaveFile);
    return d->directWriteFallback;
}

QT_END_NAMESPACE

#endif // QT_NO_TEMPORARYFILE
