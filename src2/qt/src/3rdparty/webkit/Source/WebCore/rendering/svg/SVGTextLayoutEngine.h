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

#ifndef SVGTextLayoutEngine_h
#define SVGTextLayoutEngine_h

#if ENABLE(SVG)
#include "Path.h"
#include "SVGTextChunkBuilder.h"
#include "SVGTextFragment.h"
#include "SVGTextLayoutAttributes.h"
#include "SVGTextMetrics.h"
#include <wtf/Vector.h>

namespace WebCore {

class RenderObject;
class RenderStyle;
class RenderSVGInlineText;
class SVGElement;
class SVGInlineTextBox;
class SVGRenderStyle;

// SVGTextLayoutEngine performs the second layout phase for SVG text.
//
// The InlineBox tree was created, containing the text chunk information, necessary to apply
// certain SVG specific text layout properties (text-length adjustments and text-anchor).
// The second layout phase uses the SVGTextLayoutAttributes stored in the individual
// RenderSVGInlineText renderers to compute the final positions for each character
// which are stored in the SVGInlineTextBox objects.

class SVGTextLayoutEngine {
    WTF_MAKE_NONCOPYABLE(SVGTextLayoutEngine);
public:
    SVGTextLayoutEngine(Vector<SVGTextLayoutAttributes>&);
    SVGTextChunkBuilder& chunkLayoutBuilder() { return m_chunkLayoutBuilder; }

    void beginTextPathLayout(RenderObject*, SVGTextLayoutEngine& lineLayout);
    void endTextPathLayout();

    void layoutInlineTextBox(SVGInlineTextBox*);
    void finishLayout();

private:
    void updateCharacerPositionIfNeeded(float& x, float& y);
    void updateCurrentTextPosition(float x, float y, float glyphAdvance);
    void updateRelativePositionAdjustmentsIfNeeded(Vector<float>& dxValues, Vector<float>& dyValues);

    void recordTextFragment(SVGInlineTextBox*, Vector<SVGTextMetrics>& textMetricValues);
    bool parentDefinesTextLength(RenderObject*) const;

    void layoutTextOnLineOrPath(SVGInlineTextBox*, RenderSVGInlineText*, const RenderStyle*);
    void finalizeTransformMatrices(Vector<SVGInlineTextBox*>&);

    bool currentLogicalCharacterAttributes(SVGTextLayoutAttributes&);
    bool currentLogicalCharacterMetrics(SVGTextLayoutAttributes&, SVGTextMetrics&);
    bool currentVisualCharacterMetrics(SVGInlineTextBox*, RenderSVGInlineText*, SVGTextMetrics&);

    void advanceToNextLogicalCharacter(const SVGTextMetrics&);
    void advanceToNextVisualCharacter(const SVGTextMetrics&);

private:
    Vector<SVGTextLayoutAttributes> m_layoutAttributes;
    Vector<SVGInlineTextBox*> m_lineLayoutBoxes;
    Vector<SVGInlineTextBox*> m_pathLayoutBoxes;
    SVGTextChunkBuilder m_chunkLayoutBuilder;

    SVGTextFragment m_currentTextFragment;
    unsigned m_logicalCharacterOffset;
    unsigned m_logicalMetricsListOffset;
    unsigned m_visualCharacterOffset;
    unsigned m_visualMetricsListOffset;
    float m_x;
    float m_y;
    float m_dx;
    float m_dy;
    bool m_isVerticalText;
    bool m_inPathLayout;

    // Text on path layout
    Path m_textPath;
    float m_textPathLength;
    float m_textPathStartOffset;
    float m_textPathCurrentOffset;
    float m_textPathSpacing;
    float m_textPathScaling;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
