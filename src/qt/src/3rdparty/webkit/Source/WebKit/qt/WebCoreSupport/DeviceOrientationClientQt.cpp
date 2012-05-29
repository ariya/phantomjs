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
#include "DeviceOrientationClientQt.h"

#include "DeviceOrientationClientMockQt.h"
#include "DeviceOrientationController.h"
#include "DeviceOrientationProviderQt.h"
#include "qwebpage.h"

namespace WebCore {

DeviceOrientationClientQt::DeviceOrientationClientQt(QWebPage* page)
    : m_page(page)
    , m_controller(0)
    , m_provider(new DeviceOrientationProviderQt())
{
    connect(m_provider, SIGNAL(deviceOrientationChanged(DeviceOrientation*)), SLOT(changeDeviceOrientation(DeviceOrientation*)));
}

DeviceOrientationClientQt::~DeviceOrientationClientQt()
{
    disconnect();
    delete m_provider;
}

void DeviceOrientationClientQt::setController(DeviceOrientationController* controller)
{
    m_controller = controller;
}

void DeviceOrientationClientQt::startUpdating()
{
    m_provider->start();
}

void DeviceOrientationClientQt::stopUpdating()
{
    m_provider->stop();
}

DeviceOrientation* DeviceOrientationClientQt::lastOrientation() const
{
    return m_provider->orientation();
}

void DeviceOrientationClientQt::deviceOrientationControllerDestroyed()
{
    delete this;
}

void DeviceOrientationClientQt::changeDeviceOrientation(DeviceOrientation* orientation)
{
    if (!m_controller)
        return;

    m_controller->didChangeDeviceOrientation(orientation);
}

} // namespace WebCore

#include "moc_DeviceOrientationClientQt.cpp"
