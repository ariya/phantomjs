/*
 * Copyright (C) 2007 Rob Buis <buis@kde.org>
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

#ifndef SVGInlineTextBox_h
#define SVGInlineTextBox_h

#if ENABLE(SVG)
#include "InlineTextBox.h"
#include "SVGTextLayoutEngine.h"

namespace WebCore {

class RenderSVGResource;
class SVGRootInlineBox;

class SVGInlineTextBox : public InlineTextBox {
public:
    SVGInlineTextBox(RenderObject*);

    virtual bool isSVGInlineTextBox() const { return true; }

    virtual int virtualLogicalHeight() const { return m_logicalHeight; }
    void setLogicalHeight(int height) { m_logicalHeight = height; }

    virtual int selectionTop() { return m_y; }
    virtual int selectionHeight() { return m_logicalHeight; }
    virtual int offsetForPosition(float x, bool includePartialGlyphs = true) const;
    virtual float positionForOffset(int offset) const;

    void paintSelectionBackground(PaintInfo&);
    virtual void paint(PaintInfo&, int tx, int ty, int lineTop, int lineBottom);
    virtual IntRect selectionRect(int absx, int absy, int startPosition, int endPosition);

    bool mapStartEndPositionsIntoFragmentCoordinates(const SVGTextFragment&, int& startPosition, int& endPosition) const;

    virtual IntRect calculateBoundaries() const;

    void clearTextFragments() { m_textFragments.clear(); }
    Vector<SVGTextFragment>& textFragments() { return m_textFragments; }
    const Vector<SVGTextFragment>& textFragments() const { return m_textFragments; }

    bool startsNewTextChunk() const { return m_startsNewTextChunk; }
    void setStartsNewTextChunk(bool newTextChunk) { m_startsNewTextChunk = newTextChunk; }

    int offsetForPositionInFragment(const SVGTextFragment&, float position, bool includePartialGlyphs) const;
    FloatRect selectionRectForTextFragment(const SVGTextFragment&, int fragmentStartPosition, int fragmentEndPosition, RenderStyle*);

private:
    TextRun constructTextRun(RenderStyle*, const SVGTextFragment&) const;

    bool acquirePaintingResource(GraphicsContext*&, float scalingFactor, RenderObject*, RenderStyle*);
    void releasePaintingResource(GraphicsContext*&, const Path*);

    bool prepareGraphicsContextForTextPainting(GraphicsContext*&, float scalingFactor, TextRun&, RenderStyle*);
    void restoreGraphicsContextAfterTextPainting(GraphicsContext*&, TextRun&);

    void paintDecoration(GraphicsContext*, ETextDecoration, const SVGTextFragment&);
    void paintDecorationWithStyle(GraphicsContext*, ETextDecoration, const SVGTextFragment&, RenderObject* decorationRenderer);
    void paintTextWithShadows(GraphicsContext*, RenderStyle*, TextRun&, const SVGTextFragment&, int startPosition, int endPosition);
    void paintText(GraphicsContext*, RenderStyle*, RenderStyle* selectionStyle, const SVGTextFragment&, bool hasSelection, bool paintSelectedTextOnly);

private:
    int m_logicalHeight;
    int m_paintingResourceMode;
    bool m_startsNewTextChunk : 1;
    RenderSVGResource* m_paintingResource;
    Vector<SVGTextFragment> m_textFragments;
};

} // namespace WebCore

#endif
#endif // SVGInlineTextBox_h
