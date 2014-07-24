/*
 * Copyright (C) 2009 Martin Robinson
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
#include "GRefPtr.h"

#if USE(GLIB)

#include <glib.h>

namespace WTF {

template <> GHashTable* refGPtr(GHashTable* ptr)
{
    if (ptr)
        g_hash_table_ref(ptr);
    return ptr;
}

template <> void derefGPtr(GHashTable* ptr)
{
    g_hash_table_unref(ptr);
}

template <> GMainContext* refGPtr(GMainContext* ptr)
{
    if (ptr)
        g_main_context_ref(ptr);
    return ptr;
}

template <> void derefGPtr(GMainContext* ptr)
{
    if (ptr)
        g_main_context_unref(ptr);
}

template <> GMainLoop* refGPtr(GMainLoop* ptr)
{
    if (ptr)
        g_main_loop_ref(ptr);
    return ptr;
}

template <> void derefGPtr(GMainLoop* ptr)
{
    if (ptr)
        g_main_loop_unref(ptr);
}

#if GLIB_CHECK_VERSION(2, 32, 0)
template <> GBytes* refGPtr(GBytes* ptr)
{
    if (ptr)
        g_bytes_ref(ptr);
    return ptr;
}

template <> void derefGPtr(GBytes* ptr)
{
    if (ptr)
        g_bytes_unref(ptr);
}

# else

typedef struct _GBytes {
    bool fake;
} GBytes;

template <> GBytes* refGPtr(GBytes* ptr)
{
    return ptr;
}

template <> void derefGPtr(GBytes* ptr)
{
}

#endif

template <> GVariant* refGPtr(GVariant* ptr)
{
    if (ptr)
        g_variant_ref(ptr);
    return ptr;
}

template <> void derefGPtr(GVariant* ptr)
{
    g_variant_unref(ptr);
}

template <> GSource* refGPtr(GSource* ptr)
{
    if (ptr)
        g_source_ref(ptr);
    return ptr;
}

template <> void derefGPtr(GSource* ptr)
{
    if (ptr)
        g_source_unref(ptr);
}

template <> GPtrArray* refGPtr(GPtrArray* ptr)
{
    if (ptr)
        g_ptr_array_ref(ptr);
    return ptr;
}

template <> void derefGPtr(GPtrArray* ptr)
{
    if (ptr)
        g_ptr_array_unref(ptr);
}

template <> GByteArray* refGPtr(GByteArray* ptr)
{
    if (ptr)
        g_byte_array_ref(ptr);
    return ptr;
}

template <> void derefGPtr(GByteArray* ptr)
{
    if (ptr)
        g_byte_array_unref(ptr);
}

} // namespace WTF

#endif // USE(GLIB)
