/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2011 Igalia S.L.
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

#ifndef GtkPopupMenu_h
#define GtkPopupMenu_h

#include "GRefPtrGtk.h"
#include "IntPoint.h"
#include "IntSize.h"
#include <wtf/FastAllocBase.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/WTFString.h>

typedef struct _GdkEventKey GdkEventKey;

namespace WebCore {

class GtkPopupMenu {
    WTF_MAKE_NONCOPYABLE(GtkPopupMenu);
    WTF_MAKE_FAST_ALLOCATED;

public:
    static PassOwnPtr<GtkPopupMenu> create()
    {
        return adoptPtr(new GtkPopupMenu());
    }

    ~GtkPopupMenu();

    GtkWidget* platformMenu() const { return m_popup.get(); }
    void clear();
    void appendSeparator();
    void appendItem(GtkAction*);
    void popUp(const IntSize&, const IntPoint&, int itemsCount, int selectedItem, const GdkEvent*);
    void popDown();

private:
    GtkPopupMenu();

    void resetTypeAheadFindState();
    bool typeAheadFind(GdkEventKey*);

    static void menuItemActivated(GtkMenuItem*, GtkPopupMenu*);
    static void menuPositionFunction(GtkMenu*, gint*, gint*, gboolean*, GtkPopupMenu*);
    static void menuRemoveItem(GtkWidget*, GtkPopupMenu*);
    static void selectItemCallback(GtkMenuItem*, GtkPopupMenu*);
    static gboolean keyPressEventCallback(GtkWidget*, GdkEventKey*, GtkPopupMenu*);

    GRefPtr<GtkWidget> m_popup;
    IntPoint m_menuPosition;
    String m_currentSearchString;
    uint32_t m_previousKeyEventTimestamp;
    unsigned int m_previousKeyEventCharacter;
    GtkWidget* m_currentlySelectedMenuItem;
    unsigned int m_keyPressHandlerID;
};

}

#endif // GtkPopupMenu_h
