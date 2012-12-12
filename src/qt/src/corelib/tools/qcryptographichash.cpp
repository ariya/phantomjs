/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include <qcryptographichash.h>

#ifdef Q_OS_SYMBIAN
#define _MD5_H_ // Needed to disable system header
#endif

#include "../../3rdparty/md5/md5.h"
#include "../../3rdparty/md5/md5.cpp"
#include "../../3rdparty/md4/md4.h"
#include "../../3rdparty/md4/md4.cpp"
#include "../../3rdparty/sha1/sha1.cpp"


QT_BEGIN_NAMESPACE

class QCryptographicHashPrivate
{
public:
    QCryptographicHash::Algorithm method;
    union {
        MD5Context md5Context;
        md4_context md4Context;
        Sha1State sha1Context;
    };
    QByteArray result;
};

/*!
  \class QCryptographicHash

  \brief The QCryptographicHash class provides a way to generate cryptographic hashes.

  \since 4.3

  \ingroup tools
  \reentrant

  QCryptographicHash can be used to generate cryptographic hashes of binary or text data.

  Currently MD4, MD5, and SHA-1 are supported.
*/

/*!
  \enum QCryptographicHash::Algorithm

  \value Md4 Generate an MD4 hash sum
  \value Md5 Generate an MD5 hash sum
  \value Sha1 Generate an SHA1 hash sum
*/

/*!
  Constructs an object that can be used to create a cryptographic hash from data using \a method.
*/
QCryptographicHash::QCryptographicHash(Algorithm method)
    : d(new QCryptographicHashPrivate)
{
    d->method = method;
    reset();
}

/*!
  Destroys the object.
*/
QCryptographicHash::~QCryptographicHash()
{
    delete d;
}

/*!
  Resets the object.
*/
void QCryptographicHash::reset()
{
    switch (d->method) {
    case Md4:
        md4_init(&d->md4Context);
        break;
    case Md5:
        MD5Init(&d->md5Context);
        break;
    case Sha1:
        sha1InitState(&d->sha1Context);
        break;
    }
    d->result.clear();
}

/*!
    Adds the first \a length chars of \a data to the cryptographic
    hash.
*/
void QCryptographicHash::addData(const char *data, int length)
{
    switch (d->method) {
    case Md4:
        md4_update(&d->md4Context, (const unsigned char *)data, length);
        break;
    case Md5:
        MD5Update(&d->md5Context, (const unsigned char *)data, length);
        break;
    case Sha1:
        sha1Update(&d->sha1Context, (const unsigned char *)data, length);
        break;
    }    
    d->result.clear();
}

/*!
  \overload addData()
*/
void QCryptographicHash::addData(const QByteArray &data)
{
    addData(data.constData(), data.length());
}

/*!
  Returns the final hash value.

  \sa QByteArray::toHex()
*/
QByteArray QCryptographicHash::result() const
{
    if (!d->result.isEmpty()) 
        return d->result;

    switch (d->method) {
    case Md4: {
        md4_context copy = d->md4Context;
        d->result.resize(MD4_RESULTLEN);
        md4_final(&copy, (unsigned char *)d->result.data());
        break;
    }
    case Md5: {
        MD5Context copy = d->md5Context;
        d->result.resize(16);
        MD5Final(&copy, (unsigned char *)d->result.data());
        break;
    }
    case Sha1: {
        Sha1State copy = d->sha1Context;
        d->result.resize(20);
        sha1FinalizeState(&copy);
        sha1ToHash(&copy, (unsigned char *)d->result.data());
    }
    }
    return d->result;
}

/*!
  Returns the hash of \a data using \a method.
*/
QByteArray QCryptographicHash::hash(const QByteArray &data, Algorithm method)
{
    QCryptographicHash hash(method);
    hash.addData(data);
    return hash.result();
}

QT_END_NAMESPACE
