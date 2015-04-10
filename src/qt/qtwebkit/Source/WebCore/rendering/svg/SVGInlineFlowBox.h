/*
 * Copyright (C) 2006 Oliver Hunt <ojh16@student.canterbury.ac.nz>
 * Copyright (C) 2006 Apple Computer Inc.
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

#ifndef SVGInlineFlowBox_h
#define SVGInlineFlowBox_h

#if ENABLE(SVG)
#include "InlineFlowBox.h"

namespace WebCore {

class RenderSVGInlineText;

class SVGInlineFlowBox FINAL : public InlineFlowBox {
public:
    SVGInlineFlowBox(RenderObject* obj)
        : InlineFlowBox(obj)
        , m_logicalHeight(0)
    {
    }

    virtual bool isSVGInlineFlowBox() const { return true; }
    virtual float virtualLogicalHeight() const { return m_logicalHeight; }
    void setLogicalHeight(float h) { m_logicalHeight = h; }

    void paintSelectionBackground(PaintInfo&);
    virtual void paint(PaintInfo&, const LayoutPoint&, LayoutUnit lineTop, LayoutUnit lineBottom);

    virtual FloatRect calculateBoundaries() const;

    static void computeTextMatchMarkerRectForRenderer(RenderSVGInlineText*);

private:
    float m_logicalHeight;
};

inline SVGInlineFlowBox* toSVGInlineFlowBox(InlineBox* box)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!box || box->isSVGInlineFlowBox());
    return static_cast<SVGInlineFlowBox*>(box);
}

} // namespace WebCore

#endif // ENABLE(SVG)

#endif // SVGInlineFlowBox_h
