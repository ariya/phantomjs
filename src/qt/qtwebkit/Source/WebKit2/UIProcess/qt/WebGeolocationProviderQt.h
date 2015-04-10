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


#ifndef WebGeolocationProviderQt_h
#define WebGeolocationProviderQt_h

#include <QObject>
#include <WebKit2/WKGeolocationManager.h>
#include <WebKit2/WKGeolocationPosition.h>
#include <WebKit2/WKRetainPtr.h>

QT_BEGIN_NAMESPACE
class QGeoPositionInfoSource;
class QGeoPositionInfo;
QT_END_NAMESPACE

namespace WebKit {

class WebGeolocationProviderQt : public QObject {
    Q_OBJECT
public:
    static WebGeolocationProviderQt* create(WKGeolocationManagerRef);
    static WKGeolocationProvider* provider(const WebGeolocationProviderQt*);

    virtual ~WebGeolocationProviderQt();

    void startUpdating() const;
    void stopUpdating() const;

public Q_SLOTS:
    void updateTimeout();
    void positionUpdated(const QGeoPositionInfo&);

private:
    Q_DISABLE_COPY(WebGeolocationProviderQt);
    WebGeolocationProviderQt(WKGeolocationManagerRef);

    WKGeolocationManagerRef m_manager;
    WKRetainPtr<WKGeolocationPositionRef> m_lastPosition;
    mutable QGeoPositionInfoSource* m_source;
};

} // namespace WebKit

#endif /* WebGeolocationProviderQt_h */
