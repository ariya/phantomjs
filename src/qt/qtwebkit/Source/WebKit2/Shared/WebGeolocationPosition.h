/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebGeolocationPosition_h
#define WebGeolocationPosition_h

#include "APIObject.h"
#include "ArgumentEncoder.h"
#include "ArgumentDecoder.h"
#include <wtf/PassRefPtr.h>

namespace WebKit {

class WebGeolocationPosition : public TypedAPIObject<APIObject::TypeGeolocationPosition> {
public:
    struct Data {
        void encode(CoreIPC::ArgumentEncoder&) const;
        static bool decode(CoreIPC::ArgumentDecoder&, Data&);

        double timestamp;
        double latitude;
        double longitude;
        double accuracy;

        double altitude;
        double altitudeAccuracy;
        double heading;
        double speed;

        bool canProvideAltitude;
        bool canProvideAltitudeAccuracy;
        bool canProvideHeading;
        bool canProvideSpeed;
    };

    static PassRefPtr<WebGeolocationPosition> create(double timestamp, double latitude, double longitude, double accuracy, bool providesAltitude, double altitude, bool providesAltitudeAccuracy, double altitudeAccuracy, bool providesHeading, double heading, bool providesSpeed, double speed)
    {
        return adoptRef(new WebGeolocationPosition(timestamp, latitude, longitude, accuracy, providesAltitude, altitude, providesAltitudeAccuracy, altitudeAccuracy, providesHeading, heading, providesSpeed, speed));
    }

    virtual ~WebGeolocationPosition();

    double timestamp() const { return m_data.timestamp; }
    double latitude() const { return m_data.latitude; }
    double longitude() const { return m_data.longitude; }
    double accuracy() const { return m_data.accuracy; }

    bool canProvideAltitude() const { return m_data.canProvideAltitude; }
    double altitude() const { return m_data.altitude; }

    bool canProvideAltitudeAccuracy() const { return m_data.canProvideAltitudeAccuracy; }
    double altitudeAccuracy() const { return m_data.altitudeAccuracy; }

    bool canProvideHeading() const { return m_data.canProvideHeading; }
    double heading() const { return m_data.heading; }

    bool canProvideSpeed() const { return m_data.canProvideSpeed; }
    double speed() const { return m_data.speed; }

    const Data& data() const { return m_data; }

private:
    WebGeolocationPosition(double timestamp, double latitude, double longitude, double accuracy, bool providesAltitude, double altitude, bool providesAltitudeAccuracy, double altitudeAccuracy, bool providesHeading, double heading, bool providesSpeed, double speed);

    Data m_data;
};

} // namespace WebKit

#endif // WebGeolocationPosition_h
