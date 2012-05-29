/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
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
#include "RenderFieldset.h"

#include "CSSPropertyNames.h"
#include "GraphicsContext.h"
#include "HTMLNames.h"
#include "PaintInfo.h"

using std::min;
using std::max;

namespace WebCore {

using namespace HTMLNames;

RenderFieldset::RenderFieldset(Node* element)
    : RenderBlock(element)
{
}

void RenderFieldset::computePreferredLogicalWidths()
{
    RenderBlock::computePreferredLogicalWidths();
    if (RenderBox* legend = findLegend()) {
        int legendMinWidth = legend->minPreferredLogicalWidth();

        Length legendMarginLeft = legend->style()->marginLeft();
        Length legendMarginRight = legend->style()->marginLeft();

        if (legendMarginLeft.isFixed())
            legendMinWidth += legendMarginLeft.value();

        if (legendMarginRight.isFixed())
            legendMinWidth += legendMarginRight.value();

        m_minPreferredLogicalWidth = max(m_minPreferredLogicalWidth, legendMinWidth + borderAndPaddingWidth());
    }
}

RenderObject* RenderFieldset::layoutSpecialExcludedChild(bool relayoutChildren)
{
    RenderBox* legend = findLegend();
    if (legend) {
        if (relayoutChildren)
            legend->setNeedsLayout(true);
        legend->layoutIfNeeded();

        int logicalLeft;
        if (style()->isLeftToRightDirection()) {
            switch (legend->style()->textAlign()) {
            case CENTER:
                logicalLeft = (logicalWidth() - logicalWidthForChild(legend)) / 2;
                break;
            case RIGHT:
                logicalLeft = logicalWidth() - borderEnd() - paddingEnd() - logicalWidthForChild(legend);
                break;
            default:
                logicalLeft = borderStart() + paddingStart() + marginStartForChild(legend);
                break;
            }
        } else {
            switch (legend->style()->textAlign()) {
            case LEFT:
                logicalLeft = borderStart() + paddingStart();
                break;
            case CENTER: {
                // Make sure that the extra pixel goes to the end side in RTL (since it went to the end side
                // in LTR).
                int centeredWidth = logicalWidth() - logicalWidthForChild(legend);
                logicalLeft = centeredWidth - centeredWidth / 2;
                break;
            }
            default:
                logicalLeft = logicalWidth() - borderStart() - paddingStart() - marginStartForChild(legend) - logicalWidthForChild(legend);
                break;
            }
        }

        setLogicalLeftForChild(legend, logicalLeft);

        int b = borderBefore();
        int h = logicalHeightForChild(legend);
        setLogicalTopForChild(legend, max((b - h) / 2, 0));
        setLogicalHeight(max(b, h) + paddingBefore());
    }
    return legend;
}

RenderBox* RenderFieldset::findLegend() const
{
    for (RenderObject* legend = firstChild(); legend; legend = legend->nextSibling()) {
        if (!legend->isFloatingOrPositioned() && legend->node() && (legend->node()->hasTagName(legendTag)))
            return toRenderBox(legend);
    }
    return 0;
}

void RenderFieldset::paintBoxDecorations(PaintInfo& paintInfo, int tx, int ty)
{
    if (!paintInfo.shouldPaintWithinRoot(this))
        return;

    int w = width();
    int h = height();
    RenderBox* legend = findLegend();
    if (!legend)
        return RenderBlock::paintBoxDecorations(paintInfo, tx, ty);

    // FIXME: We need to work with "rl" and "bt" block flow directions.  In those
    // cases the legend is embedded in the right and bottom borders respectively.
    // https://bugs.webkit.org/show_bug.cgi?id=47236
    if (style()->isHorizontalWritingMode()) {
        int yOff = (legend->y() > 0) ? 0 : (legend->height() - borderTop()) / 2;
        h -= yOff;
        ty += yOff;
    } else {
        int xOff = (legend->x() > 0) ? 0 : (legend->width() - borderLeft()) / 2;
        w -= xOff;
        tx += xOff;
    }

    paintBoxShadow(paintInfo.context, tx, ty, w, h, style(), Normal);

    paintFillLayers(paintInfo, style()->visitedDependentColor(CSSPropertyBackgroundColor), style()->backgroundLayers(), tx, ty, w, h);
    paintBoxShadow(paintInfo.context, tx, ty, w, h, style(), Inset);

    if (!style()->hasBorder())
        return;
    
    // Create a clipping region around the legend and paint the border as normal
    GraphicsContext* graphicsContext = paintInfo.context;
    GraphicsContextStateSaver stateSaver(*graphicsContext);

    // FIXME: We need to work with "rl" and "bt" block flow directions.  In those
    // cases the legend is embedded in the right and bottom borders respectively.
    // https://bugs.webkit.org/show_bug.cgi?id=47236
    if (style()->isHorizontalWritingMode()) {
        int clipTop = ty;
        int clipHeight = max(static_cast<int>(style()->borderTopWidth()), legend->height());
        graphicsContext->clipOut(IntRect(tx + legend->x(), clipTop, legend->width(), clipHeight));
    } else {
        int clipLeft = tx;
        int clipWidth = max(static_cast<int>(style()->borderLeftWidth()), legend->width());
        graphicsContext->clipOut(IntRect(clipLeft, ty + legend->y(), clipWidth, legend->height()));
    }

    paintBorder(paintInfo.context, tx, ty, w, h, style());
}

void RenderFieldset::paintMask(PaintInfo& paintInfo, int tx, int ty)
{
    if (style()->visibility() != VISIBLE || paintInfo.phase != PaintPhaseMask)
        return;

    int w = width();
    int h = height();
    RenderBox* legend = findLegend();
    if (!legend)
        return RenderBlock::paintMask(paintInfo, tx, ty);

    // FIXME: We need to work with "rl" and "bt" block flow directions.  In those
    // cases the legend is embedded in the right and bottom borders respectively.
    // https://bugs.webkit.org/show_bug.cgi?id=47236
    if (style()->isHorizontalWritingMode()) {
        int yOff = (legend->y() > 0) ? 0 : (legend->height() - borderTop()) / 2;
        h -= yOff;
        ty += yOff;
    } else {
        int xOff = (legend->x() > 0) ? 0 : (legend->width() - borderLeft()) / 2;
        w -= xOff;
        tx += xOff;
    }

    paintMaskImages(paintInfo, tx, ty, w, h);
}

} // namespace WebCore
