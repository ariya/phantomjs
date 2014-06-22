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

#include "qdbuspendingreply.h"
#include "qdbuspendingcall_p.h"
#include "qdbusmetatype.h"

#ifndef QT_NO_DBUS

/*!
    \class QDBusPendingReply
    \inmodule QtDBus
    \since 4.5

    \brief The QDBusPendingReply class contains the reply to an asynchronous method call

    The QDBusPendingReply is a template class with up to 8 template
    parameters. Those parameters are the types that will be used to
    extract the contents of the reply's data.

    This class is similar in functionality to QDBusReply, but with two
    important differences:

    \list
      \li QDBusReply accepts exactly one return type, whereas
         QDBusPendingReply can have from 1 to 8 types
      \li QDBusReply only works on already completed replies, whereas
         QDBusPendingReply allows one to wait for replies from pending
         calls
    \endlist

    Where with QDBusReply you would write:

    \snippet code/src_qdbus_qdbusreply.cpp 0

    with QDBusPendingReply, the equivalent code (including the blocking
    wait for the reply) would be:

    \snippet code/src_qdbus_qdbuspendingreply.cpp 0

    For method calls that have more than one output argument, with
    QDBusReply, you would write:

    \snippet code/src_qdbus_qdbusreply.cpp 1

    whereas with QDBusPendingReply, all of the output arguments should
    be template parameters:

    \snippet code/src_qdbus_qdbuspendingreply.cpp 2

    QDBusPendingReply objects can be associated with
    QDBusPendingCallWatcher objects, which emit signals when the reply
    arrives.

    \sa QDBusPendingCallWatcher, QDBusReply,
        QDBusAbstractInterface::asyncCall()
*/

/*!
    \fn QDBusPendingReply::QDBusPendingReply()

    Creates an empty QDBusPendingReply object. Without assigning a
    QDBusPendingCall object to this reply, QDBusPendingReply cannot do
    anything. All functions return their failure values.
*/

/*!
    \fn QDBusPendingReply::QDBusPendingReply(const QDBusPendingReply &other)

    Creates a copy of the \a other QDBusPendingReply object. Just like
    QDBusPendingCall and QDBusPendingCallWatcher, this QDBusPendingReply
    object will share the same pending call reference. All copies
    share the same return values.
*/

/*!
    \fn QDBusPendingReply::QDBusPendingReply(const QDBusPendingCall &call)

    Creates a QDBusPendingReply object that will take its contents from
    the \a call pending asynchronous call. This QDBusPendingReply object
    will share the same pending call reference as \a call.
*/

/*!
    \fn QDBusPendingReply::QDBusPendingReply(const QDBusMessage &message)

    Creates a QDBusPendingReply object that will take its contents from
    the message \a message. In this case, this object will be already
    in its finished state and the reply's contents will be accessible.

    \sa isFinished()
*/

/*!
    \fn QDBusPendingReply &QDBusPendingReply::operator=(const QDBusPendingReply &other)

    Makes a copy of \a other and drops the reference to the current
    pending call. If the current reference is to an unfinished pending
    call and this is the last reference, the pending call will be
    canceled and there will be no way of retrieving the reply's
    contents, when they arrive.
*/

/*!
    \fn QDBusPendingReply &QDBusPendingReply::operator=(const QDBusPendingCall &call)

    Makes this object take its contents from the \a call pending call
    and drops the reference to the current pending call. If the
    current reference is to an unfinished pending call and this is the
    last reference, the pending call will be canceled and there will
    be no way of retrieving the reply's contents, when they arrive.
*/

/*!
    \fn QDBusPendingReply &QDBusPendingReply::operator=(const QDBusMessage &message)

    Makes this object take its contents from the \a message message
    and drops the reference to the current pending call. If the
    current reference is to an unfinished pending call and this is the
    last reference, the pending call will be canceled and there will
    be no way of retrieving the reply's contents, when they arrive.

    After this function is finished, the QDBusPendingReply object will
    be in its "finished" state and the \a message contents will be
    accessible.

    \sa isFinished()
*/

/*!
    \fn int QDBusPendingReply::count() const

    Return the number of arguments the reply is supposed to have. This
    number matches the number of non-void template parameters in this
    class.

    If the reply arrives with a different number of arguments (or with
    different types), it will be transformed into an error reply
    indicating a bad signature.
*/

/*!
    \fn QVariant QDBusPendingReply::argumentAt(int index) const

    Returns the argument at position \a index in the reply's
    contents. If the reply doesn't have that many elements, this
    function's return value is undefined (will probably cause an
    assertion failure), so it is important to verify that the
    processing is finished and the reply is valid.

    If the reply does not contain an argument at position \a index or if the
    reply was an error, this function returns an invalid QVariant. Since D-Bus
    messages can never contain invalid QVariants, this return can be used to
    detect an error condition.
*/

/*!
    \fn Type QDBusPendingReply::argumentAt() const

    Returns the argument at position \c Index (which is a template
    parameter) cast to type \c Type. This function uses template code
    to determine the proper \c Type type, according to the type list
    used in the construction of this object.

    Note that, if the reply hasn't arrived, this function causes the
    calling thread to block until the reply is processed.

    If the reply does not contain an argument at position \c Index or if the
    reply was an error, this function returns a \c Type object that is default
    constructed, which may be indistinguishable from a valid value. To reliably
    determine whether the message was an error, use isError().
*/

/*!
    \fn T1 QDBusPendingReply::value() const

    Returns the first argument in this reply, cast to type \c T1 (the
    first template parameter of this class). This is equivalent to
    calling argumentAt<0>().

    This function is provided as a convenience, matching the
    QDBusReply::value() function.

    Note that, if the reply hasn't arrived, this function causes the
    calling thread to block until the reply is processed.

    If the reply is an error reply, this function returns a default-constructed
    \c T1 object, which may be indistinguishable from a valid value. To
    reliably determine whether the message was an error, use isError().
*/

/*!
    \fn QDBusPendingReply::operator T1() const

    Returns the first argument in this reply, cast to type \c T1 (the
    first template parameter of this class). This is equivalent to
    calling argumentAt<0>().

    This function is provided as a convenience, matching the
    QDBusReply::value() function.

    Note that, if the reply hasn't arrived, this function causes the
    calling thread to block until the reply is processed.

    If the reply is an error reply, this function returns a default-constructed
    \c T1 object, which may be indistinguishable from a valid value. To
    reliably determine whether the message was an error, use isError().
*/

/*!
    \fn void QDBusPendingReply::waitForFinished()

    Suspends the execution of the calling thread until the reply is
    received and processed. After this function returns, isFinished()
    should return true, indicating the reply's contents are ready to
    be processed.

    \sa QDBusPendingCallWatcher::waitForFinished()
*/

QDBusPendingReplyData::QDBusPendingReplyData()
    : QDBusPendingCall(0)         // initialize base class empty
{
}

QDBusPendingReplyData::~QDBusPendingReplyData()
{
}

void QDBusPendingReplyData::assign(const QDBusPendingCall &other)
{
    QDBusPendingCall::operator=(other);
}

void QDBusPendingReplyData::assign(const QDBusMessage &message)
{
    d = new QDBusPendingCallPrivate(QDBusMessage(), 0); // drops the reference to the old one
    d->replyMessage = message;
}

QVariant QDBusPendingReplyData::argumentAt(int index) const
{
    if (!d)
        return QVariant();

    d->waitForFinished();   // bypasses "const"

    return d->replyMessage.arguments().value(index);
}

void QDBusPendingReplyData::setMetaTypes(int count, const int *types)
{
    Q_ASSERT(d);
    QMutexLocker locker(&d->mutex);
    d->setMetaTypes(count, types);
    d->checkReceivedSignature();
}

#endif // QT_NO_DBUS
