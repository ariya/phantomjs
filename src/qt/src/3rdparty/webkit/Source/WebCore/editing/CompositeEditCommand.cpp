/*
 * Copyright (C) 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#include "CompositeEditCommand.h"

#include "AppendNodeCommand.h"
#include "ApplyStyleCommand.h"
#include "DeleteFromTextNodeCommand.h"
#include "DeleteSelectionCommand.h"
#include "Document.h"
#include "DocumentFragment.h"
#include "EditorInsertAction.h"
#include "Frame.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "InlineTextBox.h"
#include "InsertIntoTextNodeCommand.h"
#include "InsertLineBreakCommand.h"
#include "InsertNodeBeforeCommand.h"
#include "InsertParagraphSeparatorCommand.h"
#include "InsertTextCommand.h"
#include "JoinTextNodesCommand.h"
#include "MergeIdenticalElementsCommand.h"
#include "Range.h"
#include "RemoveCSSPropertyCommand.h"
#include "RemoveNodeCommand.h"
#include "RemoveNodePreservingChildrenCommand.h"
#include "ReplaceNodeWithSpanCommand.h"
#include "ReplaceSelectionCommand.h"
#include "RenderBlock.h"
#include "RenderText.h"
#include "SetNodeAttributeCommand.h"
#include "SplitElementCommand.h"
#include "SplitTextNodeCommand.h"
#include "SplitTextNodeContainingElementCommand.h"
#include "Text.h"
#include "TextIterator.h"
#include "WrapContentsInDummySpanCommand.h"
#include "htmlediting.h"
#include "markup.h"
#include "visible_units.h"
#include <wtf/unicode/CharacterNames.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;

CompositeEditCommand::CompositeEditCommand(Document *document)
    : EditCommand(document)
{
}

CompositeEditCommand::~CompositeEditCommand()
{
}

void CompositeEditCommand::doUnapply()
{
    size_t size = m_commands.size();
    for (size_t i = size; i != 0; --i)
        m_commands[i - 1]->unapply();
}

void CompositeEditCommand::doReapply()
{
    size_t size = m_commands.size();
    for (size_t i = 0; i != size; ++i)
        m_commands[i]->reapply();
}

//
// sugary-sweet convenience functions to help create and apply edit commands in composite commands
//
void CompositeEditCommand::applyCommandToComposite(PassRefPtr<EditCommand> cmd)
{
    cmd->setParent(this);
    cmd->apply();
    m_commands.append(cmd);
}

void CompositeEditCommand::applyStyle(const EditingStyle* style, EditAction editingAction)
{
    applyCommandToComposite(ApplyStyleCommand::create(document(), style, editingAction));
}

void CompositeEditCommand::applyStyle(const EditingStyle* style, const Position& start, const Position& end, EditAction editingAction)
{
    applyCommandToComposite(ApplyStyleCommand::create(document(), style, start, end, editingAction));
}

void CompositeEditCommand::applyStyledElement(PassRefPtr<Element> element)
{
    applyCommandToComposite(ApplyStyleCommand::create(element, false));
}

void CompositeEditCommand::removeStyledElement(PassRefPtr<Element> element)
{
    applyCommandToComposite(ApplyStyleCommand::create(element, true));
}

void CompositeEditCommand::insertParagraphSeparator(bool useDefaultParagraphElement)
{
    applyCommandToComposite(InsertParagraphSeparatorCommand::create(document(), useDefaultParagraphElement));
}

void CompositeEditCommand::insertLineBreak()
{
    applyCommandToComposite(InsertLineBreakCommand::create(document()));
}

void CompositeEditCommand::insertNodeBefore(PassRefPtr<Node> insertChild, PassRefPtr<Node> refChild)
{
    ASSERT(!refChild->hasTagName(bodyTag));
    applyCommandToComposite(InsertNodeBeforeCommand::create(insertChild, refChild));
}

void CompositeEditCommand::insertNodeAfter(PassRefPtr<Node> insertChild, PassRefPtr<Node> refChild)
{
    ASSERT(insertChild);
    ASSERT(refChild);
    ASSERT(!refChild->hasTagName(bodyTag));
    ContainerNode* parent = refChild->parentNode();
    ASSERT(parent);
    if (parent->lastChild() == refChild)
        appendNode(insertChild, parent);
    else {
        ASSERT(refChild->nextSibling());
        insertNodeBefore(insertChild, refChild->nextSibling());
    }
}

void CompositeEditCommand::insertNodeAt(PassRefPtr<Node> insertChild, const Position& editingPosition)
{
    ASSERT(isEditablePosition(editingPosition));
    // For editing positions like [table, 0], insert before the table,
    // likewise for replaced elements, brs, etc.
    Position p = editingPosition.parentAnchoredEquivalent();
    Node* refChild = p.deprecatedNode();
    int offset = p.deprecatedEditingOffset();
    
    if (canHaveChildrenForEditing(refChild)) {
        Node* child = refChild->firstChild();
        for (int i = 0; child && i < offset; i++)
            child = child->nextSibling();
        if (child)
            insertNodeBefore(insertChild, child);
        else
            appendNode(insertChild, static_cast<Element*>(refChild));
    } else if (caretMinOffset(refChild) >= offset)
        insertNodeBefore(insertChild, refChild);
    else if (refChild->isTextNode() && caretMaxOffset(refChild) > offset) {
        splitTextNode(static_cast<Text *>(refChild), offset);

        // Mutation events (bug 22634) from the text node insertion may have removed the refChild
        if (!refChild->inDocument())
            return;
        insertNodeBefore(insertChild, refChild);
    } else
        insertNodeAfter(insertChild, refChild);
}

void CompositeEditCommand::appendNode(PassRefPtr<Node> node, PassRefPtr<ContainerNode> parent)
{
    ASSERT(canHaveChildrenForEditing(parent.get()));
    applyCommandToComposite(AppendNodeCommand::create(parent, node));
}

void CompositeEditCommand::removeChildrenInRange(PassRefPtr<Node> node, unsigned from, unsigned to)
{
    Vector<RefPtr<Node> > children;
    Node* child = node->childNode(from);
    for (unsigned i = from; child && i < to; i++, child = child->nextSibling())
        children.append(child);

    size_t size = children.size();
    for (size_t i = 0; i < size; ++i)
        removeNode(children[i].release());
}

void CompositeEditCommand::removeNode(PassRefPtr<Node> node)
{
    if (!node || !node->parentNode())
        return;
    applyCommandToComposite(RemoveNodeCommand::create(node));
}

void CompositeEditCommand::removeNodePreservingChildren(PassRefPtr<Node> node)
{
    applyCommandToComposite(RemoveNodePreservingChildrenCommand::create(node));
}

void CompositeEditCommand::removeNodeAndPruneAncestors(PassRefPtr<Node> node)
{
    RefPtr<ContainerNode> parent = node->parentNode();
    removeNode(node);
    prune(parent.release());
}

HTMLElement* CompositeEditCommand::replaceElementWithSpanPreservingChildrenAndAttributes(PassRefPtr<HTMLElement> node)
{
    // It would also be possible to implement all of ReplaceNodeWithSpanCommand
    // as a series of existing smaller edit commands.  Someone who wanted to
    // reduce the number of edit commands could do so here.
    RefPtr<ReplaceNodeWithSpanCommand> command = ReplaceNodeWithSpanCommand::create(node);
    applyCommandToComposite(command);
    // Returning a raw pointer here is OK because the command is retained by
    // applyCommandToComposite (thus retaining the span), and the span is also
    // in the DOM tree, and thus alive whie it has a parent.
    ASSERT(command->spanElement()->inDocument());
    return command->spanElement();
}

static bool hasARenderedDescendant(Node* node)
{
    Node* n = node->firstChild();
    while (n) {
        if (n->renderer())
            return true;
        n = n->traverseNextNode(node);
    }
    return false;
}

void CompositeEditCommand::prune(PassRefPtr<Node> node)
{
    while (node) {
        // If you change this rule you may have to add an updateLayout() here.
        RenderObject* renderer = node->renderer();
        if (renderer && (!renderer->canHaveChildren() || hasARenderedDescendant(node.get()) || node->rootEditableElement() == node))
            return;
            
        RefPtr<ContainerNode> next = node->parentNode();
        removeNode(node);
        node = next;
    }
}

void CompositeEditCommand::splitTextNode(PassRefPtr<Text> node, unsigned offset)
{
    applyCommandToComposite(SplitTextNodeCommand::create(node, offset));
}

void CompositeEditCommand::splitElement(PassRefPtr<Element> element, PassRefPtr<Node> atChild)
{
    applyCommandToComposite(SplitElementCommand::create(element, atChild));
}

void CompositeEditCommand::mergeIdenticalElements(PassRefPtr<Element> prpFirst, PassRefPtr<Element> prpSecond)
{
    RefPtr<Element> first = prpFirst;
    RefPtr<Element> second = prpSecond;
    ASSERT(!first->isDescendantOf(second.get()) && second != first);
    if (first->nextSibling() != second) {
        removeNode(second);
        insertNodeAfter(second, first);
    }
    applyCommandToComposite(MergeIdenticalElementsCommand::create(first, second));
}

void CompositeEditCommand::wrapContentsInDummySpan(PassRefPtr<Element> element)
{
    applyCommandToComposite(WrapContentsInDummySpanCommand::create(element));
}

void CompositeEditCommand::splitTextNodeContainingElement(PassRefPtr<Text> text, unsigned offset)
{
    applyCommandToComposite(SplitTextNodeContainingElementCommand::create(text, offset));
}

void CompositeEditCommand::joinTextNodes(PassRefPtr<Text> text1, PassRefPtr<Text> text2)
{
    applyCommandToComposite(JoinTextNodesCommand::create(text1, text2));
}

void CompositeEditCommand::inputText(const String& text, bool selectInsertedText)
{
    unsigned offset = 0;
    unsigned length = text.length();
    RefPtr<Range> startRange = Range::create(document(), firstPositionInNode(document()->documentElement()), endingSelection().start());
    unsigned startIndex = TextIterator::rangeLength(startRange.get());
    size_t newline;
    do {
        newline = text.find('\n', offset);
        if (newline != offset) {
            RefPtr<InsertTextCommand> command = InsertTextCommand::create(document());
            applyCommandToComposite(command);
            int substringLength = newline == notFound ? length - offset : newline - offset;
            command->input(text.substring(offset, substringLength), false);
        }
        if (newline != notFound)
            insertLineBreak();
            
        offset = newline + 1;
    } while (newline != notFound && offset != length);
    
    if (selectInsertedText) {
        RefPtr<Range> selectedRange = TextIterator::rangeFromLocationAndLength(document()->documentElement(), startIndex, length);
        setEndingSelection(VisibleSelection(selectedRange.get()));
    }
}

void CompositeEditCommand::insertTextIntoNode(PassRefPtr<Text> node, unsigned offset, const String& text)
{
    applyCommandToComposite(InsertIntoTextNodeCommand::create(node, offset, text));
}

void CompositeEditCommand::deleteTextFromNode(PassRefPtr<Text> node, unsigned offset, unsigned count)
{
    applyCommandToComposite(DeleteFromTextNodeCommand::create(node, offset, count));
}

void CompositeEditCommand::replaceTextInNode(PassRefPtr<Text> node, unsigned offset, unsigned count, const String& replacementText)
{
    applyCommandToComposite(DeleteFromTextNodeCommand::create(node.get(), offset, count));
    applyCommandToComposite(InsertIntoTextNodeCommand::create(node, offset, replacementText));
}

Position CompositeEditCommand::positionOutsideTabSpan(const Position& pos)
{
    if (!isTabSpanTextNode(pos.anchorNode()))
        return pos;

    if (pos.anchorType() == Position::PositionIsAfterAnchor)
        return positionInParentAfterNode(pos.anchorNode());
    if (pos.anchorType() == Position::PositionIsBeforeAnchor)
        return positionInParentBeforeNode(pos.anchorNode());

    Node* tabSpan = tabSpanNode(pos.containerNode());

    if (pos.offsetInContainerNode() <= caretMinOffset(pos.containerNode()))
        return positionInParentBeforeNode(tabSpan);

    if (pos.offsetInContainerNode() >= caretMaxOffset(pos.containerNode()))
        return positionInParentAfterNode(tabSpan);

    splitTextNodeContainingElement(static_cast<Text *>(pos.containerNode()), pos.offsetInContainerNode());
    return positionInParentBeforeNode(tabSpan);
}

void CompositeEditCommand::insertNodeAtTabSpanPosition(PassRefPtr<Node> node, const Position& pos)
{
    // insert node before, after, or at split of tab span
    insertNodeAt(node, positionOutsideTabSpan(pos));
}

void CompositeEditCommand::deleteSelection(bool smartDelete, bool mergeBlocksAfterDelete, bool replace, bool expandForSpecialElements)
{
    if (endingSelection().isRange())
        applyCommandToComposite(DeleteSelectionCommand::create(document(), smartDelete, mergeBlocksAfterDelete, replace, expandForSpecialElements));
}

void CompositeEditCommand::deleteSelection(const VisibleSelection &selection, bool smartDelete, bool mergeBlocksAfterDelete, bool replace, bool expandForSpecialElements)
{
    if (selection.isRange())
        applyCommandToComposite(DeleteSelectionCommand::create(selection, smartDelete, mergeBlocksAfterDelete, replace, expandForSpecialElements));
}

void CompositeEditCommand::removeCSSProperty(PassRefPtr<StyledElement> element, CSSPropertyID property)
{
    applyCommandToComposite(RemoveCSSPropertyCommand::create(document(), element, property));
}

void CompositeEditCommand::removeNodeAttribute(PassRefPtr<Element> element, const QualifiedName& attribute)
{
    setNodeAttribute(element, attribute, AtomicString());
}

void CompositeEditCommand::setNodeAttribute(PassRefPtr<Element> element, const QualifiedName& attribute, const AtomicString& value)
{
    applyCommandToComposite(SetNodeAttributeCommand::create(element, attribute, value));
}

static inline bool containsOnlyWhitespace(const String& text)
{
    for (unsigned i = 0; i < text.length(); ++i) {
        if (!isWhitespace(text.characters()[i]))
            return false;
    }
    
    return true;
}

bool CompositeEditCommand::shouldRebalanceLeadingWhitespaceFor(const String& text) const
{
    return containsOnlyWhitespace(text);
}

bool CompositeEditCommand::canRebalance(const Position& position) const
{
    Node* node = position.containerNode();
    if (position.anchorType() != Position::PositionIsOffsetInAnchor || !node || !node->isTextNode())
        return false;

    Text* textNode = static_cast<Text*>(node);
    if (textNode->length() == 0)
        return false;

    RenderObject* renderer = textNode->renderer();
    if (renderer && !renderer->style()->collapseWhiteSpace())
        return false;

    return true;
}

// FIXME: Doesn't go into text nodes that contribute adjacent text (siblings, cousins, etc).
void CompositeEditCommand::rebalanceWhitespaceAt(const Position& position)
{
    Node* node = position.containerNode();
    if (!canRebalance(position))
        return;

    // If the rebalance is for the single offset, and neither text[offset] nor text[offset - 1] are some form of whitespace, do nothing.
    int offset = position.deprecatedEditingOffset();
    String text = static_cast<Text*>(node)->data();
    if (!isWhitespace(text[offset])) {
        offset--;
        if (offset < 0 || !isWhitespace(text[offset]))
            return;
    }

    rebalanceWhitespaceOnTextSubstring(static_cast<Text*>(node), position.offsetInContainerNode(), position.offsetInContainerNode());
}

void CompositeEditCommand::rebalanceWhitespaceOnTextSubstring(RefPtr<Text> textNode, int startOffset, int endOffset)
{
    String text = textNode->data();
    ASSERT(!text.isEmpty());

    // Set upstream and downstream to define the extent of the whitespace surrounding text[offset].
    int upstream = startOffset;
    while (upstream > 0 && isWhitespace(text[upstream - 1]))
        upstream--;
    
    int downstream = endOffset;
    while ((unsigned)downstream < text.length() && isWhitespace(text[downstream]))
        downstream++;
    
    int length = downstream - upstream;
    if (!length)
        return;

    VisiblePosition visibleUpstreamPos(Position(textNode, upstream, Position::PositionIsOffsetInAnchor));
    VisiblePosition visibleDownstreamPos(Position(textNode, downstream, Position::PositionIsOffsetInAnchor));
    
    String string = text.substring(upstream, length);
    String rebalancedString = stringWithRebalancedWhitespace(string,
    // FIXME: Because of the problem mentioned at the top of this function, we must also use nbsps at the start/end of the string because
    // this function doesn't get all surrounding whitespace, just the whitespace in the current text node.
                                                             isStartOfParagraph(visibleUpstreamPos) || upstream == 0, 
                                                             isEndOfParagraph(visibleDownstreamPos) || (unsigned)downstream == text.length());
    
    if (string != rebalancedString)
        replaceTextInNode(textNode, upstream, length, rebalancedString);
}

void CompositeEditCommand::prepareWhitespaceAtPositionForSplit(Position& position)
{
    Node* node = position.deprecatedNode();
    if (!node || !node->isTextNode())
        return;
    Text* textNode = static_cast<Text*>(node);    
    
    if (textNode->length() == 0)
        return;
    RenderObject* renderer = textNode->renderer();
    if (renderer && !renderer->style()->collapseWhiteSpace())
        return;

    // Delete collapsed whitespace so that inserting nbsps doesn't uncollapse it.
    Position upstreamPos = position.upstream();
    deleteInsignificantText(position.upstream(), position.downstream());
    position = upstreamPos.downstream();

    VisiblePosition visiblePos(position);
    VisiblePosition previousVisiblePos(visiblePos.previous());
    Position previous(previousVisiblePos.deepEquivalent());
    
    if (isCollapsibleWhitespace(previousVisiblePos.characterAfter()) && previous.deprecatedNode()->isTextNode() && !previous.deprecatedNode()->hasTagName(brTag))
        replaceTextInNode(static_cast<Text*>(previous.deprecatedNode()), previous.deprecatedEditingOffset(), 1, nonBreakingSpaceString());
    if (isCollapsibleWhitespace(visiblePos.characterAfter()) && position.deprecatedNode()->isTextNode() && !position.deprecatedNode()->hasTagName(brTag))
        replaceTextInNode(static_cast<Text*>(position.deprecatedNode()), position.deprecatedEditingOffset(), 1, nonBreakingSpaceString());
}

void CompositeEditCommand::rebalanceWhitespace()
{
    VisibleSelection selection = endingSelection();
    if (selection.isNone())
        return;
        
    rebalanceWhitespaceAt(selection.start());
    if (selection.isRange())
        rebalanceWhitespaceAt(selection.end());
}

void CompositeEditCommand::deleteInsignificantText(PassRefPtr<Text> textNode, unsigned start, unsigned end)
{
    if (!textNode || start >= end)
        return;

    RenderText* textRenderer = toRenderText(textNode->renderer());
    if (!textRenderer)
        return;

    Vector<InlineTextBox*> sortedTextBoxes;
    size_t sortedTextBoxesPosition = 0;
   
    for (InlineTextBox* textBox = textRenderer->firstTextBox(); textBox; textBox = textBox->nextTextBox())
        sortedTextBoxes.append(textBox);
    
    // If there is mixed directionality text, the boxes can be out of order,
    // (like Arabic with embedded LTR), so sort them first. 
    if (textRenderer->containsReversedText())    
        std::sort(sortedTextBoxes.begin(), sortedTextBoxes.end(), InlineTextBox::compareByStart);
    InlineTextBox* box = sortedTextBoxes.isEmpty() ? 0 : sortedTextBoxes[sortedTextBoxesPosition];

    if (!box) {
        // whole text node is empty
        removeNode(textNode);
        return;    
    }
    
    unsigned length = textNode->length();
    if (start >= length || end > length)
        return;

    unsigned removed = 0;
    InlineTextBox* prevBox = 0;
    String str;

    // This loop structure works to process all gaps preceding a box,
    // and also will look at the gap after the last box.
    while (prevBox || box) {
        unsigned gapStart = prevBox ? prevBox->start() + prevBox->len() : 0;
        if (end < gapStart)
            // No more chance for any intersections
            break;

        unsigned gapEnd = box ? box->start() : length;
        bool indicesIntersect = start <= gapEnd && end >= gapStart;
        int gapLen = gapEnd - gapStart;
        if (indicesIntersect && gapLen > 0) {
            gapStart = max(gapStart, start);
            gapEnd = min(gapEnd, end);
            if (str.isNull())
                str = textNode->data().substring(start, end - start);
            // remove text in the gap
            str.remove(gapStart - start - removed, gapLen);
            removed += gapLen;
        }
        
        prevBox = box;
        if (box) {
            if (++sortedTextBoxesPosition < sortedTextBoxes.size())
                box = sortedTextBoxes[sortedTextBoxesPosition];
            else
                box = 0;
        }
    }

    if (!str.isNull()) {
        // Replace the text between start and end with our pruned version.
        if (!str.isEmpty())
            replaceTextInNode(textNode, start, end - start, str);
        else {
            // Assert that we are not going to delete all of the text in the node.
            // If we were, that should have been done above with the call to 
            // removeNode and return.
            ASSERT(start > 0 || end - start < textNode->length());
            deleteTextFromNode(textNode, start, end - start);
        }
    }
}

void CompositeEditCommand::deleteInsignificantText(const Position& start, const Position& end)
{
    if (start.isNull() || end.isNull())
        return;

    if (comparePositions(start, end) >= 0)
        return;

    Node* next;
    for (Node* node = start.deprecatedNode(); node; node = next) {
        next = node->traverseNextNode();
        if (node->isTextNode()) {
            Text* textNode = static_cast<Text*>(node);
            int startOffset = node == start.deprecatedNode() ? start.deprecatedEditingOffset() : 0;
            int endOffset = node == end.deprecatedNode() ? end.deprecatedEditingOffset() : static_cast<int>(textNode->length());
            deleteInsignificantText(textNode, startOffset, endOffset);
        }
        if (node == end.deprecatedNode())
            break;
    }
}

void CompositeEditCommand::deleteInsignificantTextDownstream(const Position& pos)
{
    Position end = VisiblePosition(pos, VP_DEFAULT_AFFINITY).next().deepEquivalent().downstream();
    deleteInsignificantText(pos, end);
}

PassRefPtr<Node> CompositeEditCommand::appendBlockPlaceholder(PassRefPtr<Element> container)
{
    if (!container)
        return 0;
    
    // Should assert isBlockFlow || isInlineFlow when deletion improves. See 4244964.
    ASSERT(container->renderer());

    RefPtr<Node> placeholder = createBlockPlaceholderElement(document());
    appendNode(placeholder, container);
    return placeholder.release();
}

PassRefPtr<Node> CompositeEditCommand::insertBlockPlaceholder(const Position& pos)
{
    if (pos.isNull())
        return 0;

    // Should assert isBlockFlow || isInlineFlow when deletion improves.  See 4244964.
    ASSERT(pos.deprecatedNode()->renderer());

    RefPtr<Node> placeholder = createBlockPlaceholderElement(document());
    insertNodeAt(placeholder, pos);
    return placeholder.release();
}

PassRefPtr<Node> CompositeEditCommand::addBlockPlaceholderIfNeeded(Element* container)
{
    if (!container)
        return 0;

    updateLayout();

    RenderObject* renderer = container->renderer();
    if (!renderer || !renderer->isBlockFlow())
        return 0;
    
    // append the placeholder to make sure it follows
    // any unrendered blocks
    RenderBlock* block = toRenderBlock(renderer);
    if (block->height() == 0 || (block->isListItem() && block->isEmpty()))
        return appendBlockPlaceholder(container);

    return 0;
}

// Assumes that the position is at a placeholder and does the removal without much checking.
void CompositeEditCommand::removePlaceholderAt(const Position& p)
{
    ASSERT(lineBreakExistsAtPosition(p));
    
    // We are certain that the position is at a line break, but it may be a br or a preserved newline.
    if (p.anchorNode()->hasTagName(brTag)) {
        removeNode(p.anchorNode());
        return;
    }
    
    deleteTextFromNode(static_cast<Text*>(p.anchorNode()), p.offsetInContainerNode(), 1);
}

PassRefPtr<Node> CompositeEditCommand::insertNewDefaultParagraphElementAt(const Position& position)
{
    RefPtr<Element> paragraphElement = createDefaultParagraphElement(document());
    ExceptionCode ec;
    paragraphElement->appendChild(createBreakElement(document()), ec);
    insertNodeAt(paragraphElement, position);
    return paragraphElement.release();
}

// If the paragraph is not entirely within it's own block, create one and move the paragraph into 
// it, and return that block.  Otherwise return 0.
PassRefPtr<Node> CompositeEditCommand::moveParagraphContentsToNewBlockIfNecessary(const Position& pos)
{
    if (pos.isNull())
        return 0;
    
    updateLayout();
    
    // It's strange that this function is responsible for verifying that pos has not been invalidated
    // by an earlier call to this function.  The caller, applyBlockStyle, should do this.
    VisiblePosition visiblePos(pos, VP_DEFAULT_AFFINITY);
    VisiblePosition visibleParagraphStart(startOfParagraph(visiblePos));
    VisiblePosition visibleParagraphEnd = endOfParagraph(visiblePos);
    VisiblePosition next = visibleParagraphEnd.next();
    VisiblePosition visibleEnd = next.isNotNull() ? next : visibleParagraphEnd;
    
    Position upstreamStart = visibleParagraphStart.deepEquivalent().upstream();
    Position upstreamEnd = visibleEnd.deepEquivalent().upstream();

    // If there are no VisiblePositions in the same block as pos then 
    // upstreamStart will be outside the paragraph
    if (comparePositions(pos, upstreamStart) < 0)
        return 0;

    // Perform some checks to see if we need to perform work in this function.
    if (isBlock(upstreamStart.deprecatedNode())) {
        // If the block is the root editable element, always move content to a new block,
        // since it is illegal to modify attributes on the root editable element for editing.
        if (upstreamStart.deprecatedNode() == editableRootForPosition(upstreamStart)) {
            // If the block is the root editable element and it contains no visible content, create a new
            // block but don't try and move content into it, since there's nothing for moveParagraphs to move.
            if (!Position::hasRenderedNonAnonymousDescendantsWithHeight(upstreamStart.deprecatedNode()->renderer()))
                return insertNewDefaultParagraphElementAt(upstreamStart);
        } else if (isBlock(upstreamEnd.deprecatedNode())) {
            if (!upstreamEnd.deprecatedNode()->isDescendantOf(upstreamStart.deprecatedNode())) {
                // If the paragraph end is a descendant of paragraph start, then we need to run
                // the rest of this function. If not, we can bail here.
                return 0;
            }
        } else if (enclosingBlock(upstreamEnd.deprecatedNode()) != upstreamStart.deprecatedNode()) {
            // The visibleEnd.  It must be an ancestor of the paragraph start.
            // We can bail as we have a full block to work with.
            ASSERT(upstreamStart.deprecatedNode()->isDescendantOf(enclosingBlock(upstreamEnd.deprecatedNode())));
            return 0;
        } else if (isEndOfDocument(visibleEnd)) {
            // At the end of the document. We can bail here as well.
            return 0;
        }
    }

    RefPtr<Node> newBlock = insertNewDefaultParagraphElementAt(upstreamStart);

    bool endWasBr = visibleParagraphEnd.deepEquivalent().deprecatedNode()->hasTagName(brTag);

    moveParagraphs(visibleParagraphStart, visibleParagraphEnd, VisiblePosition(firstPositionInNode(newBlock.get())));

    if (newBlock->lastChild() && newBlock->lastChild()->hasTagName(brTag) && !endWasBr)
        removeNode(newBlock->lastChild());

    return newBlock.release();
}

void CompositeEditCommand::pushAnchorElementDown(Node* anchorNode)
{
    if (!anchorNode)
        return;
    
    ASSERT(anchorNode->isLink());
    
    setEndingSelection(VisibleSelection::selectionFromContentsOfNode(anchorNode));
    applyStyledElement(static_cast<Element*>(anchorNode));
    // Clones of anchorNode have been pushed down, now remove it.
    if (anchorNode->inDocument())
        removeNodePreservingChildren(anchorNode);
}

// Clone the paragraph between start and end under blockElement,
// preserving the hierarchy up to outerNode. 

void CompositeEditCommand::cloneParagraphUnderNewElement(Position& start, Position& end, Node* outerNode, Element* blockElement)
{
    // First we clone the outerNode
    
    RefPtr<Node> topNode = outerNode->cloneNode(isTableElement(outerNode));
    appendNode(topNode, blockElement);
    RefPtr<Node> lastNode = topNode;

    if (start.deprecatedNode() != outerNode) {
        Vector<RefPtr<Node> > ancestors;
        
        // Insert each node from innerNode to outerNode (excluded) in a list.
        for (Node* n = start.deprecatedNode(); n && n != outerNode; n = n->parentNode())
            ancestors.append(n);

        // Clone every node between start.deprecatedNode() and outerBlock.

        for (size_t i = ancestors.size(); i != 0; --i) {
            Node* item = ancestors[i - 1].get();
            RefPtr<Node> child = item->cloneNode(isTableElement(item));
            appendNode(child, static_cast<Element *>(lastNode.get()));
            lastNode = child.release();
        }
    }

    // Handle the case of paragraphs with more than one node,
    // cloning all the siblings until end.deprecatedNode() is reached.
    
    if (start.deprecatedNode() != end.deprecatedNode() && !start.deprecatedNode()->isDescendantOf(end.deprecatedNode())) {
        // If end is not a descendant of outerNode we need to
        // find the first common ancestor and adjust the insertion
        // point accordingly.
        while (!end.deprecatedNode()->isDescendantOf(outerNode)) {
            outerNode = outerNode->parentNode();
            topNode = topNode->parentNode();
        }

        for (Node* n = start.deprecatedNode()->traverseNextSibling(outerNode); n; n = n->traverseNextSibling(outerNode)) {
            if (n->parentNode() != start.deprecatedNode()->parentNode())
                lastNode = topNode->lastChild();

            RefPtr<Node> clonedNode = n->cloneNode(true);
            insertNodeAfter(clonedNode, lastNode);
            lastNode = clonedNode.release();
            if (n == end.deprecatedNode() || end.deprecatedNode()->isDescendantOf(n))
                break;
        }
    }
}

    
// There are bugs in deletion when it removes a fully selected table/list.
// It expands and removes the entire table/list, but will let content
// before and after the table/list collapse onto one line.   
// Deleting a paragraph will leave a placeholder. Remove it (and prune
// empty or unrendered parents).

void CompositeEditCommand::cleanupAfterDeletion(VisiblePosition destination)
{
    VisiblePosition caretAfterDelete = endingSelection().visibleStart();
    if (caretAfterDelete != destination && isStartOfParagraph(caretAfterDelete) && isEndOfParagraph(caretAfterDelete)) {
        // Note: We want the rightmost candidate.
        Position position = caretAfterDelete.deepEquivalent().downstream();
        Node* node = position.deprecatedNode();
        // Normally deletion will leave a br as a placeholder.
        if (node->hasTagName(brTag))
            removeNodeAndPruneAncestors(node);
        // If the selection to move was empty and in an empty block that 
        // doesn't require a placeholder to prop itself open (like a bordered
        // div or an li), remove it during the move (the list removal code
        // expects this behavior).
        else if (isBlock(node))
            removeNodeAndPruneAncestors(node);
        else if (lineBreakExistsAtPosition(position)) {
            // There is a preserved '\n' at caretAfterDelete.
            // We can safely assume this is a text node.
            Text* textNode = static_cast<Text*>(node);
            if (textNode->length() == 1)
                removeNodeAndPruneAncestors(node);
            else
                deleteTextFromNode(textNode, position.deprecatedEditingOffset(), 1);
        }
    }
}
    
// This is a version of moveParagraph that preserves style by keeping the original markup
// It is currently used only by IndentOutdentCommand but it is meant to be used in the
// future by several other commands such as InsertList and the align commands.
// The blockElement parameter is the element to move the paragraph to,
// outerNode is the top element of the paragraph hierarchy. 

void CompositeEditCommand::moveParagraphWithClones(const VisiblePosition& startOfParagraphToMove, const VisiblePosition& endOfParagraphToMove, Element* blockElement, Node* outerNode)
{
    ASSERT(outerNode);
    ASSERT(blockElement);

    VisiblePosition beforeParagraph = startOfParagraphToMove.previous();
    VisiblePosition afterParagraph(endOfParagraphToMove.next());
    
    // We upstream() the end and downstream() the start so that we don't include collapsed whitespace in the move.
    // When we paste a fragment, spaces after the end and before the start are treated as though they were rendered.
    Position start = startOfParagraphToMove.deepEquivalent().downstream();
    Position end = endOfParagraphToMove.deepEquivalent().upstream();

    cloneParagraphUnderNewElement(start, end, outerNode, blockElement);
      
    setEndingSelection(VisibleSelection(start, end, DOWNSTREAM));
    deleteSelection(false, false, false, false);
    
    // There are bugs in deletion when it removes a fully selected table/list.
    // It expands and removes the entire table/list, but will let content
    // before and after the table/list collapse onto one line.
       
    cleanupAfterDeletion();
    
    // Add a br if pruning an empty block level element caused a collapse.  For example:
    // foo^
    // <div>bar</div>
    // baz
    // Imagine moving 'bar' to ^.  'bar' will be deleted and its div pruned.  That would
    // cause 'baz' to collapse onto the line with 'foobar' unless we insert a br.
    // Must recononicalize these two VisiblePositions after the pruning above.
    beforeParagraph = VisiblePosition(beforeParagraph.deepEquivalent());
    afterParagraph = VisiblePosition(afterParagraph.deepEquivalent());

    if (beforeParagraph.isNotNull() && !isTableElement(beforeParagraph.deepEquivalent().deprecatedNode())
        && ((!isEndOfParagraph(beforeParagraph) && !isStartOfParagraph(beforeParagraph)) || beforeParagraph == afterParagraph)) {
        // FIXME: Trim text between beforeParagraph and afterParagraph if they aren't equal.
        insertNodeAt(createBreakElement(document()), beforeParagraph.deepEquivalent());
    }
}
    
    
// This moves a paragraph preserving its style.
void CompositeEditCommand::moveParagraph(const VisiblePosition& startOfParagraphToMove, const VisiblePosition& endOfParagraphToMove, const VisiblePosition& destination, bool preserveSelection, bool preserveStyle)
{
    ASSERT(isStartOfParagraph(startOfParagraphToMove));
    ASSERT(isEndOfParagraph(endOfParagraphToMove));
    moveParagraphs(startOfParagraphToMove, endOfParagraphToMove, destination, preserveSelection, preserveStyle);
}

void CompositeEditCommand::moveParagraphs(const VisiblePosition& startOfParagraphToMove, const VisiblePosition& endOfParagraphToMove, const VisiblePosition& destination, bool preserveSelection, bool preserveStyle)
{
    if (startOfParagraphToMove == destination)
        return;
    
    int startIndex = -1;
    int endIndex = -1;
    int destinationIndex = -1;
    if (preserveSelection && !endingSelection().isNone()) {
        VisiblePosition visibleStart = endingSelection().visibleStart();
        VisiblePosition visibleEnd = endingSelection().visibleEnd();
        
        bool startAfterParagraph = comparePositions(visibleStart, endOfParagraphToMove) > 0;
        bool endBeforeParagraph = comparePositions(visibleEnd, startOfParagraphToMove) < 0;
        
        if (!startAfterParagraph && !endBeforeParagraph) {
            bool startInParagraph = comparePositions(visibleStart, startOfParagraphToMove) >= 0;
            bool endInParagraph = comparePositions(visibleEnd, endOfParagraphToMove) <= 0;
            
            startIndex = 0;
            if (startInParagraph) {
                RefPtr<Range> startRange = Range::create(document(), startOfParagraphToMove.deepEquivalent().parentAnchoredEquivalent(), visibleStart.deepEquivalent().parentAnchoredEquivalent());
                startIndex = TextIterator::rangeLength(startRange.get(), true);
            }

            endIndex = 0;
            if (endInParagraph) {
                RefPtr<Range> endRange = Range::create(document(), startOfParagraphToMove.deepEquivalent().parentAnchoredEquivalent(), visibleEnd.deepEquivalent().parentAnchoredEquivalent());
                endIndex = TextIterator::rangeLength(endRange.get(), true);
            }
        }
    }
    
    VisiblePosition beforeParagraph = startOfParagraphToMove.previous(CannotCrossEditingBoundary);
    VisiblePosition afterParagraph(endOfParagraphToMove.next(CannotCrossEditingBoundary));

    // We upstream() the end and downstream() the start so that we don't include collapsed whitespace in the move.
    // When we paste a fragment, spaces after the end and before the start are treated as though they were rendered.
    Position start = startOfParagraphToMove.deepEquivalent().downstream();
    Position end = endOfParagraphToMove.deepEquivalent().upstream();
    
    // start and end can't be used directly to create a Range; they are "editing positions"
    Position startRangeCompliant = start.parentAnchoredEquivalent();
    Position endRangeCompliant = end.parentAnchoredEquivalent();
    RefPtr<Range> range = Range::create(document(), startRangeCompliant.deprecatedNode(), startRangeCompliant.deprecatedEditingOffset(), endRangeCompliant.deprecatedNode(), endRangeCompliant.deprecatedEditingOffset());

    // FIXME: This is an inefficient way to preserve style on nodes in the paragraph to move. It
    // shouldn't matter though, since moved paragraphs will usually be quite small.
    RefPtr<DocumentFragment> fragment;
    // This used to use a ternary for initialization, but that confused some versions of GCC, see bug 37912
    if (startOfParagraphToMove != endOfParagraphToMove)
        fragment = createFragmentFromMarkup(document(), createMarkup(range.get(), 0, DoNotAnnotateForInterchange, true), "");

    // A non-empty paragraph's style is moved when we copy and move it.  We don't move 
    // anything if we're given an empty paragraph, but an empty paragraph can have style
    // too, <div><b><br></b></div> for example.  Save it so that we can preserve it later.
    RefPtr<EditingStyle> styleInEmptyParagraph;
    if (startOfParagraphToMove == endOfParagraphToMove && preserveStyle) {
        styleInEmptyParagraph = EditingStyle::create(startOfParagraphToMove.deepEquivalent());
        styleInEmptyParagraph->mergeTypingStyle(document());
        // The moved paragraph should assume the block style of the destination.
        styleInEmptyParagraph->removeBlockProperties();
    }
    
    // FIXME (5098931): We should add a new insert action "WebViewInsertActionMoved" and call shouldInsertFragment here.
    
    setEndingSelection(VisibleSelection(start, end, DOWNSTREAM));
    document()->frame()->editor()->clearMisspellingsAndBadGrammar(endingSelection());
    deleteSelection(false, false, false, false);

    ASSERT(destination.deepEquivalent().anchorNode()->inDocument());
    cleanupAfterDeletion(destination);
    ASSERT(destination.deepEquivalent().anchorNode()->inDocument());

    // Add a br if pruning an empty block level element caused a collapse. For example:
    // foo^
    // <div>bar</div>
    // baz
    // Imagine moving 'bar' to ^. 'bar' will be deleted and its div pruned. That would
    // cause 'baz' to collapse onto the line with 'foobar' unless we insert a br.
    // Must recononicalize these two VisiblePositions after the pruning above.
    beforeParagraph = VisiblePosition(beforeParagraph.deepEquivalent());
    afterParagraph = VisiblePosition(afterParagraph.deepEquivalent());
    if (beforeParagraph.isNotNull() && (!isEndOfParagraph(beforeParagraph) || beforeParagraph == afterParagraph)) {
        // FIXME: Trim text between beforeParagraph and afterParagraph if they aren't equal.
        insertNodeAt(createBreakElement(document()), beforeParagraph.deepEquivalent());
        // Need an updateLayout here in case inserting the br has split a text node.
        updateLayout();
    }

    RefPtr<Range> startToDestinationRange(Range::create(document(), firstPositionInNode(document()->documentElement()), destination.deepEquivalent().parentAnchoredEquivalent()));
    destinationIndex = TextIterator::rangeLength(startToDestinationRange.get(), true);

    setEndingSelection(destination);
    ASSERT(endingSelection().isCaretOrRange());
    ReplaceSelectionCommand::CommandOptions options = ReplaceSelectionCommand::SelectReplacement | ReplaceSelectionCommand::MovingParagraph;
    if (!preserveStyle)
        options |= ReplaceSelectionCommand::MatchStyle;
    applyCommandToComposite(ReplaceSelectionCommand::create(document(), fragment, options));

    document()->frame()->editor()->markMisspellingsAndBadGrammar(endingSelection());

    // If the selection is in an empty paragraph, restore styles from the old empty paragraph to the new empty paragraph.
    bool selectionIsEmptyParagraph = endingSelection().isCaret() && isStartOfParagraph(endingSelection().visibleStart()) && isEndOfParagraph(endingSelection().visibleStart());
    if (styleInEmptyParagraph && selectionIsEmptyParagraph)
        applyStyle(styleInEmptyParagraph.get());

    if (preserveSelection && startIndex != -1) {
        // Fragment creation (using createMarkup) incorrectly uses regular
        // spaces instead of nbsps for some spaces that were rendered (11475), which
        // causes spaces to be collapsed during the move operation.  This results
        // in a call to rangeFromLocationAndLength with a location past the end
        // of the document (which will return null).
        RefPtr<Range> start = TextIterator::rangeFromLocationAndLength(document()->documentElement(), destinationIndex + startIndex, 0, true);
        RefPtr<Range> end = TextIterator::rangeFromLocationAndLength(document()->documentElement(), destinationIndex + endIndex, 0, true);
        if (start && end)
            setEndingSelection(VisibleSelection(start->startPosition(), end->startPosition(), DOWNSTREAM));
    }
}

// FIXME: Send an appropriate shouldDeleteRange call.
bool CompositeEditCommand::breakOutOfEmptyListItem()
{
    Node* emptyListItem = enclosingEmptyListItem(endingSelection().visibleStart());
    if (!emptyListItem)
        return false;

    RefPtr<EditingStyle> style = EditingStyle::create(endingSelection().start());
    style->mergeTypingStyle(document());

    ContainerNode* listNode = emptyListItem->parentNode();
    // FIXME: Can't we do something better when the immediate parent wasn't a list node?
    if (!listNode
        || (!listNode->hasTagName(ulTag) && !listNode->hasTagName(olTag))
        || !listNode->rendererIsEditable()
        || listNode == emptyListItem->rootEditableElement())
        return false;

    RefPtr<Element> newBlock = 0;
    if (ContainerNode* blockEnclosingList = listNode->parentNode()) {
        if (blockEnclosingList->hasTagName(liTag)) { // listNode is inside another list item
            if (visiblePositionAfterNode(blockEnclosingList) == visiblePositionAfterNode(listNode)) {
                // If listNode appears at the end of the outer list item, then move listNode outside of this list item
                // e.g. <ul><li>hello <ul><li><br></li></ul> </li></ul> should become <ul><li>hello</li> <ul><li><br></li></ul> </ul> after this section
                // If listNode does NOT appear at the end, then we should consider it as a regular paragraph.
                // e.g. <ul><li> <ul><li><br></li></ul> hello</li></ul> should become <ul><li> <div><br></div> hello</li></ul> at the end
                splitElement(static_cast<Element*>(blockEnclosingList), listNode);
                removeNodePreservingChildren(listNode->parentNode());
                newBlock = createListItemElement(document());
            }
            // If listNode does NOT appear at the end of the outer list item, then behave as if in a regular paragraph.
        } else if (blockEnclosingList->hasTagName(olTag) || blockEnclosingList->hasTagName(ulTag))
            newBlock = createListItemElement(document());
    }
    if (!newBlock)
        newBlock = createDefaultParagraphElement(document());

    if (emptyListItem->renderer()->nextSibling()) {
        // If emptyListItem follows another list item, split the list node.
        if (emptyListItem->renderer()->previousSibling())
            splitElement(static_cast<Element*>(listNode), emptyListItem);

        // If emptyListItem is followed by other list item, then insert newBlock before the list node.
        // Because we have splitted the element, emptyListItem is the first element in the list node.
        // i.e. insert newBlock before ul or ol whose first element is emptyListItem
        insertNodeBefore(newBlock, listNode);
        removeNode(emptyListItem);
    } else {
        // When emptyListItem does not follow any list item, insert newBlock after the enclosing list node.
        // Remove the enclosing node if emptyListItem is the only child; otherwise just remove emptyListItem.
        insertNodeAfter(newBlock, listNode);
        removeNode(emptyListItem->renderer()->previousSibling() ? emptyListItem : listNode);
    }

    appendBlockPlaceholder(newBlock);
    setEndingSelection(VisibleSelection(firstPositionInNode(newBlock.get()), DOWNSTREAM));

    style->prepareToApplyAt(endingSelection().start());
    if (!style->isEmpty())
        applyStyle(style.get());

    return true;
}

// If the caret is in an empty quoted paragraph, and either there is nothing before that
// paragraph, or what is before is unquoted, and the user presses delete, unquote that paragraph.
bool CompositeEditCommand::breakOutOfEmptyMailBlockquotedParagraph()
{
    if (!endingSelection().isCaret())
        return false;
        
    VisiblePosition caret(endingSelection().visibleStart());
    Node* highestBlockquote = highestEnclosingNodeOfType(caret.deepEquivalent(), &isMailBlockquote);
    if (!highestBlockquote)
        return false;
        
    if (!isStartOfParagraph(caret) || !isEndOfParagraph(caret))
        return false;
    
    VisiblePosition previous(caret.previous(CannotCrossEditingBoundary));
    // Only move forward if there's nothing before the caret, or if there's unquoted content before it.
    if (enclosingNodeOfType(previous.deepEquivalent(), &isMailBlockquote))
        return false;
    
    RefPtr<Node> br = createBreakElement(document());
    // We want to replace this quoted paragraph with an unquoted one, so insert a br
    // to hold the caret before the highest blockquote.
    insertNodeBefore(br, highestBlockquote);
    VisiblePosition atBR(positionBeforeNode(br.get()));
    // If the br we inserted collapsed, for example foo<br><blockquote>...</blockquote>, insert
    // a second one.
    if (!isStartOfParagraph(atBR))
        insertNodeBefore(createBreakElement(document()), br);
    setEndingSelection(VisibleSelection(atBR));
    
    // If this is an empty paragraph there must be a line break here.
    if (!lineBreakExistsAtVisiblePosition(caret))
        return false;

    Position caretPos(caret.deepEquivalent().downstream());
    // A line break is either a br or a preserved newline.
    ASSERT(caretPos.deprecatedNode()->hasTagName(brTag) || (caretPos.deprecatedNode()->isTextNode() && caretPos.deprecatedNode()->renderer()->style()->preserveNewline()));
    
    if (caretPos.deprecatedNode()->hasTagName(brTag)) {
        Position beforeBR(positionInParentBeforeNode(caretPos.deprecatedNode()));
        removeNode(caretPos.deprecatedNode());
        prune(beforeBR.deprecatedNode());
    } else if (caretPos.deprecatedNode()->isTextNode()) {
        ASSERT(caretPos.deprecatedEditingOffset() == 0);
        Text* textNode = static_cast<Text*>(caretPos.deprecatedNode());
        ContainerNode* parentNode = textNode->parentNode();
        // The preserved newline must be the first thing in the node, since otherwise the previous
        // paragraph would be quoted, and we verified that it wasn't above.
        deleteTextFromNode(textNode, 0, 1);
        prune(parentNode);
    }

    return true;
}

// Operations use this function to avoid inserting content into an anchor when at the start or the end of
// that anchor, as in NSTextView.
// FIXME: This is only an approximation of NSTextViews insertion behavior, which varies depending on how
// the caret was made. 
Position CompositeEditCommand::positionAvoidingSpecialElementBoundary(const Position& original)
{
    if (original.isNull())
        return original;
        
    VisiblePosition visiblePos(original);
    Node* enclosingAnchor = enclosingAnchorElement(original);
    Position result = original;

    if (!enclosingAnchor)
        return result;

    // Don't avoid block level anchors, because that would insert content into the wrong paragraph.
    if (enclosingAnchor && !isBlock(enclosingAnchor)) {
        VisiblePosition firstInAnchor(firstPositionInNode(enclosingAnchor));
        VisiblePosition lastInAnchor(lastPositionInNode(enclosingAnchor));
        // If visually just after the anchor, insert *inside* the anchor unless it's the last
        // VisiblePosition in the document, to match NSTextView.
        if (visiblePos == lastInAnchor) {
            // Make sure anchors are pushed down before avoiding them so that we don't
            // also avoid structural elements like lists and blocks (5142012).
            if (original.deprecatedNode() != enclosingAnchor && original.deprecatedNode()->parentNode() != enclosingAnchor) {
                pushAnchorElementDown(enclosingAnchor);
                enclosingAnchor = enclosingAnchorElement(original);
                if (!enclosingAnchor)
                    return original;
            }
            // Don't insert outside an anchor if doing so would skip over a line break.  It would
            // probably be safe to move the line break so that we could still avoid the anchor here.
            Position downstream(visiblePos.deepEquivalent().downstream());
            if (lineBreakExistsAtVisiblePosition(visiblePos) && downstream.deprecatedNode()->isDescendantOf(enclosingAnchor))
                return original;
            
            result = positionInParentAfterNode(enclosingAnchor);
        }
        // If visually just before an anchor, insert *outside* the anchor unless it's the first
        // VisiblePosition in a paragraph, to match NSTextView.
        if (visiblePos == firstInAnchor) {
            // Make sure anchors are pushed down before avoiding them so that we don't
            // also avoid structural elements like lists and blocks (5142012).
            if (original.deprecatedNode() != enclosingAnchor && original.deprecatedNode()->parentNode() != enclosingAnchor) {
                pushAnchorElementDown(enclosingAnchor);
                enclosingAnchor = enclosingAnchorElement(original);
            }
            if (!enclosingAnchor)
                return original;

            result = positionInParentBeforeNode(enclosingAnchor);
        }
    }
        
    if (result.isNull() || !editableRootForPosition(result))
        result = original;
    
    return result;
}

// Splits the tree parent by parent until we reach the specified ancestor. We use VisiblePositions
// to determine if the split is necessary. Returns the last split node.
PassRefPtr<Node> CompositeEditCommand::splitTreeToNode(Node* start, Node* end, bool shouldSplitAncestor)
{
    ASSERT(start);
    ASSERT(end);
    ASSERT(start != end);

    RefPtr<Node> node;
    if (shouldSplitAncestor && end->parentNode())
        end = end->parentNode();

    RefPtr<Node> endNode = end;
    for (node = start; node && node->parentNode() != endNode; node = node->parentNode()) {
        if (!node->parentNode()->isElementNode())
            break;
        // Do not split a node when doing so introduces an empty node.
        VisiblePosition positionInParent = firstPositionInNode(node->parentNode());
        VisiblePosition positionInNode = firstPositionInOrBeforeNode(node.get());
        if (positionInParent != positionInNode)
            splitElement(static_cast<Element*>(node->parentNode()), node);
    }

    return node.release();
}

PassRefPtr<Element> createBlockPlaceholderElement(Document* document)
{
    RefPtr<Element> breakNode = document->createElement(brTag, false);
    return breakNode.release();
}

} // namespace WebCore
