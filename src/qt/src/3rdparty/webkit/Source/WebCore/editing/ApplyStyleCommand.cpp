/*
 * Copyright (C) 2005, 2006, 2008, 2009 Apple Inc. All rights reserved.
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
#include "ApplyStyleCommand.h"

#include "CSSComputedStyleDeclaration.h"
#include "CSSMutableStyleDeclaration.h"
#include "CSSParser.h"
#include "CSSProperty.h"
#include "CSSPropertyNames.h"
#include "CSSStyleSelector.h"
#include "CSSValueKeywords.h"
#include "CSSValueList.h"
#include "Document.h"
#include "EditingStyle.h"
#include "Editor.h"
#include "Frame.h"
#include "HTMLFontElement.h"
#include "HTMLInterchange.h"
#include "HTMLNames.h"
#include "NodeList.h"
#include "Range.h"
#include "RenderObject.h"
#include "Text.h"
#include "TextIterator.h"
#include "htmlediting.h"
#include "visible_units.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

using namespace HTMLNames;

static String& styleSpanClassString()
{
    DEFINE_STATIC_LOCAL(String, styleSpanClassString, ((AppleStyleSpanClass)));
    return styleSpanClassString;
}

bool isStyleSpan(const Node *node)
{
    if (!node || !node->isHTMLElement())
        return false;

    const HTMLElement* elem = static_cast<const HTMLElement*>(node);
    return elem->hasLocalName(spanAttr) && elem->getAttribute(classAttr) == styleSpanClassString();
}

static bool isUnstyledStyleSpan(const Node* node)
{
    if (!node || !node->isHTMLElement() || !node->hasTagName(spanTag))
        return false;

    const HTMLElement* elem = static_cast<const HTMLElement*>(node);
    CSSMutableStyleDeclaration* inlineStyleDecl = elem->inlineStyleDecl();
    return (!inlineStyleDecl || inlineStyleDecl->isEmpty()) && elem->getAttribute(classAttr) == styleSpanClassString();
}

static bool isSpanWithoutAttributesOrUnstyleStyleSpan(const Node* node)
{
    if (!node || !node->isHTMLElement() || !node->hasTagName(spanTag))
        return false;

    const HTMLElement* elem = static_cast<const HTMLElement*>(node);
    NamedNodeMap* attributes = elem->attributes(true); // readonly
    if (attributes->isEmpty())
        return true;

    return isUnstyledStyleSpan(node);
}

static bool isEmptyFontTag(const Node *node)
{
    if (!node || !node->hasTagName(fontTag))
        return false;

    const Element *elem = static_cast<const Element *>(node);
    NamedNodeMap *map = elem->attributes(true); // true for read-only
    if (!map)
        return true;
    return map->isEmpty() || (map->length() == 1 && elem->getAttribute(classAttr) == styleSpanClassString());
}

static PassRefPtr<Element> createFontElement(Document* document)
{
    RefPtr<Element> fontNode = createHTMLElement(document, fontTag);
    fontNode->setAttribute(classAttr, styleSpanClassString());
    return fontNode.release();
}

PassRefPtr<HTMLElement> createStyleSpanElement(Document* document)
{
    RefPtr<HTMLElement> styleElement = createHTMLElement(document, spanTag);
    styleElement->setAttribute(classAttr, styleSpanClassString());
    return styleElement.release();
}

ApplyStyleCommand::ApplyStyleCommand(Document* document, const EditingStyle* style, EditAction editingAction, EPropertyLevel propertyLevel)
    : CompositeEditCommand(document)
    , m_style(style->copy())
    , m_editingAction(editingAction)
    , m_propertyLevel(propertyLevel)
    , m_start(endingSelection().start().downstream())
    , m_end(endingSelection().end().upstream())
    , m_useEndingSelection(true)
    , m_styledInlineElement(0)
    , m_removeOnly(false)
    , m_isInlineElementToRemoveFunction(0)
{
}

ApplyStyleCommand::ApplyStyleCommand(Document* document, const EditingStyle* style, const Position& start, const Position& end, EditAction editingAction, EPropertyLevel propertyLevel)
    : CompositeEditCommand(document)
    , m_style(style->copy())
    , m_editingAction(editingAction)
    , m_propertyLevel(propertyLevel)
    , m_start(start)
    , m_end(end)
    , m_useEndingSelection(false)
    , m_styledInlineElement(0)
    , m_removeOnly(false)
    , m_isInlineElementToRemoveFunction(0)
{
}

ApplyStyleCommand::ApplyStyleCommand(PassRefPtr<Element> element, bool removeOnly, EditAction editingAction)
    : CompositeEditCommand(element->document())
    , m_style(EditingStyle::create())
    , m_editingAction(editingAction)
    , m_propertyLevel(PropertyDefault)
    , m_start(endingSelection().start().downstream())
    , m_end(endingSelection().end().upstream())
    , m_useEndingSelection(true)
    , m_styledInlineElement(element)
    , m_removeOnly(removeOnly)
    , m_isInlineElementToRemoveFunction(0)
{
}

ApplyStyleCommand::ApplyStyleCommand(Document* document, const EditingStyle* style, IsInlineElementToRemoveFunction isInlineElementToRemoveFunction, EditAction editingAction)
    : CompositeEditCommand(document)
    , m_style(style->copy())
    , m_editingAction(editingAction)
    , m_propertyLevel(PropertyDefault)
    , m_start(endingSelection().start().downstream())
    , m_end(endingSelection().end().upstream())
    , m_useEndingSelection(true)
    , m_styledInlineElement(0)
    , m_removeOnly(true)
    , m_isInlineElementToRemoveFunction(isInlineElementToRemoveFunction)
{
}

void ApplyStyleCommand::updateStartEnd(const Position& newStart, const Position& newEnd)
{
    ASSERT(comparePositions(newEnd, newStart) >= 0);

    if (!m_useEndingSelection && (newStart != m_start || newEnd != m_end))
        m_useEndingSelection = true;

    setEndingSelection(VisibleSelection(newStart, newEnd, VP_DEFAULT_AFFINITY));
    m_start = newStart;
    m_end = newEnd;
}

Position ApplyStyleCommand::startPosition()
{
    if (m_useEndingSelection)
        return endingSelection().start();
    
    return m_start;
}

Position ApplyStyleCommand::endPosition()
{
    if (m_useEndingSelection)
        return endingSelection().end();
    
    return m_end;
}

void ApplyStyleCommand::doApply()
{
    switch (m_propertyLevel) {
    case PropertyDefault: {
        // Apply the block-centric properties of the style.
        RefPtr<EditingStyle> blockStyle = m_style->extractAndRemoveBlockProperties();
        if (!blockStyle->isEmpty())
            applyBlockStyle(blockStyle.get());
        // Apply any remaining styles to the inline elements.
        if (!m_style->isEmpty() || m_styledInlineElement || m_isInlineElementToRemoveFunction) {
            applyRelativeFontStyleChange(m_style.get());
            applyInlineStyle(m_style.get());
        }
        break;
    }
    case ForceBlockProperties:
        // Force all properties to be applied as block styles.
        applyBlockStyle(m_style.get());
        break;
    }
}

EditAction ApplyStyleCommand::editingAction() const
{
    return m_editingAction;
}

void ApplyStyleCommand::applyBlockStyle(EditingStyle *style)
{
    // update document layout once before removing styles
    // so that we avoid the expense of updating before each and every call
    // to check a computed style
    updateLayout();

    // get positions we want to use for applying style
    Position start = startPosition();
    Position end = endPosition();
    if (comparePositions(end, start) < 0) {
        Position swap = start;
        start = end;
        end = swap;
    }

    VisiblePosition visibleStart(start);
    VisiblePosition visibleEnd(end);

    if (visibleStart.isNull() || visibleStart.isOrphan() || visibleEnd.isNull() || visibleEnd.isOrphan())
        return;

    // Save and restore the selection endpoints using their indices in the document, since
    // addBlockStyleIfNeeded may moveParagraphs, which can remove these endpoints.
    // Calculate start and end indices from the start of the tree that they're in.
    Node* scope = highestAncestor(visibleStart.deepEquivalent().deprecatedNode());
    RefPtr<Range> startRange = Range::create(document(), firstPositionInNode(scope), visibleStart.deepEquivalent().parentAnchoredEquivalent());
    RefPtr<Range> endRange = Range::create(document(), firstPositionInNode(scope), visibleEnd.deepEquivalent().parentAnchoredEquivalent());
    int startIndex = TextIterator::rangeLength(startRange.get(), true);
    int endIndex = TextIterator::rangeLength(endRange.get(), true);

    VisiblePosition paragraphStart(startOfParagraph(visibleStart));
    VisiblePosition nextParagraphStart(endOfParagraph(paragraphStart).next());
    VisiblePosition beyondEnd(endOfParagraph(visibleEnd).next());
    while (paragraphStart.isNotNull() && paragraphStart != beyondEnd) {
        StyleChange styleChange(style, paragraphStart.deepEquivalent());
        if (styleChange.cssStyle().length() || m_removeOnly) {
            RefPtr<Node> block = enclosingBlock(paragraphStart.deepEquivalent().deprecatedNode());
            if (!m_removeOnly) {
                RefPtr<Node> newBlock = moveParagraphContentsToNewBlockIfNecessary(paragraphStart.deepEquivalent());
                if (newBlock)
                    block = newBlock;
            }
            ASSERT(block->isHTMLElement());
            if (block->isHTMLElement()) {
                removeCSSStyle(style, toHTMLElement(block.get()));
                if (!m_removeOnly)
                    addBlockStyle(styleChange, toHTMLElement(block.get()));
            }

            if (nextParagraphStart.isOrphan())
                nextParagraphStart = endOfParagraph(paragraphStart).next();
        }

        paragraphStart = nextParagraphStart;
        nextParagraphStart = endOfParagraph(paragraphStart).next();
    }
    
    startRange = TextIterator::rangeFromLocationAndLength(static_cast<Element*>(scope), startIndex, 0, true);
    endRange = TextIterator::rangeFromLocationAndLength(static_cast<Element*>(scope), endIndex, 0, true);
    if (startRange && endRange)
        updateStartEnd(startRange->startPosition(), endRange->startPosition());
}

void ApplyStyleCommand::applyRelativeFontStyleChange(EditingStyle* style)
{
    static const float MinimumFontSize = 0.1f;

    if (!style || !style->hasFontSizeDelta())
        return;

    Position start = startPosition();
    Position end = endPosition();
    if (comparePositions(end, start) < 0) {
        Position swap = start;
        start = end;
        end = swap;
    }

    // Join up any adjacent text nodes.
    if (start.deprecatedNode()->isTextNode()) {
        joinChildTextNodes(start.deprecatedNode()->parentNode(), start, end);
        start = startPosition();
        end = endPosition();
    }
    if (end.deprecatedNode()->isTextNode() && start.deprecatedNode()->parentNode() != end.deprecatedNode()->parentNode()) {
        joinChildTextNodes(end.deprecatedNode()->parentNode(), start, end);
        start = startPosition();
        end = endPosition();
    }

    // Split the start text nodes if needed to apply style.
    if (isValidCaretPositionInTextNode(start)) {
        splitTextAtStart(start, end);
        start = startPosition();
        end = endPosition();
    }

    if (isValidCaretPositionInTextNode(end)) {
        splitTextAtEnd(start, end);
        start = startPosition();
        end = endPosition();
    }

    // Calculate loop end point.
    // If the end node is before the start node (can only happen if the end node is
    // an ancestor of the start node), we gather nodes up to the next sibling of the end node
    Node *beyondEnd;
    if (start.deprecatedNode()->isDescendantOf(end.deprecatedNode()))
        beyondEnd = end.deprecatedNode()->traverseNextSibling();
    else
        beyondEnd = end.deprecatedNode()->traverseNextNode();
    
    start = start.upstream(); // Move upstream to ensure we do not add redundant spans.
    Node* startNode = start.deprecatedNode();
    if (startNode->isTextNode() && start.deprecatedEditingOffset() >= caretMaxOffset(startNode)) // Move out of text node if range does not include its characters.
        startNode = startNode->traverseNextNode();

    // Store away font size before making any changes to the document.
    // This ensures that changes to one node won't effect another.
    HashMap<Node*, float> startingFontSizes;
    for (Node *node = startNode; node != beyondEnd; node = node->traverseNextNode())
        startingFontSizes.set(node, computedFontSize(node));

    // These spans were added by us. If empty after font size changes, they can be removed.
    Vector<RefPtr<HTMLElement> > unstyledSpans;
    
    Node* lastStyledNode = 0;
    for (Node* node = startNode; node != beyondEnd; node = node->traverseNextNode()) {
        RefPtr<HTMLElement> element;
        if (node->isHTMLElement()) {
            // Only work on fully selected nodes.
            if (!nodeFullySelected(node, start, end))
                continue;
            element = toHTMLElement(node);
        } else if (node->isTextNode() && node->renderer() && node->parentNode() != lastStyledNode) {
            // Last styled node was not parent node of this text node, but we wish to style this
            // text node. To make this possible, add a style span to surround this text node.
            RefPtr<HTMLElement> span = createStyleSpanElement(document());
            surroundNodeRangeWithElement(node, node, span.get());
            element = span.release();
        }  else {
            // Only handle HTML elements and text nodes.
            continue;
        }
        lastStyledNode = node;

        CSSMutableStyleDeclaration* inlineStyleDecl = element->getInlineStyleDecl();
        float currentFontSize = computedFontSize(node);
        float desiredFontSize = max(MinimumFontSize, startingFontSizes.get(node) + style->fontSizeDelta());
        RefPtr<CSSValue> value = inlineStyleDecl->getPropertyCSSValue(CSSPropertyFontSize);
        if (value) {
            inlineStyleDecl->removeProperty(CSSPropertyFontSize, true);
            currentFontSize = computedFontSize(node);
        }
        if (currentFontSize != desiredFontSize) {
            inlineStyleDecl->setProperty(CSSPropertyFontSize, String::number(desiredFontSize) + "px", false, false);
            setNodeAttribute(element.get(), styleAttr, inlineStyleDecl->cssText());
        }
        if (inlineStyleDecl->isEmpty()) {
            removeNodeAttribute(element.get(), styleAttr);
            // FIXME: should this be isSpanWithoutAttributesOrUnstyleStyleSpan?  Need a test.
            if (isUnstyledStyleSpan(element.get()))
                unstyledSpans.append(element.release());
        }
    }

    size_t size = unstyledSpans.size();
    for (size_t i = 0; i < size; ++i)
        removeNodePreservingChildren(unstyledSpans[i].get());
}

static Node* dummySpanAncestorForNode(const Node* node)
{
    while (node && !isStyleSpan(node))
        node = node->parentNode();
    
    return node ? node->parentNode() : 0;
}

void ApplyStyleCommand::cleanupUnstyledAppleStyleSpans(Node* dummySpanAncestor)
{
    if (!dummySpanAncestor)
        return;

    // Dummy spans are created when text node is split, so that style information
    // can be propagated, which can result in more splitting. If a dummy span gets
    // cloned/split, the new node is always a sibling of it. Therefore, we scan
    // all the children of the dummy's parent
    Node* next;
    for (Node* node = dummySpanAncestor->firstChild(); node; node = next) {
        next = node->nextSibling();
        if (isUnstyledStyleSpan(node))
            removeNodePreservingChildren(node);
        node = next;
    }
}

HTMLElement* ApplyStyleCommand::splitAncestorsWithUnicodeBidi(Node* node, bool before, WritingDirection allowedDirection)
{
    // We are allowed to leave the highest ancestor with unicode-bidi unsplit if it is unicode-bidi: embed and direction: allowedDirection.
    // In that case, we return the unsplit ancestor. Otherwise, we return 0.
    Node* block = enclosingBlock(node);
    if (!block)
        return 0;

    Node* highestAncestorWithUnicodeBidi = 0;
    Node* nextHighestAncestorWithUnicodeBidi = 0;
    int highestAncestorUnicodeBidi = 0;
    for (Node* n = node->parentNode(); n != block; n = n->parentNode()) {
        int unicodeBidi = getIdentifierValue(computedStyle(n).get(), CSSPropertyUnicodeBidi);
        if (unicodeBidi && unicodeBidi != CSSValueNormal) {
            highestAncestorUnicodeBidi = unicodeBidi;
            nextHighestAncestorWithUnicodeBidi = highestAncestorWithUnicodeBidi;
            highestAncestorWithUnicodeBidi = n;
        }
    }

    if (!highestAncestorWithUnicodeBidi)
        return 0;

    HTMLElement* unsplitAncestor = 0;

    WritingDirection highestAncestorDirection;
    if (allowedDirection != NaturalWritingDirection
        && highestAncestorUnicodeBidi != CSSValueBidiOverride
        && highestAncestorWithUnicodeBidi->isHTMLElement()
        && EditingStyle::create(highestAncestorWithUnicodeBidi, EditingStyle::AllProperties)->textDirection(highestAncestorDirection)
        && highestAncestorDirection == allowedDirection) {
        if (!nextHighestAncestorWithUnicodeBidi)
            return toHTMLElement(highestAncestorWithUnicodeBidi);

        unsplitAncestor = toHTMLElement(highestAncestorWithUnicodeBidi);
        highestAncestorWithUnicodeBidi = nextHighestAncestorWithUnicodeBidi;
    }

    // Split every ancestor through highest ancestor with embedding.
    Node* n = node;
    while (true) {
        Element* parent = static_cast<Element*>(n->parentNode());
        if (before ? n->previousSibling() : n->nextSibling())
            splitElement(parent, before ? n : n->nextSibling());
        if (parent == highestAncestorWithUnicodeBidi)
            break;
        n = n->parentNode();
    }
    return unsplitAncestor;
}

void ApplyStyleCommand::removeEmbeddingUpToEnclosingBlock(Node* node, Node* unsplitAncestor)
{
    Node* block = enclosingBlock(node);
    if (!block)
        return;

    Node* parent = 0;
    for (Node* n = node->parentNode(); n != block && n != unsplitAncestor; n = parent) {
        parent = n->parentNode();
        if (!n->isStyledElement())
            continue;

        StyledElement* element = static_cast<StyledElement*>(n);
        int unicodeBidi = getIdentifierValue(computedStyle(element).get(), CSSPropertyUnicodeBidi);
        if (!unicodeBidi || unicodeBidi == CSSValueNormal)
            continue;

        // FIXME: This code should really consider the mapped attribute 'dir', the inline style declaration,
        // and all matching style rules in order to determine how to best set the unicode-bidi property to 'normal'.
        // For now, it assumes that if the 'dir' attribute is present, then removing it will suffice, and
        // otherwise it sets the property in the inline style declaration.
        if (element->hasAttribute(dirAttr)) {
            // FIXME: If this is a BDO element, we should probably just remove it if it has no
            // other attributes, like we (should) do with B and I elements.
            removeNodeAttribute(element, dirAttr);
        } else {
            RefPtr<CSSMutableStyleDeclaration> inlineStyle = element->getInlineStyleDecl()->copy();
            inlineStyle->setProperty(CSSPropertyUnicodeBidi, CSSValueNormal);
            inlineStyle->removeProperty(CSSPropertyDirection);
            setNodeAttribute(element, styleAttr, inlineStyle->cssText());
            // FIXME: should this be isSpanWithoutAttributesOrUnstyleStyleSpan?  Need a test.
            if (isUnstyledStyleSpan(element))
                removeNodePreservingChildren(element);
        }
    }
}

static Node* highestEmbeddingAncestor(Node* startNode, Node* enclosingNode)
{
    for (Node* n = startNode; n && n != enclosingNode; n = n->parentNode()) {
        if (n->isHTMLElement() && getIdentifierValue(computedStyle(n).get(), CSSPropertyUnicodeBidi) == CSSValueEmbed)
            return n;
    }

    return 0;
}

void ApplyStyleCommand::applyInlineStyle(EditingStyle* style)
{
    Node* startDummySpanAncestor = 0;
    Node* endDummySpanAncestor = 0;

    style->collapseTextDecorationProperties();

    // update document layout once before removing styles
    // so that we avoid the expense of updating before each and every call
    // to check a computed style
    updateLayout();

    // adjust to the positions we want to use for applying style
    Position start = startPosition();
    Position end = endPosition();
    if (comparePositions(end, start) < 0) {
        Position swap = start;
        start = end;
        end = swap;
    }

    // split the start node and containing element if the selection starts inside of it
    bool splitStart = isValidCaretPositionInTextNode(start);
    if (splitStart) {
        if (shouldSplitTextElement(start.deprecatedNode()->parentElement(), style))
            splitTextElementAtStart(start, end);
        else
            splitTextAtStart(start, end);
        start = startPosition();
        end = endPosition();
        startDummySpanAncestor = dummySpanAncestorForNode(start.deprecatedNode());
    }

    // split the end node and containing element if the selection ends inside of it
    bool splitEnd = isValidCaretPositionInTextNode(end);
    if (splitEnd) {
        if (shouldSplitTextElement(end.deprecatedNode()->parentElement(), style))
            splitTextElementAtEnd(start, end);
        else
            splitTextAtEnd(start, end);
        start = startPosition();
        end = endPosition();
        endDummySpanAncestor = dummySpanAncestorForNode(end.deprecatedNode());
    }

    // Remove style from the selection.
    // Use the upstream position of the start for removing style.
    // This will ensure we remove all traces of the relevant styles from the selection
    // and prevent us from adding redundant ones, as described in:
    // <rdar://problem/3724344> Bolding and unbolding creates extraneous tags
    Position removeStart = start.upstream();
    WritingDirection textDirection = NaturalWritingDirection;
    bool hasTextDirection = style->textDirection(textDirection);
    RefPtr<EditingStyle> styleWithoutEmbedding;
    RefPtr<EditingStyle> embeddingStyle;
    if (hasTextDirection) {
        // Leave alone an ancestor that provides the desired single level embedding, if there is one.
        HTMLElement* startUnsplitAncestor = splitAncestorsWithUnicodeBidi(start.deprecatedNode(), true, textDirection);
        HTMLElement* endUnsplitAncestor = splitAncestorsWithUnicodeBidi(end.deprecatedNode(), false, textDirection);
        removeEmbeddingUpToEnclosingBlock(start.deprecatedNode(), startUnsplitAncestor);
        removeEmbeddingUpToEnclosingBlock(end.deprecatedNode(), endUnsplitAncestor);

        // Avoid removing the dir attribute and the unicode-bidi and direction properties from the unsplit ancestors.
        Position embeddingRemoveStart = removeStart;
        if (startUnsplitAncestor && nodeFullySelected(startUnsplitAncestor, removeStart, end))
            embeddingRemoveStart = positionInParentAfterNode(startUnsplitAncestor);

        Position embeddingRemoveEnd = end;
        if (endUnsplitAncestor && nodeFullySelected(endUnsplitAncestor, removeStart, end))
            embeddingRemoveEnd = positionInParentBeforeNode(endUnsplitAncestor).downstream();

        if (embeddingRemoveEnd != removeStart || embeddingRemoveEnd != end) {
            styleWithoutEmbedding = style->copy();
            embeddingStyle = styleWithoutEmbedding->extractAndRemoveTextDirection();

            if (comparePositions(embeddingRemoveStart, embeddingRemoveEnd) <= 0)
                removeInlineStyle(embeddingStyle.get(), embeddingRemoveStart, embeddingRemoveEnd);
        }
    }

    removeInlineStyle(styleWithoutEmbedding ? styleWithoutEmbedding.get() : style, removeStart, end);
    start = startPosition();
    end = endPosition();
    if (start.isNull() || start.isOrphan() || end.isNull() || end.isOrphan())
        return;

    if (splitStart && mergeStartWithPreviousIfIdentical(start, end)) {
        start = startPosition();
        end = endPosition();
    }

    if (splitEnd) {
        mergeEndWithNextIfIdentical(start, end);
        start = startPosition();
        end = endPosition();
    }

    // update document layout once before running the rest of the function
    // so that we avoid the expense of updating before each and every call
    // to check a computed style
    updateLayout();

    RefPtr<EditingStyle> styleToApply = style;
    if (hasTextDirection) {
        // Avoid applying the unicode-bidi and direction properties beneath ancestors that already have them.
        Node* embeddingStartNode = highestEmbeddingAncestor(start.deprecatedNode(), enclosingBlock(start.deprecatedNode()));
        Node* embeddingEndNode = highestEmbeddingAncestor(end.deprecatedNode(), enclosingBlock(end.deprecatedNode()));

        if (embeddingStartNode || embeddingEndNode) {
            Position embeddingApplyStart = embeddingStartNode ? positionInParentAfterNode(embeddingStartNode) : start;
            Position embeddingApplyEnd = embeddingEndNode ? positionInParentBeforeNode(embeddingEndNode) : end;
            ASSERT(embeddingApplyStart.isNotNull() && embeddingApplyEnd.isNotNull());

            if (!embeddingStyle) {
                styleWithoutEmbedding = style->copy();
                embeddingStyle = styleWithoutEmbedding->extractAndRemoveTextDirection();
            }
            fixRangeAndApplyInlineStyle(embeddingStyle.get(), embeddingApplyStart, embeddingApplyEnd);

            styleToApply = styleWithoutEmbedding;
        }
    }

    fixRangeAndApplyInlineStyle(styleToApply.get(), start, end);

    // Remove dummy style spans created by splitting text elements.
    cleanupUnstyledAppleStyleSpans(startDummySpanAncestor);
    if (endDummySpanAncestor != startDummySpanAncestor)
        cleanupUnstyledAppleStyleSpans(endDummySpanAncestor);
}

void ApplyStyleCommand::fixRangeAndApplyInlineStyle(EditingStyle* style, const Position& start, const Position& end)
{
    Node* startNode = start.deprecatedNode();

    if (start.deprecatedEditingOffset() >= caretMaxOffset(start.deprecatedNode())) {
        startNode = startNode->traverseNextNode();
        if (!startNode || comparePositions(end, firstPositionInOrBeforeNode(startNode)) < 0)
            return;
    }

    Node* pastEndNode = end.deprecatedNode();
    if (end.deprecatedEditingOffset() >= caretMaxOffset(end.deprecatedNode()))
        pastEndNode = end.deprecatedNode()->traverseNextSibling();

    // FIXME: Callers should perform this operation on a Range that includes the br
    // if they want style applied to the empty line.
    if (start == end && start.deprecatedNode()->hasTagName(brTag))
        pastEndNode = start.deprecatedNode()->traverseNextNode();

    // Start from the highest fully selected ancestor so that we can modify the fully selected node.
    // e.g. When applying font-size: large on <font color="blue">hello</font>, we need to include the font element in our run
    // to generate <font color="blue" size="4">hello</font> instead of <font color="blue"><font size="4">hello</font></font>
    RefPtr<Range> range = Range::create(startNode->document(), start, end);
    Element* editableRoot = startNode->rootEditableElement();
    if (startNode != editableRoot) {
        while (editableRoot && startNode->parentNode() != editableRoot && isNodeVisiblyContainedWithin(startNode->parentNode(), range.get()))
            startNode = startNode->parentNode();
    }

    applyInlineStyleToNodeRange(style, startNode, pastEndNode);
}

static bool containsNonEditableRegion(Node* node)
{
    if (!node->rendererIsEditable())
        return true;

    Node* sibling = node->traverseNextSibling();
    for (Node* descendent = node->firstChild(); descendent && descendent != sibling; descendent = descendent->traverseNextNode()) {
        if (!descendent->rendererIsEditable())
            return true;
    }

    return false;
}

void ApplyStyleCommand::applyInlineStyleToNodeRange(EditingStyle* style, Node* node, Node* pastEndNode)
{
    if (m_removeOnly)
        return;

    for (RefPtr<Node> next; node && node != pastEndNode; node = next.get()) {
        next = node->traverseNextNode();

        if (!node->renderer() || !node->rendererIsEditable())
            continue;
        
        if (!node->rendererIsRichlyEditable() && node->isHTMLElement()) {
            // This is a plaintext-only region. Only proceed if it's fully selected.
            // pastEndNode is the node after the last fully selected node, so if it's inside node then
            // node isn't fully selected.
            if (pastEndNode && pastEndNode->isDescendantOf(node))
                break;
            // Add to this element's inline style and skip over its contents.
            HTMLElement* element = toHTMLElement(node);
            RefPtr<CSSMutableStyleDeclaration> inlineStyle = element->getInlineStyleDecl()->copy();
            inlineStyle->merge(style->style());
            setNodeAttribute(element, styleAttr, inlineStyle->cssText());
            next = node->traverseNextSibling();
            continue;
        }
        
        if (isBlock(node))
            continue;
        
        if (node->childNodeCount()) {
            if (node->contains(pastEndNode) || containsNonEditableRegion(node) || !node->parentNode()->rendererIsEditable())
                continue;
            if (editingIgnoresContent(node)) {
                next = node->traverseNextSibling();
                continue;
            }
        }

        RefPtr<Node> runStart = node;
        RefPtr<Node> runEnd = node;
        Node* sibling = node->nextSibling();
        while (sibling && sibling != pastEndNode && !sibling->contains(pastEndNode)
               && (!isBlock(sibling) || sibling->hasTagName(brTag))
               && !containsNonEditableRegion(sibling)) {
            runEnd = sibling;
            sibling = runEnd->nextSibling();
        }
        next = runEnd->traverseNextSibling();

        if (!removeStyleFromRunBeforeApplyingStyle(style, runStart, runEnd))
            continue;
        addInlineStyleIfNeeded(style, runStart.get(), runEnd.get(), AddStyledElement);
    }
}

bool ApplyStyleCommand::isStyledInlineElementToRemove(Element* element) const
{
    return (m_styledInlineElement && element->hasTagName(m_styledInlineElement->tagQName()))
        || (m_isInlineElementToRemoveFunction && m_isInlineElementToRemoveFunction(element));
}

bool ApplyStyleCommand::removeStyleFromRunBeforeApplyingStyle(EditingStyle* style, RefPtr<Node>& runStart, RefPtr<Node>& runEnd)
{
    ASSERT(runStart && runEnd && runStart->parentNode() == runEnd->parentNode());
    RefPtr<Node> pastEndNode = runEnd->traverseNextSibling();
    bool needToApplyStyle = false;
    for (Node* node = runStart.get(); node && node != pastEndNode.get(); node = node->traverseNextNode()) {
        if (node->childNodeCount())
            continue;
        // We don't consider m_isInlineElementToRemoveFunction here because we never apply style when m_isInlineElementToRemoveFunction is specified
        if (!style->styleIsPresentInComputedStyleOfNode(node)
            || (m_styledInlineElement && !enclosingNodeWithTag(positionBeforeNode(node), m_styledInlineElement->tagQName()))) {
            needToApplyStyle = true;
            break;
        }
    }
    if (!needToApplyStyle)
        return false;

    RefPtr<Node> next = runStart;
    for (RefPtr<Node> node = next; node && node->inDocument() && node != pastEndNode; node = next) {
        next = node->traverseNextNode();
        if (!node->isHTMLElement())
            continue;

        RefPtr<Node> previousSibling = node->previousSibling();
        RefPtr<Node> nextSibling = node->nextSibling();
        RefPtr<ContainerNode> parent = node->parentNode();
        removeInlineStyleFromElement(style, toHTMLElement(node.get()), RemoveAlways);
        if (!node->inDocument()) {
            // FIXME: We might need to update the start and the end of current selection here but need a test.
            if (runStart == node)
                runStart = previousSibling ? previousSibling->nextSibling() : parent->firstChild();
            if (runEnd == node)
                runEnd = nextSibling ? nextSibling->previousSibling() : parent->lastChild();
        }
    }

    return true;
}

bool ApplyStyleCommand::removeInlineStyleFromElement(EditingStyle* style, PassRefPtr<HTMLElement> element, InlineStyleRemovalMode mode, EditingStyle* extractedStyle)
{
    ASSERT(element);

    if (!element->parentNode() || !element->parentNode()->rendererIsEditable())
        return false;

    if (isStyledInlineElementToRemove(element.get())) {
        if (mode == RemoveNone)
            return true;
        ASSERT(extractedStyle);
        extractedStyle->mergeInlineStyleOfElement(element.get());
        removeNodePreservingChildren(element);
        return true;
    }

    bool removed = false;
    if (removeImplicitlyStyledElement(style, element.get(), mode, extractedStyle))
        removed = true;

    if (!element->inDocument())
        return removed;

    // If the node was converted to a span, the span may still contain relevant
    // styles which must be removed (e.g. <b style='font-weight: bold'>)
    if (removeCSSStyle(style, element.get(), mode, extractedStyle))
        removed = true;

    return removed;
}
    
void ApplyStyleCommand::replaceWithSpanOrRemoveIfWithoutAttributes(HTMLElement*& elem)
{
    bool removeNode = false;

    // Similar to isSpanWithoutAttributesOrUnstyleStyleSpan, but does not look for Apple-style-span.
    NamedNodeMap* attributes = elem->attributes(true); // readonly
    if (!attributes || attributes->isEmpty())
        removeNode = true;
    else if (attributes->length() == 1 && elem->hasAttribute(styleAttr)) {
        // Remove the element even if it has just style='' (this might be redundantly checked later too)
        CSSMutableStyleDeclaration* inlineStyleDecl = elem->inlineStyleDecl();
        if (!inlineStyleDecl || inlineStyleDecl->isEmpty())
            removeNode = true;
    }

    if (removeNode)
        removeNodePreservingChildren(elem);
    else {
        HTMLElement* newSpanElement = replaceElementWithSpanPreservingChildrenAndAttributes(elem);
        ASSERT(newSpanElement && newSpanElement->inDocument());
        elem = newSpanElement;
    }
}
    
bool ApplyStyleCommand::removeImplicitlyStyledElement(EditingStyle* style, HTMLElement* element, InlineStyleRemovalMode mode, EditingStyle* extractedStyle)
{
    ASSERT(style);
    if (mode == RemoveNone) {
        ASSERT(!extractedStyle);
        return style->conflictsWithImplicitStyleOfElement(element) || style->conflictsWithImplicitStyleOfAttributes(element);
    }

    ASSERT(mode == RemoveIfNeeded || mode == RemoveAlways);
    if (style->conflictsWithImplicitStyleOfElement(element, extractedStyle, mode == RemoveAlways ? EditingStyle::ExtractMatchingStyle : EditingStyle::DoNotExtractMatchingStyle)) {
        replaceWithSpanOrRemoveIfWithoutAttributes(element);
        return true;
    }

    // unicode-bidi and direction are pushed down separately so don't push down with other styles
    Vector<QualifiedName> attributes;
    if (!style->extractConflictingImplicitStyleOfAttributes(element, extractedStyle ? EditingStyle::PreserveWritingDirection : EditingStyle::DoNotPreserveWritingDirection,
        extractedStyle, attributes, mode == RemoveAlways ? EditingStyle::ExtractMatchingStyle : EditingStyle::DoNotExtractMatchingStyle))
        return false;

    for (size_t i = 0; i < attributes.size(); i++)
        removeNodeAttribute(element, attributes[i]);

    if (isEmptyFontTag(element) || isSpanWithoutAttributesOrUnstyleStyleSpan(element))
        removeNodePreservingChildren(element);

    return true;
}

bool ApplyStyleCommand::removeCSSStyle(EditingStyle* style, HTMLElement* element, InlineStyleRemovalMode mode, EditingStyle* extractedStyle)
{
    ASSERT(style);
    ASSERT(element);

    if (mode == RemoveNone)
        return style->conflictsWithInlineStyleOfElement(element);

    Vector<CSSPropertyID> properties;
    if (!style->conflictsWithInlineStyleOfElement(element, extractedStyle, properties))
        return false;

    CSSMutableStyleDeclaration* inlineStyle = element->inlineStyleDecl();
    ASSERT(inlineStyle);
    // FIXME: We should use a mass-removal function here but we don't have an undoable one yet.
    for (size_t i = 0; i < properties.size(); i++)
        removeCSSProperty(element, properties[i]);

    // No need to serialize <foo style=""> if we just removed the last css property
    if (inlineStyle->isEmpty())
        removeNodeAttribute(element, styleAttr);

    if (isSpanWithoutAttributesOrUnstyleStyleSpan(element))
        removeNodePreservingChildren(element);

    return true;
}

HTMLElement* ApplyStyleCommand::highestAncestorWithConflictingInlineStyle(EditingStyle* style, Node* node)
{
    if (!node)
        return 0;

    HTMLElement* result = 0;
    Node* unsplittableElement = unsplittableElementForPosition(firstPositionInOrBeforeNode(node));

    for (Node *n = node; n; n = n->parentNode()) {
        if (n->isHTMLElement() && shouldRemoveInlineStyleFromElement(style, toHTMLElement(n)))
            result = toHTMLElement(n);
        // Should stop at the editable root (cannot cross editing boundary) and
        // also stop at the unsplittable element to be consistent with other UAs
        if (n == unsplittableElement)
            break;
    }

    return result;
}

void ApplyStyleCommand::applyInlineStyleToPushDown(Node* node, EditingStyle* style)
{
    ASSERT(node);

    if (!style || style->isEmpty() || !node->renderer())
        return;

    RefPtr<EditingStyle> newInlineStyle = style;
    if (node->isHTMLElement() && static_cast<HTMLElement*>(node)->inlineStyleDecl()) {
        newInlineStyle = style->copy();
        newInlineStyle->mergeInlineStyleOfElement(static_cast<HTMLElement*>(node));
    }

    // Since addInlineStyleIfNeeded can't add styles to block-flow render objects, add style attribute instead.
    // FIXME: applyInlineStyleToRange should be used here instead.
    if ((node->renderer()->isBlockFlow() || node->childNodeCount()) && node->isHTMLElement()) {
        setNodeAttribute(toHTMLElement(node), styleAttr, newInlineStyle->style()->cssText());
        return;
    }

    if (node->renderer()->isText() && static_cast<RenderText*>(node->renderer())->isAllCollapsibleWhitespace())
        return;

    // We can't wrap node with the styled element here because new styled element will never be removed if we did.
    // If we modified the child pointer in pushDownInlineStyleAroundNode to point to new style element
    // then we fall into an infinite loop where we keep removing and adding styled element wrapping node.
    addInlineStyleIfNeeded(newInlineStyle.get(), node, node, DoNotAddStyledElement);
}

void ApplyStyleCommand::pushDownInlineStyleAroundNode(EditingStyle* style, Node* targetNode)
{
    HTMLElement* highestAncestor = highestAncestorWithConflictingInlineStyle(style, targetNode);
    if (!highestAncestor)
        return;

    // The outer loop is traversing the tree vertically from highestAncestor to targetNode
    Node* current = highestAncestor;
    // Along the way, styled elements that contain targetNode are removed and accumulated into elementsToPushDown.
    // Each child of the removed element, exclusing ancestors of targetNode, is then wrapped by clones of elements in elementsToPushDown.
    Vector<RefPtr<Element> > elementsToPushDown;
    while (current != targetNode) {
        ASSERT(current);
        ASSERT(current->contains(targetNode));
        Node* child = current->firstChild();
        Node* lastChild = current->lastChild();
        RefPtr<StyledElement> styledElement;
        if (current->isStyledElement() && isStyledInlineElementToRemove(static_cast<Element*>(current))) {
            styledElement = static_cast<StyledElement*>(current);
            elementsToPushDown.append(styledElement);
        }

        RefPtr<EditingStyle> styleToPushDown = EditingStyle::create();
        if (current->isHTMLElement())
            removeInlineStyleFromElement(style, toHTMLElement(current), RemoveIfNeeded, styleToPushDown.get());

        // The inner loop will go through children on each level
        // FIXME: we should aggregate inline child elements together so that we don't wrap each child separately.
        while (child) {
            Node* nextChild = child->nextSibling();

            if (!child->contains(targetNode) && elementsToPushDown.size()) {
                for (size_t i = 0; i < elementsToPushDown.size(); i++) {
                    RefPtr<Element> wrapper = elementsToPushDown[i]->cloneElementWithoutChildren();
                    ExceptionCode ec = 0;
                    wrapper->removeAttribute(styleAttr, ec);
                    ASSERT(!ec);
                    surroundNodeRangeWithElement(child, child, wrapper);
                }
            }

            // Apply text decoration to all nodes containing targetNode and their siblings but NOT to targetNode
            // But if we've removed styledElement then go ahead and always apply the style.
            if (child != targetNode || styledElement)
                applyInlineStyleToPushDown(child, styleToPushDown.get());

            // We found the next node for the outer loop (contains targetNode)
            // When reached targetNode, stop the outer loop upon the completion of the current inner loop
            if (child == targetNode || child->contains(targetNode))
                current = child;

            if (child == lastChild || child->contains(lastChild))
                break;
            child = nextChild;
        }
    }
}

void ApplyStyleCommand::removeInlineStyle(EditingStyle* style, const Position &start, const Position &end)
{
    ASSERT(start.isNotNull());
    ASSERT(end.isNotNull());
    ASSERT(start.anchorNode()->inDocument());
    ASSERT(end.anchorNode()->inDocument());
    ASSERT(comparePositions(start, end) <= 0);

    Position pushDownStart = start.downstream();
    // If the pushDownStart is at the end of a text node, then this node is not fully selected.
    // Move it to the next deep quivalent position to avoid removing the style from this node.
    // e.g. if pushDownStart was at Position("hello", 5) in <b>hello<div>world</div></b>, we want Position("world", 0) instead.
    Node* pushDownStartContainer = pushDownStart.containerNode();
    if (pushDownStartContainer && pushDownStartContainer->isTextNode()
        && pushDownStart.computeOffsetInContainerNode() == pushDownStartContainer->maxCharacterOffset())
        pushDownStart = nextVisuallyDistinctCandidate(pushDownStart);
    Position pushDownEnd = end.upstream();

    pushDownInlineStyleAroundNode(style, pushDownStart.deprecatedNode());
    pushDownInlineStyleAroundNode(style, pushDownEnd.deprecatedNode());

    // The s and e variables store the positions used to set the ending selection after style removal
    // takes place. This will help callers to recognize when either the start node or the end node
    // are removed from the document during the work of this function.
    // If pushDownInlineStyleAroundNode has pruned start.deprecatedNode() or end.deprecatedNode(),
    // use pushDownStart or pushDownEnd instead, which pushDownInlineStyleAroundNode won't prune.
    Position s = start.isNull() || start.isOrphan() ? pushDownStart : start;
    Position e = end.isNull() || end.isOrphan() ? pushDownEnd : end;

    Node* node = start.deprecatedNode();
    while (node) {
        RefPtr<Node> next = node->traverseNextNode();
        if (node->isHTMLElement() && nodeFullySelected(node, start, end)) {
            RefPtr<HTMLElement> elem = toHTMLElement(node);
            RefPtr<Node> prev = elem->traversePreviousNodePostOrder();
            RefPtr<Node> next = elem->traverseNextNode();
            RefPtr<EditingStyle> styleToPushDown;
            PassRefPtr<Node> childNode = 0;
            if (isStyledInlineElementToRemove(elem.get())) {
                styleToPushDown = EditingStyle::create();
                childNode = elem->firstChild();
            }

            removeInlineStyleFromElement(style, elem.get(), RemoveIfNeeded, styleToPushDown.get());
            if (!elem->inDocument()) {
                if (s.deprecatedNode() == elem) {
                    // Since elem must have been fully selected, and it is at the start
                    // of the selection, it is clear we can set the new s offset to 0.
                    ASSERT(s.anchorType() == Position::PositionIsBeforeAnchor || s.offsetInContainerNode() <= 0);
                    s = firstPositionInOrBeforeNode(next.get());
                }
                if (e.deprecatedNode() == elem) {
                    // Since elem must have been fully selected, and it is at the end
                    // of the selection, it is clear we can set the new e offset to
                    // the max range offset of prev.
                    ASSERT(s.anchorType() == Position::PositionIsAfterAnchor
                           || s.offsetInContainerNode() >= lastOffsetInNode(s.containerNode()));
                    e = lastPositionInOrAfterNode(prev.get());
                }
            }

            if (styleToPushDown) {
                for (; childNode; childNode = childNode->nextSibling())
                    applyInlineStyleToPushDown(childNode.get(), styleToPushDown.get());
            }
        }
        if (node == end.deprecatedNode())
            break;
        node = next.get();
    }

    updateStartEnd(s, e);
}

bool ApplyStyleCommand::nodeFullySelected(Node *node, const Position &start, const Position &end) const
{
    ASSERT(node);
    ASSERT(node->isElementNode());

    return comparePositions(firstPositionInOrBeforeNode(node), start) >= 0
        && comparePositions(lastPositionInOrAfterNode(node).upstream(), end) <= 0;
}

bool ApplyStyleCommand::nodeFullyUnselected(Node *node, const Position &start, const Position &end) const
{
    ASSERT(node);
    ASSERT(node->isElementNode());

    bool isFullyBeforeStart = comparePositions(lastPositionInOrAfterNode(node).upstream(), start) < 0;
    bool isFullyAfterEnd = comparePositions(firstPositionInOrBeforeNode(node), end) > 0;

    return isFullyBeforeStart || isFullyAfterEnd;
}

void ApplyStyleCommand::splitTextAtStart(const Position& start, const Position& end)
{
    ASSERT(start.anchorType() == Position::PositionIsOffsetInAnchor);

    Position newEnd;
    if (end.anchorType() == Position::PositionIsOffsetInAnchor && start.containerNode() == end.containerNode())
        newEnd = Position(end.containerNode(), end.offsetInContainerNode() - start.offsetInContainerNode(), Position::PositionIsOffsetInAnchor);
    else
        newEnd = end;

    Text* text = static_cast<Text*>(start.deprecatedNode());
    splitTextNode(text, start.offsetInContainerNode());
    updateStartEnd(firstPositionInNode(start.deprecatedNode()), newEnd);
}

void ApplyStyleCommand::splitTextAtEnd(const Position& start, const Position& end)
{
    ASSERT(end.anchorType() == Position::PositionIsOffsetInAnchor);

    bool shouldUpdateStart = start.anchorType() == Position::PositionIsOffsetInAnchor && start.containerNode() == end.containerNode();
    Text* text = static_cast<Text *>(end.deprecatedNode());
    splitTextNode(text, end.offsetInContainerNode());

    Node* prevNode = text->previousSibling();
    ASSERT(prevNode);
    Position newStart = shouldUpdateStart ? Position(prevNode, start.offsetInContainerNode(), Position::PositionIsOffsetInAnchor) : start;
    updateStartEnd(newStart, lastPositionInNode(prevNode));
}

void ApplyStyleCommand::splitTextElementAtStart(const Position& start, const Position& end)
{
    ASSERT(start.anchorType() == Position::PositionIsOffsetInAnchor);

    Position newEnd;
    if (end.anchorType() == Position::PositionIsOffsetInAnchor && start.containerNode() == end.containerNode())
        newEnd = Position(end.containerNode(), end.offsetInContainerNode() - start.offsetInContainerNode(), Position::PositionIsOffsetInAnchor);
    else
        newEnd = end;

    Text* text = static_cast<Text*>(start.deprecatedNode());
    splitTextNodeContainingElement(text, start.deprecatedEditingOffset());
    updateStartEnd(Position(start.deprecatedNode()->parentNode(), start.deprecatedNode()->nodeIndex(), Position::PositionIsOffsetInAnchor), newEnd);
}

void ApplyStyleCommand::splitTextElementAtEnd(const Position& start, const Position& end)
{
    ASSERT(end.anchorType() == Position::PositionIsOffsetInAnchor);

    bool shouldUpdateStart = start.anchorType() == Position::PositionIsOffsetInAnchor && start.containerNode() == end.containerNode();
    Text* text = static_cast<Text*>(end.deprecatedNode());
    splitTextNodeContainingElement(text, end.deprecatedEditingOffset());

    Node* prevNode = text->parentNode()->previousSibling()->lastChild();
    ASSERT(prevNode);
    Position newStart = shouldUpdateStart ? Position(prevNode, start.offsetInContainerNode(), Position::PositionIsOffsetInAnchor) : start;
    updateStartEnd(newStart, Position(prevNode->parentNode(), prevNode->nodeIndex() + 1, Position::PositionIsOffsetInAnchor));
}

bool ApplyStyleCommand::shouldSplitTextElement(Element* element, EditingStyle* style)
{
    if (!element || !element->isHTMLElement())
        return false;

    return shouldRemoveInlineStyleFromElement(style, toHTMLElement(element));
}

bool ApplyStyleCommand::isValidCaretPositionInTextNode(const Position& position)
{
    Node* node = position.containerNode();
    if (position.anchorType() != Position::PositionIsOffsetInAnchor || !node->isTextNode())
        return false;
    int offsetInText = position.offsetInContainerNode();
    return offsetInText > caretMinOffset(node) && offsetInText < caretMaxOffset(node);
}

static bool areIdenticalElements(Node *first, Node *second)
{
    // check that tag name and all attribute names and values are identical

    if (!first->isElementNode())
        return false;
    
    if (!second->isElementNode())
        return false;

    Element *firstElement = static_cast<Element *>(first);
    Element *secondElement = static_cast<Element *>(second);
    
    if (!firstElement->tagQName().matches(secondElement->tagQName()))
        return false;

    NamedNodeMap *firstMap = firstElement->attributes();
    NamedNodeMap *secondMap = secondElement->attributes();

    unsigned firstLength = firstMap->length();

    if (firstLength != secondMap->length())
        return false;

    for (unsigned i = 0; i < firstLength; i++) {
        Attribute *attribute = firstMap->attributeItem(i);
        Attribute *secondAttribute = secondMap->getAttributeItem(attribute->name());

        if (!secondAttribute || attribute->value() != secondAttribute->value())
            return false;
    }
    
    return true;
}

bool ApplyStyleCommand::mergeStartWithPreviousIfIdentical(const Position& start, const Position& end)
{
    Node* startNode = start.containerNode();
    int startOffset = start.computeOffsetInContainerNode();
    if (startOffset)
        return false;

    if (isAtomicNode(startNode)) {
        // note: prior siblings could be unrendered elements. it's silly to miss the
        // merge opportunity just for that.
        if (startNode->previousSibling())
            return false;

        startNode = startNode->parentNode();
        startOffset = 0;
    }

    if (!startNode->isElementNode())
        return false;

    Node* previousSibling = startNode->previousSibling();

    if (previousSibling && areIdenticalElements(startNode, previousSibling)) {
        Element* previousElement = static_cast<Element*>(previousSibling);
        Element* element = static_cast<Element*>(startNode);
        Node* startChild = element->firstChild();
        ASSERT(startChild);
        mergeIdenticalElements(previousElement, element);

        int startOffsetAdjustment = startChild->nodeIndex();
        int endOffsetAdjustment = startNode == end.deprecatedNode() ? startOffsetAdjustment : 0;
        updateStartEnd(Position(startNode, startOffsetAdjustment, Position::PositionIsOffsetInAnchor),
                       Position(end.deprecatedNode(), end.deprecatedEditingOffset() + endOffsetAdjustment, Position::PositionIsOffsetInAnchor)); 
        return true;
    }

    return false;
}

bool ApplyStyleCommand::mergeEndWithNextIfIdentical(const Position& start, const Position& end)
{
    Node* endNode = end.containerNode();
    int endOffset = end.computeOffsetInContainerNode();

    if (isAtomicNode(endNode)) {
        if (endOffset < lastOffsetInNode(endNode))
            return false;

        unsigned parentLastOffset = end.deprecatedNode()->parentNode()->childNodes()->length() - 1;
        if (end.deprecatedNode()->nextSibling())
            return false;

        endNode = end.deprecatedNode()->parentNode();
        endOffset = parentLastOffset;
    }

    if (!endNode->isElementNode() || endNode->hasTagName(brTag))
        return false;

    Node* nextSibling = endNode->nextSibling();
    if (nextSibling && areIdenticalElements(endNode, nextSibling)) {
        Element* nextElement = static_cast<Element *>(nextSibling);
        Element* element = static_cast<Element *>(endNode);
        Node* nextChild = nextElement->firstChild();

        mergeIdenticalElements(element, nextElement);

        bool shouldUpdateStart = start.containerNode() == endNode;
        int endOffset = nextChild ? nextChild->nodeIndex() : nextElement->childNodes()->length();
        updateStartEnd(shouldUpdateStart ? Position(nextElement, start.offsetInContainerNode(), Position::PositionIsOffsetInAnchor) : start,
                       Position(nextElement, endOffset, Position::PositionIsOffsetInAnchor));
        return true;
    }

    return false;
}

void ApplyStyleCommand::surroundNodeRangeWithElement(PassRefPtr<Node> passedStartNode, PassRefPtr<Node> endNode, PassRefPtr<Element> elementToInsert)
{
    ASSERT(passedStartNode);
    ASSERT(endNode);
    ASSERT(elementToInsert);
    RefPtr<Node> startNode = passedStartNode;
    RefPtr<Element> element = elementToInsert;

    insertNodeBefore(element, startNode);

    RefPtr<Node> node = startNode;
    while (node) {
        RefPtr<Node> next = node->nextSibling();
        if (node->isContentEditable()) {
            removeNode(node);
            appendNode(node, element);
        }
        if (node == endNode)
            break;
        node = next;
    }

    RefPtr<Node> nextSibling = element->nextSibling();
    RefPtr<Node> previousSibling = element->previousSibling();
    if (nextSibling && nextSibling->isElementNode() && nextSibling->rendererIsEditable()
        && areIdenticalElements(element.get(), static_cast<Element*>(nextSibling.get())))
        mergeIdenticalElements(element.get(), static_cast<Element*>(nextSibling.get()));

    if (previousSibling && previousSibling->isElementNode() && previousSibling->rendererIsEditable()) {
        Node* mergedElement = previousSibling->nextSibling();
        if (mergedElement->isElementNode() && mergedElement->rendererIsEditable()
            && areIdenticalElements(static_cast<Element*>(previousSibling.get()), static_cast<Element*>(mergedElement)))
            mergeIdenticalElements(static_cast<Element*>(previousSibling.get()), static_cast<Element*>(mergedElement));
    }

    // FIXME: We should probably call updateStartEnd if the start or end was in the node
    // range so that the endingSelection() is canonicalized.  See the comments at the end of
    // VisibleSelection::validate().
}

void ApplyStyleCommand::addBlockStyle(const StyleChange& styleChange, HTMLElement* block)
{
    // Do not check for legacy styles here. Those styles, like <B> and <I>, only apply for
    // inline content.
    if (!block)
        return;
        
    String cssText = styleChange.cssStyle();
    CSSMutableStyleDeclaration* decl = block->inlineStyleDecl();
    if (decl)
        cssText += decl->cssText();
    setNodeAttribute(block, styleAttr, cssText);
}

void ApplyStyleCommand::addInlineStyleIfNeeded(EditingStyle* style, PassRefPtr<Node> passedStart, PassRefPtr<Node> passedEnd, EAddStyledElement addStyledElement)
{
    if (!passedStart || !passedEnd || !passedStart->inDocument() || !passedEnd->inDocument())
        return;
    RefPtr<Node> startNode = passedStart;
    RefPtr<Node> endNode = passedEnd;

    // It's okay to obtain the style at the startNode because we've removed all relevant styles from the current run.
    RefPtr<HTMLElement> dummyElement;
    Position positionForStyleComparison;
    if (!startNode->isElementNode()) {
        dummyElement = createStyleSpanElement(document());
        insertNodeAt(dummyElement, positionBeforeNode(startNode.get()));
        positionForStyleComparison = positionBeforeNode(dummyElement.get());
    } else
        positionForStyleComparison = firstPositionInOrBeforeNode(startNode.get());

    StyleChange styleChange(style, positionForStyleComparison);

    if (dummyElement)
        removeNode(dummyElement);

    // Find appropriate font and span elements top-down.
    HTMLElement* fontContainer = 0;
    HTMLElement* styleContainer = 0;
    for (Node* container = startNode.get(); container && startNode == endNode; container = container->firstChild()) {
        if (container->isHTMLElement() && container->hasTagName(fontTag))
            fontContainer = toHTMLElement(container);
        bool styleContainerIsNotSpan = !styleContainer || !styleContainer->hasTagName(spanTag);
        if (container->isHTMLElement() && (container->hasTagName(spanTag) || (styleContainerIsNotSpan && container->childNodeCount())))
            styleContainer = toHTMLElement(container);
        if (!container->firstChild())
            break;
        startNode = container->firstChild();
        endNode = container->lastChild();
    }

    // Font tags need to go outside of CSS so that CSS font sizes override leagcy font sizes.
    if (styleChange.applyFontColor() || styleChange.applyFontFace() || styleChange.applyFontSize()) {
        if (fontContainer) {
            if (styleChange.applyFontColor())
                setNodeAttribute(fontContainer, colorAttr, styleChange.fontColor());
            if (styleChange.applyFontFace())
                setNodeAttribute(fontContainer, faceAttr, styleChange.fontFace());
            if (styleChange.applyFontSize())
                setNodeAttribute(fontContainer, sizeAttr, styleChange.fontSize());
        } else {
            RefPtr<Element> fontElement = createFontElement(document());
            if (styleChange.applyFontColor())
                fontElement->setAttribute(colorAttr, styleChange.fontColor());
            if (styleChange.applyFontFace())
                fontElement->setAttribute(faceAttr, styleChange.fontFace());
            if (styleChange.applyFontSize())
                fontElement->setAttribute(sizeAttr, styleChange.fontSize());
            surroundNodeRangeWithElement(startNode, endNode, fontElement.get());
        }
    }

    if (styleChange.cssStyle().length()) {
        if (styleContainer) {
            CSSMutableStyleDeclaration* existingStyle = toHTMLElement(styleContainer)->inlineStyleDecl();
            if (existingStyle)
                setNodeAttribute(styleContainer, styleAttr, existingStyle->cssText() + styleChange.cssStyle());
            else
                setNodeAttribute(styleContainer, styleAttr, styleChange.cssStyle());
        } else {
            RefPtr<Element> styleElement = createStyleSpanElement(document());
            styleElement->setAttribute(styleAttr, styleChange.cssStyle());
            surroundNodeRangeWithElement(startNode, endNode, styleElement.release());
        }
    }

    if (styleChange.applyBold())
        surroundNodeRangeWithElement(startNode, endNode, createHTMLElement(document(), bTag));

    if (styleChange.applyItalic())
        surroundNodeRangeWithElement(startNode, endNode, createHTMLElement(document(), iTag));

    if (styleChange.applyUnderline())
        surroundNodeRangeWithElement(startNode, endNode, createHTMLElement(document(), uTag));

    if (styleChange.applyLineThrough())
        surroundNodeRangeWithElement(startNode, endNode, createHTMLElement(document(), strikeTag));

    if (styleChange.applySubscript())
        surroundNodeRangeWithElement(startNode, endNode, createHTMLElement(document(), subTag));
    else if (styleChange.applySuperscript())
        surroundNodeRangeWithElement(startNode, endNode, createHTMLElement(document(), supTag));

    if (m_styledInlineElement && addStyledElement == AddStyledElement)
        surroundNodeRangeWithElement(startNode, endNode, m_styledInlineElement->cloneElementWithoutChildren());
}

float ApplyStyleCommand::computedFontSize(Node* node)
{
    if (!node)
        return 0;

    RefPtr<CSSComputedStyleDeclaration> style = computedStyle(node);
    if (!style)
        return 0;

    RefPtr<CSSPrimitiveValue> value = static_pointer_cast<CSSPrimitiveValue>(style->getPropertyCSSValue(CSSPropertyFontSize));
    if (!value)
        return 0;

    return value->getFloatValue(CSSPrimitiveValue::CSS_PX);
}

void ApplyStyleCommand::joinChildTextNodes(Node* node, const Position& start, const Position& end)
{
    if (!node)
        return;

    Position newStart = start;
    Position newEnd = end;

    Node* child = node->firstChild();
    while (child) {
        Node* next = child->nextSibling();
        if (child->isTextNode() && next && next->isTextNode()) {
            Text* childText = static_cast<Text *>(child);
            Text* nextText = static_cast<Text *>(next);
            if (start.anchorType() == Position::PositionIsOffsetInAnchor && next == start.containerNode())
                newStart = Position(childText, childText->length() + start.offsetInContainerNode(), Position::PositionIsOffsetInAnchor);
            if (end.anchorType() == Position::PositionIsOffsetInAnchor && next == end.containerNode())
                newEnd = Position(childText, childText->length() + end.offsetInContainerNode(), Position::PositionIsOffsetInAnchor);
            String textToMove = nextText->data();
            insertTextIntoNode(childText, childText->length(), textToMove);
            removeNode(next);
            // don't move child node pointer. it may want to merge with more text nodes.
        }
        else {
            child = child->nextSibling();
        }
    }

    updateStartEnd(newStart, newEnd);
}

}
