/*
 * Copyright (C) 2011 Igalia S.L.
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

#include "config.h"
#include "TestMain.h"

#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <wtf/gobject/GOwnPtr.h>

void beforeAll();
void afterAll();

static void registerGResource(void)
{
    GOwnPtr<char> resourcesPath(g_build_filename(WEBKIT_EXEC_PATH, "resources", "webkit2gtk-tests-resources.gresource", NULL));
    GResource* resource = g_resource_load(resourcesPath.get(), 0);
    g_assert(resource);

    g_resources_register(resource);
    g_resource_unref(resource);
}

static void removeNonEmptyDirectory(const char* directoryPath)
{
    GDir* directory = g_dir_open(directoryPath, 0, 0);
    g_assert(directory);
    const char* fileName;
    while ((fileName = g_dir_read_name(directory))) {
        GOwnPtr<char> filePath(g_build_filename(directoryPath, fileName, NULL));
        g_unlink(filePath.get());
    }
    g_dir_close(directory);
    g_rmdir(directoryPath);
}

int main(int argc, char** argv)
{
    gtk_test_init(&argc, &argv, 0);
    g_setenv("WEBKIT_EXEC_PATH", WEBKIT_EXEC_PATH, FALSE);
    g_setenv("WEBKIT_INJECTED_BUNDLE_PATH", WEBKIT_INJECTED_BUNDLE_PATH, FALSE);
    g_setenv("LC_ALL", "C", TRUE);
    g_test_bug_base("https://bugs.webkit.org/");

    registerGResource();

    GOwnPtr<char> diskCacheTempDirectory(g_dir_make_tmp("WebKit2TestsDiskCache-XXXXXX", 0));
    g_assert(diskCacheTempDirectory.get());
    webkit_web_context_set_disk_cache_directory(webkit_web_context_get_default(), diskCacheTempDirectory.get());

    beforeAll();
    int returnValue = g_test_run();
    afterAll();

    removeNonEmptyDirectory(diskCacheTempDirectory.get());

    return returnValue;
}
