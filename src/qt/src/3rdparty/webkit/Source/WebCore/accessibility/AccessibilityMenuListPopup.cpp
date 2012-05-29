/*
 * Copyright (C) 2010 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "AccessibilityMenuListPopup.h"

#include "AXObjectCache.h"
#include "AccessibilityMenuList.h"
#include "AccessibilityMenuListOption.h"
#include "HTMLNames.h"
#include "HTMLSelectElement.h"
#include "RenderMenuList.h"

namespace WebCore {

using namespace HTMLNames;

AccessibilityMenuListPopup::AccessibilityMenuListPopup()
    : m_menuList(0)
{
}

bool AccessibilityMenuListPopup::isVisible() const
{
    return false;
}

bool AccessibilityMenuListPopup::isOffScreen() const
{
    return m_menuList->isCollapsed();
}

AccessibilityObject* AccessibilityMenuListPopup::parentObject() const
{
    return m_menuList;
}

bool AccessibilityMenuListPopup::isEnabled() const
{
    return m_menuList->isEnabled();
}

AccessibilityMenuListOption* AccessibilityMenuListPopup::menuListOptionAccessibilityObject(HTMLElement* element) const
{
    if (!element || !element->hasTagName(optionTag))
        return 0;

    AccessibilityObject* object = m_menuList->renderer()->document()->axObjectCache()->getOrCreate(MenuListOptionRole);
    ASSERT(object->isMenuListOption());

    AccessibilityMenuListOption* option = static_cast<AccessibilityMenuListOption*>(object);
    option->setElement(element);

    return option;
}

bool AccessibilityMenuListPopup::press() const
{
    m_menuList->press();
    return true;
}

void AccessibilityMenuListPopup::addChildren()
{
    Node* selectNode = m_menuList->renderer()->node();
    if (!selectNode)
        return;

    m_haveChildren = true;

    ASSERT(selectNode->hasTagName(selectTag));

    const Vector<Element*>& listItems = static_cast<HTMLSelectElement*>(selectNode)->listItems();
    unsigned length = listItems.size();
    for (unsigned i = 0; i < length; i++) {
        // The cast to HTMLElement below is safe because the only other possible listItem type
        // would be a WMLElement, but WML builds don't use accessbility features at all.
        AccessibilityMenuListOption* option = menuListOptionAccessibilityObject(toHTMLElement(listItems[i]));
        if (option) {
            option->setParent(this);
            m_children.append(option);
        }
    }
}

void AccessibilityMenuListPopup::childrenChanged()
{
    for (size_t i = m_children.size(); i > 0 ; --i) {
        AccessibilityObject* child = m_children[i - 1].get();
        if (child->actionElement() && !child->actionElement()->attached()) {
            m_menuList->renderer()->document()->axObjectCache()->remove(child->axObjectID());
            m_children.remove(i - 1);
        }
    }
}

void AccessibilityMenuListPopup::setMenuList(AccessibilityMenuList* menuList)
{
    ASSERT_ARG(menuList, menuList);
    ASSERT(!m_menuList);
    m_menuList = menuList;
}

} // namespace WebCore
