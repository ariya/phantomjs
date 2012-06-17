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

#ifndef QABSTRACTNETWORKCACHE_H
#define QABSTRACTNETWORKCACHE_H

#include <QtCore/qobject.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qpair.h>
#include <QtNetwork/qnetworkrequest.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Network)

class QIODevice;
class QDateTime;
class QUrl;
template<class T> class QList;

class QNetworkCacheMetaDataPrivate;
class Q_NETWORK_EXPORT QNetworkCacheMetaData
{

public:
    typedef QPair<QByteArray, QByteArray> RawHeader;
    typedef QList<RawHeader> RawHeaderList;
    typedef QHash<QNetworkRequest::Attribute, QVariant> AttributesMap;

    QNetworkCacheMetaData();
    QNetworkCacheMetaData(const QNetworkCacheMetaData &other);
    ~QNetworkCacheMetaData();

    QNetworkCacheMetaData &operator=(const QNetworkCacheMetaData &other);
    bool operator==(const QNetworkCacheMetaData &other) const;
    inline bool operator!=(const QNetworkCacheMetaData &other) const
        { return !(*this == other); }

    bool isValid() const;

    QUrl url() const;
    void setUrl(const QUrl &url);

    RawHeaderList rawHeaders() const;
    void setRawHeaders(const RawHeaderList &headers);

    QDateTime lastModified() const;
    void setLastModified(const QDateTime &dateTime);

    QDateTime expirationDate() const;
    void setExpirationDate(const QDateTime &dateTime);

    bool saveToDisk() const;
    void setSaveToDisk(bool allow);

    AttributesMap attributes() const;
    void setAttributes(const AttributesMap &attributes);

private:
    friend class QNetworkCacheMetaDataPrivate;
    QSharedDataPointer<QNetworkCacheMetaDataPrivate> d;
};

Q_NETWORK_EXPORT QDataStream &operator<<(QDataStream &, const QNetworkCacheMetaData &);
Q_NETWORK_EXPORT QDataStream &operator>>(QDataStream &, QNetworkCacheMetaData &);


class QAbstractNetworkCachePrivate;
class Q_NETWORK_EXPORT QAbstractNetworkCache : public QObject
{
    Q_OBJECT

public:
    virtual ~QAbstractNetworkCache();

    virtual QNetworkCacheMetaData metaData(const QUrl &url) = 0;
    virtual void updateMetaData(const QNetworkCacheMetaData &metaData) = 0;
    virtual QIODevice *data(const QUrl &url) = 0;
    virtual bool remove(const QUrl &url) = 0;
    virtual qint64 cacheSize() const = 0;

    virtual QIODevice *prepare(const QNetworkCacheMetaData &metaData) = 0;
    virtual void insert(QIODevice *device) = 0;

public Q_SLOTS:
    virtual void clear() = 0;

protected:
    explicit QAbstractNetworkCache(QObject *parent = 0);
    QAbstractNetworkCache(QAbstractNetworkCachePrivate &dd, QObject *parent);

private:
    Q_DECLARE_PRIVATE(QAbstractNetworkCache)
    Q_DISABLE_COPY(QAbstractNetworkCache)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif
