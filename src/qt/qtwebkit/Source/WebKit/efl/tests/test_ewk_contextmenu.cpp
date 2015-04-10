/*
    Copyright (C) 2012 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "config.h"

#include "UnitTestUtils/EWKTestBase.h"
#include "UnitTestUtils/EWKTestConfig.h"
#include <EWebKit.h>
#include <Ecore.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

using namespace EWKUnitTests;

/**
 * @brief Checking whether function creates proper menu item.
 *
 * This test creates a menu item and checks if all menu item's attributes are
 * the same as passed to tested function.
 */
TEST_F(EWKTestBase, ewk_context_menu_item_new)
{
    loadUrl();

    Evas* evas = evas_object_evas_get(webView());
    ASSERT_TRUE(evas);

    Evas_Event_Mouse_Down mouseDown;
    mouseDown.button = 3;
    mouseDown.output.x = 0;
    mouseDown.output.y = 0;
    mouseDown.canvas.x = 0;
    mouseDown.canvas.y = 0;
    mouseDown.data = 0;
    mouseDown.modifiers = const_cast<Evas_Modifier*>(evas_key_modifier_get(evas));
    mouseDown.locks = const_cast<Evas_Lock*>(evas_key_lock_get(evas));
    mouseDown.flags = EVAS_BUTTON_NONE;
    mouseDown.timestamp = ecore_loop_time_get();
    mouseDown.event_flags = EVAS_EVENT_FLAG_NONE;
    mouseDown.dev = 0;

    ASSERT_TRUE(ewk_view_context_menu_forward_event(webView(), &mouseDown));

    Ewk_Context_Menu* contextMenu = ewk_view_context_menu_get(webView());

    ASSERT_TRUE(contextMenu);

    Ewk_Context_Menu_Item_Type itemType = EWK_ACTION_TYPE;
    Ewk_Context_Menu_Action itemAction = EWK_CONTEXT_MENU_ITEM_CUSTOM_TAG_NO_ACTION;
    const char* itemTitle = "Test Item";
    bool itemChecked = false;
    bool itemEnabled = false;

    Ewk_Context_Menu_Item* contextMenuItem = ewk_context_menu_item_new(itemType, itemAction, contextMenu, 0, itemTitle, itemChecked, itemEnabled);

    ASSERT_TRUE(contextMenuItem);

    EXPECT_EQ(itemType, ewk_context_menu_item_type_get(contextMenuItem));
    EXPECT_EQ(itemAction, ewk_context_menu_item_action_get(contextMenuItem));
    EXPECT_EQ(contextMenu, ewk_context_menu_item_parent_get(contextMenuItem));
    EXPECT_STREQ(itemTitle, ewk_context_menu_item_title_get(contextMenuItem));
    EXPECT_EQ(itemChecked, ewk_context_menu_item_checked_get(contextMenuItem));
    EXPECT_EQ(itemEnabled, ewk_context_menu_item_enabled_get(contextMenuItem));

    ewk_context_menu_item_free(contextMenuItem);
}

/**
 * @brief Checking whether function returns proper parent menu.
 *
 * This test creates a context menus, and checks if created context menu's
 * parent is the same for each of menu items.
 */
TEST_F(EWKTestBase, ewk_context_menu_item_parent_get)
{
    loadUrl();

    Evas* evas = evas_object_evas_get(webView());
    ASSERT_TRUE(evas);

    Evas_Event_Mouse_Down mouseDown;
    mouseDown.button = 3;
    mouseDown.output.x = 0;
    mouseDown.output.y = 0;
    mouseDown.canvas.x = 0;
    mouseDown.canvas.y = 0;
    mouseDown.data = 0;
    mouseDown.modifiers = const_cast<Evas_Modifier*>(evas_key_modifier_get(evas));
    mouseDown.locks = const_cast<Evas_Lock*>(evas_key_lock_get(evas));
    mouseDown.flags = EVAS_BUTTON_NONE;
    mouseDown.timestamp = ecore_loop_time_get();
    mouseDown.event_flags = EVAS_EVENT_FLAG_NONE;
    mouseDown.dev = 0;

    ASSERT_TRUE(ewk_view_context_menu_forward_event(webView(), &mouseDown));

    Ewk_Context_Menu* contextMenu = ewk_view_context_menu_get(webView());

    ASSERT_TRUE(contextMenu);

    const Eina_List* contextMenuItems = ewk_context_menu_item_list_get(contextMenu);

    ASSERT_TRUE(contextMenuItems);

    const Eina_List* listIterator;
    void* data;
    EINA_LIST_FOREACH(contextMenuItems, listIterator, data)
        EXPECT_EQ(contextMenu, ewk_context_menu_item_parent_get(static_cast<Ewk_Context_Menu_Item*>(data)));
}
