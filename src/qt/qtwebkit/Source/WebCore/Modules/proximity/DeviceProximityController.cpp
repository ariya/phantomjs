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

#include "config.h"
#include "DeviceProximityController.h"

#if ENABLE(PROXIMITY_EVENTS)

#include "DeviceProximityClient.h"
#include "DeviceProximityEvent.h"
#include "Page.h"

namespace WebCore {

DeviceProximityController::DeviceProximityController(DeviceProximityClient* client)
    : DeviceController(client)
{
    ASSERT(m_client);
}

PassOwnPtr<DeviceProximityController> DeviceProximityController::create(DeviceProximityClient* client)
{
    return adoptPtr(new DeviceProximityController(client));
}

void DeviceProximityController::didChangeDeviceProximity(const double value, const double min, const double max)
{
    ASSERT(value >= min && value <= max);

    dispatchDeviceEvent(DeviceProximityEvent::create(eventNames().webkitdeviceproximityEvent, value, min, max));
}

DeviceProximityClient* DeviceProximityController::deviceProximityClient()
{
    return static_cast<DeviceProximityClient*>(m_client);
}

bool DeviceProximityController::hasLastData()
{
    return deviceProximityClient()->hasLastData();
}

PassRefPtr<Event> DeviceProximityController::getLastEvent()
{
    return DeviceProximityEvent::create(eventNames().webkitdeviceproximityEvent, deviceProximityClient()->value(), deviceProximityClient()->min(), deviceProximityClient()->max());
}

const char* DeviceProximityController::supplementName()
{
    return "DeviceProximityController";
}

DeviceProximityController* DeviceProximityController::from(Page* page)
{
    return static_cast<DeviceProximityController*>(Supplement<Page>::from(page, supplementName()));
}

bool DeviceProximityController::isActiveAt(Page* page)
{
    if (DeviceProximityController* controller = DeviceProximityController::from(page))
        return controller->isActive();
    return false;
}

void provideDeviceProximityTo(Page* page, DeviceProximityClient* client)
{
    DeviceProximityController::provideTo(page, DeviceProximityController::supplementName(), DeviceProximityController::create(client));
}

} // namespace WebCore

#endif // PROXIMITY_EVENTS
