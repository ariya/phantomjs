/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "ContextMenu.h"

#include "Document.h"
#include "Frame.h"
#include "FrameView.h"
#include "Node.h"
#include "NotImplemented.h"
#include <windows.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>

#ifndef MIIM_FTYPE
#define MIIM_FTYPE MIIM_TYPE
#endif
#ifndef MIIM_STRING
#define MIIM_STRING MIIM_TYPE
#endif

namespace WebCore {

ContextMenu::ContextMenu(HMENU menu)
{
    getContextMenuItems(menu, m_items);
}

void ContextMenu::getContextMenuItems(HMENU menu, Vector<ContextMenuItem>& items)
{
#if OS(WINCE)
    notImplemented();
#else
    int count = ::GetMenuItemCount(menu);
    if (count <= 0)
        return;

    for (int i = 0; i < count; ++i) {
        MENUITEMINFO info = {0};
        info.cbSize = sizeof(MENUITEMINFO);
        info.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING | MIIM_STATE | MIIM_SUBMENU;

        if (!::GetMenuItemInfo(menu, i, TRUE, &info))
            continue;

        if (info.fType == MFT_SEPARATOR) {
            items.append(ContextMenuItem(SeparatorType, ContextMenuItemTagNoAction, String()));
            continue;
        }

        int menuStringLength = info.cch + 1;
        OwnArrayPtr<WCHAR> menuString = adoptArrayPtr(new WCHAR[menuStringLength]);
        info.dwTypeData = menuString.get();
        info.cch = menuStringLength;

        if (::GetMenuItemInfo(menu, i, TRUE, &info))
           items.append(ContextMenuItem(info));
    }
#endif
}

HMENU ContextMenu::createPlatformContextMenuFromItems(const Vector<ContextMenuItem>& items)
{
    HMENU menu = ::CreatePopupMenu();

    for (size_t i = 0; i < items.size(); ++i) {
        const ContextMenuItem& item = items[i];

        MENUITEMINFO menuItem = item.platformContextMenuItem();

#if OS(WINCE)
        UINT flags = MF_BYPOSITION;
        UINT newItem = 0;
        LPCWSTR title = 0;

        if (item.type() == SeparatorType)
            flags |= MF_SEPARATOR;
        else {
            flags |= MF_STRING;
            flags |= item.checked() ? MF_CHECKED : MF_UNCHECKED;
            flags |= item.enabled() ? MF_ENABLED : MF_GRAYED;

            title = menuItem.dwTypeData;
            menuItem.dwTypeData = 0;

            if (menuItem.hSubMenu) {
                flags |= MF_POPUP;
                newItem = reinterpret_cast<UINT>(menuItem.hSubMenu);
                menuItem.hSubMenu = 0;
            } else
                newItem = menuItem.wID;
        }

        ::InsertMenuW(menu, i, flags, newItem, title);
#else
        // ContextMenuItem::platformContextMenuItem doesn't set the title of the MENUITEMINFO to make the
        // lifetime handling easier for callers.
        Vector<UChar> wideCharTitle; // Retain buffer for long enough to make the InsertMenuItem call

        const String& itemTitle = item.title();
        if (item.type() != SeparatorType) {
            menuItem.fMask |= MIIM_STRING;
            menuItem.cch = itemTitle.length();
            wideCharTitle = itemTitle.charactersWithNullTermination();
            menuItem.dwTypeData = const_cast<LPWSTR>(wideCharTitle.data());
        }

        ::InsertMenuItem(menu, i, TRUE, &menuItem);
#endif
    }

    return menu;
}

HMENU ContextMenu::platformContextMenu() const
{
    return createPlatformContextMenuFromItems(m_items);
}

} // namespace WebCore
