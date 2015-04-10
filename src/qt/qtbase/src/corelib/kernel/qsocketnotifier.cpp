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

#include "qsocketnotifier.h"

#include "qplatformdefs.h"

#include "qabstracteventdispatcher.h"
#include "qcoreapplication.h"

#include "qobject_p.h"
#include <private/qthread_p.h>

QT_BEGIN_NAMESPACE

class QSocketNotifierPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QSocketNotifier)
public:
    qintptr sockfd;
    QSocketNotifier::Type sntype;
    bool snenabled;
};

/*!
    \class QSocketNotifier
    \inmodule QtCore
    \brief The QSocketNotifier class provides support for monitoring
    activity on a file descriptor.

    \ingroup network
    \ingroup io

    The QSocketNotifier makes it possible to integrate Qt's event
    loop with other event loops based on file descriptors. File
    descriptor action is detected in Qt's main event
    loop (QCoreApplication::exec()).

    \target write notifiers

    Once you have opened a device using a low-level (usually
    platform-specific) API, you can create a socket notifier to
    monitor the file descriptor. The socket notifier is enabled by
    default, i.e. it emits the activated() signal whenever a socket
    event corresponding to its type occurs. Connect the activated()
    signal to the slot you want to be called when an event
    corresponding to your socket notifier's type occurs.

    There are three types of socket notifiers: read, write, and
    exception. The type is described by the \l Type enum, and must be
    specified when constructing the socket notifier. After
    construction it can be determined using the type() function. Note
    that if you need to monitor both reads and writes for the same
    file descriptor, you must create two socket notifiers. Note also
    that it is not possible to install two socket notifiers of the
    same type (\l Read, \l Write, \l Exception) on the same socket.

    The setEnabled() function allows you to disable as well as enable
    the socket notifier. It is generally advisable to explicitly
    enable or disable the socket notifier, especially for write
    notifiers. A disabled notifier ignores socket events (the same
    effect as not creating the socket notifier). Use the isEnabled()
    function to determine the notifier's current status.

    Finally, you can use the socket() function to retrieve the
    socket identifier.  Although the class is called QSocketNotifier,
    it is normally used for other types of devices than sockets.
    QTcpSocket and QUdpSocket provide notification through signals, so
    there is normally no need to use a QSocketNotifier on them.

    \section1 Notes for Windows Users

    The socket passed to QSocketNotifier will become non-blocking, even if
    it was created as a blocking socket.
    The activated() signal is sometimes triggered by high general activity
    on the host, even if there is nothing to read. A subsequent read from
    the socket can then fail, the error indicating that there is no data
    available (e.g., \c{WSAEWOULDBLOCK}). This is an operating system
    limitation, and not a bug in QSocketNotifier.

    To ensure that the socket notifier handles read notifications correctly,
    follow these steps when you receive a notification:

    \list 1
    \li Disable the notifier.
    \li Read data from the socket.
    \li Re-enable the notifier if you are interested in more data (such as after
       having written a new command to a remote server).
    \endlist

    To ensure that the socket notifier handles write notifications correctly,
    follow these steps when you receive a notification:

    \list 1
    \li Disable the notifier.
    \li Write as much data as you can (before \c EWOULDBLOCK is returned).
    \li Re-enable notifier if you have more data to write.
    \endlist

    \b{Further information:}
    On Windows, Qt always disables the notifier after getting a notification,
    and only re-enables it if more data is expected. For example, if data is
    read from the socket and it can be used to read more, or if reading or
    writing is not possible because the socket would block, in which case
    it is necessary to wait before attempting to read or write again.

    \sa QFile, QProcess, QTcpSocket, QUdpSocket
*/

/*!
    \enum QSocketNotifier::Type

    This enum describes the various types of events that a socket
    notifier can recognize. The type must be specified when
    constructing the socket notifier.

    Note that if you need to monitor both reads and writes for the
    same file descriptor, you must create two socket notifiers. Note
    also that it is not possible to install two socket notifiers of
    the same type (Read, Write, Exception) on the same socket.

    \value Read      There is data to be read.
    \value Write      Data can be written.
    \value Exception  An exception has occurred. We recommend against using this.

    \sa QSocketNotifier(), type()
*/

/*!
    Constructs a socket notifier with the given \a parent. It enables
    the \a socket, and watches for events of the given \a type.

    It is generally advisable to explicitly enable or disable the
    socket notifier, especially for write notifiers.

    \b{Note for Windows users:} The socket passed to QSocketNotifier
    will become non-blocking, even if it was created as a blocking socket.

    \sa setEnabled(), isEnabled()
*/

QSocketNotifier::QSocketNotifier(qintptr socket, Type type, QObject *parent)
    : QObject(*new QSocketNotifierPrivate, parent)
{
    Q_D(QSocketNotifier);
    d->sockfd = socket;
    d->sntype = type;
    d->snenabled = true;

    if (socket < 0)
        qWarning("QSocketNotifier: Invalid socket specified");
    else if (!d->threadData->eventDispatcher.load())
        qWarning("QSocketNotifier: Can only be used with threads started with QThread");
    else
        d->threadData->eventDispatcher.load()->registerSocketNotifier(this);
}

/*!
    Destroys this socket notifier.
*/

QSocketNotifier::~QSocketNotifier()
{
    setEnabled(false);
}


/*!
    \fn void QSocketNotifier::activated(int socket)

    This signal is emitted whenever the socket notifier is enabled and
    a socket event corresponding to its \l {Type}{type} occurs.

    The socket identifier is passed in the \a socket parameter.

    \sa type(), socket()
*/


/*!
    Returns the socket identifier specified to the constructor.

    \sa type()
*/
qintptr QSocketNotifier::socket() const
{
    Q_D(const QSocketNotifier);
    return d->sockfd;
}

/*!
    Returns the socket event type specified to the constructor.

    \sa socket()
*/
QSocketNotifier::Type QSocketNotifier::type() const
{
    Q_D(const QSocketNotifier);
    return d->sntype;
}

/*!
    Returns \c true if the notifier is enabled; otherwise returns \c false.

    \sa setEnabled()
*/
bool QSocketNotifier::isEnabled() const
{
    Q_D(const QSocketNotifier);
    return d->snenabled;
}

/*!
    If \a enable is true, the notifier is enabled; otherwise the notifier
    is disabled.

    The notifier is enabled by default, i.e. it emits the activated()
    signal whenever a socket event corresponding to its
    \l{type()}{type} occurs. If it is disabled, it ignores socket
    events (the same effect as not creating the socket notifier).

    Write notifiers should normally be disabled immediately after the
    activated() signal has been emitted

    \sa isEnabled(), activated()
*/

void QSocketNotifier::setEnabled(bool enable)
{
    Q_D(QSocketNotifier);
    if (d->sockfd < 0)
        return;
    if (d->snenabled == enable)                        // no change
        return;
    d->snenabled = enable;

    if (!d->threadData->eventDispatcher.load()) // perhaps application/thread is shutting down
        return;
    if (d->snenabled)
        d->threadData->eventDispatcher.load()->registerSocketNotifier(this);
    else
        d->threadData->eventDispatcher.load()->unregisterSocketNotifier(this);
}


/*!\reimp
*/
bool QSocketNotifier::event(QEvent *e)
{
    Q_D(QSocketNotifier);
    // Emits the activated() signal when a QEvent::SockAct or QEvent::SockClose is
    // received.
    if (e->type() == QEvent::ThreadChange) {
        if (d->snenabled) {
            QMetaObject::invokeMethod(this, "setEnabled", Qt::QueuedConnection,
                                      Q_ARG(bool, d->snenabled));
            setEnabled(false);
        }
    }
    QObject::event(e);                        // will activate filters
    if ((e->type() == QEvent::SockAct) || (e->type() == QEvent::SockClose)) {
        emit activated(d->sockfd, QPrivateSignal());
        return true;
    }
    return false;
}

QT_END_NAMESPACE
