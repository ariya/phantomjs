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

#ifndef SVGTextChunkBuilder_h
#define SVGTextChunkBuilder_h

#if ENABLE(SVG)
#include "SVGTextChunk.h"
#include <wtf/Vector.h>

namespace WebCore {

class SVGInlineTextBox;
struct SVGTextFragment;

// SVGTextChunkBuilder performs the third layout phase for SVG text.
//
// Phase one built the layout information from the SVG DOM stored in the RenderSVGInlineText objects (SVGTextLayoutAttributes).
// Phase two performed the actual per-character layout, computing the final positions for each character, stored in the SVGInlineTextBox objects (SVGTextFragment).
// Phase three performs all modifications that have to be applied to each individual text chunk (text-anchor & textLength).

class SVGTextChunkBuilder {
    WTF_MAKE_NONCOPYABLE(SVGTextChunkBuilder);
public:
    SVGTextChunkBuilder();

    const Vector<SVGTextChunk>& textChunks() const { return m_textChunks; }
    void transformationForTextBox(SVGInlineTextBox*, AffineTransform&) const;

    void buildTextChunks(Vector<SVGInlineTextBox*>& lineLayoutBoxes);
    void layoutTextChunks(Vector<SVGInlineTextBox*>& lineLayoutBoxes);

private:
    void addTextChunk(Vector<SVGInlineTextBox*>& lineLayoutBoxes, unsigned boxPosition, unsigned boxCount);
    void processTextChunk(const SVGTextChunk&);

    void processTextLengthSpacingCorrection(bool isVerticalText, float textLengthShift, Vector<SVGTextFragment>&, unsigned& atCharacter);
    void processTextAnchorCorrection(bool isVerticalText, float textAnchorShift, Vector<SVGTextFragment>&);
    void buildSpacingAndGlyphsTransform(bool isVerticalText, float scale, const SVGTextFragment&, AffineTransform&);

private:
    Vector<SVGTextChunk> m_textChunks;
    HashMap<SVGInlineTextBox*, AffineTransform> m_textBoxTransformations;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
