/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#ifndef QABSTRACTNETWORKCACHE_H
#define QABSTRACTNETWORKCACHE_H

#include <QtCore/qobject.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qpair.h>
#include <QtNetwork/qnetworkrequest.h>

QT_BEGIN_NAMESPACE


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

    void swap(QNetworkCacheMetaData &other)
    { qSwap(d, other.d); }

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

Q_DECLARE_SHARED(QNetworkCacheMetaData)

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

#endif
