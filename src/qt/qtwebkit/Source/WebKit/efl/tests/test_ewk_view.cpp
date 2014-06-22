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
* @brief Checking whether function properly returns correct value.
*/
TEST_F(EWKTestBase, ewk_view_editable_get)
{
    loadUrl();
    ewk_view_editable_set(webView(), true);
    ASSERT_TRUE(ewk_view_editable_get(webView()));
}

/**
* @brief Checking whether function returns correct uri string.
*/
TEST_F(EWKTestBase, ewk_view_uri_get)
{
    loadUrl();
    ASSERT_STREQ(Config::defaultTestPage, ewk_view_uri_get(webView()));
}

/**
* @brief Checking whether function properly get/set fullscreen setting value.
*/
TEST_F(EWKTestBase, ewk_view_setting_enable_fullscreen)
{
    loadUrl();
#if ENABLE(FULLSCREEN_API)
    ASSERT_TRUE(ewk_view_setting_enable_fullscreen_get(webView()));

    ASSERT_TRUE(ewk_view_setting_enable_fullscreen_set(webView(), true));
    ASSERT_TRUE(ewk_view_setting_enable_fullscreen_get(webView()));

    ASSERT_TRUE(ewk_view_setting_enable_fullscreen_set(webView(), false));
    ASSERT_FALSE(ewk_view_setting_enable_fullscreen_get(webView()));
#else
    ASSERT_FALSE(ewk_view_setting_enable_fullscreen_get(webView()));

    ASSERT_FALSE(ewk_view_setting_enable_fullscreen_set(webView(), true));
    ASSERT_FALSE(ewk_view_setting_enable_fullscreen_get(webView()));

    ASSERT_FALSE(ewk_view_setting_enable_fullscreen_set(webView(), false));
    ASSERT_FALSE(ewk_view_setting_enable_fullscreen_get(webView()));
#endif
}

/**
* @brief Checking whether function properly get/set fullscreen setting value.
*/
TEST_F(EWKTestBase, ewk_view_setting_tiled_backing_store)
{
    loadUrl();
    ASSERT_FALSE(ewk_view_setting_tiled_backing_store_enabled_get(webView()));

#if USE(TILED_BACKING_STORE)
    ASSERT_TRUE(ewk_view_setting_tiled_backing_store_enabled_set(webView(), true));
    ASSERT_TRUE(ewk_view_setting_tiled_backing_store_enabled_get(webView()));

    ASSERT_TRUE(ewk_view_setting_tiled_backing_store_enabled_set(webView(), false));
    ASSERT_FALSE(ewk_view_setting_tiled_backing_store_enabled_get(webView()));
#else
    ASSERT_FALSE(ewk_view_setting_tiled_backing_store_enabled_set(webView(), true));
    ASSERT_FALSE(ewk_view_setting_tiled_backing_store_enabled_get(webView()));

    ASSERT_FALSE(ewk_view_setting_tiled_backing_store_enabled_set(webView(), false));
    ASSERT_FALSE(ewk_view_setting_tiled_backing_store_enabled_get(webView()));
#endif
}

/**
 * @brief Checking whether function returns proper context menu structure.
 *
 * This test creates a context menu and checks if context menu structure
 * is not NULL;
 */
TEST_F(EWKTestBase, ewk_view_context_menu_get)
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

    ASSERT_TRUE(ewk_view_context_menu_get(webView()));
}
