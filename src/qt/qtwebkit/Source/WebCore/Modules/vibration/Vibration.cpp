/*
 *  Copyright (C) 2012 Samsung Electronics
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "Vibration.h"

#if ENABLE(VIBRATION)

#include "VibrationClient.h"

namespace WebCore {

Vibration::Vibration(VibrationClient* client)
    : m_vibrationClient(client)
    , m_timerStart(this, &Vibration::timerStartFired)
    , m_timerStop(this, &Vibration::timerStopFired)
    , m_isVibrating(false)
{
}

Vibration::~Vibration()
{
    m_vibrationClient->vibrationDestroyed();
}

PassOwnPtr<Vibration> Vibration::create(VibrationClient* client)
{
    return adoptPtr(new Vibration(client));
}

bool Vibration::vibrate(const VibrationPattern& pattern)
{
    size_t length = pattern.size();

    m_pattern = pattern;
    if (length && !(length % 2))
        m_pattern.removeLast();

    // Pre-exsiting instance need to be canceled when vibration is called.
    // And if time is 0, vibration have to be canceled also.
    if (m_isVibrating || (m_pattern.size() == 1 && !m_pattern[0]))
        cancelVibration();

    if (!m_pattern.size())
        return true;

    m_timerStart.startOneShot(0);
    m_isVibrating = true;
    return true;
}

void Vibration::cancelVibration()
{
    m_pattern.clear();
    if (m_isVibrating)
        stopVibration();
}

void Vibration::suspendVibration()
{
    if (!m_isVibrating)
        return;

    m_pattern.insert(0, m_timerStop.nextFireInterval());
    stopVibration();
}

void Vibration::resumeVibration()
{
    m_timerStart.startOneShot(0);
    m_isVibrating = true;
}

void Vibration::stopVibration()
{
    m_timerStart.stop();
    m_timerStop.stop();
    m_vibrationClient->cancelVibration();
    m_isVibrating = false;
}

void Vibration::timerStartFired(Timer<Vibration>* timer)
{
    ASSERT_UNUSED(timer, timer == &m_timerStart);

    m_timerStart.stop();

    if (!m_pattern.isEmpty()) {
        m_vibrationClient->vibrate(m_pattern[0]);
        m_timerStop.startOneShot(m_pattern[0] / 1000.0);
        m_pattern.remove(0);
    }
}

void Vibration::timerStopFired(Timer<Vibration>* timer)
{
    ASSERT_UNUSED(timer, timer == &m_timerStop);

    m_timerStop.stop();

    if (!m_pattern.isEmpty()) {
        m_timerStart.startOneShot(m_pattern[0] / 1000.0);
        m_pattern.remove(0);
        if (m_pattern.isEmpty())
            m_isVibrating = false;
    }
}

const char* Vibration::supplementName()
{
    return "Vibration";
}

bool Vibration::isActive(Page* page)
{
    return static_cast<bool>(Vibration::from(page));
}

void provideVibrationTo(Page* page, VibrationClient* client)
{
    Vibration::provideTo(page, Vibration::supplementName(), Vibration::create(client));
}

} // namespace WebCore

#endif // ENABLE(VIBRATION)

