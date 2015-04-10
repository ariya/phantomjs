/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "BatteryClientBlackBerry.h"

#if ENABLE(BATTERY_STATUS)

#include "BatteryController.h"
#include "WebPage_p.h"
#include <stdio.h>

namespace WebCore {

BatteryClientBlackBerry::BatteryClientBlackBerry(BlackBerry::WebKit::WebPagePrivate* webPagePrivate)
    : m_webPagePrivate(webPagePrivate)
    , m_isActive(false)
{
}

void BatteryClientBlackBerry::startUpdating()
{
    if (!m_isActive) {
        BlackBerry::Platform::BatteryStatusHandler::instance()->addListener(this);
        m_isActive = true;
    }
}

void BatteryClientBlackBerry::stopUpdating()
{
    if (m_isActive) {
        BlackBerry::Platform::BatteryStatusHandler::instance()->removeListener(this);
        m_isActive = false;
    }
}

void BatteryClientBlackBerry::batteryControllerDestroyed()
{
    delete this;
}

void BatteryClientBlackBerry::onStatusChange(bool charging, double chargingTime, double dischargingTime, double level)
{
    BatteryController::from(m_webPagePrivate->m_page)->updateBatteryStatus(BatteryStatus::create(charging, chargingTime, dischargingTime, level));
}

}

#endif // BATTERY_STATUS
