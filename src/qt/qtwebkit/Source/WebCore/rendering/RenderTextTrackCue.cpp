/*
 * Copyright (C) 2012 Victor Carbune (victor@rosedu.org)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if ENABLE(VIDEO_TRACK)

#include "RenderTextTrackCue.h"

#include "TextTrackCue.h"
#include "TextTrackCueGeneric.h"
#include <wtf/StackStats.h>

namespace WebCore {

RenderTextTrackCue::RenderTextTrackCue(TextTrackCueBox* element)
    : RenderBlock(element)
    , m_cue(element->getCue())
{
}

void RenderTextTrackCue::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;
    RenderBlock::layout();

    LayoutStateMaintainer statePusher(view(), this, locationOffset(), hasTransform() || hasReflection() || style()->isFlippedBlocksWritingMode());
    
    if (m_cue->cueType()== TextTrackCue::WebVTT) {
        if (m_cue->snapToLines())
            repositionCueSnapToLinesSet();
        else
            repositionCueSnapToLinesNotSet();
    } else
        repositionGenericCue();

    statePusher.pop();
}

bool RenderTextTrackCue::initializeLayoutParameters(InlineFlowBox*& firstLineBox, LayoutUnit& step, LayoutUnit& position)
{
    ASSERT(firstChild());

    RenderBlock* parentBlock = containingBlock();
    firstLineBox = toRenderInline(firstChild())->firstLineBox();

    // 1. Horizontal: Let step be the height of the first line box in boxes.
    //    Vertical: Let step be the width of the first line box in boxes.
    step = m_cue->getWritingDirection() == TextTrackCue::Horizontal ? firstLineBox->height() : firstLineBox->width();

    // 2. If step is zero, then jump to the step labeled done positioning below.
    if (!step)
        return false;

    // 3. Let line position be the text track cue computed line position.
    int linePosition = m_cue->calculateComputedLinePosition();

    // 4. Vertical Growing Left: Add one to line position then negate it.
    if (m_cue->getWritingDirection() == TextTrackCue::VerticalGrowingLeft)
        linePosition = -(linePosition + 1);

    // 5. Let position be the result of multiplying step and line position.
    position = step * linePosition;

    // 6. Vertical Growing Left: Decrease position by the width of the
    // bounding box of the boxes in boxes, then increase position by step.
    if (m_cue->getWritingDirection() == TextTrackCue::VerticalGrowingLeft) {
        position -= width();
        position += step;
    }

    // 7. If line position is less than zero...
    if (linePosition < 0) {
        // Horizontal / Vertical: ... then increase position by the
        // height / width of the video's rendering area ...
        position += m_cue->getWritingDirection() == TextTrackCue::Horizontal ? parentBlock->height() : parentBlock->width();

        // ... and negate step.
        step = -step;
    }

    return true;
}

void RenderTextTrackCue::placeBoxInDefaultPosition(LayoutUnit position, bool& switched)
{
    // 8. Move all boxes in boxes ...
    if (m_cue->getWritingDirection() == TextTrackCue::Horizontal)
        // Horizontal: ... down by the distance given by position
        setY(y() + position);
    else
        // Vertical: ... right by the distance given by position
        setX(x() + position);

    // 9. Default: Remember the position of all the boxes in boxes as their
    // default position.
    m_fallbackPosition = FloatPoint(x(), y());

    // 10. Let switched be false.
    switched = false;
}

bool RenderTextTrackCue::isOutside() const
{
    return !rectIsWithinContainer(absoluteContentBox());
}

bool RenderTextTrackCue::rectIsWithinContainer(const IntRect& rect) const
{
    return containingBlock()->absoluteBoundingBoxRect().contains(rect);
}


bool RenderTextTrackCue::isOverlapping() const
{
    return overlappingObject();
}

RenderObject* RenderTextTrackCue::overlappingObject() const
{
    return overlappingObjectForRect(absoluteBoundingBoxRect());
}

RenderObject* RenderTextTrackCue::overlappingObjectForRect(const IntRect& rect) const
{
    for (RenderObject* box = previousSibling(); box; box = box->previousSibling()) {
        IntRect boxRect = box->absoluteBoundingBoxRect();

        if (rect.intersects(boxRect))
            return box;
    }

    return 0;
}

bool RenderTextTrackCue::shouldSwitchDirection(InlineFlowBox* firstLineBox, LayoutUnit step) const
{
    LayoutUnit top = y();
    LayoutUnit left = x();
    LayoutUnit bottom = top + firstLineBox->height();
    LayoutUnit right = left + firstLineBox->width();

    // 12. Horizontal: If step is negative and the top of the first line
    // box in boxes is now above the top of the video's rendering area,
    // or if step is positive and the bottom of the first line box in
    // boxes is now below the bottom of the video's rendering area, jump
    // to the step labeled switch direction.
    LayoutUnit parentHeight = containingBlock()->height();
    if (m_cue->getWritingDirection() == TextTrackCue::Horizontal && ((step < 0 && top < 0) || (step > 0 && bottom > parentHeight)))
        return true;

    // 12. Vertical: If step is negative and the left edge of the first line
    // box in boxes is now to the left of the left edge of the video's
    // rendering area, or if step is positive and the right edge of the
    // first line box in boxes is now to the right of the right edge of
    // the video's rendering area, jump to the step labeled switch direction.
    LayoutUnit parentWidth = containingBlock()->width();
    if (m_cue->getWritingDirection() != TextTrackCue::Horizontal && ((step < 0 && left < 0) || (step > 0 && right > parentWidth)))
        return true;

    return false;
}

void RenderTextTrackCue::moveBoxesByStep(LayoutUnit step)
{
    // 13. Horizontal: Move all the boxes in boxes down by the distance
    // given by step. (If step is negative, then this will actually
    // result in an upwards movement of the boxes in absolute terms.)
    if (m_cue->getWritingDirection() == TextTrackCue::Horizontal)
        setY(y() + step);

    // 13. Vertical: Move all the boxes in boxes right by the distance
    // given by step. (If step is negative, then this will actually
    // result in a leftwards movement of the boxes in absolute terms.)
    else
        setX(x() + step);
}

bool RenderTextTrackCue::switchDirection(bool& switched, LayoutUnit& step)
{
    // 15. Switch direction: Move all the boxes in boxes back to their
    // default position as determined in the step above labeled default.
    setX(m_fallbackPosition.x());
    setY(m_fallbackPosition.y());

    // 16. If switched is true, jump to the step labeled done
    // positioning below.
    if (switched)
        return false;

    // 17. Negate step.
    step = -step;

    // 18. Set switched to true.
    switched = true;
    return true;
}

void RenderTextTrackCue::moveIfNecessaryToKeepWithinContainer()
{
    IntRect containerRect = containingBlock()->absoluteBoundingBoxRect();
    IntRect cueRect = absoluteBoundingBoxRect();

    int topOverflow = cueRect.y() - containerRect.y();
    int bottomOverflow = containerRect.maxY() - cueRect.maxY();

    int verticalAdjustment = 0;
    if (topOverflow < 0)
        verticalAdjustment = -topOverflow;
    else if (bottomOverflow < 0)
        verticalAdjustment = bottomOverflow;

    if (verticalAdjustment)
        setY(y() + verticalAdjustment);

    int leftOverflow = cueRect.x() - containerRect.x();
    int rightOverflow = containerRect.maxX() - cueRect.maxX();

    int horizontalAdjustment = 0;
    if (leftOverflow < 0)
        horizontalAdjustment = -leftOverflow;
    else if (rightOverflow < 0)
        horizontalAdjustment = rightOverflow;

    if (horizontalAdjustment)
        setX(x() + horizontalAdjustment);
}

bool RenderTextTrackCue::findNonOverlappingPosition(int& newX, int& newY) const
{
    newX = x();
    newY = y();
    IntRect srcRect = absoluteBoundingBoxRect();
    IntRect destRect = srcRect;

    // Move the box up, looking for a non-overlapping position:
    while (RenderObject* box = overlappingObjectForRect(destRect)) {
        if (m_cue->getWritingDirection() == TextTrackCue::Horizontal)
            destRect.setY(box->absoluteBoundingBoxRect().y() - destRect.height());
        else
            destRect.setX(box->absoluteBoundingBoxRect().x() - destRect.width());
    }

    if (rectIsWithinContainer(destRect)) {
        newX += destRect.x() - srcRect.x();
        newY += destRect.y() - srcRect.y();
        return true;
    }

    destRect = srcRect;

    // Move the box down, looking for a non-overlapping position:
    while (RenderObject* box = overlappingObjectForRect(destRect)) {
        if (m_cue->getWritingDirection() == TextTrackCue::Horizontal)
            destRect.setY(box->absoluteBoundingBoxRect().maxY());
        else
            destRect.setX(box->absoluteBoundingBoxRect().maxX());
    }

    if (rectIsWithinContainer(destRect)) {
        newX += destRect.x() - srcRect.x();
        newY += destRect.y() - srcRect.y();
        return true;
    }

    return false;
}

void RenderTextTrackCue::repositionCueSnapToLinesSet()
{
    InlineFlowBox* firstLineBox;
    LayoutUnit step;
    LayoutUnit position;
    if (!initializeLayoutParameters(firstLineBox, step, position))
        return;

    bool switched;
    placeBoxInDefaultPosition(position, switched);

    // 11. Step loop: If none of the boxes in boxes would overlap any of the boxes
    // in output and all the boxes in output are within the video's rendering area
    // then jump to the step labeled done positioning.
    while (isOutside() || isOverlapping()) {
        if (!shouldSwitchDirection(firstLineBox, step))
            // 13. Move all the boxes in boxes ...
            // 14. Jump back to the step labeled step loop.
            moveBoxesByStep(step);
        else if (!switchDirection(switched, step))
            break;

        // 19. Jump back to the step labeled step loop.
    }

    // Acommodate extra top and bottom padding, border or margin.
    // Note: this is supported only for internal UA styling, not through the cue selector.
    if (hasInlineDirectionBordersPaddingOrMargin())
        moveIfNecessaryToKeepWithinContainer();
}

void RenderTextTrackCue::repositionGenericCue()
{
    ASSERT(firstChild());
    InlineFlowBox* firstLineBox = toRenderInline(firstChild())->firstLineBox();
    if (static_cast<TextTrackCueGeneric*>(m_cue)->useDefaultPosition() && firstLineBox) {
        LayoutUnit parentWidth = containingBlock()->logicalWidth();
        LayoutUnit width = firstLineBox->width();
        LayoutUnit right = (parentWidth / 2) - (width / 2);
        setX(right);
    }
    repositionCueSnapToLinesNotSet();
}

void RenderTextTrackCue::repositionCueSnapToLinesNotSet()
{
    // 3. If none of the boxes in boxes would overlap any of the boxes in output, and all the boxes in
    // output are within the video's rendering area, then jump to the step labeled done positioning below.
    if (!isOutside() && !isOverlapping())
        return;

    // 4. If there is a position to which the boxes in boxes can be moved while maintaining the relative
    // positions of the boxes in boxes to each other such that none of the boxes in boxes would overlap
    // any of the boxes in output, and all the boxes in output would be within the video's rendering area,
    // then move the boxes in boxes to the closest such position to their current position, and then jump
    // to the step labeled done positioning below. If there are multiple such positions that are equidistant
    // from their current position, use the highest one amongst them; if there are several at that height,
    // then use the leftmost one amongst them.
    moveIfNecessaryToKeepWithinContainer();
    int x = 0;
    int y = 0;
    if (!findNonOverlappingPosition(x, y))
        return;

    setX(x);
    setY(y);
}

} // namespace WebCore

#endif
