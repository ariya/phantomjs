/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GeolocationClientQt_h
#define GeolocationClientQt_h

#include "GeolocationClient.h"
#include <QGeoPositionInfoSource>
#include <wtf/RefPtr.h>

// FIXME: Remove usage of "using namespace" in a header file.
// There is bug in qtMobility signal names are not full qualified when used with namespace
// QtMobility namespace in slots throws up error and its required to be fixed in qtmobility.
using namespace QtMobility;

class QWebPage;

namespace WebCore {

// This class provides a implementation of a GeolocationService for qtWebkit.
// It uses QtMobility (v1.0.0) location service to get positions
class GeolocationClientQt : public QObject, public GeolocationClient {
    Q_OBJECT

public:
    GeolocationClientQt(const QWebPage*);
    virtual ~GeolocationClientQt();

    virtual void geolocationDestroyed();
    virtual void startUpdating();
    virtual void stopUpdating();

    void setEnableHighAccuracy(bool);
    virtual GeolocationPosition* lastPosition() { return m_lastPosition.get(); }

    virtual void requestPermission(Geolocation*);
    virtual void cancelPermissionRequest(Geolocation*);

private Q_SLOTS:
    // QGeoPositionInfoSource
    void positionUpdated(const QGeoPositionInfo&);

private:
    const QWebPage* m_page;
    RefPtr<GeolocationPosition> m_lastPosition;
    QtMobility::QGeoPositionInfoSource* m_location;
};

} // namespace WebCore

#endif // GeolocationClientQt_h
