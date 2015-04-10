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
#include "AccessibilityMenuList.h"

#include "AXObjectCache.h"
#include "AccessibilityMenuListPopup.h"
#include "RenderMenuList.h"

namespace WebCore {

AccessibilityMenuList::AccessibilityMenuList(RenderMenuList* renderer)
    : AccessibilityRenderObject(renderer)
{
}

PassRefPtr<AccessibilityMenuList> AccessibilityMenuList::create(RenderMenuList* renderer)
{
    return adoptRef(new AccessibilityMenuList(renderer));
}

bool AccessibilityMenuList::press() const
{
    RenderMenuList* menuList = static_cast<RenderMenuList*>(m_renderer);
    if (menuList->popupIsVisible())
        menuList->hidePopup();
    else
        menuList->showPopup();
    return true;
}

void AccessibilityMenuList::addChildren()
{
    m_haveChildren = true;

    AXObjectCache* cache = m_renderer->document()->axObjectCache();

    AccessibilityObject* list = cache->getOrCreate(MenuListPopupRole);
    if (!list)
        return;

    static_cast<AccessibilityMockObject*>(list)->setParent(this);
    if (list->accessibilityIsIgnored()) {
        cache->remove(list->axObjectID());
        return;
    }

    m_children.append(list);

    list->addChildren();
}

void AccessibilityMenuList::childrenChanged()
{
    if (m_children.isEmpty())
        return;

    ASSERT(m_children.size() == 1);
    m_children[0]->childrenChanged();
}

bool AccessibilityMenuList::isCollapsed() const
{
    return !static_cast<RenderMenuList*>(m_renderer)->popupIsVisible();
}

bool AccessibilityMenuList::canSetFocusAttribute() const
{
    if (!node())
        return false;

    return !toElement(node())->isDisabledFormControl();
}

void AccessibilityMenuList::didUpdateActiveOption(int optionIndex)
{
    RefPtr<Document> document = m_renderer->document();
    AXObjectCache* cache = document->axObjectCache();

    const AccessibilityChildrenVector& childObjects = children();
    if (!childObjects.isEmpty()) {
        ASSERT(childObjects.size() == 1);
        ASSERT(childObjects[0]->isMenuListPopup());

        if (childObjects[0]->isMenuListPopup()) {
            if (AccessibilityMenuListPopup* popup = static_cast<AccessibilityMenuListPopup*>(childObjects[0].get()))
                popup->didUpdateActiveOption(optionIndex);
        }
    }

    cache->postNotification(this, document.get(), AXObjectCache::AXMenuListValueChanged, true, PostSynchronously);
}

} // namespace WebCore
