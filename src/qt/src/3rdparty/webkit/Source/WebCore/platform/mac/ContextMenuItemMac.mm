/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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

#if ENABLE(CONTEXT_MENUS)

#include "ContextMenu.h"

namespace WebCore {

static NSMutableArray* menuToArray(NSMenu* menu)
{
    NSMutableArray* itemsArray = [NSMutableArray array];
    int total = [menu numberOfItems];
    for (int i = 0; i < total; i++)
        [itemsArray addObject:[menu itemAtIndex:i]];

    return itemsArray;
}

ContextMenuItem::ContextMenuItem(NSMenuItem* item)
{
    m_platformDescription = item;
}

ContextMenuItem::ContextMenuItem(ContextMenu* subMenu)
{
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    m_platformDescription = item;
    [item release];

    [m_platformDescription.get() setTag:ContextMenuItemTagNoAction];
    if (subMenu)
        setSubMenu(subMenu);
}

static PlatformMenuItemDescription createPlatformMenuItemDescription(ContextMenuItemType type, ContextMenuAction action, const String& title, bool enabled, bool checked)
{
    if (type == SeparatorType)
        return [[NSMenuItem separatorItem] retain];

    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:title action:nil keyEquivalent:@""];
    [item setEnabled:enabled];
    [item setState:checked ? NSOnState : NSOffState];
    [item setTag:action];

    return item;
}

ContextMenuItem::ContextMenuItem(ContextMenuItemType type, ContextMenuAction action, const String& title, ContextMenu* subMenu)
{
    m_platformDescription.adoptNS(createPlatformMenuItemDescription(type, action, title, true, false));

    if (subMenu)
        setSubMenu(subMenu);
}

ContextMenuItem::ContextMenuItem(ContextMenuItemType type, ContextMenuAction action, const String& title, bool enabled, bool checked)
{
    m_platformDescription.adoptNS(createPlatformMenuItemDescription(type, action, title, enabled, checked));
}

ContextMenuItem::ContextMenuItem(ContextMenuAction action, const String& title, bool enabled, bool checked, Vector<ContextMenuItem>& subMenuItems)
{
    m_platformDescription.adoptNS(createPlatformMenuItemDescription(SubmenuType, action, title, enabled, checked));
    
    setSubMenu(subMenuItems);
}

ContextMenuItem::~ContextMenuItem()
{
}

NSMenuItem* ContextMenuItem::releasePlatformDescription()
{
    NSMenuItem* item = [m_platformDescription.get() retain];
    m_platformDescription = 0;
    return item;
}

ContextMenuItemType ContextMenuItem::type() const
{
    if ([m_platformDescription.get() isSeparatorItem])
        return SeparatorType;
    if ([m_platformDescription.get() hasSubmenu])
        return SubmenuType;
    return ActionType;
}

ContextMenuAction ContextMenuItem::action() const
{ 
    return static_cast<ContextMenuAction>([m_platformDescription.get() tag]);
}

String ContextMenuItem::title() const 
{
    return [m_platformDescription.get() title];
}

NSMutableArray* ContextMenuItem::platformSubMenu() const
{
    return menuToArray([m_platformDescription.get() submenu]);
}

void ContextMenuItem::setType(ContextMenuItemType type)
{
    if (type == SeparatorType)
        m_platformDescription = [NSMenuItem separatorItem];
}

void ContextMenuItem::setAction(ContextMenuAction action)
{
    [m_platformDescription.get() setTag:action]; 
}

void ContextMenuItem::setTitle(const String& title)
{
    [m_platformDescription.get() setTitle:title];
}

void ContextMenuItem::setSubMenu(ContextMenu* menu)
{
    NSArray* subMenuArray = menu->platformDescription();
    NSMenu* subMenu = [[NSMenu alloc] init];
    [subMenu setAutoenablesItems:NO];
    for (unsigned i = 0; i < [subMenuArray count]; i++)
        [subMenu insertItem:[subMenuArray objectAtIndex:i] atIndex:i];
    [m_platformDescription.get() setSubmenu:subMenu];
    [subMenu release];
}

void ContextMenuItem::setSubMenu(Vector<ContextMenuItem>& subMenuItems)
{
    NSMenu* subMenu = [[NSMenu alloc] init];
    [subMenu setAutoenablesItems:NO];
    for (unsigned i = 0; i < subMenuItems.size(); ++i)
        [subMenu addItem:subMenuItems[i].releasePlatformDescription()];
        
    [m_platformDescription.get() setSubmenu:subMenu];
    [subMenu release];
}

void ContextMenuItem::setChecked(bool checked)
{
    if (checked)
        [m_platformDescription.get() setState:NSOnState];
    else
        [m_platformDescription.get() setState:NSOffState];
}

void ContextMenuItem::setEnabled(bool enable)
{
    [m_platformDescription.get() setEnabled:enable];
}

bool ContextMenuItem::enabled() const
{
    return [m_platformDescription.get() isEnabled];
}

bool ContextMenuItem::checked() const
{
    return [m_platformDescription.get() state] == NSOnState;
}

} // namespace WebCore

#endif // ENABLE(CONTEXT_MENUS)
