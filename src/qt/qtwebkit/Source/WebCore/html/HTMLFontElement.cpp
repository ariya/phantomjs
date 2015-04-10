/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2003, 2006, 2008, 2010 Apple Inc. All rights reserved.
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
#include "HTMLFontElement.h"

#include "Attribute.h"
#include "CSSPropertyNames.h"
#include "CSSStyleSheet.h"
#include "CSSValueKeywords.h"
#include "CSSValueList.h"
#include "CSSValuePool.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "StylePropertySet.h"
#include <wtf/text/StringBuilder.h>

using namespace WTF;

namespace WebCore {

using namespace HTMLNames;

HTMLFontElement::HTMLFontElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
{
    ASSERT(hasTagName(fontTag));
}

PassRefPtr<HTMLFontElement> HTMLFontElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLFontElement(tagName, document));
}

// http://www.whatwg.org/specs/web-apps/current-work/multipage/rendering.html#fonts-and-colors
template <typename CharacterType>
static bool parseFontSize(const CharacterType* characters, unsigned length, int& size)
{

    // Step 1
    // Step 2
    const CharacterType* position = characters;
    const CharacterType* end = characters + length;

    // Step 3
    while (position < end) {
        if (!isHTMLSpace(*position))
            break;
        ++position;
    }

    // Step 4
    if (position == end)
        return false;
    ASSERT(position < end);

    // Step 5
    enum {
        RelativePlus,
        RelativeMinus,
        Absolute
    } mode;

    switch (*position) {
    case '+':
        mode = RelativePlus;
        ++position;
        break;
    case '-':
        mode = RelativeMinus;
        ++position;
        break;
    default:
        mode = Absolute;
        break;
    }

    // Step 6
    StringBuilder digits;
    digits.reserveCapacity(16);
    while (position < end) {
        if (!isASCIIDigit(*position))
            break;
        digits.append(*position++);
    }

    // Step 7
    if (digits.isEmpty())
        return false;

    // Step 8
    int value;

    if (digits.is8Bit())
        value = charactersToIntStrict(digits.characters8(), digits.length());
    else
        value = charactersToIntStrict(digits.characters16(), digits.length());

    // Step 9
    if (mode == RelativePlus)
        value += 3;
    else if (mode == RelativeMinus)
        value = 3 - value;

    // Step 10
    if (value > 7)
        value = 7;

    // Step 11
    if (value < 1)
        value = 1;

    size = value;
    return true;
}

static bool parseFontSize(const String& input, int& size)
{
    if (input.isEmpty())
        return false;

    if (input.is8Bit())
        return parseFontSize(input.characters8(), input.length(), size);

    return parseFontSize(input.characters16(), input.length(), size);
}

bool HTMLFontElement::cssValueFromFontSizeNumber(const String& s, CSSValueID& size)
{
    int num = 0;
    if (!parseFontSize(s, num))
        return false;

    switch (num) {
    case 1:
        // FIXME: The spec says that we're supposed to use CSSValueXxSmall here.
        size = CSSValueXSmall;
        break;
    case 2: 
        size = CSSValueSmall;
        break;
    case 3: 
        size = CSSValueMedium;
        break;
    case 4: 
        size = CSSValueLarge;
        break;
    case 5: 
        size = CSSValueXLarge;
        break;
    case 6: 
        size = CSSValueXxLarge;
        break;
    case 7:
        size = CSSValueWebkitXxxLarge;
        break;
    default:
        ASSERT_NOT_REACHED();
    }
    return true;
}

bool HTMLFontElement::isPresentationAttribute(const QualifiedName& name) const
{
    if (name == sizeAttr || name == colorAttr || name == faceAttr)
        return true;
    return HTMLElement::isPresentationAttribute(name);
}

void HTMLFontElement::collectStyleForPresentationAttribute(const QualifiedName& name, const AtomicString& value, MutableStylePropertySet* style)
{
    if (name == sizeAttr) {
        CSSValueID size = CSSValueInvalid;
        if (cssValueFromFontSizeNumber(value, size))
            addPropertyToPresentationAttributeStyle(style, CSSPropertyFontSize, size);
    } else if (name == colorAttr)
        addHTMLColorToStyle(style, CSSPropertyColor, value);
    else if (name == faceAttr) {
        if (RefPtr<CSSValueList> fontFaceValue = cssValuePool().createFontFaceValue(value))
            style->setProperty(CSSProperty(CSSPropertyFontFamily, fontFaceValue.release()));
    } else
        HTMLElement::collectStyleForPresentationAttribute(name, value, style);
}

}
