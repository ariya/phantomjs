/*
 * Copyright (C) 2006 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007 Rob Buis <buis@kde.org>
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
#include "SVGDocumentExtensions.h"

#include "Console.h"
#include "DOMWindow.h"
#include "Document.h"
#include "EventListener.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "Page.h"
#include "SMILTimeContainer.h"
#include "SVGElement.h"
#include "SVGSMILElement.h"
#include "SVGSVGElement.h"
#include "ScriptableDocumentParser.h"
#include <wtf/text/AtomicString.h>

namespace WebCore {

SVGDocumentExtensions::SVGDocumentExtensions(Document* document)
    : m_document(document)
    , m_resourcesCache(adoptPtr(new SVGResourcesCache))
{
}

SVGDocumentExtensions::~SVGDocumentExtensions()
{
    deleteAllValues(m_animatedElements);
    deleteAllValues(m_pendingResources);
}

void SVGDocumentExtensions::addTimeContainer(SVGSVGElement* element)
{
    m_timeContainers.add(element);
}

void SVGDocumentExtensions::removeTimeContainer(SVGSVGElement* element)
{
    m_timeContainers.remove(element);
}

void SVGDocumentExtensions::addResource(const AtomicString& id, RenderSVGResourceContainer* resource)
{
    ASSERT(resource);

    if (id.isEmpty())
        return;

    // Replaces resource if already present, to handle potential id changes
    m_resources.set(id, resource);
}

void SVGDocumentExtensions::removeResource(const AtomicString& id)
{
    if (id.isEmpty() || !m_resources.contains(id))
        return;

    m_resources.remove(id);
}

RenderSVGResourceContainer* SVGDocumentExtensions::resourceById(const AtomicString& id) const
{
    if (id.isEmpty())
        return 0;

    return m_resources.get(id);
}

void SVGDocumentExtensions::startAnimations()
{
    // FIXME: Eventually every "Time Container" will need a way to latch on to some global timer
    // starting animations for a document will do this "latching"
#if ENABLE(SVG_ANIMATION)    
    // FIXME: We hold a ref pointers to prevent a shadow tree from getting removed out from underneath us.
    // In the future we should refactor the use-element to avoid this. See https://webkit.org/b/53704
    Vector<RefPtr<SVGSVGElement> > timeContainers;
    timeContainers.appendRange(m_timeContainers.begin(), m_timeContainers.end());
    Vector<RefPtr<SVGSVGElement> >::iterator end = timeContainers.end();
    for (Vector<RefPtr<SVGSVGElement> >::iterator itr = timeContainers.begin(); itr != end; ++itr)
        (*itr)->timeContainer()->begin();
#endif
}
    
void SVGDocumentExtensions::pauseAnimations()
{
    HashSet<SVGSVGElement*>::iterator end = m_timeContainers.end();
    for (HashSet<SVGSVGElement*>::iterator itr = m_timeContainers.begin(); itr != end; ++itr)
        (*itr)->pauseAnimations();
}

void SVGDocumentExtensions::unpauseAnimations()
{
    HashSet<SVGSVGElement*>::iterator end = m_timeContainers.end();
    for (HashSet<SVGSVGElement*>::iterator itr = m_timeContainers.begin(); itr != end; ++itr)
        (*itr)->unpauseAnimations();
}

bool SVGDocumentExtensions::sampleAnimationAtTime(const String& elementId, SVGSMILElement* element, double time)
{
#if !ENABLE(SVG_ANIMATION)
    UNUSED_PARAM(elementId);
    UNUSED_PARAM(element);
    UNUSED_PARAM(time);
    return false;
#else
    ASSERT(element);
    SMILTimeContainer* container = element->timeContainer();
    if (!container || container->isPaused())
        return false;

    container->sampleAnimationAtTime(elementId, time);
    return true;
#endif
}

void SVGDocumentExtensions::addAnimationElementToTarget(SVGSMILElement* animationElement, SVGElement* targetElement)
{
    ASSERT(targetElement);
    ASSERT(animationElement);

    if (HashSet<SVGSMILElement*>* animationElementsForTarget = m_animatedElements.get(targetElement)) {
        animationElementsForTarget->add(animationElement);
        return;
    }

    HashSet<SVGSMILElement*>* animationElementsForTarget = new HashSet<SVGSMILElement*>;
    animationElementsForTarget->add(animationElement);
    m_animatedElements.set(targetElement, animationElementsForTarget);
}

void SVGDocumentExtensions::removeAnimationElementFromTarget(SVGSMILElement* animationElement, SVGElement* targetElement)
{
    ASSERT(targetElement);
    ASSERT(animationElement);

    HashMap<SVGElement*, HashSet<SVGSMILElement*>* >::iterator it = m_animatedElements.find(targetElement);
    ASSERT(it != m_animatedElements.end());
    
    HashSet<SVGSMILElement*>* animationElementsForTarget = it->second;
    ASSERT(!animationElementsForTarget->isEmpty());

    animationElementsForTarget->remove(animationElement);
    if (animationElementsForTarget->isEmpty()) {
        m_animatedElements.remove(it);
        delete animationElementsForTarget;
    }
}

void SVGDocumentExtensions::removeAllAnimationElementsFromTarget(SVGElement* targetElement)
{
    ASSERT(targetElement);
    HashSet<SVGSMILElement*>* animationElementsForTarget = m_animatedElements.take(targetElement);
    if (!animationElementsForTarget)
        return;
#if ENABLE(SVG_ANIMATION)
    HashSet<SVGSMILElement*>::iterator it = animationElementsForTarget->begin();
    HashSet<SVGSMILElement*>::iterator end = animationElementsForTarget->end();
    for (; it != end; ++it)
        (*it)->resetTargetElement();
    delete animationElementsForTarget;
#else
    ASSERT_NOT_REACHED();
#endif
}

// FIXME: Callers should probably use ScriptController::eventHandlerLineNumber()
static int parserLineNumber(Document* document)
{
    ScriptableDocumentParser* parser = document->scriptableDocumentParser();
    if (!parser)
        return 1;
    return parser->lineNumber();
}

static void reportMessage(Document* document, MessageLevel level, const String& message)
{
    if (Frame* frame = document->frame())
        frame->domWindow()->console()->addMessage(JSMessageSource, LogMessageType, level, message, parserLineNumber(document), String());
}

void SVGDocumentExtensions::reportWarning(const String& message)
{
    reportMessage(m_document, WarningMessageLevel, "Warning: " + message);
}

void SVGDocumentExtensions::reportError(const String& message)
{
    reportMessage(m_document, ErrorMessageLevel, "Error: " + message);
}

void SVGDocumentExtensions::addPendingResource(const AtomicString& id, SVGStyledElement* element)
{
    ASSERT(element);

    if (id.isEmpty())
        return;

    if (m_pendingResources.contains(id))
        m_pendingResources.get(id)->add(element);
    else {
        SVGPendingElements* set = new SVGPendingElements;
        set->add(element);

        m_pendingResources.add(id, set);
    }

    element->setHasPendingResources();
}

bool SVGDocumentExtensions::hasPendingResources(const AtomicString& id) const
{
    if (id.isEmpty())
        return false;

    return m_pendingResources.contains(id);
}

bool SVGDocumentExtensions::isElementInPendingResources(SVGStyledElement* element) const
{
    ASSERT(element);

    if (m_pendingResources.isEmpty())
        return false;

    HashMap<AtomicString, SVGPendingElements*>::const_iterator end = m_pendingResources.end();
    for (HashMap<AtomicString, SVGPendingElements*>::const_iterator it = m_pendingResources.begin(); it != end; ++it) {
        SVGPendingElements* elements = it->second;
        ASSERT(elements);

        if (elements->contains(element))
            return true;
    }
    return false;
}

void SVGDocumentExtensions::removeElementFromPendingResources(SVGStyledElement* element)
{
    ASSERT(element);

    if (m_pendingResources.isEmpty() || !element->hasPendingResources())
        return;

    Vector<AtomicString> toBeRemoved;
    HashMap<AtomicString, SVGPendingElements*>::iterator end = m_pendingResources.end();
    for (HashMap<AtomicString, SVGPendingElements*>::iterator it = m_pendingResources.begin(); it != end; ++it) {
        SVGPendingElements* elements = it->second;
        ASSERT(elements);
        ASSERT(!elements->isEmpty());

        elements->remove(element);
        if (elements->isEmpty())
            toBeRemoved.append(it->first);
    }

    element->clearHasPendingResourcesIfPossible();

    if (toBeRemoved.isEmpty())
        return;

    Vector<AtomicString>::iterator endVector = toBeRemoved.end();
    for (Vector<AtomicString>::iterator it = toBeRemoved.begin(); it != endVector; ++it)
        m_pendingResources.remove(*it);
}

PassOwnPtr<SVGDocumentExtensions::SVGPendingElements> SVGDocumentExtensions::removePendingResource(const AtomicString& id)
{
    ASSERT(m_pendingResources.contains(id));

    OwnPtr<SVGPendingElements> set = adoptPtr(m_pendingResources.get(id));
    m_pendingResources.remove(id);
    return set.release();
}

}

#endif
