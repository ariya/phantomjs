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

bool MathMLElement::mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const
{
    if (attrName == mathcolorAttr || attrName == mathbackgroundAttr
        || attrName == colorAttr || attrName == backgroundAttr
        || attrName == fontsizeAttr || attrName == fontstyleAttr
        || attrName == fontweightAttr || attrName == fontfamilyAttr) {
        result = eMathML;
        return false;
    }
    return StyledElement::mapToEntry(attrName, result);
}

void MathMLElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == mathbackgroundAttr)
        addCSSProperty(attr, CSSPropertyBackgroundColor, attr->value());
    else if (attr->name() == mathsizeAttr) {
        // The following three values of mathsize are handled in WebCore/css/mathml.css
        if (attr->value() != "normal" && attr->value() != "small" && attr->value() != "big")
            addCSSProperty(attr, CSSPropertyFontSize, attr->value());
    } else if (attr->name() == mathcolorAttr)
        addCSSProperty(attr, CSSPropertyColor, attr->value());
    // FIXME: deprecated attributes that should loose in a conflict with a non deprecated attribute
    else if (attr->name() == fontsizeAttr)
        addCSSProperty(attr, CSSPropertyFontSize, attr->value());
    else if (attr->name() == backgroundAttr)
        addCSSProperty(attr, CSSPropertyBackgroundColor, attr->value());
    else if (attr->name() == colorAttr)
        addCSSProperty(attr, CSSPropertyColor, attr->value());
    else if (attr->name() == fontstyleAttr)
        addCSSProperty(attr, CSSPropertyFontStyle, attr->value());
    else if (attr->name() == fontweightAttr)
        addCSSProperty(attr, CSSPropertyFontWeight, attr->value());
    else if (attr->name() == fontfamilyAttr)
        addCSSProperty(attr, CSSPropertyFontFamily, attr->value());
    else
        StyledElement::parseMappedAttribute(attr);
}
    
}

#endif // ENABLE(MATHML)
