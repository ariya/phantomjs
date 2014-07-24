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

#ifndef BatteryStatus_h
#define BatteryStatus_h

#if ENABLE(BATTERY_STATUS)

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class BatteryStatus : public RefCounted<BatteryStatus> {
public:
    static PassRefPtr<BatteryStatus> create();
    static PassRefPtr<BatteryStatus> create(bool charging, double chargingTime, double dischargingTime, double level);

    bool charging() const { return m_charging; }
    double chargingTime() const  { return m_chargingTime; }
    double dischargingTime() const  { return m_dischargingTime; }
    double level() const  { return m_level; }

private:
    BatteryStatus();
    BatteryStatus(bool charging, double chargingTime, double dischargingTime, double level);

    bool m_charging;
    double m_chargingTime;
    double m_dischargingTime;
    double m_level;
};

} // namespace WebCore

#endif // BATTERY_STATUS
#endif // BatteryStatus_h

