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
#include "DeviceOrientationClientMockQt.h"

#include "DeviceOrientation.h"
#include "DeviceOrientationClientMock.h"
#include "DeviceOrientationController.h"

namespace WebCore {

bool DeviceOrientationClientMockQt::mockIsActive = false;

DeviceOrientationClientMockQt* DeviceOrientationClientMockQt::client()
{
    static DeviceOrientationClientMockQt* client = 0;
    if (!client)
        client = new DeviceOrientationClientMockQt;

    return client;
}

DeviceOrientationClientMockQt::DeviceOrientationClientMockQt()
    : m_clientMock(new DeviceOrientationClientMock())
{
    m_orientation = DeviceOrientation::create();
}

DeviceOrientationClientMockQt::~DeviceOrientationClientMockQt()
{
    delete m_clientMock;
}

void DeviceOrientationClientMockQt::setController(DeviceOrientationController* controller)
{
    m_clientMock->setController(m_controller);
}

void DeviceOrientationClientMockQt::startUpdating()
{
    m_clientMock->startUpdating();
}

void DeviceOrientationClientMockQt::stopUpdating()
{
    m_clientMock->stopUpdating();
}

DeviceOrientation* DeviceOrientationClientMockQt::lastOrientation() const
{
    return m_orientation.get();
}

void DeviceOrientationClientMockQt::deviceOrientationControllerDestroyed()
{
    delete this;
}

void DeviceOrientationClientMockQt::setOrientation(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma)
{
    m_orientation = DeviceOrientation::create(canProvideAlpha, alpha,
            canProvideBeta, beta,
            canProvideGamma, gamma);
    m_clientMock->setOrientation(m_orientation);

    emit mockOrientationChanged(m_orientation.get());
}

} // namespace WebCore

#include "moc_DeviceOrientationClientMockQt.cpp"
