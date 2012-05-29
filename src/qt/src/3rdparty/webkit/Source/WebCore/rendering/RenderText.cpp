/*
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Andrew Wellington (proton@wiretapped.net)
 * Copyright (C) 2006 Graham Dennis (graham.dennis@gmail.com)
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
#include "RenderText.h"

#include "AXObjectCache.h"
#include "EllipsisBox.h"
#include "FloatQuad.h"
#include "FontTranscoder.h"
#include "FrameView.h"
#include "InlineTextBox.h"
#include "Range.h"
#include "RenderArena.h"
#include "RenderBlock.h"
#include "RenderCombineText.h"
#include "RenderLayer.h"
#include "RenderView.h"
#include "Settings.h"
#include "Text.h"
#include "TextBreakIterator.h"
#include "TextResourceDecoder.h"
#include "TextRun.h"
#include "VisiblePosition.h"
#include "break_lines.h"
#include <wtf/AlwaysInline.h>
#include <wtf/text/StringBuffer.h>
#include <wtf/unicode/CharacterNames.h>

using namespace std;
using namespace WTF;
using namespace Unicode;

namespace WebCore {

class SecureTextTimer;
typedef HashMap<RenderText*, SecureTextTimer*> SecureTextTimerMap;
static SecureTextTimerMap* gSecureTextTimers = 0;

class SecureTextTimer : public TimerBase {
public:
    SecureTextTimer(RenderText* renderText)
        : m_renderText(renderText)
        , m_lastTypedCharacterOffset(-1)
    {
    }

    void restartWithNewText(unsigned lastTypedCharacterOffset)
    {
        m_lastTypedCharacterOffset = lastTypedCharacterOffset;
        startOneShot(m_renderText->document()->settings()->passwordEchoDurationInSeconds());
    }
    void invalidate() { m_lastTypedCharacterOffset = -1; }
    unsigned lastTypedCharacterOffset() { return m_lastTypedCharacterOffset; }

private:
    virtual void fired()
    {
        ASSERT(gSecureTextTimers->contains(m_renderText));
        m_renderText->setText(m_renderText->text(), true /* forcing setting text as it may be masked later */);
    }

    RenderText* m_renderText;
    int m_lastTypedCharacterOffset;
};

static void makeCapitalized(String* string, UChar previous)
{
    if (string->isNull())
        return;

    unsigned length = string->length();
    const UChar* characters = string->characters();

    if (length >= numeric_limits<unsigned>::max())
        CRASH();

    StringBuffer stringWithPrevious(length + 1);
    stringWithPrevious[0] = previous == noBreakSpace ? ' ' : previous;
    for (unsigned i = 1; i < length + 1; i++) {
        // Replace &nbsp with a real space since ICU no longer treats &nbsp as a word separator.
        if (characters[i - 1] == noBreakSpace)
            stringWithPrevious[i] = ' ';
        else
            stringWithPrevious[i] = characters[i - 1];
    }

    TextBreakIterator* boundary = wordBreakIterator(stringWithPrevious.characters(), length + 1);
    if (!boundary)
        return;

    StringBuffer data(length);

    int32_t endOfWord;
    int32_t startOfWord = textBreakFirst(boundary);
    for (endOfWord = textBreakNext(boundary); endOfWord != TextBreakDone; startOfWord = endOfWord, endOfWord = textBreakNext(boundary)) {
        if (startOfWord != 0) // Ignore first char of previous string
            data[startOfWord - 1] = characters[startOfWord - 1] == noBreakSpace ? noBreakSpace : toTitleCase(stringWithPrevious[startOfWord]);
        for (int i = startOfWord + 1; i < endOfWord; i++)
            data[i - 1] = characters[i - 1];
    }

    *string = String::adopt(data);
}

RenderText::RenderText(Node* node, PassRefPtr<StringImpl> str)
     : RenderObject(node)
     , m_minWidth(-1)
     , m_text(str)
     , m_firstTextBox(0)
     , m_lastTextBox(0)
     , m_maxWidth(-1)
     , m_beginMinWidth(0)
     , m_endMinWidth(0)
     , m_hasTab(false)
     , m_linesDirty(false)
     , m_containsReversedText(false)
     , m_isAllASCII(m_text.containsOnlyASCII())
     , m_knownToHaveNoOverflowAndNoFallbackFonts(false)
     , m_needsTranscoding(false)
{
    ASSERT(m_text);

    setIsText();

    // FIXME: It would be better to call this only if !m_text->containsOnlyWhitespace().
    // But that might slow things down, and maybe should only be done if visuallyNonEmpty
    // is still false. Not making any change for now, but should consider in the future.
    view()->frameView()->setIsVisuallyNonEmpty();
}

#ifndef NDEBUG

RenderText::~RenderText()
{
    ASSERT(!m_firstTextBox);
    ASSERT(!m_lastTextBox);
}

#endif

const char* RenderText::renderName() const
{
    return "RenderText";
}

bool RenderText::isTextFragment() const
{
    return false;
}

bool RenderText::isWordBreak() const
{
    return false;
}

void RenderText::updateNeedsTranscoding()
{
    const TextEncoding* encoding = document()->decoder() ? &document()->decoder()->encoding() : 0;
    m_needsTranscoding = fontTranscoder().needsTranscoding(style()->font().fontDescription(), encoding);
}

void RenderText::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    // There is no need to ever schedule repaints from a style change of a text run, since
    // we already did this for the parent of the text run.
    // We do have to schedule layouts, though, since a style change can force us to
    // need to relayout.
    if (diff == StyleDifferenceLayout) {
        setNeedsLayoutAndPrefWidthsRecalc();
        m_knownToHaveNoOverflowAndNoFallbackFonts = false;
    }

    bool needsResetText = false;
    if (!oldStyle) {
        updateNeedsTranscoding();
        needsResetText = m_needsTranscoding;
    } else if (oldStyle->font().needsTranscoding() != style()->font().needsTranscoding() || (style()->font().needsTranscoding() && oldStyle->font().family().family() != style()->font().family().family())) {
        updateNeedsTranscoding();
        needsResetText = true;
    }

    ETextTransform oldTransform = oldStyle ? oldStyle->textTransform() : TTNONE;
    ETextSecurity oldSecurity = oldStyle ? oldStyle->textSecurity() : TSNONE;
    if (needsResetText || oldTransform != style()->textTransform() || oldSecurity != style()->textSecurity()) {
        if (RefPtr<StringImpl> textToTransform = originalText())
            setText(textToTransform.release(), true);
    }
}

void RenderText::removeAndDestroyTextBoxes()
{
    if (!documentBeingDestroyed()) {
        if (firstTextBox()) {
            if (isBR()) {
                RootInlineBox* next = firstTextBox()->root()->nextRootBox();
                if (next)
                    next->markDirty();
            }
            for (InlineTextBox* box = firstTextBox(); box; box = box->nextTextBox())
                box->remove();
        } else if (parent())
            parent()->dirtyLinesFromChangedChild(this);
    }
    deleteTextBoxes();
}

void RenderText::destroy()
{
    if (SecureTextTimer* secureTextTimer = gSecureTextTimers ? gSecureTextTimers->take(this) : 0)
        delete secureTextTimer;

    removeAndDestroyTextBoxes();
    RenderObject::destroy();
}

void RenderText::extractTextBox(InlineTextBox* box)
{
    checkConsistency();

    m_lastTextBox = box->prevTextBox();
    if (box == m_firstTextBox)
        m_firstTextBox = 0;
    if (box->prevTextBox())
        box->prevTextBox()->setNextTextBox(0);
    box->setPreviousTextBox(0);
    for (InlineTextBox* curr = box; curr; curr = curr->nextTextBox())
        curr->setExtracted();

    checkConsistency();
}

void RenderText::attachTextBox(InlineTextBox* box)
{
    checkConsistency();

    if (m_lastTextBox) {
        m_lastTextBox->setNextTextBox(box);
        box->setPreviousTextBox(m_lastTextBox);
    } else
        m_firstTextBox = box;
    InlineTextBox* last = box;
    for (InlineTextBox* curr = box; curr; curr = curr->nextTextBox()) {
        curr->setExtracted(false);
        last = curr;
    }
    m_lastTextBox = last;

    checkConsistency();
}

void RenderText::removeTextBox(InlineTextBox* box)
{
    checkConsistency();

    if (box == m_firstTextBox)
        m_firstTextBox = box->nextTextBox();
    if (box == m_lastTextBox)
        m_lastTextBox = box->prevTextBox();
    if (box->nextTextBox())
        box->nextTextBox()->setPreviousTextBox(box->prevTextBox());
    if (box->prevTextBox())
        box->prevTextBox()->setNextTextBox(box->nextTextBox());

    checkConsistency();
}

void RenderText::deleteTextBoxes()
{
    if (firstTextBox()) {
        RenderArena* arena = renderArena();
        InlineTextBox* next;
        for (InlineTextBox* curr = firstTextBox(); curr; curr = next) {
            next = curr->nextTextBox();
            curr->destroy(arena);
        }
        m_firstTextBox = m_lastTextBox = 0;
    }
}

PassRefPtr<StringImpl> RenderText::originalText() const
{
    Node* e = node();
    return (e && e->isTextNode()) ? static_cast<Text*>(e)->dataImpl() : 0;
}

void RenderText::absoluteRects(Vector<IntRect>& rects, int tx, int ty)
{
    for (InlineTextBox* box = firstTextBox(); box; box = box->nextTextBox())
        rects.append(enclosingIntRect(FloatRect(tx + box->x(), ty + box->y(), box->width(), box->height())));
}

void RenderText::absoluteRectsForRange(Vector<IntRect>& rects, unsigned start, unsigned end, bool useSelectionHeight)
{
    // Work around signed/unsigned issues. This function takes unsigneds, and is often passed UINT_MAX
    // to mean "all the way to the end". InlineTextBox coordinates are unsigneds, so changing this 
    // function to take ints causes various internal mismatches. But selectionRect takes ints, and 
    // passing UINT_MAX to it causes trouble. Ideally we'd change selectionRect to take unsigneds, but 
    // that would cause many ripple effects, so for now we'll just clamp our unsigned parameters to INT_MAX.
    ASSERT(end == UINT_MAX || end <= INT_MAX);
    ASSERT(start <= INT_MAX);
    start = min(start, static_cast<unsigned>(INT_MAX));
    end = min(end, static_cast<unsigned>(INT_MAX));
    
    for (InlineTextBox* box = firstTextBox(); box; box = box->nextTextBox()) {
        // Note: box->end() returns the index of the last character, not the index past it
        if (start <= box->start() && box->end() < end) {
            IntRect r = box->calculateBoundaries();
            if (useSelectionHeight) {
                IntRect selectionRect = box->selectionRect(0, 0, start, end);
                if (box->isHorizontal()) {
                    r.setHeight(selectionRect.height());
                    r.setY(selectionRect.y());
                } else {
                    r.setWidth(selectionRect.width());
                    r.setX(selectionRect.x());
                }
            }
            rects.append(localToAbsoluteQuad(FloatQuad(r)).enclosingBoundingBox());
        } else {
            unsigned realEnd = min(box->end() + 1, end);
            IntRect r = box->selectionRect(0, 0, start, realEnd);
            if (!r.isEmpty()) {
                if (!useSelectionHeight) {
                    // change the height and y position because selectionRect uses selection-specific values
                    if (box->isHorizontal()) {
                        r.setHeight(box->logicalHeight());
                        r.setY(box->y());
                    } else {
                        r.setWidth(box->logicalWidth());
                        r.setX(box->x());
                    }
                }
                rects.append(localToAbsoluteQuad(FloatQuad(r)).enclosingBoundingBox());
            }
        }
    }
}

static IntRect ellipsisRectForBox(InlineTextBox* box, unsigned startPos, unsigned endPos)
{
    if (!box)
        return IntRect();
    
    unsigned short truncation = box->truncation();
    if (truncation == cNoTruncation)
        return IntRect();
    
    IntRect rect;
    if (EllipsisBox* ellipsis = box->root()->ellipsisBox()) {
        int ellipsisStartPosition = max<int>(startPos - box->start(), 0);
        int ellipsisEndPosition = min<int>(endPos - box->start(), box->len());
        
        // The ellipsis should be considered to be selected if the end of
        // the selection is past the beginning of the truncation and the
        // beginning of the selection is before or at the beginning of the truncation.
        if (ellipsisEndPosition >= truncation && ellipsisStartPosition <= truncation)
            return ellipsis->selectionRect(0, 0);
    }
    
    return IntRect();
}
    
void RenderText::absoluteQuads(Vector<FloatQuad>& quads, ClippingOption option)
{
    for (InlineTextBox* box = firstTextBox(); box; box = box->nextTextBox()) {
        IntRect boundaries = box->calculateBoundaries();

        // Shorten the width of this text box if it ends in an ellipsis.
        IntRect ellipsisRect = (option == ClipToEllipsis) ? ellipsisRectForBox(box, 0, textLength()) : IntRect();
        if (!ellipsisRect.isEmpty()) {
            if (style()->isHorizontalWritingMode())
                boundaries.setWidth(ellipsisRect.maxX() - boundaries.x());
            else
                boundaries.setHeight(ellipsisRect.maxY() - boundaries.y());
        }
        quads.append(localToAbsoluteQuad(FloatRect(boundaries)));
    }
}
    
void RenderText::absoluteQuads(Vector<FloatQuad>& quads)
{
    absoluteQuads(quads, NoClipping);
}

void RenderText::absoluteQuadsForRange(Vector<FloatQuad>& quads, unsigned start, unsigned end, bool useSelectionHeight)
{
    // Work around signed/unsigned issues. This function takes unsigneds, and is often passed UINT_MAX
    // to mean "all the way to the end". InlineTextBox coordinates are unsigneds, so changing this 
    // function to take ints causes various internal mismatches. But selectionRect takes ints, and 
    // passing UINT_MAX to it causes trouble. Ideally we'd change selectionRect to take unsigneds, but 
    // that would cause many ripple effects, so for now we'll just clamp our unsigned parameters to INT_MAX.
    ASSERT(end == UINT_MAX || end <= INT_MAX);
    ASSERT(start <= INT_MAX);
    start = min(start, static_cast<unsigned>(INT_MAX));
    end = min(end, static_cast<unsigned>(INT_MAX));
    
    for (InlineTextBox* box = firstTextBox(); box; box = box->nextTextBox()) {
        // Note: box->end() returns the index of the last character, not the index past it
        if (start <= box->start() && box->end() < end) {
            IntRect r(box->calculateBoundaries());
            if (useSelectionHeight) {
                IntRect selectionRect = box->selectionRect(0, 0, start, end);
                if (box->isHorizontal()) {
                    r.setHeight(selectionRect.height());
                    r.setY(selectionRect.y());
                } else {
                    r.setWidth(selectionRect.width());
                    r.setX(selectionRect.x());
                }
            }
            quads.append(localToAbsoluteQuad(FloatRect(r)));
        } else {
            unsigned realEnd = min(box->end() + 1, end);
            IntRect r = box->selectionRect(0, 0, start, realEnd);
            if (r.height()) {
                if (!useSelectionHeight) {
                    // change the height and y position because selectionRect uses selection-specific values
                    if (box->isHorizontal()) {
                        r.setHeight(box->logicalHeight());
                        r.setY(box->y());
                    } else {
                        r.setWidth(box->logicalHeight());
                        r.setX(box->x());
                    }
                }
                quads.append(localToAbsoluteQuad(FloatRect(r)));
            }
        }
    }
}

InlineTextBox* RenderText::findNextInlineTextBox(int offset, int& pos) const
{
    // The text runs point to parts of the RenderText's m_text
    // (they don't include '\n')
    // Find the text run that includes the character at offset
    // and return pos, which is the position of the char in the run.

    if (!m_firstTextBox)
        return 0;

    InlineTextBox* s = m_firstTextBox;
    int off = s->len();
    while (offset > off && s->nextTextBox()) {
        s = s->nextTextBox();
        off = s->start() + s->len();
    }
    // we are now in the correct text run
    pos = (offset > off ? s->len() : s->len() - (off - offset) );
    return s;
}

static bool lineDirectionPointFitsInBox(int pointLineDirection, InlineTextBox* box, int offset, EAffinity& affinity)
{
    affinity = DOWNSTREAM;

    // the x coordinate is equal to the left edge of this box
    // the affinity must be downstream so the position doesn't jump back to the previous line
    if (pointLineDirection == box->logicalLeft())
        return true;

    // and the x coordinate is to the left of the right edge of this box
    // check to see if position goes in this box
    if (pointLineDirection < box->logicalRight()) {
        affinity = offset > 0 ? VP_UPSTREAM_IF_POSSIBLE : DOWNSTREAM;
        return true;
    }

    // box is first on line
    // and the x coordinate is to the left of the first text box left edge
    if (!box->prevOnLine() && pointLineDirection < box->logicalLeft())
        return true;

    if (!box->nextOnLine()) {
        // box is last on line
        // and the x coordinate is to the right of the last text box right edge
        // generate VisiblePosition, use UPSTREAM affinity if possible
        affinity = offset > 0 ? VP_UPSTREAM_IF_POSSIBLE : DOWNSTREAM;
        return true;
    }

    return false;
}

VisiblePosition RenderText::positionForPoint(const IntPoint& point)
{
    if (!firstTextBox() || textLength() == 0)
        return createVisiblePosition(0, DOWNSTREAM);

    // Get the offset for the position, since this will take rtl text into account.
    int offset;

    int pointLineDirection = firstTextBox()->isHorizontal() ? point.x() : point.y();
    int pointBlockDirection = firstTextBox()->isHorizontal() ? point.y() : point.x();
    
    // FIXME: We should be able to roll these special cases into the general cases in the loop below.
    if (firstTextBox() && pointBlockDirection <  firstTextBox()->root()->selectionBottom() && pointLineDirection < firstTextBox()->logicalLeft()) {
        // at the y coordinate of the first line or above
        // and the x coordinate is to the left of the first text box left edge
        offset = firstTextBox()->offsetForPosition(pointLineDirection);
        return createVisiblePosition(offset + firstTextBox()->start(), offset > 0 ? VP_UPSTREAM_IF_POSSIBLE : DOWNSTREAM);
    }
    if (lastTextBox() && pointBlockDirection >= lastTextBox()->root()->selectionTop() && pointLineDirection >= lastTextBox()->logicalRight()) {
        // at the y coordinate of the last line or below
        // and the x coordinate is to the right of the last text box right edge
        offset = lastTextBox()->offsetForPosition(pointLineDirection);
        return createVisiblePosition(offset + lastTextBox()->start(), VP_UPSTREAM_IF_POSSIBLE);
    }

    InlineTextBox* lastBoxAbove = 0;
    for (InlineTextBox* box = firstTextBox(); box; box = box->nextTextBox()) {
        RootInlineBox* rootBox = box->root();
        if (pointBlockDirection >= rootBox->selectionTop() || pointBlockDirection >= rootBox->lineTop()) {
            int bottom = rootBox->selectionBottom();
            if (rootBox->nextRootBox())
                bottom = min(bottom, rootBox->nextRootBox()->lineTop());

            if (pointBlockDirection < bottom) {
                EAffinity affinity;
                int offset = box->offsetForPosition(pointLineDirection);
                if (lineDirectionPointFitsInBox(pointLineDirection, box, offset, affinity))
                    return createVisiblePosition(offset + box->start(), affinity);
            }
            lastBoxAbove = box;
        }
    }

    return createVisiblePosition(lastBoxAbove ? lastBoxAbove->start() + lastBoxAbove->len() : 0, DOWNSTREAM);
}

IntRect RenderText::localCaretRect(InlineBox* inlineBox, int caretOffset, int* extraWidthToEndOfLine)
{
    if (!inlineBox)
        return IntRect();

    ASSERT(inlineBox->isInlineTextBox());
    if (!inlineBox->isInlineTextBox())
        return IntRect();

    InlineTextBox* box = static_cast<InlineTextBox*>(inlineBox);

    int height = box->root()->selectionHeight();
    int top = box->root()->selectionTop();

    // Go ahead and round left to snap it to the nearest pixel.
    float left = box->positionForOffset(caretOffset);

    // Distribute the caret's width to either side of the offset.
    int caretWidthLeftOfOffset = caretWidth / 2;
    left -= caretWidthLeftOfOffset;
    int caretWidthRightOfOffset = caretWidth - caretWidthLeftOfOffset;

    left = roundf(left);

    float rootLeft = box->root()->logicalLeft();
    float rootRight = box->root()->logicalRight();

    // FIXME: should we use the width of the root inline box or the
    // width of the containing block for this?
    if (extraWidthToEndOfLine)
        *extraWidthToEndOfLine = (box->root()->logicalWidth() + rootLeft) - (left + 1);

    RenderBlock* cb = containingBlock();
    RenderStyle* cbStyle = cb->style();
    float leftEdge;
    float rightEdge;
    if (style()->autoWrap()) {
        leftEdge = 0;
        rightEdge = cb->logicalWidth();
    } else {
        leftEdge = min(static_cast<float>(0), rootLeft);
        rightEdge = max(static_cast<float>(cb->logicalWidth()), rootRight);
    }

    bool rightAligned = false;
    switch (cbStyle->textAlign()) {
    case TAAUTO:
    case JUSTIFY:
        rightAligned = !cbStyle->isLeftToRightDirection();
        break;
    case RIGHT:
    case WEBKIT_RIGHT:
        rightAligned = true;
        break;
    case LEFT:
    case WEBKIT_LEFT:
    case CENTER:
    case WEBKIT_CENTER:
        break;
    case TASTART:
        rightAligned = !cbStyle->isLeftToRightDirection();
        break;
    case TAEND:
        rightAligned = cbStyle->isLeftToRightDirection();
        break;
    }

    if (rightAligned) {
        left = max(left, leftEdge);
        left = min(left, rootRight - caretWidth);
    } else {
        left = min(left, rightEdge - caretWidthRightOfOffset);
        left = max(left, rootLeft);
    }

    return style()->isHorizontalWritingMode() ? IntRect(left, top, caretWidth, height) : IntRect(top, left, height, caretWidth);
}

ALWAYS_INLINE float RenderText::widthFromCache(const Font& f, int start, int len, float xPos, HashSet<const SimpleFontData*>* fallbackFonts, GlyphOverflow* glyphOverflow) const
{
    if (style()->hasTextCombine() && isCombineText()) {
        const RenderCombineText* combineText = toRenderCombineText(this);
        if (combineText->isCombined())
            return combineText->combinedTextWidth(f);
    }

    if (f.isFixedPitch() && !f.isSmallCaps() && m_isAllASCII && (!glyphOverflow || !glyphOverflow->computeBounds)) {
        float monospaceCharacterWidth = f.spaceWidth();
        float tabWidth = allowTabs() ? monospaceCharacterWidth * 8 : 0;
        float w = 0;
        bool isSpace;
        bool previousCharWasSpace = true; // FIXME: Preserves historical behavior, but seems wrong for start > 0.
        ASSERT(m_text);
        StringImpl& text = *m_text.impl();
        for (int i = start; i < start + len; i++) {
            char c = text[i];
            if (c <= ' ') {
                if (c == ' ' || c == '\n') {
                    w += monospaceCharacterWidth;
                    isSpace = true;
                } else if (c == '\t') {
                    w += tabWidth ? tabWidth - fmodf(xPos + w, tabWidth) : monospaceCharacterWidth;
                    isSpace = true;
                } else
                    isSpace = false;
            } else {
                w += monospaceCharacterWidth;
                isSpace = false;
            }
            if (isSpace && !previousCharWasSpace)
                w += f.wordSpacing();
            previousCharWasSpace = isSpace;
        }
        return w;
    }

    return f.width(TextRun(text()->characters() + start, len, allowTabs(), xPos), fallbackFonts, glyphOverflow);
}

void RenderText::trimmedPrefWidths(float leadWidth,
                                   float& beginMinW, bool& beginWS,
                                   float& endMinW, bool& endWS,
                                   bool& hasBreakableChar, bool& hasBreak,
                                   float& beginMaxW, float& endMaxW,
                                   float& minW, float& maxW, bool& stripFrontSpaces)
{
    bool collapseWhiteSpace = style()->collapseWhiteSpace();
    if (!collapseWhiteSpace)
        stripFrontSpaces = false;

    if (m_hasTab || preferredLogicalWidthsDirty())
        computePreferredLogicalWidths(leadWidth);

    beginWS = !stripFrontSpaces && m_hasBeginWS;
    endWS = m_hasEndWS;

    int len = textLength();

    if (!len || (stripFrontSpaces && text()->containsOnlyWhitespace())) {
        beginMinW = 0;
        endMinW = 0;
        beginMaxW = 0;
        endMaxW = 0;
        minW = 0;
        maxW = 0;
        hasBreak = false;
        return;
    }

    minW = m_minWidth;
    maxW = m_maxWidth;

    beginMinW = m_beginMinWidth;
    endMinW = m_endMinWidth;

    hasBreakableChar = m_hasBreakableChar;
    hasBreak = m_hasBreak;

    ASSERT(m_text);
    StringImpl& text = *m_text.impl();
    if (text[0] == ' ' || (text[0] == '\n' && !style()->preserveNewline()) || text[0] == '\t') {
        const Font& f = style()->font(); // FIXME: This ignores first-line.
        if (stripFrontSpaces) {
            const UChar space = ' ';
            float spaceWidth = f.width(TextRun(&space, 1));
            maxW -= spaceWidth;
        } else
            maxW += f.wordSpacing();
    }

    stripFrontSpaces = collapseWhiteSpace && m_hasEndWS;

    if (!style()->autoWrap() || minW > maxW)
        minW = maxW;

    // Compute our max widths by scanning the string for newlines.
    if (hasBreak) {
        const Font& f = style()->font(); // FIXME: This ignores first-line.
        bool firstLine = true;
        beginMaxW = maxW;
        endMaxW = maxW;
        for (int i = 0; i < len; i++) {
            int linelen = 0;
            while (i + linelen < len && text[i + linelen] != '\n')
                linelen++;

            if (linelen) {
                endMaxW = widthFromCache(f, i, linelen, leadWidth + endMaxW, 0, 0);
                if (firstLine) {
                    firstLine = false;
                    leadWidth = 0;
                    beginMaxW = endMaxW;
                }
                i += linelen;
            } else if (firstLine) {
                beginMaxW = 0;
                firstLine = false;
                leadWidth = 0;
            }

            if (i == len - 1)
                // A <pre> run that ends with a newline, as in, e.g.,
                // <pre>Some text\n\n<span>More text</pre>
                endMaxW = 0;
        }
    }
}

static inline bool isSpaceAccordingToStyle(UChar c, RenderStyle* style)
{
    return c == ' ' || (c == noBreakSpace && style->nbspMode() == SPACE);
}

float RenderText::minLogicalWidth() const
{
    if (preferredLogicalWidthsDirty())
        const_cast<RenderText*>(this)->computePreferredLogicalWidths(0);
        
    return m_minWidth;
}

float RenderText::maxLogicalWidth() const
{
    if (preferredLogicalWidthsDirty())
        const_cast<RenderText*>(this)->computePreferredLogicalWidths(0);
        
    return m_maxWidth;
}

void RenderText::computePreferredLogicalWidths(float leadWidth)
{
    HashSet<const SimpleFontData*> fallbackFonts;
    GlyphOverflow glyphOverflow;
    computePreferredLogicalWidths(leadWidth, fallbackFonts, glyphOverflow);
    if (fallbackFonts.isEmpty() && !glyphOverflow.left && !glyphOverflow.right && !glyphOverflow.top && !glyphOverflow.bottom)
        m_knownToHaveNoOverflowAndNoFallbackFonts = true;
}

void RenderText::computePreferredLogicalWidths(float leadWidth, HashSet<const SimpleFontData*>& fallbackFonts, GlyphOverflow& glyphOverflow)
{
    ASSERT(m_hasTab || preferredLogicalWidthsDirty() || !m_knownToHaveNoOverflowAndNoFallbackFonts);

    m_minWidth = 0;
    m_beginMinWidth = 0;
    m_endMinWidth = 0;
    m_maxWidth = 0;

    if (isBR())
        return;

    float currMinWidth = 0;
    float currMaxWidth = 0;
    m_hasBreakableChar = false;
    m_hasBreak = false;
    m_hasTab = false;
    m_hasBeginWS = false;
    m_hasEndWS = false;

    const Font& f = style()->font(); // FIXME: This ignores first-line.
    float wordSpacing = style()->wordSpacing();
    int len = textLength();
    const UChar* txt = characters();
    LazyLineBreakIterator breakIterator(txt, len);
    bool needsWordSpacing = false;
    bool ignoringSpaces = false;
    bool isSpace = false;
    bool firstWord = true;
    bool firstLine = true;
    int nextBreakable = -1;
    int lastWordBoundary = 0;

    int firstGlyphLeftOverflow = -1;

    bool breakNBSP = style()->autoWrap() && style()->nbspMode() == SPACE;
    bool breakAll = (style()->wordBreak() == BreakAllWordBreak || style()->wordBreak() == BreakWordBreak) && style()->autoWrap();

    for (int i = 0; i < len; i++) {
        UChar c = txt[i];

        bool previousCharacterIsSpace = isSpace;

        bool isNewline = false;
        if (c == '\n') {
            if (style()->preserveNewline()) {
                m_hasBreak = true;
                isNewline = true;
                isSpace = false;
            } else
                isSpace = true;
        } else if (c == '\t') {
            if (!style()->collapseWhiteSpace()) {
                m_hasTab = true;
                isSpace = false;
            } else
                isSpace = true;
        } else
            isSpace = c == ' ';

        if ((isSpace || isNewline) && !i)
            m_hasBeginWS = true;
        if ((isSpace || isNewline) && i == len - 1)
            m_hasEndWS = true;

        if (!ignoringSpaces && style()->collapseWhiteSpace() && previousCharacterIsSpace && isSpace)
            ignoringSpaces = true;

        if (ignoringSpaces && !isSpace)
            ignoringSpaces = false;

        // Ignore spaces and soft hyphens
        if (ignoringSpaces) {
            ASSERT(lastWordBoundary == i);
            lastWordBoundary++;
            continue;
        } else if (c == softHyphen) {
            currMaxWidth += widthFromCache(f, lastWordBoundary, i - lastWordBoundary, leadWidth + currMaxWidth, &fallbackFonts, &glyphOverflow);
            if (firstGlyphLeftOverflow < 0)
                firstGlyphLeftOverflow = glyphOverflow.left;
            lastWordBoundary = i + 1;
            continue;
        }

        bool hasBreak = breakAll || isBreakable(breakIterator, i, nextBreakable, breakNBSP);
        bool betweenWords = true;
        int j = i;
        while (c != '\n' && !isSpaceAccordingToStyle(c, style()) && c != '\t' && c != softHyphen) {
            j++;
            if (j == len)
                break;
            c = txt[j];
            if (isBreakable(breakIterator, j, nextBreakable, breakNBSP))
                break;
            if (breakAll) {
                betweenWords = false;
                break;
            }
        }

        int wordLen = j - i;
        if (wordLen) {
            float w = widthFromCache(f, i, wordLen, leadWidth + currMaxWidth, &fallbackFonts, &glyphOverflow);
            if (firstGlyphLeftOverflow < 0)
                firstGlyphLeftOverflow = glyphOverflow.left;
            currMinWidth += w;
            if (betweenWords) {
                if (lastWordBoundary == i)
                    currMaxWidth += w;
                else
                    currMaxWidth += widthFromCache(f, lastWordBoundary, j - lastWordBoundary, leadWidth + currMaxWidth, &fallbackFonts, &glyphOverflow);
                lastWordBoundary = j;
            }

            bool isSpace = (j < len) && isSpaceAccordingToStyle(c, style());
            bool isCollapsibleWhiteSpace = (j < len) && style()->isCollapsibleWhiteSpace(c);
            if (j < len && style()->autoWrap())
                m_hasBreakableChar = true;

            // Add in wordSpacing to our currMaxWidth, but not if this is the last word on a line or the
            // last word in the run.
            if (wordSpacing && (isSpace || isCollapsibleWhiteSpace) && !containsOnlyWhitespace(j, len-j))
                currMaxWidth += wordSpacing;

            if (firstWord) {
                firstWord = false;
                // If the first character in the run is breakable, then we consider ourselves to have a beginning
                // minimum width of 0, since a break could occur right before our run starts, preventing us from ever
                // being appended to a previous text run when considering the total minimum width of the containing block.
                if (hasBreak)
                    m_hasBreakableChar = true;
                m_beginMinWidth = hasBreak ? 0 : w;
            }
            m_endMinWidth = w;

            if (currMinWidth > m_minWidth)
                m_minWidth = currMinWidth;
            currMinWidth = 0;

            i += wordLen - 1;
        } else {
            // Nowrap can never be broken, so don't bother setting the
            // breakable character boolean. Pre can only be broken if we encounter a newline.
            if (style()->autoWrap() || isNewline)
                m_hasBreakableChar = true;

            if (currMinWidth > m_minWidth)
                m_minWidth = currMinWidth;
            currMinWidth = 0;

            if (isNewline) { // Only set if preserveNewline was true and we saw a newline.
                if (firstLine) {
                    firstLine = false;
                    leadWidth = 0;
                    if (!style()->autoWrap())
                        m_beginMinWidth = currMaxWidth;
                }

                if (currMaxWidth > m_maxWidth)
                    m_maxWidth = currMaxWidth;
                currMaxWidth = 0;
            } else {
                currMaxWidth += f.width(TextRun(txt + i, 1, allowTabs(), leadWidth + currMaxWidth));
                glyphOverflow.right = 0;
                needsWordSpacing = isSpace && !previousCharacterIsSpace && i == len - 1;
            }
            ASSERT(lastWordBoundary == i);
            lastWordBoundary++;
        }
    }

    if (firstGlyphLeftOverflow > 0)
        glyphOverflow.left = firstGlyphLeftOverflow;

    if ((needsWordSpacing && len > 1) || (ignoringSpaces && !firstWord))
        currMaxWidth += wordSpacing;

    m_minWidth = max(currMinWidth, m_minWidth);
    m_maxWidth = max(currMaxWidth, m_maxWidth);

    if (!style()->autoWrap())
        m_minWidth = m_maxWidth;

    if (style()->whiteSpace() == PRE) {
        if (firstLine)
            m_beginMinWidth = m_maxWidth;
        m_endMinWidth = currMaxWidth;
    }

    setPreferredLogicalWidthsDirty(false);
}

bool RenderText::isAllCollapsibleWhitespace()
{
    int length = textLength();
    const UChar* text = characters();
    for (int i = 0; i < length; i++) {
        if (!style()->isCollapsibleWhiteSpace(text[i]))
            return false;
    }
    return true;
}
    
bool RenderText::containsOnlyWhitespace(unsigned from, unsigned len) const
{
    ASSERT(m_text);
    StringImpl& text = *m_text.impl();
    unsigned currPos;
    for (currPos = from;
         currPos < from + len && (text[currPos] == '\n' || text[currPos] == ' ' || text[currPos] == '\t');
         currPos++) { }
    return currPos >= (from + len);
}

FloatPoint RenderText::firstRunOrigin() const
{
    return IntPoint(firstRunX(), firstRunY());
}

float RenderText::firstRunX() const
{
    return m_firstTextBox ? m_firstTextBox->m_x : 0;
}

float RenderText::firstRunY() const
{
    return m_firstTextBox ? m_firstTextBox->m_y : 0;
}
    
void RenderText::setSelectionState(SelectionState state)
{
    InlineTextBox* box;

    RenderObject::setSelectionState(state);
    if (state == SelectionStart || state == SelectionEnd || state == SelectionBoth) {
        int startPos, endPos;
        selectionStartEnd(startPos, endPos);
        if (selectionState() == SelectionStart) {
            endPos = textLength();

            // to handle selection from end of text to end of line
            if (startPos != 0 && startPos == endPos)
                startPos = endPos - 1;
        } else if (selectionState() == SelectionEnd)
            startPos = 0;

        for (box = firstTextBox(); box; box = box->nextTextBox()) {
            if (box->isSelected(startPos, endPos)) {
                RootInlineBox* line = box->root();
                if (line)
                    line->setHasSelectedChildren(true);
            }
        }
    } else {
        for (box = firstTextBox(); box; box = box->nextTextBox()) {
            RootInlineBox* line = box->root();
            if (line)
                line->setHasSelectedChildren(state == SelectionInside);
        }
    }

    // The returned value can be null in case of an orphaned tree.
    if (RenderBlock* cb = containingBlock())
        cb->setSelectionState(state);
}

void RenderText::setTextWithOffset(PassRefPtr<StringImpl> text, unsigned offset, unsigned len, bool force)
{
    unsigned oldLen = textLength();
    unsigned newLen = text->length();
    int delta = newLen - oldLen;
    unsigned end = len ? offset + len - 1 : offset;

    RootInlineBox* firstRootBox = 0;
    RootInlineBox* lastRootBox = 0;

    bool dirtiedLines = false;

    // Dirty all text boxes that include characters in between offset and offset+len.
    for (InlineTextBox* curr = firstTextBox(); curr; curr = curr->nextTextBox()) {
        // Text run is entirely before the affected range.
        if (curr->end() < offset)
            continue;

        // Text run is entirely after the affected range.
        if (curr->start() > end) {
            curr->offsetRun(delta);
            RootInlineBox* root = curr->root();
            if (!firstRootBox) {
                firstRootBox = root;
                if (!dirtiedLines) {
                    // The affected area was in between two runs. Go ahead and mark the root box of
                    // the run after the affected area as dirty.
                    firstRootBox->markDirty();
                    dirtiedLines = true;
                }
            }
            lastRootBox = root;
        } else if (curr->end() >= offset && curr->end() <= end) {
            // Text run overlaps with the left end of the affected range.
            curr->dirtyLineBoxes();
            dirtiedLines = true;
        } else if (curr->start() <= offset && curr->end() >= end) {
            // Text run subsumes the affected range.
            curr->dirtyLineBoxes();
            dirtiedLines = true;
        } else if (curr->start() <= end && curr->end() >= end) {
            // Text run overlaps with right end of the affected range.
            curr->dirtyLineBoxes();
            dirtiedLines = true;
        }
    }

    // Now we have to walk all of the clean lines and adjust their cached line break information
    // to reflect our updated offsets.
    if (lastRootBox)
        lastRootBox = lastRootBox->nextRootBox();
    if (firstRootBox) {
        RootInlineBox* prev = firstRootBox->prevRootBox();
        if (prev)
            firstRootBox = prev;
    } else if (lastTextBox()) {
        ASSERT(!lastRootBox);
        firstRootBox = lastTextBox()->root();
        firstRootBox->markDirty();
        dirtiedLines = true;
    }
    for (RootInlineBox* curr = firstRootBox; curr && curr != lastRootBox; curr = curr->nextRootBox()) {
        if (curr->lineBreakObj() == this && curr->lineBreakPos() > end)
            curr->setLineBreakPos(curr->lineBreakPos() + delta);
    }

    // If the text node is empty, dirty the line where new text will be inserted.
    if (!firstTextBox() && parent()) {
        parent()->dirtyLinesFromChangedChild(this);
        dirtiedLines = true;
    }

    m_linesDirty = dirtiedLines;
    setText(text, force);
}

static inline bool isInlineFlowOrEmptyText(const RenderObject* o)
{
    if (o->isRenderInline())
        return true;
    if (!o->isText())
        return false;
    StringImpl* text = toRenderText(o)->text();
    if (!text)
        return true;
    return !text->length();
}

UChar RenderText::previousCharacter() const
{
    // find previous text renderer if one exists
    const RenderObject* previousText = this;
    while ((previousText = previousText->previousInPreOrder()))
        if (!isInlineFlowOrEmptyText(previousText))
            break;
    UChar prev = ' ';
    if (previousText && previousText->isText())
        if (StringImpl* previousString = toRenderText(previousText)->text())
            prev = (*previousString)[previousString->length() - 1];
    return prev;
}

void RenderText::transformText(String& text) const
{
    ASSERT(style());
    switch (style()->textTransform()) {
    case TTNONE:
        break;
    case CAPITALIZE:
        makeCapitalized(&text, previousCharacter());
        break;
    case UPPERCASE:
        text.makeUpper();
        break;
    case LOWERCASE:
        text.makeLower();
        break;
    }
}

void RenderText::setTextInternal(PassRefPtr<StringImpl> text)
{
    ASSERT(text);
    m_text = text;
    if (m_needsTranscoding) {
        const TextEncoding* encoding = document()->decoder() ? &document()->decoder()->encoding() : 0;
        fontTranscoder().convert(m_text, style()->font().fontDescription(), encoding);
    }
    ASSERT(m_text);

    if (style()) {
        transformText(m_text);

        // We use the same characters here as for list markers.
        // See the listMarkerText function in RenderListMarker.cpp.
        switch (style()->textSecurity()) {
        case TSNONE:
            break;
        case TSCIRCLE:
            secureText(whiteBullet);
            break;
        case TSDISC:
            secureText(bullet);
            break;
        case TSSQUARE:
            secureText(blackSquare);
        }
    }

    ASSERT(m_text);
    ASSERT(!isBR() || (textLength() == 1 && m_text[0] == '\n'));

    m_isAllASCII = m_text.containsOnlyASCII();
}

void RenderText::secureText(UChar mask)
{
    if (!m_text.length())
        return;

    int lastTypedCharacterOffsetToReveal = -1;
    String revealedText;
    SecureTextTimer* secureTextTimer = gSecureTextTimers ? gSecureTextTimers->get(this) : 0;
    if (secureTextTimer && secureTextTimer->isActive()) {
        lastTypedCharacterOffsetToReveal = secureTextTimer->lastTypedCharacterOffset();
        if (lastTypedCharacterOffsetToReveal >= 0)
            revealedText.append(m_text[lastTypedCharacterOffsetToReveal]);
    }

    m_text.fill(mask);
    if (lastTypedCharacterOffsetToReveal >= 0) {
        m_text.replace(lastTypedCharacterOffsetToReveal, 1, revealedText);
        // m_text may be updated later before timer fires. We invalidate the lastTypedCharacterOffset to avoid inconsistency.
        secureTextTimer->invalidate();
    }
}

void RenderText::setText(PassRefPtr<StringImpl> text, bool force)
{
    ASSERT(text);

    if (!force && equal(m_text.impl(), text.get()))
        return;

    setTextInternal(text);
    setNeedsLayoutAndPrefWidthsRecalc();
    m_knownToHaveNoOverflowAndNoFallbackFonts = false;
    
    AXObjectCache* axObjectCache = document()->axObjectCache();
    if (axObjectCache->accessibilityEnabled())
        axObjectCache->contentChanged(this);
}

String RenderText::textWithoutTranscoding() const
{
    // If m_text isn't transcoded or is secure, we can just return the modified text.
    if (!m_needsTranscoding || style()->textSecurity() != TSNONE)
        return text();

    // Otherwise, we should use original text. If text-transform is
    // specified, we should transform the text on the fly.
    String text = originalText();
    if (style())
        transformText(text);
    return text;
}

void RenderText::dirtyLineBoxes(bool fullLayout)
{
    if (fullLayout)
        deleteTextBoxes();
    else if (!m_linesDirty) {
        for (InlineTextBox* box = firstTextBox(); box; box = box->nextTextBox())
            box->dirtyLineBoxes();
    }
    m_linesDirty = false;
}

InlineTextBox* RenderText::createTextBox()
{
    return new (renderArena()) InlineTextBox(this);
}

InlineTextBox* RenderText::createInlineTextBox()
{
    InlineTextBox* textBox = createTextBox();
    if (!m_firstTextBox)
        m_firstTextBox = m_lastTextBox = textBox;
    else {
        m_lastTextBox->setNextTextBox(textBox);
        textBox->setPreviousTextBox(m_lastTextBox);
        m_lastTextBox = textBox;
    }
    textBox->setIsText(true);
    return textBox;
}

void RenderText::positionLineBox(InlineBox* box)
{
    InlineTextBox* s = static_cast<InlineTextBox*>(box);

    // FIXME: should not be needed!!!
    if (!s->len()) {
        // We want the box to be destroyed.
        s->remove();
        if (m_firstTextBox == s)
            m_firstTextBox = s->nextTextBox();
        else
            s->prevTextBox()->setNextTextBox(s->nextTextBox());
        if (m_lastTextBox == s)
            m_lastTextBox = s->prevTextBox();
        else
            s->nextTextBox()->setPreviousTextBox(s->prevTextBox());
        s->destroy(renderArena());
        return;
    }

    m_containsReversedText |= !s->isLeftToRightDirection();
}

float RenderText::width(unsigned from, unsigned len, float xPos, bool firstLine, HashSet<const SimpleFontData*>* fallbackFonts, GlyphOverflow* glyphOverflow) const
{
    if (from >= textLength())
        return 0;

    if (from + len > textLength())
        len = textLength() - from;

    return width(from, len, style(firstLine)->font(), xPos, fallbackFonts, glyphOverflow);
}

float RenderText::width(unsigned from, unsigned len, const Font& f, float xPos, HashSet<const SimpleFontData*>* fallbackFonts, GlyphOverflow* glyphOverflow) const
{
    ASSERT(from + len <= textLength());
    if (!characters())
        return 0;

    float w;
    if (&f == &style()->font()) {
        if (!style()->preserveNewline() && !from && len == textLength() && (!glyphOverflow || !glyphOverflow->computeBounds)) {
            if (fallbackFonts) {
                ASSERT(glyphOverflow);
                if (preferredLogicalWidthsDirty() || !m_knownToHaveNoOverflowAndNoFallbackFonts) {
                    const_cast<RenderText*>(this)->computePreferredLogicalWidths(0, *fallbackFonts, *glyphOverflow);
                    if (fallbackFonts->isEmpty() && !glyphOverflow->left && !glyphOverflow->right && !glyphOverflow->top && !glyphOverflow->bottom)
                        m_knownToHaveNoOverflowAndNoFallbackFonts = true;
                }
                w = m_maxWidth;
            } else
                w = maxLogicalWidth();
        } else
            w = widthFromCache(f, from, len, xPos, fallbackFonts, glyphOverflow);
    } else
        w = f.width(TextRun(text()->characters() + from, len, allowTabs(), xPos), fallbackFonts, glyphOverflow);

    return w;
}

IntRect RenderText::linesBoundingBox() const
{
    IntRect result;
    
    ASSERT(!firstTextBox() == !lastTextBox());  // Either both are null or both exist.
    if (firstTextBox() && lastTextBox()) {
        // Return the width of the minimal left side and the maximal right side.
        float logicalLeftSide = 0;
        float logicalRightSide = 0;
        for (InlineTextBox* curr = firstTextBox(); curr; curr = curr->nextTextBox()) {
            if (curr == firstTextBox() || curr->logicalLeft() < logicalLeftSide)
                logicalLeftSide = curr->logicalLeft();
            if (curr == firstTextBox() || curr->logicalRight() > logicalRightSide)
                logicalRightSide = curr->logicalRight();
        }
        
        bool isHorizontal = style()->isHorizontalWritingMode();
        
        float x = isHorizontal ? logicalLeftSide : firstTextBox()->x();
        float y = isHorizontal ? firstTextBox()->y() : logicalLeftSide;
        float width = isHorizontal ? logicalRightSide - logicalLeftSide : lastTextBox()->logicalBottom() - x;
        float height = isHorizontal ? lastTextBox()->logicalBottom() - y : logicalRightSide - logicalLeftSide;
        result = enclosingIntRect(FloatRect(x, y, width, height));
    }

    return result;
}

IntRect RenderText::linesVisualOverflowBoundingBox() const
{
    if (!firstTextBox())
        return IntRect();

    // Return the width of the minimal left side and the maximal right side.
    int logicalLeftSide = numeric_limits<int>::max();
    int logicalRightSide = numeric_limits<int>::min();
    for (InlineTextBox* curr = firstTextBox(); curr; curr = curr->nextTextBox()) {
        logicalLeftSide = min(logicalLeftSide, curr->logicalLeftVisualOverflow());
        logicalRightSide = max(logicalRightSide, curr->logicalRightVisualOverflow());
    }
    
    int logicalTop = firstTextBox()->logicalTopVisualOverflow();
    int logicalWidth = logicalRightSide - logicalLeftSide;
    int logicalHeight = lastTextBox()->logicalBottomVisualOverflow() - logicalTop;
    
    IntRect rect(logicalLeftSide, logicalTop, logicalWidth, logicalHeight);
    if (!style()->isHorizontalWritingMode())
        rect = rect.transposedRect();
    return rect;
}

IntRect RenderText::clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer)
{
    RenderObject* rendererToRepaint = containingBlock();

    // Do not cross self-painting layer boundaries.
    RenderObject* enclosingLayerRenderer = enclosingLayer()->renderer();
    if (enclosingLayerRenderer != rendererToRepaint && !rendererToRepaint->isDescendantOf(enclosingLayerRenderer))
        rendererToRepaint = enclosingLayerRenderer;

    // The renderer we chose to repaint may be an ancestor of repaintContainer, but we need to do a repaintContainer-relative repaint.
    if (repaintContainer && repaintContainer != rendererToRepaint && !rendererToRepaint->isDescendantOf(repaintContainer))
        return repaintContainer->clippedOverflowRectForRepaint(repaintContainer);

    return rendererToRepaint->clippedOverflowRectForRepaint(repaintContainer);
}

IntRect RenderText::selectionRectForRepaint(RenderBoxModelObject* repaintContainer, bool clipToVisibleContent)
{
    ASSERT(!needsLayout());

    if (selectionState() == SelectionNone)
        return IntRect();
    RenderBlock* cb = containingBlock();
    if (!cb)
        return IntRect();

    // Now calculate startPos and endPos for painting selection.
    // We include a selection while endPos > 0
    int startPos, endPos;
    if (selectionState() == SelectionInside) {
        // We are fully selected.
        startPos = 0;
        endPos = textLength();
    } else {
        selectionStartEnd(startPos, endPos);
        if (selectionState() == SelectionStart)
            endPos = textLength();
        else if (selectionState() == SelectionEnd)
            startPos = 0;
    }

    if (startPos == endPos)
        return IntRect();

    IntRect rect;
    for (InlineTextBox* box = firstTextBox(); box; box = box->nextTextBox()) {
        rect.unite(box->selectionRect(0, 0, startPos, endPos));
        rect.unite(ellipsisRectForBox(box, startPos, endPos));
    }

    if (clipToVisibleContent)
        computeRectForRepaint(repaintContainer, rect);
    else {
        if (cb->hasColumns())
            cb->adjustRectForColumns(rect);

        rect = localToContainerQuad(FloatRect(rect), repaintContainer).enclosingBoundingBox();
    }

    return rect;
}

int RenderText::caretMinOffset() const
{
    InlineTextBox* box = firstTextBox();
    if (!box)
        return 0;
    int minOffset = box->start();
    for (box = box->nextTextBox(); box; box = box->nextTextBox())
        minOffset = min<int>(minOffset, box->start());
    return minOffset;
}

int RenderText::caretMaxOffset() const
{
    InlineTextBox* box = lastTextBox();
    if (!box)
        return textLength();
    int maxOffset = box->start() + box->len();
    for (box = box->prevTextBox(); box; box = box->prevTextBox())
        maxOffset = max<int>(maxOffset, box->start() + box->len());
    return maxOffset;
}

unsigned RenderText::caretMaxRenderedOffset() const
{
    int l = 0;
    for (InlineTextBox* box = firstTextBox(); box; box = box->nextTextBox())
        l += box->len();
    return l;
}

int RenderText::previousOffset(int current) const
{
    StringImpl* si = m_text.impl();
    TextBreakIterator* iterator = cursorMovementIterator(si->characters(), si->length());
    if (!iterator)
        return current - 1;

    long result = textBreakPreceding(iterator, current);
    if (result == TextBreakDone)
        result = current - 1;


    return result;
}

#if PLATFORM(MAC)

#define HANGUL_CHOSEONG_START (0x1100)
#define HANGUL_CHOSEONG_END (0x115F)
#define HANGUL_JUNGSEONG_START (0x1160)
#define HANGUL_JUNGSEONG_END (0x11A2)
#define HANGUL_JONGSEONG_START (0x11A8)
#define HANGUL_JONGSEONG_END (0x11F9)
#define HANGUL_SYLLABLE_START (0xAC00)
#define HANGUL_SYLLABLE_END (0xD7AF)
#define HANGUL_JONGSEONG_COUNT (28)

enum HangulState {
    HangulStateL,
    HangulStateV,
    HangulStateT,
    HangulStateLV,
    HangulStateLVT,
    HangulStateBreak
};

inline bool isHangulLVT(UChar32 character)
{
    return (character - HANGUL_SYLLABLE_START) % HANGUL_JONGSEONG_COUNT;
}

inline bool isMark(UChar32 c)
{
    int8_t charType = u_charType(c);
    return charType == U_NON_SPACING_MARK || charType == U_ENCLOSING_MARK || charType == U_COMBINING_SPACING_MARK;
}

#endif

int RenderText::previousOffsetForBackwardDeletion(int current) const
{
#if PLATFORM(MAC)
    ASSERT(m_text);
    StringImpl& text = *m_text.impl();
    UChar32 character;
    while (current > 0) {
        if (U16_IS_TRAIL(text[--current]))
            --current;
        if (current < 0)
            break;

        UChar32 character = text.characterStartingAt(current);

        // We don't combine characters in Armenian ... Limbu range for backward deletion.
        if ((character >= 0x0530) && (character < 0x1950))
            break;

        if (!isMark(character) && (character != 0xFF9E) && (character != 0xFF9F))
            break;
    }

    if (current <= 0)
        return current;

    // Hangul
    character = text.characterStartingAt(current);
    if (((character >= HANGUL_CHOSEONG_START) && (character <= HANGUL_JONGSEONG_END)) || ((character >= HANGUL_SYLLABLE_START) && (character <= HANGUL_SYLLABLE_END))) {
        HangulState state;
        HangulState initialState;

        if (character < HANGUL_JUNGSEONG_START)
            state = HangulStateL;
        else if (character < HANGUL_JONGSEONG_START)
            state = HangulStateV;
        else if (character < HANGUL_SYLLABLE_START)
            state = HangulStateT;
        else
            state = isHangulLVT(character) ? HangulStateLVT : HangulStateLV;

        initialState = state;

        while (current > 0 && ((character = text.characterStartingAt(current - 1)) >= HANGUL_CHOSEONG_START) && (character <= HANGUL_SYLLABLE_END) && ((character <= HANGUL_JONGSEONG_END) || (character >= HANGUL_SYLLABLE_START))) {
            switch (state) {
            case HangulStateV:
                if (character <= HANGUL_CHOSEONG_END)
                    state = HangulStateL;
                else if ((character >= HANGUL_SYLLABLE_START) && (character <= HANGUL_SYLLABLE_END) && !isHangulLVT(character))
                    state = HangulStateLV;
                else if (character > HANGUL_JUNGSEONG_END)
                    state = HangulStateBreak;
                break;
            case HangulStateT:
                if ((character >= HANGUL_JUNGSEONG_START) && (character <= HANGUL_JUNGSEONG_END))
                    state = HangulStateV;
                else if ((character >= HANGUL_SYLLABLE_START) && (character <= HANGUL_SYLLABLE_END))
                    state = (isHangulLVT(character) ? HangulStateLVT : HangulStateLV);
                else if (character < HANGUL_JUNGSEONG_START)
                    state = HangulStateBreak;
                break;
            default:
                state = (character < HANGUL_JUNGSEONG_START) ? HangulStateL : HangulStateBreak;
                break;
            }
            if (state == HangulStateBreak)
                break;

            --current;
        }
    }

    return current;
#else
    // Platforms other than Mac delete by one code point.
    return current - 1;
#endif
}

int RenderText::nextOffset(int current) const
{
    StringImpl* si = m_text.impl();
    TextBreakIterator* iterator = cursorMovementIterator(si->characters(), si->length());
    if (!iterator)
        return current + 1;

    long result = textBreakFollowing(iterator, current);
    if (result == TextBreakDone)
        result = current + 1;


    return result;
}

#ifndef NDEBUG

void RenderText::checkConsistency() const
{
#ifdef CHECK_CONSISTENCY
    const InlineTextBox* prev = 0;
    for (const InlineTextBox* child = m_firstTextBox; child != 0; child = child->nextTextBox()) {
        ASSERT(child->renderer() == this);
        ASSERT(child->prevTextBox() == prev);
        prev = child;
    }
    ASSERT(prev == m_lastTextBox);
#endif
}

#endif

void RenderText::momentarilyRevealLastTypedCharacter(unsigned lastTypedCharacterOffset)
{
    if (!gSecureTextTimers)
        gSecureTextTimers = new SecureTextTimerMap;

    SecureTextTimer* secureTextTimer = gSecureTextTimers->get(this);
    if (!secureTextTimer) {
        secureTextTimer = new SecureTextTimer(this);
        gSecureTextTimers->add(this, secureTextTimer);
    }
    secureTextTimer->restartWithNewText(lastTypedCharacterOffset);
}

} // namespace WebCore
