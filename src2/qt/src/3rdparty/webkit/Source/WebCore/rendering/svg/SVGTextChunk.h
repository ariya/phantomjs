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

#ifndef SVGTextChunk_h
#define SVGTextChunk_h

#if ENABLE(SVG)
#include "SVGRenderStyleDefs.h"
#include "SVGTextContentElement.h"

namespace WebCore {

class SVGInlineTextBox;

// A SVGTextChunk describes a range of SVGTextFragments, see the SVG spec definition of a "text chunk".
class SVGTextChunk {
public:
    enum ChunkStyle {
        DefaultStyle = 1 << 0,
        MiddleAnchor = 1 << 1,
        EndAnchor = 1 << 2,
        RightToLeftText = 1 << 3,
        VerticalText = 1 << 4,
        LengthAdjustSpacing = 1 << 5,
        LengthAdjustSpacingAndGlyphs = 1 << 6
    };

    SVGTextChunk(unsigned chunkStyle, float desiredTextLength);

    void calculateLength(float& length, unsigned& characters) const;
    float calculateTextAnchorShift(float length) const;

    bool isVerticalText() const { return m_chunkStyle & VerticalText; }
    float desiredTextLength() const { return m_desiredTextLength; }

    Vector<SVGInlineTextBox*>& boxes() { return m_boxes; }
    const Vector<SVGInlineTextBox*>& boxes() const { return m_boxes; }

    bool hasDesiredTextLength() const { return m_desiredTextLength > 0 && ((m_chunkStyle & LengthAdjustSpacing) || (m_chunkStyle & LengthAdjustSpacingAndGlyphs)); }
    bool hasTextAnchor() const {  return m_chunkStyle & RightToLeftText ? !(m_chunkStyle & EndAnchor) : (m_chunkStyle & MiddleAnchor) || (m_chunkStyle & EndAnchor); }
    bool hasLengthAdjustSpacing() const { return m_chunkStyle & LengthAdjustSpacing; }
    bool hasLengthAdjustSpacingAndGlyphs() const { return m_chunkStyle & LengthAdjustSpacingAndGlyphs; }

private:
    // Contains all SVGInlineTextBoxes this chunk spans.
    Vector<SVGInlineTextBox*> m_boxes;

    unsigned m_chunkStyle;
    float m_desiredTextLength;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
