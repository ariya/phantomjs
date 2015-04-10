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
#ifndef DeviceOrientationProviderQt_h
#define DeviceOrientationProviderQt_h

#include "DeviceOrientationController.h"
#include "DeviceOrientationData.h"
#include <QRotationFilter>
#include <wtf/RefPtr.h>

namespace WebCore {

class DeviceOrientationProviderQt : public QRotationFilter {
public:
    DeviceOrientationProviderQt();
    virtual ~DeviceOrientationProviderQt();

    void setController(DeviceOrientationController*);

    bool filter(QRotationReading*);

    void start();
    void stop();
    bool isActive() const { return m_sensor.isActive(); }
    DeviceOrientationData* lastOrientation() const { return m_lastOrientation.get(); }
    bool hasAlpha() const { return m_sensor.property("hasZ").toBool(); }

private:
    RefPtr<DeviceOrientationData> m_lastOrientation;
    DeviceOrientationController* m_controller;
    QRotationSensor m_sensor;
};

}

#endif // DeviceOrientationProviderQt_h
