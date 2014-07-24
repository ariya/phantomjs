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

// Animated property definitions
DEFINE_ANIMATED_BOOLEAN(SVGFontElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGFontElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGStyledElement)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGFontElement::SVGFontElement(const QualifiedName& tagName, Document* document)
    : SVGStyledElement(tagName, document) 
    , m_missingGlyph(0)
    , m_isGlyphCacheValid(false)
{
    ASSERT(hasTagName(SVGNames::fontTag));
    registerAnimatedPropertiesForSVGFontElement();
}

PassRefPtr<SVGFontElement> SVGFontElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFontElement(tagName, document));
}

void SVGFontElement::invalidateGlyphCache()
{
    if (m_isGlyphCacheValid) {
        m_glyphMap.clear();
        m_horizontalKerningMap.clear();
        m_verticalKerningMap.clear();
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

void SVGFontElement::registerLigaturesInGlyphCache(Vector<String>& ligatures)
{
    ASSERT(!ligatures.isEmpty());

    // Register each character of a ligature in the map, if not present.
    // Eg. If only a "fi" ligature is present, but not "f" and "i", the
    // GlyphPage will not contain any entries for "f" and "i", so the
    // SVGFont is not used to render the text "fi1234". Register an
    // empty SVGGlyph with the character, so the SVG Font will be used
    // to render the text. If someone tries to render "f2" the SVG Font
    // will not be able to find a glyph for "f", but handles the fallback
    // character substitution properly through glyphDataForCharacter().
    Vector<SVGGlyph> glyphs;
    size_t ligaturesSize = ligatures.size();
    for (size_t i = 0; i < ligaturesSize; ++i) {
        const String& unicode = ligatures[i];

        unsigned unicodeLength = unicode.length();
        ASSERT(unicodeLength > 1);

        const UChar* characters = unicode.characters();
        for (unsigned i = 0; i < unicodeLength; ++i) {
            String lookupString(characters + i, 1);
            m_glyphMap.collectGlyphsForString(lookupString, glyphs);
            if (!glyphs.isEmpty()) {
                glyphs.clear();
                continue;
            }
                
            // This glyph is never meant to be used for rendering, only as identifier as a part of a ligature.
            SVGGlyph newGlyphPart;
            newGlyphPart.isPartOfLigature = true;
            m_glyphMap.addGlyph(String(), lookupString, newGlyphPart);
        }
    }
}

void SVGFontElement::ensureGlyphCache()
{
    if (m_isGlyphCacheValid)
        return;

    SVGMissingGlyphElement* firstMissingGlyphElement = 0;
    Vector<String> ligatures;
    for (Node* child = firstChild(); child; child = child->nextSibling()) {
        if (child->hasTagName(SVGNames::glyphTag)) {
            SVGGlyphElement* glyph = static_cast<SVGGlyphElement*>(child);
            AtomicString unicode = glyph->fastGetAttribute(SVGNames::unicodeAttr);
            AtomicString glyphId = glyph->getIdAttribute();
            if (glyphId.isEmpty() && unicode.isEmpty())
                continue;

            m_glyphMap.addGlyph(glyphId, unicode, glyph->buildGlyphIdentifier());

            // Register ligatures, if needed, don't mix up with surrogate pairs though!
            if (unicode.length() > 1 && !U16_IS_SURROGATE(unicode[0]))
                ligatures.append(unicode.string());
        } else if (child->hasTagName(SVGNames::hkernTag)) {
            SVGHKernElement* hkern = static_cast<SVGHKernElement*>(child);
            hkern->buildHorizontalKerningPair(m_horizontalKerningMap);
        } else if (child->hasTagName(SVGNames::vkernTag)) {
            SVGVKernElement* vkern = static_cast<SVGVKernElement*>(child);
            vkern->buildVerticalKerningPair(m_verticalKerningMap);
        } else if (child->hasTagName(SVGNames::missing_glyphTag) && !firstMissingGlyphElement)
            firstMissingGlyphElement = static_cast<SVGMissingGlyphElement*>(child);
    }

    // Register each character of each ligature, if needed.
    if (!ligatures.isEmpty())
        registerLigaturesInGlyphCache(ligatures);

    // Register missing-glyph element, if present.
    if (firstMissingGlyphElement) {
        SVGGlyph svgGlyph = SVGGlyphElement::buildGenericGlyphIdentifier(firstMissingGlyphElement);
        m_glyphMap.appendToGlyphTable(svgGlyph);
        m_missingGlyph = svgGlyph.tableEntry;
        ASSERT(m_missingGlyph > 0);
    }

    m_isGlyphCacheValid = true;
}

void SVGKerningMap::clear()
{
    unicodeMap.clear();
    glyphMap.clear();
    kerningUnicodeRangeMap.clear();
}

void SVGKerningMap::insert(const SVGKerningPair& kerningPair)
{
    SVGKerning svgKerning;
    svgKerning.kerning = kerningPair.kerning;
    svgKerning.unicodeRange2 = kerningPair.unicodeRange2;
    svgKerning.unicodeName2 = kerningPair.unicodeName2;
    svgKerning.glyphName2 = kerningPair.glyphName2;

    HashSet<String>::const_iterator uIt = kerningPair.unicodeName1.begin();
    const HashSet<String>::const_iterator uEnd = kerningPair.unicodeName1.end();
    for (; uIt != uEnd; ++uIt) {
        if (unicodeMap.contains(*uIt))
            unicodeMap.get(*uIt)->append(svgKerning);
        else {
            OwnPtr<SVGKerningVector> newVector = adoptPtr(new SVGKerningVector);
            newVector->append(svgKerning);
            unicodeMap.add(*uIt, newVector.release());
        }
    }

    HashSet<String>::const_iterator gIt = kerningPair.glyphName1.begin();
    const HashSet<String>::const_iterator gEnd = kerningPair.glyphName1.end();
    for (; gIt != gEnd; ++gIt) {
        if (glyphMap.contains(*gIt))
            glyphMap.get(*gIt)->append(svgKerning);
        else {
            OwnPtr<SVGKerningVector> newVector = adoptPtr(new SVGKerningVector);
            newVector->append(svgKerning);
            glyphMap.add(*gIt, newVector.release());
        }
    }

    if (!kerningPair.unicodeRange1.isEmpty())
        kerningUnicodeRangeMap.append(kerningPair);
}

static inline bool stringMatchesUnicodeRange(const String& unicodeString, const UnicodeRanges& ranges)
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

    return false;
}

static inline bool stringMatchesGlyphName(const String& glyphName, const HashSet<String>& glyphValues)
{
    if (glyphName.isEmpty())
        return false;

    return glyphValues.contains(glyphName);
}

static inline bool stringMatchesUnicodeName(const String& unicodeName, const HashSet<String>& unicodeValues)
{
    if (unicodeName.isEmpty())
        return false;

    return unicodeValues.contains(unicodeName);
}

static inline bool matches(const String& u2, const String& g2, const SVGKerning& svgKerning)
{
    return stringMatchesGlyphName(g2, svgKerning.glyphName2)
        || stringMatchesUnicodeName(u2, svgKerning.unicodeName2)
        || stringMatchesUnicodeRange(u2, svgKerning.unicodeRange2);
}

static inline bool matches(const String& u1, const String& u2, const String& g2, const SVGKerningPair& svgKerningPair)
{
    return stringMatchesUnicodeRange(u1, svgKerningPair.unicodeRange1) && matches(u2, g2, svgKerningPair);
}

static inline float kerningForPairOfStringsAndGlyphs(const SVGKerningMap& kerningMap, const String& u1, const String& g1, const String& u2, const String& g2)
{
    if (!g1.isEmpty() && kerningMap.glyphMap.contains(g1)) {
        SVGKerningVector* kerningVector = kerningMap.glyphMap.get(g1);
        SVGKerningVector::const_iterator it = kerningVector->end() - 1;
        const SVGKerningVector::const_iterator begin = kerningVector->begin() - 1;
        for (; it != begin; --it) {
            if (matches(u2, g2, *it))
                return it->kerning;
        }
    }

    if (!u1.isEmpty()) {
        if (kerningMap.unicodeMap.contains(u1)) {
            SVGKerningVector* kerningVector = kerningMap.unicodeMap.get(u1);
            SVGKerningVector::const_iterator it = kerningVector->end() - 1;
            const SVGKerningVector::const_iterator begin = kerningVector->begin() - 1;
            for (; it != begin; --it) {
                if (matches(u2, g2, *it))
                    return it->kerning;
            }
        }

        if (!kerningMap.kerningUnicodeRangeMap.isEmpty()) {
            Vector<SVGKerningPair>::const_iterator it = kerningMap.kerningUnicodeRangeMap.end() - 1;
            const Vector<SVGKerningPair>::const_iterator begin = kerningMap.kerningUnicodeRangeMap.begin() - 1;
            for (; it != begin; --it) {
                if (matches(u1, u2, g2, *it))
                    return it->kerning;
            }
        }
    }

    return 0;
}
    
float SVGFontElement::horizontalKerningForPairOfStringsAndGlyphs(const String& u1, const String& g1, const String& u2, const String& g2) const
{
    if (m_horizontalKerningMap.isEmpty())
        return 0;

    return kerningForPairOfStringsAndGlyphs(m_horizontalKerningMap, u1, g1, u2, g2);
}

float SVGFontElement::verticalKerningForPairOfStringsAndGlyphs(const String& u1, const String& g1, const String& u2, const String& g2) const
{
    if (m_verticalKerningMap.isEmpty())
        return 0;

    return kerningForPairOfStringsAndGlyphs(m_verticalKerningMap, u1, g1, u2, g2);
}

void SVGFontElement::collectGlyphsForString(const String& string, Vector<SVGGlyph>& glyphs)
{
    ensureGlyphCache();
    m_glyphMap.collectGlyphsForString(string, glyphs);
}

void SVGFontElement::collectGlyphsForGlyphName(const String& glyphName, Vector<SVGGlyph>& glyphs)
{
    ensureGlyphCache();
    // FIXME: We only support glyphName -> single glyph mapping so far.
    glyphs.append(m_glyphMap.glyphIdentifierForGlyphName(glyphName));
}

SVGGlyph SVGFontElement::svgGlyphForGlyph(Glyph glyph)
{
    ensureGlyphCache();
    return m_glyphMap.svgGlyphForGlyph(glyph);
}
    
Glyph SVGFontElement::missingGlyph()
{
    ensureGlyphCache();
    return m_missingGlyph;
}

}

#endif // ENABLE(SVG_FONTS)
