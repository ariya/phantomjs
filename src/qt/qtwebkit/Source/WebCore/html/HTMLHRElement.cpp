/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
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
 *
 */

#include "config.h"
#include "HTMLHRElement.h"

#include "Attribute.h"
#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "CSSValuePool.h"
#include "HTMLNames.h"
#include "StylePropertySet.h"

namespace WebCore {

using namespace HTMLNames;

HTMLHRElement::HTMLHRElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
{
    ASSERT(hasTagName(hrTag));
}

PassRefPtr<HTMLHRElement> HTMLHRElement::create(Document* document)
{
    return adoptRef(new HTMLHRElement(hrTag, document));
}

PassRefPtr<HTMLHRElement> HTMLHRElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLHRElement(tagName, document));
}

bool HTMLHRElement::isPresentationAttribute(const QualifiedName& name) const
{
    if (name == alignAttr || name == widthAttr || name == colorAttr || name == noshadeAttr || name == sizeAttr)
        return true;
    return HTMLElement::isPresentationAttribute(name);
}

void HTMLHRElement::collectStyleForPresentationAttribute(const QualifiedName& name, const AtomicString& value, MutableStylePropertySet* style)
{
    if (name == alignAttr) {
        if (equalIgnoringCase(value, "left")) {
            addPropertyToPresentationAttributeStyle(style, CSSPropertyMarginLeft, 0, CSSPrimitiveValue::CSS_PX);
            addPropertyToPresentationAttributeStyle(style, CSSPropertyMarginRight, CSSValueAuto);
        } else if (equalIgnoringCase(value, "right")) {
            addPropertyToPresentationAttributeStyle(style, CSSPropertyMarginLeft, CSSValueAuto);
            addPropertyToPresentationAttributeStyle(style, CSSPropertyMarginRight, 0, CSSPrimitiveValue::CSS_PX);
        } else {
            addPropertyToPresentationAttributeStyle(style, CSSPropertyMarginLeft, CSSValueAuto);
            addPropertyToPresentationAttributeStyle(style, CSSPropertyMarginRight, CSSValueAuto);
        }
    } else if (name == widthAttr) {
        bool ok;
        int v = value.toInt(&ok);
        if (ok && !v)
            addPropertyToPresentationAttributeStyle(style, CSSPropertyWidth, 1, CSSPrimitiveValue::CSS_PX);
        else
            addHTMLLengthToStyle(style, CSSPropertyWidth, value);
    } else if (name == colorAttr) {
        addPropertyToPresentationAttributeStyle(style, CSSPropertyBorderStyle, CSSValueSolid);
        addHTMLColorToStyle(style, CSSPropertyBorderColor, value);
        addHTMLColorToStyle(style, CSSPropertyBackgroundColor, value);
    } else if (name == noshadeAttr) {
        addPropertyToPresentationAttributeStyle(style, CSSPropertyBorderStyle, CSSValueSolid);

        RefPtr<CSSPrimitiveValue> darkGrayValue = cssValuePool().createColorValue(Color::darkGray);
        style->setProperty(CSSPropertyBorderColor, darkGrayValue);
        style->setProperty(CSSPropertyBackgroundColor, darkGrayValue);
    } else if (name == sizeAttr) {
        StringImpl* si = value.impl();
        int size = si->toInt();
        if (size <= 1)
            addPropertyToPresentationAttributeStyle(style, CSSPropertyBorderBottomWidth, 0, CSSPrimitiveValue::CSS_PX);
        else
            addPropertyToPresentationAttributeStyle(style, CSSPropertyHeight, size - 2, CSSPrimitiveValue::CSS_PX);
    } else
        HTMLElement::collectStyleForPresentationAttribute(name, value, style);
}

}
