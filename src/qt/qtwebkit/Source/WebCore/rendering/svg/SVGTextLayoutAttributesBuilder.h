/*
 * Copyright (C) Research In Motion Limited 2010-2012. All rights reserved.
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

#ifndef SVGTextLayoutAttributesBuilder_h
#define SVGTextLayoutAttributesBuilder_h

#if ENABLE(SVG)
#include "SVGTextMetricsBuilder.h"
#include <wtf/Vector.h>

namespace WebCore {

class RenderObject;
class RenderSVGInlineText;
class RenderSVGText;
class SVGTextPositioningElement;

// SVGTextLayoutAttributesBuilder performs the first layout phase for SVG text.
//
// It extracts the x/y/dx/dy/rotate values from the SVGTextPositioningElements in the DOM.
// These values are propagated to the corresponding RenderSVGInlineText renderers.
// The first layout phase only extracts the relevant information needed in RenderBlockLineLayout
// to create the InlineBox tree based on text chunk boundaries & BiDi information.
// The second layout phase is carried out by SVGTextLayoutEngine.

class SVGTextLayoutAttributesBuilder {
    WTF_MAKE_NONCOPYABLE(SVGTextLayoutAttributesBuilder);
public:
    SVGTextLayoutAttributesBuilder();
    bool buildLayoutAttributesForForSubtree(RenderSVGText*);
    void buildLayoutAttributesForTextRenderer(RenderSVGInlineText*);

    void rebuildMetricsForTextRenderer(RenderSVGInlineText*);

    // Invoked whenever the underlying DOM tree changes, so that m_textPositions is rebuild.
    void clearTextPositioningElements() { m_textPositions.clear(); }
    unsigned numberOfTextPositioningElements() const { return m_textPositions.size(); }

private:
    struct TextPosition {
        TextPosition(SVGTextPositioningElement* newElement = 0, unsigned newStart = 0, unsigned newLength = 0)
            : element(newElement)
            , start(newStart)
            , length(newLength)
        {
        }

        SVGTextPositioningElement* element;
        unsigned start;
        unsigned length;
    };

    void buildCharacterDataMap(RenderSVGText*);
    void collectTextPositioningElements(RenderObject*, const UChar*& lastCharacter);
    void fillCharacterDataMap(const TextPosition&);

private:
    unsigned m_textLength;
    Vector<TextPosition> m_textPositions;
    SVGCharacterDataMap m_characterDataMap;
    SVGTextMetricsBuilder m_metricsBuilder;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
