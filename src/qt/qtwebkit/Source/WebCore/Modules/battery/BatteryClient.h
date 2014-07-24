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

#ifndef BatteryClient_h
#define BatteryClient_h

#if ENABLE(BATTERY_STATUS)

namespace WebCore {

class Page;

class BatteryClient {
public:
    virtual ~BatteryClient() { }

    virtual void startUpdating() = 0;
    virtual void stopUpdating() = 0;
    virtual void batteryControllerDestroyed() = 0;
};

void provideBatteryTo(Page*, BatteryClient*);

}

#endif // BATTERY_STATUS
#endif // BatteryClient_h

