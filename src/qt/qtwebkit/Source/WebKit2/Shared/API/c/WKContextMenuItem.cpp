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
#include "WKContextMenuItem.h"

#include "MutableArray.h"
#include "WebContextMenuItem.h"
#include "WebContextMenuItemData.h"
#include "WKAPICast.h"
#include "WKContextMenuItemTypes.h"

#if PLATFORM(MAC)
#import <mach-o/dyld.h>
#endif

using namespace WebCore;
using namespace WebKit;

WKTypeID WKContextMenuItemGetTypeID()
{
#if ENABLE(CONTEXT_MENUS)
    return toAPI(WebContextMenuItem::APIType);
#else
    return toAPI(APIObject::TypeNull);
#endif
}

WKContextMenuItemRef WKContextMenuItemCreateAsAction(WKContextMenuItemTag tag, WKStringRef title, bool enabled)
{
#if ENABLE(CONTEXT_MENUS)
    return toAPI(WebContextMenuItem::create(WebContextMenuItemData(ActionType, toImpl(tag), toImpl(title)->string(), enabled, false)).leakRef());
#else
    return 0;
#endif
}

WKContextMenuItemRef WKContextMenuItemCreateAsCheckableAction(WKContextMenuItemTag tag, WKStringRef title, bool enabled, bool checked)
{
#if ENABLE(CONTEXT_MENUS)
    return toAPI(WebContextMenuItem::create(WebContextMenuItemData(CheckableActionType, toImpl(tag), toImpl(title)->string(), enabled, checked)).leakRef());
#else
    return 0;
#endif
}

WKContextMenuItemRef WKContextMenuItemCreateAsSubmenu(WKStringRef title, bool enabled, WKArrayRef submenuItems)
{
#if ENABLE(CONTEXT_MENUS)
    return toAPI(WebContextMenuItem::create(toImpl(title)->string(), enabled, toImpl(submenuItems)).leakRef());
#else
    return 0;
#endif
}

WKContextMenuItemRef WKContextMenuItemSeparatorItem()
{
#if ENABLE(CONTEXT_MENUS)
    return toAPI(WebContextMenuItem::separatorItem());
#else
    return 0;
#endif
}

#if PLATFORM(MAC)
static WKContextMenuItemTag compatibleContextMenuItemTag(WKContextMenuItemTag tag)
{
    static bool needsWorkaround = ^bool {
        const int32_t safariFrameworkVersionWithIncompatibleContextMenuItemTags = 0x02181900; // 536.25.0 (Safari 6.0)
        return NSVersionOfRunTimeLibrary("Safari") == safariFrameworkVersionWithIncompatibleContextMenuItemTags;
    }();

    if (!needsWorkaround)
        return tag;

    // kWKContextMenuItemTagDictationAlternative was inserted before kWKContextMenuItemTagInspectElement.
    // DictationAlternative is now at the end like it should have been. To be compatible we need to return
    // InspectElement for DictationAlternative and shift InspectElement and after by one.
    if (tag == kWKContextMenuItemTagDictationAlternative)
        return kWKContextMenuItemTagInspectElement;
    if (tag >= kWKContextMenuItemTagInspectElement && tag < kWKContextMenuItemBaseApplicationTag)
        return tag + 1;
    return tag;
}
#endif

WKContextMenuItemTag WKContextMenuItemGetTag(WKContextMenuItemRef itemRef)
{
#if ENABLE(CONTEXT_MENUS)
#if PLATFORM(MAC)
    return compatibleContextMenuItemTag(toAPI(toImpl(itemRef)->data()->action()));
#else
    return toAPI(toImpl(itemRef)->data()->action());
#endif
#else
    return toAPI(ContextMenuItemTagNoAction);
#endif
}

WKContextMenuItemType WKContextMenuItemGetType(WKContextMenuItemRef itemRef)
{
#if ENABLE(CONTEXT_MENUS)
    return toAPI(toImpl(itemRef)->data()->type());
#else
    return toAPI(ActionType);
#endif
}

WKStringRef WKContextMenuItemCopyTitle(WKContextMenuItemRef itemRef)
{
#if ENABLE(CONTEXT_MENUS)
    return toCopiedAPI(toImpl(itemRef)->data()->title().impl());
#else
    return 0;
#endif
}

bool WKContextMenuItemGetEnabled(WKContextMenuItemRef itemRef)
{
#if ENABLE(CONTEXT_MENUS)
    return toImpl(itemRef)->data()->enabled();
#else
    return false;
#endif
}

bool WKContextMenuItemGetChecked(WKContextMenuItemRef itemRef)
{
#if ENABLE(CONTEXT_MENUS)
    return toImpl(itemRef)->data()->checked();
#else
    return false;
#endif
}

WKArrayRef WKContextMenuCopySubmenuItems(WKContextMenuItemRef itemRef)
{
#if ENABLE(CONTEXT_MENUS)
    return toAPI(toImpl(itemRef)->submenuItemsAsImmutableArray().leakRef());
#else
    return 0;
#endif
}

WKTypeRef WKContextMenuItemGetUserData(WKContextMenuItemRef itemRef)
{
#if ENABLE(CONTEXT_MENUS)
    return toAPI(toImpl(itemRef)->userData());
#else
    return 0;
#endif
}

void WKContextMenuItemSetUserData(WKContextMenuItemRef itemRef, WKTypeRef userDataRef)
{
#if ENABLE(CONTEXT_MENUS)
    toImpl(itemRef)->setUserData(toImpl(userDataRef));
#endif
}
