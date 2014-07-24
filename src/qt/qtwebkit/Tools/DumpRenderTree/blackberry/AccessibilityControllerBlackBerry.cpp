/*
 * Copyright (C) 2010, 2012 Research In Motion Limited. All rights reserved.
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
#include "AccessibilityController.h"

#include "AccessibilityUIElement.h"
#include "NotImplemented.h"

AccessibilityController::AccessibilityController()
{
}

AccessibilityController::~AccessibilityController()
{
}

AccessibilityUIElement AccessibilityController::focusedElement()
{
    notImplemented();
    return 0;
}

AccessibilityUIElement AccessibilityController::rootElement()
{
    notImplemented();
    return 0;
}

void AccessibilityController::setLogFocusEvents(bool)
{
    notImplemented();
}

void AccessibilityController::setLogScrollingStartEvents(bool)
{
    notImplemented();
}

void AccessibilityController::setLogValueChangeEvents(bool)
{
    notImplemented();
}

AccessibilityUIElement AccessibilityController::elementAtPoint(int, int)
{
    notImplemented();
    return 0;
}

void AccessibilityController::setLogAccessibilityEvents(bool)
{
    notImplemented();
}

bool AccessibilityController::addNotificationListener(JSObjectRef)
{
    return false;
}

void AccessibilityController::removeNotificationListener()
{
}

AccessibilityUIElement AccessibilityController::accessibleElementById(JSStringRef)
{
    notImplemented();
    return 0;
}
