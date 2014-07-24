/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtDBus module of the Qt Toolkit.
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


#include "qdbusunixfiledescriptor.h"

#ifdef Q_OS_UNIX
# include <private/qcore_unix_p.h>
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DBUS

/*!
    \class QDBusUnixFileDescriptor
    \inmodule QtDBus
    \ingroup shared
    \since 4.8

    \brief The QDBusUnixFileDescriptor class holds one Unix file descriptor.

    The QDBusUnixFileDescriptor class is used to hold one Unix file
    descriptor for use with the Qt D-Bus module. This allows applications to
    send and receive Unix file descriptors over the D-Bus connection, mapping
    automatically to the D-Bus type 'h'.

    Objects of type QDBusUnixFileDescriptors can be used also as parameters
    in signals and slots that get exported to D-Bus by registering with
    QDBusConnection::registerObject.

    QDBusUnixFileDescriptor does not take ownership of the file descriptor.
    Instead, it will use the Unix system call \c dup(2) to make a copy of the
    file descriptor. This file descriptor belongs to the
    QDBusUnixFileDescriptor object and should not be stored or closed by the
    user. Instead, you should make your own copy if you need that.

    \section2 Availability

    Unix file descriptor passing is not available in all D-Bus connections.
    This feature is present with D-Bus library and bus daemon version 1.4 and
    upwards on Unix systems. Qt D-Bus automatically enables the feature if such
    a version was found at compile-time and run-time.

    To verify that your connection does support passing file descriptors,
    check if the QDBusConnection::UnixFileDescriptorPassing capability is set
    with QDBusConnection::connectionCapabilities(). If the flag is not
    active, then you will not be able to make calls to methods that have
    QDBusUnixFileDescriptor as arguments or even embed such a type in a
    variant. You will also not receive calls containing that type.

    Note also that remote applications may not have support for Unix file
    descriptor passing. If you make a D-Bus to a remote application that
    cannot receive such a type, you will receive an error reply. If you try
    to send a signal containing a D-Bus file descriptor or return one from a
    method call, the message will be silently dropped.

    Even if the feature is not available, QDBusUnixFileDescriptor will
    continue to operate, so code need not have compile-time checks for the
    availability of this feature.

    On non-Unix systems, QDBusUnixFileDescriptor will always report an
    invalid state and QDBusUnixFileDescriptor::isSupported() will return
    false.

    \sa QDBusConnection::ConnectionCapabilities, QDBusConnection::connectionCapabilities()
*/

/*!
    \typedef QDBusUnixFileDescriptor::Data
    \internal
*/

/*!
    \variable QDBusUnixFileDescriptor::d
    \internal
*/

class QDBusUnixFileDescriptorPrivate : public QSharedData {
public:
    QDBusUnixFileDescriptorPrivate() : fd(-1) { }
    QDBusUnixFileDescriptorPrivate(const QDBusUnixFileDescriptorPrivate &other)
        : QSharedData(other), fd(-1)
    {  }
    ~QDBusUnixFileDescriptorPrivate();

    QAtomicInt fd;
};

template<> inline
QExplicitlySharedDataPointer<QDBusUnixFileDescriptorPrivate>::~QExplicitlySharedDataPointer()
{ if (d && !d->ref.deref()) delete d; }

/*!
    Constructs a QDBusUnixFileDescriptor without a wrapped file descriptor.
    This is equivalent to constructing the object with an invalid file
    descriptor (like -1).

    \sa fileDescriptor(), isValid()
*/
QDBusUnixFileDescriptor::QDBusUnixFileDescriptor()
    : d(0)
{
}

/*!
    Constructs a QDBusUnixFileDescriptor object by copying the \a
    fileDescriptor parameter. The original file descriptor is not touched and
    must be closed by the user.

    Note that the value returned by fileDescriptor() will be different from
    the \a fileDescriptor parameter passed.

    If the \a fileDescriptor parameter is not valid, isValid() will return
    false and fileDescriptor() will return -1.

    \sa setFileDescriptor(), fileDescriptor()
*/
QDBusUnixFileDescriptor::QDBusUnixFileDescriptor(int fileDescriptor)
    : d(0)
{
    if (fileDescriptor != -1)
        setFileDescriptor(fileDescriptor);
}

/*!
    Constructs a QDBusUnixFileDescriptor object by copying \a other.
*/
QDBusUnixFileDescriptor::QDBusUnixFileDescriptor(const QDBusUnixFileDescriptor &other)
    : d(other.d)
{
}

/*!
    Copies the Unix file descriptor from the \a other QDBusUnixFileDescriptor
    object. If the current object contained a file descriptor, it will be
    properly disposed of before.
*/
QDBusUnixFileDescriptor &QDBusUnixFileDescriptor::operator=(const QDBusUnixFileDescriptor &other)
{
    if (this != &other)
        d.operator=(other.d);
    return *this;
}

/*!
    Destroys this QDBusUnixFileDescriptor object and disposes of the Unix file descriptor that it contained.
*/
QDBusUnixFileDescriptor::~QDBusUnixFileDescriptor()
{
}

/*!
    \fn void QDBusUnixFileDescriptor::swap(QDBusUnixFileDescriptor &other)
    \since 5.0

    Swaps this file descriptor instance with \a other. This function
    is very fast and never fails.
*/

/*!
    Returns \c true if this Unix file descriptor is valid. A valid Unix file
    descriptor is not -1.

    \sa fileDescriptor()
*/
bool QDBusUnixFileDescriptor::isValid() const
{
    return d ? d->fd.load() != -1 : false;
}

/*!
    Returns the Unix file descriptor contained by this
    QDBusUnixFileDescriptor object. An invalid file descriptor is represented
    by the value -1.

    Note that the file descriptor returned by this function is owned by the
    QDBusUnixFileDescriptor object and must not be stored past the lifetime
    of this object. It is ok to use it while this object is valid, but if one
    wants to store it for longer use, the file descriptor should be cloned
    using the Unix \c dup(2), \c dup2(2) or \c dup3(2) functions.

    \sa isValid()
*/
int QDBusUnixFileDescriptor::fileDescriptor() const
{
    return d ? d->fd.load() : -1;
}

// actual implementation
#ifdef Q_OS_UNIX

// qdoc documentation is generated on Unix

/*!
    Returns \c true if Unix file descriptors are supported on this platform. In
    other words, this function returns \c true if this is a Unix platform.

    Note that QDBusUnixFileDescriptor continues to operate even if this
    function returns \c false. The only difference is that the
    QDBusUnixFileDescriptor objects will always be in the isValid() == false
    state and fileDescriptor() will always return -1. The class will not
    consume any operating system resources.
*/
bool QDBusUnixFileDescriptor::isSupported()
{
    return true;
}

/*!
    Sets the file descriptor that this QDBusUnixFileDescriptor object holds
    to a copy of \a fileDescriptor. The original file descriptor is not
    touched and must be closed by the user.

    Note that the value returned by fileDescriptor() will be different from
    the \a fileDescriptor parameter passed.

    If the \a fileDescriptor parameter is not valid, isValid() will return
    false and fileDescriptor() will return -1.

    \sa isValid(), fileDescriptor()
*/
void QDBusUnixFileDescriptor::setFileDescriptor(int fileDescriptor)
{
    if (fileDescriptor != -1)
        giveFileDescriptor(qt_safe_dup(fileDescriptor));
}

/*!
    \internal
    Sets the Unix file descriptor to \a fileDescriptor without copying.

    \sa setFileDescriptor()
*/
void QDBusUnixFileDescriptor::giveFileDescriptor(int fileDescriptor)
{
    // if we are the sole ref, d remains unchanged
    // if detaching happens, d->fd will be -1
    if (d)
        d.detach();
    else
        d = new QDBusUnixFileDescriptorPrivate;

    const int fdl = d->fd.load();
    if (fdl != -1)
        qt_safe_close(fdl);

    if (fileDescriptor != -1)
        d->fd.store(fileDescriptor);
}

/*!
    \internal
    Extracts the Unix file descriptor from the QDBusUnixFileDescriptor object
    and transfers ownership.

    Note: since QDBusUnixFileDescriptor is implicitly shared, this function
    is inherently racy and should be avoided.
*/
int QDBusUnixFileDescriptor::takeFileDescriptor()
{
    if (!d)
        return -1;

    return d->fd.fetchAndStoreRelaxed(-1);
}

QDBusUnixFileDescriptorPrivate::~QDBusUnixFileDescriptorPrivate()
{
    const int fdl = fd.load();
    if (fdl != -1)
        qt_safe_close(fdl);
}

#else
bool QDBusUnixFileDescriptor::isSupported()
{
    return false;
}

void QDBusUnixFileDescriptor::setFileDescriptor(int)
{
}

void QDBusUnixFileDescriptor::giveFileDescriptor(int)
{
}

int QDBusUnixFileDescriptor::takeFileDescriptor()
{
    return -1;
}

QDBusUnixFileDescriptorPrivate::~QDBusUnixFileDescriptorPrivate()
{
}

#endif

#endif // QT_NO_DBUS

QT_END_NAMESPACE
