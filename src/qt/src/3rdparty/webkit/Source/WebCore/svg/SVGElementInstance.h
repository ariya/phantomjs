/*
 * Copyright (C) 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
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

#ifndef SVGElementInstance_h
#define SVGElementInstance_h

#if ENABLE(SVG)
#include "EventTarget.h"
#include "SVGElement.h"
#include "TreeShared.h"

namespace WebCore {

namespace Private {
template<class GenericNode, class GenericNodeContainer>
void addChildNodesToDeletionQueue(GenericNode*& head, GenericNode*& tail, GenericNodeContainer* container);
};

class SVGUseElement;
class SVGElementInstanceList;

// SVGElementInstance mimics Node, but without providing all its functionality
class SVGElementInstance : public TreeShared<SVGElementInstance>,
                           public EventTarget {
public:
    static PassRefPtr<SVGElementInstance> create(SVGUseElement* correspondingUseElement, SVGUseElement* directUseElement, PassRefPtr<SVGElement> originalElement)
    {
        return adoptRef(new SVGElementInstance(correspondingUseElement, directUseElement, originalElement));
    }

    virtual ~SVGElementInstance();

    virtual ScriptExecutionContext* scriptExecutionContext() const;

    virtual bool addEventListener(const AtomicString& eventType, PassRefPtr<EventListener>, bool useCapture);
    virtual bool removeEventListener(const AtomicString& eventType, EventListener*, bool useCapture);
    virtual void removeAllEventListeners();
    using EventTarget::dispatchEvent;
    virtual bool dispatchEvent(PassRefPtr<Event>);

    SVGElement* correspondingElement() const { return m_element.get(); }
    SVGUseElement* correspondingUseElement() const { return m_correspondingUseElement; }
    SVGUseElement* directUseElement() const { return m_directUseElement; }
    SVGElement* shadowTreeElement() const { return m_shadowTreeElement.get(); }
    void clearChildren();
    void clearUseElements()
    {
        m_directUseElement = 0;
        m_correspondingUseElement = 0;
    }

    SVGElementInstance* parentNode() const { return parent(); }
    PassRefPtr<SVGElementInstanceList> childNodes();

    SVGElementInstance* previousSibling() const { return m_previousSibling; }
    SVGElementInstance* nextSibling() const { return m_nextSibling; }

    SVGElementInstance* firstChild() const { return m_firstChild; }
    SVGElementInstance* lastChild() const { return m_lastChild; }

    Document* ownerDocument() const { return m_element ? m_element->ownerDocument() : 0; }

    static void invalidateAllInstancesOfElement(SVGElement*);

    using TreeShared<SVGElementInstance>::ref;
    using TreeShared<SVGElementInstance>::deref;

    // EventTarget API
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), abort);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), blur);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), change);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), click);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), contextmenu);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), dblclick);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), error);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), focus);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), input);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), keydown);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), keypress);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), keyup);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), load);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), mousedown);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), mousemove);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), mouseout);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), mouseover);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), mouseup);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), mousewheel);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), beforecut);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), cut);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), beforecopy);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), copy);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), beforepaste);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), paste);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), dragenter);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), dragover);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), dragleave);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), drop);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), dragstart);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), drag);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), dragend);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), reset);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), resize);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), scroll);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), search);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), select);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), selectstart);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), submit);
    DEFINE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), unload);

private:
    friend class SVGUseElement;

    SVGElementInstance(SVGUseElement*, SVGUseElement*, PassRefPtr<SVGElement> originalElement);

    virtual Node* toNode() { return shadowTreeElement(); }
    virtual SVGElementInstance* toSVGElementInstance() { return this; }

    void appendChild(PassRefPtr<SVGElementInstance> child);
    void setShadowTreeElement(SVGElement*);

    template<class GenericNode, class GenericNodeContainer>
    friend void appendChildToContainer(GenericNode* child, GenericNodeContainer* container);

    template<class GenericNode, class GenericNodeContainer>
    friend void removeAllChildrenInContainer(GenericNodeContainer* container);

    template<class GenericNode, class GenericNodeContainer>
    friend void Private::addChildNodesToDeletionQueue(GenericNode*& head, GenericNode*& tail, GenericNodeContainer* container);

    bool hasChildNodes() const { return m_firstChild; }

    void setFirstChild(SVGElementInstance* child) { m_firstChild = child; }
    void setLastChild(SVGElementInstance* child) { m_lastChild = child; }

    void setNextSibling(SVGElementInstance* sibling) { m_nextSibling = sibling; }
    void setPreviousSibling(SVGElementInstance* sibling) { m_previousSibling = sibling; }    

    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }
    virtual EventTargetData* eventTargetData();
    virtual EventTargetData* ensureEventTargetData();

    SVGUseElement* m_correspondingUseElement;
    SVGUseElement* m_directUseElement;
    RefPtr<SVGElement> m_element;
    RefPtr<SVGElement> m_shadowTreeElement;

    SVGElementInstance* m_previousSibling;
    SVGElementInstance* m_nextSibling;

    SVGElementInstance* m_firstChild;
    SVGElementInstance* m_lastChild;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
