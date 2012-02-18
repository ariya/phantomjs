/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Google Inc.
 * Copyright (C) 2011 Igalia S.L.
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
#include "CSSComputedStyleDeclaration.h"
#include "CSSMutableStyleDeclaration.h"
#include "CSSPrimitiveValue.h"
#include "CSSProperty.h"
#include "CSSPropertyNames.h"
#include "CSSRule.h"
#include "CSSRuleList.h"
#include "CSSStyleRule.h"
#include "CSSStyleSelector.h"
#include "CSSValue.h"
#include "CSSValueKeywords.h"
#include "DeleteButtonController.h"
#include "DocumentFragment.h"
#include "DocumentType.h"
#include "Editor.h"
#include "Frame.h"
#include "HTMLBodyElement.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "KURL.h"
#include "MarkupAccumulator.h"
#include "Range.h"
#include "TextIterator.h"
#include "VisibleSelection.h"
#include "XMLNSNames.h"
#include "htmlediting.h"
#include "visible_units.h"
#include <wtf/StdLibExtras.h>
#include <wtf/unicode/CharacterNames.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;

static bool propertyMissingOrEqualToNone(CSSStyleDeclaration*, int propertyID);

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

static void completeURLs(Node* node, const String& baseURL)
{
    Vector<AttributeChange> changes;

    KURL parsedBaseURL(ParsedURLString, baseURL);

    Node* end = node->traverseNextSibling();
    for (Node* n = node; n != end; n = n->traverseNextNode()) {
        if (n->isElementNode()) {
            Element* e = static_cast<Element*>(n);
            NamedNodeMap* attributes = e->attributes();
            unsigned length = attributes->length();
            for (unsigned i = 0; i < length; i++) {
                Attribute* attribute = attributes->attributeItem(i);
                if (e->isURLAttribute(attribute))
                    changes.append(AttributeChange(e, attribute->name(), KURL(parsedBaseURL, attribute->value()).string()));
            }
        }
    }

    size_t numChanges = changes.size();
    for (size_t i = 0; i < numChanges; ++i)
        changes[i].apply();
}
    
class StyledMarkupAccumulator : public MarkupAccumulator {
public:
    enum RangeFullySelectsNode { DoesFullySelectNode, DoesNotFullySelectNode };

    StyledMarkupAccumulator(Vector<Node*>* nodes, EAbsoluteURLs shouldResolveURLs, EAnnotateForInterchange shouldAnnotate, const Range* range)
    : MarkupAccumulator(nodes, shouldResolveURLs, range)
    , m_shouldAnnotate(shouldAnnotate)
    {
    }

    Node* serializeNodes(Node* startNode, Node* pastEnd);
    virtual void appendString(const String& s) { return MarkupAccumulator::appendString(s); }
    void wrapWithNode(Node*, bool convertBlocksToInlines = false, RangeFullySelectsNode = DoesFullySelectNode);
    void wrapWithStyleNode(CSSStyleDeclaration*, Document*, bool isBlock = false);
    String takeResults();

private:
    virtual void appendText(Vector<UChar>& out, Text*);
    String renderedText(const Node*, const Range*);
    String stringValueForRange(const Node*, const Range*);
    void removeExteriorStyles(CSSMutableStyleDeclaration*);
    void appendElement(Vector<UChar>& out, Element* element, bool addDisplayInline, RangeFullySelectsNode);
    void appendElement(Vector<UChar>& out, Element* element, Namespaces*) { appendElement(out, element, false, DoesFullySelectNode); }

    bool shouldAnnotate() { return m_shouldAnnotate == AnnotateForInterchange; }

    Vector<String> m_reversedPrecedingMarkup;
    const EAnnotateForInterchange m_shouldAnnotate;
};

void StyledMarkupAccumulator::wrapWithNode(Node* node, bool convertBlocksToInlines, RangeFullySelectsNode rangeFullySelectsNode)
{
    Vector<UChar> markup;
    if (node->isElementNode())
        appendElement(markup, static_cast<Element*>(node), convertBlocksToInlines && isBlock(const_cast<Node*>(node)), rangeFullySelectsNode);
    else
        appendStartMarkup(markup, node, 0);
    m_reversedPrecedingMarkup.append(String::adopt(markup));
    appendEndTag(node);
    if (m_nodes)
        m_nodes->append(node);
}

void StyledMarkupAccumulator::wrapWithStyleNode(CSSStyleDeclaration* style, Document* document, bool isBlock)
{
    // All text-decoration-related elements should have been treated as special ancestors
    // If we ever hit this ASSERT, we should export StyleChange in ApplyStyleCommand and use it here
    ASSERT(propertyMissingOrEqualToNone(style, CSSPropertyTextDecoration) && propertyMissingOrEqualToNone(style, CSSPropertyWebkitTextDecorationsInEffect));
    DEFINE_STATIC_LOCAL(const String, divStyle, ("<div style=\""));
    DEFINE_STATIC_LOCAL(const String, divClose, ("</div>"));
    DEFINE_STATIC_LOCAL(const String, styleSpanOpen, ("<span class=\"" AppleStyleSpanClass "\" style=\""));
    DEFINE_STATIC_LOCAL(const String, styleSpanClose, ("</span>"));
    Vector<UChar> openTag;
    append(openTag, isBlock ? divStyle : styleSpanOpen);
    appendAttributeValue(openTag, style->cssText(), document->isHTMLDocument());
    openTag.append('\"');
    openTag.append('>');
    m_reversedPrecedingMarkup.append(String::adopt(openTag));
    appendString(isBlock ? divClose : styleSpanClose);
}

String StyledMarkupAccumulator::takeResults()
{
    Vector<UChar> result;
    result.reserveInitialCapacity(totalLength(m_reversedPrecedingMarkup) + length());

    for (size_t i = m_reversedPrecedingMarkup.size(); i > 0; --i)
        append(result, m_reversedPrecedingMarkup[i - 1]);

    concatenateMarkup(result);

    // We remove '\0' characters because they are not visibly rendered to the user.
    return String::adopt(result).replace(0, "");
}

void StyledMarkupAccumulator::appendText(Vector<UChar>& out, Text* text)
{
    if (!shouldAnnotate() || (text->parentElement() && text->parentElement()->tagQName() == textareaTag)) {
        MarkupAccumulator::appendText(out, text);
        return;
    }

    bool useRenderedText = !enclosingNodeWithTag(firstPositionInNode(text), selectTag);
    String content = useRenderedText ? renderedText(text, m_range) : stringValueForRange(text, m_range);
    Vector<UChar> buffer;
    appendCharactersReplacingEntities(buffer, content.characters(), content.length(), EntityMaskInPCDATA);
    append(out, convertHTMLTextToInterchangeFormat(String::adopt(buffer), text));
}
    
String StyledMarkupAccumulator::renderedText(const Node* node, const Range* range)
{
    if (!node->isTextNode())
        return String();

    ExceptionCode ec;
    const Text* textNode = static_cast<const Text*>(node);
    unsigned startOffset = 0;
    unsigned endOffset = textNode->length();

    if (range && node == range->startContainer(ec))
        startOffset = range->startOffset(ec);
    if (range && node == range->endContainer(ec))
        endOffset = range->endOffset(ec);

    Position start(const_cast<Node*>(node), startOffset);
    Position end(const_cast<Node*>(node), endOffset);
    return plainText(Range::create(node->document(), start, end).get());
}

String StyledMarkupAccumulator::stringValueForRange(const Node* node, const Range* range)
{
    if (!range)
        return node->nodeValue();

    String str = node->nodeValue();
    ExceptionCode ec;
    if (node == range->endContainer(ec))
        str.truncate(range->endOffset(ec));
    if (node == range->startContainer(ec))
        str.remove(0, range->startOffset(ec));
    return str;
}

static PassRefPtr<CSSMutableStyleDeclaration> styleFromMatchedRulesForElement(Element* element, bool authorOnly = true)
{
    RefPtr<CSSMutableStyleDeclaration> style = CSSMutableStyleDeclaration::create();
    RefPtr<CSSRuleList> matchedRules = element->document()->styleSelector()->styleRulesForElement(element, authorOnly);
    if (matchedRules) {
        for (unsigned i = 0; i < matchedRules->length(); i++) {
            if (matchedRules->item(i)->type() == CSSRule::STYLE_RULE) {
                RefPtr<CSSMutableStyleDeclaration> s = static_cast<CSSStyleRule*>(matchedRules->item(i))->style();
                style->merge(s.get(), true);
            }
        }
    }

    return style.release();
}

void StyledMarkupAccumulator::appendElement(Vector<UChar>& out, Element* element, bool addDisplayInline, RangeFullySelectsNode rangeFullySelectsNode)
{
    bool documentIsHTML = element->document()->isHTMLDocument();
    appendOpenTag(out, element, 0);

    NamedNodeMap* attributes = element->attributes();
    unsigned length = attributes->length();
    for (unsigned int i = 0; i < length; i++) {
        Attribute* attribute = attributes->attributeItem(i);
        // We'll handle the style attribute separately, below.
        if (attribute->name() == styleAttr && element->isHTMLElement() && (shouldAnnotate() || addDisplayInline))
            continue;
        appendAttribute(out, element, *attribute, 0);
    }

    if (element->isHTMLElement() && (shouldAnnotate() || addDisplayInline)) {
        RefPtr<CSSMutableStyleDeclaration> style = toHTMLElement(element)->getInlineStyleDecl()->copy();
        if (shouldAnnotate()) {
            RefPtr<CSSMutableStyleDeclaration> styleFromMatchedRules = styleFromMatchedRulesForElement(const_cast<Element*>(element));
            // Styles from the inline style declaration, held in the variable "style", take precedence 
            // over those from matched rules.
            styleFromMatchedRules->merge(style.get());
            style = styleFromMatchedRules;

            RefPtr<CSSComputedStyleDeclaration> computedStyleForElement = computedStyle(element);
            RefPtr<CSSMutableStyleDeclaration> fromComputedStyle = CSSMutableStyleDeclaration::create();

            {
                CSSMutableStyleDeclaration::const_iterator end = style->end();
                for (CSSMutableStyleDeclaration::const_iterator it = style->begin(); it != end; ++it) {
                    const CSSProperty& property = *it;
                    CSSValue* value = property.value();
                    // The property value, if it's a percentage, may not reflect the actual computed value.  
                    // For example: style="height: 1%; overflow: visible;" in quirksmode
                    // FIXME: There are others like this, see <rdar://problem/5195123> Slashdot copy/paste fidelity problem
                    if (value->cssValueType() == CSSValue::CSS_PRIMITIVE_VALUE)
                        if (static_cast<CSSPrimitiveValue*>(value)->primitiveType() == CSSPrimitiveValue::CSS_PERCENTAGE)
                            if (RefPtr<CSSValue> computedPropertyValue = computedStyleForElement->getPropertyCSSValue(property.id()))
                                fromComputedStyle->addParsedProperty(CSSProperty(property.id(), computedPropertyValue));
                }
            }
            style->merge(fromComputedStyle.get());
        }
        if (addDisplayInline)
            style->setProperty(CSSPropertyDisplay, CSSValueInline, true);
        // If the node is not fully selected by the range, then we don't want to keep styles that affect its relationship to the nodes around it
        // only the ones that affect it and the nodes within it.
        if (rangeFullySelectsNode == DoesNotFullySelectNode)
            removeExteriorStyles(style.get());
        if (style->length() > 0) {
            DEFINE_STATIC_LOCAL(const String, stylePrefix, (" style=\""));
            append(out, stylePrefix);
            appendAttributeValue(out, style->cssText(), documentIsHTML);
            out.append('\"');
        }
    }

    appendCloseTag(out, element);
}

void StyledMarkupAccumulator::removeExteriorStyles(CSSMutableStyleDeclaration* style)
{
    style->removeProperty(CSSPropertyFloat);
}

Node* StyledMarkupAccumulator::serializeNodes(Node* startNode, Node* pastEnd)
{
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
        
        next = n->traverseNextNode();
        bool openedTag = false;

        if (isBlock(n) && canHaveChildrenForEditing(n) && next == pastEnd)
            // Don't write out empty block containers that aren't fully selected.
            continue;

        if (!n->renderer() && !enclosingNodeWithTag(firstPositionInOrBeforeNode(n), selectTag)) {
            next = n->traverseNextSibling();
            // Don't skip over pastEnd.
            if (pastEnd && pastEnd->isDescendantOf(n))
                next = pastEnd;
        } else {
            // Add the node to the markup if we're not skipping the descendants
            appendStartTag(n);

            // If node has no children, close the tag now.
            if (!n->childNodeCount()) {
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
                    wrapWithNode(parent);
                    lastClosed = parent;
                }
            }
        }
    }

    return lastClosed;
}

static Node* ancestorToRetainStructureAndAppearance(Node* commonAncestor)
{
    Node* commonAncestorBlock = enclosingBlock(commonAncestor);

    if (!commonAncestorBlock)
        return 0;

    if (commonAncestorBlock->hasTagName(tbodyTag) || commonAncestorBlock->hasTagName(trTag)) {
        ContainerNode* table = commonAncestorBlock->parentNode();
        while (table && !table->hasTagName(tableTag))
            table = table->parentNode();

        return table;
    }

    if (commonAncestorBlock->hasTagName(listingTag)
        || commonAncestorBlock->hasTagName(olTag)
        || commonAncestorBlock->hasTagName(preTag)
        || commonAncestorBlock->hasTagName(tableTag)
        || commonAncestorBlock->hasTagName(ulTag)
        || commonAncestorBlock->hasTagName(xmpTag)
        || commonAncestorBlock->hasTagName(h1Tag)
        || commonAncestorBlock->hasTagName(h2Tag)
        || commonAncestorBlock->hasTagName(h3Tag)
        || commonAncestorBlock->hasTagName(h4Tag)
        || commonAncestorBlock->hasTagName(h5Tag))
        return commonAncestorBlock;

    return 0;
}

static bool propertyMissingOrEqualToNone(CSSStyleDeclaration* style, int propertyID)
{
    if (!style)
        return false;
    RefPtr<CSSValue> value = style->getPropertyCSSValue(propertyID);
    if (!value)
        return true;
    if (!value->isPrimitiveValue())
        return false;
    return static_cast<CSSPrimitiveValue*>(value.get())->getIdent() == CSSValueNone;
}

static bool needInterchangeNewlineAfter(const VisiblePosition& v)
{
    VisiblePosition next = v.next();
    Node* upstreamNode = next.deepEquivalent().upstream().deprecatedNode();
    Node* downstreamNode = v.deepEquivalent().downstream().deprecatedNode();
    // Add an interchange newline if a paragraph break is selected and a br won't already be added to the markup to represent it.
    return isEndOfParagraph(v) && isStartOfParagraph(next) && !(upstreamNode->hasTagName(brTag) && upstreamNode == downstreamNode);
}

static PassRefPtr<CSSMutableStyleDeclaration> styleFromMatchedRulesAndInlineDecl(const Node* node)
{
    if (!node->isHTMLElement())
        return 0;

    // FIXME: Having to const_cast here is ugly, but it is quite a bit of work to untangle
    // the non-const-ness of styleFromMatchedRulesForElement.
    HTMLElement* element = const_cast<HTMLElement*>(static_cast<const HTMLElement*>(node));
    RefPtr<CSSMutableStyleDeclaration> style = styleFromMatchedRulesForElement(element);
    RefPtr<CSSMutableStyleDeclaration> inlineStyleDecl = element->getInlineStyleDecl();
    style->merge(inlineStyleDecl.get());
    return style.release();
}

static bool isElementPresentational(const Node* node)
{
    if (node->hasTagName(uTag) || node->hasTagName(sTag) || node->hasTagName(strikeTag)
        || node->hasTagName(iTag) || node->hasTagName(emTag) || node->hasTagName(bTag) || node->hasTagName(strongTag))
        return true;
    RefPtr<CSSMutableStyleDeclaration> style = styleFromMatchedRulesAndInlineDecl(node);
    if (!style)
        return false;
    return !propertyMissingOrEqualToNone(style.get(), CSSPropertyTextDecoration);
}

static bool shouldIncludeWrapperForFullySelectedRoot(Node* fullySelectedRoot, CSSMutableStyleDeclaration* style)
{
    if (fullySelectedRoot->isElementNode() && static_cast<Element*>(fullySelectedRoot)->hasAttribute(backgroundAttr))
        return true;
    
    return style->getPropertyCSSValue(CSSPropertyBackgroundImage) || style->getPropertyCSSValue(CSSPropertyBackgroundColor);
}

static Node* highestAncestorToWrapMarkup(const Range* range, Node* fullySelectedRoot, EAnnotateForInterchange shouldAnnotate)
{
    ExceptionCode ec;
    Node* commonAncestor = range->commonAncestorContainer(ec);
    ASSERT(commonAncestor);
    Node* specialCommonAncestor = 0;
    if (shouldAnnotate == AnnotateForInterchange) {
        // Include ancestors that aren't completely inside the range but are required to retain 
        // the structure and appearance of the copied markup.
        specialCommonAncestor = ancestorToRetainStructureAndAppearance(commonAncestor);

        // Retain the Mail quote level by including all ancestor mail block quotes.
        if (Node* highestMailBlockquote = highestEnclosingNodeOfType(firstPositionInOrBeforeNode(range->firstNode()), isMailBlockquote, CanCrossEditingBoundary))
            specialCommonAncestor = highestMailBlockquote;
    }

    Node* checkAncestor = specialCommonAncestor ? specialCommonAncestor : commonAncestor;
    if (checkAncestor->renderer()) {
        Node* newSpecialCommonAncestor = highestEnclosingNodeOfType(firstPositionInNode(checkAncestor), &isElementPresentational);
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

    if (shouldAnnotate == AnnotateForInterchange && fullySelectedRoot) {
        RefPtr<CSSMutableStyleDeclaration> fullySelectedRootStyle = styleFromMatchedRulesAndInlineDecl(fullySelectedRoot);
        if (shouldIncludeWrapperForFullySelectedRoot(fullySelectedRoot, fullySelectedRootStyle.get()))
            specialCommonAncestor = fullySelectedRoot;
    }
    return specialCommonAncestor;
}

// FIXME: Shouldn't we omit style info when annotate == DoNotAnnotateForInterchange? 
// FIXME: At least, annotation and style info should probably not be included in range.markupString()
String createMarkup(const Range* range, Vector<Node*>* nodes, EAnnotateForInterchange shouldAnnotate, bool convertBlocksToInlines, EAbsoluteURLs shouldResolveURLs)
{
    DEFINE_STATIC_LOCAL(const String, interchangeNewlineString, ("<br class=\"" AppleInterchangeNewline "\">"));

    if (!range)
        return "";

    Document* document = range->ownerDocument();
    if (!document)
        return "";

    // Disable the delete button so it's elements are not serialized into the markup,
    // but make sure neither endpoint is inside the delete user interface.
    Frame* frame = document->frame();
    DeleteButtonController* deleteButton = frame ? frame->editor()->deleteButtonController() : 0;
    RefPtr<Range> updatedRange = avoidIntersectionWithNode(range, deleteButton ? deleteButton->containerElement() : 0);
    if (!updatedRange)
        return "";

    if (deleteButton)
        deleteButton->disable();

    ExceptionCode ec = 0;
    bool collapsed = updatedRange->collapsed(ec);
    ASSERT(!ec);
    if (collapsed)
        return "";
    Node* commonAncestor = updatedRange->commonAncestorContainer(ec);
    ASSERT(!ec);
    if (!commonAncestor)
        return "";

    document->updateLayoutIgnorePendingStylesheets();

    StyledMarkupAccumulator accumulator(nodes, shouldResolveURLs, shouldAnnotate, updatedRange.get());
    Node* pastEnd = updatedRange->pastLastNode();

    Node* startNode = updatedRange->firstNode();
    VisiblePosition visibleStart(updatedRange->startPosition(), VP_DEFAULT_AFFINITY);
    VisiblePosition visibleEnd(updatedRange->endPosition(), VP_DEFAULT_AFFINITY);
    if (shouldAnnotate == AnnotateForInterchange && needInterchangeNewlineAfter(visibleStart)) {
        if (visibleStart == visibleEnd.previous()) {
            if (deleteButton)
                deleteButton->enable();
            return interchangeNewlineString;
        }

        accumulator.appendString(interchangeNewlineString);
        startNode = visibleStart.next().deepEquivalent().deprecatedNode();

        ExceptionCode ec = 0;
        if (pastEnd && Range::compareBoundaryPoints(startNode, 0, pastEnd, 0, ec) >= 0) {
            ASSERT(!ec);
            if (deleteButton)
                deleteButton->enable();
            return interchangeNewlineString;
        }
        ASSERT(!ec);
    }

    Node* body = enclosingNodeWithTag(firstPositionInNode(commonAncestor), bodyTag);
    Node* fullySelectedRoot = 0;
    // FIXME: Do this for all fully selected blocks, not just the body.
    if (body && areRangesEqual(VisibleSelection::selectionFromContentsOfNode(body).toNormalizedRange().get(), range))
        fullySelectedRoot = body;

    Node* specialCommonAncestor = highestAncestorToWrapMarkup(updatedRange.get(), fullySelectedRoot, shouldAnnotate);

    Node* lastClosed = accumulator.serializeNodes(startNode, pastEnd);

    if (specialCommonAncestor && lastClosed) {
        // Also include all of the ancestors of lastClosed up to this special ancestor.
        for (ContainerNode* ancestor = lastClosed->parentNode(); ancestor; ancestor = ancestor->parentNode()) {
            if (ancestor == fullySelectedRoot && !convertBlocksToInlines) {
                RefPtr<CSSMutableStyleDeclaration> fullySelectedRootStyle = styleFromMatchedRulesAndInlineDecl(fullySelectedRoot);

                // Bring the background attribute over, but not as an attribute because a background attribute on a div
                // appears to have no effect.
                if (!fullySelectedRootStyle->getPropertyCSSValue(CSSPropertyBackgroundImage) && static_cast<Element*>(fullySelectedRoot)->hasAttribute(backgroundAttr))
                    fullySelectedRootStyle->setProperty(CSSPropertyBackgroundImage, "url('" + static_cast<Element*>(fullySelectedRoot)->getAttribute(backgroundAttr) + "')");
                
                if (fullySelectedRootStyle->length()) {
                    // Reset the CSS properties to avoid an assertion error in addStyleMarkup().
                    // This assertion is caused at least when we select all text of a <body> element whose
                    // 'text-decoration' property is "inherit", and copy it.
                    if (!propertyMissingOrEqualToNone(fullySelectedRootStyle.get(), CSSPropertyTextDecoration))
                        fullySelectedRootStyle->setProperty(CSSPropertyTextDecoration, CSSValueNone);
                    if (!propertyMissingOrEqualToNone(fullySelectedRootStyle.get(), CSSPropertyWebkitTextDecorationsInEffect))
                        fullySelectedRootStyle->setProperty(CSSPropertyWebkitTextDecorationsInEffect, CSSValueNone);
                    accumulator.wrapWithStyleNode(fullySelectedRootStyle.get(), document, true);
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

    // Add a wrapper span with the styles that all of the nodes in the markup inherit.
    ContainerNode* parentOfLastClosed = lastClosed ? lastClosed->parentNode() : 0;
    if (parentOfLastClosed && parentOfLastClosed->renderer()) {
        RefPtr<EditingStyle> style = EditingStyle::create(parentOfLastClosed, EditingStyle::InheritablePropertiesAndBackgroundColorInEffect);

        // Styles that Mail blockquotes contribute should only be placed on the Mail blockquote, to help
        // us differentiate those styles from ones that the user has applied.  This helps us
        // get the color of content pasted into blockquotes right.
        style->removeStyleAddedByNode(enclosingNodeOfType(firstPositionInNode(parentOfLastClosed), isMailBlockquote, CanCrossEditingBoundary));

        if (!style->isEmpty())
            accumulator.wrapWithStyleNode(style->style(), document);
    }

    // FIXME: The interchange newline should be placed in the block that it's in, not after all of the content, unconditionally.
    if (shouldAnnotate == AnnotateForInterchange && needInterchangeNewlineAfter(visibleEnd.previous()))
        accumulator.appendString(interchangeNewlineString);

    if (deleteButton)
        deleteButton->enable();

    return accumulator.takeResults();
}

PassRefPtr<DocumentFragment> createFragmentFromMarkup(Document* document, const String& markup, const String& baseURL, FragmentScriptingPermission scriptingPermission)
{
    // We use a fake body element here to trick the HTML parser to using the
    // InBody insertion mode.  Really, all this code is wrong and need to be
    // changed not to use deprecatedCreateContextualFragment.
    RefPtr<HTMLBodyElement> fakeBody = HTMLBodyElement::create(document);
    // FIXME: This should not use deprecatedCreateContextualFragment
    RefPtr<DocumentFragment> fragment = fakeBody->deprecatedCreateContextualFragment(markup, scriptingPermission);

    if (fragment && !baseURL.isEmpty() && baseURL != blankURL() && baseURL != document->baseURL())
        completeURLs(fragment.get(), baseURL);

    return fragment.release();
}

String createMarkup(const Node* node, EChildrenOnly childrenOnly, Vector<Node*>* nodes, EAbsoluteURLs shouldResolveURLs)
{
    if (!node)
        return "";

    HTMLElement* deleteButtonContainerElement = 0;
    if (Frame* frame = node->document()->frame()) {
        deleteButtonContainerElement = frame->editor()->deleteButtonController()->containerElement();
        if (node->isDescendantOf(deleteButtonContainerElement))
            return "";
    }

    MarkupAccumulator accumulator(nodes, shouldResolveURLs);
    return accumulator.serializeNodes(const_cast<Node*>(node), deleteButtonContainerElement, childrenOnly);
}

static void fillContainerFromString(ContainerNode* paragraph, const String& string)
{
    Document* document = paragraph->document();

    ExceptionCode ec = 0;
    if (string.isEmpty()) {
        paragraph->appendChild(createBlockPlaceholderElement(document), ec);
        ASSERT(!ec);
        return;
    }

    ASSERT(string.find('\n') == notFound);

    Vector<String> tabList;
    string.split('\t', true, tabList);
    String tabText = "";
    bool first = true;
    size_t numEntries = tabList.size();
    for (size_t i = 0; i < numEntries; ++i) {
        const String& s = tabList[i];

        // append the non-tab textual part
        if (!s.isEmpty()) {
            if (!tabText.isEmpty()) {
                paragraph->appendChild(createTabSpanElement(document, tabText), ec);
                ASSERT(!ec);
                tabText = "";
            }
            RefPtr<Node> textNode = document->createTextNode(stringWithRebalancedWhitespace(s, first, i + 1 == numEntries));
            paragraph->appendChild(textNode.release(), ec);
            ASSERT(!ec);
        }

        // there is a tab after every entry, except the last entry
        // (if the last character is a tab, the list gets an extra empty entry)
        if (i + 1 != numEntries)
            tabText.append('\t');
        else if (!tabText.isEmpty()) {
            paragraph->appendChild(createTabSpanElement(document, tabText), ec);
            ASSERT(!ec);
        }
        
        first = false;
    }
}

bool isPlainTextMarkup(Node *node)
{
    if (!node->isElementNode() || !node->hasTagName(divTag) || static_cast<Element*>(node)->attributes()->length())
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

    ExceptionCode ec = 0;
    RenderObject* renderer = styleNode->renderer();
    if (renderer && renderer->style()->preserveNewline()) {
        fragment->appendChild(document->createTextNode(string), ec);
        ASSERT(!ec);
        if (string.endsWith("\n")) {
            RefPtr<Element> element = createBreakElement(document);
            element->setAttribute(classAttr, AppleInterchangeNewline);            
            fragment->appendChild(element.release(), ec);
            ASSERT(!ec);
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
    Element* block = static_cast<Element*>(blockNode);
    bool useClonesOfEnclosingBlock = blockNode
        && blockNode->isElementNode()
        && !block->hasTagName(bodyTag)
        && !block->hasTagName(htmlTag)
        && block != editableRootForPosition(context->startPosition());
    
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
        } else {
            if (useClonesOfEnclosingBlock)
                element = block->cloneElementWithoutChildren();
            else
                element = createDefaultParagraphElement(document);
            fillContainerFromString(element.get(), s);
        }
        fragment->appendChild(element.release(), ec);
        ASSERT(!ec);
    }
    return fragment.release();
}

PassRefPtr<DocumentFragment> createFragmentFromNodes(Document *document, const Vector<Node*>& nodes)
{
    if (!document)
        return 0;

    // disable the delete button so it's elements are not serialized into the markup
    if (document->frame())
        document->frame()->editor()->deleteButtonController()->disable();

    RefPtr<DocumentFragment> fragment = document->createDocumentFragment();

    ExceptionCode ec = 0;
    size_t size = nodes.size();
    for (size_t i = 0; i < size; ++i) {
        RefPtr<Element> element = createDefaultParagraphElement(document);
        element->appendChild(nodes[i], ec);
        ASSERT(!ec);
        fragment->appendChild(element.release(), ec);
        ASSERT(!ec);
    }

    if (document->frame())
        document->frame()->editor()->deleteButtonController()->enable();

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
    Vector<UChar> markup;
    append(markup, "<a href=\"");
    append(markup, url.string());
    append(markup, "\">");
    appendCharactersReplacingEntities(markup, title.characters(), title.length(), EntityMaskInPCDATA);
    append(markup, "</a>");
    return String::adopt(markup);
}

String imageToMarkup(const KURL& url, Element* element)
{
    Vector<UChar> markup;
    append(markup, "<img src=\"");
    append(markup, url.string());
    append(markup, "\"");

    NamedNodeMap* attrs = element->attributes();
    unsigned length = attrs->length();
    for (unsigned i = 0; i < length; ++i) {
        Attribute* attr = attrs->attributeItem(i);
        if (attr->localName() == "src")
            continue;
        append(markup, " ");
        append(markup, attr->localName());
        append(markup, "=\"");
        appendCharactersReplacingEntities(markup, attr->value().characters(), attr->value().length(), EntityMaskInAttributeValue);
        append(markup, "\"");
    }

    append(markup, "/>");
    return String::adopt(markup);
}

}
