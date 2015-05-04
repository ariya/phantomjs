/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QSSLSOCKET_WINRT_P_H
#define QSSLSOCKET_WINRT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QtNetwork library.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "qsslsocket_p.h"

#include <wrl.h>
#include <windows.networking.sockets.h>

QT_BEGIN_NAMESPACE

class QSslSocketConnectionHelper : public QObject
{
    Q_OBJECT
public:
    QSslSocketConnectionHelper(QSslSocketBackendPrivate *d)
        : d(d) { }

    Q_INVOKABLE void disconnectSocketFromHost();

private:
    QSslSocketBackendPrivate *d;
};

class QSslSocketBackendPrivate : public QSslSocketPrivate
{
    Q_DECLARE_PUBLIC(QSslSocket)
public:
    QSslSocketBackendPrivate();
    ~QSslSocketBackendPrivate();

    // Platform specific functions
    void startClientEncryption() Q_DECL_OVERRIDE;
    void startServerEncryption() Q_DECL_OVERRIDE;
    void transmit() Q_DECL_OVERRIDE;
    void disconnectFromHost() Q_DECL_OVERRIDE;
    void disconnected() Q_DECL_OVERRIDE;
    QSslCipher sessionCipher() const Q_DECL_OVERRIDE;
    QSsl::SslProtocol sessionProtocol() const Q_DECL_OVERRIDE;
    void continueHandshake() Q_DECL_OVERRIDE;

    static QList<QSslCipher> defaultCiphers();
    static QList<QSslError> verify(QList<QSslCertificate> certificateChain, const QString &hostName);
    static bool importPkcs12(QIODevice *device,
                             QSslKey *key, QSslCertificate *cert,
                             QList<QSslCertificate> *caCertificates,
                             const QByteArray &passPhrase);

private:
    HRESULT onSslUpgrade(ABI::Windows::Foundation::IAsyncAction *,
                         ABI::Windows::Foundation::AsyncStatus);

    QScopedPointer<QSslSocketConnectionHelper> connectionHelper;
    ABI::Windows::Networking::Sockets::SocketProtectionLevel protectionLevel;
    QSet<QSslCertificate> previousCaCertificates;
};

QT_END_NAMESPACE

#endif // QSSLSOCKET_WINRT_P_H
