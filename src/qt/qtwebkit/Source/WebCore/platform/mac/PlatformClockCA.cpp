/*
 * Copyright (C) 2011 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if USE(COREAUDIO)

#include "PlatformClockCA.h"

#include <AudioToolbox/CoreAudioClock.h>
#include <CoreAudio/AudioHardware.h>

using namespace WebCore;

PlatformClockCA::PlatformClockCA()
    : m_clock(0)
    , m_running(false)
{
    CAClockNew(0, &m_clock);
    UInt32 timebase = kCAClockTimebase_AudioDevice;
    UInt32 timebaseSize = sizeof(timebase);
    CAClockSetProperty(m_clock, kCAClockProperty_InternalTimebase, timebaseSize, &timebase);

    AudioObjectID defaultAudioOutput = 0;
    UInt32 defaultAudioOutputSize = sizeof(defaultAudioOutput);

    AudioObjectPropertyAddress address;
    address.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
    address.mScope    = kAudioObjectPropertyScopeGlobal;
    address.mElement  = kAudioObjectPropertyElementMaster;

    AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, 0, &defaultAudioOutputSize, &defaultAudioOutput);
    CAClockSetProperty(m_clock, kCAClockProperty_TimebaseSource, defaultAudioOutputSize, &defaultAudioOutput);
}

PlatformClockCA::~PlatformClockCA()
{
    CAClockDispose(m_clock);
}

void PlatformClockCA::setCurrentTime(double time)
{
    if (m_running)
        CAClockStop(m_clock);
    CAClockTime caTime;
    caTime.format = kCAClockTimeFormat_Seconds;
    caTime.time.seconds = time;
    CAClockSetCurrentTime(m_clock, &caTime);
    if (m_running)
        CAClockStart(m_clock);
}

double PlatformClockCA::currentTime() const
{
    CAClockTime caTime;

    // CAClock does not return the correct current time when stopped. Instead, query for
    // what is the start time, i.e. what the current time will be when started.
    if (m_running) {
        if (CAClockGetCurrentTime(m_clock, kCAClockTimeFormat_Seconds, &caTime) == noErr)
            return caTime.time.seconds;
    } else {
        if (CAClockGetStartTime(m_clock, kCAClockTimeFormat_Seconds, &caTime) == noErr)
            return caTime.time.seconds;
    }
    return 0;
}

void PlatformClockCA::setPlayRate(double rate)
{
    CAClockSetPlayRate(m_clock, rate);
}

double PlatformClockCA::PlatformClockCA::playRate() const
{
    double rate = 0;
    if (CAClockGetPlayRate(m_clock, &rate) == noErr)
        return rate;
    return 0;
}

void PlatformClockCA::start()
{
    if (m_running)
        return;
    m_running = true;
    CAClockStart(m_clock);
}

void PlatformClockCA::stop()
{
    if (!m_running)
        return;
    m_running = false;
    CAClockStop(m_clock);
}


#endif
