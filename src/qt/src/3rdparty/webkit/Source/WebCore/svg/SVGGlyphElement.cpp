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
#include "SVGPathParserFactory.h"
#include "SimpleFontData.h"
#include "XMLNames.h"

namespace WebCore {

using namespace SVGNames;

inline SVGGlyphElement::SVGGlyphElement(const QualifiedName& tagName, Document* document)
    : SVGStyledElement(tagName, document)
{
}

PassRefPtr<SVGGlyphElement> SVGGlyphElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGGlyphElement(tagName, document));
}

void SVGGlyphElement::invalidateGlyphCache()
{
    ContainerNode* fontNode = parentNode();
    if (fontNode && fontNode->hasTagName(SVGNames::fontTag)) {
        if (SVGFontElement* element = static_cast<SVGFontElement*>(fontNode))
            element->invalidateGlyphCache();
    }
}

void SVGGlyphElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == SVGNames::dAttr)
        invalidateGlyphCache();
    else
        SVGStyledElement::parseMappedAttribute(attr);
}

void SVGGlyphElement::insertedIntoDocument()
{
    invalidateGlyphCache();
    SVGStyledElement::insertedIntoDocument();
}

void SVGGlyphElement::removedFromDocument()
{
    invalidateGlyphCache();
    SVGStyledElement::removedFromDocument();
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

static inline Path parsePathData(const AtomicString& value)
{
    Path result;
    SVGPathParserFactory* factory = SVGPathParserFactory::self();
    factory->buildPathFromString(value, result);
    return result;
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
    AtomicString value(element->getAttribute(name));
    if (value.isEmpty())
        return SVGGlyph::inheritedValue();

    return value.toFloat();
}

AttributeToPropertyTypeMap& SVGGlyphElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGGlyphElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();

    SVGStyledElement::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);
    attributeToPropertyTypeMap.set(SVGNames::dAttr, AnimatedPath);
}

SVGGlyph SVGGlyphElement::buildGenericGlyphIdentifier(const SVGElement* element)
{
    SVGGlyph identifier;
    identifier.pathData = parsePathData(element->getAttribute(dAttr));
 
    // Spec: The horizontal advance after rendering the glyph in horizontal orientation.
    // If the attribute is not specified, the effect is as if the attribute were set to the
    // value of the font's horiz-adv-x attribute. Glyph widths are required to be non-negative,
    // even if the glyph is typically rendered right-to-left, as in Hebrew and Arabic scripts.
    identifier.horizontalAdvanceX = parseSVGGlyphAttribute(element, horiz_adv_xAttr);

    // Spec: The X-coordinate in the font coordinate system of the origin of the glyph to be
    // used when drawing vertically oriented text. If the attribute is not specified, the effect
    // is as if the attribute were set to the value of the font's vert-origin-x attribute.
    identifier.verticalOriginX = parseSVGGlyphAttribute(element, vert_origin_xAttr);

    // Spec: The Y-coordinate in the font coordinate system of the origin of a glyph to be
    // used when drawing vertically oriented text. If the attribute is not specified, the effect
    // is as if the attribute were set to the value of the font's vert-origin-y attribute.
    identifier.verticalOriginY = parseSVGGlyphAttribute(element, vert_origin_yAttr);

    // Spec: The vertical advance after rendering a glyph in vertical orientation.
    // If the attribute is not specified, the effect is as if the attribute were set to the
    // value of the font's vert-adv-y attribute.
    identifier.verticalAdvanceY = parseSVGGlyphAttribute(element, vert_adv_yAttr);

    return identifier;
}

SVGGlyph SVGGlyphElement::buildGlyphIdentifier() const
{
    SVGGlyph identifier(buildGenericGlyphIdentifier(this));
    identifier.glyphName = getAttribute(glyph_nameAttr);
    identifier.orientation = parseOrientation(getAttribute(orientationAttr));
    identifier.arabicForm = parseArabicForm(getAttribute(arabic_formAttr));

    String language = getAttribute(SVGNames::langAttr);
    if (!language.isEmpty())
        identifier.languages = parseDelimitedString(language, ',');

    return identifier;
}

}

#endif // ENABLE(SVG_FONTS)
