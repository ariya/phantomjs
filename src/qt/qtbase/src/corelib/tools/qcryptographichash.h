/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2013 Richard J. Moore <rich@kde.org>.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QCRYPTOGRAPHICHASH_H
#define QCRYPTOGRAPHICHASH_H

#include <QtCore/qbytearray.h>

QT_BEGIN_NAMESPACE


class QCryptographicHashPrivate;
class QIODevice;

class Q_CORE_EXPORT QCryptographicHash
{
public:
    enum Algorithm {
#ifndef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
        Md4,
        Md5,
#endif
        Sha1 = 2,
#ifndef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
        Sha224,
        Sha256,
        Sha384,
        Sha512,
        Sha3_224,
        Sha3_256,
        Sha3_384,
        Sha3_512
#endif
    };

    explicit QCryptographicHash(Algorithm method);
    ~QCryptographicHash();

    void reset();

    void addData(const char *data, int length);
    void addData(const QByteArray &data);
    bool addData(QIODevice* device);

    QByteArray result() const;

    static QByteArray hash(const QByteArray &data, Algorithm method);
private:
    Q_DISABLE_COPY(QCryptographicHash)
    QCryptographicHashPrivate *d;
};

QT_END_NAMESPACE

#endif
