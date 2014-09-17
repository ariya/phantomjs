/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2009 Apple Inc. All rights reserved.
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

#ifndef RenderReplaced_h
#define RenderReplaced_h

#include "RenderBox.h"

namespace WebCore {

class RenderReplaced : public RenderBox {
public:
    RenderReplaced(Node*);
    RenderReplaced(Node*, const IntSize& intrinsicSize);
    virtual ~RenderReplaced();

    virtual void destroy();

protected:
    virtual void layout();

    virtual IntSize intrinsicSize() const;

    virtual int computeReplacedLogicalWidth(bool includeMaxWidth = true) const;
    virtual int computeReplacedLogicalHeight() const;
    virtual int minimumReplacedHeight() const { return 0; }

    virtual void setSelectionState(SelectionState);

    bool isSelected() const;

    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);

    void setIntrinsicSize(const IntSize&);
    virtual void intrinsicSizeChanged();
    void setHasIntrinsicSize() { m_hasIntrinsicSize = true; }

    virtual void paint(PaintInfo&, int tx, int ty);
    bool shouldPaint(PaintInfo&, int& tx, int& ty);
    IntRect localSelectionRect(bool checkWhetherSelected = true) const; // This is in local coordinates, but it's a physical rect (so the top left corner is physical top left).

private:
    virtual const char* renderName() const { return "RenderReplaced"; }

    virtual bool canHaveChildren() const { return false; }

    virtual void computePreferredLogicalWidths();

    int calcAspectRatioLogicalWidth() const;
    int calcAspectRatioLogicalHeight() const;

    virtual void paintReplaced(PaintInfo&, int /*tx*/, int /*ty*/) { }

    virtual IntRect clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer);

    virtual unsigned caretMaxRenderedOffset() const;
    virtual VisiblePosition positionForPoint(const IntPoint&);
    
    virtual bool canBeSelectionLeaf() const { return true; }

    virtual IntRect selectionRectForRepaint(RenderBoxModelObject* repaintContainer, bool clipToVisibleContent = true);

    IntSize m_intrinsicSize;
    bool m_hasIntrinsicSize;
};

}

#endif
