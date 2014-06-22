/*
 * Copyright (C) 2011 Lukasz Slachciak
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

#ifndef webkitapplicationcache_h
#define webkitapplicationcache_h

#include <webkit/webkitdefines.h>

G_BEGIN_DECLS

WEBKIT_API unsigned long long
webkit_application_cache_get_maximum_size(void);

WEBKIT_API void
webkit_application_cache_set_maximum_size(unsigned long long size);

WEBKIT_API const gchar*
webkit_application_cache_get_database_directory_path(void);

G_END_DECLS

#endif /* webkitapplicationcache_h */
