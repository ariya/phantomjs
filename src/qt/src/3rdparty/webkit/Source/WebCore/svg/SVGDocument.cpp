/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
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
#include "SVGDocument.h"

#include "EventNames.h"
#include "ExceptionCode.h"
#include "FrameView.h"
#include "RenderView.h"
#include "SVGElement.h"
#include "SVGNames.h"
#include "SVGSVGElement.h"
#include "SVGViewSpec.h"
#include "SVGZoomAndPan.h"
#include "SVGZoomEvent.h"

namespace WebCore {

SVGDocument::SVGDocument(Frame* frame, const KURL& url)
    : Document(frame, url, false, false)
{
}

SVGSVGElement* SVGDocument::rootElement() const
{
    Element* elem = documentElement();
    if (elem && elem->hasTagName(SVGNames::svgTag))
        return static_cast<SVGSVGElement*>(elem);

    return 0;
}

void SVGDocument::dispatchZoomEvent(float prevScale, float newScale)
{
    ExceptionCode ec = 0;
    RefPtr<SVGZoomEvent> event = static_pointer_cast<SVGZoomEvent>(createEvent("SVGZoomEvents", ec));
    event->initEvent(eventNames().zoomEvent, true, false);
    event->setPreviousScale(prevScale);
    event->setNewScale(newScale);
    rootElement()->dispatchEvent(event.release(), ec);
}

void SVGDocument::dispatchScrollEvent()
{
    ExceptionCode ec = 0;
    RefPtr<Event> event = createEvent("SVGEvents", ec);
    event->initEvent(eventNames().scrollEvent, true, false);
    rootElement()->dispatchEvent(event.release(), ec);
}

bool SVGDocument::zoomAndPanEnabled() const
{
    if (rootElement()) {
        if (rootElement()->useCurrentView()) {
            if (rootElement()->currentView())
                return rootElement()->currentView()->zoomAndPan() == SVGZoomAndPan::SVG_ZOOMANDPAN_MAGNIFY;
        } else
            return rootElement()->zoomAndPan() == SVGZoomAndPan::SVG_ZOOMANDPAN_MAGNIFY;
    }

    return false;
}

void SVGDocument::startPan(const FloatPoint& start)
{
    if (rootElement())
        m_translate = FloatPoint(start.x() - rootElement()->currentTranslate().x(), rootElement()->currentTranslate().y() + start.y());
}

void SVGDocument::updatePan(const FloatPoint& pos) const
{
    if (rootElement()) {
        rootElement()->setCurrentTranslate(FloatPoint(pos.x() - m_translate.x(), m_translate.y() - pos.y()));
        if (renderer())
            renderer()->repaint();
    }
}

bool SVGDocument::childShouldCreateRenderer(Node* node) const
{
    if (node->hasTagName(SVGNames::svgTag))
        return static_cast<SVGSVGElement*>(node)->isValid();
    return true;
}

}

// vim:ts=4:noet
#endif // ENABLE(SVG)
