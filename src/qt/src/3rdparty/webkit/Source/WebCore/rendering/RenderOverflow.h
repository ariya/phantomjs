/*
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

#ifndef RenderOverflow_h
#define RenderOverflow_h

#include "IntRect.h"

namespace WebCore
{
// RenderOverflow is a class for tracking content that spills out of a box.  This class is used by RenderBox and
// InlineFlowBox.
//
// There are two types of overflow: layout overflow (which is expected to be reachable via scrolling mechanisms) and
// visual overflow (which is not expected to be reachable via scrolling mechanisms).
//
// Layout overflow examples include other boxes that spill out of our box,  For example, in the inline case a tall image
// could spill out of a line box. 
    
// Examples of visual overflow are shadows, text stroke (and eventually outline and border-image).

// This object is allocated only when some of these fields have non-default values in the owning box.
class RenderOverflow {
    WTF_MAKE_NONCOPYABLE(RenderOverflow); WTF_MAKE_FAST_ALLOCATED;
public:
    RenderOverflow(const IntRect& layoutRect, const IntRect& visualRect) 
        : m_minYLayoutOverflow(layoutRect.y())
        , m_maxYLayoutOverflow(layoutRect.maxY())
        , m_minXLayoutOverflow(layoutRect.x())
        , m_maxXLayoutOverflow(layoutRect.maxX())
        , m_minYVisualOverflow(visualRect.y())
        , m_maxYVisualOverflow(visualRect.maxY())
        , m_minXVisualOverflow(visualRect.x())
        , m_maxXVisualOverflow(visualRect.maxX())
    {
    }
   
    int minYLayoutOverflow() const { return m_minYLayoutOverflow; }
    int maxYLayoutOverflow() const { return m_maxYLayoutOverflow; }
    int minXLayoutOverflow() const { return m_minXLayoutOverflow; }
    int maxXLayoutOverflow() const { return m_maxXLayoutOverflow; }
    IntRect layoutOverflowRect() const;

    int minYVisualOverflow() const { return m_minYVisualOverflow; }
    int maxYVisualOverflow() const { return m_maxYVisualOverflow; }
    int minXVisualOverflow() const { return m_minXVisualOverflow; }
    int maxXVisualOverflow() const { return m_maxXVisualOverflow; }
    IntRect visualOverflowRect() const;

    void setMinYLayoutOverflow(int overflow) { m_minYLayoutOverflow = overflow; }
    void setMaxYLayoutOverflow(int overflow) { m_maxYLayoutOverflow = overflow; }
    void setMinXLayoutOverflow(int overflow) { m_minXLayoutOverflow = overflow; }
    void setMaxXLayoutOverflow(int overflow) { m_maxXLayoutOverflow = overflow; }
    
    void setMinYVisualOverflow(int overflow) { m_minYVisualOverflow = overflow; }
    void setMaxYVisualOverflow(int overflow) { m_maxYVisualOverflow = overflow; }
    void setMinXVisualOverflow(int overflow) { m_minXVisualOverflow = overflow; }
    void setMaxXVisualOverflow(int overflow) { m_maxXVisualOverflow = overflow; }
    
    void move(int dx, int dy);
    
    void addLayoutOverflow(const IntRect&);
    void addVisualOverflow(const IntRect&);

    void setLayoutOverflow(const IntRect&);
    void setVisualOverflow(const IntRect&);

    void resetLayoutOverflow(const IntRect& defaultRect);

private:
    int m_minYLayoutOverflow;
    int m_maxYLayoutOverflow;
    int m_minXLayoutOverflow;
    int m_maxXLayoutOverflow;

    int m_minYVisualOverflow;
    int m_maxYVisualOverflow;
    int m_minXVisualOverflow;
    int m_maxXVisualOverflow;
};

inline IntRect RenderOverflow::layoutOverflowRect() const
{
    return IntRect(m_minXLayoutOverflow, m_minYLayoutOverflow, m_maxXLayoutOverflow - m_minXLayoutOverflow, m_maxYLayoutOverflow - m_minYLayoutOverflow);
}

inline IntRect RenderOverflow::visualOverflowRect() const
{
    return IntRect(m_minXVisualOverflow, m_minYVisualOverflow, m_maxXVisualOverflow - m_minXVisualOverflow, m_maxYVisualOverflow - m_minYVisualOverflow);
}

inline void RenderOverflow::move(int dx, int dy)
{
    m_minYLayoutOverflow += dy;
    m_maxYLayoutOverflow += dy;
    m_minXLayoutOverflow += dx;
    m_maxXLayoutOverflow += dx;
    
    m_minYVisualOverflow += dy;
    m_maxYVisualOverflow += dy;
    m_minXVisualOverflow += dx;
    m_maxXVisualOverflow += dx;
}

inline void RenderOverflow::addLayoutOverflow(const IntRect& rect)
{
    m_minYLayoutOverflow = std::min(rect.y(), m_minYLayoutOverflow);
    m_maxYLayoutOverflow = std::max(rect.maxY(), m_maxYLayoutOverflow);
    m_minXLayoutOverflow = std::min(rect.x(), m_minXLayoutOverflow);
    m_maxXLayoutOverflow = std::max(rect.maxX(), m_maxXLayoutOverflow);
}

inline void RenderOverflow::addVisualOverflow(const IntRect& rect)
{
    m_minYVisualOverflow = std::min(rect.y(), m_minYVisualOverflow);
    m_maxYVisualOverflow = std::max(rect.maxY(), m_maxYVisualOverflow);
    m_minXVisualOverflow = std::min(rect.x(), m_minXVisualOverflow);
    m_maxXVisualOverflow = std::max(rect.maxX(), m_maxXVisualOverflow);
}

inline void RenderOverflow::setLayoutOverflow(const IntRect& rect)
{
    m_minYLayoutOverflow = rect.y();
    m_maxYLayoutOverflow = rect.maxY();
    m_minXLayoutOverflow = rect.x();
    m_maxXLayoutOverflow = rect.maxX();
}

inline void RenderOverflow::setVisualOverflow(const IntRect& rect)
{
    m_minYVisualOverflow = rect.y();
    m_maxYVisualOverflow = rect.maxY();
    m_minXVisualOverflow = rect.x();
    m_maxXVisualOverflow = rect.maxX();
}

inline void RenderOverflow::resetLayoutOverflow(const IntRect& rect)
{
    m_minYLayoutOverflow = rect.y();
    m_maxYLayoutOverflow = rect.maxY();
    m_minXLayoutOverflow = rect.x();
    m_maxXLayoutOverflow = rect.maxX();
}

} // namespace WebCore

#endif // RenderOverflow_h
