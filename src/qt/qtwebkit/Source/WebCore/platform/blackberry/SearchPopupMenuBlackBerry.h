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

#ifndef SearchPopupMenuBlackBerry_h
#define SearchPopupMenuBlackBerry_h

#include "PopupMenuBlackBerry.h"
#include "SearchPopupMenu.h"

namespace WebCore {

class SearchPopupMenuBlackBerry : public SearchPopupMenu {
public:
    SearchPopupMenuBlackBerry(PopupMenuClient*);

    virtual PopupMenu* popupMenu();
    virtual void saveRecentSearches(const AtomicString& name, const Vector<String>& searchItems);
    virtual void loadRecentSearches(const AtomicString& name, Vector<String>& searchItems);
    virtual bool enabled();

private:
    RefPtr<PopupMenuBlackBerry> m_popup;
};

}

#endif // SearchPopupMenuBlackBerry_h
