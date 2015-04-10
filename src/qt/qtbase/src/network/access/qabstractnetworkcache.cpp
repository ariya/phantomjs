/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qabstractnetworkcache.h"
#include "qabstractnetworkcache_p.h"

#include <qdatastream.h>
#include <qdatetime.h>
#include <qurl.h>

#include <qdebug.h>

QT_BEGIN_NAMESPACE

class QNetworkCacheMetaDataPrivate : public QSharedData
{

public:
    QNetworkCacheMetaDataPrivate()
        : QSharedData()
        , saveToDisk(true)
    {}

    bool operator==(const QNetworkCacheMetaDataPrivate &other) const
    {
        return
            url == other.url
            && lastModified == other.lastModified
            && expirationDate == other.expirationDate
            && headers == other.headers
            && saveToDisk == other.saveToDisk;
    }

    QUrl url;
    QDateTime lastModified;
    QDateTime expirationDate;
    QNetworkCacheMetaData::RawHeaderList headers;
    QNetworkCacheMetaData::AttributesMap attributes;
    bool saveToDisk;

    static void save(QDataStream &out, const QNetworkCacheMetaData &metaData);
    static void load(QDataStream &in, QNetworkCacheMetaData &metaData);
};
Q_GLOBAL_STATIC(QNetworkCacheMetaDataPrivate, metadata_shared_invalid)

/*!
    \class QNetworkCacheMetaData
    \since 4.5
    \ingroup shared
    \inmodule QtNetwork

    \brief The QNetworkCacheMetaData class provides cache information.

    QNetworkCacheMetaData provides information about a cache file including
    the url, when it was last modified, when the cache file was created, headers
    for file and if the file should be saved onto a disk.

    \sa QAbstractNetworkCache
*/

/*!
    \typedef QNetworkCacheMetaData::RawHeader

    Synonym for QPair<QByteArray, QByteArray>
*/

/*!
    \typedef QNetworkCacheMetaData::RawHeaderList

    Synonym for QList<RawHeader>
*/

/*!
  \typedef  QNetworkCacheMetaData::AttributesMap

  Synonym for QHash<QNetworkRequest::Attribute, QVariant>
*/

/*!
    Constructs an invalid network cache meta data.

    \sa isValid()
 */
QNetworkCacheMetaData::QNetworkCacheMetaData()
    : d(new QNetworkCacheMetaDataPrivate)
{
}

/*!
    Destroys the network cache meta data.
 */
QNetworkCacheMetaData::~QNetworkCacheMetaData()
{
    // QSharedDataPointer takes care of freeing d
}

/*!
    Constructs a copy of the \a other QNetworkCacheMetaData.
 */
QNetworkCacheMetaData::QNetworkCacheMetaData(const QNetworkCacheMetaData &other)
    : d(other.d)
{
}

/*!
    Makes a copy of the \a other QNetworkCacheMetaData and returns a reference to the copy.
 */
QNetworkCacheMetaData &QNetworkCacheMetaData::operator=(const QNetworkCacheMetaData &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn void QNetworkCacheMetaData::swap(QNetworkCacheMetaData &other)
    \since 5.0

    Swaps this metadata instance with \a other. This function is very
    fast and never fails.
 */

/*!
    Returns \c true if this meta data is equal to the \a other meta data; otherwise returns \c false.

    \sa operator!=()
 */
bool QNetworkCacheMetaData::operator==(const QNetworkCacheMetaData &other) const
{
    if (d == other.d)
        return true;
    if (d && other.d)
        return *d == *other.d;
    return false;
}

/*!
    \fn bool QNetworkCacheMetaData::operator!=(const QNetworkCacheMetaData &other) const

    Returns \c true if this meta data is not equal to the \a other meta data; otherwise returns \c false.

    \sa operator==()
 */

/*!
    Returns \c true if this network cache meta data has attributes that have been set otherwise false.
 */
bool QNetworkCacheMetaData::isValid() const
{
    return !(*d == *metadata_shared_invalid());
}

/*!
    Returns is this cache should be allowed to be stored on disk.

    Some cache implementations can keep these cache items in memory for performance reasons,
    but for security reasons they should not be written to disk.

    Specifically with http, documents marked with Pragma: no-cache, or have a Cache-control set to
    no-store or no-cache or any https document that doesn't have "Cache-control: public" set will
    set the saveToDisk to false.

    \sa setSaveToDisk()
 */
bool QNetworkCacheMetaData::saveToDisk() const
{
    return d->saveToDisk;
}

/*!
    Sets whether this network cache meta data and associated content should be
    allowed to be stored on disk to \a allow.

    \sa saveToDisk()
 */
void QNetworkCacheMetaData::setSaveToDisk(bool allow)
{
    d->saveToDisk = allow;
}

/*!
    Returns the URL this network cache meta data is referring to.

    \sa setUrl()
 */
QUrl QNetworkCacheMetaData::url() const
{
    return d->url;
}

/*!
    Sets the URL this network cache meta data to be \a url.

    The password and fragment are removed from the url.

    \sa url()
 */
void QNetworkCacheMetaData::setUrl(const QUrl &url)
{
    d->url = url;
    d->url.setPassword(QString());
    d->url.setFragment(QString());
}

/*!
    Returns a list of all raw headers that are set in this meta data.
    The list is in the same order that the headers were set.

    \sa setRawHeaders()
 */
QNetworkCacheMetaData::RawHeaderList QNetworkCacheMetaData::rawHeaders() const
{
    return d->headers;
}

/*!
    Sets the raw headers to \a list.

    \sa rawHeaders()
 */
void QNetworkCacheMetaData::setRawHeaders(const RawHeaderList &list)
{
    d->headers = list;
}

/*!
    Returns the date and time when the meta data was last modified.
 */
QDateTime QNetworkCacheMetaData::lastModified() const
{
    return d->lastModified;
}

/*!
    Sets the date and time when the meta data was last modified to \a dateTime.
 */
void QNetworkCacheMetaData::setLastModified(const QDateTime &dateTime)
{
    d->lastModified = dateTime;
}

/*!
    Returns the date and time when the meta data expires.
 */
QDateTime QNetworkCacheMetaData::expirationDate() const
{
    return d->expirationDate;
}

/*!
    Sets the date and time when the meta data expires to \a dateTime.
 */
void QNetworkCacheMetaData::setExpirationDate(const QDateTime &dateTime)
{
    d->expirationDate = dateTime;
}

/*!
    \since 4.6

    Returns all the attributes stored with this cache item.

    \sa setAttributes(), QNetworkRequest::Attribute
*/
QNetworkCacheMetaData::AttributesMap QNetworkCacheMetaData::attributes() const
{
    return d->attributes;
}

/*!
    \since 4.6

    Sets all attributes of this cache item to be the map \a attributes.

    \sa attributes(), QNetworkRequest::setAttribute()
*/
void QNetworkCacheMetaData::setAttributes(const AttributesMap &attributes)
{
    d->attributes = attributes;
}

/*!
    \relates QNetworkCacheMetaData
    \since 4.5

    Writes \a metaData to the \a out stream.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator<<(QDataStream &out, const QNetworkCacheMetaData &metaData)
{
    QNetworkCacheMetaDataPrivate::save(out, metaData);
    return out;
}

static inline QDataStream &operator<<(QDataStream &out, const QNetworkCacheMetaData::AttributesMap &hash)
{
    out << quint32(hash.size());
    QNetworkCacheMetaData::AttributesMap::ConstIterator it = hash.end();
    QNetworkCacheMetaData::AttributesMap::ConstIterator begin = hash.begin();
    while (it != begin) {
        --it;
        out << int(it.key()) << it.value();
    }
    return out;
}

void QNetworkCacheMetaDataPrivate::save(QDataStream &out, const QNetworkCacheMetaData &metaData)
{
    // note: if you change the contents of the meta data here
    // remember to bump the cache version in qnetworkdiskcache.cpp CurrentCacheVersion
    out << metaData.url();
    out << metaData.expirationDate();
    out << metaData.lastModified();
    out << metaData.saveToDisk();
    out << metaData.attributes();
    out << metaData.rawHeaders();
}

/*!
    \relates QNetworkCacheMetaData
    \since 4.5

    Reads a QNetworkCacheMetaData from the stream \a in into \a metaData.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator>>(QDataStream &in, QNetworkCacheMetaData &metaData)
{
    QNetworkCacheMetaDataPrivate::load(in, metaData);
    return in;
}

static inline QDataStream &operator>>(QDataStream &in, QNetworkCacheMetaData::AttributesMap &hash)
{
    hash.clear();
    QDataStream::Status oldStatus = in.status();
    in.resetStatus();
    hash.clear();

    quint32 n;
    in >> n;

    for (quint32 i = 0; i < n; ++i) {
        if (in.status() != QDataStream::Ok)
            break;

        int k;
        QVariant t;
        in >> k >> t;
        hash.insertMulti(QNetworkRequest::Attribute(k), t);
    }

    if (in.status() != QDataStream::Ok)
        hash.clear();
    if (oldStatus != QDataStream::Ok)
        in.setStatus(oldStatus);
    return in;
}

void QNetworkCacheMetaDataPrivate::load(QDataStream &in, QNetworkCacheMetaData &metaData)
{
    in >> metaData.d->url;
    in >> metaData.d->expirationDate;
    in >> metaData.d->lastModified;
    in >> metaData.d->saveToDisk;
    in >> metaData.d->attributes;
    in >> metaData.d->headers;
}

/*!
    \class QAbstractNetworkCache
    \since 4.5
    \inmodule QtNetwork

    \brief The QAbstractNetworkCache class provides the interface for cache implementations.

    QAbstractNetworkCache is the base class for every standard cache that is used by
    QNetworkAccessManager.  QAbstractNetworkCache is an abstract class and cannot be
    instantiated.

    \sa QNetworkDiskCache
*/

/*!
    Constructs an abstract network cache with the given \a parent.
*/
QAbstractNetworkCache::QAbstractNetworkCache(QObject *parent)
    : QObject(*new QAbstractNetworkCachePrivate, parent)
{
}

/*!
    \internal
*/
QAbstractNetworkCache::QAbstractNetworkCache(QAbstractNetworkCachePrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
}

/*!
    Destroys the cache.

    Any operations that have not been inserted are discarded.

    \sa insert()
 */
QAbstractNetworkCache::~QAbstractNetworkCache()
{
}

/*!
    \fn QNetworkCacheMetaData QAbstractNetworkCache::metaData(const QUrl &url) = 0
    Returns the meta data for the url \a url.

    If the url is valid and the cache contains the data for url,
    a valid QNetworkCacheMetaData is returned.

    In the base class this is a pure virtual function.

    \sa updateMetaData(), data()
*/

/*!
    \fn void QAbstractNetworkCache::updateMetaData(const QNetworkCacheMetaData &metaData) = 0
    Updates the cache meta date for the metaData's url to \a metaData

    If the cache does not contains a cache item for the url then no action is taken.

    In the base class this is a pure virtual function.

    \sa metaData(), prepare()
*/

/*!
    \fn QIODevice *QAbstractNetworkCache::data(const QUrl &url) = 0
    Returns the data associated with \a url.

    It is up to the application that requests the data to delete
    the QIODevice when done with it.

    If there is no cache for \a url, the url is invalid, or if there
    is an internal cache error 0 is returned.

    In the base class this is a pure virtual function.

    \sa metaData(), prepare()
*/

/*!
    \fn bool QAbstractNetworkCache::remove(const QUrl &url) = 0
    Removes the cache entry for \a url, returning true if success otherwise false.

    In the base class this is a pure virtual function.

    \sa clear(), prepare()
*/

/*!
    \fn QIODevice *QAbstractNetworkCache::prepare(const QNetworkCacheMetaData &metaData) = 0
    Returns the device that should be populated with the data for
    the cache item \a metaData.  When all of the data has been written
    insert() should be called.  If metaData is invalid or the url in
    the metadata is invalid 0 is returned.

    The cache owns the device and will take care of deleting it when
    it is inserted or removed.

    To cancel a prepared inserted call remove() on the metadata's url.

    In the base class this is a pure virtual function.

    \sa remove(), updateMetaData(), insert()
*/

/*!
    \fn void QAbstractNetworkCache::insert(QIODevice *device) = 0
    Inserts the data in \a device and the prepared meta data into the cache.
    After this function is called the data and meta data should be retrievable
    using data() and metaData().

    To cancel a prepared inserted call remove() on the metadata's url.

    In the base class this is a pure virtual function.

    \sa prepare(), remove()
*/

/*!
    \fn qint64 QAbstractNetworkCache::cacheSize() const = 0
    Returns the current size taken up by the cache.  Depending upon
    the cache implementation this might be disk or memory size.

    In the base class this is a pure virtual function.

    \sa clear()
*/

/*!
    \fn void QAbstractNetworkCache::clear() = 0
    Removes all items from the cache.  Unless there was failures
    clearing the cache cacheSize() should return 0 after a call to clear.

    In the base class this is a pure virtual function.

    \sa cacheSize(), remove()
*/

QT_END_NAMESPACE
