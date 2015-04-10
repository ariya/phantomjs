/*
 *  Copyright (C) 2012 Samsung Electronics
 *  Copyright (C) 2012 Google Inc.
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
#include "BatteryController.h"

#if ENABLE(BATTERY_STATUS)

#include "BatteryClient.h"
#include "BatteryStatus.h"
#include "Event.h"

namespace WebCore {

BatteryController::BatteryController(BatteryClient* client)
    : m_client(client)
{
    ASSERT(m_client);
}

BatteryController::~BatteryController()
{
    for (ListenerVector::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
        (*it)->batteryControllerDestroyed();
    m_client->batteryControllerDestroyed();
}

PassOwnPtr<BatteryController> BatteryController::create(BatteryClient* client)
{
    return adoptPtr(new BatteryController(client));
}

void BatteryController::addListener(BatteryManager* batteryManager)
{
    m_listeners.append(batteryManager);
    m_client->startUpdating();

    if (m_batteryStatus)
        batteryManager->updateBatteryStatus(m_batteryStatus);
}

void BatteryController::removeListener(BatteryManager* batteryManager)
{
    size_t pos = m_listeners.find(batteryManager);
    if (pos == WTF::notFound)
        return;
    m_listeners.remove(pos);
    if (m_listeners.isEmpty())
        m_client->stopUpdating();
}

void BatteryController::updateBatteryStatus(PassRefPtr<BatteryStatus> batteryStatus)
{
    RefPtr<BatteryStatus> status = batteryStatus;
    if (m_batteryStatus) {
        if (m_batteryStatus->charging() != status->charging())
            didChangeBatteryStatus(WebCore::eventNames().chargingchangeEvent, status);
        else if (status->charging() && m_batteryStatus->chargingTime() != status->chargingTime())
            didChangeBatteryStatus(WebCore::eventNames().chargingtimechangeEvent, status);
        else if (!status->charging() && m_batteryStatus->dischargingTime() != status->dischargingTime())
            didChangeBatteryStatus(WebCore::eventNames().dischargingtimechangeEvent, status);

        if (m_batteryStatus->level() != status->level())
            didChangeBatteryStatus(WebCore::eventNames().levelchangeEvent, status);
    } else {
        for (ListenerVector::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
            (*it)->updateBatteryStatus(status);
    }

    m_batteryStatus = status.release();
}

void BatteryController::didChangeBatteryStatus(const AtomicString& eventType, PassRefPtr<BatteryStatus> batteryStatus)
{
    RefPtr<Event> event = Event::create(eventType, false, false);
    RefPtr<BatteryStatus> battery = batteryStatus;
    for (ListenerVector::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
        (*it)->didChangeBatteryStatus(event, battery);
}

const char* BatteryController::supplementName()
{
    return "BatteryController";
}

bool BatteryController::isActive(Page* page)
{
    return static_cast<bool>(BatteryController::from(page));
}

void provideBatteryTo(Page* page, BatteryClient* client)
{
    Supplement<Page>::provideTo(page, BatteryController::supplementName(), BatteryController::create(client));
}

}

#endif // BATTERY_STATUS

