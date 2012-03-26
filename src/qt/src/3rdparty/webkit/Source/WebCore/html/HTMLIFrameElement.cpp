/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Simon Hausmann (hausmann@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2006, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Ericsson AB. All rights reserved.
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
#include "HTMLIFrameElement.h"

#include "Attribute.h"
#include "CSSPropertyNames.h"
#include "Frame.h"
#include "HTMLDocument.h"
#include "HTMLNames.h"
#include "RenderIFrame.h"

namespace WebCore {

using namespace HTMLNames;

inline HTMLIFrameElement::HTMLIFrameElement(const QualifiedName& tagName, Document* document)
    : HTMLFrameElementBase(tagName, document)
{
    ASSERT(hasTagName(iframeTag));
}

PassRefPtr<HTMLIFrameElement> HTMLIFrameElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLIFrameElement(tagName, document));
}

bool HTMLIFrameElement::mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const
{
    if (attrName == widthAttr || attrName == heightAttr) {
        result = eUniversal;
        return false;
    }
    
    if (attrName == alignAttr) {
        result = eReplaced; // Share with <img> since the alignment behavior is the same.
        return false;
    }
    
    if (attrName == frameborderAttr) {
        result = eReplaced;
        return false;
    }

    return HTMLFrameElementBase::mapToEntry(attrName, result);
}

static SandboxFlags parseSandboxAttribute(Attribute* attribute)
{
    if (attribute->isNull())
        return SandboxNone;

    // Parse the unordered set of unique space-separated tokens.
    SandboxFlags flags = SandboxAll;
    const UChar* characters = attribute->value().characters();
    unsigned length = attribute->value().length();
    unsigned start = 0;
    while (true) {
        while (start < length && isASCIISpace(characters[start]))
            ++start;
        if (start >= length)
            break;
        unsigned end = start + 1;
        while (end < length && !isASCIISpace(characters[end]))
            ++end;

        // Turn off the corresponding sandbox flag if it's set as "allowed".
        String sandboxToken = String(characters + start, end - start);
        if (equalIgnoringCase(sandboxToken, "allow-same-origin"))
            flags &= ~SandboxOrigin;
        else if (equalIgnoringCase(sandboxToken, "allow-forms"))
            flags &= ~SandboxForms;
        else if (equalIgnoringCase(sandboxToken, "allow-scripts"))
            flags &= ~SandboxScripts;
        else if (equalIgnoringCase(sandboxToken, "allow-top-navigation"))
            flags &= ~SandboxTopNavigation;

        start = end + 1;
    }

    return flags;
}

void HTMLIFrameElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == widthAttr)
        addCSSLength(attr, CSSPropertyWidth, attr->value());
    else if (attr->name() == heightAttr)
        addCSSLength(attr, CSSPropertyHeight, attr->value());
    else if (attr->name() == alignAttr)
        addHTMLAlignment(attr);
    else if (attr->name() == nameAttr) {
        const AtomicString& newName = attr->value();
        if (inDocument() && document()->isHTMLDocument()) {
            HTMLDocument* document = static_cast<HTMLDocument*>(this->document());
            document->removeExtraNamedItem(m_name);
            document->addExtraNamedItem(newName);
        }
        m_name = newName;
    } else if (attr->name() == frameborderAttr) {
        // Frame border doesn't really match the HTML4 spec definition for iframes.  It simply adds
        // a presentational hint that the border should be off if set to zero.
        if (!attr->isNull() && !attr->value().toInt())
            // Add a rule that nulls out our border width.
            addCSSLength(attr, CSSPropertyBorderWidth, "0");
    } else if (attr->name() == sandboxAttr)
        setSandboxFlags(parseSandboxAttribute(attr));
    else
        HTMLFrameElementBase::parseMappedAttribute(attr);
}

bool HTMLIFrameElement::rendererIsNeeded(RenderStyle* style)
{
    return isURLAllowed() && style->display() != NONE;
}

RenderObject* HTMLIFrameElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderIFrame(this);
}

void HTMLIFrameElement::insertedIntoDocument()
{
    if (document()->isHTMLDocument())
        static_cast<HTMLDocument*>(document())->addExtraNamedItem(m_name);

    HTMLFrameElementBase::insertedIntoDocument();
}

void HTMLIFrameElement::removedFromDocument()
{
    if (document()->isHTMLDocument())
        static_cast<HTMLDocument*>(document())->removeExtraNamedItem(m_name);

    HTMLFrameElementBase::removedFromDocument();
}

bool HTMLIFrameElement::isURLAttribute(Attribute* attr) const
{
    return attr->name() == srcAttr;
}

}
