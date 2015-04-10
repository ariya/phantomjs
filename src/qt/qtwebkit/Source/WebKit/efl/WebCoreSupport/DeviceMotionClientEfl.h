/*
 * Copyright (C) 2011 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
#ifndef DeviceMotionClientEfl_h
#define DeviceMotionClientEfl_h

#if ENABLE(DEVICE_ORIENTATION)

#include "DeviceMotionClient.h"
#include "DeviceMotionData.h"

namespace WebCore {

class DeviceMotionClientEfl : public DeviceMotionClient {
public:
    DeviceMotionClientEfl();
    virtual ~DeviceMotionClientEfl();

    virtual void setController(DeviceMotionController*);
    virtual void startUpdating();
    virtual void stopUpdating();
    virtual DeviceMotionData* lastMotion() const;
    virtual void deviceMotionControllerDestroyed();

private:
    DeviceMotionController* m_controller;
};

} // namespece WebCore

#endif
#endif // DeviceMotionClientEfl_h
