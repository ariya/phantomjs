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

#ifndef BatteryClientBlackBerry_h
#define BatteryClientBlackBerry_h

#if ENABLE(BATTERY_STATUS)

#include "BatteryClient.h"

#include <BatteryStatusHandler.h>

namespace BlackBerry {
namespace WebKit {
class WebPagePrivate;
}
}

namespace WebCore {

class BatteryStatus;

class BatteryClientBlackBerry : public BatteryClient, public BlackBerry::Platform::BatteryStatusListener {
public:
    explicit BatteryClientBlackBerry(BlackBerry::WebKit::WebPagePrivate*);
    ~BatteryClientBlackBerry() { }

    virtual void startUpdating();
    virtual void stopUpdating();
    virtual void batteryControllerDestroyed();

    void onStatusChange(bool charging, double chargingTime, double dischargingTime, double level);

private:
    BlackBerry::WebKit::WebPagePrivate* m_webPagePrivate;
    bool m_isActive;
};

}

#endif // BATTERY_STATUS
#endif // BatteryClientBlackBerry_h
