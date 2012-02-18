/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 * Copyright (C) 2004, 2005, 2006, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "HTMLOptionElement.h"

#include "Attribute.h"
#include "CSSStyleSelector.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "HTMLNames.h"
#include "HTMLSelectElement.h"
#include "NodeRenderStyle.h"
#include "RenderMenuList.h"
#include "Text.h"
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>

namespace WebCore {

using namespace HTMLNames;

HTMLOptionElement::HTMLOptionElement(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
    : HTMLFormControlElement(tagName, document, form)
{
    ASSERT(hasTagName(optionTag));
}

PassRefPtr<HTMLOptionElement> HTMLOptionElement::create(Document* document, HTMLFormElement* form)
{
    return adoptRef(new HTMLOptionElement(optionTag, document, form));
}

PassRefPtr<HTMLOptionElement> HTMLOptionElement::create(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
{
    return adoptRef(new HTMLOptionElement(tagName, document, form));
}

PassRefPtr<HTMLOptionElement> HTMLOptionElement::createForJSConstructor(Document* document, const String& data, const String& value,
        bool defaultSelected, bool selected, ExceptionCode& ec)
{
    RefPtr<HTMLOptionElement> element = adoptRef(new HTMLOptionElement(optionTag, document));

    RefPtr<Text> text = Text::create(document, data.isNull() ? "" : data);

    ec = 0;
    element->appendChild(text.release(), ec);
    if (ec)
        return 0;

    if (!value.isNull())
        element->setValue(value);
    element->setDefaultSelected(defaultSelected);
    element->setSelected(selected);

    return element.release();
}

void HTMLOptionElement::attach()
{
    if (parentNode()->renderStyle())
        setRenderStyle(styleForRenderer());
    HTMLFormControlElement::attach();
}

void HTMLOptionElement::detach()
{
    m_style.clear();
    HTMLFormControlElement::detach();
}

bool HTMLOptionElement::supportsFocus() const
{
    return HTMLElement::supportsFocus();
}

bool HTMLOptionElement::isFocusable() const
{
    // Option elements do not have a renderer so we check the renderStyle instead.
    return supportsFocus() && renderStyle() && renderStyle()->display() != NONE;
}

const AtomicString& HTMLOptionElement::formControlType() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, option, ("option"));
    return option;
}

String HTMLOptionElement::text() const
{
    return OptionElement::collectOptionLabelOrText(m_data, this);
}

void HTMLOptionElement::setText(const String &text, ExceptionCode& ec)
{
    // Handle the common special case where there's exactly 1 child node, and it's a text node.
    Node* child = firstChild();
    if (child && child->isTextNode() && !child->nextSibling()) {
        static_cast<Text *>(child)->setData(text, ec);
        return;
    }

    removeChildren();
    appendChild(Text::create(document(), text), ec);
}

void HTMLOptionElement::accessKeyAction(bool)
{
    HTMLSelectElement* select = ownerSelectElement();
    if (select)
        select->accessKeySetSelectedIndex(index());
}

int HTMLOptionElement::index() const
{
    return OptionElement::optionIndex(ownerSelectElement(), this);
}

void HTMLOptionElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == selectedAttr)
        m_data.setSelected(!attr->isNull());
    else if (attr->name() == valueAttr)
        m_data.setValue(attr->value());
    else if (attr->name() == labelAttr)
        m_data.setLabel(attr->value());
    else
        HTMLFormControlElement::parseMappedAttribute(attr);
}

String HTMLOptionElement::value() const
{
    return OptionElement::collectOptionValue(m_data, this);
}

void HTMLOptionElement::setValue(const String& value)
{
    setAttribute(valueAttr, value);
}

bool HTMLOptionElement::selected() const
{
    if (HTMLSelectElement* select = ownerSelectElement())
        select->recalcListItemsIfNeeded();
    return m_data.selected();
}

void HTMLOptionElement::setSelected(bool selected)
{
    if (m_data.selected() == selected)
        return;

    OptionElement::setSelectedState(m_data, this, selected);

    if (HTMLSelectElement* select = ownerSelectElement())
        select->setSelectedIndex(selected ? index() : -1, false);
}

void HTMLOptionElement::setSelectedState(bool selected)
{
    OptionElement::setSelectedState(m_data, this, selected);
}

void HTMLOptionElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    HTMLSelectElement* select = ownerSelectElement();
    if (select)
        select->childrenChanged(changedByParser);
    HTMLFormControlElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);
}

HTMLSelectElement* HTMLOptionElement::ownerSelectElement() const
{
    ContainerNode* select = parentNode();
    while (select && !select->hasTagName(selectTag))
        select = select->parentNode();

    if (!select)
        return 0;

    return static_cast<HTMLSelectElement*>(select);
}

bool HTMLOptionElement::defaultSelected() const
{
    return fastHasAttribute(selectedAttr);
}

void HTMLOptionElement::setDefaultSelected(bool b)
{
    setAttribute(selectedAttr, b ? "" : 0);
}

String HTMLOptionElement::label() const
{
    return m_data.label();
}

void HTMLOptionElement::setRenderStyle(PassRefPtr<RenderStyle> newStyle)
{
    m_style = newStyle;
    if (HTMLSelectElement* select = ownerSelectElement())
        if (RenderObject* renderer = select->renderer())
            renderer->repaint();
}

RenderStyle* HTMLOptionElement::nonRendererRenderStyle() const
{
    return m_style.get();
}

String HTMLOptionElement::textIndentedToRespectGroupLabel() const
{
    return OptionElement::collectOptionTextRespectingGroupLabel(m_data, this);
}

bool HTMLOptionElement::disabled() const
{
    return ownElementDisabled() || (parentNode() && static_cast<HTMLFormControlElement*>(parentNode())->disabled());
}

void HTMLOptionElement::insertedIntoTree(bool deep)
{
    if (HTMLSelectElement* select = ownerSelectElement()) {
        select->setRecalcListItems();
        // Avoid our selected() getter since it will recalculate list items incorrectly for us.
        if (m_data.selected())
            select->setSelectedIndex(index(), false);
        select->scrollToSelection();
    }

    HTMLFormControlElement::insertedIntoTree(deep);
}

} // namespace
