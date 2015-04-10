/*
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2012 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef ewk_contextmenu_private_h
#define ewk_contextmenu_private_h

#include "ewk_contextmenu.h"

// forward declarations
namespace WebCore {
struct ContextMenu;
struct ContextMenuItem;
}

#if ENABLE(CONTEXT_MENUS)
Ewk_Context_Menu* ewk_context_menu_new(Evas_Object* view, WebCore::ContextMenuController* controller, WebCore::ContextMenu* coreMenu);
bool ewk_context_menu_free(Ewk_Context_Menu* menu);
void ewk_context_menu_item_append(Ewk_Context_Menu* menu, const WebCore::ContextMenuItem& core);
void ewk_context_menu_show(Ewk_Context_Menu* menu);
#endif

#endif // ewk_context_menu_private_h
