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

#include "config.h"
#include "DeviceMotionProviderQt.h"

#include "DeviceMotionController.h"
#include "DeviceOrientationProviderQt.h"

namespace WebCore {

DeviceMotionProviderQt::DeviceMotionProviderQt()
    : m_motion(DeviceMotionData::create())
    , m_deviceOrientation(new DeviceOrientationProviderQt)
    , m_controller(0)
{
    m_acceleration.addFilter(this);
}

DeviceMotionProviderQt::~DeviceMotionProviderQt()
{
    delete m_deviceOrientation;
}

void DeviceMotionProviderQt::setController(DeviceMotionController* controller)
{
    ASSERT(controller);
    ASSERT(!m_controller);
    m_controller = controller;
}

void DeviceMotionProviderQt::start()
{
    m_acceleration.start();
    m_deviceOrientation->start();
}

void DeviceMotionProviderQt::stop()
{
    m_acceleration.stop();
    m_deviceOrientation->stop();
}

bool DeviceMotionProviderQt::filter(QAccelerometerReading* reading)
{
    if (!m_controller) {
        // We are the only filter. No need to propagate from here.
        return false;
    }

    RefPtr<DeviceMotionData::Acceleration> accel = DeviceMotionData::Acceleration::create(
            /* x available */ true, reading->x(),
            /* y available */ true, reading->y(),
            /* z available */ true, reading->z());

    RefPtr<DeviceMotionData::RotationRate> rotation = DeviceMotionData::RotationRate::create(
            m_deviceOrientation->hasAlpha(), m_deviceOrientation->lastOrientation()->alpha(),
            /* beta available */ true, m_deviceOrientation->lastOrientation()->beta(),
            /* gamma available */ true, m_deviceOrientation->lastOrientation()->gamma());

    m_motion = DeviceMotionData::create(accel,
            accel, /* FIXME: Needs to provide acceleration include gravity. */
            rotation,
            false, 0 /* The interval is treated internally by Qt mobility (QtSensors in Qt5) */);

    m_controller->didChangeDeviceMotion(m_motion.get());

    // We are the only filter. No need to propagate from here.
    return false;
}

} // namespace WebCore
