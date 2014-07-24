/*
 *  Copyright (C) 2012 Samsung Electronics.  All rights reserved.
 *  Copyright (C) 2012 Intel Corporation. All rights reserved.
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
#include "BatteryClientEfl.h"

#if ENABLE(BATTERY_STATUS)

#include "BatteryController.h"
#include "ewk_view_private.h"

BatteryClientEfl::BatteryClientEfl(Evas_Object* view)
    : m_view(view)
    , m_provider(this)
{
}

BatteryClientEfl::~BatteryClientEfl()
{
    m_provider.stopUpdating();
}

void BatteryClientEfl::startUpdating()
{
    m_provider.startUpdating();
}

void BatteryClientEfl::stopUpdating()
{
    m_provider.stopUpdating();
}

void BatteryClientEfl::batteryControllerDestroyed()
{
    delete this;
}

void BatteryClientEfl::didChangeBatteryStatus(const AtomicString& eventType, PassRefPtr<WebCore::BatteryStatus> status)
{
    WebCore::BatteryController::from(EWKPrivate::corePage(m_view))->didChangeBatteryStatus(eventType, status);
}

#endif // ENABLE(BATTERY_STATUS)
