/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
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
#include "SVGFontElement.h"

#include "Document.h"
#include "Font.h"
#include "GlyphPageTreeNode.h"
#include "SVGGlyphElement.h"
#include "SVGHKernElement.h"
#include "SVGMissingGlyphElement.h"
#include "SVGNames.h"
#include "SVGVKernElement.h"
#include <wtf/ASCIICType.h>

namespace WebCore {

// Animated property declarations
DEFINE_ANIMATED_BOOLEAN(SVGFontElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

inline SVGFontElement::SVGFontElement(const QualifiedName& tagName, Document* document)
    : SVGStyledElement(tagName, document) 
    , m_isGlyphCacheValid(false)
{
}

PassRefPtr<SVGFontElement> SVGFontElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFontElement(tagName, document));
}

void SVGFontElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGStyledElement::synchronizeProperty(attrName);

    if (attrName == anyQName() || SVGExternalResourcesRequired::isKnownAttribute(attrName))
        synchronizeExternalResourcesRequired();
}

void SVGFontElement::invalidateGlyphCache()
{
    if (m_isGlyphCacheValid) {
        m_glyphMap.clear();
        m_horizontalKerningPairs.clear();
        m_verticalKerningPairs.clear();
    }
    m_isGlyphCacheValid = false;
}

SVGMissingGlyphElement* SVGFontElement::firstMissingGlyphElement() const
{
    for (Node* child = firstChild(); child; child = child->nextSibling()) {
        if (child->hasTagName(SVGNames::missing_glyphTag))
            return static_cast<SVGMissingGlyphElement*>(child);
    }

    return 0;
}

void SVGFontElement::ensureGlyphCache() const
{
    if (m_isGlyphCacheValid)
        return;

    for (Node* child = firstChild(); child; child = child->nextSibling()) {
        if (child->hasTagName(SVGNames::glyphTag)) {
            SVGGlyphElement* glyph = static_cast<SVGGlyphElement*>(child);
            String unicode = glyph->getAttribute(SVGNames::unicodeAttr);
            if (unicode.length())
                m_glyphMap.add(unicode, glyph->buildGlyphIdentifier());
        } else if (child->hasTagName(SVGNames::hkernTag)) {
            SVGHKernElement* hkern = static_cast<SVGHKernElement*>(child);
            hkern->buildHorizontalKerningPair(m_horizontalKerningPairs);
        } else if (child->hasTagName(SVGNames::vkernTag)) {
            SVGVKernElement* vkern = static_cast<SVGVKernElement*>(child);
            vkern->buildVerticalKerningPair(m_verticalKerningPairs);
        }
    }
        
    m_isGlyphCacheValid = true;
}

static bool stringMatchesUnicodeRange(const String& unicodeString, const UnicodeRanges& ranges, const HashSet<String>& unicodeValues)
{
    if (unicodeString.isEmpty())
        return false;

    if (!ranges.isEmpty()) {
        UChar firstChar = unicodeString[0];
        const UnicodeRanges::const_iterator end = ranges.end();
        for (UnicodeRanges::const_iterator it = ranges.begin(); it != end; ++it) {
            if (firstChar >= it->first && firstChar <= it->second)
                return true;
        }
    }

    if (!unicodeValues.isEmpty())
        return unicodeValues.contains(unicodeString);
    
    return false;
}

static bool stringMatchesGlyphName(const String& glyphName, const HashSet<String>& glyphValues)
{
    if (glyphName.isEmpty())
        return false;

    if (!glyphValues.isEmpty())
        return glyphValues.contains(glyphName);
    
    return false;
}
    
static bool matches(const String& u1, const String& g1, const String& u2, const String& g2, const SVGKerningPair& kerningPair)
{
    if (!stringMatchesUnicodeRange(u1, kerningPair.unicodeRange1, kerningPair.unicodeName1)
        && !stringMatchesGlyphName(g1, kerningPair.glyphName1))
        return false;

    if (!stringMatchesUnicodeRange(u2, kerningPair.unicodeRange2, kerningPair.unicodeName2)
        && !stringMatchesGlyphName(g2, kerningPair.glyphName2))
        return false;

    return true;
}

static float kerningForPairOfStringsAndGlyphs(KerningPairVector& kerningPairs, const String& u1, const String& g1, const String& u2, const String& g2)
{
    KerningPairVector::const_iterator it = kerningPairs.end() - 1;
    const KerningPairVector::const_iterator begin = kerningPairs.begin() - 1;
    for (; it != begin; --it) {
        if (matches(u1, g1, u2, g2, *it))
            return it->kerning;
    }

    return 0.0f;
}
    
float SVGFontElement::horizontalKerningForPairOfStringsAndGlyphs(const String& u1, const String& g1, const String& u2, const String& g2) const
{
    if (m_horizontalKerningPairs.isEmpty())
        return 0.0f;

    return kerningForPairOfStringsAndGlyphs(m_horizontalKerningPairs, u1, g1, u2, g2);
}

float SVGFontElement::verticalKerningForPairOfStringsAndGlyphs(const String& u1, const String& g1, const String& u2, const String& g2) const
{
    if (m_verticalKerningPairs.isEmpty())
        return 0.0f;

    return kerningForPairOfStringsAndGlyphs(m_verticalKerningPairs, u1, g1, u2, g2);
}

void SVGFontElement::getGlyphIdentifiersForString(const String& string, Vector<SVGGlyph>& glyphs) const
{
    ensureGlyphCache();
    m_glyphMap.get(string, glyphs);
}

AttributeToPropertyTypeMap& SVGFontElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGFontElement::fillAttributeToPropertyTypeMap()
{
    SVGStyledElement::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap());
}

}

#endif // ENABLE(SVG_FONTS)
