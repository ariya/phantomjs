/*
 * Copyright (C) 2006 Apple Computer, Inc.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#import "config.h"
#import "SearchPopupMenuMac.h"

#include <wtf/text/AtomicString.h>

namespace WebCore {

SearchPopupMenuMac::SearchPopupMenuMac(PopupMenuClient* client)
    : m_popup(adoptRef(new PopupMenuMac(client)))
{
}

static NSString* autosaveKey(const String& name)
{
    return [@"com.apple.WebKit.searchField:" stringByAppendingString:name];
}

PopupMenu* SearchPopupMenuMac::popupMenu()
{
    return m_popup.get();
}

bool SearchPopupMenuMac::enabled()
{
    return true;
}

void SearchPopupMenuMac::saveRecentSearches(const AtomicString& name, const Vector<String>& searchItems)
{
    if (name.isEmpty())
        return;

    size_t size = searchItems.size();
    if (size == 0)
        [[NSUserDefaults standardUserDefaults] removeObjectForKey:autosaveKey(name)];
    else {
        NSMutableArray* items = [[NSMutableArray alloc] initWithCapacity:size];
        for (size_t i = 0; i < size; ++i)
            [items addObject:searchItems[i]];
        [[NSUserDefaults standardUserDefaults] setObject:items forKey:autosaveKey(name)];
        [items release];
    }
}

void SearchPopupMenuMac::loadRecentSearches(const AtomicString& name, Vector<String>& searchItems)
{
    if (name.isEmpty())
        return;

    searchItems.clear();
    NSArray* items = [[NSUserDefaults standardUserDefaults] arrayForKey:autosaveKey(name)];
    size_t size = [items count];
    for (size_t i = 0; i < size; ++i) {
        NSString* item = [items objectAtIndex:i];
        if ([item isKindOfClass:[NSString class]])
            searchItems.append(item);
    }
}

}
