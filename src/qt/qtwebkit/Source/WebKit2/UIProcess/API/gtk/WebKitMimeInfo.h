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

#if !defined(__WEBKIT2_H_INSIDE__) && !defined(WEBKIT2_COMPILATION)
#error "Only <webkit2/webkit2.h> can be included directly."
#endif

#ifndef WebKitMimeInfo_h
#define WebKitMimeInfo_h

#include <glib-object.h>
#include <webkit2/WebKitDefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_MIME_INFO (webkit_mime_info_get_type())

typedef struct _WebKitMimeInfo WebKitMimeInfo;


WEBKIT_API GType
webkit_mime_info_get_type        (void);

WEBKIT_API WebKitMimeInfo *
webkit_mime_info_ref             (WebKitMimeInfo *info);

WEBKIT_API void
webkit_mime_info_unref           (WebKitMimeInfo *info);

WEBKIT_API const gchar *
webkit_mime_info_get_mime_type   (WebKitMimeInfo *info);

WEBKIT_API const gchar *
webkit_mime_info_get_description (WebKitMimeInfo *info);

WEBKIT_API const gchar * const *
webkit_mime_info_get_extensions  (WebKitMimeInfo *info);

G_END_DECLS

#endif
