/*
 * Copyright (C) 2012 Zan Dobersek <zandobersek@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "config.h"
#include "GamepadDeviceLinux.h"

#if ENABLE(GAMEPAD)

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <wtf/text/CString.h>

namespace WebCore {

GamepadDeviceLinux::GamepadDeviceLinux(String deviceFile)
    : m_fileDescriptor(-1)
    , m_connected(false)
    , m_lastTimestamp(0)
{
    // FIXME: Log errors when returning early.
    m_fileDescriptor = open(deviceFile.utf8().data(), O_RDONLY | O_NONBLOCK);
    if (m_fileDescriptor == -1)
        return;

    char deviceName[1024];
    if (ioctl(m_fileDescriptor, JSIOCGNAME(sizeof(deviceName)), deviceName) < 0)
        return;
    m_deviceName = String(deviceName).simplifyWhiteSpace();

    uint8_t numberOfAxes;
    uint8_t numberOfButtons;
    if (ioctl(m_fileDescriptor, JSIOCGAXES, &numberOfAxes) < 0 || ioctl(m_fileDescriptor, JSIOCGBUTTONS, &numberOfButtons) < 0)
        return;
    m_axes.fill(0.0, numberOfAxes);
    m_buttons.fill(0.0, numberOfButtons);
}

GamepadDeviceLinux::~GamepadDeviceLinux()
{
    if (m_fileDescriptor != -1)
        close(m_fileDescriptor);
}

void GamepadDeviceLinux::updateForEvent(struct js_event event)
{
    if (!(event.type & JS_EVENT_AXIS || event.type & JS_EVENT_BUTTON))
        return;

    // Mark the device as connected only if it is not yet connected, the event is not an initialization
    // and the value is not 0 (indicating a genuine interaction with the device).
    if (!m_connected && !(event.type & JS_EVENT_INIT) && event.value)
        m_connected = true;

    if (event.type & JS_EVENT_AXIS)
        m_axes[event.number] = normalizeAxisValue(event.value);
    else if (event.type & JS_EVENT_BUTTON)
        m_buttons[event.number] = normalizeButtonValue(event.value);

    m_lastTimestamp = event.time;
}

float GamepadDeviceLinux::normalizeAxisValue(short value)
{
    // Normalize from range [-32767, 32767] into range [-1.0, 1.0]
    return value / 32767.0f;
}

float GamepadDeviceLinux::normalizeButtonValue(short value)
{
    // Normalize from range [0, 1] into range [0.0, 1.0]
    return value / 1.0f;
}

} // namespace WebCore

#endif // ENABLE(GAMEPAD)
