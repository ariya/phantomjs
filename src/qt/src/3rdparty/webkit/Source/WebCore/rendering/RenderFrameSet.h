/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
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

#ifndef RenderFrameSet_h
#define RenderFrameSet_h

#include "RenderBox.h"

namespace WebCore {

class HTMLFrameSetElement;
class MouseEvent;

enum FrameEdge { LeftFrameEdge, RightFrameEdge, TopFrameEdge, BottomFrameEdge };

struct FrameEdgeInfo {
    FrameEdgeInfo(bool preventResize = false, bool allowBorder = true)
        : m_preventResize(4)
        , m_allowBorder(4)
    {
        m_preventResize.fill(preventResize);
        m_allowBorder.fill(allowBorder);
    }

    bool preventResize(FrameEdge edge) const { return m_preventResize[edge]; }
    bool allowBorder(FrameEdge edge) const { return m_allowBorder[edge]; }

    void setPreventResize(FrameEdge edge, bool preventResize) { m_preventResize[edge] = preventResize; }
    void setAllowBorder(FrameEdge edge, bool allowBorder) { m_allowBorder[edge] = allowBorder; }

private:
    Vector<bool> m_preventResize;
    Vector<bool> m_allowBorder;
};

class RenderFrameSet : public RenderBox {
public:
    RenderFrameSet(HTMLFrameSetElement*);
    virtual ~RenderFrameSet();

    const RenderObjectChildList* children() const { return &m_children; }
    RenderObjectChildList* children() { return &m_children; }

    FrameEdgeInfo edgeInfo() const;

    bool userResize(MouseEvent*);

    bool isResizingRow() const;
    bool isResizingColumn() const;

    bool canResizeRow(const IntPoint&) const;
    bool canResizeColumn(const IntPoint&) const;

private:
    static const int noSplit = -1;

    class GridAxis {
        WTF_MAKE_NONCOPYABLE(GridAxis);
    public:
        GridAxis();
        void resize(int);
        Vector<int> m_sizes;
        Vector<int> m_deltas;
        Vector<bool> m_preventResize;
        Vector<bool> m_allowBorder;
        int m_splitBeingResized;
        int m_splitResizeOffset;
    };

    virtual RenderObjectChildList* virtualChildren() { return children(); }
    virtual const RenderObjectChildList* virtualChildren() const { return children(); }

    virtual const char* renderName() const { return "RenderFrameSet"; }
    virtual bool isFrameSet() const { return true; }

    virtual void layout();
    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int x, int y, int tx, int ty, HitTestAction);
    virtual void paint(PaintInfo&, int tx, int ty);
    virtual bool isChildAllowed(RenderObject*, RenderStyle*) const;

    inline HTMLFrameSetElement* frameSet() const;

    bool flattenFrameSet() const;

    void setIsResizing(bool);

    void layOutAxis(GridAxis&, const Length*, int availableSpace);
    void computeEdgeInfo();
    void fillFromEdgeInfo(const FrameEdgeInfo& edgeInfo, int r, int c);
    void positionFrames();
    void positionFramesWithFlattening();

    int splitPosition(const GridAxis&, int split) const;
    int hitTestSplit(const GridAxis&, int position) const;

    void startResizing(GridAxis&, int position);
    void continueResizing(GridAxis&, int position);

    void paintRowBorder(const PaintInfo&, const IntRect&);
    void paintColumnBorder(const PaintInfo&, const IntRect&);

    RenderObjectChildList m_children;

    GridAxis m_rows;
    GridAxis m_cols;

    bool m_isResizing;
    bool m_isChildResizing;
};


inline RenderFrameSet* toRenderFrameSet(RenderObject* object)
{
    ASSERT(!object || object->isFrameSet());
    return static_cast<RenderFrameSet*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderFrameSet(const RenderFrameSet*);

} // namespace WebCore

#endif // RenderFrameSet_h
