/*
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef DeviceMotionClientQt_h
#define DeviceMotionClientQt_h

#include "DeviceMotionClient.h"
#include "DeviceMotionData.h"

#include "DeviceMotionProviderQt.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

class DeviceMotionController;
class DeviceMotionProviderQt;

class DeviceMotionClientQt : public DeviceMotionClient {
public:
    DeviceMotionClientQt() { }
    virtual ~DeviceMotionClientQt();

private:
    virtual void deviceMotionControllerDestroyed();

    virtual void startUpdating();
    virtual void stopUpdating();

    virtual DeviceMotionData* lastMotion() const;

    virtual void setController(DeviceMotionController*);

    OwnPtr<DeviceMotionProviderQt> m_provider;
};

} // namespece WebCore

#endif // DeviceMotionClientQt_h
