/*
 * Copyright (C) 2010 Arno Renevier
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

#include "test_utils.h"

#include <glib.h>
#include <glib/gstdio.h>

int testutils_relative_chdir(const gchar *targetFilename, const gchar *executablePath)
{
    /* user can set location of the webkit repository directory if it differs from build directory */
    const gchar *repoPath = g_getenv("WEBKITREPODIR");
    if (repoPath) {
        if (g_chdir(repoPath))
            return -1;
    } else if (g_chdir(g_path_get_dirname(executablePath)))
            return -1;

    while (!g_file_test(targetFilename, G_FILE_TEST_EXISTS)) {
        gchar *pathName;
        if (g_chdir(".."))
            return -1;
        g_assert(!g_str_equal((pathName = g_get_current_dir()), "/"));
        g_free(pathName);
    }

    gchar *dirName = g_path_get_dirname(targetFilename);
    if (g_chdir(dirName)) {
        g_free(dirName);
        return -1;
    }

    g_free(dirName);
    return 0;
}
