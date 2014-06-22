/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009, 2010, 2011 Google Inc. All rights reserved.
 * Copyright (C) 2011 Igalia S.L.
 * Copyright (C) 2011 Motorola Mobility. All rights reserved.
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
#include "markup.h"

#include "CDATASection.h"
#include "CSSPrimitiveValue.h"
#include "CSSPropertyNames.h"
#include "CSSValue.h"
#include "CSSValueKeywords.h"
#include "ChildListMutationScope.h"
#include "ContextFeatures.h"
#if ENABLE(DELETION_UI)
#include "DeleteButtonController.h"
#endif
#include "DocumentFragment.h"
#include "Editor.h"
#include "ExceptionCode.h"
#include "ExceptionCodePlaceholder.h"
#include "Frame.h"
#include "HTMLBodyElement.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "HTMLTableElement.h"
#include "HTMLTextAreaElement.h"
#include "HTMLTextFormControlElement.h"
#include "KURL.h"
#include "MarkupAccumulator.h"
#include "NodeTraversal.h"
#include "Range.h"
#include "RenderBlock.h"
#include "RenderObject.h"
#include "StylePropertySet.h"
#include "TextIterator.h"
#include "VisibleSelection.h"
#include "VisibleUnits.h"
#include "htmlediting.h"
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringBuilder.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;

static bool propertyMissingOrEqualToNone(StylePropertySet*, CSSPropertyID);

class AttributeChange {
public:
    AttributeChange()
        : m_name(nullAtom, nullAtom, nullAtom)
    {
    }

    AttributeChange(PassRefPtr<Element> element, const QualifiedName& name, const String& value)
        : m_element(element), m_name(name), m_value(value)
    {
    }

    void apply()
    {
        m_element->setAttribute(m_name, m_value);
    }

private:
    RefPtr<Element> m_element;
    QualifiedName m_name;
    String m_value;
};

static void completeURLs(DocumentFragment* fragment, const String& baseURL)
{
    Vector<AttributeChange> changes;

    KURL parsedBaseURL(ParsedURLString, baseURL);

    for (Element* element = ElementTraversal::firstWithin(fragment); element; element = ElementTraversal::next(element, fragment)) {
        if (!element->hasAttributes())
            continue;
        unsigned length = element->attributeCount();
        for (unsigned i = 0; i < length; i++) {
            const Attribute* attribute = element->attributeItem(i);
            if (element->isURLAttribute(*attribute) && !attribute->value().isEmpty())
                changes.append(AttributeChange(element, attribute->name(), KURL(parsedBaseURL, attribute->value()).string()));
        }
    }

    size_t numChanges = changes.size();
    for (size_t i = 0; i < numChanges; ++i)
        changes[i].apply();
}
    
class StyledMarkupAccumulator : public MarkupAccumulator {
public:
    enum RangeFullySelectsNode { DoesFullySelectNode, DoesNotFullySelectNode };

    StyledMarkupAccumulator(Vector<Node*>* nodes, EAbsoluteURLs, EAnnotateForInterchange, const Range*, Node* highestNodeToBeSerialized = 0);
    Node* serializeNodes(Node* startNode, Node* pastEnd);
    virtual void appendString(const String& s) { return MarkupAccumulator::appendString(s); }
    void wrapWithNode(Node*, bool convertBlocksToInlines = false, RangeFullySelectsNode = DoesFullySelectNode);
    void wrapWithStyleNode(StylePropertySet*, Document*, bool isBlock = false);
    String takeResults();

private:
    void appendStyleNodeOpenTag(StringBuilder&, StylePropertySet*, Document*, bool isBlock = false);
    const String& styleNodeCloseTag(bool isBlock = false);
    virtual void appendText(StringBuilder& out, Text*);
    String renderedText(const Node*, const Range*);
    String stringValueForRange(const Node*, const Range*);
    void appendElement(StringBuilder& out, Element*, bool addDisplayInline, RangeFullySelectsNode);
    void appendElement(StringBuilder& out, Element* element, Namespaces*) { appendElement(out, element, false, DoesFullySelectNode); }

    enum NodeTraversalMode { EmitString, DoNotEmitString };
    Node* traverseNodesForSerialization(Node* startNode, Node* pastEnd, NodeTraversalMode);

    bool shouldAnnotate() { return m_shouldAnnotate == AnnotateForInterchange; }
    bool shouldApplyWrappingStyle(Node* node) const
    {
        return m_highestNodeToBeSerialized && m_highestNodeToBeSerialized->parentNode() == node->parentNode()
            && m_wrappingStyle && m_wrappingStyle->style();
    }

    Vector<String> m_reversedPrecedingMarkup;
    const EAnnotateForInterchange m_shouldAnnotate;
    Node* m_highestNodeToBeSerialized;
    RefPtr<EditingStyle> m_wrappingStyle;
};

inline StyledMarkupAccumulator::StyledMarkupAccumulator(Vector<Node*>* nodes, EAbsoluteURLs shouldResolveURLs, EAnnotateForInterchange shouldAnnotate,
    const Range* range, Node* highestNodeToBeSerialized)
    : MarkupAccumulator(nodes, shouldResolveURLs, range)
    , m_shouldAnnotate(shouldAnnotate)
    , m_highestNodeToBeSerialized(highestNodeToBeSerialized)
{
}

void StyledMarkupAccumulator::wrapWithNode(Node* node, bool convertBlocksToInlines, RangeFullySelectsNode rangeFullySelectsNode)
{
    StringBuilder markup;
    if (node->isElementNode())
        appendElement(markup, toElement(node), convertBlocksToInlines && isBlock(const_cast<Node*>(node)), rangeFullySelectsNode);
    else
        appendStartMarkup(markup, node, 0);
    m_reversedPrecedingMarkup.append(markup.toString());
    appendEndTag(node);
    if (m_nodes)
        m_nodes->append(node);
}

void StyledMarkupAccumulator::wrapWithStyleNode(StylePropertySet* style, Document* document, bool isBlock)
{
    StringBuilder openTag;
    appendStyleNodeOpenTag(openTag, style, document, isBlock);
    m_reversedPrecedingMarkup.append(openTag.toString());
    appendString(styleNodeCloseTag(isBlock));
}

void StyledMarkupAccumulator::appendStyleNodeOpenTag(StringBuilder& out, StylePropertySet* style, Document* document, bool isBlock)
{
    // wrappingStyleForSerialization should have removed -webkit-text-decorations-in-effect
    ASSERT(propertyMissingOrEqualToNone(style, CSSPropertyWebkitTextDecorationsInEffect));
    if (isBlock)
        out.appendLiteral("<div style=\"");
    else
        out.appendLiteral("<span style=\"");
    appendAttributeValue(out, style->asText(), document->isHTMLDocument());
    out.appendLiteral("\">");
}

const String& StyledMarkupAccumulator::styleNodeCloseTag(bool isBlock)
{
    DEFINE_STATIC_LOCAL(const String, divClose, (ASCIILiteral("</div>")));
    DEFINE_STATIC_LOCAL(const String, styleSpanClose, (ASCIILiteral("</span>")));
    return isBlock ? divClose : styleSpanClose;
}

String StyledMarkupAccumulator::takeResults()
{
    StringBuilder result;
    result.reserveCapacity(totalLength(m_reversedPrecedingMarkup) + length());

    for (size_t i = m_reversedPrecedingMarkup.size(); i > 0; --i)
        result.append(m_reversedPrecedingMarkup[i - 1]);

    concatenateMarkup(result);

    // We remove '\0' characters because they are not visibly rendered to the user.
    return result.toString().replaceWithLiteral('\0', "");
}

void StyledMarkupAccumulator::appendText(StringBuilder& out, Text* text)
{    
    const bool parentIsTextarea = text->parentElement() && isHTMLTextAreaElement(text->parentElement());
    const bool wrappingSpan = shouldApplyWrappingStyle(text) && !parentIsTextarea;
    if (wrappingSpan) {
        RefPtr<EditingStyle> wrappingStyle = m_wrappingStyle->copy();
        // FIXME: <rdar://problem/5371536> Style rules that match pasted content can change it's appearance
        // Make sure spans are inline style in paste side e.g. span { display: block }.
        wrappingStyle->forceInline();
        // FIXME: Should this be included in forceInline?
        wrappingStyle->style()->setProperty(CSSPropertyFloat, CSSValueNone);

        appendStyleNodeOpenTag(out, wrappingStyle->style(), text->document());
    }

    if (!shouldAnnotate() || parentIsTextarea)
        MarkupAccumulator::appendText(out, text);
    else {
        const bool useRenderedText = !enclosingNodeWithTag(firstPositionInNode(text), selectTag);
        String content = useRenderedText ? renderedText(text, m_range) : stringValueForRange(text, m_range);
        StringBuilder buffer;
        appendCharactersReplacingEntities(buffer, content, 0, content.length(), EntityMaskInPCDATA);
        out.append(convertHTMLTextToInterchangeFormat(buffer.toString(), text));
    }

    if (wrappingSpan)
        out.append(styleNodeCloseTag());
}
    
String StyledMarkupAccumulator::renderedText(const Node* node, const Range* range)
{
    if (!node->isTextNode())
        return String();

    const Text* textNode = static_cast<const Text*>(node);
    unsigned startOffset = 0;
    unsigned endOffset = textNode->length();

    if (range && node == range->startContainer())
        startOffset = range->startOffset();
    if (range && node == range->endContainer())
        endOffset = range->endOffset();

    Position start = createLegacyEditingPosition(const_cast<Node*>(node), startOffset);
    Position end = createLegacyEditingPosition(const_cast<Node*>(node), endOffset);
    return plainText(Range::create(node->document(), start, end).get());
}

String StyledMarkupAccumulator::stringValueForRange(const Node* node, const Range* range)
{
    if (!range)
        return node->nodeValue();

    String str = node->nodeValue();
    if (node == range->endContainer())
        str.truncate(range->endOffset());
    if (node == range->startContainer())
        str.remove(0, range->startOffset());
    return str;
}

void StyledMarkupAccumulator::appendElement(StringBuilder& out, Element* element, bool addDisplayInline, RangeFullySelectsNode rangeFullySelectsNode)
{
    const bool documentIsHTML = element->document()->isHTMLDocument();
    appendOpenTag(out, element, 0);

    const unsigned length = element->hasAttributes() ? element->attributeCount() : 0;
    const bool shouldAnnotateOrForceInline = element->isHTMLElement() && (shouldAnnotate() || addDisplayInline);
    const bool shouldOverrideStyleAttr = shouldAnnotateOrForceInline || shouldApplyWrappingStyle(element);
    for (unsigned i = 0; i < length; ++i) {
        const Attribute* attribute = element->attributeItem(i);
        // We'll handle the style attribute separately, below.
        if (attribute->name() == styleAttr && shouldOverrideStyleAttr)
            continue;
        appendAttribute(out, element, *attribute, 0);
    }

    if (shouldOverrideStyleAttr) {
        RefPtr<EditingStyle> newInlineStyle;

        if (shouldApplyWrappingStyle(element)) {
            newInlineStyle = m_wrappingStyle->copy();
            newInlineStyle->removePropertiesInElementDefaultStyle(element);
            newInlineStyle->removeStyleConflictingWithStyleOfNode(element);
        } else
            newInlineStyle = EditingStyle::create();

        if (element->isStyledElement() && static_cast<StyledElement*>(element)->inlineStyle())
            newInlineStyle->overrideWithStyle(static_cast<StyledElement*>(element)->inlineStyle());

        if (shouldAnnotateOrForceInline) {
            if (shouldAnnotate())
                newInlineStyle->mergeStyleFromRulesForSerialization(toHTMLElement(element));

            if (addDisplayInline)
                newInlineStyle->forceInline();

            // If the node is not fully selected by the range, then we don't want to keep styles that affect its relationship to the nodes around it
            // only the ones that affect it and the nodes within it.
            if (rangeFullySelectsNode == DoesNotFullySelectNode && newInlineStyle->style())
                newInlineStyle->style()->removeProperty(CSSPropertyFloat);
        }

        if (!newInlineStyle->isEmpty()) {
            out.appendLiteral(" style=\"");
            appendAttributeValue(out, newInlineStyle->style()->asText(), documentIsHTML);
            out.append('\"');
        }
    }

    appendCloseTag(out, element);
}

Node* StyledMarkupAccumulator::serializeNodes(Node* startNode, Node* pastEnd)
{
    if (!m_highestNodeToBeSerialized) {
        Node* lastClosed = traverseNodesForSerialization(startNode, pastEnd, DoNotEmitString);
        m_highestNodeToBeSerialized = lastClosed;
    }

    if (m_highestNodeToBeSerialized && m_highestNodeToBeSerialized->parentNode())
        m_wrappingStyle = EditingStyle::wrappingStyleForSerialization(m_highestNodeToBeSerialized->parentNode(), shouldAnnotate());

    return traverseNodesForSerialization(startNode, pastEnd, EmitString);
}

Node* StyledMarkupAccumulator::traverseNodesForSerialization(Node* startNode, Node* pastEnd, NodeTraversalMode traversalMode)
{
    const bool shouldEmit = traversalMode == EmitString;
    Vector<Node*> ancestorsToClose;
    Node* next;
    Node* lastClosed = 0;
    for (Node* n = startNode; n != pastEnd; n = next) {
        // According to <rdar://problem/5730668>, it is possible for n to blow
        // past pastEnd and become null here. This shouldn't be possible.
        // This null check will prevent crashes (but create too much markup)
        // and the ASSERT will hopefully lead us to understanding the problem.
        ASSERT(n);
        if (!n)
            break;
        
        next = NodeTraversal::next(n);
        bool openedTag = false;

        if (isBlock(n) && canHaveChildrenForEditing(n) && next == pastEnd)
            // Don't write out empty block containers that aren't fully selected.
            continue;

        if (!n->renderer() && !enclosingNodeWithTag(firstPositionInOrBeforeNode(n), selectTag)) {
            next = NodeTraversal::nextSkippingChildren(n);
            // Don't skip over pastEnd.
            if (pastEnd && pastEnd->isDescendantOf(n))
                next = pastEnd;
        } else {
            // Add the node to the markup if we're not skipping the descendants
            if (shouldEmit)
                appendStartTag(n);

            // If node has no children, close the tag now.
            if (!n->childNodeCount()) {
                if (shouldEmit)
                    appendEndTag(n);
                lastClosed = n;
            } else {
                openedTag = true;
                ancestorsToClose.append(n);
            }
        }

        // If we didn't insert open tag and there's no more siblings or we're at the end of the traversal, take care of ancestors.
        // FIXME: What happens if we just inserted open tag and reached the end?
        if (!openedTag && (!n->nextSibling() || next == pastEnd)) {
            // Close up the ancestors.
            while (!ancestorsToClose.isEmpty()) {
                Node* ancestor = ancestorsToClose.last();
                if (next != pastEnd && next->isDescendantOf(ancestor))
                    break;
                // Not at the end of the range, close ancestors up to sibling of next node.
                if (shouldEmit)
                    appendEndTag(ancestor);
                lastClosed = ancestor;
                ancestorsToClose.removeLast();
            }

            // Surround the currently accumulated markup with markup for ancestors we never opened as we leave the subtree(s) rooted at those ancestors.
            ContainerNode* nextParent = next ? next->parentNode() : 0;
            if (next != pastEnd && n != nextParent) {
                Node* lastAncestorClosedOrSelf = n->isDescendantOf(lastClosed) ? lastClosed : n;
                for (ContainerNode* parent = lastAncestorClosedOrSelf->parentNode(); parent && parent != nextParent; parent = parent->parentNode()) {
                    // All ancestors that aren't in the ancestorsToClose list should either be a) unrendered:
                    if (!parent->renderer())
                        continue;
                    // or b) ancestors that we never encountered during a pre-order traversal starting at startNode:
                    ASSERT(startNode->isDescendantOf(parent));
                    if (shouldEmit)
                        wrapWithNode(parent);
                    lastClosed = parent;
                }
            }
        }
    }

    return lastClosed;
}

static bool isHTMLBlockElement(const Node* node)
{
    return node->hasTagName(tdTag)
        || node->hasTagName(thTag)
        || isNonTableCellHTMLBlockElement(node);
}

static Node* ancestorToRetainStructureAndAppearanceForBlock(Node* commonAncestorBlock)
{
    if (!commonAncestorBlock)
        return 0;

    if (commonAncestorBlock->hasTagName(tbodyTag) || commonAncestorBlock->hasTagName(trTag)) {
        ContainerNode* table = commonAncestorBlock->parentNode();
        while (table && !isHTMLTableElement(table))
            table = table->parentNode();

        return table;
    }

    if (isNonTableCellHTMLBlockElement(commonAncestorBlock))
        return commonAncestorBlock;

    return 0;
}

static inline Node* ancestorToRetainStructureAndAppearance(Node* commonAncestor)
{
    return ancestorToRetainStructureAndAppearanceForBlock(enclosingBlock(commonAncestor));
}

static inline Node* ancestorToRetainStructureAndAppearanceWithNoRenderer(Node* commonAncestor)
{
    Node* commonAncestorBlock = enclosingNodeOfType(firstPositionInOrBeforeNode(commonAncestor), isHTMLBlockElement);
    return ancestorToRetainStructureAndAppearanceForBlock(commonAncestorBlock);
}

static bool propertyMissingOrEqualToNone(StylePropertySet* style, CSSPropertyID propertyID)
{
    if (!style)
        return false;
    RefPtr<CSSValue> value = style->getPropertyCSSValue(propertyID);
    if (!value)
        return true;
    if (!value->isPrimitiveValue())
        return false;
    return static_cast<CSSPrimitiveValue*>(value.get())->getValueID() == CSSValueNone;
}

static bool needInterchangeNewlineAfter(const VisiblePosition& v)
{
    VisiblePosition next = v.next();
    Node* upstreamNode = next.deepEquivalent().upstream().deprecatedNode();
    Node* downstreamNode = v.deepEquivalent().downstream().deprecatedNode();
    // Add an interchange newline if a paragraph break is selected and a br won't already be added to the markup to represent it.
    return isEndOfParagraph(v) && isStartOfParagraph(next) && !(upstreamNode->hasTagName(brTag) && upstreamNode == downstreamNode);
}

static PassRefPtr<EditingStyle> styleFromMatchedRulesAndInlineDecl(const Node* node)
{
    if (!node->isHTMLElement())
        return 0;

    // FIXME: Having to const_cast here is ugly, but it is quite a bit of work to untangle
    // the non-const-ness of styleFromMatchedRulesForElement.
    HTMLElement* element = const_cast<HTMLElement*>(static_cast<const HTMLElement*>(node));
    RefPtr<EditingStyle> style = EditingStyle::create(element->inlineStyle());
    style->mergeStyleFromRules(element);
    return style.release();
}

static bool isElementPresentational(const Node* node)
{
    return node->hasTagName(uTag) || node->hasTagName(sTag) || node->hasTagName(strikeTag)
        || node->hasTagName(iTag) || node->hasTagName(emTag) || node->hasTagName(bTag) || node->hasTagName(strongTag);
}

static Node* highestAncestorToWrapMarkup(const Range* range, EAnnotateForInterchange shouldAnnotate)
{
    Node* commonAncestor = range->commonAncestorContainer(IGNORE_EXCEPTION);
    ASSERT(commonAncestor);
    Node* specialCommonAncestor = 0;
    if (shouldAnnotate == AnnotateForInterchange) {
        // Include ancestors that aren't completely inside the range but are required to retain 
        // the structure and appearance of the copied markup.
        specialCommonAncestor = ancestorToRetainStructureAndAppearance(commonAncestor);

        if (Node* parentListNode = enclosingNodeOfType(firstPositionInOrBeforeNode(range->firstNode()), isListItem)) {
            if (WebCore::areRangesEqual(VisibleSelection::selectionFromContentsOfNode(parentListNode).toNormalizedRange().get(), range)) {
                specialCommonAncestor = parentListNode->parentNode();
                while (specialCommonAncestor && !isListElement(specialCommonAncestor))
                    specialCommonAncestor = specialCommonAncestor->parentNode();
            }
        }

        // Retain the Mail quote level by including all ancestor mail block quotes.
        if (Node* highestMailBlockquote = highestEnclosingNodeOfType(firstPositionInOrBeforeNode(range->firstNode()), isMailBlockquote, CanCrossEditingBoundary))
            specialCommonAncestor = highestMailBlockquote;
    }

    Node* checkAncestor = specialCommonAncestor ? specialCommonAncestor : commonAncestor;
    if (checkAncestor->renderer() && checkAncestor->renderer()->containingBlock()) {
        Node* newSpecialCommonAncestor = highestEnclosingNodeOfType(firstPositionInNode(checkAncestor), &isElementPresentational, CanCrossEditingBoundary, checkAncestor->renderer()->containingBlock()->node());
        if (newSpecialCommonAncestor)
            specialCommonAncestor = newSpecialCommonAncestor;
    }

    // If a single tab is selected, commonAncestor will be a text node inside a tab span.
    // If two or more tabs are selected, commonAncestor will be the tab span.
    // In either case, if there is a specialCommonAncestor already, it will necessarily be above 
    // any tab span that needs to be included.
    if (!specialCommonAncestor && isTabSpanTextNode(commonAncestor))
        specialCommonAncestor = commonAncestor->parentNode();
    if (!specialCommonAncestor && isTabSpanNode(commonAncestor))
        specialCommonAncestor = commonAncestor;

    if (Node *enclosingAnchor = enclosingNodeWithTag(firstPositionInNode(specialCommonAncestor ? specialCommonAncestor : commonAncestor), aTag))
        specialCommonAncestor = enclosingAnchor;

    return specialCommonAncestor;
}

// FIXME: Shouldn't we omit style info when annotate == DoNotAnnotateForInterchange? 
// FIXME: At least, annotation and style info should probably not be included in range.markupString()
static String createMarkupInternal(Document* document, const Range* range, const Range* updatedRange, Vector<Node*>* nodes,
    EAnnotateForInterchange shouldAnnotate, bool convertBlocksToInlines, EAbsoluteURLs shouldResolveURLs)
{
    ASSERT(document);
    ASSERT(range);
    ASSERT(updatedRange);
    DEFINE_STATIC_LOCAL(const String, interchangeNewlineString, (ASCIILiteral("<br class=\"" AppleInterchangeNewline "\">")));

    bool collapsed = updatedRange->collapsed(ASSERT_NO_EXCEPTION);
    if (collapsed)
        return emptyString();
    Node* commonAncestor = updatedRange->commonAncestorContainer(ASSERT_NO_EXCEPTION);
    if (!commonAncestor)
        return emptyString();

    document->updateLayoutIgnorePendingStylesheets();

    Node* body = enclosingNodeWithTag(firstPositionInNode(commonAncestor), bodyTag);
    Node* fullySelectedRoot = 0;
    // FIXME: Do this for all fully selected blocks, not just the body.
    if (body && areRangesEqual(VisibleSelection::selectionFromContentsOfNode(body).toNormalizedRange().get(), range))
        fullySelectedRoot = body;
    Node* specialCommonAncestor = highestAncestorToWrapMarkup(updatedRange, shouldAnnotate);
    StyledMarkupAccumulator accumulator(nodes, shouldResolveURLs, shouldAnnotate, updatedRange, specialCommonAncestor);
    Node* pastEnd = updatedRange->pastLastNode();

    Node* startNode = updatedRange->firstNode();
    VisiblePosition visibleStart(updatedRange->startPosition(), VP_DEFAULT_AFFINITY);
    VisiblePosition visibleEnd(updatedRange->endPosition(), VP_DEFAULT_AFFINITY);
    if (shouldAnnotate == AnnotateForInterchange && needInterchangeNewlineAfter(visibleStart)) {
        if (visibleStart == visibleEnd.previous())
            return interchangeNewlineString;

        accumulator.appendString(interchangeNewlineString);
        startNode = visibleStart.next().deepEquivalent().deprecatedNode();

        if (pastEnd && Range::compareBoundaryPoints(startNode, 0, pastEnd, 0, ASSERT_NO_EXCEPTION) >= 0)
            return interchangeNewlineString;
    }

    Node* lastClosed = accumulator.serializeNodes(startNode, pastEnd);

    if (specialCommonAncestor && lastClosed) {
        // Also include all of the ancestors of lastClosed up to this special ancestor.
        for (ContainerNode* ancestor = lastClosed->parentNode(); ancestor; ancestor = ancestor->parentNode()) {
            if (ancestor == fullySelectedRoot && !convertBlocksToInlines) {
                RefPtr<EditingStyle> fullySelectedRootStyle = styleFromMatchedRulesAndInlineDecl(fullySelectedRoot);

                // Bring the background attribute over, but not as an attribute because a background attribute on a div
                // appears to have no effect.
                if ((!fullySelectedRootStyle || !fullySelectedRootStyle->style() || !fullySelectedRootStyle->style()->getPropertyCSSValue(CSSPropertyBackgroundImage))
                    && toElement(fullySelectedRoot)->hasAttribute(backgroundAttr))
                    fullySelectedRootStyle->style()->setProperty(CSSPropertyBackgroundImage, "url('" + toElement(fullySelectedRoot)->getAttribute(backgroundAttr) + "')");

                if (fullySelectedRootStyle->style()) {
                    // Reset the CSS properties to avoid an assertion error in addStyleMarkup().
                    // This assertion is caused at least when we select all text of a <body> element whose
                    // 'text-decoration' property is "inherit", and copy it.
                    if (!propertyMissingOrEqualToNone(fullySelectedRootStyle->style(), CSSPropertyTextDecoration))
                        fullySelectedRootStyle->style()->setProperty(CSSPropertyTextDecoration, CSSValueNone);
                    if (!propertyMissingOrEqualToNone(fullySelectedRootStyle->style(), CSSPropertyWebkitTextDecorationsInEffect))
                        fullySelectedRootStyle->style()->setProperty(CSSPropertyWebkitTextDecorationsInEffect, CSSValueNone);
                    accumulator.wrapWithStyleNode(fullySelectedRootStyle->style(), document, true);
                }
            } else {
                // Since this node and all the other ancestors are not in the selection we want to set RangeFullySelectsNode to DoesNotFullySelectNode
                // so that styles that affect the exterior of the node are not included.
                accumulator.wrapWithNode(ancestor, convertBlocksToInlines, StyledMarkupAccumulator::DoesNotFullySelectNode);
            }
            if (nodes)
                nodes->append(ancestor);
            
            lastClosed = ancestor;
            
            if (ancestor == specialCommonAncestor)
                break;
        }
    }

    // FIXME: The interchange newline should be placed in the block that it's in, not after all of the content, unconditionally.
    if (shouldAnnotate == AnnotateForInterchange && needInterchangeNewlineAfter(visibleEnd.previous()))
        accumulator.appendString(interchangeNewlineString);

    return accumulator.takeResults();
}

String createMarkup(const Range* range, Vector<Node*>* nodes, EAnnotateForInterchange shouldAnnotate, bool convertBlocksToInlines, EAbsoluteURLs shouldResolveURLs)
{
    if (!range)
        return emptyString();

    Document* document = range->ownerDocument();
    if (!document)
        return emptyString();

    const Range* updatedRange = range;

#if ENABLE(DELETION_UI)
    // Disable the delete button so it's elements are not serialized into the markup,
    // but make sure neither endpoint is inside the delete user interface.
    Frame* frame = document->frame();
    DeleteButtonControllerDisableScope deleteButtonControllerDisableScope(frame);

    RefPtr<Range> updatedRangeRef;
    if (frame) {
        updatedRangeRef = frame->editor().avoidIntersectionWithDeleteButtonController(range);
        updatedRange = updatedRangeRef.get();
        if (!updatedRange)
            return emptyString();
    }
#endif

    return createMarkupInternal(document, range, updatedRange, nodes, shouldAnnotate, convertBlocksToInlines, shouldResolveURLs);
}

PassRefPtr<DocumentFragment> createFragmentFromMarkup(Document* document, const String& markup, const String& baseURL, ParserContentPolicy parserContentPolicy)
{
    // We use a fake body element here to trick the HTML parser to using the InBody insertion mode.
    RefPtr<HTMLBodyElement> fakeBody = HTMLBodyElement::create(document);
    RefPtr<DocumentFragment> fragment = DocumentFragment::create(document);

    fragment->parseHTML(markup, fakeBody.get(), parserContentPolicy);

    if (!baseURL.isEmpty() && baseURL != blankURL() && baseURL != document->baseURL())
        completeURLs(fragment.get(), baseURL);

    return fragment.release();
}

static const char fragmentMarkerTag[] = "webkit-fragment-marker";

static bool findNodesSurroundingContext(Document* document, RefPtr<Node>& nodeBeforeContext, RefPtr<Node>& nodeAfterContext)
{
    for (Node* node = document->firstChild(); node; node = NodeTraversal::next(node)) {
        if (node->nodeType() == Node::COMMENT_NODE && static_cast<CharacterData*>(node)->data() == fragmentMarkerTag) {
            if (!nodeBeforeContext)
                nodeBeforeContext = node;
            else {
                nodeAfterContext = node;
                return true;
            }
        }
    }
    return false;
}

static void trimFragment(DocumentFragment* fragment, Node* nodeBeforeContext, Node* nodeAfterContext)
{
    RefPtr<Node> next;
    for (RefPtr<Node> node = fragment->firstChild(); node; node = next) {
        if (nodeBeforeContext->isDescendantOf(node.get())) {
            next = NodeTraversal::next(node.get());
            continue;
        }
        next = NodeTraversal::nextSkippingChildren(node.get());
        ASSERT(!node->contains(nodeAfterContext));
        node->parentNode()->removeChild(node.get(), ASSERT_NO_EXCEPTION);
        if (nodeBeforeContext == node)
            break;
    }

    ASSERT(nodeAfterContext->parentNode());
    for (RefPtr<Node> node = nodeAfterContext; node; node = next) {
        next = NodeTraversal::nextSkippingChildren(node.get());
        node->parentNode()->removeChild(node.get(), ASSERT_NO_EXCEPTION);
    }
}

PassRefPtr<DocumentFragment> createFragmentFromMarkupWithContext(Document* document, const String& markup, unsigned fragmentStart, unsigned fragmentEnd,
    const String& baseURL, ParserContentPolicy parserContentPolicy)
{
    // FIXME: Need to handle the case where the markup already contains these markers.

    StringBuilder taggedMarkup;
    taggedMarkup.append(markup.left(fragmentStart));
    MarkupAccumulator::appendComment(taggedMarkup, fragmentMarkerTag);
    taggedMarkup.append(markup.substring(fragmentStart, fragmentEnd - fragmentStart));
    MarkupAccumulator::appendComment(taggedMarkup, fragmentMarkerTag);
    taggedMarkup.append(markup.substring(fragmentEnd));

    RefPtr<DocumentFragment> taggedFragment = createFragmentFromMarkup(document, taggedMarkup.toString(), baseURL, parserContentPolicy);
    RefPtr<Document> taggedDocument = Document::create(0, KURL());
    taggedDocument->setContextFeatures(document->contextFeatures());
    taggedDocument->takeAllChildrenFrom(taggedFragment.get());

    RefPtr<Node> nodeBeforeContext;
    RefPtr<Node> nodeAfterContext;
    if (!findNodesSurroundingContext(taggedDocument.get(), nodeBeforeContext, nodeAfterContext))
        return 0;

    RefPtr<Range> range = Range::create(taggedDocument.get(),
        positionAfterNode(nodeBeforeContext.get()).parentAnchoredEquivalent(),
        positionBeforeNode(nodeAfterContext.get()).parentAnchoredEquivalent());

    Node* commonAncestor = range->commonAncestorContainer(ASSERT_NO_EXCEPTION);
    Node* specialCommonAncestor = ancestorToRetainStructureAndAppearanceWithNoRenderer(commonAncestor);

    // When there's a special common ancestor outside of the fragment, we must include it as well to
    // preserve the structure and appearance of the fragment. For example, if the fragment contains
    // TD, we need to include the enclosing TABLE tag as well.
    RefPtr<DocumentFragment> fragment = DocumentFragment::create(document);
    if (specialCommonAncestor)
        fragment->appendChild(specialCommonAncestor, ASSERT_NO_EXCEPTION);
    else
        fragment->takeAllChildrenFrom(toContainerNode(commonAncestor));

    trimFragment(fragment.get(), nodeBeforeContext.get(), nodeAfterContext.get());

    return fragment;
}

String createMarkup(const Node* node, EChildrenOnly childrenOnly, Vector<Node*>* nodes, EAbsoluteURLs shouldResolveURLs, Vector<QualifiedName>* tagNamesToSkip, EFragmentSerialization fragmentSerialization)
{
    if (!node)
        return "";

    HTMLElement* deleteButtonContainerElement = 0;
#if ENABLE(DELETION_UI)
    if (Frame* frame = node->document()->frame()) {
        deleteButtonContainerElement = frame->editor().deleteButtonController()->containerElement();
        if (node->isDescendantOf(deleteButtonContainerElement))
            return "";
    }
#endif
    MarkupAccumulator accumulator(nodes, shouldResolveURLs, 0, fragmentSerialization);
    return accumulator.serializeNodes(const_cast<Node*>(node), deleteButtonContainerElement, childrenOnly, tagNamesToSkip);
}

static void fillContainerFromString(ContainerNode* paragraph, const String& string)
{
    Document* document = paragraph->document();

    if (string.isEmpty()) {
        paragraph->appendChild(createBlockPlaceholderElement(document), ASSERT_NO_EXCEPTION);
        return;
    }

    ASSERT(string.find('\n') == notFound);

    Vector<String> tabList;
    string.split('\t', true, tabList);
    String tabText = emptyString();
    bool first = true;
    size_t numEntries = tabList.size();
    for (size_t i = 0; i < numEntries; ++i) {
        const String& s = tabList[i];

        // append the non-tab textual part
        if (!s.isEmpty()) {
            if (!tabText.isEmpty()) {
                paragraph->appendChild(createTabSpanElement(document, tabText), ASSERT_NO_EXCEPTION);
                tabText = emptyString();
            }
            RefPtr<Node> textNode = document->createTextNode(stringWithRebalancedWhitespace(s, first, i + 1 == numEntries));
            paragraph->appendChild(textNode.release(), ASSERT_NO_EXCEPTION);
        }

        // there is a tab after every entry, except the last entry
        // (if the last character is a tab, the list gets an extra empty entry)
        if (i + 1 != numEntries)
            tabText.append('\t');
        else if (!tabText.isEmpty())
            paragraph->appendChild(createTabSpanElement(document, tabText), ASSERT_NO_EXCEPTION);

        first = false;
    }
}

bool isPlainTextMarkup(Node *node)
{
    if (!node->isElementNode() || !node->hasTagName(divTag) || toElement(node)->hasAttributes())
        return false;
    
    if (node->childNodeCount() == 1 && (node->firstChild()->isTextNode() || (node->firstChild()->firstChild())))
        return true;
    
    return (node->childNodeCount() == 2 && isTabSpanTextNode(node->firstChild()->firstChild()) && node->firstChild()->nextSibling()->isTextNode());
}

PassRefPtr<DocumentFragment> createFragmentFromText(Range* context, const String& text)
{
    if (!context)
        return 0;

    Node* styleNode = context->firstNode();
    if (!styleNode) {
        styleNode = context->startPosition().deprecatedNode();
        if (!styleNode)
            return 0;
    }

    Document* document = styleNode->document();
    RefPtr<DocumentFragment> fragment = document->createDocumentFragment();
    
    if (text.isEmpty())
        return fragment.release();

    String string = text;
    string.replace("\r\n", "\n");
    string.replace('\r', '\n');

    RenderObject* renderer = styleNode->renderer();
    if (renderer && renderer->style()->preserveNewline()) {
        fragment->appendChild(document->createTextNode(string), ASSERT_NO_EXCEPTION);
        if (string.endsWith('\n')) {
            RefPtr<Element> element = createBreakElement(document);
            element->setAttribute(classAttr, AppleInterchangeNewline);            
            fragment->appendChild(element.release(), ASSERT_NO_EXCEPTION);
        }
        return fragment.release();
    }

    // A string with no newlines gets added inline, rather than being put into a paragraph.
    if (string.find('\n') == notFound) {
        fillContainerFromString(fragment.get(), string);
        return fragment.release();
    }

    // Break string into paragraphs. Extra line breaks turn into empty paragraphs.
    Node* blockNode = enclosingBlock(context->firstNode());
    Element* block = toElement(blockNode);
    bool useClonesOfEnclosingBlock = blockNode
        && blockNode->isElementNode()
        && !block->hasTagName(bodyTag)
        && !block->hasTagName(htmlTag)
        && block != editableRootForPosition(context->startPosition());
    bool useLineBreak = enclosingTextFormControl(context->startPosition());

    Vector<String> list;
    string.split('\n', true, list); // true gets us empty strings in the list
    size_t numLines = list.size();
    for (size_t i = 0; i < numLines; ++i) {
        const String& s = list[i];

        RefPtr<Element> element;
        if (s.isEmpty() && i + 1 == numLines) {
            // For last line, use the "magic BR" rather than a P.
            element = createBreakElement(document);
            element->setAttribute(classAttr, AppleInterchangeNewline);
        } else if (useLineBreak) {
            element = createBreakElement(document);
            fillContainerFromString(fragment.get(), s);
        } else {
            if (useClonesOfEnclosingBlock)
                element = block->cloneElementWithoutChildren();
            else
                element = createDefaultParagraphElement(document);
            fillContainerFromString(element.get(), s);
        }
        fragment->appendChild(element.release(), ASSERT_NO_EXCEPTION);
    }
    return fragment.release();
}

PassRefPtr<DocumentFragment> createFragmentFromNodes(Document *document, const Vector<Node*>& nodes)
{
    if (!document)
        return 0;

#if ENABLE(DELETION_UI)
    // disable the delete button so it's elements are not serialized into the markup
    DeleteButtonControllerDisableScope(document->frame());
#endif

    RefPtr<DocumentFragment> fragment = document->createDocumentFragment();

    size_t size = nodes.size();
    for (size_t i = 0; i < size; ++i) {
        RefPtr<Element> element = createDefaultParagraphElement(document);
        element->appendChild(nodes[i], ASSERT_NO_EXCEPTION);
        fragment->appendChild(element.release(), ASSERT_NO_EXCEPTION);
    }

    return fragment.release();
}

String createFullMarkup(const Node* node)
{
    if (!node)
        return String();
        
    Document* document = node->document();
    if (!document)
        return String();
        
    Frame* frame = document->frame();
    if (!frame)
        return String();

    // FIXME: This is never "for interchange". Is that right?    
    String markupString = createMarkup(node, IncludeNode, 0);
    Node::NodeType nodeType = node->nodeType();
    if (nodeType != Node::DOCUMENT_NODE && nodeType != Node::DOCUMENT_TYPE_NODE)
        markupString = frame->documentTypeString() + markupString;

    return markupString;
}

String createFullMarkup(const Range* range)
{
    if (!range)
        return String();

    Node* node = range->startContainer();
    if (!node)
        return String();
        
    Document* document = node->document();
    if (!document)
        return String();
        
    Frame* frame = document->frame();
    if (!frame)
        return String();

    // FIXME: This is always "for interchange". Is that right? See the previous method.
    return frame->documentTypeString() + createMarkup(range, 0, AnnotateForInterchange);        
}

String urlToMarkup(const KURL& url, const String& title)
{
    StringBuilder markup;
    markup.append("<a href=\"");
    markup.append(url.string());
    markup.append("\">");
    MarkupAccumulator::appendCharactersReplacingEntities(markup, title, 0, title.length(), EntityMaskInPCDATA);
    markup.append("</a>");
    return markup.toString();
}

PassRefPtr<DocumentFragment> createFragmentForInnerOuterHTML(const String& markup, Element* contextElement, ParserContentPolicy parserContentPolicy, ExceptionCode& ec)
{
    Document* document = contextElement->document();
#if ENABLE(TEMPLATE_ELEMENT)
    if (contextElement->hasTagName(templateTag))
        document = document->ensureTemplateDocument();
#endif
    RefPtr<DocumentFragment> fragment = DocumentFragment::create(document);

    if (document->isHTMLDocument()) {
        fragment->parseHTML(markup, contextElement, parserContentPolicy);
        return fragment;
    }

    bool wasValid = fragment->parseXML(markup, contextElement, parserContentPolicy);
    if (!wasValid) {
        ec = SYNTAX_ERR;
        return 0;
    }
    return fragment.release();
}

PassRefPtr<DocumentFragment> createFragmentForTransformToFragment(const String& sourceString, const String& sourceMIMEType, Document* outputDoc)
{
    RefPtr<DocumentFragment> fragment = outputDoc->createDocumentFragment();
    
    if (sourceMIMEType == "text/html") {
        // As far as I can tell, there isn't a spec for how transformToFragment is supposed to work.
        // Based on the documentation I can find, it looks like we want to start parsing the fragment in the InBody insertion mode.
        // Unfortunately, that's an implementation detail of the parser.
        // We achieve that effect here by passing in a fake body element as context for the fragment.
        RefPtr<HTMLBodyElement> fakeBody = HTMLBodyElement::create(outputDoc);
        fragment->parseHTML(sourceString, fakeBody.get());
    } else if (sourceMIMEType == "text/plain")
        fragment->parserAppendChild(Text::create(outputDoc, sourceString));
    else {
        bool successfulParse = fragment->parseXML(sourceString, 0);
        if (!successfulParse)
            return 0;
    }
    
    // FIXME: Do we need to mess with URLs here?
    
    return fragment.release();
}

static inline void removeElementPreservingChildren(PassRefPtr<DocumentFragment> fragment, HTMLElement* element)
{
    RefPtr<Node> nextChild;
    for (RefPtr<Node> child = element->firstChild(); child; child = nextChild) {
        nextChild = child->nextSibling();
        element->removeChild(child.get(), ASSERT_NO_EXCEPTION);
        fragment->insertBefore(child, element, ASSERT_NO_EXCEPTION);
    }
    fragment->removeChild(element, ASSERT_NO_EXCEPTION);
}

PassRefPtr<DocumentFragment> createContextualFragment(const String& markup, HTMLElement* element, ParserContentPolicy parserContentPolicy, ExceptionCode& ec)
{
    ASSERT(element);
    if (element->ieForbidsInsertHTML()) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    if (element->hasLocalName(colTag) || element->hasLocalName(colgroupTag) || element->hasLocalName(framesetTag)
        || element->hasLocalName(headTag) || element->hasLocalName(styleTag) || element->hasLocalName(titleTag)) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    RefPtr<DocumentFragment> fragment = createFragmentForInnerOuterHTML(markup, element, parserContentPolicy, ec);
    if (!fragment)
        return 0;

    // We need to pop <html> and <body> elements and remove <head> to
    // accommodate folks passing complete HTML documents to make the
    // child of an element.

    RefPtr<Node> nextNode;
    for (RefPtr<Node> node = fragment->firstChild(); node; node = nextNode) {
        nextNode = node->nextSibling();
        if (node->hasTagName(htmlTag) || node->hasTagName(headTag) || node->hasTagName(bodyTag)) {
            HTMLElement* element = toHTMLElement(node.get());
            if (Node* firstChild = element->firstChild())
                nextNode = firstChild;
            removeElementPreservingChildren(fragment, element);
        }
    }
    return fragment.release();
}

static inline bool hasOneChild(ContainerNode* node)
{
    Node* firstChild = node->firstChild();
    return firstChild && !firstChild->nextSibling();
}

static inline bool hasOneTextChild(ContainerNode* node)
{
    return hasOneChild(node) && node->firstChild()->isTextNode();
}

void replaceChildrenWithFragment(ContainerNode* container, PassRefPtr<DocumentFragment> fragment, ExceptionCode& ec)
{
    RefPtr<ContainerNode> containerNode(container);

    ChildListMutationScope mutation(containerNode.get());

    if (!fragment->firstChild()) {
        containerNode->removeChildren();
        return;
    }

    if (hasOneTextChild(containerNode.get()) && hasOneTextChild(fragment.get())) {
        toText(containerNode->firstChild())->setData(toText(fragment->firstChild())->data(), ec);
        return;
    }

    if (hasOneChild(containerNode.get())) {
        containerNode->replaceChild(fragment, containerNode->firstChild(), ec);
        return;
    }

    containerNode->removeChildren();
    containerNode->appendChild(fragment, ec);
}

void replaceChildrenWithText(ContainerNode* container, const String& text, ExceptionCode& ec)
{
    RefPtr<ContainerNode> containerNode(container);

    ChildListMutationScope mutation(containerNode.get());

    if (hasOneTextChild(containerNode.get())) {
        toText(containerNode->firstChild())->setData(text, ec);
        return;
    }

    RefPtr<Text> textNode = Text::create(containerNode->document(), text);

    if (hasOneChild(containerNode.get())) {
        containerNode->replaceChild(textNode.release(), containerNode->firstChild(), ec);
        return;
    }

    containerNode->removeChildren();
    containerNode->appendChild(textNode.release(), ec);
}

}
