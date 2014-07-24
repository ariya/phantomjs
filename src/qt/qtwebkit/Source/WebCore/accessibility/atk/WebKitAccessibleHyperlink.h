/*
 * Copyright (C) 2010, 2011, 2012 Igalia S.L.
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

#ifndef WebKitAccessibleHyperlink_h
#define WebKitAccessibleHyperlink_h

#if HAVE(ACCESSIBILITY)

#include <atk/atk.h>

namespace WebCore {
class AccessibilityObject;
}

G_BEGIN_DECLS

#define WEBKIT_TYPE_ACCESSIBLE_HYPERLINK                  (webkitAccessibleHyperlinkGetType ())
#define WEBKIT_ACCESSIBLE_HYPERLINK(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBKIT_TYPE_ACCESSIBLE_HYPERLINK, WebKitAccessibleHyperlink))
#define WEBKIT_ACCESSIBLE_HYPERLINK_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), WEBKIT_TYPE_ACCESSIBLE_HYPERLINK, WebKitAccessibleHyperlinkClass))
#define WEBKIT_IS_ACCESSIBLE_HYPERLINK(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WEBKIT_TYPE_ACCESSIBLE_HYPERLINK))
#define WEBKIT_IS_ACCESSIBLE_HYPERLINK_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), WEBKIT_TYPE_ACCESSIBLE_HYPERLINK))
#define WEBKIT_ACCESSIBLE_HYPERLINK_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), WEBKIT_TYPE_ACCESSIBLE_HYPERLINK, WebKitAccessibleHyperlinkClass))

typedef struct _WebKitAccessibleHyperlink                WebKitAccessibleHyperlink;
typedef struct _WebKitAccessibleHyperlinkClass           WebKitAccessibleHyperlinkClass;
typedef struct _WebKitAccessibleHyperlinkPrivate         WebKitAccessibleHyperlinkPrivate;

struct _WebKitAccessibleHyperlink {
    AtkHyperlink parent;

    // private
    WebKitAccessibleHyperlinkPrivate *priv;
};

struct _WebKitAccessibleHyperlinkClass {
    AtkHyperlinkClass parentClass;
};

GType webkitAccessibleHyperlinkGetType(void) G_GNUC_CONST;

WebKitAccessibleHyperlink* webkitAccessibleHyperlinkNew(AtkHyperlinkImpl*);

WebCore::AccessibilityObject* webkitAccessibleHyperlinkGetAccessibilityObject(WebKitAccessibleHyperlink*);

G_END_DECLS

#endif // WebKitAccessibleHyperlink_h

#endif
