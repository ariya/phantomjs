/*
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
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
#include "htmlediting.h"

#include "Document.h"
#include "EditingText.h"
#include "HTMLBRElement.h"
#include "HTMLDivElement.h"
#include "HTMLElementFactory.h"
#include "HTMLInterchange.h"
#include "HTMLLIElement.h"
#include "HTMLNames.h"
#include "HTMLObjectElement.h"
#include "HTMLOListElement.h"
#include "HTMLUListElement.h"
#include "PositionIterator.h"
#include "RenderObject.h"
#include "Range.h"
#include "VisibleSelection.h"
#include "Text.h"
#include "TextIterator.h"
#include "VisiblePosition.h"
#include "visible_units.h"
#include <wtf/StdLibExtras.h>
#include <wtf/unicode/CharacterNames.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;

// Atomic means that the node has no children, or has children which are ignored for the
// purposes of editing.
bool isAtomicNode(const Node *node)
{
    return node && (!node->hasChildNodes() || editingIgnoresContent(node));
}

// Returns true for nodes that either have no content, or have content that is ignored (skipped 
// over) while editing.  There are no VisiblePositions inside these nodes.
bool editingIgnoresContent(const Node* node)
{
    return !canHaveChildrenForEditing(node) && !node->isTextNode();
}

bool canHaveChildrenForEditing(const Node* node)
{
    return !node->isTextNode()
        && !node->hasTagName(brTag)
        && !node->hasTagName(imgTag)
        && !node->hasTagName(inputTag)
        && !node->hasTagName(textareaTag)
        && (!node->hasTagName(objectTag) || static_cast<const HTMLObjectElement*>(node)->useFallbackContent())
        && !node->hasTagName(iframeTag)
        && !node->hasTagName(embedTag)
        && !node->hasTagName(appletTag)
        && !node->hasTagName(selectTag)
        && (!node->hasTagName(hrTag) || node->hasChildNodes());
}

// Compare two positions, taking into account the possibility that one or both
// could be inside a shadow tree. Only works for non-null values.
int comparePositions(const Position& a, const Position& b)
{
    Node* nodeA = a.deprecatedNode();
    ASSERT(nodeA);
    Node* nodeB = b.deprecatedNode();
    ASSERT(nodeB);
    int offsetA = a.deprecatedEditingOffset();
    int offsetB = b.deprecatedEditingOffset();

    Node* shadowAncestorA = nodeA->shadowAncestorNode();
    if (shadowAncestorA == nodeA)
        shadowAncestorA = 0;
    Node* shadowAncestorB = nodeB->shadowAncestorNode();
    if (shadowAncestorB == nodeB)
        shadowAncestorB = 0;

    int bias = 0;
    if (shadowAncestorA != shadowAncestorB) {
        if (shadowAncestorA) {
            nodeA = shadowAncestorA;
            offsetA = 0;
            bias = 1;
        }
        if (shadowAncestorB) {
            nodeB = shadowAncestorB;
            offsetB = 0;
            bias = -1;
        }
    }

    ExceptionCode ec;
    int result = Range::compareBoundaryPoints(nodeA, offsetA, nodeB, offsetB, ec);
    return result ? result : bias;
}

int comparePositions(const VisiblePosition& a, const VisiblePosition& b)
{
    return comparePositions(a.deepEquivalent(), b.deepEquivalent());
}

Node* highestEditableRoot(const Position& position)
{
    Node* node = position.deprecatedNode();
    if (!node)
        return 0;
        
    Node* highestRoot = editableRootForPosition(position);
    if (!highestRoot)
        return 0;
    
    node = highestRoot;
    while (node) {
        if (node->rendererIsEditable())
            highestRoot = node;
        if (node->hasTagName(bodyTag))
            break;
        node = node->parentNode();
    }
    
    return highestRoot;
}

Node* lowestEditableAncestor(Node* node)
{
    if (!node)
        return 0;
    
    Node *lowestRoot = 0;
    while (node) {
        if (node->rendererIsEditable())
            return node->rootEditableElement();
        if (node->hasTagName(bodyTag))
            break;
        node = node->parentNode();
    }
    
    return lowestRoot;
}

bool isEditablePosition(const Position& p)
{
    Node* node = p.deprecatedNode();
    if (!node)
        return false;
        
    if (node->renderer() && node->renderer()->isTable())
        node = node->parentNode();
    
    return node->rendererIsEditable();
}

bool isAtUnsplittableElement(const Position& pos)
{
    Node* node = pos.deprecatedNode();
    return (node == editableRootForPosition(pos) || node == enclosingNodeOfType(pos, &isTableCell));
}
    
    
bool isRichlyEditablePosition(const Position& p)
{
    Node* node = p.deprecatedNode();
    if (!node)
        return false;
        
    if (node->renderer() && node->renderer()->isTable())
        node = node->parentNode();
    
    return node->rendererIsRichlyEditable();
}

Element* editableRootForPosition(const Position& p)
{
    Node* node = p.deprecatedNode();
    if (!node)
        return 0;
        
    if (node->renderer() && node->renderer()->isTable())
        node = node->parentNode();
    
    return node->rootEditableElement();
}

// Finds the enclosing element until which the tree can be split.
// When a user hits ENTER, he/she won't expect this element to be split into two.
// You may pass it as the second argument of splitTreeToNode.
Element* unsplittableElementForPosition(const Position& p)
{
    // Since enclosingNodeOfType won't search beyond the highest root editable node,
    // this code works even if the closest table cell was outside of the root editable node.
    Element* enclosingCell = static_cast<Element*>(enclosingNodeOfType(p, &isTableCell));
    if (enclosingCell)
        return enclosingCell;

    return editableRootForPosition(p);
}

Position nextCandidate(const Position& position)
{
    PositionIterator p = position;
    while (!p.atEnd()) {
        p.increment();
        if (p.isCandidate())
            return p;
    }
    return Position();
}

Position nextVisuallyDistinctCandidate(const Position& position)
{
    Position p = position;
    Position downstreamStart = p.downstream();
    while (!p.atEndOfTree()) {
        p = p.next(Character);
        if (p.isCandidate() && p.downstream() != downstreamStart)
            return p;
    }
    return Position();
}

Position previousCandidate(const Position& position)
{
    PositionIterator p = position;
    while (!p.atStart()) {
        p.decrement();
        if (p.isCandidate())
            return p;
    }
    return Position();
}

Position previousVisuallyDistinctCandidate(const Position& position)
{
    Position p = position;
    Position downstreamStart = p.downstream();
    while (!p.atStartOfTree()) {
        p = p.previous(Character);
        if (p.isCandidate() && p.downstream() != downstreamStart)
            return p;
    }
    return Position();
}

VisiblePosition firstEditablePositionAfterPositionInRoot(const Position& position, Node* highestRoot)
{
    // position falls before highestRoot.
    if (comparePositions(position, firstPositionInNode(highestRoot)) == -1 && highestRoot->rendererIsEditable())
        return firstPositionInNode(highestRoot);

    Position p = position;
    
    if (Node* shadowAncestor = p.deprecatedNode()->shadowAncestorNode())
        if (shadowAncestor != p.deprecatedNode())
            p = positionAfterNode(shadowAncestor);
    
    while (p.deprecatedNode() && !isEditablePosition(p) && p.deprecatedNode()->isDescendantOf(highestRoot))
        p = isAtomicNode(p.deprecatedNode()) ? positionInParentAfterNode(p.deprecatedNode()) : nextVisuallyDistinctCandidate(p);
    
    if (p.deprecatedNode() && p.deprecatedNode() != highestRoot && !p.deprecatedNode()->isDescendantOf(highestRoot))
        return VisiblePosition();
    
    return VisiblePosition(p);
}

VisiblePosition lastEditablePositionBeforePositionInRoot(const Position& position, Node* highestRoot)
{
    // When position falls after highestRoot, the result is easy to compute.
    if (comparePositions(position, lastPositionInNode(highestRoot)) == 1)
        return lastPositionInNode(highestRoot);

    Position p = position;

    if (Node* shadowAncestor = p.deprecatedNode()->shadowAncestorNode()) {
        if (shadowAncestor != p.deprecatedNode())
            p = firstPositionInOrBeforeNode(shadowAncestor);
    }
    
    while (p.deprecatedNode() && !isEditablePosition(p) && p.deprecatedNode()->isDescendantOf(highestRoot))
        p = isAtomicNode(p.deprecatedNode()) ? positionInParentBeforeNode(p.deprecatedNode()) : previousVisuallyDistinctCandidate(p);
    
    if (p.deprecatedNode() && p.deprecatedNode() != highestRoot && !p.deprecatedNode()->isDescendantOf(highestRoot))
        return VisiblePosition();
    
    return VisiblePosition(p);
}

// FIXME: The method name, comment, and code say three different things here!
// Whether or not content before and after this node will collapse onto the same line as it.
bool isBlock(const Node* node)
{
    return node && node->renderer() && !node->renderer()->isInline();
}

// FIXME: Deploy this in all of the places where enclosingBlockFlow/enclosingBlockFlowOrTableElement are used.
// FIXME: Pass a position to this function.  The enclosing block of [table, x] for example, should be the 
// block that contains the table and not the table, and this function should be the only one responsible for 
// knowing about these kinds of special cases.
Node* enclosingBlock(Node* node, EditingBoundaryCrossingRule rule)
{
    return static_cast<Element*>(enclosingNodeOfType(firstPositionInOrBeforeNode(node), isBlock, rule));
}

TextDirection directionOfEnclosingBlock(const Position& position)
{
    Node* enclosingBlockNode = enclosingBlock(position.containerNode());
    if (!enclosingBlockNode)
        return LTR;
    RenderObject* renderer = enclosingBlockNode->renderer();
    return renderer ? renderer->style()->direction() : LTR;
}

// This method is used to create positions in the DOM. It returns the maximum valid offset
// in a node.  It returns 1 for some elements even though they do not have children, which
// creates technically invalid DOM Positions.  Be sure to call parentAnchoredEquivalent
// on a Position before using it to create a DOM Range, or an exception will be thrown.
int lastOffsetForEditing(const Node* node)
{
    ASSERT(node);
    if (!node)
        return 0;
    if (node->offsetInCharacters())
        return node->maxCharacterOffset();

    if (node->hasChildNodes())
        return node->childNodeCount();

    // NOTE: This should preempt the childNodeCount for, e.g., select nodes
    if (editingIgnoresContent(node))
        return 1;

    return 0;
}

String stringWithRebalancedWhitespace(const String& string, bool startIsStartOfParagraph, bool endIsEndOfParagraph)
{
    Vector<UChar> rebalancedString;
    append(rebalancedString, string);

    bool previousCharacterWasSpace = false;
    for (size_t i = 0; i < rebalancedString.size(); i++) {
        if (!isWhitespace(rebalancedString[i])) {
            previousCharacterWasSpace = false;
            continue;
        }

        if (previousCharacterWasSpace || (!i && startIsStartOfParagraph) || (i + 1 == rebalancedString.size() && endIsEndOfParagraph)) {
            rebalancedString[i] = noBreakSpace;
            previousCharacterWasSpace = false;
        } else {
            rebalancedString[i] = ' ';
            previousCharacterWasSpace = true;
        }
            
    }

    return String::adopt(rebalancedString);
}

bool isTableStructureNode(const Node *node)
{
    RenderObject *r = node->renderer();
    return (r && (r->isTableCell() || r->isTableRow() || r->isTableSection() || r->isTableCol()));
}

const String& nonBreakingSpaceString()
{
    DEFINE_STATIC_LOCAL(String, nonBreakingSpaceString, (&noBreakSpace, 1));
    return nonBreakingSpaceString;
}

// FIXME: need to dump this
bool isSpecialElement(const Node *n)
{
    if (!n)
        return false;
        
    if (!n->isHTMLElement())
        return false;

    if (n->isLink())
        return true;

    RenderObject *renderer = n->renderer();
    if (!renderer)
        return false;
        
    if (renderer->style()->display() == TABLE || renderer->style()->display() == INLINE_TABLE)
        return true;

    if (renderer->style()->isFloating())
        return true;

    if (renderer->style()->position() != StaticPosition)
        return true;
        
    return false;
}

static Node* firstInSpecialElement(const Position& pos)
{
    // FIXME: This begins at pos.deprecatedNode(), which doesn't necessarily contain pos (suppose pos was [img, 0]).  See <rdar://problem/5027702>.
    Node* rootEditableElement = pos.deprecatedNode()->rootEditableElement();
    for (Node* n = pos.deprecatedNode(); n && n->rootEditableElement() == rootEditableElement; n = n->parentNode())
        if (isSpecialElement(n)) {
            VisiblePosition vPos = VisiblePosition(pos, DOWNSTREAM);
            VisiblePosition firstInElement = VisiblePosition(firstPositionInOrBeforeNode(n), DOWNSTREAM);
            if (isTableElement(n) && vPos == firstInElement.next())
                return n;
            if (vPos == firstInElement)
                return n;
        }
    return 0;
}

static Node* lastInSpecialElement(const Position& pos)
{
    // FIXME: This begins at pos.deprecatedNode(), which doesn't necessarily contain pos (suppose pos was [img, 0]).  See <rdar://problem/5027702>.
    Node* rootEditableElement = pos.deprecatedNode()->rootEditableElement();
    for (Node* n = pos.deprecatedNode(); n && n->rootEditableElement() == rootEditableElement; n = n->parentNode())
        if (isSpecialElement(n)) {
            VisiblePosition vPos = VisiblePosition(pos, DOWNSTREAM);
            VisiblePosition lastInElement = VisiblePosition(Position(n, n->childNodeCount(), Position::PositionIsOffsetInAnchor), DOWNSTREAM);
            if (isTableElement(n) && vPos == lastInElement.previous())
                return n;
            if (vPos == lastInElement)
                return n;
        }
    return 0;
}

bool isFirstVisiblePositionInSpecialElement(const Position& pos)
{
    return firstInSpecialElement(pos);
}

Position positionBeforeContainingSpecialElement(const Position& pos, Node** containingSpecialElement)
{
    Node* n = firstInSpecialElement(pos);
    if (!n)
        return pos;
    Position result = positionInParentBeforeNode(n);
    if (result.isNull() || result.deprecatedNode()->rootEditableElement() != pos.deprecatedNode()->rootEditableElement())
        return pos;
    if (containingSpecialElement)
        *containingSpecialElement = n;
    return result;
}

bool isLastVisiblePositionInSpecialElement(const Position& pos)
{
    return lastInSpecialElement(pos);
}

Position positionAfterContainingSpecialElement(const Position& pos, Node **containingSpecialElement)
{
    Node* n = lastInSpecialElement(pos);
    if (!n)
        return pos;
    Position result = positionInParentAfterNode(n);
    if (result.isNull() || result.deprecatedNode()->rootEditableElement() != pos.deprecatedNode()->rootEditableElement())
        return pos;
    if (containingSpecialElement)
        *containingSpecialElement = n;
    return result;
}

Position positionOutsideContainingSpecialElement(const Position &pos, Node **containingSpecialElement)
{
    if (isFirstVisiblePositionInSpecialElement(pos))
        return positionBeforeContainingSpecialElement(pos, containingSpecialElement);
    if (isLastVisiblePositionInSpecialElement(pos))
        return positionAfterContainingSpecialElement(pos, containingSpecialElement);
    return pos;
}

Node* isFirstPositionAfterTable(const VisiblePosition& visiblePosition)
{
    Position upstream(visiblePosition.deepEquivalent().upstream());
    if (upstream.deprecatedNode() && upstream.deprecatedNode()->renderer() && upstream.deprecatedNode()->renderer()->isTable() && upstream.atLastEditingPositionForNode())
        return upstream.deprecatedNode();
    
    return 0;
}

Node* isLastPositionBeforeTable(const VisiblePosition& visiblePosition)
{
    Position downstream(visiblePosition.deepEquivalent().downstream());
    if (downstream.deprecatedNode() && downstream.deprecatedNode()->renderer() && downstream.deprecatedNode()->renderer()->isTable() && downstream.atFirstEditingPositionForNode())
        return downstream.deprecatedNode();
    
    return 0;
}

// Returns the visible position at the beginning of a node
VisiblePosition visiblePositionBeforeNode(Node* node)
{
    ASSERT(node);
    if (node->childNodeCount())
        return VisiblePosition(firstPositionInOrBeforeNode(node), DOWNSTREAM);
    ASSERT(node->parentNode());
    return positionInParentBeforeNode(node);
}

// Returns the visible position at the ending of a node
VisiblePosition visiblePositionAfterNode(Node* node)
{
    ASSERT(node);
    if (node->childNodeCount())
        return VisiblePosition(lastPositionInOrAfterNode(node), DOWNSTREAM);
    ASSERT(node->parentNode());
    return positionInParentAfterNode(node);
}

// Create a range object with two visible positions, start and end.
// create(PassRefPtr<Document>, const Position&, const Position&); will use deprecatedEditingOffset
// Use this function instead of create a regular range object (avoiding editing offset).
PassRefPtr<Range> createRange(PassRefPtr<Document> document, const VisiblePosition& start, const VisiblePosition& end, ExceptionCode& ec)
{
    ec = 0;
    RefPtr<Range> selectedRange = Range::create(document);
    selectedRange->setStart(start.deepEquivalent().containerNode(), start.deepEquivalent().computeOffsetInContainerNode(), ec);
    if (!ec)
        selectedRange->setEnd(end.deepEquivalent().containerNode(), end.deepEquivalent().computeOffsetInContainerNode(), ec);
    return selectedRange.release();
}

// Extend rangeToExtend to include nodes that wraps range and visibly starts and ends inside or at the boudnaries of maximumRange
// e.g. if the original range spaned "hello" in <div>hello</div>, then this function extends the range to contain div's around it.
// Call this function before copying / moving paragraphs to contain all wrapping nodes.
// This function stops extending the range immediately below rootNode; i.e. the extended range can contain a child node of rootNode
// but it can never contain rootNode itself.
PassRefPtr<Range> extendRangeToWrappingNodes(PassRefPtr<Range> range, const Range* maximumRange, const Node* rootNode)
{
    ASSERT(range);
    ASSERT(maximumRange);

    ExceptionCode ec = 0;
    Node* ancestor = range->commonAncestorContainer(ec);// find the cloeset common ancestor
    Node* highestNode = 0;
    // traverse through ancestors as long as they are contained within the range, content-editable, and below rootNode (could be =0).
    while (ancestor && ancestor->rendererIsEditable() && isNodeVisiblyContainedWithin(ancestor, maximumRange) && ancestor != rootNode) {
        highestNode = ancestor;
        ancestor = ancestor->parentNode();
    }

    if (!highestNode)
        return range;

    // Create new range with the highest editable node contained within the range
    RefPtr<Range> extendedRange = Range::create(range->ownerDocument());
    extendedRange->selectNode(highestNode, ec);
    return extendedRange.release();
}

bool isListElement(Node *n)
{
    return (n && (n->hasTagName(ulTag) || n->hasTagName(olTag) || n->hasTagName(dlTag)));
}

bool isListItem(Node *n)
{
    return n && n->renderer() && n->renderer()->isListItem();
}

Node* enclosingNodeWithTag(const Position& p, const QualifiedName& tagName)
{
    if (p.isNull())
        return 0;
        
    Node* root = highestEditableRoot(p);
    for (Node* n = p.deprecatedNode(); n; n = n->parentNode()) {
        if (root && !n->rendererIsEditable())
            continue;
        if (n->hasTagName(tagName))
            return n;
        if (n == root)
            return 0;
    }
    
    return 0;
}

Node* enclosingNodeOfType(const Position& p, bool (*nodeIsOfType)(const Node*), EditingBoundaryCrossingRule rule)
{
    // FIXME: support CanSkipCrossEditingBoundary
    ASSERT(rule == CanCrossEditingBoundary || rule == CannotCrossEditingBoundary);
    if (p.isNull())
        return 0;
        
    Node* root = rule == CannotCrossEditingBoundary ? highestEditableRoot(p) : 0;
    for (Node* n = p.deprecatedNode(); n; n = n->parentNode()) {
        // Don't return a non-editable node if the input position was editable, since
        // the callers from editing will no doubt want to perform editing inside the returned node.
        if (root && !n->rendererIsEditable())
            continue;
        if (nodeIsOfType(n))
            return n;
        if (n == root)
            return 0;
    }
    
    return 0;
}

Node* highestEnclosingNodeOfType(const Position& p, bool (*nodeIsOfType)(const Node*), EditingBoundaryCrossingRule rule)
{
    Node* highest = 0;
    Node* root = rule == CannotCrossEditingBoundary ? highestEditableRoot(p) : 0;
    for (Node* n = p.containerNode(); n; n = n->parentNode()) {
        if (root && !n->rendererIsEditable())
            continue;
        if (nodeIsOfType(n))
            highest = n;
        if (n == root)
            break;
    }
    
    return highest;
}

Node* enclosingTableCell(const Position& p)
{
    return static_cast<Element*>(enclosingNodeOfType(p, isTableCell));
}

Node* enclosingAnchorElement(const Position& p)
{
    if (p.isNull())
        return 0;
    
    Node* node = p.deprecatedNode();
    while (node && !(node->isElementNode() && node->isLink()))
        node = node->parentNode();
    return node;
}

HTMLElement* enclosingList(Node* node)
{
    if (!node)
        return 0;
        
    Node* root = highestEditableRoot(firstPositionInOrBeforeNode(node));
    
    for (ContainerNode* n = node->parentNode(); n; n = n->parentNode()) {
        if (n->hasTagName(ulTag) || n->hasTagName(olTag))
            return toHTMLElement(n);
        if (n == root)
            return 0;
    }
    
    return 0;
}

Node* enclosingListChild(Node *node)
{
    if (!node)
        return 0;
    // Check for a list item element, or for a node whose parent is a list element.  Such a node
    // will appear visually as a list item (but without a list marker)
    Node* root = highestEditableRoot(firstPositionInOrBeforeNode(node));
    
    // FIXME: This function is inappropriately named if it starts with node instead of node->parentNode()
    for (Node* n = node; n && n->parentNode(); n = n->parentNode()) {
        if (n->hasTagName(liTag) || isListElement(n->parentNode()))
            return n;
        if (n == root || isTableCell(n))
            return 0;
    }
    
    return 0;
}

static HTMLElement* embeddedSublist(Node* listItem)
{
    // Check the DOM so that we'll find collapsed sublists without renderers.
    for (Node* n = listItem->firstChild(); n; n = n->nextSibling()) {
        if (isListElement(n))
            return toHTMLElement(n);
    }
    
    return 0;
}

static Node* appendedSublist(Node* listItem)
{
    // Check the DOM so that we'll find collapsed sublists without renderers.
    for (Node* n = listItem->nextSibling(); n; n = n->nextSibling()) {
        if (isListElement(n))
            return toHTMLElement(n);
        if (isListItem(listItem))
            return 0;
    }
    
    return 0;
}

// FIXME: This method should not need to call isStartOfParagraph/isEndOfParagraph
Node* enclosingEmptyListItem(const VisiblePosition& visiblePos)
{
    // Check that position is on a line by itself inside a list item
    Node* listChildNode = enclosingListChild(visiblePos.deepEquivalent().deprecatedNode());
    if (!listChildNode || !isStartOfParagraph(visiblePos) || !isEndOfParagraph(visiblePos))
        return 0;

    VisiblePosition firstInListChild(firstPositionInOrBeforeNode(listChildNode));
    VisiblePosition lastInListChild(lastPositionInOrAfterNode(listChildNode));

    if (firstInListChild != visiblePos || lastInListChild != visiblePos)
        return 0;
    
    if (embeddedSublist(listChildNode) || appendedSublist(listChildNode))
        return 0;
        
    return listChildNode;
}

HTMLElement* outermostEnclosingList(Node* node, Node* rootList)
{
    HTMLElement* list = enclosingList(node);
    if (!list)
        return 0;

    while (HTMLElement* nextList = enclosingList(list)) {
        if (nextList == rootList)
            break;
        list = nextList;
    }

    return list;
}

bool canMergeLists(Element* firstList, Element* secondList)
{
    if (!firstList || !secondList || !firstList->isHTMLElement() || !secondList->isHTMLElement())
        return false;

    return firstList->hasTagName(secondList->tagQName())// make sure the list types match (ol vs. ul)
    && firstList->rendererIsEditable() && secondList->rendererIsEditable() // both lists are editable
    && firstList->rootEditableElement() == secondList->rootEditableElement()// don't cross editing boundaries
    && isVisiblyAdjacent(positionInParentAfterNode(firstList), positionInParentBeforeNode(secondList));
    // Make sure there is no visible content between this li and the previous list
}

Node* highestAncestor(Node* node)
{
    ASSERT(node);
    Node* parent = node;
    while ((node = node->parentNode()))
        parent = node;
    return parent;
}

// FIXME: do not require renderer, so that this can be used within fragments, or rename to isRenderedTable()
bool isTableElement(Node* n)
{
    if (!n || !n->isElementNode())
        return false;

    RenderObject* renderer = n->renderer();
    return (renderer && (renderer->style()->display() == TABLE || renderer->style()->display() == INLINE_TABLE));
}

bool isTableCell(const Node* node)
{
    RenderObject* r = node->renderer();
    if (!r)
        return node->hasTagName(tdTag) || node->hasTagName(thTag);
    
    return r->isTableCell();
}

bool isEmptyTableCell(const Node* node)
{
    // Returns true IFF the passed in node is one of:
    //   .) a table cell with no children,
    //   .) a table cell with a single BR child, and which has no other child renderers, including :before and :after renderers
    //   .) the BR child of such a table cell

    // Find rendered node
    while (node && !node->renderer())
        node = node->parentNode();
    if (!node)
        return false;

    // Make sure the rendered node is a table cell or <br>.
    // If it's a <br>, then the parent node has to be a table cell.
    RenderObject* renderer = node->renderer();
    if (renderer->isBR()) {
        renderer = renderer->parent();
        if (!renderer)
            return false;
    }
    if (!renderer->isTableCell())
        return false;

    // Check that the table cell contains no child renderers except for perhaps a single <br>.
    RenderObject* childRenderer = renderer->firstChild();
    if (!childRenderer)
        return true;
    if (!childRenderer->isBR())
        return false;
    return !childRenderer->nextSibling();
}

PassRefPtr<HTMLElement> createDefaultParagraphElement(Document* document)
{
    return HTMLDivElement::create(document);
}

PassRefPtr<HTMLElement> createBreakElement(Document* document)
{
    return HTMLBRElement::create(document);
}

PassRefPtr<HTMLElement> createOrderedListElement(Document* document)
{
    return HTMLOListElement::create(document);
}

PassRefPtr<HTMLElement> createUnorderedListElement(Document* document)
{
    return HTMLUListElement::create(document);
}

PassRefPtr<HTMLElement> createListItemElement(Document* document)
{
    return HTMLLIElement::create(document);
}

PassRefPtr<HTMLElement> createHTMLElement(Document* document, const QualifiedName& name)
{
    return HTMLElementFactory::createHTMLElement(name, document, 0, false);
}

PassRefPtr<HTMLElement> createHTMLElement(Document* document, const AtomicString& tagName)
{
    return createHTMLElement(document, QualifiedName(nullAtom, tagName, xhtmlNamespaceURI));
}

bool isTabSpanNode(const Node *node)
{
    return node && node->hasTagName(spanTag) && node->isElementNode() && static_cast<const Element *>(node)->getAttribute(classAttr) == AppleTabSpanClass;
}

bool isTabSpanTextNode(const Node *node)
{
    return node && node->isTextNode() && node->parentNode() && isTabSpanNode(node->parentNode());
}

Node *tabSpanNode(const Node *node)
{
    return isTabSpanTextNode(node) ? node->parentNode() : 0;
}

bool isNodeInTextFormControl(Node* node)
{
    if (!node)
        return false;
    Node* ancestor = node->shadowAncestorNode();
    if (ancestor == node)
        return false;
    return ancestor->isElementNode() && static_cast<Element*>(ancestor)->isTextFormControl();
}
    
Position positionOutsideTabSpan(const Position& pos)
{
    Node* node = pos.containerNode();
    if (isTabSpanTextNode(node))
        node = tabSpanNode(node);
    else if (!isTabSpanNode(node))
        return pos;

    if (node && VisiblePosition(pos) == lastPositionInNode(node))
        return positionInParentAfterNode(node);

    return positionInParentBeforeNode(node);
}

PassRefPtr<Element> createTabSpanElement(Document* document, PassRefPtr<Node> tabTextNode)
{
    // Make the span to hold the tab.
    RefPtr<Element> spanElement = document->createElement(spanTag, false);
    spanElement->setAttribute(classAttr, AppleTabSpanClass);
    spanElement->setAttribute(styleAttr, "white-space:pre");

    // Add tab text to that span.
    if (!tabTextNode)
        tabTextNode = document->createEditingTextNode("\t");

    ExceptionCode ec = 0;
    spanElement->appendChild(tabTextNode, ec);
    ASSERT(ec == 0);

    return spanElement.release();
}

PassRefPtr<Element> createTabSpanElement(Document* document, const String& tabText)
{
    return createTabSpanElement(document, document->createTextNode(tabText));
}

PassRefPtr<Element> createTabSpanElement(Document* document)
{
    return createTabSpanElement(document, PassRefPtr<Node>());
}

bool isNodeRendered(const Node *node)
{
    if (!node)
        return false;

    RenderObject *renderer = node->renderer();
    if (!renderer)
        return false;

    return renderer->style()->visibility() == VISIBLE;
}

unsigned numEnclosingMailBlockquotes(const Position& p)
{
    unsigned num = 0;
    for (Node* n = p.deprecatedNode(); n; n = n->parentNode())
        if (isMailBlockquote(n))
            num++;
    
    return num;
}

bool isMailBlockquote(const Node *node)
{
    if (!node || !node->hasTagName(blockquoteTag))
        return false;
        
    return static_cast<const Element *>(node)->getAttribute("type") == "cite";
}

int caretMinOffset(const Node* n)
{
    RenderObject* r = n->renderer();
    ASSERT(!n->isCharacterDataNode() || !r || r->isText()); // FIXME: This was a runtime check that seemingly couldn't fail; changed it to an assertion for now.
    return r ? r->caretMinOffset() : 0;
}

// If a node can contain candidates for VisiblePositions, return the offset of the last candidate, otherwise 
// return the number of children for container nodes and the length for unrendered text nodes.
int caretMaxOffset(const Node* n)
{
    // For rendered text nodes, return the last position that a caret could occupy.
    if (n->isTextNode() && n->renderer())
        return n->renderer()->caretMaxOffset();
    // For containers return the number of children.  For others do the same as above.
    return lastOffsetForEditing(n);
}

bool lineBreakExistsAtVisiblePosition(const VisiblePosition& visiblePosition)
{
    return lineBreakExistsAtPosition(visiblePosition.deepEquivalent().downstream());
}

bool lineBreakExistsAtPosition(const Position& position)
{
    if (position.isNull())
        return false;
    
    if (position.anchorNode()->hasTagName(brTag) && position.atFirstEditingPositionForNode())
        return true;
    
    if (!position.anchorNode()->renderer())
        return false;
    
    if (!position.anchorNode()->isTextNode() || !position.anchorNode()->renderer()->style()->preserveNewline())
        return false;
    
    Text* textNode = static_cast<Text*>(position.anchorNode());
    unsigned offset = position.offsetInContainerNode();
    return offset < textNode->length() && textNode->data()[offset] == '\n';
}

// Modifies selections that have an end point at the edge of a table
// that contains the other endpoint so that they don't confuse
// code that iterates over selected paragraphs.
VisibleSelection selectionForParagraphIteration(const VisibleSelection& original)
{
    VisibleSelection newSelection(original);
    VisiblePosition startOfSelection(newSelection.visibleStart());
    VisiblePosition endOfSelection(newSelection.visibleEnd());
    
    // If the end of the selection to modify is just after a table, and
    // if the start of the selection is inside that table, then the last paragraph
    // that we'll want modify is the last one inside the table, not the table itself
    // (a table is itself a paragraph).
    if (Node* table = isFirstPositionAfterTable(endOfSelection))
        if (startOfSelection.deepEquivalent().deprecatedNode()->isDescendantOf(table))
            newSelection = VisibleSelection(startOfSelection, endOfSelection.previous(CannotCrossEditingBoundary));
    
    // If the start of the selection to modify is just before a table,
    // and if the end of the selection is inside that table, then the first paragraph
    // we'll want to modify is the first one inside the table, not the paragraph
    // containing the table itself.
    if (Node* table = isLastPositionBeforeTable(startOfSelection))
        if (endOfSelection.deepEquivalent().deprecatedNode()->isDescendantOf(table))
            newSelection = VisibleSelection(startOfSelection.next(CannotCrossEditingBoundary), endOfSelection);
    
    return newSelection;
}


int indexForVisiblePosition(const VisiblePosition& visiblePosition)
{
    if (visiblePosition.isNull())
        return 0;
    Position p(visiblePosition.deepEquivalent());
    RefPtr<Range> range = Range::create(p.anchorNode()->document(), firstPositionInNode(p.anchorNode()->document()->documentElement()),
                                        p.parentAnchoredEquivalent());
    return TextIterator::rangeLength(range.get(), true);
}

// Determines whether two positions are visibly next to each other (first then second)
// while ignoring whitespaces and unrendered nodes
bool isVisiblyAdjacent(const Position& first, const Position& second)
{
    return VisiblePosition(first) == VisiblePosition(second.upstream());
}

// Determines whether a node is inside a range or visibly starts and ends at the boundaries of the range.
// Call this function to determine whether a node is visibly fit inside selectedRange
bool isNodeVisiblyContainedWithin(Node* node, const Range* selectedRange)
{
    ASSERT(node);
    ASSERT(selectedRange);
    // If the node is inside the range, then it surely is contained within
    ExceptionCode ec = 0;
    if (selectedRange->compareNode(node, ec) == Range::NODE_INSIDE)
        return true;

    bool startIsVisuallySame = visiblePositionBeforeNode(node) == selectedRange->startPosition();
    if (startIsVisuallySame && comparePositions(positionInParentAfterNode(node), selectedRange->endPosition()) < 0)
        return true;

    bool endIsVisuallySame = visiblePositionAfterNode(node) == selectedRange->endPosition();
    if (endIsVisuallySame && comparePositions(selectedRange->startPosition(), positionInParentBeforeNode(node)) < 0)
        return true;

    return startIsVisuallySame && endIsVisuallySame;
}

bool isRenderedAsNonInlineTableImageOrHR(const Node* node)
{
    if (!node)
        return false;
    RenderObject* renderer = node->renderer();
    return renderer && ((renderer->isTable() && !renderer->isInline()) || (renderer->isImage() && !renderer->isInline()) || renderer->isHR());
}

PassRefPtr<Range> avoidIntersectionWithNode(const Range* range, Node* node)
{
    if (!range)
        return 0;

    Document* document = range->ownerDocument();

    Node* startContainer = range->startContainer();
    int startOffset = range->startOffset();
    Node* endContainer = range->endContainer();
    int endOffset = range->endOffset();

    if (!startContainer)
        return 0;

    ASSERT(endContainer);

    if (startContainer == node || startContainer->isDescendantOf(node)) {
        ASSERT(node->parentNode());
        startContainer = node->parentNode();
        startOffset = node->nodeIndex();
    }
    if (endContainer == node || endContainer->isDescendantOf(node)) {
        ASSERT(node->parentNode());
        endContainer = node->parentNode();
        endOffset = node->nodeIndex();
    }

    return Range::create(document, startContainer, startOffset, endContainer, endOffset);
}

VisibleSelection avoidIntersectionWithNode(const VisibleSelection& selection, Node* node)
{
    if (selection.isNone())
        return VisibleSelection(selection);

    VisibleSelection updatedSelection(selection);
    Node* base = selection.base().deprecatedNode();
    Node* extent = selection.extent().deprecatedNode();
    ASSERT(base);
    ASSERT(extent);

    if (base == node || base->isDescendantOf(node)) {
        ASSERT(node->parentNode());
        updatedSelection.setBase(positionInParentBeforeNode(node));
    }

    if (extent == node || extent->isDescendantOf(node)) {
        ASSERT(node->parentNode());
        updatedSelection.setExtent(positionInParentBeforeNode(node));
    }

    return updatedSelection;
}

} // namespace WebCore
