/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2010 Apple Inc. All rights reserved.
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
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
 *
 */

#include "config.h"
#include "HTMLIsIndexElement.h"

#include "Attribute.h"
#include "HTMLNames.h"

namespace WebCore {

using namespace HTMLNames;

HTMLIsIndexElement::HTMLIsIndexElement(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
    : HTMLInputElement(tagName, document, form, false)
{
    ASSERT(hasTagName(isindexTag));
    setDefaultName(isindexTag.localName());
}

PassRefPtr<HTMLIsIndexElement> HTMLIsIndexElement::create(Document* document, HTMLFormElement* form)
{
    return adoptRef(new HTMLIsIndexElement(isindexTag, document, form));
}

PassRefPtr<HTMLIsIndexElement> HTMLIsIndexElement::create(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
{
    return adoptRef(new HTMLIsIndexElement(tagName, document, form));
}

void HTMLIsIndexElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == promptAttr)
        setValue(attr->value());
    else if (attr->name() == placeholderAttr)
        updatePlaceholderVisibility(true);
    else
        // don't call HTMLInputElement::parseMappedAttribute here, as it would
        // accept attributes this element does not support
        HTMLFormControlElement::parseMappedAttribute(attr);
}

} // namespace
