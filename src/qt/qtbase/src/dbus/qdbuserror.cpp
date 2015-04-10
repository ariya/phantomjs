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

#include "qdbuserror.h"

#include <qdebug.h>
#include <qvarlengtharray.h>

#ifndef QT_BOOTSTRAPPED
#include "qdbus_symbols_p.h"
#include "qdbusmessage.h"
#include "qdbusmessage_p.h"
#endif

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

/*
 * Use the following Perl script to generate the error string index list:
===== PERL SCRIPT ====
print "static const char errorMessages_string[] =\n";
$counter = 0;
$i = 0;
while (<STDIN>) {
    chomp;
    print "    \"$_\\0\"\n";
    $sizes[$i++] = $counter;
    $counter += 1 + length $_;
}
print "    \"\\0\";\n\nstatic const int errorMessages_indices[] = {\n    ";
for ($j = 0; $j < $i; ++$j) {
    printf "$sizes[$j], ";
}
print "0\n};\n";
===== PERL SCRIPT ====

 * The input data is as follows:
other
org.freedesktop.DBus.Error.Failed
org.freedesktop.DBus.Error.NoMemory
org.freedesktop.DBus.Error.ServiceUnknown
org.freedesktop.DBus.Error.NoReply
org.freedesktop.DBus.Error.BadAddress
org.freedesktop.DBus.Error.NotSupported
org.freedesktop.DBus.Error.LimitsExceeded
org.freedesktop.DBus.Error.AccessDenied
org.freedesktop.DBus.Error.NoServer
org.freedesktop.DBus.Error.Timeout
org.freedesktop.DBus.Error.NoNetwork
org.freedesktop.DBus.Error.AddressInUse
org.freedesktop.DBus.Error.Disconnected
org.freedesktop.DBus.Error.InvalidArgs
org.freedesktop.DBus.Error.UnknownMethod
org.freedesktop.DBus.Error.TimedOut
org.freedesktop.DBus.Error.InvalidSignature
org.freedesktop.DBus.Error.UnknownInterface
org.freedesktop.DBus.Error.UnknownObject
org.freedesktop.DBus.Error.UnknownProperty
org.freedesktop.DBus.Error.PropertyReadOnly
org.qtproject.QtDBus.Error.InternalError
org.qtproject.QtDBus.Error.InvalidService
org.qtproject.QtDBus.Error.InvalidObjectPath
org.qtproject.QtDBus.Error.InvalidInterface
org.qtproject.QtDBus.Error.InvalidMember
*/

// in the same order as KnownErrors!
static const char errorMessages_string[] =
    "other\0"
    "org.freedesktop.DBus.Error.Failed\0"
    "org.freedesktop.DBus.Error.NoMemory\0"
    "org.freedesktop.DBus.Error.ServiceUnknown\0"
    "org.freedesktop.DBus.Error.NoReply\0"
    "org.freedesktop.DBus.Error.BadAddress\0"
    "org.freedesktop.DBus.Error.NotSupported\0"
    "org.freedesktop.DBus.Error.LimitsExceeded\0"
    "org.freedesktop.DBus.Error.AccessDenied\0"
    "org.freedesktop.DBus.Error.NoServer\0"
    "org.freedesktop.DBus.Error.Timeout\0"
    "org.freedesktop.DBus.Error.NoNetwork\0"
    "org.freedesktop.DBus.Error.AddressInUse\0"
    "org.freedesktop.DBus.Error.Disconnected\0"
    "org.freedesktop.DBus.Error.InvalidArgs\0"
    "org.freedesktop.DBus.Error.UnknownMethod\0"
    "org.freedesktop.DBus.Error.TimedOut\0"
    "org.freedesktop.DBus.Error.InvalidSignature\0"
    "org.freedesktop.DBus.Error.UnknownInterface\0"
    "org.freedesktop.DBus.Error.UnknownObject\0"
    "org.freedesktop.DBus.Error.UnknownProperty\0"
    "org.freedesktop.DBus.Error.PropertyReadOnly\0"
    "org.qtproject.QtDBus.Error.InternalError\0"
    "org.qtproject.QtDBus.Error.InvalidService\0"
    "org.qtproject.QtDBus.Error.InvalidObjectPath\0"
    "org.qtproject.QtDBus.Error.InvalidInterface\0"
    "org.qtproject.QtDBus.Error.InvalidMember\0"
    "\0";

static const int errorMessages_indices[] = {
    0,    6,   40,   76,  118,  153,  191,  231,
    273,  313,  349,  384,  421,  461,  501,  540,
    581,  617,  661,  705,  746,  789,  833,  874,
    916,  961, 1005,   -1
};

static const int errorMessages_count = sizeof errorMessages_indices /
                                       sizeof errorMessages_indices[0] - 1;

static inline const char *get(QDBusError::ErrorType code)
{
    int intcode = qBound(0, int(code) - int(QDBusError::Other), errorMessages_count);
    return errorMessages_string + errorMessages_indices[intcode];
}

static inline QDBusError::ErrorType get(const char *name)
{
    if (!name || !*name)
        return QDBusError::NoError;
    for (int i = 0; i < errorMessages_count; ++i)
        if (strcmp(name, errorMessages_string + errorMessages_indices[i]) == 0)
            return QDBusError::ErrorType(i + int(QDBusError::Other));
    return QDBusError::Other;
}

/*!
    \class QDBusError
    \inmodule QtDBus
    \since 4.2

    \brief The QDBusError class represents an error received from the
    D-Bus bus or from remote applications found in the bus.

    When dealing with the D-Bus bus service or with remote
    applications over D-Bus, a number of error conditions can
    happen. This error conditions are sometimes signalled by a
    returned error value or by a QDBusError.

    C++ and Java exceptions are a valid analogy for D-Bus errors:
    instead of returning normally with a return value, remote
    applications and the bus may decide to throw an error
    condition. However, the Qt D-Bus implementation does not use the C++
    exception-throwing mechanism, so you will receive QDBusErrors in
    the return reply (see QDBusReply::error()).

    QDBusError objects are used to inspect the error name and message
    as received from the bus and remote applications. You should not
    create such objects yourself to signal error conditions when
    called from D-Bus: instead, use QDBusMessage::createError() and
    QDBusConnection::send().

    \sa QDBusConnection::send(), QDBusMessage, QDBusReply
*/

/*!
    \enum QDBusError::ErrorType

    In order to facilitate verification of the most common D-Bus errors generated by the D-Bus
    implementation and by the bus daemon itself, QDBusError can be compared to a set of pre-defined
    values:

    \value NoError              QDBusError is invalid (i.e., the call succeeded)
    \value Other                QDBusError contains an error that is one of the well-known ones
    \value Failed               The call failed (\c org.freedesktop.DBus.Error.Failed)
    \value NoMemory             Out of memory (\c org.freedesktop.DBus.Error.NoMemory)
    \value ServiceUnknown       The called service is not known
                                (\c org.freedesktop.DBus.Error.ServiceUnknown)
    \value NoReply              The called method did not reply within the specified timeout
                                (\c org.freedesktop.DBus.Error.NoReply)
    \value BadAddress           The address given is not valid
                                (\c org.freedesktop.DBus.Error.BadAddress)
    \value NotSupported         The call/operation is not supported
                                (\c org.freedesktop.DBus.Error.NotSupported)
    \value LimitsExceeded       The limits allocated to this process/call/connection exceeded the
                                pre-defined values (\c org.freedesktop.DBus.Error.LimitsExceeded)
    \value AccessDenied         The call/operation tried to access a resource it isn't allowed to
                                (\c org.freedesktop.DBus.Error.AccessDenied)
    \value NoServer             \e {Documentation doesn't say what this is for}
                                (\c org.freedesktop.DBus.Error.NoServer)
    \value Timeout              \e {Documentation doesn't say what this is for or how it's used}
                                (\c org.freedesktop.DBus.Error.Timeout)
    \value NoNetwork            \e {Documentation doesn't say what this is for}
                                (\c org.freedesktop.DBus.Error.NoNetwork)
    \value AddressInUse         QDBusServer tried to bind to an address that is already in use
                                (\c org.freedesktop.DBus.Error.AddressInUse)
    \value Disconnected         The call/process/message was sent after QDBusConnection disconnected
                                (\c org.freedesktop.DBus.Error.Disconnected)
    \value InvalidArgs          The arguments passed to this call/operation are not valid
                                (\c org.freedesktop.DBus.Error.InvalidArgs)
    \value UnknownMethod        The method called was not found in this object/interface with the
                                given parameters (\c org.freedesktop.DBus.Error.UnknownMethod)
    \value TimedOut             \e {Documentation doesn't say...}
                                (\c org.freedesktop.DBus.Error.TimedOut)
    \value InvalidSignature     The type signature is not valid or compatible
                                (\c org.freedesktop.DBus.Error.InvalidSignature)
    \value UnknownInterface     The interface is not known in this object
                                (\c org.freedesktop.DBus.Error.UnknownInterface)
    \value UnknownObject        The object path points to an object that does not exist
                                (\c org.freedesktop.DBus.Error.UnknownObject)
    \value UnknownProperty      The property does not exist in this interface
                                (\c org.freedesktop.DBus.Error.UnknownProperty)
    \value PropertyReadOnly     The property set failed because the property is read-only
                                (\c org.freedesktop.DBus.Error.PropertyReadOnly)

    \value InternalError        An internal error occurred

    \value InvalidObjectPath    The object path provided is invalid.

    \value InvalidService       The service requested is invalid.

    \value InvalidMember        The member is invalid.

    \value InvalidInterface     The interface is invalid.
*/

/*!
    \internal
    Constructs a QDBusError that represents no error.
*/
QDBusError::QDBusError()
    : code(NoError)
{
    // ### This class has an implicit (therefore inline) destructor
    // so the following field cannot be used.
    Q_UNUSED(unused);
}

#ifndef QT_BOOTSTRAPPED
/*!
    \internal
    Constructs a QDBusError from a DBusError structure.
*/
QDBusError::QDBusError(const DBusError *error)
    : code(NoError)
{
    if (!error || !q_dbus_error_is_set(error))
        return;

    code = ::get(error->name);
    msg = QString::fromUtf8(error->message);
    nm = QString::fromUtf8(error->name);
}

/*!
    \internal
    Constructs a QDBusError from a QDBusMessage.
*/
QDBusError::QDBusError(const QDBusMessage &qdmsg)
    : code(NoError)
{
    if (qdmsg.type() != QDBusMessage::ErrorMessage)
        return;

    code = ::get(qdmsg.errorName().toUtf8().constData());
    nm = qdmsg.errorName();
    msg = qdmsg.errorMessage();
}
#endif

/*!
    \internal
    Constructs a QDBusError from a well-known error code
*/
QDBusError::QDBusError(ErrorType error, const QString &mess)
    : code(error)
{
    nm = QLatin1String(::get(error));
    msg = mess;
}

/*!
    \internal
    Constructs a QDBusError from another QDBusError object
*/
QDBusError::QDBusError(const QDBusError &other)
        : code(other.code), msg(other.msg), nm(other.nm)
{
}

/*!
  \internal
  Assignment operator
*/

QDBusError &QDBusError::operator=(const QDBusError &other)
{
    code = other.code;
    msg = other.msg;
    nm = other.nm;
    return *this;
}

#ifndef QT_BOOTSTRAPPED
/*!
  \internal
  Assignment operator from a QDBusMessage
*/
QDBusError &QDBusError::operator=(const QDBusMessage &qdmsg)
{
    if (qdmsg.type() == QDBusMessage::ErrorMessage) {
        code = ::get(qdmsg.errorName().toUtf8().constData());
        nm = qdmsg.errorName();
        msg = qdmsg.errorMessage();
    } else {
        code =NoError;
        nm.clear();
        msg.clear();
    }
    return *this;
}
#endif

/*!
    Returns this error's ErrorType.

    \sa ErrorType
*/

QDBusError::ErrorType QDBusError::type() const
{
    return code;
}

/*!
    Returns this error's name. Error names are similar to D-Bus Interface names, like
    \c org.freedesktop.DBus.InvalidArgs.

    \sa type()
*/

QString QDBusError::name() const
{
    return nm;
}

/*!
    Returns the message that the callee associated with this error. Error messages are
    implementation defined and usually contain a human-readable error code, though this does not
    mean it is suitable for your end-users.
*/

QString QDBusError::message() const
{
    return msg;
}

/*!
    Returns \c true if this is a valid error condition (i.e., if there was an error),
    otherwise false.
*/

bool QDBusError::isValid() const
{
    return (code != NoError);
}

/*!
    \since 4.3
    Returns the error name associated with error condition \a error.
*/
QString QDBusError::errorString(ErrorType error)
{
    return QLatin1String(::get(error));
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QDBusError &msg)
{
    dbg.nospace() << "QDBusError(" << msg.name() << ", " << msg.message() << ')';
    return dbg.space();
}
#endif

QT_END_NAMESPACE

#endif // QT_NO_DBUS
