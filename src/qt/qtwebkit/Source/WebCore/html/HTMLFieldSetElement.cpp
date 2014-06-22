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
#include "HTMLFieldSetElement.h"

#include "HTMLCollection.h"
#include "HTMLLegendElement.h"
#include "HTMLNames.h"
#include "HTMLObjectElement.h"
#include "NodeTraversal.h"
#include "RenderFieldset.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

using namespace HTMLNames;

inline HTMLFieldSetElement::HTMLFieldSetElement(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
    : HTMLFormControlElement(tagName, document, form)
    , m_documentVersion(0)
{
    ASSERT(hasTagName(fieldsetTag));
}

PassRefPtr<HTMLFieldSetElement> HTMLFieldSetElement::create(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
{
    return adoptRef(new HTMLFieldSetElement(tagName, document, form));
}

void HTMLFieldSetElement::invalidateDisabledStateUnder(Element* base)
{
    for (Element* element = ElementTraversal::firstWithin(base); element; element = ElementTraversal::next(element, base)) {
        if (element->isFormControlElement())
            static_cast<HTMLFormControlElement*>(element)->ancestorDisabledStateWasChanged();
    }
}

void HTMLFieldSetElement::disabledAttributeChanged()
{
    // This element must be updated before the style of nodes in its subtree gets recalculated.
    HTMLFormControlElement::disabledAttributeChanged();
    invalidateDisabledStateUnder(this);
}

void HTMLFieldSetElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    HTMLFormControlElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);
    for (Element* element = ElementTraversal::firstWithin(this); element; element = ElementTraversal::nextSkippingChildren(element, this)) {
        if (element->hasTagName(legendTag))
            invalidateDisabledStateUnder(element);
    }
}

bool HTMLFieldSetElement::supportsFocus() const
{
    return HTMLElement::supportsFocus();
}

const AtomicString& HTMLFieldSetElement::formControlType() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, fieldset, ("fieldset", AtomicString::ConstructFromLiteral));
    return fieldset;
}

RenderObject* HTMLFieldSetElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderFieldset(this);
}

HTMLLegendElement* HTMLFieldSetElement::legend() const
{
    for (Element* child = ElementTraversal::firstWithin(this); child; child = ElementTraversal::nextSkippingChildren(child, this)) {
        if (child->hasTagName(legendTag))
            return static_cast<HTMLLegendElement*>(child);
    }
    return 0;
}

PassRefPtr<HTMLCollection> HTMLFieldSetElement::elements()
{
    return ensureCachedHTMLCollection(FormControls);
}

void HTMLFieldSetElement::refreshElementsIfNeeded() const
{
    uint64_t docVersion = document()->domTreeVersion();
    if (m_documentVersion == docVersion)
        return;

    m_documentVersion = docVersion;

    m_associatedElements.clear();

    for (Element* element = ElementTraversal::firstWithin(this); element; element = ElementTraversal::next(element, this)) {
        if (element->hasTagName(objectTag)) {
            m_associatedElements.append(static_cast<HTMLObjectElement*>(element));
            continue;
        }

        if (!element->isFormControlElement())
            continue;

        m_associatedElements.append(static_cast<HTMLFormControlElement*>(element));
    }
}

const Vector<FormAssociatedElement*>& HTMLFieldSetElement::associatedElements() const
{
    refreshElementsIfNeeded();
    return m_associatedElements;
}

unsigned HTMLFieldSetElement::length() const
{
    refreshElementsIfNeeded();
    unsigned len = 0;
    for (unsigned i = 0; i < m_associatedElements.size(); ++i)
        if (m_associatedElements[i]->isEnumeratable())
            ++len;
    return len;
}

} // namespace
