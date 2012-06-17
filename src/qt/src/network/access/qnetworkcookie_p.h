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

#ifndef QNETWORKCOOKIE_P_H
#define QNETWORKCOOKIE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access framework.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qdatetime.h"

QT_BEGIN_NAMESPACE

class QNetworkCookiePrivate: public QSharedData
{
public:
    inline QNetworkCookiePrivate() : secure(false), httpOnly(false) { }
    static QList<QNetworkCookie> parseSetCookieHeaderLine(const QByteArray &cookieString);

    QDateTime expirationDate;
    QString domain;
    QString path;
    QString comment;
    QByteArray name;
    QByteArray value;
    bool secure;
    bool httpOnly;
};

static inline bool isLWS(register char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static int nextNonWhitespace(const QByteArray &text, int from)
{
    // RFC 2616 defines linear whitespace as:
    //  LWS = [CRLF] 1*( SP | HT )
    // We ignore the fact that CRLF must come as a pair at this point
    // It's an invalid HTTP header if that happens.
    while (from < text.length()) {
        if (isLWS(text.at(from)))
            ++from;
        else
            return from;        // non-whitespace
    }

    // reached the end
    return text.length();
}

QT_END_NAMESPACE

#endif
