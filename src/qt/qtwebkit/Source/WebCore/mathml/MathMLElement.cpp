/*
 * Copyright (C) 2009 Alex Milowski (alex@milowski.com). All rights reserved.
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 François Sausset (sausset@gmail.com). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(MATHML)

#include "MathMLElement.h"

#include "MathMLNames.h"
#include "RenderObject.h"
#include "RenderTableCell.h"

namespace WebCore {
    
using namespace MathMLNames;
    
MathMLElement::MathMLElement(const QualifiedName& tagName, Document* document)
    : StyledElement(tagName, document, CreateStyledElement)
{
}
    
PassRefPtr<MathMLElement> MathMLElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new MathMLElement(tagName, document));
}

int MathMLElement::colSpan() const
{
    if (!hasTagName(mtdTag))
        return 1;
    const AtomicString& colSpanValue = fastGetAttribute(columnspanAttr);
    return std::max(1, colSpanValue.toInt());
}

int MathMLElement::rowSpan() const
{
    if (!hasTagName(mtdTag))
        return 1;
    const AtomicString& rowSpanValue = fastGetAttribute(rowspanAttr);
    return std::max(1, rowSpanValue.toInt());
}

void MathMLElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == rowspanAttr) {
        if (renderer() && renderer()->isTableCell() && hasTagName(mtdTag))
            toRenderTableCell(renderer())->colSpanOrRowSpanChanged();
    } else if (name == columnspanAttr) {
        if (renderer() && renderer()->isTableCell() && hasTagName(mtdTag))
            toRenderTableCell(renderer())->colSpanOrRowSpanChanged();
    } else
        StyledElement::parseAttribute(name, value);
}

bool MathMLElement::isPresentationAttribute(const QualifiedName& name) const
{
    if (name == mathbackgroundAttr || name == mathsizeAttr || name == mathcolorAttr || name == fontsizeAttr || name == backgroundAttr || name == colorAttr || name == fontstyleAttr || name == fontweightAttr || name == fontfamilyAttr)
        return true;
    return StyledElement::isPresentationAttribute(name);
}

void MathMLElement::collectStyleForPresentationAttribute(const QualifiedName& name, const AtomicString& value, MutableStylePropertySet* style)
{
    if (name == mathbackgroundAttr)
        addPropertyToPresentationAttributeStyle(style, CSSPropertyBackgroundColor, value);
    else if (name == mathsizeAttr) {
        // The following three values of mathsize are handled in WebCore/css/mathml.css
        if (value != "normal" && value != "small" && value != "big")
            addPropertyToPresentationAttributeStyle(style, CSSPropertyFontSize, value);
    } else if (name == mathcolorAttr)
        addPropertyToPresentationAttributeStyle(style, CSSPropertyColor, value);
    // FIXME: deprecated attributes that should loose in a conflict with a non deprecated attribute
    else if (name == fontsizeAttr)
        addPropertyToPresentationAttributeStyle(style, CSSPropertyFontSize, value);
    else if (name == backgroundAttr)
        addPropertyToPresentationAttributeStyle(style, CSSPropertyBackgroundColor, value);
    else if (name == colorAttr)
        addPropertyToPresentationAttributeStyle(style, CSSPropertyColor, value);
    else if (name == fontstyleAttr)
        addPropertyToPresentationAttributeStyle(style, CSSPropertyFontStyle, value);
    else if (name == fontweightAttr)
        addPropertyToPresentationAttributeStyle(style, CSSPropertyFontWeight, value);
    else if (name == fontfamilyAttr)
        addPropertyToPresentationAttributeStyle(style, CSSPropertyFontFamily, value);
    else {
        ASSERT(!isPresentationAttribute(name));
        StyledElement::collectStyleForPresentationAttribute(name, value
        , style);
    }
}

}

#endif // ENABLE(MATHML)
