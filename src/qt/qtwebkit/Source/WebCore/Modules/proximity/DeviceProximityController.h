/*
 * Copyright 2012 Samsung Electronics. All rights reserved.
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

#ifndef DeviceProximityController_h
#define DeviceProximityController_h

#if ENABLE(PROXIMITY_EVENTS)

#include "DeviceController.h"

namespace WebCore {

class DeviceProximityClient;

class DeviceProximityController : public DeviceController {
public:
    ~DeviceProximityController() { }

    static PassOwnPtr<DeviceProximityController> create(DeviceProximityClient*);

    void didChangeDeviceProximity(const double value, const double min, const double max);
    DeviceProximityClient* deviceProximityClient();

    virtual bool hasLastData();
    virtual PassRefPtr<Event> getLastEvent();

    static const char* supplementName();
    static DeviceProximityController* from(Page*);
    static bool isActiveAt(Page*);

private:
    explicit DeviceProximityController(DeviceProximityClient*);
};

} // namespace WebCore

#endif // PROXIMITY_EVENTS
#endif // DeviceProximityController_h
