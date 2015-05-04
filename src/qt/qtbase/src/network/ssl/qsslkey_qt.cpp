/****************************************************************************
**
** Copyright (C) 2014 Jeremy Lainé <jeremy.laine@m4x.org>
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

#include "qsslkey.h"
#include "qsslkey_p.h"
#include "qasn1element_p.h"

#include <QtCore/qcryptographichash.h>

QT_USE_NAMESPACE

static const quint8 bits_table[256] = {
    0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
};

static int numberOfBits(const QByteArray &modulus)
{
    int bits = modulus.size() * 8;
    for (int i = 0; i < modulus.size(); ++i) {
        quint8 b = modulus[i];
        bits -= 8;
        if (b != 0) {
            bits += bits_table[b];
            break;
        }
    }
    return bits;
}

static QByteArray deriveKey(QSslKeyPrivate::Cipher cipher, const QByteArray &passPhrase, const QByteArray &iv)
{
    QByteArray key;
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(passPhrase);
    hash.addData(iv);
    switch (cipher) {
    case QSslKeyPrivate::DesCbc:
        key = hash.result().left(8);
        break;
    case QSslKeyPrivate::DesEde3Cbc:
        key = hash.result();
        hash.reset();
        hash.addData(key);
        hash.addData(passPhrase);
        hash.addData(iv);
        key += hash.result().left(8);
        break;
    case QSslKeyPrivate::Rc2Cbc:
        key = hash.result();
        break;
    }
    return key;
}

void QSslKeyPrivate::clear(bool deep)
{
    Q_UNUSED(deep);
    isNull = true;
    derData.clear();
    keyLength = -1;
}

void QSslKeyPrivate::decodeDer(const QByteArray &der, bool deepClear)
{
    clear(deepClear);

    if (der.isEmpty())
        return;

    QAsn1Element elem;
    if (!elem.read(der) || elem.type() != QAsn1Element::SequenceType)
        return;

    if (type == QSsl::PublicKey) {
        // key info
        QDataStream keyStream(elem.value());
        if (!elem.read(keyStream) || elem.type() != QAsn1Element::SequenceType)
            return;
        QVector<QAsn1Element> infoItems = elem.toVector();
        if (infoItems.size() < 2 || infoItems[0].type() != QAsn1Element::ObjectIdentifierType)
            return;
        if (algorithm == QSsl::Rsa) {
            if (infoItems[0].toObjectId() != RSA_ENCRYPTION_OID)
                return;
            // key data
            if (!elem.read(keyStream) || elem.type() != QAsn1Element::BitStringType || elem.value().isEmpty())
                return;
            if (!elem.read(elem.value().mid(1)) || elem.type() != QAsn1Element::SequenceType)
                return;
            if (!elem.read(elem.value()) || elem.type() != QAsn1Element::IntegerType)
                return;
            keyLength = numberOfBits(elem.value());
        } else if (algorithm == QSsl::Dsa) {
            if (infoItems[0].toObjectId() != DSA_ENCRYPTION_OID)
                return;
            if (infoItems[1].type() != QAsn1Element::SequenceType)
                return;
            // key params
            QVector<QAsn1Element> params = infoItems[1].toVector();
            if (params.isEmpty() || params[0].type() != QAsn1Element::IntegerType)
                return;
            keyLength = numberOfBits(params[0].value());
        }

    } else {
        QVector<QAsn1Element> items = elem.toVector();
        if (items.isEmpty())
            return;

        // version
        if (items[0].type() != QAsn1Element::IntegerType || items[0].value().toHex() != "00")
            return;

        if (algorithm == QSsl::Rsa) {
            if (items.size() != 9 || items[1].type() != QAsn1Element::IntegerType)
                return;
            keyLength = numberOfBits(items[1].value());
        } else if (algorithm == QSsl::Dsa) {
            if (items.size() != 6 || items[1].type() != QAsn1Element::IntegerType)
                return;
            keyLength = numberOfBits(items[1].value());
        }
    }

    derData = der;
    isNull = false;
}

void QSslKeyPrivate::decodePem(const QByteArray &pem, const QByteArray &passPhrase,
                               bool deepClear)
{
    QMap<QByteArray, QByteArray> headers;
    QByteArray data = derFromPem(pem, &headers);
    if (headers.value("Proc-Type") == "4,ENCRYPTED") {
        QList<QByteArray> dekInfo = headers.value("DEK-Info").split(',');
        if (dekInfo.size() != 2) {
            clear(deepClear);
            return;
        }

        Cipher cipher;
        if (dekInfo.first() == "DES-CBC") {
            cipher = DesCbc;
        } else if (dekInfo.first() == "DES-EDE3-CBC") {
            cipher = DesEde3Cbc;
        } else if (dekInfo.first() == "RC2-CBC") {
            cipher = Rc2Cbc;
        } else {
            clear(deepClear);
            return;
        }

        const QByteArray iv = QByteArray::fromHex(dekInfo.last());
        const QByteArray key = deriveKey(cipher, passPhrase, iv);
        data = decrypt(cipher, data, key, iv);
    }
    decodeDer(data, deepClear);
}

int QSslKeyPrivate::length() const
{
    return keyLength;
}

QByteArray QSslKeyPrivate::toPem(const QByteArray &passPhrase) const
{
    QByteArray data;
    QMap<QByteArray, QByteArray> headers;

    if (type == QSsl::PrivateKey && !passPhrase.isEmpty()) {
        // ### use a cryptographically secure random number generator
        QByteArray iv;
        iv.resize(8);
        for (int i = 0; i < iv.size(); ++i)
            iv[i] = (qrand() & 0xff);

        Cipher cipher = DesEde3Cbc;
        const QByteArray key = deriveKey(cipher, passPhrase, iv);
        data = encrypt(cipher, derData, key, iv);

        headers.insert("Proc-Type", "4,ENCRYPTED");
        headers.insert("DEK-Info", "DES-EDE3-CBC," + iv.toHex());
    } else {
        data = derData;
    }

    return pemFromDer(data, headers);
}

Qt::HANDLE QSslKeyPrivate::handle() const
{
    return opaque;
}
