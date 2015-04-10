/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

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

#include "qwebpermissionrequest_p.h"

#include "WKStringQt.h"
#include <WebKit2/WKBase.h>
#include <WebKit2/WKRetainPtr.h>

class QWebPermissionRequestPrivate : public QSharedData {
public:
    QWebPermissionRequestPrivate(WKSecurityOriginRef securityOrigin, WKGeolocationPermissionRequestRef geo = 0, WKNotificationPermissionRequestRef notify = 0, QWebPermissionRequest::RequestType reqType = QWebPermissionRequest::Geolocation)
        : origin(securityOrigin)
        , geolocationRequest(geo)
        , notificationRequest(notify)
        , type(reqType)
        , allow(false)
    {
        WKRetainPtr<WKStringRef> url = adoptWK(WKSecurityOriginCopyProtocol(origin.get()));
        securityInfo.setScheme(WKStringCopyQString(url.get()));

        WKRetainPtr<WKStringRef> host = adoptWK(WKSecurityOriginCopyHost(origin.get()));
        securityInfo.setHost(WKStringCopyQString(host.get()));

        securityInfo.setPort(static_cast<int>(WKSecurityOriginGetPort(origin.get())));
    }

    ~QWebPermissionRequestPrivate()
    {
    }

    WKRetainPtr<WKSecurityOriginRef> origin;
    WKRetainPtr<WKGeolocationPermissionRequestRef> geolocationRequest;
    WKRetainPtr<WKNotificationPermissionRequestRef> notificationRequest;
    QWebPermissionRequest::RequestType type;
    QtWebSecurityOrigin securityInfo;
    bool allow;
};

QWebPermissionRequest* QWebPermissionRequest::create(WKSecurityOriginRef origin, WKGeolocationPermissionRequestRef request)
{
    return new QWebPermissionRequest(origin, request);
}

QWebPermissionRequest* QWebPermissionRequest::create(WKSecurityOriginRef origin, WKNotificationPermissionRequestRef request)
{
    return new QWebPermissionRequest(origin, 0, request, QWebPermissionRequest::Notification);
}

QWebPermissionRequest::QWebPermissionRequest(WKSecurityOriginRef securityOrigin
                                             , WKGeolocationPermissionRequestRef geo
                                             , WKNotificationPermissionRequestRef notify
                                             , QWebPermissionRequest::RequestType type
                                             , QObject* parent)
    : QObject(parent)
    , d(new QWebPermissionRequestPrivate(securityOrigin, geo, notify, type))
{
}

QWebPermissionRequest::~QWebPermissionRequest()
{
}

QWebPermissionRequest::RequestType QWebPermissionRequest::type() const
{
    return d->type;
}

void QWebPermissionRequest::setAllow(bool accepted)
{
    d->allow = accepted;
    switch (type()) {
    case Geolocation: {
        if (accepted)
            WKGeolocationPermissionRequestAllow(d->geolocationRequest.get());
        else
            WKGeolocationPermissionRequestDeny(d->geolocationRequest.get());
        break;
    }
    case Notification: {
        if (accepted)
            WKNotificationPermissionRequestAllow(d->notificationRequest.get());
        else
            WKNotificationPermissionRequestDeny(d->notificationRequest.get());
        break;
    }
    default:
        ASSERT_NOT_REACHED();
    }

    deleteLater();
}

bool QWebPermissionRequest::allow() const
{
    return d->allow;
}

QtWebSecurityOrigin* QWebPermissionRequest::securityOrigin()
{
    return &(d->securityInfo);
}

