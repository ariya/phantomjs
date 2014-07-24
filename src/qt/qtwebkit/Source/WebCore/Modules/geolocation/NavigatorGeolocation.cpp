/*
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *  Copyright (c) 2000 Daniel Molkentin (molkentin@kde.org)
 *  Copyright (c) 2000 Stefan Schimanski (schimmi@kde.org)
 *  Copyright (C) 2003, 2004, 2005, 2006 Apple Computer, Inc.
 *  Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#if ENABLE(GEOLOCATION)

#include "NavigatorGeolocation.h"

#include "Document.h"
#include "Frame.h"
#include "Geolocation.h"
#include "Navigator.h"

namespace WebCore {

NavigatorGeolocation::NavigatorGeolocation(Frame* frame)
    : DOMWindowProperty(frame)
{
}

NavigatorGeolocation::~NavigatorGeolocation()
{
}

const char* NavigatorGeolocation::supplementName()
{
    return "NavigatorGeolocation";
}

NavigatorGeolocation* NavigatorGeolocation::from(Navigator* navigator)
{
    NavigatorGeolocation* supplement = static_cast<NavigatorGeolocation*>(Supplement<Navigator>::from(navigator, supplementName()));
    if (!supplement) {
        supplement = new NavigatorGeolocation(navigator->frame());
        provideTo(navigator, supplementName(), adoptPtr(supplement));
    }
    return supplement;
}

Geolocation* NavigatorGeolocation::geolocation(Navigator* navigator)
{
    return NavigatorGeolocation::from(navigator)->geolocation();
}

Geolocation* NavigatorGeolocation::geolocation() const
{
    if (!m_geolocation && frame())
        m_geolocation = Geolocation::create(frame()->document());
    return m_geolocation.get();
}

} // namespace WebCore

#endif // ENABLE(GEOLOCATION)
