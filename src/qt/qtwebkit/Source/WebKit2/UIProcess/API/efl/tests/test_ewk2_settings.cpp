/*
 * Copyright (C) 2012 Samsung Electronics
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

TEST_F(EWK2UnitTestBase, ewk_settings_fullscreen_enabled)
{
    Ewk_Settings* settings = ewk_view_settings_get(webView());

#if ENABLE(FULLSCREEN_API)
    ASSERT_TRUE(ewk_settings_fullscreen_enabled_get(settings));

    ASSERT_TRUE(ewk_settings_fullscreen_enabled_set(settings, EINA_TRUE));
    ASSERT_TRUE(ewk_settings_fullscreen_enabled_get(settings));

    ASSERT_TRUE(ewk_settings_fullscreen_enabled_set(settings, EINA_FALSE));
    ASSERT_FALSE(ewk_settings_fullscreen_enabled_get(settings));
#else
    ASSERT_FALSE(ewk_settings_fullscreen_enabled_get(settings));

    ASSERT_FALSE(ewk_settings_fullscreen_enabled_set(settings, EINA_TRUE));
    ASSERT_FALSE(ewk_settings_fullscreen_enabled_get(settings));

    ASSERT_FALSE(ewk_settings_fullscreen_enabled_set(settings, EINA_FALSE));
    ASSERT_FALSE(ewk_settings_fullscreen_enabled_get(settings));
#endif
}

TEST_F(EWK2UnitTestBase, ewk_settings_javascript_enabled)
{
    Ewk_Settings* settings = ewk_view_settings_get(webView());

    ASSERT_TRUE(ewk_settings_javascript_enabled_set(settings, EINA_TRUE));
    ASSERT_TRUE(ewk_settings_javascript_enabled_get(settings));

    ASSERT_TRUE(ewk_settings_javascript_enabled_set(settings, 2));
    ASSERT_TRUE(ewk_settings_javascript_enabled_get(settings));

    ASSERT_TRUE(ewk_settings_javascript_enabled_set(settings, EINA_FALSE));
    ASSERT_FALSE(ewk_settings_javascript_enabled_get(settings));
}

TEST_F(EWK2UnitTestBase, ewk_settings_loads_images_automatically)
{
    Ewk_Settings* settings = ewk_view_settings_get(webView());

    ASSERT_TRUE(ewk_settings_loads_images_automatically_set(settings, EINA_TRUE));
    ASSERT_TRUE(ewk_settings_loads_images_automatically_get(settings));

    ASSERT_TRUE(ewk_settings_loads_images_automatically_set(settings, 2));
    ASSERT_TRUE(ewk_settings_loads_images_automatically_get(settings));

    ASSERT_TRUE(ewk_settings_loads_images_automatically_set(settings, EINA_FALSE));
    ASSERT_FALSE(ewk_settings_loads_images_automatically_get(settings));
}

TEST_F(EWK2UnitTestBase, ewk_settings_developer_extras_enabled)
{
    Ewk_Settings* settings = ewk_view_settings_get(webView());

    ASSERT_TRUE(ewk_settings_developer_extras_enabled_set(settings, EINA_TRUE));
    ASSERT_TRUE(ewk_settings_developer_extras_enabled_get(settings));

    ASSERT_TRUE(ewk_settings_developer_extras_enabled_set(settings, 2));
    ASSERT_TRUE(ewk_settings_developer_extras_enabled_get(settings));

    ASSERT_TRUE(ewk_settings_developer_extras_enabled_set(settings, EINA_FALSE));
    ASSERT_FALSE(ewk_settings_developer_extras_enabled_get(settings));
}

TEST_F(EWK2UnitTestBase, ewk_settings_file_access_from_file_urls_allowed)
{
    CString testURL = environment->urlForResource("local_file_access.html");
    Ewk_Settings* settings = ewk_view_settings_get(webView());

    ASSERT_FALSE(ewk_settings_file_access_from_file_urls_allowed_get(settings));

    ASSERT_TRUE(ewk_settings_file_access_from_file_urls_allowed_set(settings, true));
    ASSERT_TRUE(ewk_settings_file_access_from_file_urls_allowed_get(settings));

    // Check that file access from file:// URLs is allowed.
    ewk_view_url_set(webView(), testURL.data());
    ASSERT_TRUE(waitUntilTitleChangedTo("Frame loaded"));

    ASSERT_TRUE(ewk_settings_file_access_from_file_urls_allowed_set(settings, false));
    ASSERT_FALSE(ewk_settings_file_access_from_file_urls_allowed_get(settings));

    // Check that file access from file:// URLs is NOT allowed.
    ewk_view_url_set(webView(), testURL.data());
    ASSERT_TRUE(waitUntilTitleChangedTo("Frame NOT loaded"));
}

TEST_F(EWK2UnitTestBase, ewk_settings_frame_flattening_enabled_set)
{
    Ewk_Settings* settings = ewk_view_settings_get(webView());
    ASSERT_TRUE(settings);

    // The frame flattening is disabled by default.
    ASSERT_FALSE(ewk_settings_frame_flattening_enabled_get(settings));
    ewk_view_url_set(webView(), environment->urlForResource("frame_flattening_test.html").data());
    waitUntilTitleChangedTo("200"); // width of iframe tag.
    ASSERT_STREQ("200", ewk_view_title_get(webView()));

    ASSERT_TRUE(ewk_settings_frame_flattening_enabled_set(settings, true));
    ASSERT_TRUE(ewk_settings_frame_flattening_enabled_get(settings));
    ewk_view_url_set(webView(), environment->urlForResource("frame_flattening_test.html").data());
    waitUntilTitleChangedTo("600"); // width of frame_flattening_test_subframe.html
    ASSERT_STREQ("600", ewk_view_title_get(webView()));

    ASSERT_TRUE(ewk_settings_frame_flattening_enabled_set(settings, false));
    ASSERT_FALSE(ewk_settings_frame_flattening_enabled_get(settings));
    ewk_view_url_set(webView(), environment->urlForResource("frame_flattening_test.html").data());
    waitUntilTitleChangedTo("200"); // width of iframe tag.
    ASSERT_STREQ("200", ewk_view_title_get(webView()));
}

TEST_F(EWK2UnitTestBase, ewk_settings_dns_prefetching_enabled)
{
    Ewk_Settings* settings = ewk_view_settings_get(webView());

    // DNS prefeching is disabled by default.
    ASSERT_FALSE(ewk_settings_dns_prefetching_enabled_get(settings));
    ASSERT_TRUE(ewk_settings_dns_prefetching_enabled_set(settings, true));
    ASSERT_TRUE(ewk_settings_dns_prefetching_enabled_get(settings));

    ASSERT_TRUE(ewk_settings_dns_prefetching_enabled_set(settings, false));
    ASSERT_FALSE(ewk_settings_dns_prefetching_enabled_get(settings));
}

TEST_F(EWK2UnitTestBase, ewk_settings_encoding_detector_enabled)
{
    Ewk_Settings* settings = ewk_view_settings_get(webView());

    // The encoding detector is disabled by default.
    ASSERT_FALSE(ewk_settings_encoding_detector_enabled_get(settings));

    ASSERT_TRUE(ewk_settings_encoding_detector_enabled_set(settings, true));
    ASSERT_TRUE(ewk_settings_encoding_detector_enabled_get(settings));

    ASSERT_TRUE(ewk_settings_encoding_detector_enabled_set(settings, false));
    ASSERT_FALSE(ewk_settings_encoding_detector_enabled_get(settings));
}

TEST_F(EWK2UnitTestBase, ewk_settings_preferred_minimum_contents_width)
{
    Ewk_Settings* settings = ewk_view_settings_get(webView());

    // 980 by default.
    ASSERT_EQ(980, ewk_settings_preferred_minimum_contents_width_get(settings));

    ASSERT_TRUE(ewk_settings_preferred_minimum_contents_width_set(settings, 0));
    ASSERT_EQ(0, ewk_settings_preferred_minimum_contents_width_get(settings));
}

TEST_F(EWK2UnitTestBase, ewk_settings_offline_web_application_cache_enabled)
{
    Ewk_Settings* settings = ewk_view_settings_get(webView());

    // The offline web application cache is enabled by default.
    ASSERT_TRUE(ewk_settings_offline_web_application_cache_enabled_get(settings));

    ASSERT_TRUE(ewk_settings_offline_web_application_cache_enabled_set(settings, true));
    ASSERT_TRUE(ewk_settings_offline_web_application_cache_enabled_get(settings));

    ASSERT_TRUE(ewk_settings_offline_web_application_cache_enabled_set(settings, false));
    ASSERT_FALSE(ewk_settings_offline_web_application_cache_enabled_get(settings));
}

TEST_F(EWK2UnitTestBase, ewk_settings_scripts_can_open_windows)
{
    Ewk_Settings* settings = ewk_view_settings_get(webView());

    // The scripts can open new windows by default.
    ASSERT_TRUE(ewk_settings_scripts_can_open_windows_get(settings));

    ASSERT_TRUE(ewk_settings_scripts_can_open_windows_set(settings, true));
    ASSERT_TRUE(ewk_settings_scripts_can_open_windows_get(settings));

    ASSERT_TRUE(ewk_settings_scripts_can_open_windows_set(settings, false));
    ASSERT_FALSE(ewk_settings_scripts_can_open_windows_get(settings));
}

TEST_F(EWK2UnitTestBase, ewk_settings_local_storage_enabled)
{
    Ewk_Settings* settings = ewk_view_settings_get(webView());

    // HTML5 local storage should be enabled by default.
    ASSERT_TRUE(ewk_settings_local_storage_enabled_get(settings));

    ASSERT_TRUE(ewk_settings_local_storage_enabled_set(settings, false));
    ASSERT_FALSE(ewk_settings_local_storage_enabled_get(settings));

    ASSERT_TRUE(ewk_settings_local_storage_enabled_set(settings, true));
    ASSERT_TRUE(ewk_settings_local_storage_enabled_get(settings));
}

TEST_F(EWK2UnitTestBase, ewk_settings_plugins_enabled)
{
    Ewk_Settings* settings = ewk_view_settings_get(webView());

    // Plug-ins support is enabled by default.
    ASSERT_TRUE(ewk_settings_plugins_enabled_get(settings));

    ASSERT_TRUE(ewk_settings_plugins_enabled_set(settings, true));
    ASSERT_TRUE(ewk_settings_plugins_enabled_get(settings));

    ASSERT_TRUE(ewk_settings_plugins_enabled_set(settings, false));
    ASSERT_FALSE(ewk_settings_plugins_enabled_get(settings));
}

TEST_F(EWK2UnitTestBase, ewk_settings_default_font_size)
{
    Ewk_Settings* settings = ewk_view_settings_get(webView());

    // 16 by default.
    ASSERT_EQ(16, ewk_settings_default_font_size_get(settings));

    ASSERT_TRUE(ewk_settings_default_font_size_set(settings, 10));
    ASSERT_EQ(10, ewk_settings_default_font_size_get(settings));

    ASSERT_TRUE(ewk_settings_default_font_size_set(settings, 20));
    ASSERT_EQ(20, ewk_settings_default_font_size_get(settings));
}

TEST_F(EWK2UnitTestBase, ewk_settings_private_browsing_enabled)
{
    Ewk_Settings* settings = ewk_view_settings_get(webView());

    // Private browsing is disabled by default.
    ASSERT_FALSE(ewk_settings_private_browsing_enabled_get(settings));

    ASSERT_TRUE(ewk_settings_private_browsing_enabled_set(settings, true));
    ASSERT_TRUE(ewk_settings_private_browsing_enabled_get(settings));

    ASSERT_TRUE(ewk_settings_private_browsing_enabled_set(settings, false));
    ASSERT_FALSE(ewk_settings_private_browsing_enabled_get(settings));
}

TEST_F(EWK2UnitTestBase, ewk_settings_text_autosizing_enabled)
{
    Ewk_Settings* settings = ewk_view_settings_get(webView());

#if ENABLE(TEXT_AUTOSIZING)
    // Text autosizing should be disabled by default.
    ASSERT_FALSE(ewk_settings_text_autosizing_enabled_get(settings));

    ASSERT_TRUE(ewk_settings_text_autosizing_enabled_set(settings, false));
    ASSERT_FALSE(ewk_settings_text_autosizing_enabled_get(settings));

    ASSERT_TRUE(ewk_settings_text_autosizing_enabled_set(settings, true));
    ASSERT_TRUE(ewk_settings_text_autosizing_enabled_get(settings));
#else
    ASSERT_FALSE(ewk_settings_text_autosizing_enabled_get(settings));

    ASSERT_FALSE(ewk_settings_text_autosizing_enabled_set(settings, false));
    ASSERT_FALSE(ewk_settings_text_autosizing_enabled_get(settings));

    ASSERT_FALSE(ewk_settings_text_autosizing_enabled_set(settings, true));
    ASSERT_FALSE(ewk_settings_text_autosizing_enabled_get(settings));
#endif
}
