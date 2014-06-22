/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2008 Rob Buis <buis@kde.org>
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
#include "SVGGlyphElement.h"

#include "Attribute.h"
#include "SVGFontData.h"
#include "SVGFontElement.h"
#include "SVGFontFaceElement.h"
#include "SVGNames.h"
#include "SVGPathUtilities.h"

namespace WebCore {

inline SVGGlyphElement::SVGGlyphElement(const QualifiedName& tagName, Document* document)
    : SVGStyledElement(tagName, document)
{
    ASSERT(hasTagName(SVGNames::glyphTag));
}

PassRefPtr<SVGGlyphElement> SVGGlyphElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGGlyphElement(tagName, document));
}

void SVGGlyphElement::invalidateGlyphCache()
{
    ContainerNode* fontNode = parentNode();
    if (fontNode && isSVGFontElement(fontNode))
        toSVGFontElement(fontNode)->invalidateGlyphCache();
}

void SVGGlyphElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == SVGNames::dAttr)
        invalidateGlyphCache();
    else
        SVGStyledElement::parseAttribute(name, value);
}

Node::InsertionNotificationRequest SVGGlyphElement::insertedInto(ContainerNode* rootParent)
{
    invalidateGlyphCache();
    return SVGStyledElement::insertedInto(rootParent);
}

void SVGGlyphElement::removedFrom(ContainerNode* rootParent)
{
    if (rootParent->inDocument())
        invalidateGlyphCache();
    SVGStyledElement::removedFrom(rootParent);
}

static inline SVGGlyph::ArabicForm parseArabicForm(const AtomicString& value)
{
    if (value == "medial")
        return SVGGlyph::Medial;
    if (value == "terminal")
        return SVGGlyph::Terminal;
    if (value == "isolated")
        return SVGGlyph::Isolated;
    if (value == "initial")
        return SVGGlyph::Initial;

    return SVGGlyph::None;
}

static inline SVGGlyph::Orientation parseOrientation(const AtomicString& value)
{
    if (value == "h")
        return SVGGlyph::Horizontal;
    if (value == "v")
        return SVGGlyph::Vertical;

    return SVGGlyph::Both;
}

void SVGGlyphElement::inheritUnspecifiedAttributes(SVGGlyph& identifier, const SVGFontData* svgFontData)
{
    if (identifier.horizontalAdvanceX == SVGGlyph::inheritedValue())
        identifier.horizontalAdvanceX = svgFontData->horizontalAdvanceX();

    if (identifier.verticalOriginX == SVGGlyph::inheritedValue())
        identifier.verticalOriginX = svgFontData->verticalOriginX();

    if (identifier.verticalOriginY == SVGGlyph::inheritedValue())
        identifier.verticalOriginY = svgFontData->verticalOriginY();

    if (identifier.verticalAdvanceY == SVGGlyph::inheritedValue())
        identifier.verticalAdvanceY = svgFontData->verticalAdvanceY();
}

static inline float parseSVGGlyphAttribute(const SVGElement* element, const WebCore::QualifiedName& name)
{
    AtomicString value(element->fastGetAttribute(name));
    if (value.isEmpty())
        return SVGGlyph::inheritedValue();

    return value.toFloat();
}

SVGGlyph SVGGlyphElement::buildGenericGlyphIdentifier(const SVGElement* element)
{
    SVGGlyph identifier;
    buildPathFromString(element->fastGetAttribute(SVGNames::dAttr), identifier.pathData);
 
    // Spec: The horizontal advance after rendering the glyph in horizontal orientation.
    // If the attribute is not specified, the effect is as if the attribute were set to the
    // value of the font's horiz-adv-x attribute. Glyph widths are required to be non-negative,
    // even if the glyph is typically rendered right-to-left, as in Hebrew and Arabic scripts.
    identifier.horizontalAdvanceX = parseSVGGlyphAttribute(element, SVGNames::horiz_adv_xAttr);

    // Spec: The X-coordinate in the font coordinate system of the origin of the glyph to be
    // used when drawing vertically oriented text. If the attribute is not specified, the effect
    // is as if the attribute were set to the value of the font's vert-origin-x attribute.
    identifier.verticalOriginX = parseSVGGlyphAttribute(element, SVGNames::vert_origin_xAttr);

    // Spec: The Y-coordinate in the font coordinate system of the origin of a glyph to be
    // used when drawing vertically oriented text. If the attribute is not specified, the effect
    // is as if the attribute were set to the value of the font's vert-origin-y attribute.
    identifier.verticalOriginY = parseSVGGlyphAttribute(element, SVGNames::vert_origin_yAttr);

    // Spec: The vertical advance after rendering a glyph in vertical orientation.
    // If the attribute is not specified, the effect is as if the attribute were set to the
    // value of the font's vert-adv-y attribute.
    identifier.verticalAdvanceY = parseSVGGlyphAttribute(element, SVGNames::vert_adv_yAttr);

    return identifier;
}

SVGGlyph SVGGlyphElement::buildGlyphIdentifier() const
{
    SVGGlyph identifier(buildGenericGlyphIdentifier(this));
    identifier.glyphName = fastGetAttribute(SVGNames::glyph_nameAttr);
    identifier.orientation = parseOrientation(fastGetAttribute(SVGNames::orientationAttr));
    identifier.arabicForm = parseArabicForm(fastGetAttribute(SVGNames::arabic_formAttr));

    String language = fastGetAttribute(SVGNames::langAttr);
    if (!language.isEmpty())
        identifier.languages = parseDelimitedString(language, ',');

    return identifier;
}

}

#endif // ENABLE(SVG_FONTS)
