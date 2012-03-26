/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#if ENABLE(SVG_FONTS)
#include "SVGVKernElement.h"

#include "SVGFontData.h"
#include "SVGFontElement.h"
#include "SVGFontFaceElement.h"
#include "SVGNames.h"
#include "SimpleFontData.h"
#include "XMLNames.h"

namespace WebCore {

using namespace SVGNames;

inline SVGVKernElement::SVGVKernElement(const QualifiedName& tagName, Document* document)
    : SVGElement(tagName, document)
{
}

PassRefPtr<SVGVKernElement> SVGVKernElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGVKernElement(tagName, document));
}

void SVGVKernElement::insertedIntoDocument()
{
    ContainerNode* fontNode = parentNode();
    if (fontNode && fontNode->hasTagName(SVGNames::fontTag)) {
        if (SVGFontElement* element = static_cast<SVGFontElement*>(fontNode))
            element->invalidateGlyphCache();
    }
    SVGElement::insertedIntoDocument();
}

void SVGVKernElement::removedFromDocument()
{
    ContainerNode* fontNode = parentNode();
    if (fontNode && fontNode->hasTagName(SVGNames::fontTag)) {
        if (SVGFontElement* element = static_cast<SVGFontElement*>(fontNode))
            element->invalidateGlyphCache();
    }
    SVGElement::removedFromDocument();
}

void SVGVKernElement::buildVerticalKerningPair(KerningPairVector& kerningPairs)
{
    String u1 = getAttribute(u1Attr);
    String g1 = getAttribute(g1Attr);
    String u2 = getAttribute(u2Attr);
    String g2 = getAttribute(g2Attr);
    if ((u1.isEmpty() && g1.isEmpty()) || (u2.isEmpty() && g2.isEmpty()))
        return;

    SVGKerningPair kerningPair;
    if (parseGlyphName(g1, kerningPair.glyphName1)
        && parseGlyphName(g2, kerningPair.glyphName2)
        && parseKerningUnicodeString(u1, kerningPair.unicodeRange1, kerningPair.unicodeName1)
        && parseKerningUnicodeString(u2, kerningPair.unicodeRange2, kerningPair.unicodeName2)) {
        kerningPair.kerning = getAttribute(kAttr).string().toFloat();
        kerningPairs.append(kerningPair);
    }
}

}

#endif // ENABLE(SVG_FONTS)
