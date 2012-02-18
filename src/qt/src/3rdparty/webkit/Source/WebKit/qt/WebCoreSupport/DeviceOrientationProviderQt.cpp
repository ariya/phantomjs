/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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

#include "DeviceOrientationClientMockQt.h"

namespace WebCore {

DeviceOrientationProviderQt::DeviceOrientationProviderQt()
{
    m_rotation.addFilter(this);
    m_orientation = DeviceOrientation::create();

    if (DeviceOrientationClientMockQt::mockIsActive)
        activeClientMock();
}

DeviceOrientationProviderQt::~DeviceOrientationProviderQt()
{
    disconnect();
}

void DeviceOrientationProviderQt::start()
{
    m_rotation.start();
}

void DeviceOrientationProviderQt::stop()
{
    m_rotation.stop();
}

bool DeviceOrientationProviderQt::filter(QRotationReading* reading)
{
    // Provide device orientation data according W3C spec:
    // http://dev.w3.org/geo/api/spec-source-orientation.html
    // Qt mobility provide these data via QRotationSensor using the
    // QRotationReading class:
    //  - the rotation around z axis (alpha) is given as z in QRotationReading;
    //  - the rotation around x axis (beta) is given as x in QRotationReading;
    //  - the rotation around y axis (gamma) is given as y in QRotationReading;
    // See: http://doc.qt.nokia.com/qtmobility-1.0/qrotationreading.html
    // The Z (alpha) rotation angle is checked via hasAlpha() private method,
    // depending if the device is able do detect the alpha rotation. X (beta) and
    // Y (gamma) axis are availble in this context.
    m_orientation = DeviceOrientation::create(hasAlpha(), reading->z(),
            /* x available */ true, reading->x(),
            /* y available */ true, reading->y());
    emit deviceOrientationChanged(m_orientation.get());

    return false;
}

void DeviceOrientationProviderQt::changeDeviceOrientation(DeviceOrientation* orientation)
{
    m_orientation = orientation;
}

void DeviceOrientationProviderQt::activeClientMock()
{
    connect(DeviceOrientationClientMockQt::client(), SIGNAL(mockOrientationChanged(DeviceOrientation*)), SLOT(changeDeviceOrientation(DeviceOrientation*)));
}

}

#include "moc_DeviceOrientationProviderQt.cpp"
