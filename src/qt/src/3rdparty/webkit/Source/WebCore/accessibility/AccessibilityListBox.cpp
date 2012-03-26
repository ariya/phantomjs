/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AccessibilityListBox.h"

#include "AXObjectCache.h"
#include "AccessibilityListBoxOption.h"
#include "HTMLNames.h"
#include "HTMLSelectElement.h"
#include "HitTestResult.h"
#include "RenderListBox.h"
#include "RenderObject.h"

using namespace std;

namespace WebCore {

using namespace HTMLNames;

AccessibilityListBox::AccessibilityListBox(RenderObject* renderer)
    : AccessibilityRenderObject(renderer)
{
}

AccessibilityListBox::~AccessibilityListBox()
{
}
    
PassRefPtr<AccessibilityListBox> AccessibilityListBox::create(RenderObject* renderer)
{
    return adoptRef(new AccessibilityListBox(renderer));
}
    
bool AccessibilityListBox::canSetSelectedChildrenAttribute() const
{
    Node* selectNode = m_renderer->node();
    if (!selectNode)
        return false;
    
    return !static_cast<HTMLSelectElement*>(selectNode)->disabled();
}

void AccessibilityListBox::addChildren()
{
    Node* selectNode = m_renderer->node();
    if (!selectNode)
        return;
    
    m_haveChildren = true;
    
    const Vector<Element*>& listItems = static_cast<HTMLSelectElement*>(selectNode)->listItems();
    unsigned length = listItems.size();
    for (unsigned i = 0; i < length; i++) {
        // The cast to HTMLElement below is safe because the only other possible listItem type
        // would be a WMLElement, but WML builds don't use accessibility features at all.
        AccessibilityObject* listOption = listBoxOptionAccessibilityObject(toHTMLElement(listItems[i]));
        if (listOption && !listOption->accessibilityIsIgnored())
            m_children.append(listOption);
    }
}

void AccessibilityListBox::setSelectedChildren(AccessibilityChildrenVector& children)
{
    if (!canSetSelectedChildrenAttribute())
        return;
    
    Node* selectNode = m_renderer->node();
    if (!selectNode)
        return;
    
    // disable any selected options
    unsigned length = m_children.size();
    for (unsigned i = 0; i < length; i++) {
        AccessibilityListBoxOption* listBoxOption = static_cast<AccessibilityListBoxOption*>(m_children[i].get());
        if (listBoxOption->isSelected())
            listBoxOption->setSelected(false);
    }
    
    length = children.size();
    for (unsigned i = 0; i < length; i++) {
        AccessibilityObject* obj = children[i].get();
        if (obj->roleValue() != ListBoxOptionRole)
            continue;
                
        static_cast<AccessibilityListBoxOption*>(obj)->setSelected(true);
    }
}
    
void AccessibilityListBox::selectedChildren(AccessibilityChildrenVector& result)
{
    ASSERT(result.isEmpty());

    if (!hasChildren())
        addChildren();
        
    unsigned length = m_children.size();
    for (unsigned i = 0; i < length; i++) {
        if (static_cast<AccessibilityListBoxOption*>(m_children[i].get())->isSelected())
            result.append(m_children[i]);
    }    
}

void AccessibilityListBox::visibleChildren(AccessibilityChildrenVector& result)
{
    ASSERT(result.isEmpty());
    
    if (!hasChildren())
        addChildren();
    
    unsigned length = m_children.size();
    for (unsigned i = 0; i < length; i++) {
        if (toRenderListBox(m_renderer)->listIndexIsVisible(i))
            result.append(m_children[i]);
    }
}

AccessibilityObject* AccessibilityListBox::listBoxOptionAccessibilityObject(HTMLElement* element) const
{
    // skip hr elements
    if (!element || element->hasTagName(hrTag))
        return 0;
    
    AccessibilityObject* listBoxObject = m_renderer->document()->axObjectCache()->getOrCreate(ListBoxOptionRole);
    static_cast<AccessibilityListBoxOption*>(listBoxObject)->setHTMLElement(element);
    
    return listBoxObject;
}
    
bool AccessibilityListBox::accessibilityIsIgnored() const
{
    AccessibilityObjectInclusion decision = accessibilityIsIgnoredBase();
    if (decision == IncludeObject)
        return false;
    if (decision == IgnoreObject)
        return true;
    
    return false;
}

AccessibilityObject* AccessibilityListBox::elementAccessibilityHitTest(const IntPoint& point) const
{
    // the internal HTMLSelectElement methods for returning a listbox option at a point
    // ignore optgroup elements.
    if (!m_renderer)
        return 0;
    
    Node* node = m_renderer->node();
    if (!node)
        return 0;
    
    IntRect parentRect = boundingBoxRect();
    
    AccessibilityObject* listBoxOption = 0;
    unsigned length = m_children.size();
    for (unsigned i = 0; i < length; i++) {
        IntRect rect = toRenderListBox(m_renderer)->itemBoundingBoxRect(parentRect.x(), parentRect.y(), i);
        // The cast to HTMLElement below is safe because the only other possible listItem type
        // would be a WMLElement, but WML builds don't use accessibility features at all.
        if (rect.contains(point)) {
            listBoxOption = m_children[i].get();
            break;
        }
    }
    
    if (listBoxOption && !listBoxOption->accessibilityIsIgnored())
        return listBoxOption;
    
    return axObjectCache()->getOrCreate(m_renderer);
}

} // namespace WebCore
