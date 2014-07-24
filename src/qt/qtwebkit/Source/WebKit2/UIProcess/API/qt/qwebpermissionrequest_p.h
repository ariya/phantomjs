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

#ifndef qwebpermissionrequest_p_h
#define qwebpermissionrequest_p_h

#include "qtwebsecurityorigin_p.h"
#include "qwebkitglobal.h"

#include <QtCore/QObject>
#include <QtCore/qshareddata.h>
#include <WebKit2/WKGeolocationPermissionRequest.h>
#include <WebKit2/WKNotificationPermissionRequest.h>
#include <WebKit2/WKSecurityOrigin.h>

class QWebPermissionRequestPrivate;

class QWEBKIT_EXPORT QWebPermissionRequest : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool allow READ allow WRITE setAllow)
    Q_PROPERTY(RequestType type READ type CONSTANT)
    Q_PROPERTY(QtWebSecurityOrigin* origin READ securityOrigin)
    Q_ENUMS(RequestType)

public:
    enum RequestType {
        Geolocation,
        Notification
    };

    static QWebPermissionRequest* create(WKSecurityOriginRef, WKGeolocationPermissionRequestRef);
    static QWebPermissionRequest* create(WKSecurityOriginRef, WKNotificationPermissionRequestRef);
    virtual ~QWebPermissionRequest();

    RequestType type() const;
    bool allow() const;

public Q_SLOTS:
    void setAllow(bool);
    QtWebSecurityOrigin* securityOrigin();

private:
    friend class QWebPermissionRequestPrivate;
    QWebPermissionRequest(WKSecurityOriginRef securityOrigin
                          , WKGeolocationPermissionRequestRef geo = 0
                          , WKNotificationPermissionRequestRef notify = 0
                          , QWebPermissionRequest::RequestType type = Geolocation
                          , QObject* parent = 0);

private:
    QExplicitlySharedDataPointer<QWebPermissionRequestPrivate> d;
};


#endif // qwebpermissionrequest_h
