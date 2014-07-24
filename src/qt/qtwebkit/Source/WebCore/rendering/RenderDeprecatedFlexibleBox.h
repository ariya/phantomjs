/*
 * This file is part of the render object implementation for KHTML.
 *
 * Copyright (C) 2003 Apple Computer, Inc.
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
 *
 */

#ifndef RenderDeprecatedFlexibleBox_h
#define RenderDeprecatedFlexibleBox_h

#include "RenderBlock.h"

namespace WebCore {

class FlexBoxIterator;

class RenderDeprecatedFlexibleBox : public RenderBlock {
public:
    RenderDeprecatedFlexibleBox(Element*);
    virtual ~RenderDeprecatedFlexibleBox();

    static RenderDeprecatedFlexibleBox* createAnonymous(Document*);

    virtual const char* renderName() const;

    virtual void styleWillChange(StyleDifference, const RenderStyle* newStyle) OVERRIDE;

    virtual void layoutBlock(bool relayoutChildren, LayoutUnit pageHeight = 0);
    void layoutHorizontalBox(bool relayoutChildren);
    void layoutVerticalBox(bool relayoutChildren);

    virtual bool avoidsFloats() const { return true; }
    virtual bool isDeprecatedFlexibleBox() const { return true; }
    virtual bool isStretchingChildren() const { return m_stretchingChildren; }
    virtual bool canCollapseAnonymousBlockChild() const OVERRIDE { return false; }

    void placeChild(RenderBox* child, const LayoutPoint& location, LayoutSize* childLayoutDelta = 0);

protected:
    virtual void computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const OVERRIDE;
    virtual void computePreferredLogicalWidths() OVERRIDE;

    LayoutUnit allowedChildFlex(RenderBox* child, bool expanding, unsigned group);

    bool hasMultipleLines() const { return style()->boxLines() == MULTIPLE; }
    bool isVertical() const { return style()->boxOrient() == VERTICAL; }
    bool isHorizontal() const { return style()->boxOrient() == HORIZONTAL; }

    bool m_stretchingChildren;

private:
    void applyLineClamp(FlexBoxIterator&, bool relayoutChildren);
    void clearLineClamp();
};

} // namespace WebCore

#endif // RenderDeprecatedFlexibleBox_h
