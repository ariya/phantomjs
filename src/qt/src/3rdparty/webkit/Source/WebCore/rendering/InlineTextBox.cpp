/*
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
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
#include "InlineTextBox.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "Document.h"
#include "DocumentMarkerController.h"
#include "Editor.h"
#include "EllipsisBox.h"
#include "Frame.h"
#include "GraphicsContext.h"
#include "HitTestResult.h"
#include "Page.h"
#include "PaintInfo.h"
#include "RenderArena.h"
#include "RenderBR.h"
#include "RenderBlock.h"
#include "RenderCombineText.h"
#include "RenderRubyRun.h"
#include "RenderRubyText.h"
#include "RenderTheme.h"
#include "Text.h"
#include "break_lines.h"
#include <wtf/AlwaysInline.h>

using namespace std;

namespace WebCore {

typedef WTF::HashMap<const InlineTextBox*, IntRect> InlineTextBoxOverflowMap;
static InlineTextBoxOverflowMap* gTextBoxesWithOverflow;

void InlineTextBox::destroy(RenderArena* arena)
{
    if (!m_knownToHaveNoOverflow && gTextBoxesWithOverflow)
        gTextBoxesWithOverflow->remove(this);
    InlineBox::destroy(arena);
}

IntRect InlineTextBox::logicalOverflowRect() const
{
    if (m_knownToHaveNoOverflow || !gTextBoxesWithOverflow)
        return enclosingIntRect(logicalFrameRect());
    return gTextBoxesWithOverflow->get(this);
}

void InlineTextBox::setLogicalOverflowRect(const IntRect& rect)
{
    ASSERT(!m_knownToHaveNoOverflow);
    if (!gTextBoxesWithOverflow)
        gTextBoxesWithOverflow = new InlineTextBoxOverflowMap;
    gTextBoxesWithOverflow->add(this, rect);
}

int InlineTextBox::baselinePosition(FontBaseline baselineType) const
{
    if (!isText() || !parent())
        return 0;
    if (parent()->renderer() == renderer()->parent())
        return parent()->baselinePosition(baselineType);
    return toRenderBoxModelObject(renderer()->parent())->baselinePosition(baselineType, m_firstLine, isHorizontal() ? HorizontalLine : VerticalLine, PositionOnContainingLine);
}
    
int InlineTextBox::lineHeight() const
{
    if (!isText() || !renderer()->parent())
        return 0;
    if (m_renderer->isBR())
        return toRenderBR(m_renderer)->lineHeight(m_firstLine);
    if (parent()->renderer() == renderer()->parent())
        return parent()->lineHeight();
    return toRenderBoxModelObject(renderer()->parent())->lineHeight(m_firstLine, isHorizontal() ? HorizontalLine : VerticalLine, PositionOnContainingLine);
}

int InlineTextBox::selectionTop()
{
    return root()->selectionTop();
}

int InlineTextBox::selectionBottom()
{
    return root()->selectionBottom();
}

int InlineTextBox::selectionHeight()
{
    return root()->selectionHeight();
}

bool InlineTextBox::isSelected(int startPos, int endPos) const
{
    int sPos = max(startPos - m_start, 0);
    int ePos = min(endPos - m_start, (int)m_len);
    return (sPos < ePos);
}

RenderObject::SelectionState InlineTextBox::selectionState()
{
    RenderObject::SelectionState state = renderer()->selectionState();
    if (state == RenderObject::SelectionStart || state == RenderObject::SelectionEnd || state == RenderObject::SelectionBoth) {
        int startPos, endPos;
        renderer()->selectionStartEnd(startPos, endPos);
        // The position after a hard line break is considered to be past its end.
        int lastSelectable = start() + len() - (isLineBreak() ? 1 : 0);

        bool start = (state != RenderObject::SelectionEnd && startPos >= m_start && startPos < m_start + m_len);
        bool end = (state != RenderObject::SelectionStart && endPos > m_start && endPos <= lastSelectable);
        if (start && end)
            state = RenderObject::SelectionBoth;
        else if (start)
            state = RenderObject::SelectionStart;
        else if (end)
            state = RenderObject::SelectionEnd;
        else if ((state == RenderObject::SelectionEnd || startPos < m_start) &&
                 (state == RenderObject::SelectionStart || endPos > lastSelectable))
            state = RenderObject::SelectionInside;
        else if (state == RenderObject::SelectionBoth)
            state = RenderObject::SelectionNone;
    }

    // If there are ellipsis following, make sure their selection is updated.
    if (m_truncation != cNoTruncation && root()->ellipsisBox()) {
        EllipsisBox* ellipsis = root()->ellipsisBox();
        if (state != RenderObject::SelectionNone) {
            int start, end;
            selectionStartEnd(start, end);
            // The ellipsis should be considered to be selected if the end of
            // the selection is past the beginning of the truncation and the
            // beginning of the selection is before or at the beginning of the
            // truncation.
            ellipsis->setSelectionState(end >= m_truncation && start <= m_truncation ?
                RenderObject::SelectionInside : RenderObject::SelectionNone);
        } else
            ellipsis->setSelectionState(RenderObject::SelectionNone);
    }

    return state;
}

typedef Vector<UChar, 256> BufferForAppendingHyphen;

static void adjustCharactersAndLengthForHyphen(BufferForAppendingHyphen& charactersWithHyphen, RenderStyle* style, const UChar*& characters, int& length)
{
    const AtomicString& hyphenString = style->hyphenString();
    charactersWithHyphen.reserveCapacity(length + hyphenString.length());
    charactersWithHyphen.append(characters, length);
    charactersWithHyphen.append(hyphenString.characters(), hyphenString.length());
    characters = charactersWithHyphen.data();
    length += hyphenString.length();
}

IntRect InlineTextBox::selectionRect(int tx, int ty, int startPos, int endPos)
{
    int sPos = max(startPos - m_start, 0);
    int ePos = min(endPos - m_start, (int)m_len);
    
    if (sPos > ePos)
        return IntRect();

    RenderText* textObj = textRenderer();
    int selTop = selectionTop();
    int selHeight = selectionHeight();
    RenderStyle* styleToUse = textObj->style(m_firstLine);
    const Font& f = styleToUse->font();

    const UChar* characters = textObj->text()->characters() + m_start;
    int len = m_len;
    BufferForAppendingHyphen charactersWithHyphen;
    if (ePos == len && hasHyphen()) {
        adjustCharactersAndLengthForHyphen(charactersWithHyphen, styleToUse, characters, len);
        ePos = len;
    }

    IntRect r = enclosingIntRect(f.selectionRectForText(TextRun(characters, len, textObj->allowTabs(), textPos(), m_expansion, expansionBehavior(), direction(), m_dirOverride),
                                                        FloatPoint(logicalLeft(), selTop), selHeight, sPos, ePos));

    int logicalWidth = r.width();
    if (r.x() > logicalRight())
        logicalWidth  = 0;
    else if (r.maxX() > logicalRight())
        logicalWidth = logicalRight() - r.x();

    IntPoint topPoint = isHorizontal() ? IntPoint(r.x(), ty + selTop) : IntPoint(tx + selTop, r.x());
    int width = isHorizontal() ? logicalWidth : selHeight;
    int height = isHorizontal() ? selHeight : logicalWidth;

    return IntRect(topPoint, IntSize(width, height));
}

void InlineTextBox::deleteLine(RenderArena* arena)
{
    toRenderText(renderer())->removeTextBox(this);
    destroy(arena);
}

void InlineTextBox::extractLine()
{
    if (m_extracted)
        return;

    toRenderText(renderer())->extractTextBox(this);
}

void InlineTextBox::attachLine()
{
    if (!m_extracted)
        return;
    
    toRenderText(renderer())->attachTextBox(this);
}

float InlineTextBox::placeEllipsisBox(bool flowIsLTR, float visibleLeftEdge, float visibleRightEdge, float ellipsisWidth, bool& foundBox)
{
    if (foundBox) {
        m_truncation = cFullTruncation;
        return -1;
    }

    // For LTR this is the left edge of the box, for RTL, the right edge in parent coordinates.
    float ellipsisX = flowIsLTR ? visibleRightEdge - ellipsisWidth : visibleLeftEdge + ellipsisWidth;
    
    // Criteria for full truncation:
    // LTR: the left edge of the ellipsis is to the left of our text run.
    // RTL: the right edge of the ellipsis is to the right of our text run.
    bool ltrFullTruncation = flowIsLTR && ellipsisX <= m_x;
    bool rtlFullTruncation = !flowIsLTR && ellipsisX >= (m_x + m_logicalWidth);
    if (ltrFullTruncation || rtlFullTruncation) {
        // Too far.  Just set full truncation, but return -1 and let the ellipsis just be placed at the edge of the box.
        m_truncation = cFullTruncation;
        foundBox = true;
        return -1;
    }

    bool ltrEllipsisWithinBox = flowIsLTR && (ellipsisX < m_x + m_logicalWidth);
    bool rtlEllipsisWithinBox = !flowIsLTR && (ellipsisX > m_x);
    if (ltrEllipsisWithinBox || rtlEllipsisWithinBox) {
        foundBox = true;

        // The inline box may have different directionality than it's parent.  Since truncation
        // behavior depends both on both the parent and the inline block's directionality, we
        // must keep track of these separately.
        bool ltr = isLeftToRightDirection();
        if (ltr != flowIsLTR) {
          // Width in pixels of the visible portion of the box, excluding the ellipsis.
          int visibleBoxWidth = visibleRightEdge - visibleLeftEdge  - ellipsisWidth;
          ellipsisX = ltr ? m_x + visibleBoxWidth : m_x + m_logicalWidth - visibleBoxWidth;
        }

        int offset = offsetForPosition(ellipsisX, false);
        if (offset == 0) {
            // No characters should be rendered.  Set ourselves to full truncation and place the ellipsis at the min of our start
            // and the ellipsis edge.
            m_truncation = cFullTruncation;
            return min(ellipsisX, m_x);
        }

        // Set the truncation index on the text run.
        m_truncation = offset;

        // If we got here that means that we were only partially truncated and we need to return the pixel offset at which
        // to place the ellipsis.
        float widthOfVisibleText = toRenderText(renderer())->width(m_start, offset, textPos(), m_firstLine);

        // The ellipsis needs to be placed just after the last visible character.
        // Where "after" is defined by the flow directionality, not the inline
        // box directionality.
        // e.g. In the case of an LTR inline box truncated in an RTL flow then we can
        // have a situation such as |Hello| -> |...He|
        if (flowIsLTR)
            return m_x + widthOfVisibleText;
        else
            return (m_x + m_logicalWidth) - widthOfVisibleText - ellipsisWidth;
    }
    return -1;
}

Color correctedTextColor(Color textColor, Color backgroundColor) 
{
    // Adjust the text color if it is too close to the background color,
    // by darkening or lightening it to move it further away.
    
    int d = differenceSquared(textColor, backgroundColor);
    // semi-arbitrarily chose 65025 (255^2) value here after a few tests; 
    if (d > 65025) {
        return textColor;
    }
    
    int distanceFromWhite = differenceSquared(textColor, Color::white);
    int distanceFromBlack = differenceSquared(textColor, Color::black);

    if (distanceFromWhite < distanceFromBlack) {
        return textColor.dark();
    }
    
    return textColor.light();
}

void updateGraphicsContext(GraphicsContext* context, const Color& fillColor, const Color& strokeColor, float strokeThickness, ColorSpace colorSpace)
{
    TextDrawingModeFlags mode = context->textDrawingMode();
    if (strokeThickness > 0) {
        TextDrawingModeFlags newMode = mode | TextModeStroke;
        if (mode != newMode) {
            context->setTextDrawingMode(newMode);
            mode = newMode;
        }
    }
    
    if (mode & TextModeFill && (fillColor != context->fillColor() || colorSpace != context->fillColorSpace()))
        context->setFillColor(fillColor, colorSpace);

    if (mode & TextModeStroke) {
        if (strokeColor != context->strokeColor())
            context->setStrokeColor(strokeColor, colorSpace);
        if (strokeThickness != context->strokeThickness())
            context->setStrokeThickness(strokeThickness);
    }
}

bool InlineTextBox::isLineBreak() const
{
    return renderer()->isBR() || (renderer()->style()->preserveNewline() && len() == 1 && (*textRenderer()->text())[start()] == '\n');
}

bool InlineTextBox::nodeAtPoint(const HitTestRequest&, HitTestResult& result, int x, int y, int tx, int ty, int /* lineTop */, int /*lineBottom*/)
{
    if (isLineBreak())
        return false;

    FloatPoint boxOrigin = locationIncludingFlipping();
    boxOrigin.move(tx, ty);
    FloatRect rect(boxOrigin, IntSize(width(), height()));
    if (m_truncation != cFullTruncation && visibleToHitTesting() && rect.intersects(result.rectForPoint(x, y))) {
        renderer()->updateHitTestResult(result, flipForWritingMode(IntPoint(x - tx, y - ty)));
        if (!result.addNodeToRectBasedTestResult(renderer()->node(), x, y, rect))
            return true;
    }
    return false;
}

FloatSize InlineTextBox::applyShadowToGraphicsContext(GraphicsContext* context, const ShadowData* shadow, const FloatRect& textRect, bool stroked, bool opaque, bool horizontal)
{
    if (!shadow)
        return FloatSize();

    FloatSize extraOffset;
    int shadowX = horizontal ? shadow->x() : shadow->y();
    int shadowY = horizontal ? shadow->y() : -shadow->x();
    FloatSize shadowOffset(shadowX, shadowY);
    int shadowBlur = shadow->blur();
    const Color& shadowColor = shadow->color();

    if (shadow->next() || stroked || !opaque) {
        FloatRect shadowRect(textRect);
        shadowRect.inflate(shadowBlur);
        shadowRect.move(shadowOffset);
        context->save();
        context->clip(shadowRect);

        extraOffset = FloatSize(0, 2 * textRect.height() + max(0.0f, shadowOffset.height()) + shadowBlur);
        shadowOffset -= extraOffset;
    }

    context->setShadow(shadowOffset, shadowBlur, shadowColor, context->fillColorSpace());
    return extraOffset;
}

static void paintTextWithShadows(GraphicsContext* context, const Font& font, const TextRun& textRun, const AtomicString& emphasisMark, int emphasisMarkOffset, int startOffset, int endOffset, int truncationPoint, const FloatPoint& textOrigin,
                                 const FloatRect& boxRect, const ShadowData* shadow, bool stroked, bool horizontal)
{
    Color fillColor = context->fillColor();
    ColorSpace fillColorSpace = context->fillColorSpace();
    bool opaque = fillColor.alpha() == 255;
    if (!opaque)
        context->setFillColor(Color::black, fillColorSpace);

    do {
        IntSize extraOffset;
        if (shadow)
            extraOffset = roundedIntSize(InlineTextBox::applyShadowToGraphicsContext(context, shadow, boxRect, stroked, opaque, horizontal));
        else if (!opaque)
            context->setFillColor(fillColor, fillColorSpace);

        if (startOffset <= endOffset) {
            if (emphasisMark.isEmpty())
                context->drawText(font, textRun, textOrigin + extraOffset, startOffset, endOffset);
            else
                context->drawEmphasisMarks(font, textRun, emphasisMark, textOrigin + extraOffset + IntSize(0, emphasisMarkOffset), startOffset, endOffset);
        } else {
            if (endOffset > 0) {
                if (emphasisMark.isEmpty())
                    context->drawText(font, textRun, textOrigin + extraOffset,  0, endOffset);
                else
                    context->drawEmphasisMarks(font, textRun, emphasisMark, textOrigin + extraOffset + IntSize(0, emphasisMarkOffset),  0, endOffset);
            } if (startOffset < truncationPoint) {
                if (emphasisMark.isEmpty())
                    context->drawText(font, textRun, textOrigin + extraOffset, startOffset, truncationPoint);
                else
                    context->drawEmphasisMarks(font, textRun, emphasisMark, textOrigin + extraOffset + IntSize(0, emphasisMarkOffset),  startOffset, truncationPoint);
            }
        }

        if (!shadow)
            break;

        if (shadow->next() || stroked || !opaque)
            context->restore();
        else
            context->clearShadow();

        shadow = shadow->next();
    } while (shadow || stroked || !opaque);
}

bool InlineTextBox::getEmphasisMarkPosition(RenderStyle* style, TextEmphasisPosition& emphasisPosition) const
{
    // This function returns true if there are text emphasis marks and they are suppressed by ruby text.
    if (style->textEmphasisMark() == TextEmphasisMarkNone)
        return false;

    emphasisPosition = style->textEmphasisPosition();
    if (emphasisPosition == TextEmphasisPositionUnder)
        return true; // Ruby text is always over, so it cannot suppress emphasis marks under.

    RenderBlock* containingBlock = renderer()->containingBlock();
    if (!containingBlock->isRubyBase())
        return true; // This text is not inside a ruby base, so it does not have ruby text over it.

    if (!containingBlock->parent()->isRubyRun())
        return true; // Cannot get the ruby text.

    RenderRubyText* rubyText = static_cast<RenderRubyRun*>(containingBlock->parent())->rubyText();

    // The emphasis marks over are suppressed only if there is a ruby text box and it not empty.
    return !rubyText || !rubyText->firstLineBox();
}

enum RotationDirection { Counterclockwise, Clockwise };

static inline AffineTransform rotation(const FloatRect& boxRect, RotationDirection clockwise)
{
    return clockwise ? AffineTransform(0, 1, -1, 0, boxRect.x() + boxRect.maxY(), boxRect.maxY() - boxRect.x())
        : AffineTransform(0, -1, 1, 0, boxRect.x() - boxRect.maxY(), boxRect.x() + boxRect.maxY());
}

void InlineTextBox::paint(PaintInfo& paintInfo, int tx, int ty, int /*lineTop*/, int /*lineBottom*/)
{
    if (isLineBreak() || !paintInfo.shouldPaintWithinRoot(renderer()) || renderer()->style()->visibility() != VISIBLE ||
        m_truncation == cFullTruncation || paintInfo.phase == PaintPhaseOutline || !m_len)
        return;

    ASSERT(paintInfo.phase != PaintPhaseSelfOutline && paintInfo.phase != PaintPhaseChildOutlines);

    int logicalLeftSide = logicalLeftVisualOverflow();
    int logicalRightSide = logicalRightVisualOverflow();
    int logicalStart = logicalLeftSide + (isHorizontal() ? tx : ty);
    int logicalExtent = logicalRightSide - logicalLeftSide;
    
    int paintEnd = isHorizontal() ? paintInfo.rect.maxX() : paintInfo.rect.maxY();
    int paintStart = isHorizontal() ? paintInfo.rect.x() : paintInfo.rect.y();
    
    if (logicalStart >= paintEnd || logicalStart + logicalExtent <= paintStart)
        return;

    bool isPrinting = textRenderer()->document()->printing();
    
    // Determine whether or not we're selected.
    bool haveSelection = !isPrinting && paintInfo.phase != PaintPhaseTextClip && selectionState() != RenderObject::SelectionNone;
    if (!haveSelection && paintInfo.phase == PaintPhaseSelection)
        // When only painting the selection, don't bother to paint if there is none.
        return;

    if (m_truncation != cNoTruncation) {
        if (renderer()->containingBlock()->style()->isLeftToRightDirection() != isLeftToRightDirection()) {
            // Make the visible fragment of text hug the edge closest to the rest of the run by moving the origin
            // at which we start drawing text.
            // e.g. In the case of LTR text truncated in an RTL Context, the correct behavior is:
            // |Hello|CBA| -> |...He|CBA|
            // In order to draw the fragment "He" aligned to the right edge of it's box, we need to start drawing
            // farther to the right.
            // NOTE: WebKit's behavior differs from that of IE which appears to just overlay the ellipsis on top of the
            // truncated string i.e.  |Hello|CBA| -> |...lo|CBA|
            int widthOfVisibleText = toRenderText(renderer())->width(m_start, m_truncation, textPos(), m_firstLine);
            int widthOfHiddenText = m_logicalWidth - widthOfVisibleText;
            // FIXME: The hit testing logic also needs to take this translation int account.
            if (isHorizontal())
                tx += isLeftToRightDirection() ? widthOfHiddenText : -widthOfHiddenText;
            else
                ty += isLeftToRightDirection() ? widthOfHiddenText : -widthOfHiddenText;
        }
    }

    GraphicsContext* context = paintInfo.context;

    RenderStyle* styleToUse = renderer()->style(m_firstLine);
    
    ty -= styleToUse->isHorizontalWritingMode() ? 0 : logicalHeight();

    FloatPoint boxOrigin = locationIncludingFlipping();
    boxOrigin.move(tx, ty);    
    FloatRect boxRect(boxOrigin, IntSize(logicalWidth(), logicalHeight()));

    RenderCombineText* combinedText = styleToUse->hasTextCombine() && textRenderer()->isCombineText() && toRenderCombineText(textRenderer())->isCombined() ? toRenderCombineText(textRenderer()) : 0;

    bool shouldRotate = !isHorizontal() && !combinedText;
    if (shouldRotate)
        context->concatCTM(rotation(boxRect, Clockwise));

    // Determine whether or not we have composition underlines to draw.
    bool containsComposition = renderer()->node() && renderer()->frame()->editor()->compositionNode() == renderer()->node();
    bool useCustomUnderlines = containsComposition && renderer()->frame()->editor()->compositionUsesCustomUnderlines();

    // Set our font.
    const Font& font = styleToUse->font();

    FloatPoint textOrigin = FloatPoint(boxOrigin.x(), boxOrigin.y() + font.fontMetrics().ascent());

    if (combinedText)
        combinedText->adjustTextOrigin(textOrigin, boxRect);

    // 1. Paint backgrounds behind text if needed. Examples of such backgrounds include selection
    // and composition underlines.
    if (paintInfo.phase != PaintPhaseSelection && paintInfo.phase != PaintPhaseTextClip && !isPrinting) {
#if PLATFORM(MAC)
        // Custom highlighters go behind everything else.
        if (styleToUse->highlight() != nullAtom && !context->paintingDisabled())
            paintCustomHighlight(tx, ty, styleToUse->highlight());
#endif

        if (containsComposition && !useCustomUnderlines)
            paintCompositionBackground(context, boxOrigin, styleToUse, font,
                renderer()->frame()->editor()->compositionStart(),
                renderer()->frame()->editor()->compositionEnd());

        paintDocumentMarkers(context, boxOrigin, styleToUse, font, true);

        if (haveSelection && !useCustomUnderlines)
            paintSelection(context, boxOrigin, styleToUse, font);
    }

    // 2. Now paint the foreground, including text and decorations like underline/overline (in quirks mode only).
    Color textFillColor;
    Color textStrokeColor;
    Color emphasisMarkColor;
    float textStrokeWidth = styleToUse->textStrokeWidth();
    const ShadowData* textShadow = paintInfo.forceBlackText ? 0 : styleToUse->textShadow();

    if (paintInfo.forceBlackText) {
        textFillColor = Color::black;
        textStrokeColor = Color::black;
        emphasisMarkColor = Color::black;
    } else {
        textFillColor = styleToUse->visitedDependentColor(CSSPropertyWebkitTextFillColor);
        
        // Make the text fill color legible against a white background
        if (styleToUse->forceBackgroundsToWhite())
            textFillColor = correctedTextColor(textFillColor, Color::white);

        textStrokeColor = styleToUse->visitedDependentColor(CSSPropertyWebkitTextStrokeColor);
        
        // Make the text stroke color legible against a white background
        if (styleToUse->forceBackgroundsToWhite())
            textStrokeColor = correctedTextColor(textStrokeColor, Color::white);

        emphasisMarkColor = styleToUse->visitedDependentColor(CSSPropertyWebkitTextEmphasisColor);
        
        // Make the text stroke color legible against a white background
        if (styleToUse->forceBackgroundsToWhite())
            emphasisMarkColor = correctedTextColor(emphasisMarkColor, Color::white);
    }

    bool paintSelectedTextOnly = (paintInfo.phase == PaintPhaseSelection);
    bool paintSelectedTextSeparately = false;

    Color selectionFillColor = textFillColor;
    Color selectionStrokeColor = textStrokeColor;
    Color selectionEmphasisMarkColor = emphasisMarkColor;
    float selectionStrokeWidth = textStrokeWidth;
    const ShadowData* selectionShadow = textShadow;
    if (haveSelection) {
        // Check foreground color first.
        Color foreground = paintInfo.forceBlackText ? Color::black : renderer()->selectionForegroundColor();
        if (foreground.isValid() && foreground != selectionFillColor) {
            if (!paintSelectedTextOnly)
                paintSelectedTextSeparately = true;
            selectionFillColor = foreground;
        }

        Color emphasisMarkForeground = paintInfo.forceBlackText ? Color::black : renderer()->selectionEmphasisMarkColor();
        if (emphasisMarkForeground.isValid() && emphasisMarkForeground != selectionEmphasisMarkColor) {
            if (!paintSelectedTextOnly)
                paintSelectedTextSeparately = true;
            selectionEmphasisMarkColor = emphasisMarkForeground;
        }

        if (RenderStyle* pseudoStyle = renderer()->getCachedPseudoStyle(SELECTION)) {
            const ShadowData* shadow = paintInfo.forceBlackText ? 0 : pseudoStyle->textShadow();
            if (shadow != selectionShadow) {
                if (!paintSelectedTextOnly)
                    paintSelectedTextSeparately = true;
                selectionShadow = shadow;
            }

            float strokeWidth = pseudoStyle->textStrokeWidth();
            if (strokeWidth != selectionStrokeWidth) {
                if (!paintSelectedTextOnly)
                    paintSelectedTextSeparately = true;
                selectionStrokeWidth = strokeWidth;
            }

            Color stroke = paintInfo.forceBlackText ? Color::black : pseudoStyle->visitedDependentColor(CSSPropertyWebkitTextStrokeColor);
            if (stroke != selectionStrokeColor) {
                if (!paintSelectedTextOnly)
                    paintSelectedTextSeparately = true;
                selectionStrokeColor = stroke;
            }
        }
    }

    int length = m_len;
    const UChar* characters;
    if (!combinedText)
        characters = textRenderer()->text()->characters() + m_start;
    else
        combinedText->charactersToRender(m_start, characters, length);

    BufferForAppendingHyphen charactersWithHyphen;
    if (hasHyphen())
        adjustCharactersAndLengthForHyphen(charactersWithHyphen, styleToUse, characters, length);

    TextRun textRun(characters, length, textRenderer()->allowTabs(), textPos(), m_expansion, expansionBehavior(), direction(), m_dirOverride || styleToUse->visuallyOrdered());

    int sPos = 0;
    int ePos = 0;
    if (paintSelectedTextOnly || paintSelectedTextSeparately)
        selectionStartEnd(sPos, ePos);

    if (m_truncation != cNoTruncation) {
        sPos = min<int>(sPos, m_truncation);
        ePos = min<int>(ePos, m_truncation);
        length = m_truncation;
    }

    int emphasisMarkOffset = 0;
    TextEmphasisPosition emphasisMarkPosition;
    bool hasTextEmphasis = getEmphasisMarkPosition(styleToUse, emphasisMarkPosition);
    const AtomicString& emphasisMark = hasTextEmphasis ? styleToUse->textEmphasisMarkString() : nullAtom;
    if (!emphasisMark.isEmpty())
        emphasisMarkOffset = emphasisMarkPosition == TextEmphasisPositionOver ? -font.fontMetrics().ascent() - font.emphasisMarkDescent(emphasisMark) : font.fontMetrics().descent() + font.emphasisMarkAscent(emphasisMark);

    if (!paintSelectedTextOnly) {
        // For stroked painting, we have to change the text drawing mode.  It's probably dangerous to leave that mutated as a side
        // effect, so only when we know we're stroking, do a save/restore.
        GraphicsContextStateSaver stateSaver(*context, textStrokeWidth > 0);

        updateGraphicsContext(context, textFillColor, textStrokeColor, textStrokeWidth, styleToUse->colorSpace());
        if (!paintSelectedTextSeparately || ePos <= sPos) {
            // FIXME: Truncate right-to-left text correctly.
            paintTextWithShadows(context, font, textRun, nullAtom, 0, 0, length, length, textOrigin, boxRect, textShadow, textStrokeWidth > 0, isHorizontal());
        } else
            paintTextWithShadows(context, font, textRun, nullAtom, 0, ePos, sPos, length, textOrigin, boxRect, textShadow, textStrokeWidth > 0, isHorizontal());

        if (!emphasisMark.isEmpty()) {
            updateGraphicsContext(context, emphasisMarkColor, textStrokeColor, textStrokeWidth, styleToUse->colorSpace());

            static TextRun objectReplacementCharacterTextRun(&objectReplacementCharacter, 1);
            TextRun& emphasisMarkTextRun = combinedText ? objectReplacementCharacterTextRun : textRun;
            FloatPoint emphasisMarkTextOrigin = combinedText ? FloatPoint(boxOrigin.x() + boxRect.width() / 2, boxOrigin.y() + font.fontMetrics().ascent()) : textOrigin;
            if (combinedText)
                context->concatCTM(rotation(boxRect, Clockwise));

            if (!paintSelectedTextSeparately || ePos <= sPos) {
                // FIXME: Truncate right-to-left text correctly.
                paintTextWithShadows(context, combinedText ? combinedText->originalFont() : font, emphasisMarkTextRun, emphasisMark, emphasisMarkOffset, 0, length, length, emphasisMarkTextOrigin, boxRect, textShadow, textStrokeWidth > 0, isHorizontal());
            } else
                paintTextWithShadows(context, combinedText ? combinedText->originalFont() : font, emphasisMarkTextRun, emphasisMark, emphasisMarkOffset, ePos, sPos, length, emphasisMarkTextOrigin, boxRect, textShadow, textStrokeWidth > 0, isHorizontal());

            if (combinedText)
                context->concatCTM(rotation(boxRect, Counterclockwise));
        }
    }

    if ((paintSelectedTextOnly || paintSelectedTextSeparately) && sPos < ePos) {
        // paint only the text that is selected
        GraphicsContextStateSaver stateSaver(*context, selectionStrokeWidth > 0);

        updateGraphicsContext(context, selectionFillColor, selectionStrokeColor, selectionStrokeWidth, styleToUse->colorSpace());
        paintTextWithShadows(context, font, textRun, nullAtom, 0, sPos, ePos, length, textOrigin, boxRect, selectionShadow, selectionStrokeWidth > 0, isHorizontal());
        if (!emphasisMark.isEmpty()) {
            updateGraphicsContext(context, selectionEmphasisMarkColor, textStrokeColor, textStrokeWidth, styleToUse->colorSpace());

            static TextRun objectReplacementCharacterTextRun(&objectReplacementCharacter, 1);
            TextRun& emphasisMarkTextRun = combinedText ? objectReplacementCharacterTextRun : textRun;
            FloatPoint emphasisMarkTextOrigin = combinedText ? FloatPoint(boxOrigin.x() + boxRect.width() / 2, boxOrigin.y() + font.fontMetrics().ascent()) : textOrigin;
            if (combinedText)
                context->concatCTM(rotation(boxRect, Clockwise));

            paintTextWithShadows(context, combinedText ? combinedText->originalFont() : font, emphasisMarkTextRun, emphasisMark, emphasisMarkOffset, sPos, ePos, length, emphasisMarkTextOrigin, boxRect, selectionShadow, selectionStrokeWidth > 0, isHorizontal());

            if (combinedText)
                context->concatCTM(rotation(boxRect, Counterclockwise));
        }
    }

    // Paint decorations
    int textDecorations = styleToUse->textDecorationsInEffect();
    if (textDecorations != TDNONE && paintInfo.phase != PaintPhaseSelection) {
        updateGraphicsContext(context, textFillColor, textStrokeColor, textStrokeWidth, styleToUse->colorSpace());
        paintDecoration(context, boxOrigin, textDecorations, textShadow);
    }

    if (paintInfo.phase == PaintPhaseForeground) {
        paintDocumentMarkers(context, boxOrigin, styleToUse, font, false);

        if (useCustomUnderlines) {
            const Vector<CompositionUnderline>& underlines = renderer()->frame()->editor()->customCompositionUnderlines();
            size_t numUnderlines = underlines.size();

            for (size_t index = 0; index < numUnderlines; ++index) {
                const CompositionUnderline& underline = underlines[index];

                if (underline.endOffset <= start())
                    // underline is completely before this run.  This might be an underline that sits
                    // before the first run we draw, or underlines that were within runs we skipped 
                    // due to truncation.
                    continue;
                
                if (underline.startOffset <= end()) {
                    // underline intersects this run.  Paint it.
                    paintCompositionUnderline(context, boxOrigin, underline);
                    if (underline.endOffset > end() + 1)
                        // underline also runs into the next run. Bail now, no more marker advancement.
                        break;
                } else
                    // underline is completely after this run, bail.  A later run will paint it.
                    break;
            }
        }
    }
    
    if (shouldRotate)
        context->concatCTM(rotation(boxRect, Counterclockwise));
}

void InlineTextBox::selectionStartEnd(int& sPos, int& ePos)
{
    int startPos, endPos;
    if (renderer()->selectionState() == RenderObject::SelectionInside) {
        startPos = 0;
        endPos = textRenderer()->textLength();
    } else {
        textRenderer()->selectionStartEnd(startPos, endPos);
        if (renderer()->selectionState() == RenderObject::SelectionStart)
            endPos = textRenderer()->textLength();
        else if (renderer()->selectionState() == RenderObject::SelectionEnd)
            startPos = 0;
    }

    sPos = max(startPos - m_start, 0);
    ePos = min(endPos - m_start, (int)m_len);
}

void InlineTextBox::paintSelection(GraphicsContext* context, const FloatPoint& boxOrigin, RenderStyle* style, const Font& font)
{
    if (context->paintingDisabled())
        return;

    // See if we have a selection to paint at all.
    int sPos, ePos;
    selectionStartEnd(sPos, ePos);
    if (sPos >= ePos)
        return;

    Color textColor = style->visitedDependentColor(CSSPropertyColor);
    Color c = renderer()->selectionBackgroundColor();
    if (!c.isValid() || c.alpha() == 0)
        return;

    // If the text color ends up being the same as the selection background, invert the selection
    // background.  This should basically never happen, since the selection has transparency.
    if (textColor == c)
        c = Color(0xff - c.red(), 0xff - c.green(), 0xff - c.blue());

    GraphicsContextStateSaver stateSaver(*context);
    updateGraphicsContext(context, c, c, 0, style->colorSpace());  // Don't draw text at all!
    
    // If the text is truncated, let the thing being painted in the truncation
    // draw its own highlight.
    int length = m_truncation != cNoTruncation ? m_truncation : m_len;
    const UChar* characters = textRenderer()->text()->characters() + m_start;

    BufferForAppendingHyphen charactersWithHyphen;
    if (ePos == length && hasHyphen()) {
        adjustCharactersAndLengthForHyphen(charactersWithHyphen, style, characters, length);
        ePos = length;
    }

    int deltaY = renderer()->style()->isFlippedLinesWritingMode() ? selectionBottom() - logicalBottom() : logicalTop() - selectionTop();
    int selHeight = selectionHeight();
    FloatPoint localOrigin(boxOrigin.x(), boxOrigin.y() - deltaY);

    FloatRect clipRect(localOrigin, FloatSize(m_logicalWidth, selHeight));
    float maxX = floorf(clipRect.maxX());
    clipRect.setX(floorf(clipRect.x()));
    clipRect.setWidth(maxX - clipRect.x());
    context->clip(clipRect);

    context->drawHighlightForText(font, TextRun(characters, length, textRenderer()->allowTabs(), textPos(), m_expansion, expansionBehavior(), 
                                  direction(), m_dirOverride || style->visuallyOrdered()),
                                  localOrigin, selHeight, c, style->colorSpace(), sPos, ePos);
}

void InlineTextBox::paintCompositionBackground(GraphicsContext* context, const FloatPoint& boxOrigin, RenderStyle* style, const Font& font, int startPos, int endPos)
{
    int offset = m_start;
    int sPos = max(startPos - offset, 0);
    int ePos = min(endPos - offset, (int)m_len);

    if (sPos >= ePos)
        return;

    GraphicsContextStateSaver stateSaver(*context);

    Color c = Color(225, 221, 85);
    
    updateGraphicsContext(context, c, c, 0, style->colorSpace()); // Don't draw text at all!

    int deltaY = renderer()->style()->isFlippedLinesWritingMode() ? selectionBottom() - logicalBottom() : logicalTop() - selectionTop();
    int selHeight = selectionHeight();
    FloatPoint localOrigin(boxOrigin.x(), boxOrigin.y() - deltaY);
    context->drawHighlightForText(font, TextRun(textRenderer()->text()->characters() + m_start, m_len, textRenderer()->allowTabs(), textPos(), m_expansion, expansionBehavior(),
                                  direction(), m_dirOverride || style->visuallyOrdered()),
                                  localOrigin, selHeight, c, style->colorSpace(), sPos, ePos);
}

#if PLATFORM(MAC)

void InlineTextBox::paintCustomHighlight(int tx, int ty, const AtomicString& type)
{
    Frame* frame = renderer()->frame();
    if (!frame)
        return;
    Page* page = frame->page();
    if (!page)
        return;

    RootInlineBox* r = root();
    FloatRect rootRect(tx + r->x(), ty + selectionTop(), r->logicalWidth(), selectionHeight());
    FloatRect textRect(tx + x(), rootRect.y(), logicalWidth(), rootRect.height());

    page->chrome()->client()->paintCustomHighlight(renderer()->node(), type, textRect, rootRect, true, false);
}

#endif

void InlineTextBox::paintDecoration(GraphicsContext* context, const FloatPoint& boxOrigin, int deco, const ShadowData* shadow)
{
    if (m_truncation == cFullTruncation)
        return;

    FloatPoint localOrigin = boxOrigin;

    float width = m_logicalWidth;
    if (m_truncation != cNoTruncation) {
        width = toRenderText(renderer())->width(m_start, m_truncation, textPos(), m_firstLine);
        if (!isLeftToRightDirection())
            localOrigin.move(m_logicalWidth - width, 0);
    }
    
    // Get the text decoration colors.
    Color underline, overline, linethrough;
    renderer()->getTextDecorationColors(deco, underline, overline, linethrough, true);
    
    // Use a special function for underlines to get the positioning exactly right.
    bool isPrinting = textRenderer()->document()->printing();
    context->setStrokeThickness(1.0f); // FIXME: We should improve this rule and not always just assume 1.

    bool linesAreOpaque = !isPrinting && (!(deco & UNDERLINE) || underline.alpha() == 255) && (!(deco & OVERLINE) || overline.alpha() == 255) && (!(deco & LINE_THROUGH) || linethrough.alpha() == 255);

    RenderStyle* styleToUse = renderer()->style(m_firstLine);
    int baseline = styleToUse->fontMetrics().ascent();

    bool setClip = false;
    int extraOffset = 0;
    if (!linesAreOpaque && shadow && shadow->next()) {
        FloatRect clipRect(localOrigin, FloatSize(width, baseline + 2));
        for (const ShadowData* s = shadow; s; s = s->next()) {
            FloatRect shadowRect(localOrigin, FloatSize(width, baseline + 2));
            shadowRect.inflate(s->blur());
            int shadowX = isHorizontal() ? s->x() : s->y();
            int shadowY = isHorizontal() ? s->y() : -s->x();
            shadowRect.move(shadowX, shadowY);
            clipRect.unite(shadowRect);
            extraOffset = max(extraOffset, max(0, shadowY) + s->blur());
        }
        context->save();
        context->clip(clipRect);
        extraOffset += baseline + 2;
        localOrigin.move(0, extraOffset);
        setClip = true;
    }

    ColorSpace colorSpace = renderer()->style()->colorSpace();
    bool setShadow = false;
    
    do {
        if (shadow) {
            if (!shadow->next()) {
                // The last set of lines paints normally inside the clip.
                localOrigin.move(0, -extraOffset);
                extraOffset = 0;
            }
            int shadowX = isHorizontal() ? shadow->x() : shadow->y();
            int shadowY = isHorizontal() ? shadow->y() : -shadow->x();
            context->setShadow(FloatSize(shadowX, shadowY - extraOffset), shadow->blur(), shadow->color(), colorSpace);
            setShadow = true;
            shadow = shadow->next();
        }

        if (deco & UNDERLINE) {
            context->setStrokeColor(underline, colorSpace);
            context->setStrokeStyle(SolidStroke);
            // Leave one pixel of white between the baseline and the underline.
            context->drawLineForText(FloatPoint(localOrigin.x(), localOrigin.y() + baseline + 1), width, isPrinting);
        }
        if (deco & OVERLINE) {
            context->setStrokeColor(overline, colorSpace);
            context->setStrokeStyle(SolidStroke);
            context->drawLineForText(localOrigin, width, isPrinting);
        }
        if (deco & LINE_THROUGH) {
            context->setStrokeColor(linethrough, colorSpace);
            context->setStrokeStyle(SolidStroke);
            context->drawLineForText(FloatPoint(localOrigin.x(), localOrigin.y() + 2 * baseline / 3), width, isPrinting);
        }
    } while (shadow);

    if (setClip)
        context->restore();
    else if (setShadow)
        context->clearShadow();
}

static GraphicsContext::TextCheckingLineStyle textCheckingLineStyleForMarkerType(DocumentMarker::MarkerType markerType)
{
    switch (markerType) {
    case DocumentMarker::Spelling:
        return GraphicsContext::TextCheckingSpellingLineStyle;
    case DocumentMarker::Grammar:
        return GraphicsContext::TextCheckingGrammarLineStyle;
    case DocumentMarker::CorrectionIndicator:
        return GraphicsContext::TextCheckingReplacementLineStyle;
    default:
        ASSERT_NOT_REACHED();
        return GraphicsContext::TextCheckingSpellingLineStyle;
    }
}

void InlineTextBox::paintSpellingOrGrammarMarker(GraphicsContext* pt, const FloatPoint& boxOrigin, const DocumentMarker& marker, RenderStyle* style, const Font& font, bool grammar)
{
    // Never print spelling/grammar markers (5327887)
    if (textRenderer()->document()->printing())
        return;

    if (m_truncation == cFullTruncation)
        return;

    float start = 0; // start of line to draw, relative to tx
    float width = m_logicalWidth; // how much line to draw

    // Determine whether we need to measure text
    bool markerSpansWholeBox = true;
    if (m_start <= (int)marker.startOffset)
        markerSpansWholeBox = false;
    if ((end() + 1) != marker.endOffset)      // end points at the last char, not past it
        markerSpansWholeBox = false;
    if (m_truncation != cNoTruncation)
        markerSpansWholeBox = false;

    if (!markerSpansWholeBox || grammar) {
        int startPosition = max<int>(marker.startOffset - m_start, 0);
        int endPosition = min<int>(marker.endOffset - m_start, m_len);
        
        if (m_truncation != cNoTruncation)
            endPosition = min<int>(endPosition, m_truncation);

        // Calculate start & width
        int deltaY = renderer()->style()->isFlippedLinesWritingMode() ? selectionBottom() - logicalBottom() : logicalTop() - selectionTop();
        int selHeight = selectionHeight();
        FloatPoint startPoint(boxOrigin.x(), boxOrigin.y() - deltaY);
        TextRun run(textRenderer()->text()->characters() + m_start, m_len, textRenderer()->allowTabs(), textPos(), m_expansion, expansionBehavior(), direction(), m_dirOverride || style->visuallyOrdered());
        
        // FIXME: Convert the document markers to float rects.
        IntRect markerRect = enclosingIntRect(font.selectionRectForText(run, startPoint, selHeight, startPosition, endPosition));
        start = markerRect.x() - startPoint.x();
        width = markerRect.width();
        
        // Store rendered rects for bad grammar markers, so we can hit-test against it elsewhere in order to
        // display a toolTip. We don't do this for misspelling markers.
        if (grammar) {
            markerRect.move(-boxOrigin.x(), -boxOrigin.y());
            markerRect = renderer()->localToAbsoluteQuad(FloatRect(markerRect)).enclosingBoundingBox();
            renderer()->document()->markers()->setRenderedRectForMarker(renderer()->node(), marker, markerRect);
        }
    }
    
    // IMPORTANT: The misspelling underline is not considered when calculating the text bounds, so we have to
    // make sure to fit within those bounds.  This means the top pixel(s) of the underline will overlap the
    // bottom pixel(s) of the glyphs in smaller font sizes.  The alternatives are to increase the line spacing (bad!!)
    // or decrease the underline thickness.  The overlap is actually the most useful, and matches what AppKit does.
    // So, we generally place the underline at the bottom of the text, but in larger fonts that's not so good so
    // we pin to two pixels under the baseline.
    int lineThickness = cMisspellingLineThickness;
    int baseline = renderer()->style(m_firstLine)->fontMetrics().ascent();
    int descent = logicalHeight() - baseline;
    int underlineOffset;
    if (descent <= (2 + lineThickness)) {
        // Place the underline at the very bottom of the text in small/medium fonts.
        underlineOffset = logicalHeight() - lineThickness;
    } else {
        // In larger fonts, though, place the underline up near the baseline to prevent a big gap.
        underlineOffset = baseline + 2;
    }
    pt->drawLineForTextChecking(FloatPoint(boxOrigin.x() + start, boxOrigin.y() + underlineOffset), width, textCheckingLineStyleForMarkerType(marker.type));
}

void InlineTextBox::paintTextMatchMarker(GraphicsContext* pt, const FloatPoint& boxOrigin, const DocumentMarker& marker, RenderStyle* style, const Font& font)
{
    // Use same y positioning and height as for selection, so that when the selection and this highlight are on
    // the same word there are no pieces sticking out.
    int deltaY = renderer()->style()->isFlippedLinesWritingMode() ? selectionBottom() - logicalBottom() : logicalTop() - selectionTop();
    int selHeight = selectionHeight();

    int sPos = max(marker.startOffset - m_start, (unsigned)0);
    int ePos = min(marker.endOffset - m_start, (unsigned)m_len);    
    TextRun run(textRenderer()->text()->characters() + m_start, m_len, textRenderer()->allowTabs(), textPos(), m_expansion, expansionBehavior(), direction(), m_dirOverride || style->visuallyOrdered());
    
    // Always compute and store the rect associated with this marker. The computed rect is in absolute coordinates.
    IntRect markerRect = enclosingIntRect(font.selectionRectForText(run, IntPoint(m_x, selectionTop()), selHeight, sPos, ePos));
    markerRect = renderer()->localToAbsoluteQuad(FloatRect(markerRect)).enclosingBoundingBox();
    renderer()->document()->markers()->setRenderedRectForMarker(renderer()->node(), marker, markerRect);
    
    // Optionally highlight the text
    if (renderer()->frame()->editor()->markedTextMatchesAreHighlighted()) {
        Color color = marker.activeMatch ?
            renderer()->theme()->platformActiveTextSearchHighlightColor() :
            renderer()->theme()->platformInactiveTextSearchHighlightColor();
        GraphicsContextStateSaver stateSaver(*pt);
        updateGraphicsContext(pt, color, color, 0, style->colorSpace());  // Don't draw text at all!
        pt->clip(FloatRect(boxOrigin.x(), boxOrigin.y() - deltaY, m_logicalWidth, selHeight));
        pt->drawHighlightForText(font, run, FloatPoint(boxOrigin.x(), boxOrigin.y() - deltaY), selHeight, color, style->colorSpace(), sPos, ePos);
    }
}

void InlineTextBox::computeRectForReplacementMarker(const DocumentMarker& marker, RenderStyle* style, const Font& font)
{
    // Replacement markers are not actually drawn, but their rects need to be computed for hit testing.
    int y = selectionTop();
    int h = selectionHeight();
    
    int sPos = max(marker.startOffset - m_start, (unsigned)0);
    int ePos = min(marker.endOffset - m_start, (unsigned)m_len);    
    TextRun run(textRenderer()->text()->characters() + m_start, m_len, textRenderer()->allowTabs(), textPos(), m_expansion, expansionBehavior(), direction(), m_dirOverride || style->visuallyOrdered());
    IntPoint startPoint = IntPoint(m_x, y);
    
    // Compute and store the rect associated with this marker.
    IntRect markerRect = enclosingIntRect(font.selectionRectForText(run, startPoint, h, sPos, ePos));
    markerRect = renderer()->localToAbsoluteQuad(FloatRect(markerRect)).enclosingBoundingBox();
    renderer()->document()->markers()->setRenderedRectForMarker(renderer()->node(), marker, markerRect);
}
    
void InlineTextBox::paintDocumentMarkers(GraphicsContext* pt, const FloatPoint& boxOrigin, RenderStyle* style, const Font& font, bool background)
{
    if (!renderer()->node())
        return;

    Vector<DocumentMarker> markers = renderer()->document()->markers()->markersForNode(renderer()->node());
    Vector<DocumentMarker>::iterator markerIt = markers.begin();

    // Give any document markers that touch this run a chance to draw before the text has been drawn.
    // Note end() points at the last char, not one past it like endOffset and ranges do.
    for ( ; markerIt != markers.end(); markerIt++) {
        const DocumentMarker& marker = *markerIt;
        
        // Paint either the background markers or the foreground markers, but not both
        switch (marker.type) {
            case DocumentMarker::Grammar:
            case DocumentMarker::Spelling:
            case DocumentMarker::CorrectionIndicator:
            case DocumentMarker::Replacement:
                if (background)
                    continue;
                break;
            case DocumentMarker::TextMatch:
                if (!background)
                    continue;
                break;
            default:
                continue;
        }

        if (marker.endOffset <= start())
            // marker is completely before this run.  This might be a marker that sits before the
            // first run we draw, or markers that were within runs we skipped due to truncation.
            continue;
        
        if (marker.startOffset > end())
            // marker is completely after this run, bail.  A later run will paint it.
            break;
        
        // marker intersects this run.  Paint it.
        switch (marker.type) {
            case DocumentMarker::Spelling:
                paintSpellingOrGrammarMarker(pt, boxOrigin, marker, style, font, false);
                break;
            case DocumentMarker::Grammar:
                paintSpellingOrGrammarMarker(pt, boxOrigin, marker, style, font, true);
                break;
            case DocumentMarker::TextMatch:
                paintTextMatchMarker(pt, boxOrigin, marker, style, font);
                break;
            case DocumentMarker::CorrectionIndicator:
                paintSpellingOrGrammarMarker(pt, boxOrigin, marker, style, font, false);
                break;
            case DocumentMarker::Replacement:
                computeRectForReplacementMarker(marker, style, font);
                break;
            default:
                ASSERT_NOT_REACHED();
        }

    }
}

void InlineTextBox::paintCompositionUnderline(GraphicsContext* ctx, const FloatPoint& boxOrigin, const CompositionUnderline& underline)
{
    if (m_truncation == cFullTruncation)
        return;
    
    float start = 0; // start of line to draw, relative to tx
    float width = m_logicalWidth; // how much line to draw
    bool useWholeWidth = true;
    unsigned paintStart = m_start;
    unsigned paintEnd = end() + 1; // end points at the last char, not past it
    if (paintStart <= underline.startOffset) {
        paintStart = underline.startOffset;
        useWholeWidth = false;
        start = toRenderText(renderer())->width(m_start, paintStart - m_start, textPos(), m_firstLine);
    }
    if (paintEnd != underline.endOffset) {      // end points at the last char, not past it
        paintEnd = min(paintEnd, (unsigned)underline.endOffset);
        useWholeWidth = false;
    }
    if (m_truncation != cNoTruncation) {
        paintEnd = min(paintEnd, (unsigned)m_start + m_truncation);
        useWholeWidth = false;
    }
    if (!useWholeWidth) {
        width = toRenderText(renderer())->width(paintStart, paintEnd - paintStart, textPos() + start, m_firstLine);
    }

    // Thick marked text underlines are 2px thick as long as there is room for the 2px line under the baseline.
    // All other marked text underlines are 1px thick.
    // If there's not enough space the underline will touch or overlap characters.
    int lineThickness = 1;
    int baseline = renderer()->style(m_firstLine)->fontMetrics().ascent();
    if (underline.thick && logicalHeight() - baseline >= 2)
        lineThickness = 2;

    // We need to have some space between underlines of subsequent clauses, because some input methods do not use different underline styles for those.
    // We make each line shorter, which has a harmless side effect of shortening the first and last clauses, too.
    start += 1;
    width -= 2;

    ctx->setStrokeColor(underline.color, renderer()->style()->colorSpace());
    ctx->setStrokeThickness(lineThickness);
    ctx->drawLineForText(FloatPoint(boxOrigin.x() + start, boxOrigin.y() + logicalHeight() - lineThickness), width, textRenderer()->document()->printing());
}

int InlineTextBox::caretMinOffset() const
{
    return m_start;
}

int InlineTextBox::caretMaxOffset() const
{
    return m_start + m_len;
}

unsigned InlineTextBox::caretMaxRenderedOffset() const
{
    return m_start + m_len;
}

float InlineTextBox::textPos() const
{
    // When computing the width of a text run, RenderBlock::computeInlineDirectionPositionsForLine() doesn't include the actual offset
    // from the containing block edge in its measurement. textPos() should be consistent so the text are rendered in the same width.
    if (logicalLeft() == 0)
        return 0;
    return logicalLeft() - root()->logicalLeft();
}

int InlineTextBox::offsetForPosition(float lineOffset, bool includePartialGlyphs) const
{
    if (isLineBreak())
        return 0;

    int leftOffset = isLeftToRightDirection() ? 0 : m_len;
    int rightOffset = isLeftToRightDirection() ? m_len : 0;
    bool blockIsInOppositeDirection = renderer()->containingBlock()->style()->isLeftToRightDirection() != isLeftToRightDirection();
    if (blockIsInOppositeDirection)
        swap(leftOffset, rightOffset);

    if (lineOffset - logicalLeft() > logicalWidth())
        return rightOffset;
    if (lineOffset - logicalLeft() < 0)
        return leftOffset;

    RenderText* text = toRenderText(renderer());
    RenderStyle* style = text->style(m_firstLine);
    const Font* f = &style->font();
    int offset = f->offsetForPosition(TextRun(textRenderer()->text()->characters() + m_start, m_len,
        textRenderer()->allowTabs(), textPos(), m_expansion, expansionBehavior(), direction(), m_dirOverride || style->visuallyOrdered()),
        lineOffset - logicalLeft(), includePartialGlyphs);
    if (blockIsInOppositeDirection && (!offset || offset == m_len))
        return !offset ? m_len : 0;
    return offset;
}

float InlineTextBox::positionForOffset(int offset) const
{
    ASSERT(offset >= m_start);
    ASSERT(offset <= m_start + m_len);

    if (isLineBreak())
        return logicalLeft();

    RenderText* text = toRenderText(renderer());
    const Font& f = text->style(m_firstLine)->font();
    int from = !isLeftToRightDirection() ? offset - m_start : 0;
    int to = !isLeftToRightDirection() ? m_len : offset - m_start;
    // FIXME: Do we need to add rightBearing here?
    return f.selectionRectForText(TextRun(text->text()->characters() + m_start, m_len, textRenderer()->allowTabs(), textPos(), m_expansion, expansionBehavior(), direction(), m_dirOverride),
                                  IntPoint(logicalLeft(), 0), 0, from, to).maxX();
}

bool InlineTextBox::containsCaretOffset(int offset) const
{
    // Offsets before the box are never "in".
    if (offset < m_start)
        return false;

    int pastEnd = m_start + m_len;

    // Offsets inside the box (not at either edge) are always "in".
    if (offset < pastEnd)
        return true;

    // Offsets outside the box are always "out".
    if (offset > pastEnd)
        return false;

    // Offsets at the end are "out" for line breaks (they are on the next line).
    if (isLineBreak())
        return false;

    // Offsets at the end are "in" for normal boxes (but the caller has to check affinity).
    return true;
}

#ifndef NDEBUG

const char* InlineTextBox::boxName() const
{
    return "InlineTextBox";
}

void InlineTextBox::showBox(int printedCharacters) const
{
    const RenderText* obj = toRenderText(renderer());
    String value = obj->text();
    value = value.substring(start(), len());
    value.replace('\\', "\\\\");
    value.replace('\n', "\\n");
    printedCharacters += fprintf(stderr, "%s\t%p", boxName(), this);
    for (; printedCharacters < showTreeCharacterOffset; printedCharacters++)
        fputc(' ', stderr);
    printedCharacters = fprintf(stderr, "\t%s %p", obj->renderName(), obj);
    const int rendererCharacterOffset = 24;
    for (; printedCharacters < rendererCharacterOffset; printedCharacters++)
        fputc(' ', stderr);
    fprintf(stderr, "(%d,%d) \"%s\"\n", start(), start() + len(), value.utf8().data());
}

#endif

} // namespace WebCore
