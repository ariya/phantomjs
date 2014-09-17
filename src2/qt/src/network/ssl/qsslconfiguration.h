/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

/****************************************************************************
**
** In addition, as a special exception, Nokia gives permission to link
** the code of its release of Qt with the OpenSSL project's "OpenSSL" library
** (or modified versions of the "OpenSSL" library that use the same license
** as the original version), and distribute the linked executables.
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

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Network)

#ifndef QT_NO_OPENSSL

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

    bool operator==(const QSslConfiguration &other) const;
    inline bool operator!=(const QSslConfiguration &other) const
    { return !(*this == other); }

    bool isNull() const; // ### Qt 5: remove; who would need this?

    QSsl::SslProtocol protocol() const;
    void setProtocol(QSsl::SslProtocol protocol);

    // Verification
    QSslSocket::PeerVerifyMode peerVerifyMode() const;
    void setPeerVerifyMode(QSslSocket::PeerVerifyMode mode);

    int peerVerifyDepth() const;
    void setPeerVerifyDepth(int depth);

    // Certificate & cipher configuration
    QSslCertificate localCertificate() const;
    void setLocalCertificate(const QSslCertificate &certificate);

    QSslCertificate peerCertificate() const;
    QList<QSslCertificate> peerCertificateChain() const;
    QSslCipher sessionCipher() const;

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

    static QSslConfiguration defaultConfiguration();
    static void setDefaultConfiguration(const QSslConfiguration &configuration);

private:
    friend class QSslSocket;
    friend class QSslConfigurationPrivate;
    QSslConfiguration(QSslConfigurationPrivate *dd);
    QSharedDataPointer<QSslConfigurationPrivate> d;
};

#endif  // QT_NO_OPENSSL

QT_END_NAMESPACE

QT_END_HEADER

#endif
