/*
 *  Copyright (C) 2012 Samsung Electronics
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "VibrationClientEfl.h"

#if ENABLE(VIBRATION)

namespace WebCore {

VibrationClientEfl::VibrationClientEfl(Evas_Object* view)
    : m_view(view)
{
}

void VibrationClientEfl::vibrate(const unsigned& time)
{
    evas_object_smart_callback_call(m_view, "vibration,vibrate", (void*)&time);
}

void VibrationClientEfl::cancelVibration()
{
    evas_object_smart_callback_call(m_view, "vibration,cancel", 0);
}

void VibrationClientEfl::vibrationDestroyed()
{
    delete this;
}

} // namespace WebCore

#endif // ENABLE(VIBRATION)

