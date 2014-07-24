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
#include "DeviceOrientationProviderQt.h"

namespace WebCore {

DeviceOrientationProviderQt::DeviceOrientationProviderQt()
    : m_controller(0)
{
    m_sensor.addFilter(this);
    m_lastOrientation = DeviceOrientationData::create();
}

DeviceOrientationProviderQt::~DeviceOrientationProviderQt()
{

}

void DeviceOrientationProviderQt::setController(DeviceOrientationController* controller)
{
    if (!controller)
        stop();

    m_controller = controller;
}

void DeviceOrientationProviderQt::start()
{
    m_sensor.start();
}

void DeviceOrientationProviderQt::stop()
{
    m_sensor.stop();
}

bool DeviceOrientationProviderQt::filter(QRotationReading* reading)
{
    if (m_controller) {
        // Provide device orientation data according W3C spec:
        // http://dev.w3.org/geo/api/spec-source-orientation.html
        // Qt mobility (QtSensors in Qt5) provide these data via QRotationSensor
        // using the QRotationReading class:
        //  - the rotation around z axis (alpha) is given as z in QRotationReading;
        //  - the rotation around x axis (beta) is given as x in QRotationReading;
        //  - the rotation around y axis (gamma) is given as y in QRotationReading;
        // See: http://doc.qt.nokia.com/qtmobility-1.0/qrotationreading.html
        // The Z (alpha) rotation angle is checked via hasAlpha() private method,
        // depending if the device is able do detect the alpha rotation. X (beta) and
        // Y (gamma) axis are availble in this context.
        m_lastOrientation = DeviceOrientationData::create(hasAlpha(), reading->z(),
                /* x available */ true, reading->x(),
                /* y available */ true, reading->y());
        m_controller->didChangeDeviceOrientation(m_lastOrientation.get());
    }

    // We are the only filter, so no need to propagate.
    return false;
}

}
