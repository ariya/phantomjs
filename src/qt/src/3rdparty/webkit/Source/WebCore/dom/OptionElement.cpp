/*
 * Copyright (C) 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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
#include "OptionElement.h"

#include "Document.h"
#include "Element.h"
#include "HTMLNames.h"
#include "HTMLOptionElement.h"
#include "OptionGroupElement.h"
#include "ScriptElement.h"
#include "SelectElement.h"
#include <wtf/Assertions.h>

namespace WebCore {

void OptionElement::setSelectedState(OptionElementData& data, Element* element, bool selected)
{
    if (data.selected() == selected)
        return;

    data.setSelected(selected);
    element->setNeedsStyleRecalc();
}

int OptionElement::optionIndex(SelectElement* selectElement, const Element* element)
{
    if (!selectElement)
        return 0;

    // Let's do this dynamically. Might be a bit slow, but we're sure
    // we won't forget to update a member variable in some cases...
    const Vector<Element*>& items = selectElement->listItems();
    int length = items.size();
    int optionIndex = 0;
    for (int i = 0; i < length; ++i) {
        if (!isOptionElement(items[i]))
            continue;
        if (items[i] == element)
            return optionIndex;
        ++optionIndex;
    }

    return 0;
}

String OptionElement::collectOptionLabelOrText(const OptionElementData& data, const Element* element)
{
    Document* document = element->document();
    String text;

    // WinIE does not use the label attribute, so as a quirk, we ignore it.
    if (!document->inQuirksMode())
        text = data.label();
    if (text.isEmpty())
        text = collectOptionInnerText(element);
    return normalizeText(document, text);
}

String OptionElement::collectOptionInnerText(const Element* element)
{
    String text;
    Node* n = element->firstChild();
    while (n) {
        if (n->nodeType() == Node::TEXT_NODE || n->nodeType() == Node::CDATA_SECTION_NODE)
            text += n->nodeValue();

        // skip script content
        if (n->isElementNode() && toScriptElement(static_cast<Element*>(n)))
            n = n->traverseNextSibling(element);
        else
            n = n->traverseNextNode(element);
    }
    return text;
}

String OptionElement::normalizeText(const Document* document, const String& src)
{
    String text = document->displayStringModifiedByEncoding(src);

    // In WinIE, leading and trailing whitespace is ignored in options and optgroups. We match this behavior.
    text = text.stripWhiteSpace();

    // We want to collapse our whitespace too.  This will match other browsers.
    text = text.simplifyWhiteSpace();
    return text;
}

String OptionElement::collectOptionTextRespectingGroupLabel(const OptionElementData& data, const Element* element)
{
    Element* parentElement = static_cast<Element*>(element->parentNode());
    if (parentElement && toOptionGroupElement(parentElement))
        return "    " + collectOptionLabelOrText(data, element);

    return collectOptionLabelOrText(data, element);
}

String OptionElement::collectOptionValue(const OptionElementData& data, const Element* element)
{
    String value = data.value();
    if (!value.isNull())
        return value;

    // Use the text if the value wasn't set.
    return collectOptionInnerText(element).stripWhiteSpace();
}

// OptionElementData
OptionElementData::OptionElementData()
    : m_selected(false)
{
}

OptionElementData::~OptionElementData()
{
}

OptionElement* toOptionElement(Element* element)
{
    if (element->isHTMLElement() && element->hasTagName(HTMLNames::optionTag))
        return static_cast<HTMLOptionElement*>(element);
    return 0;
}

bool isOptionElement(Element* element)
{
    return element->hasLocalName(HTMLNames::optionTag);
}

}
