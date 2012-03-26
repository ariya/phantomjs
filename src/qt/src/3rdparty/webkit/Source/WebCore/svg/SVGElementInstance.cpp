/*
 * Copyright (C) 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2011 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
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
#include "SVGElementInstance.h"

#include "ContainerNodeAlgorithms.h"
#include "Event.h"
#include "EventException.h"
#include "EventListener.h"
#include "EventNames.h"
#include "FrameView.h"
#include "SVGElementInstanceList.h"
#include "SVGUseElement.h"

#include <wtf/RefCountedLeakCounter.h>

namespace WebCore {

#ifndef NDEBUG
static WTF::RefCountedLeakCounter instanceCounter("WebCoreSVGElementInstance");
#endif

SVGElementInstance::SVGElementInstance(SVGUseElement* correspondingUseElement, SVGUseElement* directUseElement, PassRefPtr<SVGElement> originalElement)
    : m_correspondingUseElement(correspondingUseElement)
    , m_directUseElement(directUseElement)
    , m_element(originalElement)
    , m_previousSibling(0)
    , m_nextSibling(0)
    , m_firstChild(0)
    , m_lastChild(0)
{
    ASSERT(m_correspondingUseElement);
    ASSERT(m_element);

    // Register as instance for passed element.
    m_element->mapInstanceToElement(this);

#ifndef NDEBUG
    instanceCounter.increment();
#endif
}

SVGElementInstance::~SVGElementInstance()
{
#ifndef NDEBUG
    instanceCounter.decrement();
#endif

    // Deregister as instance for passed element.
    m_element->removeInstanceMapping(this);

    clearChildren();
}

void SVGElementInstance::clearChildren()
{
    removeAllChildrenInContainer<SVGElementInstance, SVGElementInstance>(this);
}

PassRefPtr<SVGElementInstanceList> SVGElementInstance::childNodes()
{
    return SVGElementInstanceList::create(this);
}

void SVGElementInstance::setShadowTreeElement(SVGElement* element)
{
    ASSERT(element);
    m_shadowTreeElement = element;
}

void SVGElementInstance::appendChild(PassRefPtr<SVGElementInstance> child)
{
    appendChildToContainer<SVGElementInstance, SVGElementInstance>(child.get(), this);
}

void SVGElementInstance::invalidateAllInstancesOfElement(SVGElement* element)
{
    if (!element || !element->inDocument())
        return;

    if (element->isStyled() && static_cast<SVGStyledElement*>(element)->instanceUpdatesBlocked())
        return;

    const HashSet<SVGElementInstance*>& set = element->instancesForElement();
    if (set.isEmpty())
        return;

    // Mark all use elements referencing 'element' for rebuilding
    const HashSet<SVGElementInstance*>::const_iterator end = set.end();
    for (HashSet<SVGElementInstance*>::const_iterator it = set.begin(); it != end; ++it) {
        ASSERT((*it)->correspondingElement() == element);
        if (SVGUseElement* element = (*it)->correspondingUseElement()) {
            ASSERT(element->inDocument());
            element->invalidateShadowTree();
        }
    }

    // Be sure to rebuild use trees, if needed
    element->document()->updateLayoutIgnorePendingStylesheets();
}

ScriptExecutionContext* SVGElementInstance::scriptExecutionContext() const
{
    return m_element->document();
}

bool SVGElementInstance::addEventListener(const AtomicString& eventType, PassRefPtr<EventListener> listener, bool useCapture)
{
    return m_element->addEventListener(eventType, listener, useCapture);
}

bool SVGElementInstance::removeEventListener(const AtomicString& eventType, EventListener* listener, bool useCapture)
{
    return m_element->removeEventListener(eventType, listener, useCapture);
}

void SVGElementInstance::removeAllEventListeners()
{
    m_element->removeAllEventListeners();
}

bool SVGElementInstance::dispatchEvent(PassRefPtr<Event> event)
{
    SVGElement* element = shadowTreeElement();
    if (!element)
        return false;

    return element->dispatchEvent(event);
}

EventTargetData* SVGElementInstance::eventTargetData()
{
    // EventTarget would use these methods if we were actually using its add/removeEventListener logic.
    // As we're forwarding those calls to the correspondingElement(), no one should ever call this function.
    ASSERT_NOT_REACHED();
    return 0;
}

EventTargetData* SVGElementInstance::ensureEventTargetData()
{
    // EventTarget would use these methods if we were actually using its add/removeEventListener logic.
    // As we're forwarding those calls to the correspondingElement(), no one should ever call this function.
    ASSERT_NOT_REACHED();
    return 0;
}

}

#endif
