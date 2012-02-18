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

#ifndef SVGFontElement_h
#define SVGFontElement_h

#if ENABLE(SVG_FONTS)
#include "SVGAnimatedBoolean.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGGlyphElement.h"
#include "SVGGlyphMap.h"
#include "SVGParserUtilities.h"
#include "SVGStyledElement.h"

namespace WebCore {

// Describe an SVG <hkern>/<vkern> element
struct SVGKerningPair {
    float kerning;
    UnicodeRanges unicodeRange1;
    UnicodeRanges unicodeRange2;
    HashSet<String> unicodeName1;
    HashSet<String> unicodeName2;
    HashSet<String> glyphName1;
    HashSet<String> glyphName2;
    
    SVGKerningPair()
        : kerning(0.0f)
    {
    }
};

typedef Vector<SVGKerningPair> KerningPairVector;

class SVGMissingGlyphElement;    

class SVGFontElement : public SVGStyledElement
                     , public SVGExternalResourcesRequired {
public:
    static PassRefPtr<SVGFontElement> create(const QualifiedName&, Document*);

    void invalidateGlyphCache();

    void getGlyphIdentifiersForString(const String&, Vector<SVGGlyph>&) const;

    float horizontalKerningForPairOfStringsAndGlyphs(const String& u1, const String& g1, const String& u2, const String& g2) const;
    float verticalKerningForPairOfStringsAndGlyphs(const String& u1, const String& g1, const String& u2, const String& g2) const;
    
    SVGMissingGlyphElement* firstMissingGlyphElement() const;

private:
    SVGFontElement(const QualifiedName&, Document*);

    virtual void synchronizeProperty(const QualifiedName&);
    virtual bool rendererIsNeeded(RenderStyle*) { return false; }  
    virtual void fillAttributeToPropertyTypeMap();
    virtual AttributeToPropertyTypeMap& attributeToPropertyTypeMap();

    void ensureGlyphCache() const;

    // Animated property declarations

    // SVGExternalResourcesRequired
    DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)

    mutable KerningPairVector m_horizontalKerningPairs;
    mutable KerningPairVector m_verticalKerningPairs;
    mutable SVGGlyphMap m_glyphMap;
    mutable bool m_isGlyphCacheValid;
};

} // namespace WebCore

#endif // ENABLE(SVG_FONTS)
#endif
