/*
 * Copyright (C) 2006 Oliver Hunt <ojh16@student.canterbury.ac.nz>
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Rob Buis <buis@kde.org>
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

#ifndef RenderSVGInlineText_h
#define RenderSVGInlineText_h

#if ENABLE(SVG)
#include "Font.h"
#include "RenderText.h"
#include "SVGTextLayoutAttributes.h"

namespace WebCore {

class SVGInlineTextBox;

class RenderSVGInlineText : public RenderText {
public:
    RenderSVGInlineText(Node*, PassRefPtr<StringImpl>);

    bool characterStartsNewTextChunk(int position) const;
    SVGTextLayoutAttributes* layoutAttributes() { return &m_layoutAttributes; }

    float scalingFactor() const { return m_scalingFactor; }
    const Font& scaledFont() const { return m_scaledFont; }
    void updateScaledFont();
    static void computeNewScaledFontForStyle(RenderObject*, const RenderStyle*, float& scalingFactor, Font& scaledFont);

    // Preserves floating point precision for the use in DRT. It knows how to round and does a better job than enclosingIntRect.
    FloatRect floatLinesBoundingBox() const;

private:
    virtual const char* renderName() const { return "RenderSVGInlineText"; }

    virtual void setTextInternal(PassRefPtr<StringImpl>);
    virtual void styleDidChange(StyleDifference, const RenderStyle*);

    virtual FloatRect objectBoundingBox() const { return floatLinesBoundingBox(); }

    virtual bool requiresLayer() const { return false; }
    virtual bool isSVGInlineText() const { return true; }

    virtual VisiblePosition positionForPoint(const LayoutPoint&);
    virtual LayoutRect localCaretRect(InlineBox*, int caretOffset, LayoutUnit* extraWidthToEndOfLine = 0);
    virtual IntRect linesBoundingBox() const;
    virtual InlineTextBox* createTextBox();

    float m_scalingFactor;
    Font m_scaledFont;
    SVGTextLayoutAttributes m_layoutAttributes;
};

inline RenderSVGInlineText* toRenderSVGInlineText(RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isSVGInlineText());
    return static_cast<RenderSVGInlineText*>(object);
}

inline const RenderSVGInlineText* toRenderSVGInlineText(const RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isSVGInlineText());
    return static_cast<const RenderSVGInlineText*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderSVGInlineText(const RenderSVGInlineText*);

}

#endif // ENABLE(SVG)
#endif // RenderSVGInlineText_h
