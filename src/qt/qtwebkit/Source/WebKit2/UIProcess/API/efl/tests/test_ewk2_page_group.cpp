/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "UnitTestUtils/EWK2UnitTestBase.h"

using namespace EWK2UnitTest;

extern EWK2UnitTestEnvironment* environment;

static const char htmlString[] = "<html><head><title>Foo</title></head><body style='background-color: red'><script>document.title=window.getComputedStyle(document.body, null).getPropertyValue('background-color')</script>HELLO</body></html>";
static const char userStyleSheet[] = "body { background-color: green !important; }";
static const char greenInRGB[] = "rgb(0, 128, 0)";
static const char redInRGB[] = "rgb(255, 0, 0)";

TEST_F(EWK2UnitTestBase, ewk_page_group)
{
    Evas_Smart* smart = evas_smart_class_new(&(ewkViewClass()->sc));
    Ewk_Page_Group* pageGroup = ewk_page_group_create("test");
    ASSERT_TRUE(pageGroup);

    Evas_Object* newWebView = ewk_view_smart_add(canvas(), smart, ewk_context_default_get(), pageGroup);
    setWebView(newWebView);

    ASSERT_EQ(pageGroup, ewk_view_page_group_get(newWebView));
    ewk_object_unref(pageGroup);
}

TEST_F(EWK2UnitTestBase, ewk_page_group_user_style_sheet_add_before_creating_view)
{
    Evas_Smart* smart = evas_smart_class_new(&(ewkViewClass()->sc));
    Ewk_Page_Group* pageGroup = ewk_page_group_create("test");
    ASSERT_TRUE(pageGroup);

    // Add a user style sheet to the page group before creating a view.
    ewk_page_group_user_style_sheet_add(pageGroup, userStyleSheet, 0, 0, 0, true);

    // Create a new web view with this page group.
    Evas_Object* newWebView = ewk_view_smart_add(canvas(), smart, ewk_context_default_get(), pageGroup);
    setWebView(newWebView);

    ewk_object_unref(pageGroup);
    ewk_view_theme_set(webView(), environment->defaultTheme());
    evas_object_resize(webView(), environment->defaultWidth(), environment->defaultHeight());
    evas_object_show(webView());
    evas_object_focus_set(webView(), true);

    ewk_view_html_string_load(webView(), htmlString, 0, 0);
    ASSERT_TRUE(waitUntilTitleChangedTo(greenInRGB));
}

TEST_F(EWK2UnitTestBase, ewk_page_group_user_style_sheet_add_after_creating_view)
{
    // Add a user style sheet to the page group after creating a view.
    Ewk_Page_Group* pageGroup = ewk_view_page_group_get(webView());
    ASSERT_TRUE(pageGroup);
    ewk_page_group_user_style_sheet_add(pageGroup, userStyleSheet, 0, 0, 0, true);

    ewk_view_html_string_load(webView(), htmlString, 0, 0);
    ASSERT_TRUE(waitUntilTitleChangedTo(greenInRGB));
}

TEST_F(EWK2UnitTestBase, ewk_page_group_user_style_sheets_remove_all)
{
    Evas_Smart* smart = evas_smart_class_new(&(ewkViewClass()->sc));
    Ewk_Page_Group* pageGroup = ewk_page_group_create("test");
    ASSERT_TRUE(pageGroup);

    // Add a user style sheet to the page group before creating a view.
    ewk_page_group_user_style_sheet_add(pageGroup, userStyleSheet, 0, 0, 0, true);

    // Create a new web view with this page group.
    Evas_Object* newWebView = ewk_view_smart_add(canvas(), smart, ewk_context_default_get(), pageGroup);
    setWebView(newWebView);

    ewk_object_unref(pageGroup);
    ewk_view_theme_set(webView(), environment->defaultTheme());
    evas_object_resize(webView(), environment->defaultWidth(), environment->defaultHeight());
    evas_object_show(webView());
    evas_object_focus_set(webView(), true);

    ewk_view_html_string_load(webView(), htmlString, 0, 0);

    // Remove all user style sheets in the page group.
    ewk_page_group_user_style_sheets_remove_all(ewk_view_page_group_get(webView()));
    ASSERT_TRUE(waitUntilTitleChangedTo(redInRGB));

    // Add a user style sheet to the page group after creating a view.
    pageGroup = ewk_view_page_group_get(webView());
    ewk_page_group_user_style_sheet_add(pageGroup, userStyleSheet, 0, 0, 0, true);
    ewk_view_html_string_load(webView(), htmlString, 0, 0);

    // Remove all user style sheets in the page group.
    ewk_page_group_user_style_sheets_remove_all(ewk_view_page_group_get(webView()));
    ASSERT_TRUE(waitUntilTitleChangedTo(redInRGB));
}
