/*
 * Copyright (C) 2011 Leo Yang <leoyang@webkit.org>
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

#if ENABLE(SVG) && ENABLE(SVG_FONTS)
#include "SVGAltGlyphDefElement.h"

#include "SVGAltGlyphItemElement.h"
#include "SVGGlyphRefElement.h"
#include "SVGNames.h"

namespace WebCore {

inline SVGAltGlyphDefElement::SVGAltGlyphDefElement(const QualifiedName& tagName, Document* document)
    : SVGElement(tagName, document)
{
    ASSERT(hasTagName(SVGNames::altGlyphDefTag));
}

PassRefPtr<SVGAltGlyphDefElement> SVGAltGlyphDefElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGAltGlyphDefElement(tagName, document));
}

bool SVGAltGlyphDefElement::hasValidGlyphElements(Vector<String>& glyphNames) const
{
    // Spec: http://www.w3.org/TR/SVG/text.html#AltGlyphDefElement
    // An 'altGlyphDef' can contain either of the following:
    //
    // 1. In the simplest case, an 'altGlyphDef' contains one or more 'glyphRef' elements.
    // Each 'glyphRef' element references a single glyph within a particular font.
    // If all of the referenced glyphs are available, then these glyphs are rendered
    // instead of the character(s) inside of the referencing 'altGlyph' element.
    // If any of the referenced glyphs are unavailable, then the character(s) that are
    // inside of the 'altGlyph' element are rendered as if there were not an 'altGlyph'
    // element surrounding those characters.
    //
    // 2. In the more complex case, an 'altGlyphDef' contains one or more 'altGlyphItem' elements.
    // Each 'altGlyphItem' represents a candidate set of substitute glyphs. Each 'altGlyphItem'
    // contains one or more 'glyphRef' elements. Each 'glyphRef' element references a single
    // glyph within a particular font. The first 'altGlyphItem' in which all referenced glyphs
    // are available is chosen. The glyphs referenced from this 'altGlyphItem' are rendered
    // instead of the character(s) that are inside of the referencing 'altGlyph' element.
    // If none of the 'altGlyphItem' elements result in a successful match (i.e., none of the
    // 'altGlyphItem' elements has all of its referenced glyphs available), then the character(s)
    // that are inside of the 'altGlyph' element are rendered as if there were not an 'altGlyph'
    // element surrounding those characters.
    //
    // The spec doesn't tell how to deal with the mixing of <glyphRef> and <altGlyItem>.
    // However, we determine content model by the the type of the first appearing element
    // just like Opera 11 does. After the content model is determined we skip elements
    // which don't comform to it. For example:
    // a.    <altGlyphDef>
    //         <glyphRef id="g1" />
    //         <altGlyphItem id="i1"> ... </altGlyphItem>
    //         <glyphRef id="g2" />
    //       </altGlyphDef>
    //
    // b.    <altGlyphDef>
    //         <altGlyphItem id="i1"> ... </altGlyphItem>
    //         <altGlyphItem id="i2"> ... </altGlyphItem>
    //         <glyphRef id="g1" />
    //         <glyphRef id="g2" />
    //       </altGlyphDef>
    // For a), the content model is 1), so we will use "g1" and "g2" if they are all valid
    // and "i1" is skipped.
    // For b), the content model is 2), so we will use <glyphRef> elements contained in
    // "i1" if they are valid and "g1" and "g2" are skipped.

    // These 2 variables are used to determine content model.
    bool fountFirstGlyphRef = false;
    bool foundFirstAltGlyphItem = false;

    for (Node* child = firstChild(); child; child = child->nextSibling()) {
        if (!foundFirstAltGlyphItem && child->hasTagName(SVGNames::glyphRefTag)) {
            fountFirstGlyphRef = true;
            String referredGlyphName;

            if (static_cast<SVGGlyphRefElement*>(child)->hasValidGlyphElement(referredGlyphName))
                glyphNames.append(referredGlyphName);
            else {
                // As the spec says "If any of the referenced glyphs are unavailable,
                // then the character(s) that are inside of the 'altGlyph' element are
                // rendered as if there were not an 'altGlyph' element surrounding
                // those characters.".
                glyphNames.clear();
                return false;
            }
        } else if (!fountFirstGlyphRef && child->hasTagName(SVGNames::altGlyphItemTag)) {
            foundFirstAltGlyphItem = true;
            Vector<String> referredGlyphNames;

            // As the spec says "The first 'altGlyphItem' in which all referenced glyphs
            // are available is chosen."
            if (static_cast<SVGAltGlyphItemElement*>(child)->hasValidGlyphElements(glyphNames) && !glyphNames.isEmpty())
                return true;
        }
    }
    return !glyphNames.isEmpty();
}

}

#endif
