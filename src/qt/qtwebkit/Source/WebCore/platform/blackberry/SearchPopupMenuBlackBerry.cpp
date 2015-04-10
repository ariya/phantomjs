/*
 * Copyright (C) 2009, 2010 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "SearchPopupMenuBlackBerry.h"

#include "NotImplemented.h"

#include <wtf/text/AtomicString.h>

namespace WebCore {

SearchPopupMenuBlackBerry::SearchPopupMenuBlackBerry(PopupMenuClient* client)
    : m_popup(adoptRef(new PopupMenuBlackBerry(client)))
{
    notImplemented();
}

PopupMenu* SearchPopupMenuBlackBerry::popupMenu()
{
    return m_popup.get();
}

bool SearchPopupMenuBlackBerry::enabled()
{
    notImplemented();
    return false;
}

void SearchPopupMenuBlackBerry::saveRecentSearches(AtomicString const&, const Vector<String>&)
{
    notImplemented();
}

void SearchPopupMenuBlackBerry::loadRecentSearches(AtomicString const&, Vector<String>&)
{
    notImplemented();
}

} // namespace WebCore
