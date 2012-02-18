/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ApplyBlockElementCommand.h"

#include "HTMLElement.h"
#include "HTMLNames.h"
#include "Text.h"
#include "TextIterator.h"
#include "VisiblePosition.h"
#include "htmlediting.h"
#include "visible_units.h"

namespace WebCore {

using namespace HTMLNames;

ApplyBlockElementCommand::ApplyBlockElementCommand(Document* document, const QualifiedName& tagName, const AtomicString& className, const AtomicString& inlineStyle)
    : CompositeEditCommand(document)
    , m_tagName(tagName)
    , m_className(className)
    , m_inlineStyle(inlineStyle)
{
}

ApplyBlockElementCommand::ApplyBlockElementCommand(Document* document, const QualifiedName& tagName)
    : CompositeEditCommand(document)
    , m_tagName(tagName)
{
}

void ApplyBlockElementCommand::doApply()
{
    if (!endingSelection().isNonOrphanedCaretOrRange())
        return;

    if (!endingSelection().rootEditableElement())
        return;

    VisiblePosition visibleEnd = endingSelection().visibleEnd();
    VisiblePosition visibleStart = endingSelection().visibleStart();
    // When a selection ends at the start of a paragraph, we rarely paint 
    // the selection gap before that paragraph, because there often is no gap.  
    // In a case like this, it's not obvious to the user that the selection 
    // ends "inside" that paragraph, so it would be confusing if Indent/Outdent 
    // operated on that paragraph.
    // FIXME: We paint the gap before some paragraphs that are indented with left 
    // margin/padding, but not others.  We should make the gap painting more consistent and 
    // then use a left margin/padding rule here.
    if (visibleEnd != visibleStart && isStartOfParagraph(visibleEnd))
        setEndingSelection(VisibleSelection(visibleStart, visibleEnd.previous(CannotCrossEditingBoundary)));

    VisibleSelection selection = selectionForParagraphIteration(endingSelection());
    VisiblePosition startOfSelection = selection.visibleStart();
    VisiblePosition endOfSelection = selection.visibleEnd();
    ASSERT(!startOfSelection.isNull());
    ASSERT(!endOfSelection.isNull());
    int startIndex = indexForVisiblePosition(startOfSelection);
    int endIndex = indexForVisiblePosition(endOfSelection);

    formatSelection(startOfSelection, endOfSelection);

    updateLayout();

    RefPtr<Range> startRange = TextIterator::rangeFromLocationAndLength(document()->documentElement(), startIndex, 0, true);
    RefPtr<Range> endRange = TextIterator::rangeFromLocationAndLength(document()->documentElement(), endIndex, 0, true);
    if (startRange && endRange)
        setEndingSelection(VisibleSelection(startRange->startPosition(), endRange->startPosition(), DOWNSTREAM));
}

void ApplyBlockElementCommand::formatSelection(const VisiblePosition& startOfSelection, const VisiblePosition& endOfSelection)
{
    // Special case empty unsplittable elements because there's nothing to split
    // and there's nothing to move.
    Position start = startOfSelection.deepEquivalent().downstream();
    if (isAtUnsplittableElement(start)) {
        RefPtr<Element> blockquote = createBlockElement();
        insertNodeAt(blockquote, start);
        RefPtr<Element> placeholder = createBreakElement(document());
        appendNode(placeholder, blockquote);
        setEndingSelection(VisibleSelection(positionBeforeNode(placeholder.get()), DOWNSTREAM));
        return;
    }

    RefPtr<Element> blockquoteForNextIndent;
    VisiblePosition endOfCurrentParagraph = endOfParagraph(startOfSelection);
    VisiblePosition endAfterSelection = endOfParagraph(endOfParagraph(endOfSelection).next());
    m_endOfLastParagraph = endOfParagraph(endOfSelection).deepEquivalent();

    bool atEnd = false;
    Position end;
    while (endOfCurrentParagraph != endAfterSelection && !atEnd) {
        if (endOfCurrentParagraph.deepEquivalent() == m_endOfLastParagraph)
            atEnd = true;

        rangeForParagraphSplittingTextNodesIfNeeded(endOfCurrentParagraph, start, end);
        endOfCurrentParagraph = end;

        Position afterEnd = end.next();
        Node* enclosingCell = enclosingNodeOfType(start, &isTableCell);
        VisiblePosition endOfNextParagraph = endOfNextParagrahSplittingTextNodesIfNeeded(endOfCurrentParagraph, start, end);

        formatRange(start, end, m_endOfLastParagraph, blockquoteForNextIndent);

        // Don't put the next paragraph in the blockquote we just created for this paragraph unless 
        // the next paragraph is in the same cell.
        if (enclosingCell && enclosingCell != enclosingNodeOfType(endOfNextParagraph.deepEquivalent(), &isTableCell))
            blockquoteForNextIndent = 0;

        // indentIntoBlockquote could move more than one paragraph if the paragraph
        // is in a list item or a table. As a result, endAfterSelection could refer to a position
        // no longer in the document.
        if (endAfterSelection.isNotNull() && !endAfterSelection.deepEquivalent().anchorNode()->inDocument())
            break;
        // Sanity check: Make sure our moveParagraph calls didn't remove endOfNextParagraph.deepEquivalent().deprecatedNode()
        // If somehow we did, return to prevent crashes.
        if (endOfNextParagraph.isNotNull() && !endOfNextParagraph.deepEquivalent().anchorNode()->inDocument()) {
            ASSERT_NOT_REACHED();
            return;
        }
        endOfCurrentParagraph = endOfNextParagraph;
    }
}

static bool isNewLineAtPosition(const Position& position)
{
    if (position.anchorType() != Position::PositionIsOffsetInAnchor)
        return false;

    Node* textNode = position.containerNode();
    int offset = position.offsetInContainerNode();
    if (!textNode || !textNode->isTextNode() || offset < 0 || offset >= textNode->maxCharacterOffset())
        return false;

    ExceptionCode ec = 0;
    String textAtPosition = static_cast<Text*>(textNode)->substringData(offset, 1, ec);
    if (ec)
        return false;

    return textAtPosition[0] == '\n';
}

static RenderStyle* renderStyleOfEnclosingTextNode(const Position& position)
{
    if (position.anchorType() != Position::PositionIsOffsetInAnchor
        || !position.containerNode()
        || !position.containerNode()->isTextNode()
        || !position.containerNode()->renderer())
        return 0;
    return position.containerNode()->renderer()->style();
}

void ApplyBlockElementCommand::rangeForParagraphSplittingTextNodesIfNeeded(const VisiblePosition& endOfCurrentParagraph, Position& start, Position& end)
{
    start = startOfParagraph(endOfCurrentParagraph).deepEquivalent();
    end = endOfCurrentParagraph.deepEquivalent();

    RenderStyle* startStyle = renderStyleOfEnclosingTextNode(start);
    bool isStartAndEndOnSameNode = false;
    if (startStyle) {
        isStartAndEndOnSameNode = renderStyleOfEnclosingTextNode(end) && start.deprecatedNode() == end.deprecatedNode();
        bool isStartAndEndOfLastParagraphOnSameNode = renderStyleOfEnclosingTextNode(m_endOfLastParagraph) && start.deprecatedNode() == m_endOfLastParagraph.deprecatedNode();

        // Avoid obtanining the start of next paragraph for start
        if (startStyle->preserveNewline() && isNewLineAtPosition(start) && !isNewLineAtPosition(start.previous()) && start.offsetInContainerNode() > 0)
            start = startOfParagraph(end.previous()).deepEquivalent();

        // If start is in the middle of a text node, split.
        if (!startStyle->collapseWhiteSpace() && start.offsetInContainerNode() > 0) {
            int startOffset = start.offsetInContainerNode();
            splitTextNode(static_cast<Text*>(start.deprecatedNode()), startOffset);
            start = firstPositionInOrBeforeNode(start.deprecatedNode());
            if (isStartAndEndOnSameNode) {
                ASSERT(end.offsetInContainerNode() >= startOffset);
                end = Position(end.deprecatedNode(), end.offsetInContainerNode() - startOffset, Position::PositionIsOffsetInAnchor);
            }
            if (isStartAndEndOfLastParagraphOnSameNode) {
                ASSERT(m_endOfLastParagraph.offsetInContainerNode() >= startOffset);
                m_endOfLastParagraph = Position(m_endOfLastParagraph.deprecatedNode(), m_endOfLastParagraph.offsetInContainerNode() - startOffset,
                    Position::PositionIsOffsetInAnchor);
            }
        }
    }

    RenderStyle* endStyle = renderStyleOfEnclosingTextNode(end);
    if (endStyle) {
        bool isEndAndEndOfLastParagraphOnSameNode = renderStyleOfEnclosingTextNode(m_endOfLastParagraph) && end.deprecatedNode() == m_endOfLastParagraph.deprecatedNode();
        // Include \n at the end of line if we're at an empty paragraph
        if (endStyle->preserveNewline() && start == end
            && end.offsetInContainerNode() < end.containerNode()->maxCharacterOffset()) {
            int endOffset = end.offsetInContainerNode();
            if (!isNewLineAtPosition(end.previous()) && isNewLineAtPosition(end))
                end = Position(end.deprecatedNode(), endOffset + 1, Position::PositionIsOffsetInAnchor);
            if (isEndAndEndOfLastParagraphOnSameNode && end.offsetInContainerNode() >= m_endOfLastParagraph.offsetInContainerNode())
                m_endOfLastParagraph = end;
        }

        // If end is in the middle of a text node, split.
        if (!endStyle->collapseWhiteSpace() && end.offsetInContainerNode()
            && end.offsetInContainerNode() < end.containerNode()->maxCharacterOffset()) {
            splitTextNode(static_cast<Text*>(end.deprecatedNode()), end.offsetInContainerNode());
            if (isStartAndEndOnSameNode)
                start = firstPositionInOrBeforeNode(end.deprecatedNode()->previousSibling());
            if (isEndAndEndOfLastParagraphOnSameNode) {
                if (m_endOfLastParagraph.offsetInContainerNode() == end.offsetInContainerNode())
                    m_endOfLastParagraph = lastPositionInNode(end.deprecatedNode()->previousSibling());
                else
                    m_endOfLastParagraph = Position(end.deprecatedNode(), m_endOfLastParagraph.offsetInContainerNode() - end.offsetInContainerNode(),
                                                    Position::PositionIsOffsetInAnchor);
            }
            end = lastPositionInNode(end.deprecatedNode()->previousSibling());
        }
    }
}

VisiblePosition ApplyBlockElementCommand::endOfNextParagrahSplittingTextNodesIfNeeded(VisiblePosition& endOfCurrentParagraph, Position& start, Position& end)
{
    VisiblePosition endOfNextParagraph = endOfParagraph(endOfCurrentParagraph.next());
    Position position = endOfNextParagraph.deepEquivalent();
    RenderStyle* style = renderStyleOfEnclosingTextNode(position);
    if (!style)
        return endOfNextParagraph;

    RefPtr<Node> containerNode = position.containerNode();
    if (!style->preserveNewline() || !position.offsetInContainerNode()
        || !isNewLineAtPosition(Position(containerNode.get(), 0, Position::PositionIsOffsetInAnchor)))
        return endOfNextParagraph;

    // \n at the beginning of the text node immediately following the current paragraph is trimmed by moveParagraphWithClones.
    // If endOfNextParagraph was pointing at this same text node, endOfNextParagraph will be shifted by one paragraph.
    // Avoid this by splitting "\n"
    splitTextNode(static_cast<Text*>(containerNode.get()), 1);

    if (start.anchorType() == Position::PositionIsOffsetInAnchor && containerNode.get() == start.containerNode()) {
        ASSERT(start.offsetInContainerNode() < position.offsetInContainerNode());
        start = Position(containerNode->previousSibling(), start.offsetInContainerNode(), Position::PositionIsOffsetInAnchor);
    }
    if (end.anchorType() == Position::PositionIsOffsetInAnchor && containerNode.get() == end.containerNode()) {
        ASSERT(end.offsetInContainerNode() < position.offsetInContainerNode());
        end = Position(containerNode->previousSibling(), end.offsetInContainerNode(), Position::PositionIsOffsetInAnchor);
    }
    if (m_endOfLastParagraph.anchorType() == Position::PositionIsOffsetInAnchor && containerNode.get() == m_endOfLastParagraph.containerNode()) {
        if (m_endOfLastParagraph.offsetInContainerNode() < position.offsetInContainerNode())
            m_endOfLastParagraph = Position(containerNode->previousSibling(), m_endOfLastParagraph.offsetInContainerNode(), Position::PositionIsOffsetInAnchor);
        else
            m_endOfLastParagraph = Position(containerNode, m_endOfLastParagraph.offsetInContainerNode() - 1, Position::PositionIsOffsetInAnchor);
    }

    return Position(containerNode.get(), position.offsetInContainerNode() - 1, Position::PositionIsOffsetInAnchor);
}

PassRefPtr<Element> ApplyBlockElementCommand::createBlockElement() const
{
    RefPtr<Element> element = createHTMLElement(document(), m_tagName);
    if (m_className.length())
        element->setAttribute(classAttr, m_className);
    if (m_inlineStyle.length())
        element->setAttribute(styleAttr, m_inlineStyle);
    return element.release();
}

}
