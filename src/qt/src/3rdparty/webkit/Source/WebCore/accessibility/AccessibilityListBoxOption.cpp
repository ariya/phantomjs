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
#include "AccessibilityListBoxOption.h"

#include "AXObjectCache.h"
#include "AccessibilityListBox.h"
#include "Element.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "HTMLOptGroupElement.h"
#include "HTMLOptionElement.h"
#include "HTMLSelectElement.h"
#include "IntRect.h"
#include "RenderListBox.h"
#include "RenderObject.h"

using namespace std;

namespace WebCore {

using namespace HTMLNames;
    
AccessibilityListBoxOption::AccessibilityListBoxOption()
    : m_optionElement(0)
{
}

AccessibilityListBoxOption::~AccessibilityListBoxOption()
{
}    
    
PassRefPtr<AccessibilityListBoxOption> AccessibilityListBoxOption::create()
{
    return adoptRef(new AccessibilityListBoxOption());
}
    
bool AccessibilityListBoxOption::isEnabled() const
{
    if (!m_optionElement)
        return false;
    
    if (m_optionElement->hasTagName(optgroupTag))
        return false;
    
    return true;
}
    
bool AccessibilityListBoxOption::isSelected() const
{
    if (!m_optionElement)
        return false;

    if (!m_optionElement->hasTagName(optionTag))
        return false;
    
    return static_cast<HTMLOptionElement*>(m_optionElement)->selected();
}

IntRect AccessibilityListBoxOption::elementRect() const
{
    IntRect rect;
    if (!m_optionElement)
        return rect;
    
    HTMLSelectElement* listBoxParentNode = listBoxOptionParentNode();
    if (!listBoxParentNode)
        return rect;
    
    RenderObject* listBoxRenderer = listBoxParentNode->renderer();
    if (!listBoxRenderer)
        return rect;
    
    IntRect parentRect = listBoxRenderer->document()->axObjectCache()->getOrCreate(listBoxRenderer)->boundingBoxRect();
    int index = listBoxOptionIndex();
    if (index != -1)
        rect = toRenderListBox(listBoxRenderer)->itemBoundingBoxRect(parentRect.x(), parentRect.y(), index);
    
    return rect;
}

bool AccessibilityListBoxOption::accessibilityIsIgnored() const
{
    if (!m_optionElement)
        return true;
    
    if (equalIgnoringCase(getAttribute(aria_hiddenAttr), "true"))
        return true;
    
    return parentObject()->accessibilityIsIgnored();
}
    
bool AccessibilityListBoxOption::canSetSelectedAttribute() const
{
    if (!m_optionElement)
        return false;
    
    if (!m_optionElement->hasTagName(optionTag))
        return false;
    
    if (m_optionElement->disabled())
        return false;
    
    HTMLSelectElement* selectElement = listBoxOptionParentNode();
    if (selectElement && selectElement->disabled())
        return false;
    
    return true;
}
    
String AccessibilityListBoxOption::stringValue() const
{
    if (!m_optionElement)
        return String();
    
    const AtomicString& ariaLabel = getAttribute(aria_labelAttr);
    if (!ariaLabel.isNull())
        return ariaLabel;
    
    if (m_optionElement->hasTagName(optionTag))
        return static_cast<HTMLOptionElement*>(m_optionElement)->text();
    
    if (m_optionElement->hasTagName(optgroupTag))
        return static_cast<HTMLOptGroupElement*>(m_optionElement)->groupLabelText();
    
    return String();
}

Element* AccessibilityListBoxOption::actionElement() const
{
    return m_optionElement;
}

AccessibilityObject* AccessibilityListBoxOption::parentObject() const
{
    HTMLSelectElement* parentNode = listBoxOptionParentNode();
    if (!parentNode)
        return 0;
    
    return m_optionElement->document()->axObjectCache()->getOrCreate(parentNode->renderer());
}

void AccessibilityListBoxOption::setSelected(bool selected)
{
    HTMLSelectElement* selectElement = listBoxOptionParentNode();
    if (!selectElement)
        return;
    
    if (!canSetSelectedAttribute())
        return;
    
    bool isOptionSelected = isSelected();
    if ((isOptionSelected && selected) || (!isOptionSelected && !selected))
        return;
    
    // Convert from the entire list index to the option index.
    int optionIndex = static_cast<SelectElement*>(selectElement)->listToOptionIndex(listBoxOptionIndex());
    selectElement->accessKeySetSelectedIndex(optionIndex);
}

HTMLSelectElement* AccessibilityListBoxOption::listBoxOptionParentNode() const
{
    if (!m_optionElement)
        return 0;
    
    if (m_optionElement->hasTagName(optionTag))
        return static_cast<HTMLOptionElement*>(m_optionElement)->ownerSelectElement();
    
    if (m_optionElement->hasTagName(optgroupTag))
        return static_cast<HTMLOptGroupElement*>(m_optionElement)->ownerSelectElement();
    
    return 0;
}

int AccessibilityListBoxOption::listBoxOptionIndex() const
{
    if (!m_optionElement)
        return -1;
    
    HTMLSelectElement* selectElement = listBoxOptionParentNode();
    if (!selectElement) 
        return -1;
    
    const Vector<Element*>& listItems = selectElement->listItems();
    unsigned length = listItems.size();
    for (unsigned i = 0; i < length; i++)
        if (listItems[i] == m_optionElement)
            return i;

    return -1;
}

} // namespace WebCore
