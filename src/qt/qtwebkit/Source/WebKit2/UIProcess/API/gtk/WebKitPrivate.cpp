/*
 * Copyright (C) 2012 Igalia S.L.
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
 */

#include "config.h"
#include "WebKitPrivate.h"

#include <gdk/gdk.h>

unsigned wkEventModifiersToGdkModifiers(WKEventModifiers wkModifiers)
{
    unsigned modifiers = 0;
    if (wkModifiers & kWKEventModifiersShiftKey)
        modifiers |= GDK_SHIFT_MASK;
    if (wkModifiers & kWKEventModifiersControlKey)
        modifiers |= GDK_CONTROL_MASK;
    if (wkModifiers & kWKEventModifiersAltKey)
        modifiers |= GDK_MOD1_MASK;
    if (wkModifiers & kWKEventModifiersMetaKey)
        modifiers |= GDK_META_MASK;
    return modifiers;
}

unsigned wkEventMouseButtonToWebKitMouseButton(WKEventMouseButton wkButton)
{
    switch (wkButton) {
    case kWKEventMouseButtonNoButton:
        return 0;
    case kWKEventMouseButtonLeftButton:
        return 1;
    case kWKEventMouseButtonMiddleButton:
        return 2;
    case kWKEventMouseButtonRightButton:
        return 3;
    }
    ASSERT_NOT_REACHED();
    return 0;
}
