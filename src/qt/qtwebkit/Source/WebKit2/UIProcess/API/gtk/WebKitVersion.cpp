/*
 * Copyright (C) 2012 Igalia S.L.
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
#include "WebKitVersion.h"

/**
 * SECTION: WebKitVersion
 * @Short_description: Provides the WebKit version
 * @Title: WebKitVersion
 *
 * Provides convenience functions returning WebKit's major, minor and
 * micro versions of the WebKit library your code is running
 * against. This is not necessarily the same as the
 * #WEBKIT_MAJOR_VERSION, #WEBKIT_MINOR_VERSION or
 * #WEBKIT_MICRO_VERSION, which represent the version of the WebKit
 * headers included when compiling the code.
 *
 */

/**
 * webkit_get_major_version:
 *
 * Returns the major version number of the WebKit library.
 * (e.g. in WebKit version 1.8.3 this is 1.)
 *
 * This function is in the library, so it represents the WebKit library
 * your code is running against. Contrast with the #WEBKIT_MAJOR_VERSION
 * macro, which represents the major version of the WebKit headers you
 * have included when compiling your code.
 *
 * Returns: the major version number of the WebKit library
 */
guint webkit_get_major_version(void)
{
    return WEBKIT_MAJOR_VERSION;
}

/**
 * webkit_get_minor_version:
 *
 * Returns the minor version number of the WebKit library.
 * (e.g. in WebKit version 1.8.3 this is 8.)
 *
 * This function is in the library, so it represents the WebKit library
 * your code is running against. Contrast with the #WEBKIT_MINOR_VERSION
 * macro, which represents the minor version of the WebKit headers you
 * have included when compiling your code.
 *
 * Returns: the minor version number of the WebKit library
 */
guint webkit_get_minor_version(void)
{
    return WEBKIT_MINOR_VERSION;
}

/**
 * webkit_get_micro_version:
 *
 * Returns the micro version number of the WebKit library.
 * (e.g. in WebKit version 1.8.3 this is 3.)
 *
 * This function is in the library, so it represents the WebKit library
 * your code is running against. Contrast with the #WEBKIT_MICRO_VERSION
 * macro, which represents the micro version of the WebKit headers you
 * have included when compiling your code.
 *
 * Returns: the micro version number of the WebKit library
 */
guint webkit_get_micro_version(void)
{
    return WEBKIT_MICRO_VERSION;
}
