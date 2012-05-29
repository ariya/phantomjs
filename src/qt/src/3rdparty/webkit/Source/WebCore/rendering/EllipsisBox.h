/**
 * Copyright (C) 2003, 2006 Apple Computer, Inc.
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

#ifndef EllipsisBox_h
#define EllipsisBox_h

#include "InlineBox.h"

namespace WebCore {

class HitTestRequest;
class HitTestResult;

class EllipsisBox : public InlineBox {
public:
    EllipsisBox(RenderObject* obj, const AtomicString& ellipsisStr, InlineFlowBox* parent,
                int width, int height, int y, bool firstLine, bool isVertical, InlineBox* markupBox)
        : InlineBox(obj, 0, y, width, firstLine, true, false, false, isVertical, 0, 0, parent)
        , m_height(height)
        , m_str(ellipsisStr)
        , m_markupBox(markupBox)
        , m_selectionState(RenderObject::SelectionNone)
    {
    }

    virtual void paint(PaintInfo&, int tx, int ty, int lineTop, int lineBottom);
    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int x, int y, int tx, int ty, int lineTop, int lineBottom);
    void setSelectionState(RenderObject::SelectionState s) { m_selectionState = s; }
    IntRect selectionRect(int tx, int ty);

private:
    virtual int height() const { return m_height; }
    virtual RenderObject::SelectionState selectionState() { return m_selectionState; }
    void paintSelection(GraphicsContext*, int tx, int ty, RenderStyle*, const Font&);

    int m_height;
    AtomicString m_str;
    InlineBox* m_markupBox;
    RenderObject::SelectionState m_selectionState;
};

} // namespace WebCore

#endif // EllipsisBox_h
