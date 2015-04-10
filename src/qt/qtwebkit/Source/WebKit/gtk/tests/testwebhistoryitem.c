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

typedef struct {
    WebKitWebHistoryItem* item;
} WebHistoryItemFixture;

static void web_history_item_fixture_setup(WebHistoryItemFixture* fixture,
                                           gconstpointer data)
{
    fixture->item = webkit_web_history_item_new_with_data("http://example.com/", "Example1");
    g_assert_cmpint(G_OBJECT(fixture->item)->ref_count, == , 1);
    g_assert(fixture->item != NULL);
}

static void web_history_item_fixture_teardown(WebHistoryItemFixture* fixture,
                                              gconstpointer data)
{
    g_assert(fixture->item != NULL);
    g_assert_cmpint(G_OBJECT(fixture->item)->ref_count, ==, 1);
}

static void test_webkit_web_history_item_get_data(WebHistoryItemFixture* fixture,
                                                  gconstpointer data)
{
    g_assert_cmpstr(webkit_web_history_item_get_title(fixture->item), ==, "Example1");
    g_assert_cmpstr(webkit_web_history_item_get_uri(fixture->item), ==, "http://example.com/");
}

static void test_webkit_web_history_item_alternate_title(WebHistoryItemFixture* fixture,
                                                         gconstpointer data)
{
    webkit_web_history_item_set_alternate_title(fixture->item, "Alternate title");
    g_assert_cmpstr(webkit_web_history_item_get_alternate_title(fixture->item), ==, "Alternate title");
}

int main(int argc, char** argv)
{
    gtk_test_init(&argc, &argv, NULL);

    g_test_bug_base("https://bugs.webkit.org/");
    g_test_add("/webkit/webhistoryitem/get_data",
               WebHistoryItemFixture, 0, web_history_item_fixture_setup,
               test_webkit_web_history_item_get_data, web_history_item_fixture_teardown);
    g_test_add("/webkit/webhistoryitem/alternate_title",
               WebHistoryItemFixture, 0, web_history_item_fixture_setup,
               test_webkit_web_history_item_alternate_title, web_history_item_fixture_teardown);
    return g_test_run ();
}
