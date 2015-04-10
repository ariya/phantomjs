/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
#include "DeviceMotionClientMock.h"

#include "DeviceMotionController.h"

namespace WebCore {

DeviceMotionClientMock::DeviceMotionClientMock()
    : m_controller(0)
    , m_timer(this, &DeviceMotionClientMock::timerFired)
    , m_isUpdating(false)
{
}

void DeviceMotionClientMock::setController(DeviceMotionController* controller)
{
    ASSERT(!m_controller);
    m_controller = controller;
    ASSERT(m_controller);
}

void DeviceMotionClientMock::startUpdating()
{
    m_isUpdating = true;
}

void DeviceMotionClientMock::stopUpdating()
{
    m_isUpdating = false;
    m_timer.stop();
}

void DeviceMotionClientMock::setMotion(PassRefPtr<DeviceMotionData> motion)
{
    m_motion = motion;
    if (m_isUpdating && !m_timer.isActive())
        m_timer.startOneShot(0);
}

void DeviceMotionClientMock::timerFired(Timer<DeviceMotionClientMock>* timer)
{
    ASSERT_UNUSED(timer, timer == &m_timer);
    m_timer.stop();
    m_controller->didChangeDeviceMotion(m_motion.get());
}

} // namespace WebCore
