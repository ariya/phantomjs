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

#ifndef BatteryProviderEfl_h
#define BatteryProviderEfl_h

#if ENABLE(BATTERY_STATUS)

#include "BatteryClient.h"
#include "BatteryStatus.h"
#include "Timer.h"
#include <wtf/text/AtomicString.h>

typedef struct DBusError DBusError;

namespace WebCore {

class BatteryProviderEflClient;

class BatteryProviderEfl {
public:
    BatteryProviderEfl(BatteryProviderEflClient*);
    ~BatteryProviderEfl() { }

    virtual void startUpdating();
    virtual void stopUpdating();

    void setBatteryStatus(const AtomicString& eventType, PassRefPtr<BatteryStatus>);
    BatteryStatus* batteryStatus() const;

private:
    void timerFired(Timer<BatteryProviderEfl>*);
    static void getBatteryStatus(void* data, void* replyData, DBusError*);
    static void setBatteryClient(void* data, void* replyData, DBusError*);

    BatteryProviderEflClient* m_client;
    Timer<BatteryProviderEfl> m_timer;
    RefPtr<BatteryStatus> m_batteryStatus;
    const double m_batteryStatusRefreshInterval;
};

}

#endif // ENABLE(BATTERY_STATUS)
#endif // BatteryProviderEfl_h

