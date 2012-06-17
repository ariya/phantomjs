/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#include "qnetworksession.h"
#include "qbearerengine_p.h"

#include <QEventLoop>
#include <QTimer>
#include <QThread>

#include "qnetworkconfigmanager_p.h"
#include "qnetworksession_p.h"

#ifdef Q_OS_SYMBIAN
#include <es_sock.h>
#include <private/qcore_symbian_p.h>
#endif

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

/*!
    \class QNetworkSession

    \brief The QNetworkSession class provides control over the system's access points
    and enables session management for cases when multiple clients access the same access point.

    \since 4.7

    \inmodule QtNetwork
    \ingroup network

    A QNetworkSession enables control over the system's network interfaces. The session's configuration
    parameter are determined via the QNetworkConfiguration object to which it is bound. Depending on the 
    type of the session (single access point or service network) a session may be linked to one or more
    network interfaces. By means of \l{open()}{opening} and \l{close()}{closing} of network sessions 
    a developer can start and stop the systems network interfaces. If the configuration represents 
    multiple access points (see \l QNetworkConfiguration::ServiceNetwork) more advanced features such as roaming may be supported.

    QNetworkSession supports session management within the same process and depending on the platform's 
    capabilities may support out-of-process sessions. If the same 
    network configuration is used by multiple open sessions the underlying network interface is only terminated once
    the last session has been closed.

    \section1 Roaming

    Applications may connect to the preferredConfigurationChanged() signal in order to 
    receive notifications when a more suitable access point becomes available. 
    In response to this signal the application must either initiate the roaming via migrate()
    or ignore() the new access point. Once the session has roamed the 
    newConfigurationActivated() signal is emitted. The application may now test the 
    carrier and must either accept() or reject() it. The session will return to the previous
    access point if the roaming was rejected. The subsequent state diagram depicts the required
    state transitions.
    
    \image roaming-states.png

    Some platforms may distinguish forced roaming and application level roaming (ALR). 
    ALR implies that the application controls (via migrate(), ignore(), accept() and reject()) 
    whether a network session can roam from one access point to the next. Such control is useful
    if the application maintains stateful socket connections and wants to control the transition from
    one interface to the next. Forced roaming implies that the system automatically roams to the next network without 
    consulting the application. This has the advantage that the application can make use of roaming features
    without actually being aware of it. It is expected that the application detects that the underlying 
    socket is broken and automatically reconnects via the new network link.

    If the platform supports both modes of roaming, an application indicates its preference
    by connecting to the preferredConfigurationChanged() signal. Connecting to this signal means that
    the application wants to take control over the roaming behavior and therefore implies application
    level roaming. If the client does not connect to the preferredConfigurationChanged(), forced roaming
    is used. If forced roaming is not supported the network session will not roam by default.

    Some applications may want to suppress any form of roaming altogether. Possible use cases may be 
    high priority downloads or remote services which cannot handle a roaming enabled client. Clients
    can suppress roaming by connecting to the preferredConfigurationChanged() signal and answer each
    signal emission with ignore().

    \sa QNetworkConfiguration, QNetworkConfigurationManager
*/

/*!
    \enum QNetworkSession::State

    This enum describes the connectivity state of the session. If the session is based on a
    single access point configuration the state of the session is the same as the state of the
    associated network interface.

    \value Invalid          The session is invalid due to an invalid configuration. This may 
                            happen due to a removed access point or a configuration that was 
                            invalid to begin with.
    \value NotAvailable     The session is based on a defined but not yet discovered QNetworkConfiguration
                            (see \l QNetworkConfiguration::StateFlag).
    \value Connecting       The network session is being established.
    \value Connected        The network session is connected. If the current process wishes to use this session
                            it has to register its interest by calling open(). A network session 
                            is considered to be ready for socket operations if it isOpen() and connected.
    \value Closing          The network session is in the process of being shut down.
    \value Disconnected     The network session is not connected. The associated QNetworkConfiguration
                            has the state QNetworkConfiguration::Discovered.
    \value Roaming          The network session is roaming from one access point to another 
                            access point.
*/

/*!
    \enum QNetworkSession::SessionError

    This enum describes the session errors that can occur.

    \value UnknownSessionError          An unidentified error occurred.
    \value SessionAbortedError          The session was aborted by the user or system.
    \value RoamingError                 The session cannot roam to a new configuration.
    \value OperationNotSupportedError   The operation is not supported for current configuration.
    \value InvalidConfigurationError    The operation cannot currently be performed for the
                                        current configuration.
*/

/*!
    \fn void QNetworkSession::stateChanged(QNetworkSession::State state)

    This signal is emitted whenever the state of the network session changes.
    The \a state parameter is the new state.

    \sa state()
*/

/*!
    \fn void QNetworkSession::error(QNetworkSession::SessionError error)

    This signal is emitted after an error occurred. The \a error parameter
    describes the error that occurred.

    \sa error(), errorString()
*/

/*!
    \fn void QNetworkSession::preferredConfigurationChanged(const QNetworkConfiguration &config, bool isSeamless)

    This signal is emitted when the preferred configuration/access point for the
    session changes. Only sessions which are based on service network configurations
    may emit this signal. \a config can be used to determine access point specific
    details such as proxy settings and \a isSeamless indicates whether roaming will
    break the sessions IP address.

    As a consequence to this signal the application must either start the roaming process
    by calling migrate() or choose to ignore() the new access point.

    If the roaming process is non-seamless the IP address will change which means that
    a socket becomes invalid. However seamless mobility can ensure that the local IP address
    does not change. This is achieved by using a virtual IP address which is bound to the actual
    link address. During the roaming process the virtual address is attached to the new link 
    address.

    Some platforms may support the concept of Forced Roaming and Application Level Roaming (ALR). 
    Forced roaming implies that the platform may simply roam to a new configuration without 
    consulting applications. It is up to the application to detect the link layer loss and reestablish
    its sockets. In contrast ALR provides the opportunity to prevent the system from roaming. 
    If this session is based on a configuration that supports roaming the application can choose
    whether it wants to be consulted (ALR use case) by connecting to this signal. For as long as this signal 
    connection remains the session remains registered as a roaming stakeholder; otherwise roaming will 
    be enforced by the platform.

    \sa migrate(), ignore(), QNetworkConfiguration::isRoamingAvailable()
*/

/*!
    \fn void QNetworkSession::newConfigurationActivated()

    This signal is emitted once the session has roamed to the new access point.
    The application may reopen its socket and test the suitability of the new network link.
    Subsequently it must either accept() or reject() the new access point. 

    \sa accept(), reject()
*/

/*!
    \fn void QNetworkSession::opened()

    This signal is emitted when the network session has been opened. 
    
    The underlying network interface will not be shut down as long as the session remains open.
    Note that this feature is dependent on \l{QNetworkConfigurationManager::SystemSessionSupport}{system wide session support}.
*/

/*!
    \fn void QNetworkSession::closed()

    This signal is emitted when the network session has been closed.
*/

/*!
    Constructs a session based on \a connectionConfig with the given \a parent.

    \sa QNetworkConfiguration
*/
QNetworkSession::QNetworkSession(const QNetworkConfiguration &connectionConfig, QObject *parent)
    : QObject(parent), d(0)
{
    // invalid configuration
    if (!connectionConfig.identifier().isEmpty()) {
        foreach (QBearerEngine *engine, qNetworkConfigurationManagerPrivate()->engines()) {
            if (engine->hasIdentifier(connectionConfig.identifier())) {
                d = engine->createSessionBackend();
                d->q = this;
                d->publicConfig = connectionConfig;
                d->syncStateWithInterface();
                connect(d, SIGNAL(quitPendingWaitsForOpened()), this, SIGNAL(opened()));
                connect(d, SIGNAL(error(QNetworkSession::SessionError)),
                        this, SIGNAL(error(QNetworkSession::SessionError)));
                connect(d, SIGNAL(stateChanged(QNetworkSession::State)),
                        this, SIGNAL(stateChanged(QNetworkSession::State)));
                connect(d, SIGNAL(closed()), this, SIGNAL(closed()));
                connect(d, SIGNAL(preferredConfigurationChanged(QNetworkConfiguration,bool)),
                        this, SIGNAL(preferredConfigurationChanged(QNetworkConfiguration,bool)));
                connect(d, SIGNAL(newConfigurationActivated()),
                        this, SIGNAL(newConfigurationActivated()));
                break;
            }
        }
    }

    qRegisterMetaType<QNetworkSession::State>();
    qRegisterMetaType<QNetworkSession::SessionError>();
}

/*!
    Frees the resources associated with the QNetworkSession object.
*/
QNetworkSession::~QNetworkSession()
{
    delete d;
}

/*!
    Creates an open session which increases the session counter on the underlying network interface.
    The system will not terminate a network interface until the session reference counter reaches zero.
    Therefore an open session allows an application to register its use of the interface.

    As a result of calling open() the interface will be started if it is not connected/up yet. 
    Some platforms may not provide support for out-of-process sessions. On such platforms the session
    counter ignores any sessions held by another process. The platform capabilities can be 
    detected via QNetworkConfigurationManager::capabilities().

    Note that this call is asynchronous. Depending on the outcome of this call the results can be enquired 
    by connecting to the stateChanged(), opened() or error() signals.

    It is not a requirement to open a session in order to monitor the underlying network interface.

    \sa close(), stop(), isOpen()
*/
void QNetworkSession::open()
{
    if (d)
        d->open();
    else
        emit error(InvalidConfigurationError);
}

/*!
    Waits until the session has been opened, up to \a msecs milliseconds. If the session has been opened, this
    function returns true; otherwise it returns false. In the case where it returns false, you can call error()
    to determine the cause of the error.

    The following example waits up to one second for the session to be opened:

    \code
        session->open();
        if (session->waitForOpened(1000))
            qDebug("Open!");
    \endcode

    If \a msecs is -1, this function will not time out.

    \sa open(), error()
*/
bool QNetworkSession::waitForOpened(int msecs)
{
    if (!d)
        return false;

    if (d->isOpen)
        return true;

    if (!(d->state == Connecting || d->state == Connected)) {
        return false;
    }

    QEventLoop loop;
    QObject::connect(d, SIGNAL(quitPendingWaitsForOpened()), &loop, SLOT(quit()));
    QObject::connect(this, SIGNAL(error(QNetworkSession::SessionError)), &loop, SLOT(quit()));

    //final call
    if (msecs >= 0)
        QTimer::singleShot(msecs, &loop, SLOT(quit()));

    // enter the event loop and wait for opened/error/timeout
    loop.exec(QEventLoop::ExcludeUserInputEvents | QEventLoop::WaitForMoreEvents);

    return d->isOpen;
}

/*!
    Decreases the session counter on the associated network configuration. If the session counter reaches zero
    the active network interface is shut down. This also means that state() will only change from \l Connected to
    \l Disconnected if the current session was the last open session.

    If the platform does not support out-of-process sessions calling this function does not stop the
    interface. In this case \l{stop()} has to be used to force a shut down. 
    The platform capabilities can be detected via QNetworkConfigurationManager::capabilities().

    Note that this call is asynchronous. Depending on the outcome of this call the results can be enquired 
    by connecting to the stateChanged(), opened() or error() signals.

    \sa open(), stop(), isOpen()
*/
void QNetworkSession::close()
{
    if (d)
        d->close();
}

/*!
    Invalidates all open sessions against the network interface and therefore stops the 
    underlying network interface. This function always changes the session's state() flag to
    \l Disconnected.

    On Symbian platform, a 'NetworkControl' capability is required for
    full interface-level stop (without the capability, only the current session is stopped).

    \sa open(), close()
*/
void QNetworkSession::stop()
{
    if (d)
        d->stop();
}

/*!
    Returns the QNetworkConfiguration that this network session object is based on.

    \sa QNetworkConfiguration
*/
QNetworkConfiguration QNetworkSession::configuration() const
{
    return d ? d->publicConfig : QNetworkConfiguration();
}

#ifndef QT_NO_NETWORKINTERFACE
/*!
    Returns the network interface that is used by this session.

    This function only returns a valid QNetworkInterface when this session is \l Connected.

    The returned interface may change as a result of a roaming process.

    Note: this function does not work in Symbian emulator due to the way the
    connectivity is emulated on Windows.

    \sa state()
*/
QNetworkInterface QNetworkSession::interface() const
{
    return d ? d->currentInterface() : QNetworkInterface();
}
#endif

/*!
    Returns true if this session is open. If the number of all open sessions is greater than
    zero the underlying network interface will remain connected/up.

    The session can be controlled via open() and close().
*/
bool QNetworkSession::isOpen() const
{
    return d ? d->isOpen : false;
}

/*!
    Returns the state of the session. 
    
    If the session is based on a single access point configuration the state of the 
    session is the same as the state of the associated network interface. Therefore
    a network session object can be used to monitor network interfaces. 

    A \l QNetworkConfiguration::ServiceNetwork based session summarizes the state of all its children
    and therefore returns the \l Connected state if at least one of the service network's 
    \l {QNetworkConfiguration::children()}{children()} configurations is active. 

    Note that it is not required to hold an open session in order to obtain the network interface state.
    A connected but closed session may be used to monitor network interfaces whereas an open and connected
    session object may prevent the network interface from being shut down.

    \sa error(), stateChanged()
*/
QNetworkSession::State QNetworkSession::state() const
{
    return d ? d->state : QNetworkSession::Invalid;
}

/*!
    Returns the type of error that last occurred.

    \sa state(), errorString()
*/
QNetworkSession::SessionError QNetworkSession::error() const
{
    return d ? d->error() : InvalidConfigurationError;
}

/*!
    Returns a human-readable description of the last device error that 
    occurred.

    \sa error()
*/
QString QNetworkSession::errorString() const
{
    return d ? d->errorString() : tr("Invalid configuration.");
}

/*!
    Returns the value for property \a key.

    A network session can have properties attached which may describe the session in more details.
    This function can be used to gain access to those properties.

    The following property keys are guaranteed to be specified on all platforms:

    \table
        \header
            \o Key \o Description
        \row
            \o ActiveConfiguration
            \o If the session \l isOpen() this property returns the identifier of the
            QNetworkConfiguration that is used by this session; otherwise an empty string.

            The main purpose of this key is to determine which Internet access point is used
            if the session is based on a \l{QNetworkConfiguration::ServiceNetwork}{ServiceNetwork}. 
            The following code snippet highlights the difference:
            \code
                    QNetworkConfigurationManager mgr;
                    QNetworkConfiguration ap = mgr.defaultConfiguration();
                    QNetworkSession *session = new QNetworkSession(ap);
                    ... //code activates session

                    QString ident = session->sessionProperty("ActiveConfiguration").toString();
                    if ( ap.type() == QNetworkConfiguration::ServiceNetwork ) {
                        Q_ASSERT( ap.identifier() != ident );
                        Q_ASSERT( ap.children().contains( mgr.configurationFromIdentifier(ident) ) );
                    } else if ( ap.type() == QNetworkConfiguration::InternetAccessPoint ) {
                        Q_ASSERT( ap.identifier() == ident );
                    }
                \endcode
        \row
            \o UserChoiceConfiguration
            \o If the session \l isOpen() and is bound to a QNetworkConfiguration of type
            UserChoice, this property returns the identifier of the QNetworkConfiguration that the
            configuration resolved to when \l open() was called; otherwise an empty string.

            The purpose of this key is to determine the real QNetworkConfiguration that the
            session is using. This key is different from \e ActiveConfiguration in that
            this key may return an identifier for either a
            \l {QNetworkConfiguration::ServiceNetwork}{service network} or a
            \l {QNetworkConfiguration::InternetAccessPoint}{Internet access points} configurations,
            whereas \e ActiveConfiguration always returns identifiers to 
            \l {QNetworkConfiguration::InternetAccessPoint}{Internet access points} configurations.
        \row
            \o ConnectInBackground
            \o Setting this property to \e true before calling \l open() implies that the connection attempt
            is made but if no connection can be established, the user is not connsulted and asked to select
            a suitable connection. This property is not set by default and support for it depends on the platform.

        \row
            \o AutoCloseSessionTimeout
            \o If the session requires polling to keep its state up to date, this property holds
               the timeout in milliseconds before the session will automatically close. If the
               value of this property is -1 the session will not automatically close. This property
               is set to -1 by default.

               The purpose of this property is to minimize resource use on platforms that use
               polling to update the state of the session. Applications can set the value of this
               property to the desired timeout before the session is closed. In response to the
               closed() signal the network session should be deleted to ensure that all polling is
               stopped. The session can then be recreated once it is required again. This property
               has no effect for sessions that do not require polling.
    \endtable
*/
QVariant QNetworkSession::sessionProperty(const QString &key) const
{
    if (!d || !d->publicConfig.isValid())
        return QVariant();

    if (key == QLatin1String("ActiveConfiguration"))
        return d->isOpen ? d->activeConfig.identifier() : QString();

    if (key == QLatin1String("UserChoiceConfiguration")) {
        if (!d->isOpen || d->publicConfig.type() != QNetworkConfiguration::UserChoice)
            return QString();

        if (d->serviceConfig.isValid())
            return d->serviceConfig.identifier();
        else
            return d->activeConfig.identifier();
    }

    return d->sessionProperty(key);
}

/*!
    Sets the property \a value on the session. The property is identified using
    \a key. Removing an already set  property can be achieved by passing an 
    invalid QVariant.

    Note that the \e UserChoiceConfiguration and \e ActiveConfiguration
    properties are read only and cannot be changed using this method.
*/
void QNetworkSession::setSessionProperty(const QString &key, const QVariant &value)
{
    if (!d)
        return;

    if (key == QLatin1String("ActiveConfiguration") ||
        key == QLatin1String("UserChoiceConfiguration")) {
        return;
    }

    d->setSessionProperty(key, value);
}

/*!
    Instructs the session to roam to the new access point. The old access point remains active 
    until the application calls accept().

   The newConfigurationActivated() signal is emitted once roaming has been completed.

    \sa accept()
*/
void QNetworkSession::migrate()
{
    if (d)
        d->migrate();
}

/*!
    This function indicates that the application does not wish to roam the session.

    \sa migrate()
*/
void QNetworkSession::ignore()
{
    // Needed on at least Symbian platform: the roaming must be explicitly
    // ignore()'d or migrate()'d
    if (d)
        d->ignore();
}

/*!
    Instructs the session to permanently accept the new access point. Once this function 
    has been called the session may not return to the old access point.

    The old access point may be closed in the process if there are no other network sessions for it.
    Therefore any open socket that still uses the old access point 
    may become unusable and should be closed before completing the migration.
*/
void QNetworkSession::accept()
{
    if (d)
        d->accept();
}

/*!
    The new access point is not suitable for the application. By calling this function the 
    session returns to the previous access point/configuration. This action may invalidate
    any socket that has been created via the not desired access point.

    \sa accept()
*/
void QNetworkSession::reject()
{
    if (d)
        d->reject();
}


/*!
    Returns the amount of data sent in bytes; otherwise 0.

    This field value includes the usage across all open network 
    sessions which use the same network interface.

    If the session is based on a service network configuration the number of 
    sent bytes across all active member configurations are returned.

    This function may not always be supported on all platforms and returns 0.
    The platform capability can be detected via QNetworkConfigurationManager::DataStatistics.

    \note On some platforms this function may run the main event loop.
*/
quint64 QNetworkSession::bytesWritten() const
{
    return d ? d->bytesWritten() : Q_UINT64_C(0);
}

/*!
    Returns the amount of data received in bytes; otherwise 0.

    This field value includes the usage across all open network 
    sessions which use the same network interface.

    If the session is based on a service network configuration the number of 
    sent bytes across all active member configurations are returned.

    This function may not always be supported on all platforms and returns 0.
    The platform capability can be detected via QNetworkConfigurationManager::DataStatistics.

    \note On some platforms this function may run the main event loop.
*/
quint64 QNetworkSession::bytesReceived() const
{
    return d ? d->bytesReceived() : Q_UINT64_C(0);
}

/*!
    Returns the number of seconds that the session has been active.
*/
quint64 QNetworkSession::activeTime() const
{
    return d ? d->activeTime() : Q_UINT64_C(0);
}

/*!
    \internal

    This function is required to detect whether the client wants to control 
    the roaming process. If he connects to preferredConfigurationChanged() signal
    he intends to influence it. Otherwise QNetworkSession always roams
    without registering this session as a stakeholder in the roaming process.

    For more details check the Forced vs ALR roaming section in the QNetworkSession 
    class description.
*/
void QNetworkSession::connectNotify(const char *signal)
{
    QObject::connectNotify(signal);

    if (!d)
        return;

    //check for preferredConfigurationChanged() signal connect notification
    //This is not required on all platforms
    if (qstrcmp(signal, SIGNAL(preferredConfigurationChanged(QNetworkConfiguration,bool))) == 0)
        d->setALREnabled(true);
}

/*!
    \internal

    This function is called when the client disconnects from the
    preferredConfigurationChanged() signal.

    \sa connectNotify()
*/
void QNetworkSession::disconnectNotify(const char *signal)
{
    QObject::disconnectNotify(signal);

    if (!d)
        return;

    //check for preferredConfigurationChanged() signal disconnect notification
    //This is not required on all platforms
    if (qstrcmp(signal, SIGNAL(preferredConfigurationChanged(QNetworkConfiguration,bool))) == 0)
        d->setALREnabled(false);
}

#ifdef Q_OS_SYMBIAN
RConnection* QNetworkSessionPrivate::nativeSession(QNetworkSession &s)
{
    if (!s.d)
        return 0;
    if (s.thread() != QThread::currentThread())
        qWarning("QNetworkSessionPrivate::nativeSession called in wrong thread");
    return s.d->nativeSession();
}

TInt QNetworkSessionPrivate::nativeOpenSocket(QNetworkSession& s, RSocket& sock, TUint family, TUint type, TUint protocol)
{
    if (!s.d)
        return 0;
    QMutexLocker lock(&(s.d->mutex));
    RConnection *con = s.d->nativeSession();
    if (!con || !con->SubSessionHandle())
        return KErrNotReady;
    return sock.Open(qt_symbianGetSocketServer(), family, type, protocol, *con);
}

TInt QNetworkSessionPrivate::nativeOpenHostResolver(QNetworkSession& s, RHostResolver& resolver, TUint family, TUint protocol)
{
    if (!s.d)
        return 0;
    QMutexLocker lock(&(s.d->mutex));
    RConnection *con = s.d->nativeSession();
    if (!con || !con->SubSessionHandle())
        return KErrNotReady;
    return resolver.Open(qt_symbianGetSocketServer(), family, protocol, *con);
}

#endif

#include "moc_qnetworksession.cpp"

QT_END_NAMESPACE

#endif // QT_NO_BEARERMANAGEMENT
