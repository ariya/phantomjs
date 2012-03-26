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
#include "DeviceMotionClientQt.h"

#include "DeviceMotionController.h"
#include "DeviceMotionProviderQt.h"

#include "qwebpage.h"

namespace WebCore {

DeviceMotionClientQt::DeviceMotionClientQt(QWebPage* page)
    : m_page(page)
    , m_controller(0)
    , m_provider(new DeviceMotionProviderQt())
{

    connect(m_provider, SIGNAL(deviceMotionChanged()), SLOT(changeDeviceMotion()));
}

DeviceMotionClientQt::~DeviceMotionClientQt()
{
    disconnect();
    delete m_provider;
}

void DeviceMotionClientQt::setController(DeviceMotionController* controller)
{
    m_controller = controller;
}

void DeviceMotionClientQt::startUpdating()
{
    m_provider->start();
}

void DeviceMotionClientQt::stopUpdating()
{
    m_provider->stop();
}

DeviceMotionData* DeviceMotionClientQt::currentDeviceMotion() const
{
    return m_provider->currentDeviceMotion();
}

void DeviceMotionClientQt::deviceMotionControllerDestroyed()
{
    delete this;
}

void DeviceMotionClientQt::changeDeviceMotion()
{
    if (!m_controller)
        return;

    m_controller->didChangeDeviceMotion(currentDeviceMotion());
}

} // namespace WebCore

#include "moc_DeviceMotionClientQt.cpp"
