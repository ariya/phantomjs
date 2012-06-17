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

#include "qsharedmemory.h"
#include "qsharedmemory_p.h"
#include "qsystemsemaphore.h"
#include <qdir.h>
#include <qcryptographichash.h>
#ifdef Q_OS_SYMBIAN
#include <e32const.h>
#endif
#include <qdebug.h>

QT_BEGIN_NAMESPACE

#if !(defined(QT_NO_SHAREDMEMORY) && defined(QT_NO_SYSTEMSEMAPHORE))
/*!
    \internal

    Generate a string from the key which can be any unicode string into
    the subset that the win/unix kernel allows.

    On Unix this will be a file name
    On Symbian key will be truncated to 80 characters
  */
QString
QSharedMemoryPrivate::makePlatformSafeKey(const QString &key,
					  const QString &prefix)
{
    if (key.isEmpty())
        return QString();

    QString result = prefix;

    QString part1 = key;
    part1.replace(QRegExp(QLatin1String("[^A-Za-z]")), QString());
    result.append(part1);

    QByteArray hex = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha1).toHex();
    result.append(QLatin1String(hex));
#ifdef Q_OS_WIN
    return result;
#elif defined(Q_OS_SYMBIAN)
    return result.left(KMaxKernelName);
#elif defined(QT_POSIX_IPC)
    return QLatin1Char('/') + result;
#else
    return QDir::tempPath() + QLatin1Char('/') + result;
#endif
}
#endif // QT_NO_SHAREDMEMORY && QT_NO_SHAREDMEMORY

#ifndef QT_NO_SHAREDMEMORY

/*!
  \class QSharedMemory
  \since 4.4

  \brief The QSharedMemory class provides access to a shared memory segment.

  QSharedMemory provides access to a shared memory segment by multiple
  threads and processes. It also provides a way for a single thread or
  process to lock the memory for exclusive access.

  When using this class, be aware of the following platform
  differences:

  \list

  \o Windows: QSharedMemory does not "own" the shared memory segment.
  When all threads or processes that have an instance of QSharedMemory
  attached to a particular shared memory segment have either destroyed
  their instance of QSharedMemory or exited, the Windows kernel
  releases the shared memory segment automatically.

  \o Unix: QSharedMemory "owns" the shared memory segment. When the
  last thread or process that has an instance of QSharedMemory
  attached to a particular shared memory segment detaches from the
  segment by destroying its instance of QSharedMemory, the Unix kernel
  release the shared memory segment. But if that last thread or
  process crashes without running the QSharedMemory destructor, the
  shared memory segment survives the crash.

  \o QNX: Due to possible race conditions in the POSIX IPC implementation, create()
  should be called prior to any attach() calls (even across multiple threads).

  \o HP-UX: Only one attach to a shared memory segment is allowed per
  process. This means that QSharedMemory should not be used across
  multiple threads in the same process in HP-UX.

  \o Symbian: QSharedMemory does not "own" the shared memory segment.
  When all threads or processes that have an instance of QSharedMemory
  attached to a particular shared memory segment have either destroyed
  their instance of QSharedMemory or exited, the Symbian kernel
  releases the shared memory segment automatically.
  Also, access to a shared memory segment cannot be limited to read-only
  in Symbian.

  \endlist

  Remember to lock the shared memory with lock() before reading from
  or writing to the shared memory, and remember to release the lock
  with unlock() after you are done.

  Unlike QtSharedMemory, QSharedMemory automatically destroys the
  shared memory segment when the last instance of QSharedMemory is
  detached from the segment, and no references to the segment
  remain. Do not mix using QtSharedMemory and QSharedMemory. Port
  everything to QSharedMemory.

  \warning QSharedMemory changes the key in a Qt-specific way, unless otherwise
  specified. Interoperation with non-Qt applications is achieved by first creating
  a default shared memory with QSharedMemory() and then setting a native key with
  setNativeKey(). When using native keys, shared memory is not protected against
  multiple accesses on it (e.g. unable to lock()) and a user-defined mechanism
  should be used to achieve a such protection.
 */

/*!
  \overload QSharedMemory()

  Constructs a shared memory object with the given \a parent.  The
  shared memory object's key is not set by the constructor, so the
  shared memory object does not have an underlying shared memory
  segment attached. The key must be set with setKey() or setNativeKey()
  before create() or attach() can be used.

  \sa setKey()
 */
QSharedMemory::QSharedMemory(QObject *parent)
  : QObject(*new QSharedMemoryPrivate, parent)
{
}

/*!
  Constructs a shared memory object with the given \a parent and with
  its key set to \a key. Because its key is set, its create() and
  attach() functions can be called.

  \sa setKey(), create(), attach()
 */
QSharedMemory::QSharedMemory(const QString &key, QObject *parent)
    : QObject(*new QSharedMemoryPrivate, parent)
{
    setKey(key);
}

/*!
  The destructor clears the key, which forces the shared memory object
  to \l {detach()} {detach} from its underlying shared memory
  segment. If this shared memory object is the last one connected to
  the shared memory segment, the detach() operation destroys the
  shared memory segment.

  \sa detach() isAttached()
 */
QSharedMemory::~QSharedMemory()
{
    setKey(QString());
}

/*!
  Sets the platform independent \a key for this shared memory object. If \a key
  is the same as the current key, the function returns without doing anything.

  You can call key() to retrieve the platform independent key. Internally,
  QSharedMemory converts this key into a platform specific key. If you instead
  call nativeKey(), you will get the platform specific, converted key.

  If the shared memory object is attached to an underlying shared memory
  segment, it will \l {detach()} {detach} from it before setting the new key.
  This function does not do an attach().

  \sa key() nativeKey() isAttached()
*/
void QSharedMemory::setKey(const QString &key)
{
    Q_D(QSharedMemory);
    if (key == d->key && d->makePlatformSafeKey(key) == d->nativeKey)
        return;

    if (isAttached())
        detach();
    d->cleanHandle();
    d->key = key;
    d->nativeKey = d->makePlatformSafeKey(key);
}

/*!
  \since 4.8

  Sets the native, platform specific, \a key for this shared memory object. If
  \a key is the same as the current native key, the function returns without
  doing anything. If all you want is to assign a key to a segment, you should
  call setKey() instead.

  You can call nativeKey() to retrieve the native key. If a native key has been
  assigned, calling key() will return a null string.

  If the shared memory object is attached to an underlying shared memory
  segment, it will \l {detach()} {detach} from it before setting the new key.
  This function does not do an attach().

  The application will not be portable if you set a native key.

  \sa nativeKey() key() isAttached()
*/
void QSharedMemory::setNativeKey(const QString &key)
{
    Q_D(QSharedMemory);
    if (key == d->nativeKey && d->key.isNull())
        return;

    if (isAttached())
        detach();
    d->cleanHandle();
    d->key.clear();
    d->nativeKey = key;
}

bool QSharedMemoryPrivate::initKey()
{
    cleanHandle();

#ifndef QT_NO_SYSTEMSEMAPHORE
    systemSemaphore.setKey(QString(), 1);
    systemSemaphore.setKey(key, 1);
    if (systemSemaphore.error() != QSystemSemaphore::NoError) {
        QString function = QLatin1String("QSharedMemoryPrivate::initKey");
        errorString = QSharedMemory::tr("%1: unable to set key on lock").arg(function);
        switch(systemSemaphore.error()) {
        case QSystemSemaphore::PermissionDenied:
            error = QSharedMemory::PermissionDenied;
            break;
        case QSystemSemaphore::KeyError:
            error = QSharedMemory::KeyError;
            break;
        case QSystemSemaphore::AlreadyExists:
            error = QSharedMemory::AlreadyExists;
            break;
        case QSystemSemaphore::NotFound:
            error = QSharedMemory::NotFound;
            break;
        case QSystemSemaphore::OutOfResources:
            error = QSharedMemory::OutOfResources;
            break;
        case QSystemSemaphore::UnknownError:
        default:
            error = QSharedMemory::UnknownError;
            break;
        }
        return false;
    }
#endif
    errorString.clear();
    error = QSharedMemory::NoError;
    return true;
}

/*!
  Returns the key assigned with setKey() to this shared memory, or a null key
  if no key has been assigned, or if the segment is using a nativeKey(). The
  key is the identifier used by Qt applications to identify the shared memory
  segment.

  You can find the native, platform specific, key used by the operating system
  by calling nativeKey().

  \sa setKey() setNativeKey()
 */
QString QSharedMemory::key() const
{
    Q_D(const QSharedMemory);
    return d->key;
}

/*!
  \since 4.8

  Returns the native, platform specific, key for this shared memory object. The
  native key is the identifier used by the operating system to identify the
  shared memory segment.

  You can use the native key to access shared memory segments that have not
  been created by Qt, or to grant shared memory access to non-Qt applications.

  \sa setKey() setNativeKey()
*/
QString QSharedMemory::nativeKey() const
{
    Q_D(const QSharedMemory);
    return d->nativeKey;
}

/*!
  Creates a shared memory segment of \a size bytes with the key passed to the
  constructor, set with setKey() or set with setNativeKey(), then attaches to
  the new shared memory segment with the given access \a mode and returns
  \tt true. If a shared memory segment identified by the key already exists,
  the attach operation is not performed and \tt false is returned. When the
  return value is \tt false, call error() to determine which error occurred.

  \sa error()
 */
bool QSharedMemory::create(int size, AccessMode mode)
{
    Q_D(QSharedMemory);

    if (!d->initKey())
        return false;

    if (size <= 0) {
        d->error = QSharedMemory::InvalidSize;
        d->errorString = QSharedMemory::tr("%1: create size is less then 0").arg(QLatin1String("QSharedMemory::create"));
        return false;
    }

#ifndef QT_NO_SYSTEMSEMAPHORE
#ifndef Q_OS_WIN
    // Take ownership and force set initialValue because the semaphore
    // might have already existed from a previous crash.
    d->systemSemaphore.setKey(d->key, 1, QSystemSemaphore::Create);
#endif

    QSharedMemoryLocker lock(this);
    if (!d->key.isNull() && !d->tryLocker(&lock, QLatin1String("QSharedMemory::create")))
        return false;
#endif

    if (!d->create(size))
        return false;

    return d->attach(mode);
}

/*!
  Returns the size of the attached shared memory segment. If no shared
  memory segment is attached, 0 is returned.

  \sa create() attach()
 */
int QSharedMemory::size() const
{
    Q_D(const QSharedMemory);
    return d->size;
}

/*!
  \enum QSharedMemory::AccessMode

  \value ReadOnly The shared memory segment is read-only. Writing to
  the shared memory segment is not allowed. An attempt to write to a
  shared memory segment created with ReadOnly causes the program to
  abort.

  \value ReadWrite Reading and writing the shared memory segment are
  both allowed.
*/

/*!
  Attempts to attach the process to the shared memory segment
  identified by the key that was passed to the constructor or to a
  call to setKey() or setNativeKey(). The access \a mode is \l {QSharedMemory::}
  {ReadWrite} by default. It can also be \l {QSharedMemory::}
  {ReadOnly}. Returns true if the attach operation is successful. If
  false is returned, call error() to determine which error occurred.
  After attaching the shared memory segment, a pointer to the shared
  memory can be obtained by calling data().

  \sa isAttached(), detach(), create()
 */
bool QSharedMemory::attach(AccessMode mode)
{
    Q_D(QSharedMemory);

    if (isAttached() || !d->initKey())
        return false;
#ifndef QT_NO_SYSTEMSEMAPHORE
    QSharedMemoryLocker lock(this);
    if (!d->key.isNull() && !d->tryLocker(&lock, QLatin1String("QSharedMemory::attach")))
        return false;
#endif

    if (isAttached() || !d->handle())
        return false;

    return d->attach(mode);
}

/*!
  Returns true if this process is attached to the shared memory
  segment.

  \sa attach(), detach()
 */
bool QSharedMemory::isAttached() const
{
    Q_D(const QSharedMemory);
    return (0 != d->memory);
}

/*!
  Detaches the process from the shared memory segment. If this was the
  last process attached to the shared memory segment, then the shared
  memory segment is released by the system, i.e., the contents are
  destroyed. The function returns true if it detaches the shared
  memory segment. If it returns false, it usually means the segment
  either isn't attached, or it is locked by another process.

  \sa attach(), isAttached()
 */
bool QSharedMemory::detach()
{
    Q_D(QSharedMemory);
    if (!isAttached())
        return false;

#ifndef QT_NO_SYSTEMSEMAPHORE
    QSharedMemoryLocker lock(this);
    if (!d->key.isNull() && !d->tryLocker(&lock, QLatin1String("QSharedMemory::detach")))
        return false;
#endif

    return d->detach();
}

/*!
  Returns a pointer to the contents of the shared memory segment, if
  one is attached. Otherwise it returns null. Remember to lock the
  shared memory with lock() before reading from or writing to the
  shared memory, and remember to release the lock with unlock() after
  you are done.

  \sa attach()
 */
void *QSharedMemory::data()
{
    Q_D(QSharedMemory);
    return d->memory;
}

/*!
  Returns a const pointer to the contents of the shared memory
  segment, if one is attached. Otherwise it returns null. Remember to
  lock the shared memory with lock() before reading from or writing to
  the shared memory, and remember to release the lock with unlock()
  after you are done.

  \sa attach() create()
 */
const void* QSharedMemory::constData() const
{
    Q_D(const QSharedMemory);
    return d->memory;
}

/*!
  \overload data()
 */
const void *QSharedMemory::data() const
{
    Q_D(const QSharedMemory);
    return d->memory;
}

#ifndef QT_NO_SYSTEMSEMAPHORE
/*!
  This is a semaphore that locks the shared memory segment for access
  by this process and returns true. If another process has locked the
  segment, this function blocks until the lock is released. Then it
  acquires the lock and returns true. If this function returns false,
  it means that you have ignored a false return from create() or attach(),
  that you have set the key with setNativeKey() or that
  QSystemSemaphore::acquire() failed due to an unknown system error.

  \sa unlock(), data(), QSystemSemaphore::acquire()
 */
bool QSharedMemory::lock()
{
    Q_D(QSharedMemory);
    if (d->lockedByMe) {
        qWarning("QSharedMemory::lock: already locked");
        return true;
    }
    if (d->systemSemaphore.acquire()) {
        d->lockedByMe = true;
        return true;
    }
    QString function = QLatin1String("QSharedMemory::lock");
    d->errorString = QSharedMemory::tr("%1: unable to lock").arg(function);
    d->error = QSharedMemory::LockError;
    return false;
}

/*!
  Releases the lock on the shared memory segment and returns true, if
  the lock is currently held by this process. If the segment is not
  locked, or if the lock is held by another process, nothing happens
  and false is returned.

  \sa lock()
 */
bool QSharedMemory::unlock()
{
    Q_D(QSharedMemory);
    if (!d->lockedByMe)
        return false;
    d->lockedByMe = false;
    if (d->systemSemaphore.release())
        return true;
    QString function = QLatin1String("QSharedMemory::unlock");
    d->errorString = QSharedMemory::tr("%1: unable to unlock").arg(function);
    d->error = QSharedMemory::LockError;
    return false;
}
#endif // QT_NO_SYSTEMSEMAPHORE

/*!
  \enum QSharedMemory::SharedMemoryError

  \value NoError No error occurred.

  \value PermissionDenied The operation failed because the caller
  didn't have the required permissions.

  \value InvalidSize A create operation failed because the requested
  size was invalid.

  \value KeyError The operation failed because of an invalid key.

  \value AlreadyExists A create() operation failed because a shared
  memory segment with the specified key already existed.

  \value NotFound An attach() failed because a shared memory segment
  with the specified key could not be found.

  \value LockError The attempt to lock() the shared memory segment
  failed because create() or attach() failed and returned false, or
  because a system error occurred in QSystemSemaphore::acquire().

  \value OutOfResources A create() operation failed because there was
  not enough memory available to fill the request.

  \value UnknownError Something else happened and it was bad.
*/

/*!
  Returns a value indicating whether an error occurred, and, if so,
  which error it was.

  \sa errorString()
 */
QSharedMemory::SharedMemoryError QSharedMemory::error() const
{
    Q_D(const QSharedMemory);
    return d->error;
}

/*!
  Returns a text description of the last error that occurred. If
  error() returns an \l {QSharedMemory::SharedMemoryError} {error
  value}, call this function to get a text string that describes the
  error.

  \sa error()
 */
QString QSharedMemory::errorString() const
{
    Q_D(const QSharedMemory);
    return d->errorString;
}

#endif // QT_NO_SHAREDMEMORY

QT_END_NAMESPACE
