/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "NetworkingContext.h"
#include "ResourceRequest.h"
#include "ThirdPartyCookiesQt.h"

#if ENABLE(BLOB)
#include "BlobData.h"
#include "BlobRegistryImpl.h"
#include "BlobStorageData.h"
#endif

#include <qglobal.h>

#include <QNetworkRequest>
#include <QUrl>

namespace WebCore {

// The limit can be found in qhttpnetworkconnection.cpp.
// To achieve the best result we want WebKit to schedule the jobs so we
// are using the limit as found in Qt. To allow Qt to fill its queue
// and prepare jobs we will schedule two more downloads.
// Per TCP connection there is 1 current processed, 3 possibly pipelined
// and 2 ready to re-fill the pipeline.
unsigned initializeMaximumHTTPConnectionCountPerHost()
{
    return 6 * (1 + 3 + 2);
}

#if ENABLE(BLOB)
static void appendBlobResolved(QByteArray& data, const QUrl& url, QString* contentType = 0)
{
    RefPtr<BlobStorageData> blobData = static_cast<BlobRegistryImpl&>(blobRegistry()).getBlobDataFromURL(url);
    if (!blobData)
        return;

    if (contentType)
        *contentType = blobData->contentType();

    BlobDataItemList::const_iterator it = blobData->items().begin();
    const BlobDataItemList::const_iterator itend = blobData->items().end();
    for (; it != itend; ++it) {
        const BlobDataItem& blobItem = *it;
        if (blobItem.type == BlobDataItem::Data)
            data.append(blobItem.data->data() + static_cast<int>(blobItem.offset), static_cast<int>(blobItem.length));
        else if (blobItem.type == BlobDataItem::Blob)
            appendBlobResolved(data, blobItem.url);
        else if (blobItem.type == BlobDataItem::File) {
            // File types are not allowed here, so just ignore it.
        } else
            ASSERT_NOT_REACHED();
    }
}

static void resolveBlobUrl(const QUrl& url, QUrl& resolvedUrl)
{
    RefPtr<BlobStorageData> blobData = static_cast<BlobRegistryImpl&>(blobRegistry()).getBlobDataFromURL(url);
    if (!blobData)
        return;

    QByteArray data;
    QString contentType;
    appendBlobResolved(data, url, &contentType);

    QString dataUri(QStringLiteral("data:"));
    dataUri.append(contentType);
    dataUri.append(QStringLiteral(";base64,"));
    dataUri.append(QString::fromLatin1(data.toBase64()));
    resolvedUrl = QUrl(dataUri);
}
#endif

QNetworkRequest ResourceRequest::toNetworkRequest(NetworkingContext *context) const
{
    QNetworkRequest request;
    QUrl newurl = url();

#if ENABLE(BLOB)
    if (newurl.scheme() == QLatin1String("blob"))
        resolveBlobUrl(url(), newurl);
#endif

    request.setUrl(newurl);
    request.setOriginatingObject(context ? context->originatingObject() : 0);

    const HTTPHeaderMap &headers = httpHeaderFields();
    for (HTTPHeaderMap::const_iterator it = headers.begin(), end = headers.end();
         it != end; ++it) {
        QByteArray name = QString(it->key).toLatin1();
        QByteArray value = QString(it->value).toLatin1();
        // QNetworkRequest::setRawHeader() would remove the header if the value is null
        // Make sure to set an empty header instead of null header.
        if (!value.isNull())
            request.setRawHeader(name, value);
        else
            request.setRawHeader(name, "");
    }

    // Make sure we always have an Accept header; some sites require this to
    // serve subresources
    if (!request.hasRawHeader("Accept"))
        request.setRawHeader("Accept", "*/*");

    switch (cachePolicy()) {
    case ReloadIgnoringCacheData:
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
        break;
    case ReturnCacheDataElseLoad:
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
        break;
    case ReturnCacheDataDontLoad:
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysCache);
        break;
    case UseProtocolCachePolicy:
        // QNetworkRequest::PreferNetwork
    default:
        break;
    }

    if (!allowCookies() || !thirdPartyCookiePolicyPermits(context, url(), firstPartyForCookies())) {
        request.setAttribute(QNetworkRequest::CookieSaveControlAttribute, QNetworkRequest::Manual);
        request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
    }

    if (!allowCookies())
        request.setAttribute(QNetworkRequest::AuthenticationReuseAttribute, QNetworkRequest::Manual);

    return request;
}

}

