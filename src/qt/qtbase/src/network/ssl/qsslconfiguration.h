/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

/****************************************************************************
**
** In addition, as a special exception, the copyright holders listed above give
** permission to link the code of its release of Qt with the OpenSSL project's
** "OpenSSL" library (or modified versions of the "OpenSSL" library that use the
** same license as the original version), and distribute the linked executables.
**
** You must comply with the GNU General Public License version 2 in all
** respects for all of the code used other than the "OpenSSL" code.  If you
** modify this file, you may extend this exception to your version of the file,
** but you are not obligated to do so.  If you do not wish to do so, delete
** this exception statement from your version of this file.
**
****************************************************************************/

#ifndef QSSLCONFIGURATION_H
#define QSSLCONFIGURATION_H

#include <QtCore/qshareddata.h>
#include <QtNetwork/qsslsocket.h>
#include <QtNetwork/qssl.h>

#ifndef QT_NO_SSL

QT_BEGIN_NAMESPACE

template<typename T> class QList;
class QSslCertificate;
class QSslCipher;
class QSslKey;

class QSslConfigurationPrivate;
class Q_NETWORK_EXPORT QSslConfiguration
{
public:
    QSslConfiguration();
    QSslConfiguration(const QSslConfiguration &other);
    ~QSslConfiguration();
    QSslConfiguration &operator=(const QSslConfiguration &other);

    inline void swap(QSslConfiguration &other)
    { qSwap(d, other.d); }

    bool operator==(const QSslConfiguration &other) const;
    inline bool operator!=(const QSslConfiguration &other) const
    { return !(*this == other); }

    bool isNull() const;

    QSsl::SslProtocol protocol() const;
    void setProtocol(QSsl::SslProtocol protocol);

    // Verification
    QSslSocket::PeerVerifyMode peerVerifyMode() const;
    void setPeerVerifyMode(QSslSocket::PeerVerifyMode mode);

    int peerVerifyDepth() const;
    void setPeerVerifyDepth(int depth);

    // Certificate & cipher configuration
    QList<QSslCertificate> localCertificateChain() const;
    void setLocalCertificateChain(const QList<QSslCertificate> &localChain);

    QSslCertificate localCertificate() const;
    void setLocalCertificate(const QSslCertificate &certificate);

    QSslCertificate peerCertificate() const;
    QList<QSslCertificate> peerCertificateChain() const;
    QSslCipher sessionCipher() const;
    QSsl::SslProtocol sessionProtocol() const;

    // Private keys, for server sockets
    QSslKey privateKey() const;
    void setPrivateKey(const QSslKey &key);

    // Cipher settings
    QList<QSslCipher> ciphers() const;
    void setCiphers(const QList<QSslCipher> &ciphers);

    // Certificate Authority (CA) settings
    QList<QSslCertificate> caCertificates() const;
    void setCaCertificates(const QList<QSslCertificate> &certificates);

    void setSslOption(QSsl::SslOption option, bool on);
    bool testSslOption(QSsl::SslOption option) const;

    QByteArray sessionTicket() const;
    void setSessionTicket(const QByteArray &sessionTicket);
    int sessionTicketLifeTimeHint() const;

    static QSslConfiguration defaultConfiguration();
    static void setDefaultConfiguration(const QSslConfiguration &configuration);

    enum NextProtocolNegotiationStatus {
        NextProtocolNegotiationNone,
        NextProtocolNegotiationNegotiated,
        NextProtocolNegotiationUnsupported
    };

    void setAllowedNextProtocols(QList<QByteArray> protocols);
    QList<QByteArray> allowedNextProtocols() const;

    QByteArray nextNegotiatedProtocol() const;
    NextProtocolNegotiationStatus nextProtocolNegotiationStatus() const;

    static const char NextProtocolSpdy3_0[];
    static const char NextProtocolHttp1_1[];

private:
    friend class QSslSocket;
    friend class QSslConfigurationPrivate;
    friend class QSslSocketBackendPrivate;
    friend class QSslContext;
    QSslConfiguration(QSslConfigurationPrivate *dd);
    QSharedDataPointer<QSslConfigurationPrivate> d;
};

Q_DECLARE_SHARED(QSslConfiguration)

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QSslConfiguration)

#endif  // QT_NO_SSL

#endif
