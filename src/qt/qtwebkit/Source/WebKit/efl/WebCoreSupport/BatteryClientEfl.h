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

#ifndef BatteryClientEfl_h
#define BatteryClientEfl_h

#if ENABLE(BATTERY_STATUS)

#include "BatteryClient.h"
#include "BatteryProviderEfl.h"
#include "BatteryProviderEflClient.h"
#include "BatteryStatus.h"
#include <wtf/PassRefPtr.h>

namespace WebCore {
class BatteryController;
}

class BatteryClientEfl : public WebCore::BatteryClient, public WebCore::BatteryProviderEflClient {
public:
    explicit BatteryClientEfl(Evas_Object* view);
    virtual ~BatteryClientEfl();

    // BatteryClient interface.
    virtual void startUpdating();
    virtual void stopUpdating();
    virtual void batteryControllerDestroyed();

private:
    // BatteryProviderEflClient interface.
    virtual void didChangeBatteryStatus(const AtomicString& eventType, PassRefPtr<WebCore::BatteryStatus>);

    Evas_Object* m_view;
    WebCore::BatteryProviderEfl m_provider;
};

#endif // ENABLE(BATTERY_STATUS)

#endif // BatteryClientEfl_h
