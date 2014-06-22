/*
 * Copyright (C) 2008 Collabora Ltd.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "GOwnPtr.h"

#if USE(GLIB)

#include <gio/gio.h>
#include <glib.h>

namespace WTF {

template <> void freeOwnedGPtr<GError>(GError* ptr)
{
    if (ptr)
        g_error_free(ptr);
}

template <> void freeOwnedGPtr<GList>(GList* ptr)
{
    g_list_free(ptr);
}

template <> void freeOwnedGPtr<GSList>(GSList* ptr)
{
    g_slist_free(ptr);
}

template <> void freeOwnedGPtr<GPatternSpec>(GPatternSpec* ptr)
{
    if (ptr)
        g_pattern_spec_free(ptr);
}

template <> void freeOwnedGPtr<GDir>(GDir* ptr)
{
    if (ptr)
        g_dir_close(ptr);
}

template <> void freeOwnedGPtr<GTimer>(GTimer* ptr)
{
    if (ptr)
        g_timer_destroy(ptr);
}

template <> void freeOwnedGPtr<GKeyFile>(GKeyFile* ptr)
{
    if (ptr)
        g_key_file_free(ptr);
}

} // namespace WTF

#endif // USE(GLIB)
