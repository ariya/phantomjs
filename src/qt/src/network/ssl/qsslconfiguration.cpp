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

#include "qsslconfiguration.h"
#include "qsslconfiguration_p.h"
#include "qsslsocket.h"
#include "qmutex.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

/*!
    \class QSslConfiguration
    \brief The QSslConfiguration class holds the configuration and state of an SSL connection
    \since 4.4

    \reentrant
    \inmodule QtNetwork
    \ingroup network
    \ingroup ssl

    QSslConfiguration is used by Qt networking classes to relay
    information about an open SSL connection and to allow the
    application to control certain features of that connection.

    The settings that QSslConfiguration currently supports are:

    \list
      \o The SSL/TLS protocol to be used
      \o The certificate to be presented to the peer during connection
         and its associated private key
      \o The ciphers allowed to be used for encrypting the connection
      \o The list of Certificate Authorities certificates that are
         used to validate the peer's certificate
    \endlist

    These settings are applied only during the connection
    handshake. Setting them after the connection has been established
    has no effect.

    The state that QSslConfiguration supports are:
    \list
      \o The certificate the peer presented during handshake, along
         with the chain leading to a CA certificate
      \o The cipher used to encrypt this session
    \endlist

    The state can only be obtained once the SSL connection starts, but
    not necessarily before it's done. Some settings may change during
    the course of the SSL connection without need to restart it (for
    instance, the cipher can be changed over time).

    State in QSslConfiguration objects cannot be changed.

    QSslConfiguration can be used with QSslSocket and the Network
    Access API.

    Note that changing settings in QSslConfiguration is not enough to
    change the settings in the related SSL connection. You must call
    setSslConfiguration on a modified QSslConfiguration object to
    achieve that. The following example illustrates how to change the
    protocol to TLSv1 in a QSslSocket object:

    \snippet doc/src/snippets/code/src_network_ssl_qsslconfiguration.cpp 0

    \sa QSsl::SslProtocol, QSslCertificate, QSslCipher, QSslKey
        QSslSocket, QNetworkAccessManager,
        QSslSocket::sslConfiguration(), QSslSocket::setSslConfiguration()
*/

/*!
    Constructs an empty SSL configuration. This configuration contains
    no valid settings and the state will be empty. isNull() will
    return true after this constructor is called.

    Once any setter methods are called, isNull() will return false.
*/
QSslConfiguration::QSslConfiguration()
    : d(new QSslConfigurationPrivate)
{
}

/*!
    Copies the configuration and state of \a other. If \a other is
    null, this object will be null too.
*/
QSslConfiguration::QSslConfiguration(const QSslConfiguration &other)
    : d(other.d)
{
}

/*!
    Releases any resources held by QSslConfiguration.
*/
QSslConfiguration::~QSslConfiguration()
{
    // QSharedDataPointer deletes d for us if necessary
}

/*!
    Copies the configuration and state of \a other. If \a other is
    null, this object will be null too.
*/
QSslConfiguration &QSslConfiguration::operator=(const QSslConfiguration &other)
{
    d = other.d;
    return *this;
}

/*!
    Returns true if this QSslConfiguration object is equal to \a
    other.

    Two QSslConfiguration objects are considered equal if they have
    the exact same settings and state.

    \sa operator!=()
*/
bool QSslConfiguration::operator==(const QSslConfiguration &other) const
{
    if (d == other.d)
        return true;
    return d->peerCertificate == other.d->peerCertificate &&
        d->peerCertificateChain == other.d->peerCertificateChain &&
        d->localCertificate == other.d->localCertificate &&
        d->privateKey == other.d->privateKey &&
        d->sessionCipher == other.d->sessionCipher &&
        d->ciphers == other.d->ciphers &&
        d->caCertificates == other.d->caCertificates &&
        d->protocol == other.d->protocol &&
        d->peerVerifyMode == other.d->peerVerifyMode &&
        d->peerVerifyDepth == other.d->peerVerifyDepth &&
        d->sslOptions == other.d->sslOptions;
}

/*!
    \fn QSslConfiguration::operator!=(const QSslConfiguration &other) const

    Returns true if this QSslConfiguration differs from \a other. Two
    QSslConfiguration objects are considered different if any state or
    setting is different.

    \sa operator==()
*/

/*!
    Returns true if this is a null QSslConfiguration object.

    A QSslConfiguration object is null if it has been
    default-constructed and no setter methods have been called.

    \sa setProtocol(), setLocalCertificate(), setPrivateKey(),
        setCiphers(), setCaCertificates()
*/
bool QSslConfiguration::isNull() const
{
    return (d->protocol == QSsl::SecureProtocols &&
            d->peerVerifyMode == QSslSocket::AutoVerifyPeer &&
            d->peerVerifyDepth == 0 &&
            d->caCertificates.count() == 0 &&
            d->ciphers.count() == 0 &&
            d->localCertificate.isNull() &&
            d->privateKey.isNull() &&
            d->peerCertificate.isNull() &&
            d->peerCertificateChain.count() == 0 &&
            d->sslOptions == (QSsl::SslOptionDisableEmptyFragments|QSsl::SslOptionDisableLegacyRenegotiation));
}

/*!
    Returns the protocol setting for this SSL configuration.

    \sa setProtocol()
*/
QSsl::SslProtocol QSslConfiguration::protocol() const
{
    return d->protocol;
}

/*!
    Sets the protocol setting for this configuration to be \a
    protocol.

    Setting the protocol once the connection has already been
    established has no effect.

    \sa protocol()
*/
void QSslConfiguration::setProtocol(QSsl::SslProtocol protocol)
{
    d->protocol = protocol;
}

/*!
    Returns the verify mode. This mode decides whether QSslSocket should
    request a certificate from the peer (i.e., the client requests a
    certificate from the server, or a server requesting a certificate from the
    client), and whether it should require that this certificate is valid.

    The default mode is AutoVerifyPeer, which tells QSslSocket to use
    VerifyPeer for clients, QueryPeer for servers.

    \sa setPeerVerifyMode()
*/
QSslSocket::PeerVerifyMode QSslConfiguration::peerVerifyMode() const
{
    return d->peerVerifyMode;
}

/*!
    Sets the verify mode to \a mode. This mode decides whether QSslSocket
    should request a certificate from the peer (i.e., the client requests a
    certificate from the server, or a server requesting a certificate from the
    client), and whether it should require that this certificate is valid.

    The default mode is AutoVerifyPeer, which tells QSslSocket to use
    VerifyPeer for clients, QueryPeer for servers.

    \sa peerVerifyMode()
*/
void QSslConfiguration::setPeerVerifyMode(QSslSocket::PeerVerifyMode mode)
{
    d->peerVerifyMode = mode;
}


/*!
    Returns the maximum number of certificates in the peer's certificate chain
    to be checked during the SSL handshake phase, or 0 (the default) if no
    maximum depth has been set, indicating that the whole certificate chain
    should be checked.

    The certificates are checked in issuing order, starting with the peer's
    own certificate, then its issuer's certificate, and so on.

    \sa setPeerVerifyDepth(), peerVerifyMode()
*/
int QSslConfiguration::peerVerifyDepth() const
{
    return d->peerVerifyDepth;
}

/*!
    Sets the maximum number of certificates in the peer's certificate chain to
    be checked during the SSL handshake phase, to \a depth. Setting a depth of
    0 means that no maximum depth is set, indicating that the whole
    certificate chain should be checked.

    The certificates are checked in issuing order, starting with the peer's
    own certificate, then its issuer's certificate, and so on.

    \sa peerVerifyDepth(), setPeerVerifyMode()
*/
void QSslConfiguration::setPeerVerifyDepth(int depth)
{
    if (depth < 0) {
        qWarning("QSslConfiguration::setPeerVerifyDepth: cannot set negative depth of %d", depth);
        return;
    }
    d->peerVerifyDepth = depth;
}

/*!
    Returns the certificate to be presented to the peer during the SSL
    handshake process.

    \sa setLocalCertificate()
*/
QSslCertificate QSslConfiguration::localCertificate() const
{
    return d->localCertificate;
}

/*!
    Sets the certificate to be presented to the peer during SSL
    handshake to be \a certificate.

    Setting the certificate once the connection has been established
    has no effect.

    A certificate is the means of identification used in the SSL
    process. The local certificate is used by the remote end to verify
    the local user's identity against its list of Certification
    Authorities. In most cases, such as in HTTP web browsing, only
    servers identify to the clients, so the client does not send a
    certificate.

    \sa localCertificate()
*/
void QSslConfiguration::setLocalCertificate(const QSslCertificate &certificate)
{
    d->localCertificate = certificate;
}

/*!
    Returns the peer's digital certificate (i.e., the immediate
    certificate of the host you are connected to), or a null
    certificate, if the peer has not assigned a certificate.

    The peer certificate is checked automatically during the
    handshake phase, so this function is normally used to fetch
    the certificate for display or for connection diagnostic
    purposes. It contains information about the peer, including
    its host name, the certificate issuer, and the peer's public
    key.

    Because the peer certificate is set during the handshake phase, it
    is safe to access the peer certificate from a slot connected to
    the QSslSocket::sslErrors() signal, QNetworkReply::sslErrors()
    signal, or the QSslSocket::encrypted() signal.

    If a null certificate is returned, it can mean the SSL handshake
    failed, or it can mean the host you are connected to doesn't have
    a certificate, or it can mean there is no connection.

    If you want to check the peer's complete chain of certificates,
    use peerCertificateChain() to get them all at once.

    \sa peerCertificateChain(),
        QSslSocket::sslErrors(), QSslSocket::ignoreSslErrors(),
        QNetworkReply::sslErrors(), QNetworkReply::ignoreSslErrors()
*/
QSslCertificate QSslConfiguration::peerCertificate() const
{
    return d->peerCertificate;
}

/*!
    Returns the peer's chain of digital certificates, starting with
    the peer's immediate certificate and ending with the CA's
    certificate.

    Peer certificates are checked automatically during the handshake
    phase. This function is normally used to fetch certificates for
    display, or for performing connection diagnostics. Certificates
    contain information about the peer and the certificate issuers,
    including host name, issuer names, and issuer public keys.

    Because the peer certificate is set during the handshake phase, it
    is safe to access the peer certificate from a slot connected to
    the QSslSocket::sslErrors() signal, QNetworkReply::sslErrors()
    signal, or the QSslSocket::encrypted() signal.

    If an empty list is returned, it can mean the SSL handshake
    failed, or it can mean the host you are connected to doesn't have
    a certificate, or it can mean there is no connection.

    If you want to get only the peer's immediate certificate, use
    peerCertificate().

    \sa peerCertificate(),
        QSslSocket::sslErrors(), QSslSocket::ignoreSslErrors(),
        QNetworkReply::sslErrors(), QNetworkReply::ignoreSslErrors()
*/
QList<QSslCertificate> QSslConfiguration::peerCertificateChain() const
{
    return d->peerCertificateChain;
}

/*!
    Returns the socket's cryptographic \l {QSslCipher} {cipher}, or a
    null cipher if the connection isn't encrypted. The socket's cipher
    for the session is set during the handshake phase. The cipher is
    used to encrypt and decrypt data transmitted through the socket.

    The SSL infrastructure also provides functions for setting the
    ordered list of ciphers from which the handshake phase will
    eventually select the session cipher. This ordered list must be in
    place before the handshake phase begins.

    \sa ciphers(), setCiphers(), QSslSocket::supportedCiphers()
*/
QSslCipher QSslConfiguration::sessionCipher() const
{
    return d->sessionCipher;
}

/*!
    Returns the \l {QSslKey} {SSL key} assigned to this connection or
    a null key if none has been assigned yet.

    \sa setPrivateKey(), localCertificate()
*/
QSslKey QSslConfiguration::privateKey() const
{
    return d->privateKey;
}

/*!
    Sets the connection's private \l {QSslKey} {key} to \a key. The
    private key and the local \l {QSslCertificate} {certificate} are
    used by clients and servers that must prove their identity to
    SSL peers.

    Both the key and the local certificate are required if you are
    creating an SSL server socket. If you are creating an SSL client
    socket, the key and local certificate are required if your client
    must identify itself to an SSL server.

    \sa privateKey(), setLocalCertificate()
*/
void QSslConfiguration::setPrivateKey(const QSslKey &key)
{
    d->privateKey = key;
}

/*!
    Returns this connection's current cryptographic cipher suite. This
    list is used during the handshake phase for choosing a
    session cipher. The returned list of ciphers is ordered by
    descending preference. (i.e., the first cipher in the list is the
    most preferred cipher). The session cipher will be the first one
    in the list that is also supported by the peer.

    By default, the handshake phase can choose any of the ciphers
    supported by this system's SSL libraries, which may vary from
    system to system. The list of ciphers supported by this system's
    SSL libraries is returned by QSslSocket::supportedCiphers(). You can restrict
    the list of ciphers used for choosing the session cipher for this
    socket by calling setCiphers() with a subset of the supported
    ciphers. You can revert to using the entire set by calling
    setCiphers() with the list returned by QSslSocket::supportedCiphers().

    \sa setCiphers(), QSslSocket::supportedCiphers()
*/
QList<QSslCipher> QSslConfiguration::ciphers() const
{
    return d->ciphers;
}

/*!
    Sets the cryptographic cipher suite for this socket to \a ciphers,
    which must contain a subset of the ciphers in the list returned by
    supportedCiphers().

    Restricting the cipher suite must be done before the handshake
    phase, where the session cipher is chosen.

    \sa ciphers(), QSslSocket::supportedCiphers()
*/
void QSslConfiguration::setCiphers(const QList<QSslCipher> &ciphers)
{
    d->ciphers = ciphers;
}

/*!
  Returns this connection's CA certificate database. The CA certificate
  database is used by the socket during the handshake phase to
  validate the peer's certificate. It can be modified prior to the
  handshake with setCaCertificates(), or with \l{QSslSocket}'s
  \l{QSslSocket::}{addCaCertificate()} and
  \l{QSslSocket::}{addCaCertificates()}.

  \sa setCaCertificates()
*/
QList<QSslCertificate> QSslConfiguration::caCertificates() const
{
    return d->caCertificates;
}

/*!
  Sets this socket's CA certificate database to be \a certificates.
  The certificate database must be set prior to the SSL handshake.
  The CA certificate database is used by the socket during the
  handshake phase to validate the peer's certificate.

  \sa caCertificates()
*/
void QSslConfiguration::setCaCertificates(const QList<QSslCertificate> &certificates)
{
    d->caCertificates = certificates;
}

/*!
  Enables or disables an SSL compatibility option.

  \sa testSSlOption()
*/
void QSslConfiguration::setSslOption(QSsl::SslOption option, bool on)
{
    if (on)
        d->sslOptions |= option;
    else
        d->sslOptions &= ~option;
}

/*!
  Returns true if the specified SSL compatibility option is enabled.

  \sa testSSlOption()
*/
bool QSslConfiguration::testSslOption(QSsl::SslOption option) const
{
    return d->sslOptions & option;
}

/*!
    Returns the default SSL configuration to be used in new SSL
    connections.

    The default SSL configuration consists of:

    \list
      \o no local certificate and no private key
      \o protocol SecureProtocols (meaning either TLS 1.0 or SSL 3 will be used)
      \o the system's default CA certificate list
      \o the cipher list equal to the list of the SSL libraries'
         supported SSL ciphers
    \endlist

    \sa QSslSocket::supportedCiphers(), setDefaultConfiguration()
*/
QSslConfiguration QSslConfiguration::defaultConfiguration()
{
    return QSslConfigurationPrivate::defaultConfiguration();
}

/*!
    Sets the default SSL configuration to be used in new SSL
    connections to be \a configuration. Existing connections are not
    affected by this call.

    \sa QSslSocket::supportedCiphers(), defaultConfiguration()
*/
void QSslConfiguration::setDefaultConfiguration(const QSslConfiguration &configuration)
{
    QSslConfigurationPrivate::setDefaultConfiguration(configuration);
}

QT_END_NAMESPACE
