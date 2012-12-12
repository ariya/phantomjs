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


#ifndef QSSL_H
#define QSSL_H

#include <QtCore/qglobal.h>
#include <QtCore/QFlags>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Network)

namespace QSsl {
    enum KeyType {
        PrivateKey,
        PublicKey
    };

    enum EncodingFormat {
        Pem,
        Der
    };

    enum KeyAlgorithm {
        Rsa,
        Dsa
    };

    enum AlternateNameEntryType {
        EmailEntry,
        DnsEntry
    };

    enum SslProtocol {
        SslV3,
        SslV2,
        TlsV1, // ### Qt 5: rename to TlsV1_0 or so
        AnyProtocol,
        TlsV1SslV3,
        SecureProtocols,
        UnknownProtocol = -1
    };

    enum SslOption {
        SslOptionDisableEmptyFragments = 0x01,
        SslOptionDisableSessionTickets = 0x02,
        SslOptionDisableCompression = 0x04,
        SslOptionDisableServerNameIndication = 0x08,
        SslOptionDisableLegacyRenegotiation = 0x10
    };
    Q_DECLARE_FLAGS(SslOptions, SslOption)
}

Q_DECLARE_OPERATORS_FOR_FLAGS(QSsl::SslOptions)

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSSL_H
