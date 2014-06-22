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

#include "config.h"
#include "EllipsisBox.h"

#include "Document.h"
#include "Font.h"
#include "GraphicsContext.h"
#include "HitTestResult.h"
#include "InlineTextBox.h"
#include "PaintInfo.h"
#include "RenderBlock.h"
#include "RootInlineBox.h"
#include "TextRun.h"

namespace WebCore {

void EllipsisBox::paint(PaintInfo& paintInfo, const LayoutPoint& paintOffset, LayoutUnit lineTop, LayoutUnit lineBottom)
{
    GraphicsContext* context = paintInfo.context;
    RenderStyle* style = m_renderer->style(isFirstLineStyle());
    Color textColor = style->visitedDependentColor(CSSPropertyWebkitTextFillColor);
    if (textColor != context->fillColor())
        context->setFillColor(textColor, style->colorSpace());
    bool setShadow = false;
    if (style->textShadow()) {
        context->setShadow(LayoutSize(style->textShadow()->x(), style->textShadow()->y()),
                           style->textShadow()->radius(), style->textShadow()->color(), style->colorSpace());
        setShadow = true;
    }

    const Font& font = style->font();
    if (selectionState() != RenderObject::SelectionNone) {
        paintSelection(context, paintOffset, style, font);

        // Select the correct color for painting the text.
        Color foreground = paintInfo.forceBlackText() ? Color::black : renderer()->selectionForegroundColor();
        if (foreground.isValid() && foreground != textColor)
            context->setFillColor(foreground, style->colorSpace());
    }

    // FIXME: Why is this always LTR? Fix by passing correct text run flags below.
    context->drawText(font, RenderBlock::constructTextRun(renderer(), font, m_str, style, TextRun::AllowTrailingExpansion), LayoutPoint(x() + paintOffset.x(), y() + paintOffset.y() + style->fontMetrics().ascent()));

    // Restore the regular fill color.
    if (textColor != context->fillColor())
        context->setFillColor(textColor, style->colorSpace());

    if (setShadow)
        context->clearShadow();

    paintMarkupBox(paintInfo, paintOffset, lineTop, lineBottom, style);
}

InlineBox* EllipsisBox::markupBox() const
{
    if (!m_shouldPaintMarkupBox || !m_renderer->isRenderBlock())
        return 0;

    RenderBlock* block = toRenderBlock(m_renderer);
    RootInlineBox* lastLine = block->lineAtIndex(block->lineCount() - 1);
    if (!lastLine)
        return 0;

    // If the last line-box on the last line of a block is a link, -webkit-line-clamp paints that box after the ellipsis.
    // It does not actually move the link.
    InlineBox* anchorBox = lastLine->lastChild();
    if (!anchorBox || !anchorBox->renderer()->style()->isLink())
        return 0;

    return anchorBox;
}

void EllipsisBox::paintMarkupBox(PaintInfo& paintInfo, const LayoutPoint& paintOffset, LayoutUnit lineTop, LayoutUnit lineBottom, RenderStyle* style)
{
    InlineBox* markupBox = this->markupBox();
    if (!markupBox)
        return;

    LayoutPoint adjustedPaintOffset = paintOffset;
    adjustedPaintOffset.move(x() + m_logicalWidth - markupBox->x(),
        y() + style->fontMetrics().ascent() - (markupBox->y() + markupBox->renderer()->style(isFirstLineStyle())->fontMetrics().ascent()));
    markupBox->paint(paintInfo, adjustedPaintOffset, lineTop, lineBottom);
}

IntRect EllipsisBox::selectionRect()
{
    RenderStyle* style = m_renderer->style(isFirstLineStyle());
    const Font& font = style->font();
    // FIXME: Why is this always LTR? Fix by passing correct text run flags below.
    return enclosingIntRect(font.selectionRectForText(RenderBlock::constructTextRun(renderer(), font, m_str, style, TextRun::AllowTrailingExpansion), IntPoint(x(), y() + root()->selectionTopAdjustedForPrecedingBlock()), root()->selectionHeightAdjustedForPrecedingBlock()));
}

void EllipsisBox::paintSelection(GraphicsContext* context, const LayoutPoint& paintOffset, RenderStyle* style, const Font& font)
{
    Color textColor = style->visitedDependentColor(CSSPropertyColor);
    Color c = m_renderer->selectionBackgroundColor();
    if (!c.isValid() || !c.alpha())
        return;

    // If the text color ends up being the same as the selection background, invert the selection
    // background.
    if (textColor == c)
        c = Color(0xff - c.red(), 0xff - c.green(), 0xff - c.blue());

    GraphicsContextStateSaver stateSaver(*context);
    LayoutUnit top = root()->selectionTop();
    LayoutUnit h = root()->selectionHeight();
    FloatRect clipRect(x() + paintOffset.x(), top + paintOffset.y(), m_logicalWidth, h);
    alignSelectionRectToDevicePixels(clipRect);
    context->clip(clipRect);
    // FIXME: Why is this always LTR? Fix by passing correct text run flags below.
    context->drawHighlightForText(font, RenderBlock::constructTextRun(renderer(), font, m_str, style, TextRun::AllowTrailingExpansion), roundedIntPoint(LayoutPoint(x() + paintOffset.x(), y() + paintOffset.y() + top)), h, c, style->colorSpace());
}

bool EllipsisBox::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, LayoutUnit lineTop, LayoutUnit lineBottom)
{
    LayoutPoint adjustedLocation = accumulatedOffset + roundedLayoutPoint(topLeft());

    // Hit test the markup box.
    if (InlineBox* markupBox = this->markupBox()) {
        RenderStyle* style = m_renderer->style(isFirstLineStyle());
        LayoutUnit mtx = adjustedLocation.x() + m_logicalWidth - markupBox->x();
        LayoutUnit mty = adjustedLocation.y() + style->fontMetrics().ascent() - (markupBox->y() + markupBox->renderer()->style(isFirstLineStyle())->fontMetrics().ascent());
        if (markupBox->nodeAtPoint(request, result, locationInContainer, LayoutPoint(mtx, mty), lineTop, lineBottom)) {
            renderer()->updateHitTestResult(result, locationInContainer.point() - LayoutSize(mtx, mty));
            return true;
        }
    }

    LayoutRect boundsRect(adjustedLocation, LayoutSize(m_logicalWidth, m_height));
    if (visibleToHitTesting() && boundsRect.intersects(HitTestLocation::rectForPoint(locationInContainer.point(), 0, 0, 0, 0))) {
        renderer()->updateHitTestResult(result, locationInContainer.point() - toLayoutSize(adjustedLocation));
        if (!result.addNodeToRectBasedTestResult(renderer()->node(), request, locationInContainer, boundsRect))
            return true;
    }

    return false;
}

} // namespace WebCore
