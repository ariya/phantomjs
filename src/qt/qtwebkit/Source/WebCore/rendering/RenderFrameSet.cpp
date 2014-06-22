/**
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 *           (C) 2000 Stefan Schimanski (1Stein@gmx.de)
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.
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

#include "config.h"
#include "RenderFrameSet.h"

#include "Cursor.h"
#include "Document.h"
#include "EventHandler.h"
#include "EventNames.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HTMLFrameSetElement.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "MouseEvent.h"
#include "PaintInfo.h"
#include "RenderFrame.h"
#include "RenderLayer.h"
#include "RenderView.h"
#include "Settings.h"
#include <wtf/StackStats.h>

namespace WebCore {

RenderFrameSet::RenderFrameSet(HTMLFrameSetElement* frameSet)
    : RenderBox(frameSet)
    , m_isResizing(false)
    , m_isChildResizing(false)
{
    setInline(false);
}

RenderFrameSet::~RenderFrameSet()
{
}

RenderFrameSet::GridAxis::GridAxis()
    : m_splitBeingResized(noSplit)
{
}

inline HTMLFrameSetElement* RenderFrameSet::frameSet() const
{
    return static_cast<HTMLFrameSetElement*>(node());
}

static Color borderStartEdgeColor()
{
    return Color(170, 170, 170);
}

static Color borderEndEdgeColor()
{
    return Color::black;
}

static Color borderFillColor()
{
    return Color(208, 208, 208);
}

void RenderFrameSet::paintColumnBorder(const PaintInfo& paintInfo, const IntRect& borderRect)
{
    if (!paintInfo.rect.intersects(borderRect))
        return;
        
    // FIXME: We should do something clever when borders from distinct framesets meet at a join.
    
    // Fill first.
    GraphicsContext* context = paintInfo.context;
    ColorSpace colorSpace = style()->colorSpace();
    context->fillRect(borderRect, frameSet()->hasBorderColor() ? style()->visitedDependentColor(CSSPropertyBorderLeftColor) : borderFillColor(), colorSpace);
    
    // Now stroke the edges but only if we have enough room to paint both edges with a little
    // bit of the fill color showing through.
    if (borderRect.width() >= 3) {
        context->fillRect(IntRect(borderRect.location(), IntSize(1, height())), borderStartEdgeColor(), colorSpace);
        context->fillRect(IntRect(IntPoint(borderRect.maxX() - 1, borderRect.y()), IntSize(1, height())), borderEndEdgeColor(), colorSpace);
    }
}

void RenderFrameSet::paintRowBorder(const PaintInfo& paintInfo, const IntRect& borderRect)
{
    if (!paintInfo.rect.intersects(borderRect))
        return;

    // FIXME: We should do something clever when borders from distinct framesets meet at a join.
    
    // Fill first.
    GraphicsContext* context = paintInfo.context;
    ColorSpace colorSpace = style()->colorSpace();
    context->fillRect(borderRect, frameSet()->hasBorderColor() ? style()->visitedDependentColor(CSSPropertyBorderLeftColor) : borderFillColor(), colorSpace);

    // Now stroke the edges but only if we have enough room to paint both edges with a little
    // bit of the fill color showing through.
    if (borderRect.height() >= 3) {
        context->fillRect(IntRect(borderRect.location(), IntSize(width(), 1)), borderStartEdgeColor(), colorSpace);
        context->fillRect(IntRect(IntPoint(borderRect.x(), borderRect.maxY() - 1), IntSize(width(), 1)), borderEndEdgeColor(), colorSpace);
    }
}

void RenderFrameSet::paint(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (paintInfo.phase != PaintPhaseForeground)
        return;
    
    RenderObject* child = firstChild();
    if (!child)
        return;

    LayoutPoint adjustedPaintOffset = paintOffset + location();

    size_t rows = m_rows.m_sizes.size();
    size_t cols = m_cols.m_sizes.size();
    LayoutUnit borderThickness = frameSet()->border();
    
    LayoutUnit yPos = 0;
    for (size_t r = 0; r < rows; r++) {
        LayoutUnit xPos = 0;
        for (size_t c = 0; c < cols; c++) {
            child->paint(paintInfo, adjustedPaintOffset);
            xPos += m_cols.m_sizes[c];
            if (borderThickness && m_cols.m_allowBorder[c + 1]) {
                paintColumnBorder(paintInfo, pixelSnappedIntRect(LayoutRect(adjustedPaintOffset.x() + xPos, adjustedPaintOffset.y() + yPos, borderThickness, height())));
                xPos += borderThickness;
            }
            child = child->nextSibling();
            if (!child)
                return;
        }
        yPos += m_rows.m_sizes[r];
        if (borderThickness && m_rows.m_allowBorder[r + 1]) {
            paintRowBorder(paintInfo, pixelSnappedIntRect(LayoutRect(adjustedPaintOffset.x(), adjustedPaintOffset.y() + yPos, width(), borderThickness)));
            yPos += borderThickness;
        }
    }
}

void RenderFrameSet::GridAxis::resize(int size)
{
    m_sizes.resize(size);
    m_deltas.resize(size);
    m_deltas.fill(0);
    
    // To track edges for resizability and borders, we need to be (size + 1). This is because a parent frameset
    // may ask us for information about our left/top/right/bottom edges in order to make its own decisions about
    // what to do. We are capable of tainting that parent frameset's borders, so we have to cache this info.
    m_preventResize.resize(size + 1);
    m_allowBorder.resize(size + 1);
}

void RenderFrameSet::layOutAxis(GridAxis& axis, const Length* grid, int availableLen)
{
    availableLen = max(availableLen, 0);

    int* gridLayout = axis.m_sizes.data();

    if (!grid) {
        gridLayout[0] = availableLen;
        return;
    }

    int gridLen = axis.m_sizes.size();
    ASSERT(gridLen);

    int totalRelative = 0;
    int totalFixed = 0;
    int totalPercent = 0;
    int countRelative = 0;
    int countFixed = 0;
    int countPercent = 0;

    // First we need to investigate how many columns of each type we have and
    // how much space these columns are going to require.
    for (int i = 0; i < gridLen; ++i) {
        // Count the total length of all of the fixed columns/rows -> totalFixed
        // Count the number of columns/rows which are fixed -> countFixed
        if (grid[i].isFixed()) {
            gridLayout[i] = max(grid[i].intValue(), 0);
            totalFixed += gridLayout[i];
            countFixed++;
        }
        
        // Count the total percentage of all of the percentage columns/rows -> totalPercent
        // Count the number of columns/rows which are percentages -> countPercent
        if (grid[i].isPercent()) {
            gridLayout[i] = max(intValueForLength(grid[i], availableLen), 0);
            totalPercent += gridLayout[i];
            countPercent++;
        }

        // Count the total relative of all the relative columns/rows -> totalRelative
        // Count the number of columns/rows which are relative -> countRelative
        if (grid[i].isRelative()) {
            totalRelative += max(grid[i].intValue(), 1);
            countRelative++;
        }            
    }

    int remainingLen = availableLen;

    // Fixed columns/rows are our first priority. If there is not enough space to fit all fixed
    // columns/rows we need to proportionally adjust their size. 
    if (totalFixed > remainingLen) {
        int remainingFixed = remainingLen;

        for (int i = 0; i < gridLen; ++i) {
            if (grid[i].isFixed()) {
                gridLayout[i] = (gridLayout[i] * remainingFixed) / totalFixed;
                remainingLen -= gridLayout[i];
            }
        }
    } else
        remainingLen -= totalFixed;

    // Percentage columns/rows are our second priority. Divide the remaining space proportionally 
    // over all percentage columns/rows. IMPORTANT: the size of each column/row is not relative 
    // to 100%, but to the total percentage. For example, if there are three columns, each of 75%,
    // and the available space is 300px, each column will become 100px in width.
    if (totalPercent > remainingLen) {
        int remainingPercent = remainingLen;

        for (int i = 0; i < gridLen; ++i) {
            if (grid[i].isPercent()) {
                gridLayout[i] = (gridLayout[i] * remainingPercent) / totalPercent;
                remainingLen -= gridLayout[i];
            }
        }
    } else
        remainingLen -= totalPercent;

    // Relative columns/rows are our last priority. Divide the remaining space proportionally
    // over all relative columns/rows. IMPORTANT: the relative value of 0* is treated as 1*.
    if (countRelative) {
        int lastRelative = 0;
        int remainingRelative = remainingLen;

        for (int i = 0; i < gridLen; ++i) {
            if (grid[i].isRelative()) {
                gridLayout[i] = (max(grid[i].intValue(), 1) * remainingRelative) / totalRelative;
                remainingLen -= gridLayout[i];
                lastRelative = i;
            }
        }
        
        // If we could not evenly distribute the available space of all of the relative  
        // columns/rows, the remainder will be added to the last column/row.
        // For example: if we have a space of 100px and three columns (*,*,*), the remainder will
        // be 1px and will be added to the last column: 33px, 33px, 34px.
        if (remainingLen) {
            gridLayout[lastRelative] += remainingLen;
            remainingLen = 0;
        }
    }

    // If we still have some left over space we need to divide it over the already existing
    // columns/rows
    if (remainingLen) {
        // Our first priority is to spread if over the percentage columns. The remaining
        // space is spread evenly, for example: if we have a space of 100px, the columns 
        // definition of 25%,25% used to result in two columns of 25px. After this the 
        // columns will each be 50px in width. 
        if (countPercent && totalPercent) {
            int remainingPercent = remainingLen;
            int changePercent = 0;

            for (int i = 0; i < gridLen; ++i) {
                if (grid[i].isPercent()) {
                    changePercent = (remainingPercent * gridLayout[i]) / totalPercent;
                    gridLayout[i] += changePercent;
                    remainingLen -= changePercent;
                }
            }
        } else if (totalFixed) {
            // Our last priority is to spread the remaining space over the fixed columns.
            // For example if we have 100px of space and two column of each 40px, both
            // columns will become exactly 50px.
            int remainingFixed = remainingLen;
            int changeFixed = 0;

            for (int i = 0; i < gridLen; ++i) {
                if (grid[i].isFixed()) {
                    changeFixed = (remainingFixed * gridLayout[i]) / totalFixed;
                    gridLayout[i] += changeFixed;
                    remainingLen -= changeFixed;
                } 
            }
        }
    }
    
    // If we still have some left over space we probably ended up with a remainder of
    // a division. We cannot spread it evenly anymore. If we have any percentage 
    // columns/rows simply spread the remainder equally over all available percentage columns, 
    // regardless of their size.
    if (remainingLen && countPercent) {
        int remainingPercent = remainingLen;
        int changePercent = 0;

        for (int i = 0; i < gridLen; ++i) {
            if (grid[i].isPercent()) {
                changePercent = remainingPercent / countPercent;
                gridLayout[i] += changePercent;
                remainingLen -= changePercent;
            }
        }
    } else if (remainingLen && countFixed) {
        // If we don't have any percentage columns/rows we only have
        // fixed columns. Spread the remainder equally over all fixed
        // columns/rows.
        int remainingFixed = remainingLen;
        int changeFixed = 0;
        
        for (int i = 0; i < gridLen; ++i) {
            if (grid[i].isFixed()) {
                changeFixed = remainingFixed / countFixed;
                gridLayout[i] += changeFixed;
                remainingLen -= changeFixed;
            }
        }
    }

    // Still some left over. Add it to the last column, because it is impossible
    // spread it evenly or equally.
    if (remainingLen)
        gridLayout[gridLen - 1] += remainingLen;

    // now we have the final layout, distribute the delta over it
    bool worked = true;
    int* gridDelta = axis.m_deltas.data();
    for (int i = 0; i < gridLen; ++i) {
        if (gridLayout[i] && gridLayout[i] + gridDelta[i] <= 0)
            worked = false;
        gridLayout[i] += gridDelta[i];
    }
    // if the deltas broke something, undo them
    if (!worked) {
        for (int i = 0; i < gridLen; ++i)
            gridLayout[i] -= gridDelta[i];
        axis.m_deltas.fill(0);
    }
}

void RenderFrameSet::notifyFrameEdgeInfoChanged()
{
    if (needsLayout())
        return;
    // FIXME: We should only recompute the edge info with respect to the frame that changed
    // and its adjacent frame(s) instead of recomputing the edge info for the entire frameset.
    computeEdgeInfo();
}

void RenderFrameSet::fillFromEdgeInfo(const FrameEdgeInfo& edgeInfo, int r, int c)
{
    if (edgeInfo.allowBorder(LeftFrameEdge))
        m_cols.m_allowBorder[c] = true;
    if (edgeInfo.allowBorder(RightFrameEdge))
        m_cols.m_allowBorder[c + 1] = true;
    if (edgeInfo.preventResize(LeftFrameEdge))
        m_cols.m_preventResize[c] = true;
    if (edgeInfo.preventResize(RightFrameEdge))
        m_cols.m_preventResize[c + 1] = true;
    
    if (edgeInfo.allowBorder(TopFrameEdge))
        m_rows.m_allowBorder[r] = true;
    if (edgeInfo.allowBorder(BottomFrameEdge))
        m_rows.m_allowBorder[r + 1] = true;
    if (edgeInfo.preventResize(TopFrameEdge))
        m_rows.m_preventResize[r] = true;
    if (edgeInfo.preventResize(BottomFrameEdge))
        m_rows.m_preventResize[r + 1] = true;
}

void RenderFrameSet::computeEdgeInfo()
{
    m_rows.m_preventResize.fill(frameSet()->noResize());    
    m_rows.m_allowBorder.fill(false);
    m_cols.m_preventResize.fill(frameSet()->noResize());    
    m_cols.m_allowBorder.fill(false);
    
    RenderObject* child = firstChild();
    if (!child)
        return;

    size_t rows = m_rows.m_sizes.size();
    size_t cols = m_cols.m_sizes.size();
    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            FrameEdgeInfo edgeInfo;
            if (child->isFrameSet())
                edgeInfo = toRenderFrameSet(child)->edgeInfo();
            else
                edgeInfo = toRenderFrame(child)->edgeInfo();
            fillFromEdgeInfo(edgeInfo, r, c);
            child = child->nextSibling();
            if (!child)
                return;
        }
    }
}

FrameEdgeInfo RenderFrameSet::edgeInfo() const
{
    FrameEdgeInfo result(frameSet()->noResize(), true);
    
    int rows = frameSet()->totalRows();
    int cols = frameSet()->totalCols();
    if (rows && cols) {
        result.setPreventResize(LeftFrameEdge, m_cols.m_preventResize[0]);
        result.setAllowBorder(LeftFrameEdge, m_cols.m_allowBorder[0]);
        result.setPreventResize(RightFrameEdge, m_cols.m_preventResize[cols]);
        result.setAllowBorder(RightFrameEdge, m_cols.m_allowBorder[cols]);
        result.setPreventResize(TopFrameEdge, m_rows.m_preventResize[0]);
        result.setAllowBorder(TopFrameEdge, m_rows.m_allowBorder[0]);
        result.setPreventResize(BottomFrameEdge, m_rows.m_preventResize[rows]);
        result.setAllowBorder(BottomFrameEdge, m_rows.m_allowBorder[rows]);
    }
    
    return result;
}

void RenderFrameSet::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;
    ASSERT(needsLayout());

    bool doFullRepaint = selfNeedsLayout() && checkForRepaintDuringLayout();
    LayoutRect oldBounds;
    RenderLayerModelObject* repaintContainer = 0;
    if (doFullRepaint) {
        repaintContainer = containerForRepaint();
        oldBounds = clippedOverflowRectForRepaint(repaintContainer);
    }

    if (!parent()->isFrameSet() && !document()->printing()) {
        setWidth(view()->viewWidth());
        setHeight(view()->viewHeight());
    }

    unsigned cols = frameSet()->totalCols();
    unsigned rows = frameSet()->totalRows();

    if (m_rows.m_sizes.size() != rows || m_cols.m_sizes.size() != cols) {
        m_rows.resize(rows);
        m_cols.resize(cols);
    }

    LayoutUnit borderThickness = frameSet()->border();
    layOutAxis(m_rows, frameSet()->rowLengths(), height() - (rows - 1) * borderThickness);
    layOutAxis(m_cols, frameSet()->colLengths(), width() - (cols - 1) * borderThickness);

    if (flattenFrameSet())
        positionFramesWithFlattening();
    else
        positionFrames();

    RenderBox::layout();

    computeEdgeInfo();

    updateLayerTransform();

    if (doFullRepaint) {
        repaintUsingContainer(repaintContainer, pixelSnappedIntRect(oldBounds));
        LayoutRect newBounds = clippedOverflowRectForRepaint(repaintContainer);
        if (newBounds != oldBounds)
            repaintUsingContainer(repaintContainer, pixelSnappedIntRect(newBounds));
    }

    setNeedsLayout(false);
}

void RenderFrameSet::positionFrames()
{
    RenderBox* child = firstChildBox();
    if (!child)
        return;

    int rows = frameSet()->totalRows();
    int cols = frameSet()->totalCols();

    int yPos = 0;
    int borderThickness = frameSet()->border();
    for (int r = 0; r < rows; r++) {
        int xPos = 0;
        int height = m_rows.m_sizes[r];
        for (int c = 0; c < cols; c++) {
            child->setLocation(IntPoint(xPos, yPos));
            int width = m_cols.m_sizes[c];

            // has to be resized and itself resize its contents
            if (width != child->width() || height != child->height()) {
                child->setWidth(width);
                child->setHeight(height);
                child->setNeedsLayout(true);
                child->layout();
            }

            xPos += width + borderThickness;

            child = child->nextSiblingBox();
            if (!child)
                return;
        }
        yPos += height + borderThickness;
    }

    // all the remaining frames are hidden to avoid ugly spurious unflowed frames
    for (; child; child = child->nextSiblingBox()) {
        child->setWidth(0);
        child->setHeight(0);
        child->setNeedsLayout(false);
    }
}

void RenderFrameSet::positionFramesWithFlattening()
{
    RenderBox* child = firstChildBox();
    if (!child)
        return;

    int rows = frameSet()->totalRows();
    int cols = frameSet()->totalCols();

    int borderThickness = frameSet()->border();
    bool repaintNeeded = false;

    // calculate frameset height based on actual content height to eliminate scrolling
    bool out = false;
    for (int r = 0; r < rows && !out; r++) {
        int extra = 0;
        int height = m_rows.m_sizes[r];

        for (int c = 0; c < cols; c++) {
            IntRect oldFrameRect = pixelSnappedIntRect(child->frameRect());

            int width = m_cols.m_sizes[c];

            bool fixedWidth = frameSet()->colLengths() && frameSet()->colLengths()[c].isFixed();
            bool fixedHeight = frameSet()->rowLengths() && frameSet()->rowLengths()[r].isFixed();

            // has to be resized and itself resize its contents
            if (!fixedWidth)
                child->setWidth(width ? width + extra / (cols - c) : 0);
            else
                child->setWidth(width);
            child->setHeight(height);

            child->setNeedsLayout(true);

            if (child->isFrameSet())
                toRenderFrameSet(child)->layout();
            else
                toRenderFrame(child)->layoutWithFlattening(fixedWidth, fixedHeight);

            if (child->height() > m_rows.m_sizes[r])
                m_rows.m_sizes[r] = child->height();
            if (child->width() > m_cols.m_sizes[c])
                m_cols.m_sizes[c] = child->width();

            if (child->frameRect() != oldFrameRect)
                repaintNeeded = true;

            // difference between calculated frame width and the width it actually decides to have
            extra += width - m_cols.m_sizes[c];

            child = child->nextSiblingBox();
            if (!child) {
                out = true;
                break;
            }
        }
    }

    int xPos = 0;
    int yPos = 0;
    out = false;
    child = firstChildBox();
    for (int r = 0; r < rows && !out; r++) {
        xPos = 0;
        for (int c = 0; c < cols; c++) {
            // ensure the rows and columns are filled
            IntRect oldRect = pixelSnappedIntRect(child->frameRect());

            child->setLocation(IntPoint(xPos, yPos));
            child->setHeight(m_rows.m_sizes[r]);
            child->setWidth(m_cols.m_sizes[c]);

            if (child->frameRect() != oldRect) {
                repaintNeeded = true;

                // update to final size
                child->setNeedsLayout(true);
                if (child->isFrameSet())
                    toRenderFrameSet(child)->layout();
                else
                    toRenderFrame(child)->layoutWithFlattening(true, true);
            }

            xPos += m_cols.m_sizes[c] + borderThickness;
            child = child->nextSiblingBox();
            if (!child) {
                out = true;
                break;
            }
        }
        yPos += m_rows.m_sizes[r] + borderThickness;
    }

    setWidth(xPos - borderThickness);
    setHeight(yPos - borderThickness);

    if (repaintNeeded)
        repaint();

    // all the remaining frames are hidden to avoid ugly spurious unflowed frames
    for (; child; child = child->nextSiblingBox()) {
        child->setWidth(0);
        child->setHeight(0);
        child->setNeedsLayout(false);
    }
}

bool RenderFrameSet::flattenFrameSet() const
{
    return frame() && frame()->settings() && frame()->settings()->frameFlatteningEnabled();
}

void RenderFrameSet::startResizing(GridAxis& axis, int position)
{
    int split = hitTestSplit(axis, position);
    if (split == noSplit || axis.m_preventResize[split]) {
        axis.m_splitBeingResized = noSplit;
        return;
    }
    axis.m_splitBeingResized = split;
    axis.m_splitResizeOffset = position - splitPosition(axis, split);
}

void RenderFrameSet::continueResizing(GridAxis& axis, int position)
{
    if (needsLayout())
        return;
    if (axis.m_splitBeingResized == noSplit)
        return;
    int currentSplitPosition = splitPosition(axis, axis.m_splitBeingResized);
    int delta = (position - currentSplitPosition) - axis.m_splitResizeOffset;
    if (!delta)
        return;
    axis.m_deltas[axis.m_splitBeingResized - 1] += delta;
    axis.m_deltas[axis.m_splitBeingResized] -= delta;
    setNeedsLayout(true);
}

bool RenderFrameSet::userResize(MouseEvent* evt)
{
    if (flattenFrameSet())
        return false;

    if (!m_isResizing) {
        if (needsLayout())
            return false;
        if (evt->type() == eventNames().mousedownEvent && evt->button() == LeftButton) {
            FloatPoint localPos = absoluteToLocal(evt->absoluteLocation(), UseTransforms);
            startResizing(m_cols, localPos.x());
            startResizing(m_rows, localPos.y());
            if (m_cols.m_splitBeingResized != noSplit || m_rows.m_splitBeingResized != noSplit) {
                setIsResizing(true);
                return true;
            }
        }
    } else {
        if (evt->type() == eventNames().mousemoveEvent || (evt->type() == eventNames().mouseupEvent && evt->button() == LeftButton)) {
            FloatPoint localPos = absoluteToLocal(evt->absoluteLocation(), UseTransforms);
            continueResizing(m_cols, localPos.x());
            continueResizing(m_rows, localPos.y());
            if (evt->type() == eventNames().mouseupEvent && evt->button() == LeftButton) {
                setIsResizing(false);
                return true;
            }
        }
    }

    return false;
}

void RenderFrameSet::setIsResizing(bool isResizing)
{
    m_isResizing = isResizing;
    for (RenderObject* ancestor = parent(); ancestor; ancestor = ancestor->parent()) {
        if (ancestor->isFrameSet())
            toRenderFrameSet(ancestor)->m_isChildResizing = isResizing;
    }
    if (Frame* frame = this->frame())
        frame->eventHandler()->setResizingFrameSet(isResizing ? frameSet() : 0);
}

bool RenderFrameSet::isResizingRow() const
{
    return m_isResizing && m_rows.m_splitBeingResized != noSplit;
}

bool RenderFrameSet::isResizingColumn() const
{
    return m_isResizing && m_cols.m_splitBeingResized != noSplit;
}

bool RenderFrameSet::canResizeRow(const IntPoint& p) const
{
    int r = hitTestSplit(m_rows, p.y());
    return r != noSplit && !m_rows.m_preventResize[r];
}

bool RenderFrameSet::canResizeColumn(const IntPoint& p) const
{
    int c = hitTestSplit(m_cols, p.x());
    return c != noSplit && !m_cols.m_preventResize[c];
}

int RenderFrameSet::splitPosition(const GridAxis& axis, int split) const
{
    if (needsLayout())
        return 0;

    int borderThickness = frameSet()->border();

    int size = axis.m_sizes.size();
    if (!size)
        return 0;

    int position = 0;
    for (int i = 0; i < split && i < size; ++i)
        position += axis.m_sizes[i] + borderThickness;
    return position - borderThickness;
}

int RenderFrameSet::hitTestSplit(const GridAxis& axis, int position) const
{
    if (needsLayout())
        return noSplit;

    int borderThickness = frameSet()->border();
    if (borderThickness <= 0)
        return noSplit;

    size_t size = axis.m_sizes.size();
    if (!size)
        return noSplit;

    int splitPosition = axis.m_sizes[0];
    for (size_t i = 1; i < size; ++i) {
        if (position >= splitPosition && position < splitPosition + borderThickness)
            return i;
        splitPosition += borderThickness + axis.m_sizes[i];
    }
    return noSplit;
}

bool RenderFrameSet::isChildAllowed(RenderObject* child, RenderStyle*) const
{
    return child->isFrame() || child->isFrameSet();
}

CursorDirective RenderFrameSet::getCursor(const LayoutPoint& point, Cursor& cursor) const
{
    IntPoint roundedPoint = roundedIntPoint(point);
    if (canResizeRow(roundedPoint)) {
        cursor = rowResizeCursor();
        return SetCursor;
    }
    if (canResizeColumn(roundedPoint)) {
        cursor = columnResizeCursor();
        return SetCursor;
    }
    return RenderBox::getCursor(point, cursor);
}

} // namespace WebCore
