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
#include "BatteryManager.h"

#if ENABLE(BATTERY_STATUS)

#include "BatteryController.h"
#include "BatteryStatus.h"
#include "Document.h"
#include "Event.h"
#include "Frame.h"
#include "Navigator.h"
#include <limits>

namespace WebCore {

PassRefPtr<BatteryManager> BatteryManager::create(Navigator* navigator)
{
    RefPtr<BatteryManager> batteryManager(adoptRef(new BatteryManager(navigator)));
    batteryManager->suspendIfNeeded();
    return batteryManager.release();
}

BatteryManager::~BatteryManager()
{
}

BatteryManager::BatteryManager(Navigator* navigator)
    : ActiveDOMObject(navigator->frame()->document())
    , m_batteryController(BatteryController::from(navigator->frame()->page()))
    , m_batteryStatus(0)
{
    m_batteryController->addListener(this);
}

bool BatteryManager::charging()
{
    return m_batteryStatus ? m_batteryStatus->charging() : true;
}

double BatteryManager::chargingTime()
{
    if (!m_batteryStatus || !m_batteryStatus->charging())
        return std::numeric_limits<double>::infinity();

    return m_batteryStatus->chargingTime();
}

double BatteryManager::dischargingTime()
{
    if (!m_batteryStatus || m_batteryStatus->charging())
        return std::numeric_limits<double>::infinity();

    return m_batteryStatus->dischargingTime();
}

double BatteryManager::level()
{
    return m_batteryStatus ? m_batteryStatus->level() : 1;
}

void BatteryManager::didChangeBatteryStatus(PassRefPtr<Event> event, PassRefPtr<BatteryStatus> batteryStatus)
{
    updateBatteryStatus(batteryStatus);
    dispatchEvent(event);
}

void BatteryManager::updateBatteryStatus(PassRefPtr<BatteryStatus> batteryStatus)
{
    m_batteryStatus = batteryStatus;
}

void BatteryManager::suspend(ReasonForSuspension)
{
    if (m_batteryController)
        m_batteryController->removeListener(this);
}

void BatteryManager::resume()
{
    if (m_batteryController)
        m_batteryController->addListener(this);
}

void BatteryManager::stop()
{
    if (m_batteryController)
        m_batteryController->removeListener(this);
}

} // namespace WebCore

#endif // BATTERY_STATUS

