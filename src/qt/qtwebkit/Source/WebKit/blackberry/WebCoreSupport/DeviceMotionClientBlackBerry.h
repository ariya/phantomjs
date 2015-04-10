/*
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
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

#ifndef DeviceMotionClientBlackBerry_h
#define DeviceMotionClientBlackBerry_h

#include "DeviceMotionClient.h"
#include "DeviceMotionData.h"

#include <BlackBerryPlatformDeviceMotionTrackerListener.h>

namespace BlackBerry {
namespace WebKit {
class WebPagePrivate;
}
}

namespace BlackBerry {
namespace Platform {
class DeviceMotionTracker;
}
}

namespace WebCore {

class DeviceMotionClientBlackBerry : public DeviceMotionClient, public BlackBerry::Platform::DeviceMotionTrackerListener {
public:
    DeviceMotionClientBlackBerry(BlackBerry::WebKit::WebPagePrivate*);
    ~DeviceMotionClientBlackBerry();

    virtual void setController(DeviceMotionController*);
    virtual void startUpdating();
    virtual void stopUpdating();
    virtual DeviceMotionData* lastMotion() const;
    virtual void deviceMotionControllerDestroyed();
    virtual void onMotion(const BlackBerry::Platform::DeviceMotionEvent*);

private:
    BlackBerry::WebKit::WebPagePrivate* m_webPagePrivate;
    BlackBerry::Platform::DeviceMotionTracker* m_tracker;
    DeviceMotionController* m_controller;
    RefPtr<DeviceMotionData> m_currentMotion;
    double m_lastEventTime;
};
}

#endif // DeviceMotionClientBlackBerry_h
