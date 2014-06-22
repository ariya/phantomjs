/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2010 Apple Inc. All rights reserved.
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
#include "HTMLMetaElement.h"

#include "Attribute.h"
#include "Document.h"
#include "HTMLNames.h"

namespace WebCore {

using namespace HTMLNames;

inline HTMLMetaElement::HTMLMetaElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
{
    ASSERT(hasTagName(metaTag));
}

PassRefPtr<HTMLMetaElement> HTMLMetaElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLMetaElement(tagName, document));
}

void HTMLMetaElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == http_equivAttr)
        process();
    else if (name == contentAttr)
        process();
    else if (name == nameAttr) {
        // Do nothing
    } else
        HTMLElement::parseAttribute(name, value);
}

Node::InsertionNotificationRequest HTMLMetaElement::insertedInto(ContainerNode* insertionPoint)
{
    HTMLElement::insertedInto(insertionPoint);
    if (insertionPoint->inDocument())
        process();
    return InsertionDone;
}

void HTMLMetaElement::process()
{
    if (!inDocument())
        return;

    const AtomicString& contentValue = fastGetAttribute(contentAttr);
    if (contentValue.isNull())
        return;

    if (equalIgnoringCase(name(), "viewport"))
        document()->processViewport(contentValue, ViewportArguments::ViewportMeta);
    else if (equalIgnoringCase(name(), "referrer"))
        document()->processReferrerPolicy(contentValue);
#if ENABLE(LEGACY_VIEWPORT_ADAPTION)
    else if (equalIgnoringCase(name(), "handheldfriendly") && equalIgnoringCase(contentValue, "true"))
        document()->processViewport("width=device-width", ViewportArguments::HandheldFriendlyMeta);
    else if (equalIgnoringCase(name(), "mobileoptimized"))
        document()->processViewport("width=device-width, initial-scale=1", ViewportArguments::MobileOptimizedMeta);
#endif

    // Get the document to process the tag, but only if we're actually part of DOM tree (changing a meta tag while
    // it's not in the tree shouldn't have any effect on the document)
    const AtomicString& httpEquivValue = fastGetAttribute(http_equivAttr);
    if (!httpEquivValue.isNull())
        document()->processHttpEquiv(httpEquivValue, contentValue);
}

String HTMLMetaElement::content() const
{
    return getAttribute(contentAttr);
}

String HTMLMetaElement::httpEquiv() const
{
    return getAttribute(http_equivAttr);
}

String HTMLMetaElement::name() const
{
    return getNameAttribute();
}

#if ENABLE(MICRODATA)
String HTMLMetaElement::itemValueText() const
{
    return getAttribute(contentAttr);
}

void HTMLMetaElement::setItemValueText(const String& value, ExceptionCode&)
{
    setAttribute(contentAttr, value);
}
#endif

}
