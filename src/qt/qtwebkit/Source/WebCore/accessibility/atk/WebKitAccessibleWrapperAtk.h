/*
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2009 Jan Alonzo
 * Copyright (C) 2009, 2010, 2011, 2012 Igalia S.L.
 * Copyright (C) 2013 Samsung Electronics
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

#ifndef WebKitAccessibleWrapperAtk_h
#define WebKitAccessibleWrapperAtk_h

#if HAVE(ACCESSIBILITY)

#include <atk/atk.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
class AccessibilityObject;
}

G_BEGIN_DECLS

#define WEBKIT_TYPE_ACCESSIBLE                  (webkitAccessibleGetType ())
#define WEBKIT_ACCESSIBLE(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBKIT_TYPE_ACCESSIBLE, WebKitAccessible))
#define WEBKIT_ACCESSIBLE_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), WEBKIT_TYPE_ACCESSIBLE, WebKitAccessibleClass))
#define WEBKIT_IS_ACCESSIBLE(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WEBKIT_TYPE_ACCESSIBLE))
#define WEBKIT_IS_ACCESSIBLE_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), WEBKIT_TYPE_ACCESSIBLE))
#define WEBKIT_ACCESSIBLE_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), WEBKIT_TYPE_ACCESSIBLE, WebKitAccessibleClass))

typedef struct _WebKitAccessible                WebKitAccessible;
typedef struct _WebKitAccessibleClass           WebKitAccessibleClass;
typedef struct _WebKitAccessiblePrivate         WebKitAccessiblePrivate;


struct _WebKitAccessible {
    AtkObject parent;
    WebCore::AccessibilityObject* m_object;

    WebKitAccessiblePrivate *priv;
};

struct _WebKitAccessibleClass {
    AtkObjectClass parentClass;
};

enum AtkCachedProperty {
    AtkCachedAccessibleName,
    AtkCachedAccessibleDescription,
    AtkCachedActionName,
    AtkCachedActionKeyBinding,
    AtkCachedDocumentLocale,
    AtkCachedDocumentType,
    AtkCachedDocumentEncoding,
    AtkCachedDocumentURI,
    AtkCachedImageDescription
};

GType webkitAccessibleGetType(void) G_GNUC_CONST;

WebKitAccessible* webkitAccessibleNew(WebCore::AccessibilityObject*);

WebCore::AccessibilityObject* webkitAccessibleGetAccessibilityObject(WebKitAccessible*);

void webkitAccessibleDetach(WebKitAccessible*);

AtkObject* webkitAccessibleGetFocusedElement(WebKitAccessible*);

WebCore::AccessibilityObject* objectFocusedAndCaretOffsetUnignored(WebCore::AccessibilityObject*, int& offset);

const char* cacheAndReturnAtkProperty(AtkObject*, AtkCachedProperty, String value);

G_END_DECLS

#endif
#endif // WebKitAccessibleWrapperAtk_h
