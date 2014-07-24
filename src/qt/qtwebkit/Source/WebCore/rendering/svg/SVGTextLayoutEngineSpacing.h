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

#ifndef SVGTextLayoutEngineSpacing_h
#define SVGTextLayoutEngineSpacing_h

#if ENABLE(SVG)
#include "SVGTextMetrics.h"

namespace WebCore {

class Font;
class SVGRenderStyle;
class SVGElement;

// Helper class used by SVGTextLayoutEngine to handle 'kerning' / 'letter-spacing' and 'word-spacing'.
class SVGTextLayoutEngineSpacing {
    WTF_MAKE_NONCOPYABLE(SVGTextLayoutEngineSpacing);
public:
    SVGTextLayoutEngineSpacing(const Font&);

    float calculateSVGKerning(bool isVerticalText, const SVGTextMetrics::Glyph& currentGlyph);
    float calculateCSSKerningAndSpacing(const SVGRenderStyle*, SVGElement* lengthContext, const UChar* currentCharacter);

private:
    const Font& m_font;
    const UChar* m_lastCharacter;

#if ENABLE(SVG_FONTS)
    SVGTextMetrics::Glyph m_lastGlyph;
#endif
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
