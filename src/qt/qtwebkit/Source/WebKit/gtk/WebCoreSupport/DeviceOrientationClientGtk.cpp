/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2011 Collabora Ltd.
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

#if ENABLE(DEVICE_ORIENTATION)
#include "DeviceOrientationClientGtk.h"

#include "NotImplemented.h"

using namespace WebCore;

namespace WebKit {

DeviceOrientationClientGtk::DeviceOrientationClientGtk()
{
}

DeviceOrientationClientGtk::~DeviceOrientationClientGtk()
{
}

void DeviceOrientationClientGtk::deviceOrientationControllerDestroyed()
{
    delete this;
}

void DeviceOrientationClientGtk::setController(DeviceOrientationController* controller)
{
    m_controller = controller;
}

void DeviceOrientationClientGtk::startUpdating()
{
    notImplemented();
}

void DeviceOrientationClientGtk::stopUpdating()
{
    notImplemented();
}

WebCore::DeviceOrientationData* DeviceOrientationClientGtk::lastOrientation() const
{
    notImplemented();
    return 0;
}

} // namespace WebKit
#endif
