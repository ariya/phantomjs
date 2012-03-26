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

#ifndef QNETWORKREQUEST_H
#define QNETWORKREQUEST_H

#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QVariant>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Network)

class QSslConfiguration;

class QNetworkRequestPrivate;
class Q_NETWORK_EXPORT QNetworkRequest
{
public:
    enum KnownHeaders {
        ContentTypeHeader,
        ContentLengthHeader,
        LocationHeader,
        LastModifiedHeader,
        CookieHeader,
        SetCookieHeader,
        ContentDispositionHeader  // added for QMultipartMessage
    };
    enum Attribute {
        HttpStatusCodeAttribute,
        HttpReasonPhraseAttribute,
        RedirectionTargetAttribute,
        ConnectionEncryptedAttribute,
        CacheLoadControlAttribute,
        CacheSaveControlAttribute,
        SourceIsFromCacheAttribute,
        DoNotBufferUploadDataAttribute,
        HttpPipeliningAllowedAttribute,
        HttpPipeliningWasUsedAttribute,
        CustomVerbAttribute,
        CookieLoadControlAttribute,
        AuthenticationReuseAttribute,
        CookieSaveControlAttribute,
        MaximumDownloadBufferSizeAttribute, // internal
        DownloadBufferAttribute, // internal
        SynchronousRequestAttribute, // internal

        User = 1000,
        UserMax = 32767
    };
    enum CacheLoadControl {
        AlwaysNetwork,
        PreferNetwork,
        PreferCache,
        AlwaysCache
    };
    enum LoadControl {
        Automatic = 0,
        Manual
    };

    enum Priority {
        HighPriority = 1,
        NormalPriority = 3,
        LowPriority = 5
    };

    explicit QNetworkRequest(const QUrl &url = QUrl());
    QNetworkRequest(const QNetworkRequest &other);
    ~QNetworkRequest();
    QNetworkRequest &operator=(const QNetworkRequest &other);

    bool operator==(const QNetworkRequest &other) const;
    inline bool operator!=(const QNetworkRequest &other) const
    { return !operator==(other); }

    QUrl url() const;
    void setUrl(const QUrl &url);

    // "cooked" headers
    QVariant header(KnownHeaders header) const;
    void setHeader(KnownHeaders header, const QVariant &value);

    // raw headers:
    bool hasRawHeader(const QByteArray &headerName) const;
    QList<QByteArray> rawHeaderList() const;
    QByteArray rawHeader(const QByteArray &headerName) const;
    void setRawHeader(const QByteArray &headerName, const QByteArray &value);

    // attributes
    QVariant attribute(Attribute code, const QVariant &defaultValue = QVariant()) const;
    void setAttribute(Attribute code, const QVariant &value);

#ifndef QT_NO_OPENSSL
    QSslConfiguration sslConfiguration() const;
    void setSslConfiguration(const QSslConfiguration &configuration);
#endif

    void setOriginatingObject(QObject *object);
    QObject *originatingObject() const;

    Priority priority() const;
    void setPriority(Priority priority);

private:
    QSharedDataPointer<QNetworkRequestPrivate> d;
    friend class QNetworkRequestPrivate;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QNetworkRequest)

QT_END_HEADER

#endif
