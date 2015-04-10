/*
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "DeviceOrientationClientBlackBerry.h"

#include "DeviceOrientationController.h"
#include "WebPage_p.h"
#include <BlackBerryPlatformDeviceOrientationTracker.h>

using namespace WebCore;

DeviceOrientationClientBlackBerry::DeviceOrientationClientBlackBerry(BlackBerry::WebKit::WebPagePrivate* webPagePrivate)
    : m_webPagePrivate(webPagePrivate)
    , m_tracker(0)
    , m_controller(0)
{
}

DeviceOrientationClientBlackBerry::~DeviceOrientationClientBlackBerry()
{
    if (m_tracker)
        m_tracker->destroy();
}

void DeviceOrientationClientBlackBerry::setController(DeviceOrientationController* controller)
{
    m_controller = controller;
}

void DeviceOrientationClientBlackBerry::deviceOrientationControllerDestroyed()
{
    delete this;
}

void DeviceOrientationClientBlackBerry::startUpdating()
{
    if (m_tracker)
        m_tracker->resume();
    else
        m_tracker = BlackBerry::Platform::DeviceOrientationTracker::create(this);
}

void DeviceOrientationClientBlackBerry::stopUpdating()
{
    if (m_tracker)
        m_tracker->suspend();
}

DeviceOrientationData* DeviceOrientationClientBlackBerry::lastOrientation() const
{
    return m_currentOrientation.get();
}

void DeviceOrientationClientBlackBerry::onOrientation(const BlackBerry::Platform::DeviceOrientationEvent* event)
{
    m_currentOrientation = DeviceOrientationData::create(true, event->alpha, true, event->beta, true, event->gamma);
    if (!m_controller)
        return;

    m_controller->didChangeDeviceOrientation(lastOrientation());
}
