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
#include "GraphicsContext.h"
#include "HitTestResult.h"
#include "PaintInfo.h"
#include "RootInlineBox.h"
#include "TextRun.h"

namespace WebCore {

void EllipsisBox::paint(PaintInfo& paintInfo, int tx, int ty, int lineTop, int lineBottom)
{
    GraphicsContext* context = paintInfo.context;
    RenderStyle* style = m_renderer->style(m_firstLine);
    Color textColor = style->visitedDependentColor(CSSPropertyColor);
    if (textColor != context->fillColor())
        context->setFillColor(textColor, style->colorSpace());
    bool setShadow = false;
    if (style->textShadow()) {
        context->setShadow(IntSize(style->textShadow()->x(), style->textShadow()->y()),
                           style->textShadow()->blur(), style->textShadow()->color(), style->colorSpace());
        setShadow = true;
    }

    if (selectionState() != RenderObject::SelectionNone) {
        paintSelection(context, tx, ty, style, style->font());

        // Select the correct color for painting the text.
        Color foreground = paintInfo.forceBlackText ? Color::black : renderer()->selectionForegroundColor();
        if (foreground.isValid() && foreground != textColor)
            context->setFillColor(foreground, style->colorSpace());
    }

    const String& str = m_str;
    // FIXME: Why is this alwasy LTR?
    context->drawText(style->font(), TextRun(str.characters(), str.length(), false, 0, 0, TextRun::AllowTrailingExpansion, LTR, style->visuallyOrdered()), IntPoint(m_x + tx, m_y + ty + style->fontMetrics().ascent()));

    // Restore the regular fill color.
    if (textColor != context->fillColor())
        context->setFillColor(textColor, style->colorSpace());

    if (setShadow)
        context->clearShadow();

    if (m_markupBox) {
        // Paint the markup box
        tx += m_x + m_logicalWidth - m_markupBox->x();
        ty += m_y + style->fontMetrics().ascent() - (m_markupBox->y() + m_markupBox->renderer()->style(m_firstLine)->fontMetrics().ascent());
        m_markupBox->paint(paintInfo, tx, ty, lineTop, lineBottom);
    }
}

IntRect EllipsisBox::selectionRect(int tx, int ty)
{
    RenderStyle* style = m_renderer->style(m_firstLine);
    const Font& f = style->font();
    // FIXME: Why is this always LTR?
    return enclosingIntRect(f.selectionRectForText(TextRun(m_str.characters(), m_str.length(), false, 0, 0, TextRun::AllowTrailingExpansion, LTR, style->visuallyOrdered()),
            IntPoint(m_x + tx, m_y + ty + root()->selectionTop()), root()->selectionHeight()));
}

void EllipsisBox::paintSelection(GraphicsContext* context, int tx, int ty, RenderStyle* style, const Font& font)
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
    int y = root()->selectionTop();
    int h = root()->selectionHeight();
    context->clip(IntRect(m_x + tx, y + ty, m_logicalWidth, h));
    // FIXME: Why is this always LTR?
    context->drawHighlightForText(font, TextRun(m_str.characters(), m_str.length(), false, 0, 0, TextRun::AllowTrailingExpansion, LTR, style->visuallyOrdered()),
        IntPoint(m_x + tx, m_y + ty + y), h, c, style->colorSpace());
}

bool EllipsisBox::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, int x, int y, int tx, int ty, int lineTop, int lineBottom)
{
    tx += m_x;
    ty += m_y;

    // Hit test the markup box.
    if (m_markupBox) {
        RenderStyle* style = m_renderer->style(m_firstLine);
        int mtx = tx + m_logicalWidth - m_markupBox->x();
        int mty = ty + style->fontMetrics().ascent() - (m_markupBox->y() + m_markupBox->renderer()->style(m_firstLine)->fontMetrics().ascent());
        if (m_markupBox->nodeAtPoint(request, result, x, y, mtx, mty, lineTop, lineBottom)) {
            renderer()->updateHitTestResult(result, IntPoint(x - mtx, y - mty));
            return true;
        }
    }

    IntRect boundsRect = IntRect(tx, ty, m_logicalWidth, m_height);
    if (visibleToHitTesting() && boundsRect.intersects(result.rectForPoint(x, y))) {
        renderer()->updateHitTestResult(result, IntPoint(x - tx, y - ty));
        if (!result.addNodeToRectBasedTestResult(renderer()->node(), x, y, boundsRect))
            return true;
    }

    return false;
}

} // namespace WebCore
