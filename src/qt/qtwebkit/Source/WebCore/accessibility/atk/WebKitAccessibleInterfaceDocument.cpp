/*
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2009 Jan Alonzo
 * Copyright (C) 2012 Igalia S.L.
 * Copyright (C) 2013 Samsung Electronics
 *
 * Portions from Mozilla a11y, copyright as follows:
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Sun Microsystems, Inc.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
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
#include "WebKitAccessibleInterfaceDocument.h"

#if HAVE(ACCESSIBILITY)

#include "AccessibilityObject.h"
#include "Document.h"
#include "DocumentType.h"
#include "WebKitAccessibleUtil.h"
#include "WebKitAccessibleWrapperAtk.h"

using namespace WebCore;

static AccessibilityObject* core(AtkDocument* document)
{
    if (!WEBKIT_IS_ACCESSIBLE(document))
        return 0;

    return webkitAccessibleGetAccessibilityObject(WEBKIT_ACCESSIBLE(document));
}

static const gchar* documentAttributeValue(AtkDocument* document, const gchar* attribute)
{
    Document* coreDocument = core(document)->document();
    if (!coreDocument)
        return 0;

    String value = String();
    AtkCachedProperty atkCachedProperty;

    if (!g_ascii_strcasecmp(attribute, "DocType") && coreDocument->doctype()) {
        value = coreDocument->doctype()->name();
        atkCachedProperty = AtkCachedDocumentType;
    } else if (!g_ascii_strcasecmp(attribute, "Encoding")) {
        value = coreDocument->charset();
        atkCachedProperty = AtkCachedDocumentEncoding;
    } else if (!g_ascii_strcasecmp(attribute, "URI")) {
        value = coreDocument->documentURI();
        atkCachedProperty = AtkCachedDocumentURI;
    }

    if (!value.isEmpty())
        return cacheAndReturnAtkProperty(ATK_OBJECT(document), atkCachedProperty, value);

    return 0;
}

static const gchar* webkitAccessibleDocumentGetAttributeValue(AtkDocument* document, const gchar* attribute)
{
    return documentAttributeValue(document, attribute);
}

static AtkAttributeSet* webkitAccessibleDocumentGetAttributes(AtkDocument* document)
{
    AtkAttributeSet* attributeSet = 0;
    const gchar* attributes[] = { "DocType", "Encoding", "URI" };

    for (unsigned i = 0; i < G_N_ELEMENTS(attributes); i++) {
        const gchar* value = documentAttributeValue(document, attributes[i]);
        if (value)
            attributeSet = addToAtkAttributeSet(attributeSet, attributes[i], value);
    }

    return attributeSet;
}

static const gchar* webkitAccessibleDocumentGetLocale(AtkDocument* document)
{
    // The logic to resolve locale has been moved to
    // AtkObject::get_object_locale() virtual method. However, to avoid breaking
    // clients expecting the deprecated AtkDocumentIface::get_document_locale()
    // to be overriden, method is kept and chained up to
    // AtkObject::get_object_locale(). <https://bugs.webkit.org/show_bug.cgi?id=115647>
    return atk_object_get_object_locale(ATK_OBJECT(document));
}

void webkitAccessibleDocumentInterfaceInit(AtkDocumentIface* iface)
{
    iface->get_document_attribute_value = webkitAccessibleDocumentGetAttributeValue;
    iface->get_document_attributes = webkitAccessibleDocumentGetAttributes;
    iface->get_document_locale = webkitAccessibleDocumentGetLocale;
}

#endif
