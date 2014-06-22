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

#ifndef WebContextMenuItemData_h
#define WebContextMenuItemData_h

#if ENABLE(CONTEXT_MENUS)

#include <WebCore/ContextMenuItem.h>
#include <wtf/text/WTFString.h>

namespace CoreIPC {
    class ArgumentDecoder;
    class ArgumentEncoder;
}

namespace WebCore {
    class ContextMenu;
}

namespace WebKit {

class APIObject;

class WebContextMenuItemData {
public:
    WebContextMenuItemData();
    WebContextMenuItemData(const WebCore::ContextMenuItem&, WebCore::ContextMenu* menu);
    WebContextMenuItemData(WebCore::ContextMenuItemType, WebCore::ContextMenuAction, const String& title, bool enabled, bool checked);
    WebContextMenuItemData(WebCore::ContextMenuAction, const String& title, bool enabled, const Vector<WebContextMenuItemData>& submenu);

    WebCore::ContextMenuItemType type() const { return m_type; }
    WebCore::ContextMenuAction action() const { return m_action; }
    const String& title() const { return m_title; }
    bool enabled() const { return m_enabled; }
    bool checked() const { return m_checked; }
    const Vector<WebContextMenuItemData>& submenu() const { return m_submenu; }
    
    WebCore::ContextMenuItem core() const;
    
    APIObject* userData() const;
    void setUserData(APIObject*);
    
    void encode(CoreIPC::ArgumentEncoder&) const;
    static bool decode(CoreIPC::ArgumentDecoder&, WebContextMenuItemData&);

private:
    WebCore::ContextMenuItemType m_type;
    WebCore::ContextMenuAction m_action;
    String m_title;
    bool m_enabled;
    bool m_checked;
    Vector<WebContextMenuItemData> m_submenu;
    RefPtr<APIObject> m_userData;
};

Vector<WebContextMenuItemData> kitItems(const Vector<WebCore::ContextMenuItem>&, WebCore::ContextMenu*);
Vector<WebCore::ContextMenuItem> coreItems(const Vector<WebContextMenuItemData>&);

} // namespace WebKit

#endif // ENABLE(CONTEXT_MENUS)
#endif // WebContextMenuItemData_h
