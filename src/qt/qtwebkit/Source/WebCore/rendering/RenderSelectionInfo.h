/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 *           (C) 2004 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef RenderSelectionInfo_h
#define RenderSelectionInfo_h

#include "IntRect.h"
#include "RenderBox.h"

namespace WebCore {

class RenderSelectionInfoBase {
    WTF_MAKE_NONCOPYABLE(RenderSelectionInfoBase); WTF_MAKE_FAST_ALLOCATED;
public:
    RenderSelectionInfoBase()
        : m_object(0)
        , m_repaintContainer(0)
        , m_state(RenderObject::SelectionNone)
    {
    }

    RenderSelectionInfoBase(RenderObject* o)
        : m_object(o)
        , m_repaintContainer(o->containerForRepaint())
        , m_state(o->selectionState())
    {
    }
    
    RenderObject* object() const { return m_object; }
    RenderLayerModelObject* repaintContainer() const { return m_repaintContainer; }
    RenderObject::SelectionState state() const { return m_state; }

protected:
    RenderObject* m_object;
    RenderLayerModelObject* m_repaintContainer;
    RenderObject::SelectionState m_state;
};

// This struct is used when the selection changes to cache the old and new state of the selection for each RenderObject.
class RenderSelectionInfo : public RenderSelectionInfoBase {
public:
    RenderSelectionInfo(RenderObject* o, bool clipToVisibleContent)
        : RenderSelectionInfoBase(o)
        , m_rect(o->canUpdateSelectionOnRootLineBoxes() ? o->selectionRectForRepaint(m_repaintContainer, clipToVisibleContent) : LayoutRect())
    {
    }
    
    void repaint()
    {
        m_object->repaintUsingContainer(m_repaintContainer, enclosingIntRect(m_rect));
    }

    LayoutRect rect() const { return m_rect; }

private:
    LayoutRect m_rect; // relative to repaint container
};


// This struct is used when the selection changes to cache the old and new state of the selection for each RenderBlock.
class RenderBlockSelectionInfo : public RenderSelectionInfoBase {
public:
    RenderBlockSelectionInfo(RenderBlock* b)
        : RenderSelectionInfoBase(b)
        , m_rects(b->canUpdateSelectionOnRootLineBoxes() ? block()->selectionGapRectsForRepaint(m_repaintContainer) : GapRects())
    { 
    }

    void repaint()
    {
        m_object->repaintUsingContainer(m_repaintContainer, enclosingIntRect(m_rects));
    }
    
    RenderBlock* block() const { return toRenderBlock(m_object); }
    GapRects rects() const { return m_rects; }

private:
    GapRects m_rects; // relative to repaint container
};

} // namespace WebCore


#endif // RenderSelectionInfo_h
