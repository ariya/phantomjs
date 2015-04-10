/*
 * Copyright (C) 2012 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2,1 of the License, or (at your option) any later version.
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

#include "config.h"

#include "TestMain.h"
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>


static void testWebKitVersion(Test*, gconstpointer)
{
    g_assert_cmpuint(webkit_get_major_version(), ==, WEBKIT_MAJOR_VERSION);
    g_assert_cmpuint(webkit_get_minor_version(), ==, WEBKIT_MINOR_VERSION);
    g_assert_cmpuint(webkit_get_micro_version(), ==, WEBKIT_MICRO_VERSION);
}

static void testWebKitCheckVersion(Test*, gconstpointer)
{
    g_assert(WEBKIT_CHECK_VERSION(WEBKIT_MAJOR_VERSION, WEBKIT_MINOR_VERSION, WEBKIT_MICRO_VERSION));
    g_assert(!WEBKIT_CHECK_VERSION(WEBKIT_MAJOR_VERSION + 1, WEBKIT_MINOR_VERSION, WEBKIT_MICRO_VERSION));
    g_assert(!WEBKIT_CHECK_VERSION(WEBKIT_MAJOR_VERSION, WEBKIT_MINOR_VERSION + 1, WEBKIT_MICRO_VERSION));
    g_assert(!WEBKIT_CHECK_VERSION(WEBKIT_MAJOR_VERSION, WEBKIT_MINOR_VERSION, WEBKIT_MICRO_VERSION + 1));
}

void beforeAll()
{
    Test::add("WebKitVersion", "version", testWebKitVersion);
    Test::add("WebKitVersion", "check-version", testWebKitCheckVersion);
}

void afterAll()
{
}
