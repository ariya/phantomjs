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

#ifndef WebKitSoupRequestGeneric_h
#define WebKitSoupRequestGeneric_h

#include <glib-object.h>
#include <libsoup/soup.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_SOUP_REQUEST_GENERIC            (webkit_soup_request_generic_get_type())
#define WEBKIT_SOUP_REQUEST_GENERIC(object)         (G_TYPE_CHECK_INSTANCE_CAST((object), WEBKIT_TYPE_SOUP_REQUEST_GENERIC, WebKitSoupRequestGeneric))
#define WEBKIT_IS_SOUP_REQUEST_GENERIC(object)      (G_TYPE_CHECK_INSTANCE_TYPE((object), WEBKIT_TYPE_SOUP_REQUEST_GENERIC))
#define WEBKIT_SOUP_REQUEST_GENERIC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), WEBKIT_TYPE_SOUP_REQUEST_GENERIC, WebKitSoupRequestGenericClass))
#define WEBKIT_IS_SOUP_REQUEST_GENERIC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), WEBKIT_TYPE_SOUP_REQUEST_GENERIC))
#define WEBKIT_SOUP_REQUEST_GENERIC_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), WEBKIT_TYPE_SOUP_REQUEST_GENERIC, WebKitSoupRequestGenericClass))

typedef struct _WebKitSoupRequestGeneric WebKitSoupRequestGeneric;
typedef struct _WebKitSoupRequestGenericClass WebKitSoupRequestGenericClass;
typedef struct _WebKitSoupRequestGenericPrivate WebKitSoupRequestGenericPrivate;

struct _WebKitSoupRequestGeneric {
    SoupRequest parent;

    WebKitSoupRequestGenericPrivate *priv;
};

struct _WebKitSoupRequestGenericClass {
    SoupRequestClass parent;
};

GType webkit_soup_request_generic_get_type();

void webkitSoupRequestGenericSetContentLength(WebKitSoupRequestGeneric*, goffset contentLength);
void webkitSoupRequestGenericSetContentType(WebKitSoupRequestGeneric*, const char* mimeType);

G_END_DECLS

#endif // WebKitSoupRequestGeneric_h
