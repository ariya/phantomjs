/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(CONTEXT_MENUS)

#include "WebContextMenuItemData.h"

#include "APIObject.h"
#include "ArgumentCoders.h"
#include "Arguments.h"
#include <wtf/text/CString.h>
#include <WebCore/ContextMenu.h>

using namespace WebCore;

namespace WebKit {

WebContextMenuItemData::WebContextMenuItemData()
    : m_type(WebCore::ActionType)
    , m_action(WebCore::ContextMenuItemTagNoAction)
    , m_enabled(true)
    , m_checked(false)
{
}

WebContextMenuItemData::WebContextMenuItemData(WebCore::ContextMenuItemType type, WebCore::ContextMenuAction action, const String& title, bool enabled, bool checked)
    : m_type(type)
    , m_action(action)
    , m_title(title)
    , m_enabled(enabled)
    , m_checked(checked)
{
    ASSERT(type == WebCore::ActionType || type == WebCore::CheckableActionType || type == WebCore::SeparatorType);
}

WebContextMenuItemData::WebContextMenuItemData(WebCore::ContextMenuAction action, const String& title, bool enabled, const Vector<WebContextMenuItemData>& submenu)
    : m_type(WebCore::SubmenuType)
    , m_action(action)
    , m_title(title)
    , m_enabled(enabled)
    , m_checked(false)
    , m_submenu(submenu)
{
}

WebContextMenuItemData::WebContextMenuItemData(const WebCore::ContextMenuItem& item, WebCore::ContextMenu* menu)
    : m_type(item.type())
    , m_action(item.action())
    , m_title(item.title())
{
    if (m_type == WebCore::SubmenuType) {
#if USE(CROSS_PLATFORM_CONTEXT_MENUS)
        const Vector<WebCore::ContextMenuItem>& coreSubmenu = item.subMenuItems();
#else
        Vector<WebCore::ContextMenuItem> coreSubmenu = WebCore::contextMenuItemVector(item.platformSubMenu());
#endif
        m_submenu = kitItems(coreSubmenu, menu);
    }
    
    m_enabled = item.enabled();
    m_checked = item.checked();
}

ContextMenuItem WebContextMenuItemData::core() const
{
    if (m_type != SubmenuType)
        return ContextMenuItem(m_type, m_action, m_title, m_enabled, m_checked);
    
    Vector<ContextMenuItem> subMenuItems = coreItems(m_submenu);
    return ContextMenuItem(m_action, m_title, m_enabled, m_checked, subMenuItems);
}

APIObject* WebContextMenuItemData::userData() const
{
    return m_userData.get();
}

void WebContextMenuItemData::setUserData(APIObject* userData)
{
    m_userData = userData;
}
    
void WebContextMenuItemData::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder.encodeEnum(m_type);
    encoder.encodeEnum(m_action);
    encoder << m_title;
    encoder << m_checked;
    encoder << m_enabled;
    encoder << m_submenu;
}

bool WebContextMenuItemData::decode(CoreIPC::ArgumentDecoder& decoder, WebContextMenuItemData& item)
{
    WebCore::ContextMenuItemType type;
    if (!decoder.decodeEnum(type))
        return false;

    WebCore::ContextMenuAction action;
    if (!decoder.decodeEnum(action))
        return false;

    String title;
    if (!decoder.decode(title))
        return false;

    bool checked;
    if (!decoder.decode(checked))
        return false;

    bool enabled;
    if (!decoder.decode(enabled))
        return false;

    Vector<WebContextMenuItemData> submenu;
    if (!decoder.decode(submenu))
        return false;

    switch (type) {
    case WebCore::ActionType:
    case WebCore::SeparatorType:
    case WebCore::CheckableActionType:
        item = WebContextMenuItemData(type, action, title, enabled, checked);
        break;
    case WebCore::SubmenuType:
        item = WebContextMenuItemData(action, title, enabled, submenu);
        break;
    default:
        ASSERT_NOT_REACHED();
        return false;
    }

    return true;
}

Vector<WebContextMenuItemData> kitItems(const Vector<WebCore::ContextMenuItem>& coreItemVector, WebCore::ContextMenu* menu)
{
    Vector<WebContextMenuItemData> result;
    result.reserveCapacity(coreItemVector.size());
    for (unsigned i = 0; i < coreItemVector.size(); ++i)
        result.append(WebContextMenuItemData(coreItemVector[i], menu));
    
    return result;
}

Vector<ContextMenuItem> coreItems(const Vector<WebContextMenuItemData>& kitItemVector)
{
    Vector<ContextMenuItem> result;
    result.reserveCapacity(kitItemVector.size());
    for (unsigned i = 0; i < kitItemVector.size(); ++i)
        result.append(kitItemVector[i].core());
    
    return result;
}

} // namespace WebKit
#endif // ENABLE(CONTEXT_MENUS)
