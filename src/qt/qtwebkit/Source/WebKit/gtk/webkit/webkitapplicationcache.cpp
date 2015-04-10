/*
 * Copyright (C) 2009 Jan Michael Alonzo <jmalonzo@gmail.com>
 * Copyright (C) 2011 Lukasz Slachciak
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
#include "webkitapplicationcache.h"

#include "ApplicationCacheStorage.h"
#include "FileSystem.h"
#include <wtf/text/CString.h>

// web application cache maximum storage size
static unsigned long long cacheMaxSize = UINT_MAX;

/**
 * webkit_application_cache_get_maximum_size:
 *
 * Returns the maximum size of the cache storage.
 * By default it is set to UINT_MAX i.e. no quota.
 *
 * Returns: the current application cache maximum storage size
 *
 * Since: 1.3.13
 **/
unsigned long long webkit_application_cache_get_maximum_size()
{
    return (cacheMaxSize = WebCore::cacheStorage().maximumSize());
}

/**
 * webkit_application_cache_set_maximum_size:
 * @size: the new web application cache maximum storage size
 *
 * Sets new application cache maximum storage size.
 * Changing the application cache storage size will clear the cache
 * and rebuild cache storage.
 *
 * Since: 1.3.13
 **/
void webkit_application_cache_set_maximum_size(unsigned long long size)
{
    if (size != cacheMaxSize) {
        WebCore::cacheStorage().empty();
        WebCore::cacheStorage().vacuumDatabaseFile();
        WebCore::cacheStorage().setMaximumSize(size);
        cacheMaxSize = size;
    }
}

/**
 * webkit_application_cache_get_database_directory_path:
 *
 * Returns the path to the directory WebKit will write web application
 * cache databases to. By default this path is set to
 * $XDG_CACHE_HOME/webkitgtk/applications and cannot be modified.
 *
 * Returns: the application cache database directory path
 *
 * Since: 1.3.13
 **/
const gchar* webkit_application_cache_get_database_directory_path()
{
    CString path = WebCore::fileSystemRepresentation(WebCore::cacheStorage().cacheDirectory());
    return path.data();
}

