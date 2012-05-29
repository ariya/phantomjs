/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "HTMLElementStack.h"

#include "DocumentFragment.h"
#include "Element.h"
#include "HTMLNames.h"
#include "MathMLNames.h"
#include "SVGNames.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

using namespace HTMLNames;

namespace {

inline bool isNumberedHeaderElement(ContainerNode* node)
{
    return node->hasTagName(h1Tag)
        || node->hasTagName(h2Tag)
        || node->hasTagName(h3Tag)
        || node->hasTagName(h4Tag)
        || node->hasTagName(h5Tag)
        || node->hasTagName(h6Tag);
}
    
inline bool isRootNode(ContainerNode* node)
{
    return node->nodeType() == Node::DOCUMENT_FRAGMENT_NODE
        || node->nodeType() == Node::SHADOW_ROOT_NODE
        || node->hasTagName(htmlTag);
}

inline bool isScopeMarker(ContainerNode* node)
{
    return node->hasTagName(appletTag)
        || node->hasTagName(captionTag)
        || node->hasTagName(marqueeTag)
        || node->hasTagName(objectTag)
        || node->hasTagName(tableTag)
        || node->hasTagName(tdTag)
        || node->hasTagName(thTag)
        || node->hasTagName(MathMLNames::miTag)
        || node->hasTagName(MathMLNames::moTag)
        || node->hasTagName(MathMLNames::mnTag)
        || node->hasTagName(MathMLNames::msTag)
        || node->hasTagName(MathMLNames::mtextTag)
        || node->hasTagName(MathMLNames::annotation_xmlTag)
        || node->hasTagName(SVGNames::foreignObjectTag)
        || node->hasTagName(SVGNames::descTag)
        || node->hasTagName(SVGNames::titleTag)
        || isRootNode(node);
}

inline bool isListItemScopeMarker(ContainerNode* node)
{
    return isScopeMarker(node)
        || node->hasTagName(olTag)
        || node->hasTagName(ulTag);
}

inline bool isTableScopeMarker(ContainerNode* node)
{
    return node->hasTagName(tableTag)
        || isRootNode(node);
}

inline bool isTableBodyScopeMarker(ContainerNode* node)
{
    return node->hasTagName(tbodyTag)
        || node->hasTagName(tfootTag)
        || node->hasTagName(theadTag)
        || isRootNode(node);
}

inline bool isTableRowScopeMarker(ContainerNode* node)
{
    return node->hasTagName(trTag)
        || isRootNode(node);
}

inline bool isForeignContentScopeMarker(ContainerNode* node)
{
    return node->hasTagName(MathMLNames::miTag)
        || node->hasTagName(MathMLNames::moTag)
        || node->hasTagName(MathMLNames::mnTag)
        || node->hasTagName(MathMLNames::msTag)
        || node->hasTagName(MathMLNames::mtextTag)
        || node->hasTagName(SVGNames::foreignObjectTag)
        || node->hasTagName(SVGNames::descTag)
        || node->hasTagName(SVGNames::titleTag)
        || isInHTMLNamespace(node);
}

inline bool isButtonScopeMarker(ContainerNode* node)
{
    return isScopeMarker(node)
        || node->hasTagName(buttonTag);
}

inline bool isSelectScopeMarker(ContainerNode* node)
{
    return !node->hasTagName(optgroupTag)
        && !node->hasTagName(optionTag);
}

}

HTMLElementStack::ElementRecord::ElementRecord(PassRefPtr<ContainerNode> node, PassOwnPtr<ElementRecord> next)
    : m_node(node)
    , m_next(next)
{
    ASSERT(m_node);
}

HTMLElementStack::ElementRecord::~ElementRecord()
{
}

void HTMLElementStack::ElementRecord::replaceElement(PassRefPtr<Element> element)
{
    ASSERT(element);
    ASSERT(!m_node || m_node->isElementNode());
    // FIXME: Should this call finishParsingChildren?
    m_node = element;
}

bool HTMLElementStack::ElementRecord::isAbove(ElementRecord* other) const
{
    for (ElementRecord* below = next(); below; below = below->next()) {
        if (below == other)
            return true;
    }
    return false;
}

HTMLElementStack::HTMLElementStack()
    : m_rootNode(0)
    , m_headElement(0)
    , m_bodyElement(0)
{
}

HTMLElementStack::~HTMLElementStack()
{
}

bool HTMLElementStack::hasOnlyOneElement() const
{
    return !topRecord()->next();
}

bool HTMLElementStack::secondElementIsHTMLBodyElement() const
{
    // This is used the fragment case of <body> and <frameset> in the "in body"
    // insertion mode.
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#parsing-main-inbody
    ASSERT(m_rootNode);
    // If we have a body element, it must always be the second element on the
    // stack, as we always start with an html element, and any other element
    // would cause the implicit creation of a body element.
    return !!m_bodyElement;
}

void HTMLElementStack::popHTMLHeadElement()
{
    ASSERT(top() == m_headElement);
    m_headElement = 0;
    popCommon();
}

void HTMLElementStack::popHTMLBodyElement()
{
    ASSERT(top() == m_bodyElement);
    m_bodyElement = 0;
    popCommon();
}

void HTMLElementStack::popAll()
{
    m_rootNode = 0;
    m_headElement = 0;
    m_bodyElement = 0;
    while (m_top) {
        topNode()->finishParsingChildren();
        m_top = m_top->releaseNext();
    }
}

void HTMLElementStack::pop()
{
    ASSERT(!top()->hasTagName(HTMLNames::headTag));
    popCommon();
}

void HTMLElementStack::popUntil(const AtomicString& tagName)
{
    while (!top()->hasLocalName(tagName)) {
        // pop() will ASSERT at <body> if callers fail to check that there is an
        // element with localName |tagName| on the stack of open elements.
        pop();
    }
}

void HTMLElementStack::popUntilPopped(const AtomicString& tagName)
{
    popUntil(tagName);
    pop();
}

void HTMLElementStack::popUntilNumberedHeaderElementPopped()
{
    while (!isNumberedHeaderElement(topNode()))
        pop();
    pop();
}

void HTMLElementStack::popUntil(Element* element)
{
    while (top() != element)
        pop();
}

void HTMLElementStack::popUntilPopped(Element* element)
{
    popUntil(element);
    pop();
}

void HTMLElementStack::popUntilTableScopeMarker()
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#clear-the-stack-back-to-a-table-context
    while (!isTableScopeMarker(topNode()))
        pop();
}

void HTMLElementStack::popUntilTableBodyScopeMarker()
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#clear-the-stack-back-to-a-table-body-context
    while (!isTableBodyScopeMarker(topNode()))
        pop();
}

void HTMLElementStack::popUntilTableRowScopeMarker()
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#clear-the-stack-back-to-a-table-row-context
    while (!isTableRowScopeMarker(topNode()))
        pop();
}

void HTMLElementStack::popUntilForeignContentScopeMarker()
{
    while (!isForeignContentScopeMarker(topNode()))
        pop();
}
    
void HTMLElementStack::pushRootNode(PassRefPtr<ContainerNode> rootNode)
{
    ASSERT(rootNode->nodeType() == Node::DOCUMENT_FRAGMENT_NODE || rootNode->nodeType() == Node::SHADOW_ROOT_NODE);
    pushRootNodeCommon(rootNode);
}

void HTMLElementStack::pushHTMLHtmlElement(PassRefPtr<Element> element)
{
    ASSERT(element->hasTagName(HTMLNames::htmlTag));
    pushRootNodeCommon(element);
}
    
void HTMLElementStack::pushRootNodeCommon(PassRefPtr<ContainerNode> rootNode)
{
    ASSERT(!m_top);
    ASSERT(!m_rootNode);
    m_rootNode = rootNode.get();
    pushCommon(rootNode);
}

void HTMLElementStack::pushHTMLHeadElement(PassRefPtr<Element> element)
{
    ASSERT(element->hasTagName(HTMLNames::headTag));
    ASSERT(!m_headElement);
    m_headElement = element.get();
    pushCommon(element);
}

void HTMLElementStack::pushHTMLBodyElement(PassRefPtr<Element> element)
{
    ASSERT(element->hasTagName(HTMLNames::bodyTag));
    ASSERT(!m_bodyElement);
    m_bodyElement = element.get();
    pushCommon(element);
}

void HTMLElementStack::push(PassRefPtr<Element> element)
{
    ASSERT(!element->hasTagName(HTMLNames::htmlTag));
    ASSERT(!element->hasTagName(HTMLNames::headTag));
    ASSERT(!element->hasTagName(HTMLNames::bodyTag));
    ASSERT(m_rootNode);
    pushCommon(element);
}

void HTMLElementStack::insertAbove(PassRefPtr<Element> element, ElementRecord* recordBelow)
{
    ASSERT(element);
    ASSERT(recordBelow);
    ASSERT(m_top);
    ASSERT(!element->hasTagName(HTMLNames::htmlTag));
    ASSERT(!element->hasTagName(HTMLNames::headTag));
    ASSERT(!element->hasTagName(HTMLNames::bodyTag));
    ASSERT(m_rootNode);
    if (recordBelow == m_top) {
        push(element);
        return;
    }

    for (ElementRecord* recordAbove = m_top.get(); recordAbove; recordAbove = recordAbove->next()) {
        if (recordAbove->next() != recordBelow)
            continue;

        recordAbove->setNext(adoptPtr(new ElementRecord(element, recordAbove->releaseNext())));
        recordAbove->next()->element()->beginParsingChildren();
        return;
    }
    ASSERT_NOT_REACHED();
}

HTMLElementStack::ElementRecord* HTMLElementStack::topRecord() const
{
    ASSERT(m_top);
    return m_top.get();
}

Element* HTMLElementStack::oneBelowTop() const
{
    // We should never be calling this if it could be 0.
    ASSERT(m_top);
    ASSERT(m_top->next());
    return m_top->next()->element();
}

Element* HTMLElementStack::bottom() const
{
    return htmlElement();
}

void HTMLElementStack::removeHTMLHeadElement(Element* element)
{
    ASSERT(m_headElement == element);
    if (m_top->element() == element) {
        popHTMLHeadElement();
        return;
    }
    m_headElement = 0;
    removeNonTopCommon(element);
}

void HTMLElementStack::remove(Element* element)
{
    ASSERT(!element->hasTagName(HTMLNames::headTag));
    if (m_top->element() == element) {
        pop();
        return;
    }
    removeNonTopCommon(element);
}

HTMLElementStack::ElementRecord* HTMLElementStack::find(Element* element) const
{
    for (ElementRecord* pos = m_top.get(); pos; pos = pos->next()) {
        if (pos->node() == element)
            return pos;
    }
    return 0;
}

HTMLElementStack::ElementRecord* HTMLElementStack::topmost(const AtomicString& tagName) const
{
    for (ElementRecord* pos = m_top.get(); pos; pos = pos->next()) {
        if (pos->node()->hasLocalName(tagName))
            return pos;
    }
    return 0;
}

bool HTMLElementStack::contains(Element* element) const
{
    return !!find(element);
}

bool HTMLElementStack::contains(const AtomicString& tagName) const
{
    return !!topmost(tagName);
}

template <bool isMarker(ContainerNode*)>
bool inScopeCommon(HTMLElementStack::ElementRecord* top, const AtomicString& targetTag)
{
    for (HTMLElementStack::ElementRecord* pos = top; pos; pos = pos->next()) {
        ContainerNode* node = pos->node();
        if (node->hasLocalName(targetTag))
            return true;
        if (isMarker(node))
            return false;
    }
    ASSERT_NOT_REACHED(); // <html> is always on the stack and is a scope marker.
    return false;
}

bool HTMLElementStack::hasOnlyHTMLElementsInScope() const
{
    for (ElementRecord* record = m_top.get(); record; record = record->next()) {
        ContainerNode* node = record->node();
        if (!isInHTMLNamespace(node))
            return false;
        if (isScopeMarker(node))
            return true;
    }
    ASSERT_NOT_REACHED(); // <html> is always on the stack and is a scope marker.
    return true;
}

bool HTMLElementStack::hasNumberedHeaderElementInScope() const
{
    for (ElementRecord* record = m_top.get(); record; record = record->next()) {
        ContainerNode* node = record->node();
        if (isNumberedHeaderElement(node))
            return true;
        if (isScopeMarker(node))
            return false;
    }
    ASSERT_NOT_REACHED(); // <html> is always on the stack and is a scope marker.
    return false;
}

bool HTMLElementStack::inScope(Element* targetElement) const
{
    for (ElementRecord* pos = m_top.get(); pos; pos = pos->next()) {
        ContainerNode* node = pos->node();
        if (node == targetElement)
            return true;
        if (isScopeMarker(node))
            return false;
    }
    ASSERT_NOT_REACHED(); // <html> is always on the stack and is a scope marker.
    return false;
}

bool HTMLElementStack::inScope(const AtomicString& targetTag) const
{
    return inScopeCommon<isScopeMarker>(m_top.get(), targetTag);
}

bool HTMLElementStack::inScope(const QualifiedName& tagName) const
{
    // FIXME: Is localName() right for non-html elements?
    return inScope(tagName.localName());
}

bool HTMLElementStack::inListItemScope(const AtomicString& targetTag) const
{
    return inScopeCommon<isListItemScopeMarker>(m_top.get(), targetTag);
}

bool HTMLElementStack::inListItemScope(const QualifiedName& tagName) const
{
    // FIXME: Is localName() right for non-html elements?
    return inListItemScope(tagName.localName());
}

bool HTMLElementStack::inTableScope(const AtomicString& targetTag) const
{
    return inScopeCommon<isTableScopeMarker>(m_top.get(), targetTag);
}

bool HTMLElementStack::inTableScope(const QualifiedName& tagName) const
{
    // FIXME: Is localName() right for non-html elements?
    return inTableScope(tagName.localName());
}

bool HTMLElementStack::inButtonScope(const AtomicString& targetTag) const
{
    return inScopeCommon<isButtonScopeMarker>(m_top.get(), targetTag);
}

bool HTMLElementStack::inButtonScope(const QualifiedName& tagName) const
{
    // FIXME: Is localName() right for non-html elements?
    return inButtonScope(tagName.localName());
}

bool HTMLElementStack::inSelectScope(const AtomicString& targetTag) const
{
    return inScopeCommon<isSelectScopeMarker>(m_top.get(), targetTag);
}

bool HTMLElementStack::inSelectScope(const QualifiedName& tagName) const
{
    // FIXME: Is localName() right for non-html elements?
    return inSelectScope(tagName.localName());
}

Element* HTMLElementStack::htmlElement() const
{
    ASSERT(m_rootNode);
    return toElement(m_rootNode);
}

Element* HTMLElementStack::headElement() const
{
    ASSERT(m_headElement);
    return m_headElement;
}

Element* HTMLElementStack::bodyElement() const
{
    ASSERT(m_bodyElement);
    return m_bodyElement;
}
    
ContainerNode* HTMLElementStack::rootNode() const
{
    ASSERT(m_rootNode);
    return m_rootNode;
}

void HTMLElementStack::pushCommon(PassRefPtr<ContainerNode> node)
{
    ASSERT(m_rootNode);
    m_top = adoptPtr(new ElementRecord(node, m_top.release()));
    topNode()->beginParsingChildren();
}

void HTMLElementStack::popCommon()
{
    ASSERT(!top()->hasTagName(HTMLNames::htmlTag));
    ASSERT(!top()->hasTagName(HTMLNames::headTag) || !m_headElement);
    ASSERT(!top()->hasTagName(HTMLNames::bodyTag) || !m_bodyElement);
    top()->finishParsingChildren();
    m_top = m_top->releaseNext();
}

void HTMLElementStack::removeNonTopCommon(Element* element)
{
    ASSERT(!element->hasTagName(HTMLNames::htmlTag));
    ASSERT(!element->hasTagName(HTMLNames::bodyTag));
    ASSERT(top() != element);
    for (ElementRecord* pos = m_top.get(); pos; pos = pos->next()) {
        if (pos->next()->element() == element) {
            // FIXME: Is it OK to call finishParsingChildren()
            // when the children aren't actually finished?
            element->finishParsingChildren();
            pos->setNext(pos->next()->releaseNext());
            return;
        }
    }
    ASSERT_NOT_REACHED();
}

#ifndef NDEBUG

void HTMLElementStack::show()
{
    for (ElementRecord* record = m_top.get(); record; record = record->next())
        record->element()->showNode();
}

#endif

}
