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
#include "ContextMenuItem.h"

#include "ContextMenu.h"

#if OS(WINCE)
#ifndef MFS_DISABLED
#define MFS_DISABLED MF_GRAYED
#endif
#ifndef MIIM_FTYPE
#define MIIM_FTYPE MIIM_TYPE
#endif
#ifndef MIIM_STRING
#define MIIM_STRING 0
#endif
#endif

namespace WebCore {

ContextMenuItem::ContextMenuItem(const MENUITEMINFO& info)
{
    if (info.fMask & MIIM_FTYPE)
        m_type = info.fType == MFT_SEPARATOR ? SeparatorType : ActionType;
    else
        m_type = SeparatorType;

    if (m_type == ActionType && info.fMask & MIIM_STRING)
        m_title = String(info.dwTypeData, info.cch);

    if ((info.fMask & MIIM_SUBMENU) && info.hSubMenu) {
        m_type = SubmenuType;
        ContextMenu::getContextMenuItems(info.hSubMenu, m_subMenuItems);
    }

    if (info.fMask & MIIM_ID)
        m_action = static_cast<ContextMenuAction>(info.wID);
    else
        m_action = ContextMenuItemTagNoAction;

    if (info.fMask & MIIM_STATE) {
        m_checked = info.fState & MFS_CHECKED;
        m_enabled = !(info.fState & MFS_DISABLED);
    } else {
        m_checked = false;
        m_enabled = false;
    }
}

// ContextMenuItem::platformContextMenuItem doesn't set the info.dwTypeData. This is
// done to make the lifetime handling of the returned MENUITEMINFO easier on
// callers. Callers can set dwTypeData themselves (and make their own decisions
// about its lifetime) if they need it.
MENUITEMINFO ContextMenuItem::platformContextMenuItem() const
{
    MENUITEMINFO info = {0};
    info.cbSize = sizeof(MENUITEMINFO);

    if (m_type == SeparatorType) {
        info.fMask = MIIM_FTYPE;
        info.fType = MFT_SEPARATOR;
        return info;
    }

    info.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STATE;
    info.fType = MFT_STRING;

    info.wID = m_action;

    if (m_type == SubmenuType) {
        info.fMask |= MIIM_SUBMENU;
        info.hSubMenu = ContextMenu::createPlatformContextMenuFromItems(m_subMenuItems);
    }

    info.fState |= m_enabled ? MFS_ENABLED : MFS_DISABLED;
    info.fState |= m_checked ? MFS_CHECKED : MFS_UNCHECKED;

    return info;
}

}
