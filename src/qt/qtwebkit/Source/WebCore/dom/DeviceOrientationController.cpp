/*
 * Copyright 2010, The Android Open Source Project
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DeviceOrientationController.h"

#include "DeviceOrientationClient.h"
#include "DeviceOrientationData.h"
#include "DeviceOrientationEvent.h"
#include "InspectorInstrumentation.h"

namespace WebCore {

DeviceOrientationController::DeviceOrientationController(Page* page, DeviceOrientationClient* client)
    : DeviceController(client)
    , m_page(page)
{
    ASSERT(m_client);
    deviceOrientationClient()->setController(this);
}

PassOwnPtr<DeviceOrientationController> DeviceOrientationController::create(Page* page, DeviceOrientationClient* client)
{
    return adoptPtr(new DeviceOrientationController(page, client));
}

void DeviceOrientationController::didChangeDeviceOrientation(DeviceOrientationData* orientation)
{
    orientation = InspectorInstrumentation::overrideDeviceOrientation(m_page, orientation);
    dispatchDeviceEvent(DeviceOrientationEvent::create(eventNames().deviceorientationEvent, orientation));
}

DeviceOrientationClient* DeviceOrientationController::deviceOrientationClient()
{
    return static_cast<DeviceOrientationClient*>(m_client);
}

bool DeviceOrientationController::hasLastData()
{
    return deviceOrientationClient()->lastOrientation();
}

PassRefPtr<Event> DeviceOrientationController::getLastEvent()
{
    return DeviceOrientationEvent::create(eventNames().deviceorientationEvent, deviceOrientationClient()->lastOrientation());
}

const char* DeviceOrientationController::supplementName()
{
    return "DeviceOrientationController";
}

DeviceOrientationController* DeviceOrientationController::from(Page* page)
{
    return static_cast<DeviceOrientationController*>(Supplement<Page>::from(page, supplementName()));
}

bool DeviceOrientationController::isActiveAt(Page* page)
{
    if (DeviceOrientationController* self = DeviceOrientationController::from(page))
        return self->isActive();
    return false;
}

void provideDeviceOrientationTo(Page* page, DeviceOrientationClient* client)
{
    DeviceOrientationController::provideTo(page, DeviceOrientationController::supplementName(), DeviceOrientationController::create(page, client));
}

} // namespace WebCore
