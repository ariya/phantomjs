/*
    Copyright (C) 2012 Samsung Electronics
    Copyright (C) 2012 Intel Corporation. All rights reserved.

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

#include "UnitTestUtils/EWK2UnitTestBase.h"

using namespace EWK2UnitTest;

extern EWK2UnitTestEnvironment* environment;

static Ewk_Popup_Menu* s_popupMenu = 0;

class EWK2PopupMenuTest : public EWK2UnitTestBase {
public:
    static void checkBasicPopupMenuItem(Ewk_Popup_Menu_Item* item, const char* title, bool enabled)
    {
        EXPECT_EQ(EWK_POPUP_MENU_ITEM, ewk_popup_menu_item_type_get(item));
        EXPECT_STREQ(title, ewk_popup_menu_item_text_get(item));
        EXPECT_EQ(enabled, ewk_popup_menu_item_enabled_get(item));
    }

    static Eina_Bool selectItemAfterDelayed(void* data)
    {
        EXPECT_TRUE(ewk_popup_menu_selected_index_set(static_cast<Ewk_Popup_Menu*>(data), 0));
        return ECORE_CALLBACK_CANCEL;
    }

    static Eina_Bool showPopupMenu(Ewk_View_Smart_Data* smartData, Eina_Rectangle, Ewk_Text_Direction, double, Ewk_Popup_Menu* popupMenu)
    {
        s_popupMenu = popupMenu;

        EXPECT_EQ(2, ewk_popup_menu_selected_index_get(popupMenu));

        const Eina_List* list = ewk_popup_menu_items_get(popupMenu);

        Ewk_Popup_Menu_Item* item = static_cast<Ewk_Popup_Menu_Item*>(eina_list_nth(list, 0));
        checkBasicPopupMenuItem(item, "first", true);
        EXPECT_EQ(EWK_TEXT_DIRECTION_LEFT_TO_RIGHT, ewk_popup_menu_item_text_direction_get(item));
        EXPECT_STREQ("", ewk_popup_menu_item_tooltip_get(item));
        EXPECT_STREQ("", ewk_popup_menu_item_accessibility_text_get(item));
        EXPECT_FALSE(ewk_popup_menu_item_is_label_get(item));
        EXPECT_FALSE(ewk_popup_menu_item_selected_get(item));

        item = static_cast<Ewk_Popup_Menu_Item*>(eina_list_nth(list, 1));
        checkBasicPopupMenuItem(item, "second", false);
        EXPECT_FALSE(ewk_popup_menu_item_enabled_get(item));

        item = static_cast<Ewk_Popup_Menu_Item*>(eina_list_nth(list, 2));
        checkBasicPopupMenuItem(item, "third", true);
        EXPECT_EQ(EWK_TEXT_DIRECTION_RIGHT_TO_LEFT, ewk_popup_menu_item_text_direction_get(item));
        EXPECT_STREQ("tooltip", ewk_popup_menu_item_tooltip_get(item));
        EXPECT_STREQ("aria", ewk_popup_menu_item_accessibility_text_get(item));
        EXPECT_TRUE(ewk_popup_menu_item_selected_get(item));

        item = static_cast<Ewk_Popup_Menu_Item*>(eina_list_nth(list, 3));
        checkBasicPopupMenuItem(item, "label", false);
        EXPECT_TRUE(ewk_popup_menu_item_is_label_get(item));

        item = static_cast<Ewk_Popup_Menu_Item*>(eina_list_nth(list, 4));
        checkBasicPopupMenuItem(item, "    forth", true);

        item = static_cast<Ewk_Popup_Menu_Item*>(eina_list_nth(list, 5));
        EXPECT_EQ(EWK_POPUP_MENU_UNKNOWN, ewk_popup_menu_item_type_get(item));
        EXPECT_STREQ(0, ewk_popup_menu_item_text_get(item));

        ecore_timer_add(0, selectItemAfterDelayed, popupMenu);
        return true;
    }
};

TEST_F(EWK2PopupMenuTest, ewk_popup_menu_select_item)
{
    const char* selectHTML =
        "<!doctype html><body><select onchange=\"document.title=this.value;\">"
        "<option>first</option><option disabled>second</option><option selected dir=\"rtl\" title=\"tooltip\" aria-label=\"aria\">third</option>"
        "<optgroup label=\"label\"><option>forth</option></optgroup>"
        "</select></body>";

    ewkViewClass()->popup_menu_show = showPopupMenu;

    ewk_view_html_string_load(webView(), selectHTML, "file:///", 0);
    ASSERT_TRUE(waitUntilLoadFinished());
    mouseClick(30, 20);
    ASSERT_TRUE(waitUntilTitleChangedTo("first"));

    ASSERT_TRUE(s_popupMenu);
    EXPECT_TRUE(ewk_popup_menu_close(s_popupMenu));
}
