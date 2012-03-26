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
#ifndef DeviceOrientationClientMockQt_h
#define DeviceOrientationClientMockQt_h

#include "DeviceOrientationClient.h"
#include "RefPtr.h"

#include <QObject>

namespace WebCore {

class DeviceOrientation;
class DeviceOrientationClientMock;
class DeviceOrientationController;

class DeviceOrientationClientMockQt : public QObject, public DeviceOrientationClient {
    Q_OBJECT
public:
    static DeviceOrientationClientMockQt* client();
    virtual ~DeviceOrientationClientMockQt();

    virtual void setController(DeviceOrientationController*);
    virtual void startUpdating();
    virtual void stopUpdating();
    virtual DeviceOrientation* lastOrientation() const;
    virtual void deviceOrientationControllerDestroyed();
    void setOrientation(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma);
    static bool mockIsActive;

Q_SIGNALS:
    void mockOrientationChanged(DeviceOrientation*);

private:
    DeviceOrientationClientMockQt();

    DeviceOrientationClientMock* m_clientMock;
    DeviceOrientationController* m_controller;
    RefPtr<DeviceOrientation> m_orientation;
};

} // namespace WebCore

#endif // DeviceOrientationClientMockQt_h
