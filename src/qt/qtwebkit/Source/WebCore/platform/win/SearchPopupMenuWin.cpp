/*
 * Copyright (C) 2006, 2007 Apple Inc.
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

#include "config.h"
#include "SearchPopupMenuWin.h"

#include <wtf/text/AtomicString.h>

#if USE(CF)
#include <wtf/RetainPtr.h>
#endif

namespace WebCore {

SearchPopupMenuWin::SearchPopupMenuWin(PopupMenuClient* client)
    : m_popup(adoptRef(new PopupMenuWin(client)))
{
}

PopupMenu* SearchPopupMenuWin::popupMenu()
{
    return m_popup.get();
}

bool SearchPopupMenuWin::enabled()
{
#if USE(CF)
    return true;
#else
    return false;
#endif
}

#if USE(CF)
static RetainPtr<CFStringRef> autosaveKey(const String& name)
{
    return String("com.apple.WebKit.searchField:" + name).createCFString();
}
#endif

void SearchPopupMenuWin::saveRecentSearches(const AtomicString& name, const Vector<String>& searchItems)
{
    if (name.isEmpty())
        return;

#if USE(CF)
    RetainPtr<CFMutableArrayRef> items;

    size_t size = searchItems.size();
    if (size) {
        items = adoptCF(CFArrayCreateMutable(0, size, &kCFTypeArrayCallBacks));
        for (size_t i = 0; i < size; ++i)
            CFArrayAppendValue(items.get(), searchItems[i].createCFString().get());
    }

    CFPreferencesSetAppValue(autosaveKey(name).get(), items.get(), kCFPreferencesCurrentApplication);
    CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
#endif
}

void SearchPopupMenuWin::loadRecentSearches(const AtomicString& name, Vector<String>& searchItems)
{
    if (name.isEmpty())
        return;

#if USE(CF)
    searchItems.clear();
    RetainPtr<CFArrayRef> items = adoptCF(reinterpret_cast<CFArrayRef>(CFPreferencesCopyAppValue(autosaveKey(name).get(), kCFPreferencesCurrentApplication)));

    if (!items || CFGetTypeID(items.get()) != CFArrayGetTypeID())
        return;

    size_t size = CFArrayGetCount(items.get());
    for (size_t i = 0; i < size; ++i) {
        CFStringRef item = (CFStringRef)CFArrayGetValueAtIndex(items.get(), i);
        if (CFGetTypeID(item) == CFStringGetTypeID())
            searchItems.append(item);
    }
#endif
}

}
