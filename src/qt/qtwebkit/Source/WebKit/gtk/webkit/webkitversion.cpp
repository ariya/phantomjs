/*
 * Copyright (C) 2008 Christian Dywan <christian@imendio.com>
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
#include "webkitversion.h"

/**
 * webkit_major_version:
 *
 * The major version number of the WebKit that is linked against.
 *
 * Return value: The major version
 *
 * Since: 1.0.1
 */
guint webkit_major_version()
{
    return WEBKIT_MAJOR_VERSION;
}

/**
 * webkit_minor_version:
 *
 * The minor version number of the WebKit that is linked against.
 *
 * Return value: The minor version
 *
 * Since: 1.0.1
 */
guint webkit_minor_version()
{
    return WEBKIT_MINOR_VERSION;
}

/**
 * webkit_micro_version:
 *
 * The micro version number of the WebKit that is linked against.
 *
 * Return value: The micro version
 *
 * Since: 1.0.1
 */
guint webkit_micro_version()
{
    return WEBKIT_MICRO_VERSION;
}
