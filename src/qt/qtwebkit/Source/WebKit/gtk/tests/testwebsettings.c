/*
 * Copyright (C) 2009 Jan Michael Alonzo
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

#include "autotoolsconfig.h"
#include <glib.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>

/* Private API */
char* webkitWebSettingsUserAgentForURI(WebKitWebSettings *settings, const char *uri);

static void test_webkit_web_settings_copy(void)
{
    WebKitWebSettings *settings = webkit_web_settings_new();

    // Set some non-default settings to verify that settings are properly copied.
    g_object_set(settings,
                 "enable-webgl", TRUE,
                 "enable-fullscreen", TRUE,
                 "auto-load-images", FALSE,
                 "default-encoding", "utf-8", NULL);

    WebKitWebSettings *copy = webkit_web_settings_copy(settings);

    gboolean enableWebGL = FALSE;
    gboolean enableFullscreen = FALSE;
    gboolean autoLoadImages = FALSE;
    char *defaultEncoding = 0;
    g_object_get(copy,
                 "enable-fullscreen", &enableFullscreen,
                 "enable-webgl", &enableWebGL,
                 "auto-load-images", &autoLoadImages,
                 "default-encoding", &defaultEncoding, NULL);

    g_assert(enableWebGL);
    g_assert(enableFullscreen);
    g_assert(!autoLoadImages);
    g_assert_cmpstr(defaultEncoding, ==, "utf-8");
    g_free(defaultEncoding);
}

static void test_non_quirky_user_agents(WebKitWebSettings *settings, const char *defaultUserAgent)
{
    char *userAgent = 0;

    // test a custom UA string
    userAgent = 0;
    g_object_set(settings, "user-agent", "testwebsettings/0.1", NULL);
    g_object_get(settings,"user-agent", &userAgent, NULL);
    g_assert_cmpstr(userAgent, ==, "testwebsettings/0.1");
    g_free(userAgent);

    // setting it to NULL or an empty value should give us the default UA string
    userAgent = 0;
    g_object_set(settings, "user-agent", 0, NULL);
    g_object_get(settings,"user-agent", &userAgent, NULL);
    g_assert_cmpstr(userAgent, ==, defaultUserAgent);
    g_free(userAgent);

    userAgent = 0;
    g_object_set(settings, "user-agent", "", NULL);
    g_object_get(settings,"user-agent", &userAgent, NULL);
    g_assert_cmpstr(userAgent, ==, defaultUserAgent);
    g_free(userAgent);
}

static void test_webkit_web_settings_user_agent(void)
{
    WebKitWebSettings *settings;
    GtkWidget *webView;
    char *defaultUserAgent;
    char *userAgent = 0;
    g_test_bug("17375");

    webView = webkit_web_view_new();
    g_object_ref_sink(webView);

    settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webView));
    defaultUserAgent = g_strdup(webkit_web_settings_get_user_agent(settings));

    test_non_quirky_user_agents(settings, defaultUserAgent);

    /* Test quirky google domains */
    g_object_set(settings, "user-agent", "testwebsettings/0.1", NULL);

    userAgent = webkitWebSettingsUserAgentForURI(settings, "http://www.google.com/");
    g_assert_cmpstr(userAgent, ==, "testwebsettings/0.1");
    g_free(userAgent);

    userAgent = webkitWebSettingsUserAgentForURI(settings, "http://gmail.com/");
    g_assert_cmpstr(userAgent, ==, "testwebsettings/0.1");
    g_free(userAgent);

    userAgent = webkitWebSettingsUserAgentForURI(settings, "http://www.google.com.br/");
    g_assert_cmpstr(userAgent, ==, "testwebsettings/0.1");
    g_free(userAgent);

    userAgent = webkitWebSettingsUserAgentForURI(settings, "http://calendar.google.com/");
    g_assert_cmpstr(userAgent, ==, "testwebsettings/0.1");
    g_free(userAgent);

    /* Now enable quirks handling */
    g_object_set(settings, "enable-site-specific-quirks", TRUE, NULL);

    test_non_quirky_user_agents(settings, defaultUserAgent);

    g_object_set(settings, "user-agent", "testwebsettings/0.1", NULL);

    userAgent = webkitWebSettingsUserAgentForURI(settings, "http://www.google.com/");
    g_assert_cmpstr(userAgent, ==, defaultUserAgent);
    g_free(userAgent);

    userAgent = webkitWebSettingsUserAgentForURI(settings, "http://gmail.com/");
    g_assert_cmpstr(userAgent, ==, defaultUserAgent);
    g_free(userAgent);

    userAgent = webkitWebSettingsUserAgentForURI(settings, "http://www.google.com.br/");
    g_assert_cmpstr(userAgent, ==, defaultUserAgent);
    g_free(userAgent);

    userAgent = webkitWebSettingsUserAgentForURI(settings, "http://www.google.uk.not.com.br/");
    g_assert_cmpstr(userAgent, ==, "testwebsettings/0.1");
    g_free(userAgent);

    g_free(defaultUserAgent);
    g_object_unref(webView);
}

int main(int argc, char **argv)
{
    gtk_test_init(&argc, &argv, NULL);

    g_test_bug_base("https://bugs.webkit.org/");
    g_test_add_func("/webkit/websettings/user_agent", test_webkit_web_settings_user_agent);
    g_test_add_func("/webkit/websettings/copy", test_webkit_web_settings_copy);
    return g_test_run ();
}
