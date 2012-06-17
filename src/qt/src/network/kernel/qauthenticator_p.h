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

#ifndef QAUTHENTICATOR_P_H
#define QAUTHENTICATOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qhash.h>
#include <qbytearray.h>
#include <qstring.h>
#include <qauthenticator.h>
#include <qvariant.h>

QT_BEGIN_NAMESPACE

class QHttpResponseHeader;

class Q_AUTOTEST_EXPORT QAuthenticatorPrivate
{
public:
    enum Method { None, Basic, Plain, Login, Ntlm, CramMd5, DigestMd5 };
    QAuthenticatorPrivate();

    QAtomicInt ref;
    QString user;
    QString extractedUser;
    QString password;
    QVariantHash options;
    Method method;
    QString realm;
    QByteArray challenge;
    bool hasFailed; //credentials have been tried but rejected by server.

    enum Phase {
        Start,
        Phase2,
        Done,
        Invalid
    };
    Phase phase;

    // digest specific
    QByteArray cnonce;
    int nonceCount;

    // ntlm specific
    QString workstation;
    QString userDomain;

    QByteArray calculateResponse(const QByteArray &method, const QByteArray &path);

    inline static QAuthenticatorPrivate *getPrivate(QAuthenticator &auth) { return auth.d; }
    inline static const QAuthenticatorPrivate *getPrivate(const QAuthenticator &auth) { return auth.d; }

    QByteArray digestMd5Response(const QByteArray &challenge, const QByteArray &method, const QByteArray &path);
    static QHash<QByteArray, QByteArray> parseDigestAuthenticationChallenge(const QByteArray &challenge);

#ifndef QT_NO_HTTP
    void parseHttpResponse(const QHttpResponseHeader &, bool isProxy);
#endif
    void parseHttpResponse(const QList<QPair<QByteArray, QByteArray> >&, bool isProxy);

};


QT_END_NAMESPACE

#endif
