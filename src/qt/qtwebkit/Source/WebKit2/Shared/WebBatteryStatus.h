/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#ifndef WebBatteryStatus_h
#define WebBatteryStatus_h

#if ENABLE(BATTERY_STATUS)

#include "APIObject.h"
#include "ArgumentDecoder.h"
#include "ArgumentEncoder.h"
#include <wtf/PassRefPtr.h>

namespace WebKit {

class WebBatteryStatus : public TypedAPIObject<APIObject::TypeBatteryStatus> {
public:
    struct Data {
        void encode(CoreIPC::ArgumentEncoder&) const;
        static bool decode(CoreIPC::ArgumentDecoder&, Data&);

        bool isCharging;
        double chargingTime;
        double dischargingTime;
        double level;
    };

    static PassRefPtr<WebBatteryStatus> create(bool isCharging, double chargingTime, double dischargingTime, double level)
    {
        return adoptRef(new WebBatteryStatus(isCharging, chargingTime, dischargingTime, level));
    }

    virtual ~WebBatteryStatus();
    double isCharging() const { return m_data.isCharging; }
    double chargingTime() const { return m_data.chargingTime; }
    double dischargingTime() const { return m_data.dischargingTime; }
    double level() const { return m_data.level; }

    const Data& data() const { return m_data; }

private:
    WebBatteryStatus(bool isCharging, double chargingTime, double dischargingTime, double level);

    Data m_data;
};

} // namespace WebKit

#endif // ENABLE(BATTERY_STATUS)

#endif // WebBatteryStatus_h
