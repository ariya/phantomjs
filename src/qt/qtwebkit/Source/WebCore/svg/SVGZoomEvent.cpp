/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
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

#if ENABLE(SVG)

#include "SVGZoomEvent.h"
#include "EventNames.h"

namespace WebCore {

SVGZoomEvent::SVGZoomEvent()
    : m_newScale(0.0f)
    , m_previousScale(0.0f)
{
}

FloatRect SVGZoomEvent::zoomRectScreen() const
{
    return m_zoomRectScreen;
}

float SVGZoomEvent::previousScale() const
{
    return m_previousScale;
}

void SVGZoomEvent::setPreviousScale(float scale)
{
    m_previousScale = scale;
}

SVGPoint SVGZoomEvent::previousTranslate() const
{
    return m_previousTranslate;
}

float SVGZoomEvent::newScale() const
{
    return m_newScale;
}

void SVGZoomEvent::setNewScale(float scale)
{
    m_newScale = scale;
}

SVGPoint SVGZoomEvent::newTranslate() const
{
    return m_newTranslate;
}

const AtomicString& SVGZoomEvent::interfaceName() const
{
    return eventNames().interfaceForSVGZoomEvent;
}

} // namespace WebCore

#endif // ENABLE(SVG)
