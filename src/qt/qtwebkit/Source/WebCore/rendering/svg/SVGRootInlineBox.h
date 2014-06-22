/*
 * Copyright (C) 2006 Oliver Hunt <ojh16@student.canterbury.ac.nz>
 * Copyright (C) 2006 Apple Computer Inc.
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

#ifndef SVGRootInlineBox_h
#define SVGRootInlineBox_h

#if ENABLE(SVG)
#include "RootInlineBox.h"
#include "SVGRenderSupport.h"
#include "SVGTextLayoutEngine.h"

namespace WebCore {

class SVGInlineTextBox;

class SVGRootInlineBox FINAL : public RootInlineBox {
public:
    SVGRootInlineBox(RenderBlock* block)
        : RootInlineBox(block)
        , m_logicalHeight(0)
    {
    }

    virtual bool isSVGRootInlineBox() const { return true; }

    virtual float virtualLogicalHeight() const { return m_logicalHeight; }
    void setLogicalHeight(float height) { m_logicalHeight = height; }

    virtual void paint(PaintInfo&, const LayoutPoint&, LayoutUnit lineTop, LayoutUnit lineBottom);

    void computePerCharacterLayoutInformation();

    virtual FloatRect objectBoundingBox() const { return FloatRect(); }
    virtual FloatRect repaintRectInLocalCoordinates() const { return FloatRect(); }

    InlineBox* closestLeafChildForPosition(const LayoutPoint&);

private:
    void reorderValueLists(Vector<SVGTextLayoutAttributes*>&);
    void layoutCharactersInTextBoxes(InlineFlowBox*, SVGTextLayoutEngine&);
    void layoutChildBoxes(InlineFlowBox*, FloatRect* = 0);
    void layoutRootBox(const FloatRect&);

private:
    float m_logicalHeight;
};

} // namespace WebCore

#endif // ENABLE(SVG)

#endif // SVGRootInlineBox_h
