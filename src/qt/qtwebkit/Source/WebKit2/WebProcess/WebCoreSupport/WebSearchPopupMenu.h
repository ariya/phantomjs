/*
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
 *
 */

#ifndef WebSearchPopupMenu_h
#define WebSearchPopupMenu_h

#include "WebPopupMenu.h"
#include <WebCore/SearchPopupMenu.h>

namespace WebKit {

class WebSearchPopupMenu : public WebCore::SearchPopupMenu {
public:
    static PassRefPtr<WebSearchPopupMenu> create(WebPage*, WebCore::PopupMenuClient*);

    virtual WebCore::PopupMenu* popupMenu() OVERRIDE;
    virtual void saveRecentSearches(const WTF::AtomicString& name, const Vector<String>& searchItems) OVERRIDE;
    virtual void loadRecentSearches(const WTF::AtomicString& name, Vector<String>& searchItems) OVERRIDE;
    virtual bool enabled() OVERRIDE;

private:
    WebSearchPopupMenu(WebPage*, WebCore::PopupMenuClient*);

    RefPtr<WebPopupMenu> m_popup;
};

}

#endif // WebSearchPopupMenu_h
