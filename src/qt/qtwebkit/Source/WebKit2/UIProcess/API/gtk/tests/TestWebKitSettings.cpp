/*
 * Copyright (c) 2011 Motorola Mobility, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer in the documentation and/or 
 * other materials provided with the distribution.
 *
 * Neither the name of Motorola Mobility, Inc. nor the names of its contributors may 
 * be used to endorse or promote products derived from this software without 
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "TestMain.h"
#include "WebViewTest.h"
#include "WebKitTestServer.h"
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <wtf/gobject/GRefPtr.h>

static WebKitTestServer* gServer;

static void testWebKitSettings(Test*, gconstpointer)
{
    WebKitSettings* settings = webkit_settings_new();

    // JavaScript is enabled by default.
    g_assert(webkit_settings_get_enable_javascript(settings));
    webkit_settings_set_enable_javascript(settings, FALSE);
    g_assert(!webkit_settings_get_enable_javascript(settings));

    // By default auto-load-image is true.
    g_assert(webkit_settings_get_auto_load_images(settings));
    webkit_settings_set_auto_load_images(settings, FALSE);
    g_assert(!webkit_settings_get_auto_load_images(settings));

    // load-icons-ignoring-image-load-setting is false by default.
    g_assert(!webkit_settings_get_load_icons_ignoring_image_load_setting(settings));
    webkit_settings_set_load_icons_ignoring_image_load_setting(settings, TRUE);
    g_assert(webkit_settings_get_load_icons_ignoring_image_load_setting(settings));
    
    // Offline application cache is true by default.
    g_assert(webkit_settings_get_enable_offline_web_application_cache(settings));
    webkit_settings_set_enable_offline_web_application_cache(settings, FALSE);
    g_assert(!webkit_settings_get_enable_offline_web_application_cache(settings));

    // Local storage is enable by default.
    g_assert(webkit_settings_get_enable_html5_local_storage(settings));
    webkit_settings_set_enable_html5_local_storage(settings, FALSE);
    g_assert(!webkit_settings_get_enable_html5_local_storage(settings));

    // HTML5 database is enabled by default.
    g_assert(webkit_settings_get_enable_html5_database(settings));
    webkit_settings_set_enable_html5_database(settings, FALSE);
    g_assert(!webkit_settings_get_enable_html5_database(settings));

    // XSS Auditor is enabled by default.
    g_assert(webkit_settings_get_enable_xss_auditor(settings));
    webkit_settings_set_enable_xss_auditor(settings, FALSE);
    g_assert(!webkit_settings_get_enable_xss_auditor(settings));

    // Frame flattening is disabled by default.
    g_assert(!webkit_settings_get_enable_frame_flattening(settings));
    webkit_settings_set_enable_frame_flattening(settings, TRUE);
    g_assert(webkit_settings_get_enable_frame_flattening(settings));

    // Plugins are enabled by default.
    g_assert(webkit_settings_get_enable_plugins(settings));
    webkit_settings_set_enable_plugins(settings, FALSE);
    g_assert(!webkit_settings_get_enable_plugins(settings));

    // Java is enabled by default.
    g_assert(webkit_settings_get_enable_java(settings));
    webkit_settings_set_enable_java(settings, FALSE);
    g_assert(!webkit_settings_get_enable_java(settings));

    // By default, JavaScript can open windows automatically is disabled.
    g_assert(!webkit_settings_get_javascript_can_open_windows_automatically(settings));
    webkit_settings_set_javascript_can_open_windows_automatically(settings, TRUE);
    g_assert(webkit_settings_get_javascript_can_open_windows_automatically(settings));

    // By default hyper link auditing is disabled.
    g_assert(!webkit_settings_get_enable_hyperlink_auditing(settings));
    webkit_settings_set_enable_hyperlink_auditing(settings, TRUE);
    g_assert(webkit_settings_get_enable_hyperlink_auditing(settings));

    // Default font family is "sans-serif".
    g_assert_cmpstr(webkit_settings_get_default_font_family(settings), ==, "sans-serif");
    webkit_settings_set_default_font_family(settings, "monospace");
    g_assert_cmpstr(webkit_settings_get_default_font_family(settings), ==, "monospace");

    // Default monospace font family font family is "monospace".
    g_assert_cmpstr(webkit_settings_get_monospace_font_family(settings), ==, "monospace");
    webkit_settings_set_monospace_font_family(settings, "sans-serif");
    g_assert_cmpstr(webkit_settings_get_monospace_font_family(settings), ==, "sans-serif");

    // Default serif font family is "serif".
    g_assert_cmpstr(webkit_settings_get_serif_font_family(settings), ==, "serif");
    webkit_settings_set_serif_font_family(settings, "sans-serif");
    g_assert_cmpstr(webkit_settings_get_serif_font_family(settings), ==, "sans-serif");

    // Default sans serif font family is "sans-serif".
    g_assert_cmpstr(webkit_settings_get_sans_serif_font_family(settings), ==, "sans-serif");
    webkit_settings_set_sans_serif_font_family(settings, "serif");
    g_assert_cmpstr(webkit_settings_get_sans_serif_font_family(settings), ==, "serif");

    // Default cursive font family "serif".
    g_assert_cmpstr(webkit_settings_get_cursive_font_family(settings), ==, "serif");
    webkit_settings_set_cursive_font_family(settings, "sans-serif");
    g_assert_cmpstr(webkit_settings_get_cursive_font_family(settings), ==, "sans-serif");

    // Default fantasy font family is "serif".
    g_assert_cmpstr(webkit_settings_get_fantasy_font_family(settings), ==, "serif");
    webkit_settings_set_fantasy_font_family(settings, "sans-serif");
    g_assert_cmpstr(webkit_settings_get_fantasy_font_family(settings), ==, "sans-serif");

    // Default pictograph font family is "serif".
    g_assert_cmpstr(webkit_settings_get_pictograph_font_family(settings), ==, "serif");
    webkit_settings_set_pictograph_font_family(settings, "sans-serif");
    g_assert_cmpstr(webkit_settings_get_pictograph_font_family(settings), ==, "sans-serif");

    // Default font size is 16.
    g_assert_cmpuint(webkit_settings_get_default_font_size(settings), ==, 16);
    webkit_settings_set_default_font_size(settings, 14);
    g_assert_cmpuint(webkit_settings_get_default_font_size(settings), ==, 14);

    // Default monospace font size is 13.
    g_assert_cmpuint(webkit_settings_get_default_monospace_font_size(settings), ==, 13);
    webkit_settings_set_default_monospace_font_size(settings, 10);
    g_assert_cmpuint(webkit_settings_get_default_monospace_font_size(settings), ==, 10);

    // Default minimum font size is 0.
    g_assert_cmpuint(webkit_settings_get_minimum_font_size(settings), ==, 0);
    webkit_settings_set_minimum_font_size(settings, 7);
    g_assert_cmpuint(webkit_settings_get_minimum_font_size(settings), ==, 7);

    // Default charset is "iso-8859-1".
    g_assert_cmpstr(webkit_settings_get_default_charset(settings), ==, "iso-8859-1");
    webkit_settings_set_default_charset(settings, "utf8");
    g_assert_cmpstr(webkit_settings_get_default_charset(settings), ==, "utf8");

    g_assert(!webkit_settings_get_enable_private_browsing(settings));
    webkit_settings_set_enable_private_browsing(settings, TRUE);
    g_assert(webkit_settings_get_enable_private_browsing(settings));

    g_assert(!webkit_settings_get_enable_developer_extras(settings));
    webkit_settings_set_enable_developer_extras(settings, TRUE);
    g_assert(webkit_settings_get_enable_developer_extras(settings));

    g_assert(webkit_settings_get_enable_resizable_text_areas(settings));
    webkit_settings_set_enable_resizable_text_areas(settings, FALSE);
    g_assert(!webkit_settings_get_enable_resizable_text_areas(settings));

    g_assert(webkit_settings_get_enable_tabs_to_links(settings));
    webkit_settings_set_enable_tabs_to_links(settings, FALSE);
    g_assert(!webkit_settings_get_enable_tabs_to_links(settings));

    g_assert(!webkit_settings_get_enable_dns_prefetching(settings));
    webkit_settings_set_enable_dns_prefetching(settings, TRUE);
    g_assert(webkit_settings_get_enable_dns_prefetching(settings));

    // Caret browsing is disabled by default.
    g_assert(!webkit_settings_get_enable_caret_browsing(settings));
    webkit_settings_set_enable_caret_browsing(settings, TRUE);
    g_assert(webkit_settings_get_enable_caret_browsing(settings));

    // Fullscreen JavaScript API is disabled by default.
    g_assert(!webkit_settings_get_enable_fullscreen(settings));
    webkit_settings_set_enable_fullscreen(settings, TRUE);
    g_assert(webkit_settings_get_enable_fullscreen(settings));

    // Print backgrounds is enabled by default
    g_assert(webkit_settings_get_print_backgrounds(settings));
    webkit_settings_set_print_backgrounds(settings, FALSE);
    g_assert(!webkit_settings_get_print_backgrounds(settings));

    // WebAudio is disabled by default.
    g_assert(!webkit_settings_get_enable_webaudio(settings));
    webkit_settings_set_enable_webaudio(settings, TRUE);
    g_assert(webkit_settings_get_enable_webaudio(settings));

    // WebGL is disabled by default.
    g_assert(!webkit_settings_get_enable_webgl(settings));
    webkit_settings_set_enable_webgl(settings, TRUE);
    g_assert(webkit_settings_get_enable_webgl(settings));

    // Allow Modal Dialogs is disabled by default.
    g_assert(!webkit_settings_get_allow_modal_dialogs(settings));
    webkit_settings_set_allow_modal_dialogs(settings, TRUE);
    g_assert(webkit_settings_get_allow_modal_dialogs(settings));

    // Zoom text only is disabled by default.
    g_assert(!webkit_settings_get_zoom_text_only(settings));
    webkit_settings_set_zoom_text_only(settings, TRUE);
    g_assert(webkit_settings_get_zoom_text_only(settings));

    // By default, JavaScript cannot access the clipboard.
    g_assert(!webkit_settings_get_javascript_can_access_clipboard(settings));
    webkit_settings_set_javascript_can_access_clipboard(settings, TRUE);
    g_assert(webkit_settings_get_javascript_can_access_clipboard(settings));

    // By default, media playback doesn't require user gestures.
    g_assert(!webkit_settings_get_media_playback_requires_user_gesture(settings));
    webkit_settings_set_media_playback_requires_user_gesture(settings, TRUE);
    g_assert(webkit_settings_get_media_playback_requires_user_gesture(settings));

    // By default, inline media playback is allowed
    g_assert(webkit_settings_get_media_playback_allows_inline(settings));
    webkit_settings_set_media_playback_allows_inline(settings, FALSE);
    g_assert(!webkit_settings_get_media_playback_allows_inline(settings));

    // By default, debug indicators are disabled.
    g_assert(!webkit_settings_get_draw_compositing_indicators(settings));
    webkit_settings_set_draw_compositing_indicators(settings, TRUE);
    g_assert(webkit_settings_get_draw_compositing_indicators(settings));

    // By default, site specific quirks are disabled.
    g_assert(!webkit_settings_get_enable_site_specific_quirks(settings));
    webkit_settings_set_enable_site_specific_quirks(settings, TRUE);
    g_assert(webkit_settings_get_enable_site_specific_quirks(settings));

    // By default, page cache is enabled.
    g_assert(webkit_settings_get_enable_page_cache(settings));
    webkit_settings_set_enable_page_cache(settings, FALSE);
    g_assert(!webkit_settings_get_enable_page_cache(settings));

    // By default, smooth scrolling is disabled.
    g_assert(!webkit_settings_get_enable_smooth_scrolling(settings));
    webkit_settings_set_enable_smooth_scrolling(settings, TRUE);
    g_assert(webkit_settings_get_enable_smooth_scrolling(settings));

    // By default, accelerated 2D canvas is disabled.
    g_assert(!webkit_settings_get_enable_accelerated_2d_canvas(settings));
    webkit_settings_set_enable_accelerated_2d_canvas(settings, TRUE);
    g_assert(webkit_settings_get_enable_accelerated_2d_canvas(settings));

    // By default, writing of console messages to stdout is disabled.
    g_assert(!webkit_settings_get_enable_write_console_messages_to_stdout(settings));
    webkit_settings_set_enable_write_console_messages_to_stdout(settings, TRUE);
    g_assert(webkit_settings_get_enable_write_console_messages_to_stdout(settings));

    g_object_unref(G_OBJECT(settings));
}

void testWebKitSettingsNewWithSettings(Test* test, gconstpointer)
{
    GRefPtr<WebKitSettings> settings = adoptGRef(webkit_settings_new_with_settings("enable-javascript", FALSE,
                                                                                   "auto-load-images", FALSE,
                                                                                   "load-icons-ignoring-image-load-setting", TRUE,
                                                                                   NULL));
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(settings.get()));
    g_assert(!webkit_settings_get_enable_javascript(settings.get()));
    g_assert(!webkit_settings_get_auto_load_images(settings.get()));
    g_assert(webkit_settings_get_load_icons_ignoring_image_load_setting(settings.get()));
}

static CString convertWebViewMainResourceDataToCString(WebViewTest* test)
{
    size_t mainResourceDataSize = 0;
    const char* mainResourceData = test->mainResourceData(mainResourceDataSize);
    return CString(mainResourceData, mainResourceDataSize);
}

static void assertThatUserAgentIsSentInHeaders(WebViewTest* test, const CString& userAgent)
{
    test->loadURI(gServer->getURIForPath("/").data());
    test->waitUntilLoadFinished();
    ASSERT_CMP_CSTRING(convertWebViewMainResourceDataToCString(test), ==, userAgent);
}

static void testWebKitSettingsUserAgent(WebViewTest* test, gconstpointer)
{
    GRefPtr<WebKitSettings> settings = adoptGRef(webkit_settings_new());
    CString defaultUserAgent = webkit_settings_get_user_agent(settings.get());
    webkit_web_view_set_settings(test->m_webView, settings.get());

    g_assert(g_strstr_len(defaultUserAgent.data(), -1, "Safari"));
    g_assert(g_strstr_len(defaultUserAgent.data(), -1, "Chromium"));
    g_assert(g_strstr_len(defaultUserAgent.data(), -1, "Chrome"));

    webkit_settings_set_user_agent(settings.get(), 0);
    g_assert_cmpstr(defaultUserAgent.data(), ==, webkit_settings_get_user_agent(settings.get()));
    assertThatUserAgentIsSentInHeaders(test, defaultUserAgent.data());

    webkit_settings_set_user_agent(settings.get(), "");
    g_assert_cmpstr(defaultUserAgent.data(), ==, webkit_settings_get_user_agent(settings.get()));

    const char* funkyUserAgent = "Funky!";
    webkit_settings_set_user_agent(settings.get(), funkyUserAgent);
    g_assert_cmpstr(funkyUserAgent, ==, webkit_settings_get_user_agent(settings.get()));
    assertThatUserAgentIsSentInHeaders(test, funkyUserAgent);

    webkit_settings_set_user_agent_with_application_details(settings.get(), "WebKitGTK+", 0);
    const char* userAgentWithNullVersion = webkit_settings_get_user_agent(settings.get());
    g_assert_cmpstr(g_strstr_len(userAgentWithNullVersion, -1, defaultUserAgent.data()), ==, userAgentWithNullVersion);
    g_assert(g_strstr_len(userAgentWithNullVersion, -1, "WebKitGTK+"));

    webkit_settings_set_user_agent_with_application_details(settings.get(), "WebKitGTK+", "");
    g_assert_cmpstr(webkit_settings_get_user_agent(settings.get()), ==, userAgentWithNullVersion);

    webkit_settings_set_user_agent_with_application_details(settings.get(), "WebCatGTK+", "3.4.5");
    const char* newUserAgent = webkit_settings_get_user_agent(settings.get());
    g_assert(g_strstr_len(newUserAgent, -1, "3.4.5"));
    g_assert(g_strstr_len(newUserAgent, -1, "WebCatGTK+"));
}

static void serverCallback(SoupServer* server, SoupMessage* message, const char* path, GHashTable*, SoupClientContext*, gpointer)
{
    if (message->method != SOUP_METHOD_GET) {
        soup_message_set_status(message, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    if (g_str_equal(path, "/")) {
        const char* userAgent = soup_message_headers_get_one(message->request_headers, "User-Agent");
        soup_message_set_status(message, SOUP_STATUS_OK);
        soup_message_body_append(message->response_body, SOUP_MEMORY_COPY, userAgent, strlen(userAgent));
        soup_message_body_complete(message->response_body);
    } else
        soup_message_set_status(message, SOUP_STATUS_NOT_FOUND);
}

void beforeAll()
{
    gServer = new WebKitTestServer();
    gServer->run(serverCallback);

    Test::add("WebKitSettings", "webkit-settings", testWebKitSettings);
    Test::add("WebKitSettings", "new-with-settings", testWebKitSettingsNewWithSettings);
    WebViewTest::add("WebKitSettings", "user-agent", testWebKitSettingsUserAgent);
}

void afterAll()
{
    delete gServer;
}

