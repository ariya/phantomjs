/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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
#include "SVGFontFaceElement.h"

#include "Attribute.h"
#include "CSSFontFaceRule.h"
#include "CSSFontFaceSrcValue.h"
#include "CSSParser.h"
#include "CSSProperty.h"
#include "CSSPropertyNames.h"
#include "CSSStyleSelector.h"
#include "CSSStyleSheet.h"
#include "CSSValueKeywords.h"
#include "CSSValueList.h"
#include "Document.h"
#include "Font.h"
#include "SVGFontElement.h"
#include "SVGFontFaceSrcElement.h"
#include "SVGGlyphElement.h"
#include "SVGNames.h"
#include <math.h>

namespace WebCore {

using namespace SVGNames;

inline SVGFontFaceElement::SVGFontFaceElement(const QualifiedName& tagName, Document* document)
    : SVGElement(tagName, document)
    , m_fontFaceRule(CSSFontFaceRule::create())
    , m_styleDeclaration(CSSMutableStyleDeclaration::create())
{
    m_styleDeclaration->setParent(document->mappedElementSheet());
    m_styleDeclaration->setStrictParsing(true);
    m_fontFaceRule->setDeclaration(m_styleDeclaration.get());
}

PassRefPtr<SVGFontFaceElement> SVGFontFaceElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFontFaceElement(tagName, document));
}

static int cssPropertyIdForSVGAttributeName(const QualifiedName& attrName)
{
    if (!attrName.namespaceURI().isNull())
        return 0;
    
    static HashMap<AtomicStringImpl*, int>* propertyNameToIdMap = 0;
    if (!propertyNameToIdMap) {
        propertyNameToIdMap = new HashMap<AtomicStringImpl*, int>;
        // This is a list of all @font-face CSS properties which are exposed as SVG XML attributes
        // Those commented out are not yet supported by WebCore's style system
        // mapAttributeToCSSProperty(propertyNameToIdMap, accent_heightAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, alphabeticAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, ascentAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, bboxAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, cap_heightAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, descentAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, font_familyAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, font_sizeAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, font_stretchAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, font_styleAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, font_variantAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, font_weightAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, hangingAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, ideographicAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, mathematicalAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, overline_positionAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, overline_thicknessAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, panose_1Attr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, slopeAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, stemhAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, stemvAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, strikethrough_positionAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, strikethrough_thicknessAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, underline_positionAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, underline_thicknessAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, unicode_rangeAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, units_per_emAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, v_alphabeticAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, v_hangingAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, v_ideographicAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, v_mathematicalAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, widthsAttr);
        // mapAttributeToCSSProperty(propertyNameToIdMap, x_heightAttr);
    }
    
    return propertyNameToIdMap->get(attrName.localName().impl());
}

void SVGFontFaceElement::parseMappedAttribute(Attribute* attr)
{    
    int propId = cssPropertyIdForSVGAttributeName(attr->name());
    if (propId > 0) {
        m_styleDeclaration->setProperty(propId, attr->value(), false);
        rebuildFontFace();
        return;
    }
    
    SVGElement::parseMappedAttribute(attr);
}

unsigned SVGFontFaceElement::unitsPerEm() const
{
    const AtomicString& value = getAttribute(units_per_emAttr);
    if (value.isEmpty())
        return gDefaultUnitsPerEm;

    return static_cast<unsigned>(ceilf(value.toFloat()));
}

int SVGFontFaceElement::xHeight() const
{
    return static_cast<int>(ceilf(getAttribute(x_heightAttr).toFloat()));
}

float SVGFontFaceElement::horizontalOriginX() const
{
    if (!m_fontElement)
        return 0.0f;

    // Spec: The X-coordinate in the font coordinate system of the origin of a glyph to be used when
    // drawing horizontally oriented text. (Note that the origin applies to all glyphs in the font.)
    // If the attribute is not specified, the effect is as if a value of "0" were specified.
    return m_fontElement->getAttribute(horiz_origin_xAttr).toFloat();
}

float SVGFontFaceElement::horizontalOriginY() const
{
    if (!m_fontElement)
        return 0.0f;

    // Spec: The Y-coordinate in the font coordinate system of the origin of a glyph to be used when
    // drawing horizontally oriented text. (Note that the origin applies to all glyphs in the font.)
    // If the attribute is not specified, the effect is as if a value of "0" were specified.
    return m_fontElement->getAttribute(horiz_origin_yAttr).toFloat();
}

float SVGFontFaceElement::horizontalAdvanceX() const
{
    if (!m_fontElement)
        return 0.0f;

    // Spec: The default horizontal advance after rendering a glyph in horizontal orientation. Glyph
    // widths are required to be non-negative, even if the glyph is typically rendered right-to-left,
    // as in Hebrew and Arabic scripts.
    return m_fontElement->getAttribute(horiz_adv_xAttr).toFloat();
}

float SVGFontFaceElement::verticalOriginX() const
{
    if (!m_fontElement)
        return 0.0f;

    // Spec: The default X-coordinate in the font coordinate system of the origin of a glyph to be used when
    // drawing vertically oriented text. If the attribute is not specified, the effect is as if the attribute
    // were set to half of the effective value of attribute horiz-adv-x.
    const AtomicString& value = m_fontElement->getAttribute(vert_origin_xAttr);
    if (value.isEmpty())
        return horizontalAdvanceX() / 2.0f;

    return value.toFloat();
}

float SVGFontFaceElement::verticalOriginY() const
{
    if (!m_fontElement)
        return 0.0f;

    // Spec: The default Y-coordinate in the font coordinate system of the origin of a glyph to be used when
    // drawing vertically oriented text. If the attribute is not specified, the effect is as if the attribute
    // were set to the position specified by the font's ascent attribute.             
    const AtomicString& value = m_fontElement->getAttribute(vert_origin_yAttr);
    if (value.isEmpty())
        return ascent();

    return value.toFloat();
}

float SVGFontFaceElement::verticalAdvanceY() const
{
    if (!m_fontElement)
        return 0.0f;

    // Spec: The default vertical advance after rendering a glyph in vertical orientation. If the attribute is
    // not specified, the effect is as if a value equivalent of one em were specified (see units-per-em).                    
    const AtomicString& value = m_fontElement->getAttribute(vert_adv_yAttr);
       if (value.isEmpty())
        return 1.0f;

    return value.toFloat();
}

int SVGFontFaceElement::ascent() const
{
    // Spec: Same syntax and semantics as the 'ascent' descriptor within an @font-face rule. The maximum
    // unaccented height of the font within the font coordinate system. If the attribute is not specified,
    // the effect is as if the attribute were set to the difference between the units-per-em value and the
    // vert-origin-y value for the corresponding font.
    const AtomicString& ascentValue = getAttribute(ascentAttr);
    if (!ascentValue.isEmpty())
        return static_cast<int>(ceilf(ascentValue.toFloat()));

    if (m_fontElement) {
        const AtomicString& vertOriginY = m_fontElement->getAttribute(vert_origin_yAttr);
        if (!vertOriginY.isEmpty())
            return static_cast<int>(unitsPerEm()) - static_cast<int>(ceilf(vertOriginY.toFloat()));
    }

    // Match Batiks default value
    return static_cast<int>(ceilf(unitsPerEm() * 0.8f));
}

int SVGFontFaceElement::descent() const
{
    // Spec: Same syntax and semantics as the 'descent' descriptor within an @font-face rule. The maximum
    // unaccented depth of the font within the font coordinate system. If the attribute is not specified,
    // the effect is as if the attribute were set to the vert-origin-y value for the corresponding font.
    const AtomicString& descentValue = getAttribute(descentAttr);
    if (!descentValue.isEmpty()) {
        // 14 different W3C SVG 1.1 testcases use a negative descent value,
        // where a positive was meant to be used  Including:
        // animate-elem-24-t.svg, fonts-elem-01-t.svg, fonts-elem-02-t.svg (and 11 others)
        int descent = static_cast<int>(ceilf(descentValue.toFloat()));
        return descent < 0 ? -descent : descent;
    }

    if (m_fontElement) {
        const AtomicString& vertOriginY = m_fontElement->getAttribute(vert_origin_yAttr);
        if (!vertOriginY.isEmpty())
            return static_cast<int>(ceilf(vertOriginY.toFloat()));
    }

    // Match Batiks default value
    return static_cast<int>(ceilf(unitsPerEm() * 0.2f));
}

String SVGFontFaceElement::fontFamily() const
{
    return m_styleDeclaration->getPropertyValue(CSSPropertyFontFamily);
}

SVGFontElement* SVGFontFaceElement::associatedFontElement() const
{
    return m_fontElement.get();
}

void SVGFontFaceElement::rebuildFontFace()
{
    if (!inDocument())
        return;

    // we currently ignore all but the first src element, alternatively we could concat them
    SVGFontFaceSrcElement* srcElement = 0;

    for (Node* child = firstChild(); child && !srcElement; child = child->nextSibling()) {
        if (child->hasTagName(font_face_srcTag))
            srcElement = static_cast<SVGFontFaceSrcElement*>(child);
    }

    bool describesParentFont = parentNode()->hasTagName(SVGNames::fontTag);
    RefPtr<CSSValueList> list;

    if (describesParentFont) {
        m_fontElement = static_cast<SVGFontElement*>(parentNode());

        list = CSSValueList::createCommaSeparated();
        list->append(CSSFontFaceSrcValue::createLocal(fontFamily()));
    } else {
        m_fontElement = 0;
        if (srcElement)
            list = srcElement->srcValue();
    }

    if (!list || !list->length())
        return;

    // Parse in-memory CSS rules
    CSSProperty srcProperty(CSSPropertySrc, list);
    const CSSProperty* srcPropertyRef = &srcProperty;
    m_styleDeclaration->addParsedProperties(&srcPropertyRef, 1);

    if (describesParentFont) {    
        // Traverse parsed CSS values and associate CSSFontFaceSrcValue elements with ourselves.
        RefPtr<CSSValue> src = m_styleDeclaration->getPropertyCSSValue(CSSPropertySrc);
        CSSValueList* srcList = static_cast<CSSValueList*>(src.get());

        unsigned srcLength = srcList ? srcList->length() : 0;
        for (unsigned i = 0; i < srcLength; i++) {
            if (CSSFontFaceSrcValue* item = static_cast<CSSFontFaceSrcValue*>(srcList->itemWithoutBoundsCheck(i)))
                item->setSVGFontFaceElement(this);
        }
    }

    document()->styleSelectorChanged(DeferRecalcStyle);
}

void SVGFontFaceElement::insertedIntoDocument()
{
    SVGElement::insertedIntoDocument();
    document()->mappedElementSheet()->append(m_fontFaceRule);
    m_fontFaceRule->setParent(document()->mappedElementSheet());
    rebuildFontFace();
}

void SVGFontFaceElement::removedFromDocument()
{
    removeFromMappedElementSheet();
    SVGElement::removedFromDocument();
}

void SVGFontFaceElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    SVGElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);
    rebuildFontFace();
}

void SVGFontFaceElement::removeFromMappedElementSheet()
{
    CSSStyleSheet* mappedElementSheet = document()->mappedElementSheet();
    if (!mappedElementSheet)
        return;

    for (unsigned i = 0; i < mappedElementSheet->length(); ++i) {
        if (mappedElementSheet->item(i) == m_fontFaceRule) {
            mappedElementSheet->remove(i);
            break;
        }
    }
    document()->styleSelectorChanged(DeferRecalcStyle);
}

} // namespace WebCore

#endif // ENABLE(SVG_FONTS)
