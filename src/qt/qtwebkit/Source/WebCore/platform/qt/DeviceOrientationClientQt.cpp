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
#include "DeviceOrientationClientQt.h"

#include "DeviceOrientationProviderQt.h"

namespace WebCore {

void DeviceOrientationClientQt::deviceOrientationControllerDestroyed()
{
    delete this;
}

void DeviceOrientationClientQt::setController(DeviceOrientationController* controller)
{
    // Initialize lazily.
    if (!m_provider)
        m_provider = adoptPtr(new DeviceOrientationProviderQt);

    m_provider->setController(controller);
}

void DeviceOrientationClientQt::startUpdating()
{
    if (m_provider)
        m_provider->start();
}

void DeviceOrientationClientQt::stopUpdating()
{
    if (m_provider)
        m_provider->stop();
}

DeviceOrientationData* DeviceOrientationClientQt::lastOrientation() const
{
    return (m_provider) ? m_provider->lastOrientation() : 0;
}


} // namespace WebCore
