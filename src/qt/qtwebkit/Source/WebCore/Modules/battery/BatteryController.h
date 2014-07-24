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

#ifndef BatteryController_h
#define BatteryController_h

#if ENABLE(BATTERY_STATUS)

#include "BatteryManager.h"
#include "Page.h"

namespace WebCore {

class BatteryClient;

class BatteryController : public Supplement<Page> {
public:
    ~BatteryController();

    static PassOwnPtr<BatteryController> create(BatteryClient*);

    void addListener(BatteryManager*);
    void removeListener(BatteryManager*);
    void updateBatteryStatus(PassRefPtr<BatteryStatus>);
    void didChangeBatteryStatus(const AtomicString& eventType, PassRefPtr<BatteryStatus>);

    BatteryClient* client() const { return m_client; }

    static const char* supplementName();
    static BatteryController* from(Page* page) { return static_cast<BatteryController*>(Supplement<Page>::from(page, supplementName())); }
    static bool isActive(Page*);

private:
    typedef Vector<BatteryManager*> ListenerVector;

    explicit BatteryController(BatteryClient*);

    BatteryClient* m_client;
    ListenerVector m_listeners;

    RefPtr<BatteryStatus> m_batteryStatus;
};

}

#endif // BATTERY_STATUS
#endif // BatteryController_h

