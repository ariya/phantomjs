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
#ifndef DeviceOrientationProviderQt_h
#define DeviceOrientationProviderQt_h

#include "DeviceOrientation.h"
#include "RefPtr.h"

#include <QObject>
#include <QRotationFilter>

QTM_USE_NAMESPACE

namespace WebCore {

class DeviceOrientationClientQt;

class DeviceOrientationProviderQt : public QObject, public QRotationFilter {
    Q_OBJECT
public:
    DeviceOrientationProviderQt();
    ~DeviceOrientationProviderQt();

    bool filter(QRotationReading*);
    void start();
    void stop();
    bool isActive() const { return m_rotation.isActive(); }
    DeviceOrientation* orientation() const { return m_orientation.get(); }
    bool hasAlpha() const { return m_rotation.property("hasZ").toBool(); }

Q_SIGNALS:
    void deviceOrientationChanged(DeviceOrientation*);

public Q_SLOTS:
    void changeDeviceOrientation(DeviceOrientation*);

private:
    void activeClientMock();

    RefPtr<DeviceOrientation> m_orientation;
    QRotationSensor m_rotation;
};

}

#endif // DeviceOrientationProviderQt_h
