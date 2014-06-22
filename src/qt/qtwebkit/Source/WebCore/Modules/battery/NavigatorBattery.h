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

#ifndef NavigatorBattery_h
#define NavigatorBattery_h

#if ENABLE(BATTERY_STATUS)

#include "Supplementable.h"

namespace WebCore {

class BatteryManager;
class Navigator;
class ScriptExecutionContext;

class NavigatorBattery : public Supplement<Navigator> {
public:
    virtual ~NavigatorBattery();

    static NavigatorBattery* from(Navigator*);

    static BatteryManager* webkitBattery(Navigator*);
    BatteryManager* batteryManager();

 private:
    NavigatorBattery();
    static const char* supplementName();

    RefPtr<BatteryManager> m_batteryManager;
};

} // namespace WebCore

#endif // ENABLE(BATTERY_STATUS)

#endif // NavigatorBattery_h


