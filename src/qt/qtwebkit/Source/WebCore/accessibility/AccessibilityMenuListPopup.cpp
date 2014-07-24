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
#include "HTMLOptionElement.h"
#include "HTMLSelectElement.h"
#include "RenderMenuList.h"

namespace WebCore {

using namespace HTMLNames;

AccessibilityMenuListPopup::AccessibilityMenuListPopup()
{
}

bool AccessibilityMenuListPopup::isVisible() const
{
    return false;
}

bool AccessibilityMenuListPopup::isOffScreen() const
{
    if (!m_parent)
        return true;
    
    return m_parent->isCollapsed();
}

bool AccessibilityMenuListPopup::isEnabled() const
{
    if (!m_parent)
        return false;
    
    return m_parent->isEnabled();
}

bool AccessibilityMenuListPopup::computeAccessibilityIsIgnored() const
{
    return accessibilityIsIgnoredByDefault();
}

AccessibilityMenuListOption* AccessibilityMenuListPopup::menuListOptionAccessibilityObject(HTMLElement* element) const
{
    if (!element || !isHTMLOptionElement(element))
        return 0;

    AccessibilityObject* object = document()->axObjectCache()->getOrCreate(MenuListOptionRole);
    ASSERT_WITH_SECURITY_IMPLICATION(object->isMenuListOption());

    AccessibilityMenuListOption* option = static_cast<AccessibilityMenuListOption*>(object);
    option->setElement(element);

    return option;
}

bool AccessibilityMenuListPopup::press() const
{
    if (!m_parent)
        return false;
    
    m_parent->press();
    return true;
}

void AccessibilityMenuListPopup::addChildren()
{
    if (!m_parent)
        return;
    
    Node* selectNode = m_parent->node();
    if (!selectNode)
        return;

    m_haveChildren = true;

    const Vector<HTMLElement*>& listItems = toHTMLSelectElement(selectNode)->listItems();
    unsigned length = listItems.size();
    for (unsigned i = 0; i < length; i++) {
        AccessibilityMenuListOption* option = menuListOptionAccessibilityObject(listItems[i]);
        if (option) {
            option->setParent(this);
            m_children.append(option);
        }
    }
}

void AccessibilityMenuListPopup::childrenChanged()
{
    AXObjectCache* cache = axObjectCache();
    for (size_t i = m_children.size(); i > 0 ; --i) {
        AccessibilityObject* child = m_children[i - 1].get();
        if (child->actionElement() && !child->actionElement()->attached()) {
            child->detachFromParent();
            cache->remove(child->axObjectID());
        }
    }
    
    m_children.clear();
    m_haveChildren = false;
    addChildren();
}

void AccessibilityMenuListPopup::didUpdateActiveOption(int optionIndex)
{
    ASSERT_ARG(optionIndex, optionIndex >= 0);
    ASSERT_ARG(optionIndex, optionIndex < static_cast<int>(m_children.size()));

    AXObjectCache* cache = axObjectCache();
    RefPtr<AccessibilityObject> child = m_children[optionIndex].get();

    cache->postNotification(child.get(), document(), AXObjectCache::AXFocusedUIElementChanged, true, PostSynchronously);
    cache->postNotification(child.get(), document(), AXObjectCache::AXMenuListItemSelected, true, PostSynchronously);
}

} // namespace WebCore
