/*
 * Copyright 2010 Apple Inc. All rights reserved.
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
#include "DeviceMotionController.h"

#include "DeviceMotionClient.h"
#include "DeviceMotionData.h"
#include "DeviceMotionEvent.h"
#include "Page.h"

namespace WebCore {

DeviceMotionController::DeviceMotionController(DeviceMotionClient* client)
    : DeviceController(client)
{
    ASSERT(m_client);
    deviceMotionClient()->setController(this);
}

PassOwnPtr<DeviceMotionController> DeviceMotionController::create(DeviceMotionClient* client)
{
    return adoptPtr(new DeviceMotionController(client));
}

void DeviceMotionController::didChangeDeviceMotion(DeviceMotionData* deviceMotionData)
{
    dispatchDeviceEvent(DeviceMotionEvent::create(eventNames().devicemotionEvent, deviceMotionData));
}

DeviceMotionClient* DeviceMotionController::deviceMotionClient()
{
    return static_cast<DeviceMotionClient*>(m_client);
}

bool DeviceMotionController::hasLastData()
{
    return deviceMotionClient()->lastMotion();
}

PassRefPtr<Event> DeviceMotionController::getLastEvent()
{
    return DeviceMotionEvent::create(eventNames().devicemotionEvent, deviceMotionClient()->lastMotion());
}

const char* DeviceMotionController::supplementName()
{
    return "DeviceMotionController";
}

DeviceMotionController* DeviceMotionController::from(Page* page)
{
    return static_cast<DeviceMotionController*>(Supplement<Page>::from(page, supplementName()));
}

bool DeviceMotionController::isActiveAt(Page* page)
{
    if (DeviceMotionController* self = DeviceMotionController::from(page))
        return self->isActive();
    return false;
}

void provideDeviceMotionTo(Page* page, DeviceMotionClient* client)
{
    DeviceMotionController::provideTo(page, DeviceMotionController::supplementName(), DeviceMotionController::create(client));
}

} // namespace WebCore
