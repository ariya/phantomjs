/*
 * Copyright (C) 2009, 2010, 2011 Research In Motion Limited. All rights reserved.
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
#include "PlatformMouseEvent.h"

#include <wtf/CurrentTime.h>

namespace WebCore {

PlatformMouseEvent::PlatformMouseEvent(const IntPoint& eventPosition, const IntPoint& globalPosition, const PlatformEvent::Type type, int clickCount, MouseButton button, bool shiftKey, bool ctrlKey, bool altKey, MouseInputMethod method)
    : PlatformEvent(type, shiftKey, ctrlKey, altKey, false, currentTime())
    , m_position(eventPosition)
    , m_globalPosition(globalPosition)
    , m_button(button)
    , m_clickCount(clickCount)
    , m_inputMethod(method)
{
    ASSERT(type != MouseScroll);
}

} // namespace WebCore
