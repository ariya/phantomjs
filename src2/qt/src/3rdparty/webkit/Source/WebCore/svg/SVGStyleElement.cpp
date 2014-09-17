/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2006 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Cameron McCormack <cam@mcc.id.au>
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

#if ENABLE(SVG)
#include "SVGStyleElement.h"

#include "Attribute.h"
#include "CSSStyleSheet.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "SVGNames.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

inline SVGStyleElement::SVGStyleElement(const QualifiedName& tagName, Document* document, bool createdByParser)
    : SVGElement(tagName, document)
    , StyleElement(document, createdByParser)
{
}

SVGStyleElement::~SVGStyleElement()
{
    StyleElement::clearDocumentData(document(), this);
}

PassRefPtr<SVGStyleElement> SVGStyleElement::create(const QualifiedName& tagName, Document* document, bool createdByParser)
{
    return adoptRef(new SVGStyleElement(tagName, document, createdByParser));
}

const AtomicString& SVGStyleElement::type() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, defaultValue, ("text/css"));
    const AtomicString& n = getAttribute(SVGNames::typeAttr);
    return n.isNull() ? defaultValue : n;
}

void SVGStyleElement::setType(const AtomicString& type, ExceptionCode& ec)
{
    setAttribute(SVGNames::typeAttr, type, ec);
}

const AtomicString& SVGStyleElement::media() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, defaultValue, ("all"));
    const AtomicString& n = getAttribute(SVGNames::mediaAttr);
    return n.isNull() ? defaultValue : n;
}

void SVGStyleElement::setMedia(const AtomicString& media, ExceptionCode& ec)
{
    setAttribute(SVGNames::mediaAttr, media, ec);
}

String SVGStyleElement::title() const
{
    return getAttribute(SVGNames::titleAttr);
}

void SVGStyleElement::setTitle(const AtomicString& title, ExceptionCode& ec)
{
    setAttribute(SVGNames::titleAttr, title, ec);
}

void SVGStyleElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == SVGNames::titleAttr && m_sheet)
        m_sheet->setTitle(attr->value());
    else {
        if (SVGLangSpace::parseMappedAttribute(attr))
            return;
        SVGElement::parseMappedAttribute(attr);
    }
}

void SVGStyleElement::finishParsingChildren()
{
    StyleElement::finishParsingChildren(this);
    SVGElement::finishParsingChildren();
}

void SVGStyleElement::insertedIntoDocument()
{
    SVGElement::insertedIntoDocument();
    StyleElement::insertedIntoDocument(document(), this);
}

void SVGStyleElement::removedFromDocument()
{
    SVGElement::removedFromDocument();
    StyleElement::removedFromDocument(document(), this);
}

void SVGStyleElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    StyleElement::childrenChanged(this);
    SVGElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);
}

}

#endif // ENABLE(SVG)
