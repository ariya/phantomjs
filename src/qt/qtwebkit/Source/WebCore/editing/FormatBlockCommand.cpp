/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
#include "Element.h"
#include "FormatBlockCommand.h"
#include "Document.h"
#include "ExceptionCodePlaceholder.h"
#include "htmlediting.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "Range.h"
#include "VisibleUnits.h"

namespace WebCore {

using namespace HTMLNames;

static Node* enclosingBlockToSplitTreeTo(Node* startNode);
static bool isElementForFormatBlock(const QualifiedName& tagName);
static inline bool isElementForFormatBlock(Node* node)
{
    return node->isElementNode() && isElementForFormatBlock(toElement(node)->tagQName());
}

FormatBlockCommand::FormatBlockCommand(Document* document, const QualifiedName& tagName) 
    : ApplyBlockElementCommand(document, tagName)
    , m_didApply(false)
{
}

void FormatBlockCommand::formatSelection(const VisiblePosition& startOfSelection, const VisiblePosition& endOfSelection)
{
    if (!isElementForFormatBlock(tagName()))
        return;
    ApplyBlockElementCommand::formatSelection(startOfSelection, endOfSelection);
    m_didApply = true;
}

void FormatBlockCommand::formatRange(const Position& start, const Position& end, const Position& endOfSelection, RefPtr<Element>& blockNode)
{
    Node* nodeToSplitTo = enclosingBlockToSplitTreeTo(start.deprecatedNode());
    RefPtr<Node> outerBlock = (start.deprecatedNode() == nodeToSplitTo) ? start.deprecatedNode() : splitTreeToNode(start.deprecatedNode(), nodeToSplitTo);
    RefPtr<Node> nodeAfterInsertionPosition = outerBlock;

    RefPtr<Range> range = Range::create(document(), start, endOfSelection);
    Element* refNode = enclosingBlockFlowElement(end);
    Element* root = editableRootForPosition(start);
    // Root is null for elements with contenteditable=false.
    if (!root || !refNode)
        return;
    if (isElementForFormatBlock(refNode->tagQName()) && start == startOfBlock(start)
        && (end == endOfBlock(end) || isNodeVisiblyContainedWithin(refNode, range.get()))
        && refNode != root && !root->isDescendantOf(refNode)) {
        // Already in a block element that only contains the current paragraph
        if (refNode->hasTagName(tagName()))
            return;
        nodeAfterInsertionPosition = refNode;
    }

    if (!blockNode) {
        // Create a new blockquote and insert it as a child of the root editable element. We accomplish
        // this by splitting all parents of the current paragraph up to that point.
        blockNode = createBlockElement();
        insertNodeBefore(blockNode, nodeAfterInsertionPosition);
    }

    Position lastParagraphInBlockNode = blockNode->lastChild() ? positionAfterNode(blockNode->lastChild()) : Position();
    bool wasEndOfParagraph = isEndOfParagraph(lastParagraphInBlockNode);

    moveParagraphWithClones(start, end, blockNode.get(), outerBlock.get());

    if (wasEndOfParagraph && !isEndOfParagraph(lastParagraphInBlockNode) && !isStartOfParagraph(lastParagraphInBlockNode))
        insertBlockPlaceholder(lastParagraphInBlockNode);
}
    
Element* FormatBlockCommand::elementForFormatBlockCommand(Range* range)
{
    if (!range)
        return 0;

    Node* commonAncestor = range->commonAncestorContainer(IGNORE_EXCEPTION);
    while (commonAncestor && !isElementForFormatBlock(commonAncestor))
        commonAncestor = commonAncestor->parentNode();

    if (!commonAncestor)
        return 0;

    Element* rootEditableElement = range->startContainer()->rootEditableElement();
    if (!rootEditableElement || commonAncestor->contains(rootEditableElement))
        return 0;

    return commonAncestor->isElementNode() ? toElement(commonAncestor) : 0;
}

bool isElementForFormatBlock(const QualifiedName& tagName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, blockTags, ());
    if (blockTags.isEmpty()) {
        blockTags.add(addressTag);
        blockTags.add(articleTag);
        blockTags.add(asideTag);
        blockTags.add(blockquoteTag);
        blockTags.add(ddTag);
        blockTags.add(divTag);
        blockTags.add(dlTag);
        blockTags.add(dtTag);
        blockTags.add(footerTag);
        blockTags.add(h1Tag);
        blockTags.add(h2Tag);
        blockTags.add(h3Tag);
        blockTags.add(h4Tag);
        blockTags.add(h5Tag);
        blockTags.add(h6Tag);
        blockTags.add(headerTag);
        blockTags.add(hgroupTag);
        blockTags.add(mainTag);
        blockTags.add(navTag);
        blockTags.add(pTag);
        blockTags.add(preTag);
        blockTags.add(sectionTag);
    }
    return blockTags.contains(tagName);
}

Node* enclosingBlockToSplitTreeTo(Node* startNode)
{
    Node* lastBlock = startNode;
    for (Node* n = startNode; n; n = n->parentNode()) {
        if (!n->rendererIsEditable())
            return lastBlock;
        if (isTableCell(n) || n->hasTagName(bodyTag) || !n->parentNode() || !n->parentNode()->rendererIsEditable() || isElementForFormatBlock(n))
            return n;
        if (isBlock(n))
            lastBlock = n;
        if (isListElement(n))
            return n->parentNode()->rendererIsEditable() ? n->parentNode() : n;
    }
    return lastBlock;
}

}
