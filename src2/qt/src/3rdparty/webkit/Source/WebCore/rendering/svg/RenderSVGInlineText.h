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
#include "RenderText.h"
#include "SVGTextLayoutAttributes.h"

namespace WebCore {

class SVGInlineTextBox;

class RenderSVGInlineText : public RenderText {
public:
    RenderSVGInlineText(Node*, PassRefPtr<StringImpl>);

    bool characterStartsNewTextChunk(int position) const;

    SVGTextLayoutAttributes& layoutAttributes() { return m_attributes; }
    const SVGTextLayoutAttributes& layoutAttributes() const { return m_attributes; }
    void storeLayoutAttributes(const SVGTextLayoutAttributes& attributes) { m_attributes = attributes; }

    float scalingFactor() const { return m_scalingFactor; }
    const Font& scaledFont() const { return m_scaledFont; }
    void updateScaledFont();
    static void computeNewScaledFontForStyle(RenderObject*, const RenderStyle*, float& scalingFactor, Font& scaledFont);

private:
    virtual const char* renderName() const { return "RenderSVGInlineText"; }

    virtual void destroy();
    virtual void styleDidChange(StyleDifference, const RenderStyle*);

    // FIXME: We need objectBoundingBox for DRT results and filters at the moment.
    // This should be fixed to give back the objectBoundingBox of the text root.
    virtual FloatRect objectBoundingBox() const { return FloatRect(); }

    virtual bool requiresLayer() const { return false; }
    virtual bool isSVGInlineText() const { return true; }

    virtual VisiblePosition positionForPoint(const IntPoint&);
    virtual IntRect localCaretRect(InlineBox*, int caretOffset, int* extraWidthToEndOfLine = 0);
    virtual IntRect linesBoundingBox() const;
    virtual InlineTextBox* createTextBox();

    float m_scalingFactor;
    Font m_scaledFont;
    SVGTextLayoutAttributes m_attributes;
};

inline RenderSVGInlineText* toRenderSVGInlineText(RenderObject* object)
{
    ASSERT(!object || object->isSVGInlineText());
    return static_cast<RenderSVGInlineText*>(object);
}

inline const RenderSVGInlineText* toRenderSVGInlineText(const RenderObject* object)
{
    ASSERT(!object || object->isSVGInlineText());
    return static_cast<const RenderSVGInlineText*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderSVGInlineText(const RenderSVGInlineText*);

}

#endif // ENABLE(SVG)
#endif // RenderSVGInlineText_h
