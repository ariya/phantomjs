/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QNETWORKCOOKIE_H
#define QNETWORKCOOKIE_H

#include <QtCore/QSharedDataPointer>
#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QObject>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Network)

class QByteArray;
class QDateTime;
class QString;
class QUrl;

class QNetworkCookiePrivate;
class Q_NETWORK_EXPORT QNetworkCookie
{
public:
    enum RawForm {
        NameAndValueOnly,
        Full
    };

    QNetworkCookie(const QByteArray &name = QByteArray(), const QByteArray &value = QByteArray());
    QNetworkCookie(const QNetworkCookie &other);
    ~QNetworkCookie();
    QNetworkCookie &operator=(const QNetworkCookie &other);
    bool operator==(const QNetworkCookie &other) const;
    inline bool operator!=(const QNetworkCookie &other) const
    { return !(*this == other); }

    bool isSecure() const;
    void setSecure(bool enable);
    bool isHttpOnly() const;
    void setHttpOnly(bool enable);

    bool isSessionCookie() const;
    QDateTime expirationDate() const;
    void setExpirationDate(const QDateTime &date);

    QString domain() const;
    void setDomain(const QString &domain);

    QString path() const;
    void setPath(const QString &path);

    QByteArray name() const;
    void setName(const QByteArray &cookieName);

    QByteArray value() const;
    void setValue(const QByteArray &value);

    QByteArray toRawForm(RawForm form = Full) const;

    static QList<QNetworkCookie> parseCookies(const QByteArray &cookieString);

private:
    QSharedDataPointer<QNetworkCookiePrivate> d;
    friend class QNetworkCookiePrivate;
};
Q_DECLARE_TYPEINFO(QNetworkCookie, Q_MOVABLE_TYPE);

#ifndef QT_NO_DEBUG_STREAM
class QDebug;
Q_NETWORK_EXPORT QDebug operator<<(QDebug, const QNetworkCookie &);
#endif

QT_END_NAMESPACE

// ### Qt5 remove this include
#include <QtNetwork/QNetworkCookieJar>

Q_DECLARE_METATYPE(QNetworkCookie)
Q_DECLARE_METATYPE(QList<QNetworkCookie>)

QT_END_HEADER

#endif
