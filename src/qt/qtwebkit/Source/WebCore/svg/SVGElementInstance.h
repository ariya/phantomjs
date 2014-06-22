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
#include "TreeShared.h"

namespace WebCore {

namespace Private {
template<class GenericNode, class GenericNodeContainer>
void addChildNodesToDeletionQueue(GenericNode*& head, GenericNode*& tail, GenericNodeContainer* container);
};

class Document;
class SVGElement;
class SVGElementInstanceList;
class SVGStyledElement;
class SVGUseElement;

// SVGElementInstance mimics Node, but without providing all its functionality
class SVGElementInstance : public EventTarget, public TreeShared<SVGElementInstance> {
public:
    static PassRefPtr<SVGElementInstance> create(SVGUseElement* correspondingUseElement, SVGUseElement* directUseElement, PassRefPtr<SVGElement> originalElement);

    virtual ~SVGElementInstance();

    void setParentOrShadowHostNode(SVGElementInstance* instance) { m_parentInstance = instance; }

    virtual const AtomicString& interfaceName() const;
    virtual ScriptExecutionContext* scriptExecutionContext() const;

    virtual bool addEventListener(const AtomicString& eventType, PassRefPtr<EventListener>, bool useCapture);
    virtual bool removeEventListener(const AtomicString& eventType, EventListener*, bool useCapture);
    virtual void removeAllEventListeners();

    using EventTarget::dispatchEvent;
    virtual bool dispatchEvent(PassRefPtr<Event>) OVERRIDE;

    SVGElement* correspondingElement() const { return m_element.get(); }
    SVGUseElement* correspondingUseElement() const { return m_correspondingUseElement; }
    SVGUseElement* directUseElement() const { return m_directUseElement; }
    SVGElement* shadowTreeElement() const { return m_shadowTreeElement.get(); }

    void detach();

    SVGElementInstance* parentNode() const { return m_parentInstance; }
    PassRefPtr<SVGElementInstanceList> childNodes();

    SVGElementInstance* previousSibling() const { return m_previousSibling; }
    SVGElementInstance* nextSibling() const { return m_nextSibling; }

    SVGElementInstance* firstChild() const { return m_firstChild; }
    SVGElementInstance* lastChild() const { return m_lastChild; }

    Document* ownerDocument() const;

    class InvalidationGuard {
        WTF_MAKE_NONCOPYABLE(InvalidationGuard);
    public:    
        InvalidationGuard(SVGElement* element) : m_element(element) { }
        ~InvalidationGuard() { SVGElementInstance::invalidateAllInstancesOfElement(m_element); }
    private:
        SVGElement* m_element;
    };

    class InstanceUpdateBlocker {
        WTF_MAKE_NONCOPYABLE(InstanceUpdateBlocker);
    public:
        InstanceUpdateBlocker(SVGElement* targetElement);
        ~InstanceUpdateBlocker();

    private:
        SVGStyledElement* m_targetElement;
    };
    
    static void invalidateAllInstancesOfElement(SVGElement*);

    using TreeShared<SVGElementInstance>::ref;
    using TreeShared<SVGElementInstance>::deref;

    // EventTarget API
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), abort);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), blur);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), change);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), click);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), contextmenu);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), dblclick);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), error);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), focus);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), input);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), keydown);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), keypress);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), keyup);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), load);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), mousedown);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), mouseenter);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), mouseleave);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), mousemove);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), mouseout);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), mouseover);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), mouseup);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), mousewheel);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), beforecut);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), cut);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), beforecopy);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), copy);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), beforepaste);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), paste);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), dragenter);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), dragover);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), dragleave);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), drop);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), dragstart);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), drag);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), dragend);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), reset);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), resize);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), scroll);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), search);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), select);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), selectstart);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), submit);
    DECLARE_FORWARDING_ATTRIBUTE_EVENT_LISTENER(correspondingElement(), unload);

private:
    friend class SVGUseElement;
    friend class TreeShared<SVGElementInstance>;

    SVGElementInstance(SVGUseElement*, SVGUseElement*, PassRefPtr<SVGElement> originalElement);

    void removedLastRef();
    bool hasTreeSharedParent() const { return !!m_parentInstance; }

    virtual Node* toNode();

    void appendChild(PassRefPtr<SVGElementInstance> child);
    void setShadowTreeElement(SVGElement*);

    template<class GenericNode, class GenericNodeContainer>
    friend void appendChildToContainer(GenericNode* child, GenericNodeContainer* container);

    template<class GenericNode, class GenericNodeContainer>
    friend void removeDetachedChildrenInContainer(GenericNodeContainer*);

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

    SVGElementInstance* m_parentInstance;

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
