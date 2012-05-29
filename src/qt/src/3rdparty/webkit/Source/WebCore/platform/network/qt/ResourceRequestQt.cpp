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
#include "ResourceRequest.h"

#include <qglobal.h>

#include <QNetworkRequest>
#include <QUrl>

namespace WebCore {

// Currently Qt allows three connections per host on symbian and six
// for everyone else. The limit can be found in qhttpnetworkconnection.cpp.
// To achieve the best result we want WebKit to schedule the jobs so we
// are using the limit as found in Qt. To allow Qt to fill its queue
// and prepare jobs we will schedule two more downloads.
// Per TCP connection there is 1 current processed, 3 possibly pipelined
// and 2 ready to re-fill the pipeline.
unsigned initializeMaximumHTTPConnectionCountPerHost()
{
#ifdef Q_OS_SYMBIAN
    return 3 * (1 + 3 + 2);
#else
    return 6 * (1 + 3 + 2);
#endif
}

QNetworkRequest ResourceRequest::toNetworkRequest(QObject* originatingFrame) const
{
    QNetworkRequest request;
    request.setUrl(url());
    request.setOriginatingObject(originatingFrame);

    const HTTPHeaderMap &headers = httpHeaderFields();
    for (HTTPHeaderMap::const_iterator it = headers.begin(), end = headers.end();
         it != end; ++it) {
        QByteArray name = QString(it->first).toAscii();
        QByteArray value = QString(it->second).toAscii();
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

    if (!allowCookies()) {
        request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
        request.setAttribute(QNetworkRequest::CookieSaveControlAttribute, QNetworkRequest::Manual);
        request.setAttribute(QNetworkRequest::AuthenticationReuseAttribute, QNetworkRequest::Manual);
    }

    return request;
}

}

