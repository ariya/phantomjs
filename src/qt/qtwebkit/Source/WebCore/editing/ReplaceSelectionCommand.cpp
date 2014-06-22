/*
 * Copyright (C) 2005, 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2009, 2010, 2011 Google Inc. All rights reserved.
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
#include "ReplaceSelectionCommand.h"

#include "ApplyStyleCommand.h"
#include "BeforeTextInsertedEvent.h"
#include "BreakBlockquoteCommand.h"
#include "CSSPropertyNames.h"
#include "CSSStyleDeclaration.h"
#include "CSSValueKeywords.h"
#include "Document.h"
#include "DocumentFragment.h"
#include "Element.h"
#include "EventNames.h"
#include "ExceptionCodePlaceholder.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "HTMLElement.h"
#include "HTMLInputElement.h"
#include "HTMLInterchange.h"
#include "HTMLNames.h"
#include "HTMLTitleElement.h"
#include "NodeList.h"
#include "NodeRenderStyle.h"
#include "NodeTraversal.h"
#include "RenderInline.h"
#include "RenderObject.h"
#include "RenderText.h"
#include "SimplifyMarkupCommand.h"
#include "SmartReplace.h"
#include "StylePropertySet.h"
#include "Text.h"
#include "TextIterator.h"
#include "VisibleUnits.h"
#include "htmlediting.h"
#include "markup.h"
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>

namespace WebCore {

using namespace HTMLNames;

enum EFragmentType { EmptyFragment, SingleTextNodeFragment, TreeFragment };

// --- ReplacementFragment helper class

class ReplacementFragment {
    WTF_MAKE_NONCOPYABLE(ReplacementFragment);
public:
    ReplacementFragment(Document*, DocumentFragment*, const VisibleSelection&);

    Node* firstChild() const;
    Node* lastChild() const;

    bool isEmpty() const;
    
    bool hasInterchangeNewlineAtStart() const { return m_hasInterchangeNewlineAtStart; }
    bool hasInterchangeNewlineAtEnd() const { return m_hasInterchangeNewlineAtEnd; }
    
    void removeNode(PassRefPtr<Node>);
    void removeNodePreservingChildren(PassRefPtr<Node>);

private:
    PassRefPtr<StyledElement> insertFragmentForTestRendering(Node* rootEditableNode);
    void removeUnrenderedNodes(Node*);
    void restoreAndRemoveTestRenderingNodesToFragment(StyledElement*);
    void removeInterchangeNodes(Node*);
    
    void insertNodeBefore(PassRefPtr<Node> node, Node* refNode);

    RefPtr<Document> m_document;
    RefPtr<DocumentFragment> m_fragment;
    bool m_hasInterchangeNewlineAtStart;
    bool m_hasInterchangeNewlineAtEnd;
};

static bool isInterchangeNewlineNode(const Node *node)
{
    DEFINE_STATIC_LOCAL(String, interchangeNewlineClassString, (AppleInterchangeNewline));
    return node && node->hasTagName(brTag) && 
           static_cast<const Element *>(node)->getAttribute(classAttr) == interchangeNewlineClassString;
}

static bool isInterchangeConvertedSpaceSpan(const Node *node)
{
    DEFINE_STATIC_LOCAL(String, convertedSpaceSpanClassString, (AppleConvertedSpace));
    return node->isHTMLElement() && 
           static_cast<const HTMLElement *>(node)->getAttribute(classAttr) == convertedSpaceSpanClassString;
}

static Position positionAvoidingPrecedingNodes(Position pos)
{
    // If we're already on a break, it's probably a placeholder and we shouldn't change our position.
    if (editingIgnoresContent(pos.deprecatedNode()))
        return pos;

    // We also stop when changing block flow elements because even though the visual position is the
    // same.  E.g.,
    //   <div>foo^</div>^
    // The two positions above are the same visual position, but we want to stay in the same block.
    Node* enclosingBlockNode = enclosingBlock(pos.containerNode());
    for (Position nextPosition = pos; nextPosition.containerNode() != enclosingBlockNode; pos = nextPosition) {
        if (lineBreakExistsAtPosition(pos))
            break;

        if (pos.containerNode()->nonShadowBoundaryParentNode())
            nextPosition = positionInParentAfterNode(pos.containerNode());
        
        if (nextPosition == pos 
            || enclosingBlock(nextPosition.containerNode()) != enclosingBlockNode
            || VisiblePosition(pos) != VisiblePosition(nextPosition))
            break;
    }
    return pos;
}

ReplacementFragment::ReplacementFragment(Document* document, DocumentFragment* fragment, const VisibleSelection& selection)
    : m_document(document),
      m_fragment(fragment),
      m_hasInterchangeNewlineAtStart(false), 
      m_hasInterchangeNewlineAtEnd(false)
{
    if (!m_document)
        return;
    if (!m_fragment)
        return;
    if (!m_fragment->firstChild())
        return;
    
    RefPtr<Element> editableRoot = selection.rootEditableElement();
    ASSERT(editableRoot);
    if (!editableRoot)
        return;
    
    Node* shadowAncestorNode = editableRoot->deprecatedShadowAncestorNode();
    
    if (!editableRoot->getAttributeEventListener(eventNames().webkitBeforeTextInsertedEvent) &&
        // FIXME: Remove these checks once textareas and textfields actually register an event handler.
        !(shadowAncestorNode && shadowAncestorNode->renderer() && shadowAncestorNode->renderer()->isTextControl()) &&
        editableRoot->rendererIsRichlyEditable()) {
        removeInterchangeNodes(m_fragment.get());
        return;
    }

    RefPtr<StyledElement> holder = insertFragmentForTestRendering(editableRoot.get());
    if (!holder) {
        removeInterchangeNodes(m_fragment.get());
        return;
    }
    
    RefPtr<Range> range = VisibleSelection::selectionFromContentsOfNode(holder.get()).toNormalizedRange();
    String text = plainText(range.get(), static_cast<TextIteratorBehavior>(TextIteratorEmitsOriginalText | TextIteratorIgnoresStyleVisibility));

    removeInterchangeNodes(holder.get());
    removeUnrenderedNodes(holder.get());
    restoreAndRemoveTestRenderingNodesToFragment(holder.get());

    // Give the root a chance to change the text.
    RefPtr<BeforeTextInsertedEvent> evt = BeforeTextInsertedEvent::create(text);
    editableRoot->dispatchEvent(evt, ASSERT_NO_EXCEPTION);
    if (text != evt->text() || !editableRoot->rendererIsRichlyEditable()) {
        restoreAndRemoveTestRenderingNodesToFragment(holder.get());

        m_fragment = createFragmentFromText(selection.toNormalizedRange().get(), evt->text());
        if (!m_fragment->firstChild())
            return;

        holder = insertFragmentForTestRendering(editableRoot.get());
        removeInterchangeNodes(holder.get());
        removeUnrenderedNodes(holder.get());
        restoreAndRemoveTestRenderingNodesToFragment(holder.get());
    }
}

bool ReplacementFragment::isEmpty() const
{
    return (!m_fragment || !m_fragment->firstChild()) && !m_hasInterchangeNewlineAtStart && !m_hasInterchangeNewlineAtEnd;
}

Node *ReplacementFragment::firstChild() const 
{ 
    return m_fragment ? m_fragment->firstChild() : 0; 
}

Node *ReplacementFragment::lastChild() const 
{ 
    return m_fragment ? m_fragment->lastChild() : 0; 
}

void ReplacementFragment::removeNodePreservingChildren(PassRefPtr<Node> node)
{
    if (!node)
        return;

    while (RefPtr<Node> n = node->firstChild()) {
        removeNode(n);
        insertNodeBefore(n.release(), node.get());
    }
    removeNode(node);
}

void ReplacementFragment::removeNode(PassRefPtr<Node> node)
{
    if (!node)
        return;
    
    ContainerNode* parent = node->nonShadowBoundaryParentNode();
    if (!parent)
        return;
    
    parent->removeChild(node.get(), ASSERT_NO_EXCEPTION);
}

void ReplacementFragment::insertNodeBefore(PassRefPtr<Node> node, Node* refNode)
{
    if (!node || !refNode)
        return;
        
    ContainerNode* parent = refNode->nonShadowBoundaryParentNode();
    if (!parent)
        return;
        
    parent->insertBefore(node, refNode, ASSERT_NO_EXCEPTION);
}

PassRefPtr<StyledElement> ReplacementFragment::insertFragmentForTestRendering(Node* rootEditableElement)
{
    RefPtr<StyledElement> holder = createDefaultParagraphElement(m_document.get());

    holder->appendChild(m_fragment, ASSERT_NO_EXCEPTION);
    rootEditableElement->appendChild(holder.get(), ASSERT_NO_EXCEPTION);
    m_document->updateLayoutIgnorePendingStylesheets();

    return holder.release();
}

void ReplacementFragment::restoreAndRemoveTestRenderingNodesToFragment(StyledElement* holder)
{
    if (!holder)
        return;
    
    while (RefPtr<Node> node = holder->firstChild()) {
        holder->removeChild(node.get(), ASSERT_NO_EXCEPTION);
        m_fragment->appendChild(node.get(), ASSERT_NO_EXCEPTION);
    }

    removeNode(holder);
}

void ReplacementFragment::removeUnrenderedNodes(Node* holder)
{
    Vector<RefPtr<Node> > unrendered;

    for (Node* node = holder->firstChild(); node; node = NodeTraversal::next(node, holder))
        if (!isNodeRendered(node) && !isTableStructureNode(node))
            unrendered.append(node);

    size_t n = unrendered.size();
    for (size_t i = 0; i < n; ++i)
        removeNode(unrendered[i]);
}

void ReplacementFragment::removeInterchangeNodes(Node* container)
{
    m_hasInterchangeNewlineAtStart = false;
    m_hasInterchangeNewlineAtEnd = false;

    // Interchange newlines at the "start" of the incoming fragment must be
    // either the first node in the fragment or the first leaf in the fragment.
    Node* node = container->firstChild();
    while (node) {
        if (isInterchangeNewlineNode(node)) {
            m_hasInterchangeNewlineAtStart = true;
            removeNode(node);
            break;
        }
        node = node->firstChild();
    }
    if (!container->hasChildNodes())
        return;
    // Interchange newlines at the "end" of the incoming fragment must be
    // either the last node in the fragment or the last leaf in the fragment.
    node = container->lastChild();
    while (node) {
        if (isInterchangeNewlineNode(node)) {
            m_hasInterchangeNewlineAtEnd = true;
            removeNode(node);
            break;
        }
        node = node->lastChild();
    }
    
    node = container->firstChild();
    while (node) {
        RefPtr<Node> next = NodeTraversal::next(node);
        if (isInterchangeConvertedSpaceSpan(node)) {
            next = NodeTraversal::nextSkippingChildren(node);
            removeNodePreservingChildren(node);
        }
        node = next.get();
    }
}

inline void ReplaceSelectionCommand::InsertedNodes::respondToNodeInsertion(Node* node)
{
    if (!node)
        return;
    
    if (!m_firstNodeInserted)
        m_firstNodeInserted = node;
    
    m_lastNodeInserted = node;
}

inline void ReplaceSelectionCommand::InsertedNodes::willRemoveNodePreservingChildren(Node* node)
{
    if (m_firstNodeInserted == node)
        m_firstNodeInserted = NodeTraversal::next(node);
    if (m_lastNodeInserted == node)
        m_lastNodeInserted = node->lastChild() ? node->lastChild() : NodeTraversal::nextSkippingChildren(node);
}

inline void ReplaceSelectionCommand::InsertedNodes::willRemoveNode(Node* node)
{
    if (m_firstNodeInserted == node && m_lastNodeInserted == node) {
        m_firstNodeInserted = 0;
        m_lastNodeInserted = 0;
    } else if (m_firstNodeInserted == node)
        m_firstNodeInserted = NodeTraversal::nextSkippingChildren(m_firstNodeInserted.get());
    else if (m_lastNodeInserted == node)
        m_lastNodeInserted = NodeTraversal::previousSkippingChildren(m_lastNodeInserted.get());
}

inline void ReplaceSelectionCommand::InsertedNodes::didReplaceNode(Node* node, Node* newNode)
{
    if (m_firstNodeInserted == node)
        m_firstNodeInserted = newNode;
    if (m_lastNodeInserted == node)
        m_lastNodeInserted = newNode;
}

ReplaceSelectionCommand::ReplaceSelectionCommand(Document* document, PassRefPtr<DocumentFragment> fragment, CommandOptions options, EditAction editAction)
    : CompositeEditCommand(document)
    , m_selectReplacement(options & SelectReplacement)
    , m_smartReplace(options & SmartReplace)
    , m_matchStyle(options & MatchStyle)
    , m_documentFragment(fragment)
    , m_preventNesting(options & PreventNesting)
    , m_movingParagraph(options & MovingParagraph)
    , m_editAction(editAction)
    , m_sanitizeFragment(options & SanitizeFragment)
    , m_shouldMergeEnd(false)
{
}

static bool hasMatchingQuoteLevel(VisiblePosition endOfExistingContent, VisiblePosition endOfInsertedContent)
{
    Position existing = endOfExistingContent.deepEquivalent();
    Position inserted = endOfInsertedContent.deepEquivalent();
    bool isInsideMailBlockquote = enclosingNodeOfType(inserted, isMailBlockquote, CanCrossEditingBoundary);
    return isInsideMailBlockquote && (numEnclosingMailBlockquotes(existing) == numEnclosingMailBlockquotes(inserted));
}

bool ReplaceSelectionCommand::shouldMergeStart(bool selectionStartWasStartOfParagraph, bool fragmentHasInterchangeNewlineAtStart, bool selectionStartWasInsideMailBlockquote)
{
    if (m_movingParagraph)
        return false;
    
    VisiblePosition startOfInsertedContent(positionAtStartOfInsertedContent());
    VisiblePosition prev = startOfInsertedContent.previous(CannotCrossEditingBoundary);
    if (prev.isNull())
        return false;
    
    // When we have matching quote levels, its ok to merge more frequently.
    // For a successful merge, we still need to make sure that the inserted content starts with the beginning of a paragraph.
    // And we should only merge here if the selection start was inside a mail blockquote.  This prevents against removing a 
    // blockquote from newly pasted quoted content that was pasted into an unquoted position.  If that unquoted position happens 
    // to be right after another blockquote, we don't want to merge and risk stripping a valid block (and newline) from the pasted content.
    if (isStartOfParagraph(startOfInsertedContent) && selectionStartWasInsideMailBlockquote && hasMatchingQuoteLevel(prev, positionAtEndOfInsertedContent()))
        return true;

    return !selectionStartWasStartOfParagraph
        && !fragmentHasInterchangeNewlineAtStart
        && isStartOfParagraph(startOfInsertedContent)
        && !startOfInsertedContent.deepEquivalent().deprecatedNode()->hasTagName(brTag)
        && shouldMerge(startOfInsertedContent, prev);
}

bool ReplaceSelectionCommand::shouldMergeEnd(bool selectionEndWasEndOfParagraph)
{
    VisiblePosition endOfInsertedContent(positionAtEndOfInsertedContent());
    VisiblePosition next = endOfInsertedContent.next(CannotCrossEditingBoundary);
    if (next.isNull())
        return false;

    return !selectionEndWasEndOfParagraph
        && isEndOfParagraph(endOfInsertedContent)
        && !endOfInsertedContent.deepEquivalent().deprecatedNode()->hasTagName(brTag)
        && shouldMerge(endOfInsertedContent, next);
}

static bool isMailPasteAsQuotationNode(const Node* node)
{
    return node && node->hasTagName(blockquoteTag) && node->isElementNode() && toElement(node)->getAttribute(classAttr) == ApplePasteAsQuotation;
}

static bool isHeaderElement(const Node* a)
{
    if (!a)
        return false;
        
    return a->hasTagName(h1Tag)
        || a->hasTagName(h2Tag)
        || a->hasTagName(h3Tag)
        || a->hasTagName(h4Tag)
        || a->hasTagName(h5Tag)
        || a->hasTagName(h6Tag);
}

static bool haveSameTagName(Node* a, Node* b)
{
    return a && b && a->isElementNode() && b->isElementNode() && toElement(a)->tagName() == toElement(b)->tagName();
}

bool ReplaceSelectionCommand::shouldMerge(const VisiblePosition& source, const VisiblePosition& destination)
{
    if (source.isNull() || destination.isNull())
        return false;
        
    Node* sourceNode = source.deepEquivalent().deprecatedNode();
    Node* destinationNode = destination.deepEquivalent().deprecatedNode();
    Node* sourceBlock = enclosingBlock(sourceNode);
    Node* destinationBlock = enclosingBlock(destinationNode);
    return !enclosingNodeOfType(source.deepEquivalent(), &isMailPasteAsQuotationNode) &&
           sourceBlock && (!sourceBlock->hasTagName(blockquoteTag) || isMailBlockquote(sourceBlock))  &&
           enclosingListChild(sourceBlock) == enclosingListChild(destinationNode) &&
           enclosingTableCell(source.deepEquivalent()) == enclosingTableCell(destination.deepEquivalent()) &&
           (!isHeaderElement(sourceBlock) || haveSameTagName(sourceBlock, destinationBlock)) &&
           // Don't merge to or from a position before or after a block because it would
           // be a no-op and cause infinite recursion.
           !isBlock(sourceNode) && !isBlock(destinationNode);
}

// Style rules that match just inserted elements could change their appearance, like
// a div inserted into a document with div { display:inline; }.
void ReplaceSelectionCommand::removeRedundantStylesAndKeepStyleSpanInline(InsertedNodes& insertedNodes)
{
    RefPtr<Node> pastEndNode = insertedNodes.pastLastLeaf();
    RefPtr<Node> next;
    for (RefPtr<Node> node = insertedNodes.firstNodeInserted(); node && node != pastEndNode; node = next) {
        // FIXME: <rdar://problem/5371536> Style rules that match pasted content can change it's appearance

        next = NodeTraversal::next(node.get());
        if (!node->isStyledElement())
            continue;

        StyledElement* element = static_cast<StyledElement*>(node.get());

        const StylePropertySet* inlineStyle = element->inlineStyle();
        RefPtr<EditingStyle> newInlineStyle = EditingStyle::create(inlineStyle);
        if (inlineStyle) {
            if (element->isHTMLElement()) {
                Vector<QualifiedName> attributes;
                HTMLElement* htmlElement = toHTMLElement(element);

                if (newInlineStyle->conflictsWithImplicitStyleOfElement(htmlElement)) {
                    // e.g. <b style="font-weight: normal;"> is converted to <span style="font-weight: normal;">
                    node = replaceElementWithSpanPreservingChildrenAndAttributes(htmlElement);
                    element = static_cast<StyledElement*>(node.get());
                    insertedNodes.didReplaceNode(htmlElement, node.get());
                } else if (newInlineStyle->extractConflictingImplicitStyleOfAttributes(htmlElement, EditingStyle::PreserveWritingDirection, 0, attributes,
                    EditingStyle::DoNotExtractMatchingStyle)) {
                    // e.g. <font size="3" style="font-size: 20px;"> is converted to <font style="font-size: 20px;">
                    for (size_t i = 0; i < attributes.size(); i++)
                        removeNodeAttribute(element, attributes[i]);
                }
            }

            ContainerNode* context = element->parentNode();

            // If Mail wraps the fragment with a Paste as Quotation blockquote, or if you're pasting into a quoted region,
            // styles from blockquoteNode are allowed to override those from the source document, see <rdar://problem/4930986> and <rdar://problem/5089327>.
            Node* blockquoteNode = isMailPasteAsQuotationNode(context) ? context : enclosingNodeOfType(firstPositionInNode(context), isMailBlockquote, CanCrossEditingBoundary);
            if (blockquoteNode)
                newInlineStyle->removeStyleFromRulesAndContext(element, document()->documentElement());

            newInlineStyle->removeStyleFromRulesAndContext(element, context);
        }

        if (!inlineStyle || newInlineStyle->isEmpty()) {
            if (isStyleSpanOrSpanWithOnlyStyleAttribute(element) || isEmptyFontTag(element, AllowNonEmptyStyleAttribute)) {
                insertedNodes.willRemoveNodePreservingChildren(element);
                removeNodePreservingChildren(element);
                continue;
            }
            removeNodeAttribute(element, styleAttr);
        } else if (newInlineStyle->style()->propertyCount() != inlineStyle->propertyCount())
            setNodeAttribute(element, styleAttr, newInlineStyle->style()->asText());

        // FIXME: Tolerate differences in id, class, and style attributes.
        if (isNonTableCellHTMLBlockElement(element) && areIdenticalElements(element, element->parentNode())
            && VisiblePosition(firstPositionInNode(element->parentNode())) == VisiblePosition(firstPositionInNode(element))
            && VisiblePosition(lastPositionInNode(element->parentNode())) == VisiblePosition(lastPositionInNode(element))) {
            insertedNodes.willRemoveNodePreservingChildren(element);
            removeNodePreservingChildren(element);
            continue;
        }

        if (element->parentNode()->rendererIsRichlyEditable())
            removeNodeAttribute(element, contenteditableAttr);

        // WebKit used to not add display: inline and float: none on copy.
        // Keep this code around for backward compatibility
        if (isLegacyAppleStyleSpan(element)) {
            if (!element->firstChild()) {
                insertedNodes.willRemoveNodePreservingChildren(element);
                removeNodePreservingChildren(element);
                continue;
            }
            // There are other styles that style rules can give to style spans,
            // but these are the two important ones because they'll prevent
            // inserted content from appearing in the right paragraph.
            // FIXME: Hyatt is concerned that selectively using display:inline will give inconsistent
            // results. We already know one issue because td elements ignore their display property
            // in quirks mode (which Mail.app is always in). We should look for an alternative.

            // Mutate using the CSSOM wrapper so we get the same event behavior as a script.
            if (isBlock(element))
                element->style()->setPropertyInternal(CSSPropertyDisplay, "inline", false, IGNORE_EXCEPTION);
            if (element->renderer() && element->renderer()->style()->isFloating())
                element->style()->setPropertyInternal(CSSPropertyFloat, "none", false, IGNORE_EXCEPTION);
        }
    }
}

static bool isProhibitedParagraphChild(const AtomicString& name)
{
    // https://dvcs.w3.org/hg/editing/raw-file/57abe6d3cb60/editing.html#prohibited-paragraph-child
    DEFINE_STATIC_LOCAL(HashSet<AtomicString>, elements, ());
    if (elements.isEmpty()) {
        elements.add(addressTag.localName());
        elements.add(articleTag.localName());
        elements.add(asideTag.localName());
        elements.add(blockquoteTag.localName());
        elements.add(captionTag.localName());
        elements.add(centerTag.localName());
        elements.add(colTag.localName());
        elements.add(colgroupTag.localName());
        elements.add(ddTag.localName());
        elements.add(detailsTag.localName());
        elements.add(dirTag.localName());
        elements.add(divTag.localName());
        elements.add(dlTag.localName());
        elements.add(dtTag.localName());
        elements.add(fieldsetTag.localName());
        elements.add(figcaptionTag.localName());
        elements.add(figureTag.localName());
        elements.add(footerTag.localName());
        elements.add(formTag.localName());
        elements.add(h1Tag.localName());
        elements.add(h2Tag.localName());
        elements.add(h3Tag.localName());
        elements.add(h4Tag.localName());
        elements.add(h5Tag.localName());
        elements.add(h6Tag.localName());
        elements.add(headerTag.localName());
        elements.add(hgroupTag.localName());
        elements.add(hrTag.localName());
        elements.add(liTag.localName());
        elements.add(listingTag.localName());
        elements.add(mainTag.localName()); // Missing in the specification.
        elements.add(menuTag.localName());
        elements.add(navTag.localName());
        elements.add(olTag.localName());
        elements.add(pTag.localName());
        elements.add(plaintextTag.localName());
        elements.add(preTag.localName());
        elements.add(sectionTag.localName());
        elements.add(summaryTag.localName());
        elements.add(tableTag.localName());
        elements.add(tbodyTag.localName());
        elements.add(tdTag.localName());
        elements.add(tfootTag.localName());
        elements.add(thTag.localName());
        elements.add(theadTag.localName());
        elements.add(trTag.localName());
        elements.add(ulTag.localName());
        elements.add(xmpTag.localName());
    }
    return elements.contains(name);
}

void ReplaceSelectionCommand::makeInsertedContentRoundTrippableWithHTMLTreeBuilder(InsertedNodes& insertedNodes)
{
    RefPtr<Node> pastEndNode = insertedNodes.pastLastLeaf();
    RefPtr<Node> next;
    for (RefPtr<Node> node = insertedNodes.firstNodeInserted(); node && node != pastEndNode; node = next) {
        next = NodeTraversal::next(node.get());

        if (!node->isHTMLElement())
            continue;

        if (isProhibitedParagraphChild(toHTMLElement(node.get())->localName())) {
            if (HTMLElement* paragraphElement = toHTMLElement(enclosingNodeWithTag(positionInParentBeforeNode(node.get()), pTag)))
                moveNodeOutOfAncestor(node, paragraphElement);
        }

        if (isHeaderElement(node.get())) {
            if (HTMLElement* headerElement = static_cast<HTMLElement*>(highestEnclosingNodeOfType(positionInParentBeforeNode(node.get()), isHeaderElement)))
                moveNodeOutOfAncestor(node, headerElement);
        }
    }
}

void ReplaceSelectionCommand::moveNodeOutOfAncestor(PassRefPtr<Node> prpNode, PassRefPtr<Node> prpAncestor)
{
    RefPtr<Node> node = prpNode;
    RefPtr<Node> ancestor = prpAncestor;

    VisiblePosition positionAtEndOfNode = lastPositionInOrAfterNode(node.get());
    VisiblePosition lastPositionInParagraph = lastPositionInNode(ancestor.get());
    if (positionAtEndOfNode == lastPositionInParagraph) {
        removeNode(node);
        if (ancestor->nextSibling())
            insertNodeBefore(node, ancestor->nextSibling());
        else
            appendNode(node, ancestor->parentNode());
    } else {
        RefPtr<Node> nodeToSplitTo = splitTreeToNode(node.get(), ancestor.get(), true);
        removeNode(node);
        insertNodeBefore(node, nodeToSplitTo);
    }
    if (!ancestor->firstChild())
        removeNode(ancestor.release());
}

static inline bool nodeHasVisibleRenderText(Text* text)
{
    return text->renderer() && toRenderText(text->renderer())->renderedTextLength() > 0;
}

void ReplaceSelectionCommand::removeUnrenderedTextNodesAtEnds(InsertedNodes& insertedNodes)
{
    document()->updateLayoutIgnorePendingStylesheets();

    Node* lastLeafInserted = insertedNodes.lastLeafInserted();
    if (lastLeafInserted && lastLeafInserted->isTextNode() && !nodeHasVisibleRenderText(toText(lastLeafInserted))
        && !enclosingNodeWithTag(firstPositionInOrBeforeNode(lastLeafInserted), selectTag)
        && !enclosingNodeWithTag(firstPositionInOrBeforeNode(lastLeafInserted), scriptTag)) {
        insertedNodes.willRemoveNode(lastLeafInserted);
        removeNode(lastLeafInserted);
    }

    // We don't have to make sure that firstNodeInserted isn't inside a select or script element
    // because it is a top level node in the fragment and the user can't insert into those elements.
    Node* firstNodeInserted = insertedNodes.firstNodeInserted();
    if (firstNodeInserted && firstNodeInserted->isTextNode() && !nodeHasVisibleRenderText(toText(firstNodeInserted))) {
        insertedNodes.willRemoveNode(firstNodeInserted);
        removeNode(firstNodeInserted);
    }
}

VisiblePosition ReplaceSelectionCommand::positionAtEndOfInsertedContent() const
{
    // FIXME: Why is this hack here?  What's special about <select> tags?
    Node* enclosingSelect = enclosingNodeWithTag(m_endOfInsertedContent, selectTag);
    return enclosingSelect ? lastPositionInOrAfterNode(enclosingSelect) : m_endOfInsertedContent;
}

VisiblePosition ReplaceSelectionCommand::positionAtStartOfInsertedContent() const
{
    return m_startOfInsertedContent;
}

static void removeHeadContents(ReplacementFragment& fragment)
{
    Node* next = 0;
    for (Node* node = fragment.firstChild(); node; node = next) {
        if (node->hasTagName(baseTag)
            || node->hasTagName(linkTag)
            || node->hasTagName(metaTag)
            || node->hasTagName(styleTag)
            || isHTMLTitleElement(node)) {
            next = NodeTraversal::nextSkippingChildren(node);
            fragment.removeNode(node);
        } else
            next = NodeTraversal::next(node);
    }
}

// Remove style spans before insertion if they are unnecessary.  It's faster because we'll 
// avoid doing a layout.
static bool handleStyleSpansBeforeInsertion(ReplacementFragment& fragment, const Position& insertionPos)
{
    Node* topNode = fragment.firstChild();

    // Handling the case where we are doing Paste as Quotation or pasting into quoted content is more complicated (see handleStyleSpans)
    // and doesn't receive the optimization.
    if (isMailPasteAsQuotationNode(topNode) || enclosingNodeOfType(firstPositionInOrBeforeNode(topNode), isMailBlockquote, CanCrossEditingBoundary))
        return false;

    // Either there are no style spans in the fragment or a WebKit client has added content to the fragment
    // before inserting it.  Look for and handle style spans after insertion.
    if (!isLegacyAppleStyleSpan(topNode))
        return false;

    Node* wrappingStyleSpan = topNode;
    RefPtr<EditingStyle> styleAtInsertionPos = EditingStyle::create(insertionPos.parentAnchoredEquivalent());
    String styleText = styleAtInsertionPos->style()->asText();

    // FIXME: This string comparison is a naive way of comparing two styles.
    // We should be taking the diff and check that the diff is empty.
    if (styleText != toElement(wrappingStyleSpan)->getAttribute(styleAttr))
        return false;

    fragment.removeNodePreservingChildren(wrappingStyleSpan);
    return true;
}

// At copy time, WebKit wraps copied content in a span that contains the source document's 
// default styles.  If the copied Range inherits any other styles from its ancestors, we put 
// those styles on a second span.
// This function removes redundant styles from those spans, and removes the spans if all their 
// styles are redundant. 
// We should remove the Apple-style-span class when we're done, see <rdar://problem/5685600>.
// We should remove styles from spans that are overridden by all of their children, either here
// or at copy time.
void ReplaceSelectionCommand::handleStyleSpans(InsertedNodes& insertedNodes)
{
    HTMLElement* wrappingStyleSpan = 0;
    // The style span that contains the source document's default style should be at
    // the top of the fragment, but Mail sometimes adds a wrapper (for Paste As Quotation),
    // so search for the top level style span instead of assuming it's at the top.
    for (Node* node = insertedNodes.firstNodeInserted(); node; node = NodeTraversal::next(node)) {
        if (isLegacyAppleStyleSpan(node)) {
            wrappingStyleSpan = toHTMLElement(node);
            break;
        }
    }
    
    // There might not be any style spans if we're pasting from another application or if 
    // we are here because of a document.execCommand("InsertHTML", ...) call.
    if (!wrappingStyleSpan)
        return;

    RefPtr<EditingStyle> style = EditingStyle::create(wrappingStyleSpan->inlineStyle());
    ContainerNode* context = wrappingStyleSpan->parentNode();

    // If Mail wraps the fragment with a Paste as Quotation blockquote, or if you're pasting into a quoted region,
    // styles from blockquoteNode are allowed to override those from the source document, see <rdar://problem/4930986> and <rdar://problem/5089327>.
    Node* blockquoteNode = isMailPasteAsQuotationNode(context) ? context : enclosingNodeOfType(firstPositionInNode(context), isMailBlockquote, CanCrossEditingBoundary);
    if (blockquoteNode)
        context = document()->documentElement();

    // This operation requires that only editing styles to be removed from sourceDocumentStyle.
    style->prepareToApplyAt(firstPositionInNode(context));

    // Remove block properties in the span's style. This prevents properties that probably have no effect 
    // currently from affecting blocks later if the style is cloned for a new block element during a future 
    // editing operation.
    // FIXME: They *can* have an effect currently if blocks beneath the style span aren't individually marked
    // with block styles by the editing engine used to style them.  WebKit doesn't do this, but others might.
    style->removeBlockProperties();

    if (style->isEmpty() || !wrappingStyleSpan->firstChild()) {
        insertedNodes.willRemoveNodePreservingChildren(wrappingStyleSpan);
        removeNodePreservingChildren(wrappingStyleSpan);
    } else
        setNodeAttribute(wrappingStyleSpan, styleAttr, style->style()->asText());
}

void ReplaceSelectionCommand::mergeEndIfNeeded()
{
    if (!m_shouldMergeEnd)
        return;

    VisiblePosition startOfInsertedContent(positionAtStartOfInsertedContent());
    VisiblePosition endOfInsertedContent(positionAtEndOfInsertedContent());
    
    // Bail to avoid infinite recursion.
    if (m_movingParagraph) {
        ASSERT_NOT_REACHED();
        return;
    }
    
    // Merging two paragraphs will destroy the moved one's block styles.  Always move the end of inserted forward 
    // to preserve the block style of the paragraph already in the document, unless the paragraph to move would 
    // include the what was the start of the selection that was pasted into, so that we preserve that paragraph's
    // block styles.
    bool mergeForward = !(inSameParagraph(startOfInsertedContent, endOfInsertedContent) && !isStartOfParagraph(startOfInsertedContent));
    
    VisiblePosition destination = mergeForward ? endOfInsertedContent.next() : endOfInsertedContent;
    VisiblePosition startOfParagraphToMove = mergeForward ? startOfParagraph(endOfInsertedContent) : endOfInsertedContent.next();
   
    // Merging forward could result in deleting the destination anchor node.
    // To avoid this, we add a placeholder node before the start of the paragraph.
    if (endOfParagraph(startOfParagraphToMove) == destination) {
        RefPtr<Node> placeholder = createBreakElement(document());
        insertNodeBefore(placeholder, startOfParagraphToMove.deepEquivalent().deprecatedNode());
        destination = VisiblePosition(positionBeforeNode(placeholder.get()));
    }

    moveParagraph(startOfParagraphToMove, endOfParagraph(startOfParagraphToMove), destination);
    
    // Merging forward will remove m_endOfInsertedContent from the document.
    if (mergeForward) {
        if (m_startOfInsertedContent.isOrphan())
            m_startOfInsertedContent = endingSelection().visibleStart().deepEquivalent();
         m_endOfInsertedContent = endingSelection().visibleEnd().deepEquivalent();
        // If we merged text nodes, m_endOfInsertedContent could be null. If this is the case, we use m_startOfInsertedContent.
        if (m_endOfInsertedContent.isNull())
            m_endOfInsertedContent = m_startOfInsertedContent;
    }
}

static Node* enclosingInline(Node* node)
{
    while (ContainerNode* parent = node->parentNode()) {
        if (isBlockFlowElement(parent) || parent->hasTagName(bodyTag))
            return node;
        // Stop if any previous sibling is a block.
        for (Node* sibling = node->previousSibling(); sibling; sibling = sibling->previousSibling()) {
            if (isBlockFlowElement(sibling))
                return node;
        }
        node = parent;
    }
    return node;
}

static bool isInlineNodeWithStyle(const Node* node)
{
    // We don't want to skip over any block elements.
    if (isBlock(node))
        return false;

    if (!node->isHTMLElement())
        return false;

    // We can skip over elements whose class attribute is
    // one of our internal classes.
    const HTMLElement* element = static_cast<const HTMLElement*>(node);
    const AtomicString& classAttributeValue = element->getAttribute(classAttr);
    if (classAttributeValue == AppleTabSpanClass
        || classAttributeValue == AppleConvertedSpace
        || classAttributeValue == ApplePasteAsQuotation)
        return true;

    return EditingStyle::elementIsStyledSpanOrHTMLEquivalent(element);
}

inline Node* nodeToSplitToAvoidPastingIntoInlineNodesWithStyle(const Position& insertionPos)
{
    Node* containgBlock = enclosingBlock(insertionPos.containerNode());
    return highestEnclosingNodeOfType(insertionPos, isInlineNodeWithStyle, CannotCrossEditingBoundary, containgBlock);
}

void ReplaceSelectionCommand::doApply()
{
    VisibleSelection selection = endingSelection();
    ASSERT(selection.isCaretOrRange());
    ASSERT(selection.start().deprecatedNode());
    if (!selection.isNonOrphanedCaretOrRange() || !selection.start().deprecatedNode())
        return;

    if (!selection.rootEditableElement())
        return;

    ReplacementFragment fragment(document(), m_documentFragment.get(), selection);
    if (performTrivialReplace(fragment))
        return;
    
    // We can skip matching the style if the selection is plain text.
    if ((selection.start().deprecatedNode()->renderer() && selection.start().deprecatedNode()->renderer()->style()->userModify() == READ_WRITE_PLAINTEXT_ONLY)
        && (selection.end().deprecatedNode()->renderer() && selection.end().deprecatedNode()->renderer()->style()->userModify() == READ_WRITE_PLAINTEXT_ONLY))
        m_matchStyle = false;
    
    if (m_matchStyle) {
        m_insertionStyle = EditingStyle::create(selection.start());
        m_insertionStyle->mergeTypingStyle(document());
    }

    VisiblePosition visibleStart = selection.visibleStart();
    VisiblePosition visibleEnd = selection.visibleEnd();
    
    bool selectionEndWasEndOfParagraph = isEndOfParagraph(visibleEnd);
    bool selectionStartWasStartOfParagraph = isStartOfParagraph(visibleStart);
    
    Node* startBlock = enclosingBlock(visibleStart.deepEquivalent().deprecatedNode());
    
    Position insertionPos = selection.start();
    bool startIsInsideMailBlockquote = enclosingNodeOfType(insertionPos, isMailBlockquote, CanCrossEditingBoundary);
    bool selectionIsPlainText = !selection.isContentRichlyEditable();
    Element* currentRoot = selection.rootEditableElement();

    if ((selectionStartWasStartOfParagraph && selectionEndWasEndOfParagraph && !startIsInsideMailBlockquote) ||
        startBlock == currentRoot || isListItem(startBlock) || selectionIsPlainText)
        m_preventNesting = false;
    
    if (selection.isRange()) {
        // When the end of the selection being pasted into is at the end of a paragraph, and that selection
        // spans multiple blocks, not merging may leave an empty line.
        // When the start of the selection being pasted into is at the start of a block, not merging 
        // will leave hanging block(s).
        // Merge blocks if the start of the selection was in a Mail blockquote, since we handle  
        // that case specially to prevent nesting. 
        bool mergeBlocksAfterDelete = startIsInsideMailBlockquote || isEndOfParagraph(visibleEnd) || isStartOfBlock(visibleStart);
        // FIXME: We should only expand to include fully selected special elements if we are copying a 
        // selection and pasting it on top of itself.
        deleteSelection(false, mergeBlocksAfterDelete, true, false);
        visibleStart = endingSelection().visibleStart();
        if (fragment.hasInterchangeNewlineAtStart()) {
            if (isEndOfParagraph(visibleStart) && !isStartOfParagraph(visibleStart)) {
                if (!isEndOfEditableOrNonEditableContent(visibleStart))
                    setEndingSelection(visibleStart.next());
            } else
                insertParagraphSeparator();
        }
        insertionPos = endingSelection().start();
    } else {
        ASSERT(selection.isCaret());
        if (fragment.hasInterchangeNewlineAtStart()) {
            VisiblePosition next = visibleStart.next(CannotCrossEditingBoundary);
            if (isEndOfParagraph(visibleStart) && !isStartOfParagraph(visibleStart) && next.isNotNull())
                setEndingSelection(next);
            else  {
                insertParagraphSeparator();
                visibleStart = endingSelection().visibleStart();
            }
        }
        // We split the current paragraph in two to avoid nesting the blocks from the fragment inside the current block.
        // For example paste <div>foo</div><div>bar</div><div>baz</div> into <div>x^x</div>, where ^ is the caret.  
        // As long as the  div styles are the same, visually you'd expect: <div>xbar</div><div>bar</div><div>bazx</div>, 
        // not <div>xbar<div>bar</div><div>bazx</div></div>.
        // Don't do this if the selection started in a Mail blockquote.
        if (m_preventNesting && !startIsInsideMailBlockquote && !isEndOfParagraph(visibleStart) && !isStartOfParagraph(visibleStart)) {
            insertParagraphSeparator();
            setEndingSelection(endingSelection().visibleStart().previous());
        }
        insertionPos = endingSelection().start();
    }
    
    // We don't want any of the pasted content to end up nested in a Mail blockquote, so first break 
    // out of any surrounding Mail blockquotes. Unless we're inserting in a table, in which case
    // breaking the blockquote will prevent the content from actually being inserted in the table.
    if (startIsInsideMailBlockquote && m_preventNesting && !(enclosingNodeOfType(insertionPos, &isTableStructureNode))) { 
        applyCommandToComposite(BreakBlockquoteCommand::create(document())); 
        // This will leave a br between the split. 
        Node* br = endingSelection().start().deprecatedNode(); 
        ASSERT(br->hasTagName(brTag)); 
        // Insert content between the two blockquotes, but remove the br (since it was just a placeholder). 
        insertionPos = positionInParentBeforeNode(br);
        removeNode(br);
    }
    
    // Inserting content could cause whitespace to collapse, e.g. inserting <div>foo</div> into hello^ world.
    prepareWhitespaceAtPositionForSplit(insertionPos);

    // If the downstream node has been removed there's no point in continuing.
    if (!insertionPos.downstream().deprecatedNode())
      return;
    
    // NOTE: This would be an incorrect usage of downstream() if downstream() were changed to mean the last position after 
    // p that maps to the same visible position as p (since in the case where a br is at the end of a block and collapsed 
    // away, there are positions after the br which map to the same visible position as [br, 0]).  
    Node* endBR = insertionPos.downstream().deprecatedNode()->hasTagName(brTag) ? insertionPos.downstream().deprecatedNode() : 0;
    VisiblePosition originalVisPosBeforeEndBR;
    if (endBR)
        originalVisPosBeforeEndBR = VisiblePosition(positionBeforeNode(endBR), DOWNSTREAM).previous();
    
    startBlock = enclosingBlock(insertionPos.deprecatedNode());
    
    // Adjust insertionPos to prevent nesting.
    // If the start was in a Mail blockquote, we will have already handled adjusting insertionPos above.
    if (m_preventNesting && startBlock && !isTableCell(startBlock) && !startIsInsideMailBlockquote) {
        ASSERT(startBlock != currentRoot);
        VisiblePosition visibleInsertionPos(insertionPos);
        if (isEndOfBlock(visibleInsertionPos) && !(isStartOfBlock(visibleInsertionPos) && fragment.hasInterchangeNewlineAtEnd()))
            insertionPos = positionInParentAfterNode(startBlock);
        else if (isStartOfBlock(visibleInsertionPos))
            insertionPos = positionInParentBeforeNode(startBlock);
    }
    
    // Paste at start or end of link goes outside of link.
    insertionPos = positionAvoidingSpecialElementBoundary(insertionPos);
    
    // FIXME: Can this wait until after the operation has been performed?  There doesn't seem to be
    // any work performed after this that queries or uses the typing style.
    if (Frame* frame = document()->frame())
        frame->selection()->clearTypingStyle();

    removeHeadContents(fragment);

    // We don't want the destination to end up inside nodes that weren't selected.  To avoid that, we move the
    // position forward without changing the visible position so we're still at the same visible location, but
    // outside of preceding tags.
    insertionPos = positionAvoidingPrecedingNodes(insertionPos);

    // Paste into run of tabs splits the tab span.
    insertionPos = positionOutsideTabSpan(insertionPos);

    bool handledStyleSpans = handleStyleSpansBeforeInsertion(fragment, insertionPos);

    // We're finished if there is nothing to add.
    if (fragment.isEmpty() || !fragment.firstChild())
        return;

    // If we are not trying to match the destination style we prefer a position
    // that is outside inline elements that provide style.
    // This way we can produce a less verbose markup.
    // We can skip this optimization for fragments not wrapped in one of
    // our style spans and for positions inside list items
    // since insertAsListItems already does the right thing.
    if (!m_matchStyle && !enclosingList(insertionPos.containerNode())) {
        if (insertionPos.containerNode()->isTextNode() && insertionPos.offsetInContainerNode() && !insertionPos.atLastEditingPositionForNode()) {
            splitTextNode(insertionPos.containerText(), insertionPos.offsetInContainerNode());
            insertionPos = firstPositionInNode(insertionPos.containerNode());
        }

        if (RefPtr<Node> nodeToSplitTo = nodeToSplitToAvoidPastingIntoInlineNodesWithStyle(insertionPos)) {
            if (insertionPos.containerNode() != nodeToSplitTo->parentNode()) {
                Node* splitStart = insertionPos.computeNodeAfterPosition();
                if (!splitStart)
                    splitStart = insertionPos.containerNode();
                nodeToSplitTo = splitTreeToNode(splitStart, nodeToSplitTo->parentNode()).get();
                insertionPos = positionInParentBeforeNode(nodeToSplitTo.get());
            }
        }
    }

    // FIXME: When pasting rich content we're often prevented from heading down the fast path by style spans.  Try
    // again here if they've been removed.

    // 1) Insert the content.
    // 2) Remove redundant styles and style tags, this inner <b> for example: <b>foo <b>bar</b> baz</b>.
    // 3) Merge the start of the added content with the content before the position being pasted into.
    // 4) Do one of the following: a) expand the last br if the fragment ends with one and it collapsed,
    // b) merge the last paragraph of the incoming fragment with the paragraph that contained the 
    // end of the selection that was pasted into, or c) handle an interchange newline at the end of the 
    // incoming fragment.
    // 5) Add spaces for smart replace.
    // 6) Select the replacement if requested, and match style if requested.

    InsertedNodes insertedNodes;
    RefPtr<Node> refNode = fragment.firstChild();
    RefPtr<Node> node = refNode->nextSibling();
    
    fragment.removeNode(refNode);

    Node* blockStart = enclosingBlock(insertionPos.deprecatedNode());
    if ((isListElement(refNode.get()) || (isLegacyAppleStyleSpan(refNode.get()) && isListElement(refNode->firstChild())))
        && blockStart && blockStart->renderer()->isListItem())
        refNode = insertAsListItems(toHTMLElement(refNode.get()), blockStart, insertionPos, insertedNodes);
    else {
        insertNodeAt(refNode, insertionPos);
        insertedNodes.respondToNodeInsertion(refNode.get());
    }

    // Mutation events (bug 22634) may have already removed the inserted content
    if (!refNode->inDocument())
        return;

    bool plainTextFragment = isPlainTextMarkup(refNode.get());

    while (node) {
        RefPtr<Node> next = node->nextSibling();
        fragment.removeNode(node.get());
        insertNodeAfter(node, refNode.get());
        insertedNodes.respondToNodeInsertion(node.get());

        // Mutation events (bug 22634) may have already removed the inserted content
        if (!node->inDocument())
            return;

        refNode = node;
        if (node && plainTextFragment)
            plainTextFragment = isPlainTextMarkup(node.get());
        node = next;
    }

    removeUnrenderedTextNodesAtEnds(insertedNodes);

    if (!handledStyleSpans)
        handleStyleSpans(insertedNodes);

    // Mutation events (bug 20161) may have already removed the inserted content
    if (!insertedNodes.firstNodeInserted() || !insertedNodes.firstNodeInserted()->inDocument())
        return;

    VisiblePosition startOfInsertedContent = firstPositionInOrBeforeNode(insertedNodes.firstNodeInserted());

    // We inserted before the startBlock to prevent nesting, and the content before the startBlock wasn't in its own block and
    // didn't have a br after it, so the inserted content ended up in the same paragraph.
    if (startBlock && insertionPos.deprecatedNode() == startBlock->parentNode() && (unsigned)insertionPos.deprecatedEditingOffset() < startBlock->nodeIndex() && !isStartOfParagraph(startOfInsertedContent))
        insertNodeAt(createBreakElement(document()).get(), startOfInsertedContent.deepEquivalent());

    if (endBR && (plainTextFragment || shouldRemoveEndBR(endBR, originalVisPosBeforeEndBR))) {
        RefPtr<Node> parent = endBR->parentNode();
        insertedNodes.willRemoveNode(endBR);
        removeNode(endBR);
        if (Node* nodeToRemove = highestNodeToRemoveInPruning(parent.get())) {
            insertedNodes.willRemoveNode(nodeToRemove);
            removeNode(nodeToRemove);
        }
    }
    
    makeInsertedContentRoundTrippableWithHTMLTreeBuilder(insertedNodes);

    removeRedundantStylesAndKeepStyleSpanInline(insertedNodes);

    if (m_sanitizeFragment)
        applyCommandToComposite(SimplifyMarkupCommand::create(document(), insertedNodes.firstNodeInserted(), insertedNodes.pastLastLeaf()));

    // Setup m_startOfInsertedContent and m_endOfInsertedContent. This should be the last two lines of code that access insertedNodes.
    m_startOfInsertedContent = firstPositionInOrBeforeNode(insertedNodes.firstNodeInserted());
    m_endOfInsertedContent = lastPositionInOrAfterNode(insertedNodes.lastLeafInserted());

    // Determine whether or not we should merge the end of inserted content with what's after it before we do
    // the start merge so that the start merge doesn't effect our decision.
    m_shouldMergeEnd = shouldMergeEnd(selectionEndWasEndOfParagraph);
    
    if (shouldMergeStart(selectionStartWasStartOfParagraph, fragment.hasInterchangeNewlineAtStart(), startIsInsideMailBlockquote)) {
        VisiblePosition startOfParagraphToMove = positionAtStartOfInsertedContent();
        VisiblePosition destination = startOfParagraphToMove.previous();
        // We need to handle the case where we need to merge the end
        // but our destination node is inside an inline that is the last in the block.
        // We insert a placeholder before the newly inserted content to avoid being merged into the inline.
        Node* destinationNode = destination.deepEquivalent().deprecatedNode();
        if (m_shouldMergeEnd && destinationNode != enclosingInline(destinationNode) && enclosingInline(destinationNode)->nextSibling())
            insertNodeBefore(createBreakElement(document()), refNode.get());
        
        // Merging the the first paragraph of inserted content with the content that came
        // before the selection that was pasted into would also move content after 
        // the selection that was pasted into if: only one paragraph was being pasted, 
        // and it was not wrapped in a block, the selection that was pasted into ended 
        // at the end of a block and the next paragraph didn't start at the start of a block.
        // Insert a line break just after the inserted content to separate it from what 
        // comes after and prevent that from happening.
        VisiblePosition endOfInsertedContent = positionAtEndOfInsertedContent();
        if (startOfParagraph(endOfInsertedContent) == startOfParagraphToMove) {
            insertNodeAt(createBreakElement(document()).get(), endOfInsertedContent.deepEquivalent());
            // Mutation events (bug 22634) triggered by inserting the <br> might have removed the content we're about to move
            if (!startOfParagraphToMove.deepEquivalent().anchorNode()->inDocument())
                return;
        }

        // FIXME: Maintain positions for the start and end of inserted content instead of keeping nodes.  The nodes are
        // only ever used to create positions where inserted content starts/ends.
        moveParagraph(startOfParagraphToMove, endOfParagraph(startOfParagraphToMove), destination);
        m_startOfInsertedContent = endingSelection().visibleStart().deepEquivalent().downstream();
        if (m_endOfInsertedContent.isOrphan())
            m_endOfInsertedContent = endingSelection().visibleEnd().deepEquivalent().upstream();
    }

    Position lastPositionToSelect;
    if (fragment.hasInterchangeNewlineAtEnd()) {
        VisiblePosition endOfInsertedContent = positionAtEndOfInsertedContent();
        VisiblePosition next = endOfInsertedContent.next(CannotCrossEditingBoundary);

        if (selectionEndWasEndOfParagraph || !isEndOfParagraph(endOfInsertedContent) || next.isNull()) {
            if (!isStartOfParagraph(endOfInsertedContent)) {
                setEndingSelection(endOfInsertedContent);
                Node* enclosingNode = enclosingBlock(endOfInsertedContent.deepEquivalent().deprecatedNode());
                if (isListItem(enclosingNode)) {
                    RefPtr<Node> newListItem = createListItemElement(document());
                    insertNodeAfter(newListItem, enclosingNode);
                    setEndingSelection(VisiblePosition(firstPositionInNode(newListItem.get())));
                } else {
                    // Use a default paragraph element (a plain div) for the empty paragraph, using the last paragraph
                    // block's style seems to annoy users.
                    insertParagraphSeparator(true, !startIsInsideMailBlockquote && highestEnclosingNodeOfType(endOfInsertedContent.deepEquivalent(),
                        isMailBlockquote, CannotCrossEditingBoundary, insertedNodes.firstNodeInserted()->parentNode()));
                }

                // Select up to the paragraph separator that was added.
                lastPositionToSelect = endingSelection().visibleStart().deepEquivalent();
                updateNodesInserted(lastPositionToSelect.deprecatedNode());
            }
        } else {
            // Select up to the beginning of the next paragraph.
            lastPositionToSelect = next.deepEquivalent().downstream();
        }
        
    } else
        mergeEndIfNeeded();

    if (Node* mailBlockquote = enclosingNodeOfType(positionAtStartOfInsertedContent().deepEquivalent(), isMailPasteAsQuotationNode))
        removeNodeAttribute(toElement(mailBlockquote), classAttr);

    if (shouldPerformSmartReplace())
        addSpacesForSmartReplace();

    // If we are dealing with a fragment created from plain text
    // no style matching is necessary.
    if (plainTextFragment)
        m_matchStyle = false;
        
    completeHTMLReplacement(lastPositionToSelect);
}

bool ReplaceSelectionCommand::shouldRemoveEndBR(Node* endBR, const VisiblePosition& originalVisPosBeforeEndBR)
{
    if (!endBR || !endBR->inDocument())
        return false;
        
    VisiblePosition visiblePos(positionBeforeNode(endBR));
    
    // Don't remove the br if nothing was inserted.
    if (visiblePos.previous() == originalVisPosBeforeEndBR)
        return false;
    
    // Remove the br if it is collapsed away and so is unnecessary.
    if (!document()->inNoQuirksMode() && isEndOfBlock(visiblePos) && !isStartOfParagraph(visiblePos))
        return true;
        
    // A br that was originally holding a line open should be displaced by inserted content or turned into a line break.
    // A br that was originally acting as a line break should still be acting as a line break, not as a placeholder.
    return isStartOfParagraph(visiblePos) && isEndOfParagraph(visiblePos);
}

bool ReplaceSelectionCommand::shouldPerformSmartReplace() const
{
    if (!m_smartReplace)
        return false;

    Element* textControl = enclosingTextFormControl(positionAtStartOfInsertedContent().deepEquivalent());
    if (textControl && isHTMLInputElement(textControl) && toHTMLInputElement(textControl)->isPasswordField())
        return false; // Disable smart replace for password fields.

    return true;
}

static bool isCharacterSmartReplaceExemptConsideringNonBreakingSpace(UChar32 character, bool previousCharacter)
{
    return isCharacterSmartReplaceExempt(character == noBreakSpace ? ' ' : character, previousCharacter);
}

void ReplaceSelectionCommand::addSpacesForSmartReplace()
{
    VisiblePosition startOfInsertedContent = positionAtStartOfInsertedContent();
    VisiblePosition endOfInsertedContent = positionAtEndOfInsertedContent();

    Position endUpstream = endOfInsertedContent.deepEquivalent().upstream();
    Node* endNode = endUpstream.computeNodeBeforePosition();
    int endOffset = endNode && endNode->isTextNode() ? toText(endNode)->length() : 0;
    if (endUpstream.anchorType() == Position::PositionIsOffsetInAnchor) {
        endNode = endUpstream.containerNode();
        endOffset = endUpstream.offsetInContainerNode();
    }

    bool needsTrailingSpace = !isEndOfParagraph(endOfInsertedContent) && !isCharacterSmartReplaceExemptConsideringNonBreakingSpace(endOfInsertedContent.characterAfter(), false);
    if (needsTrailingSpace && endNode) {
        bool collapseWhiteSpace = !endNode->renderer() || endNode->renderer()->style()->collapseWhiteSpace();
        if (endNode->isTextNode()) {
            insertTextIntoNode(toText(endNode), endOffset, collapseWhiteSpace ? nonBreakingSpaceString() : " ");
            if (m_endOfInsertedContent.containerNode() == endNode)
                m_endOfInsertedContent.moveToOffset(m_endOfInsertedContent.offsetInContainerNode() + 1);
        } else {
            RefPtr<Node> node = document()->createEditingTextNode(collapseWhiteSpace ? nonBreakingSpaceString() : " ");
            insertNodeAfter(node, endNode);
            updateNodesInserted(node.get());
        }
    }

    document()->updateLayout();

    Position startDownstream = startOfInsertedContent.deepEquivalent().downstream();
    Node* startNode = startDownstream.computeNodeAfterPosition();
    unsigned startOffset = 0;
    if (startDownstream.anchorType() == Position::PositionIsOffsetInAnchor) {
        startNode = startDownstream.containerNode();
        startOffset = startDownstream.offsetInContainerNode();
    }

    bool needsLeadingSpace = !isStartOfParagraph(startOfInsertedContent) && !isCharacterSmartReplaceExemptConsideringNonBreakingSpace(startOfInsertedContent.previous().characterAfter(), true);
    if (needsLeadingSpace && startNode) {
        bool collapseWhiteSpace = !startNode->renderer() || startNode->renderer()->style()->collapseWhiteSpace();
        if (startNode->isTextNode()) {
            insertTextIntoNode(toText(startNode), startOffset, collapseWhiteSpace ? nonBreakingSpaceString() : " ");
            if (m_endOfInsertedContent.containerNode() == startNode && m_endOfInsertedContent.offsetInContainerNode())
                m_endOfInsertedContent.moveToOffset(m_endOfInsertedContent.offsetInContainerNode() + 1);
        } else {
            RefPtr<Node> node = document()->createEditingTextNode(collapseWhiteSpace ? nonBreakingSpaceString() : " ");
            // Don't updateNodesInserted. Doing so would set m_endOfInsertedContent to be the node containing the leading space,
            // but m_endOfInsertedContent is supposed to mark the end of pasted content.
            insertNodeBefore(node, startNode);
            m_startOfInsertedContent = firstPositionInNode(node.get());
        }
    }
}

void ReplaceSelectionCommand::completeHTMLReplacement(const Position &lastPositionToSelect)
{
    Position start = positionAtStartOfInsertedContent().deepEquivalent();
    Position end = positionAtEndOfInsertedContent().deepEquivalent();

    // Mutation events may have deleted start or end
    if (start.isNotNull() && !start.isOrphan() && end.isNotNull() && !end.isOrphan()) {
        // FIXME (11475): Remove this and require that the creator of the fragment to use nbsps.
        rebalanceWhitespaceAt(start);
        rebalanceWhitespaceAt(end);

        if (m_matchStyle) {
            ASSERT(m_insertionStyle);
            applyStyle(m_insertionStyle.get(), start, end);
        }

        if (lastPositionToSelect.isNotNull())
            end = lastPositionToSelect;

        mergeTextNodesAroundPosition(start, end);
        mergeTextNodesAroundPosition(end, start);
    } else if (lastPositionToSelect.isNotNull())
        start = end = lastPositionToSelect;
    else
        return;

    if (m_selectReplacement)
        setEndingSelection(VisibleSelection(start, end, SEL_DEFAULT_AFFINITY, endingSelection().isDirectional()));
    else
        setEndingSelection(VisibleSelection(end, SEL_DEFAULT_AFFINITY, endingSelection().isDirectional()));
}

void ReplaceSelectionCommand::mergeTextNodesAroundPosition(Position& position, Position& positionOnlyToBeUpdated)
{
    bool positionIsOffsetInAnchor = position.anchorType() == Position::PositionIsOffsetInAnchor;
    bool positionOnlyToBeUpdatedIsOffsetInAnchor = positionOnlyToBeUpdated.anchorType() == Position::PositionIsOffsetInAnchor;
    RefPtr<Text> text = 0;
    if (positionIsOffsetInAnchor && position.containerNode() && position.containerNode()->isTextNode())
        text = toText(position.containerNode());
    else {
        Node* before = position.computeNodeBeforePosition();
        if (before && before->isTextNode())
            text = toText(before);
        else {
            Node* after = position.computeNodeAfterPosition();
            if (after && after->isTextNode())
                text = toText(after);
        }
    }
    if (!text)
        return;

    if (text->previousSibling() && text->previousSibling()->isTextNode()) {
        RefPtr<Text> previous = toText(text->previousSibling());
        insertTextIntoNode(text, 0, previous->data());

        if (positionIsOffsetInAnchor)
            position.moveToOffset(previous->length() + position.offsetInContainerNode());
        else
            updatePositionForNodeRemoval(position, previous.get());

        if (positionOnlyToBeUpdatedIsOffsetInAnchor) {
            if (positionOnlyToBeUpdated.containerNode() == text)
                positionOnlyToBeUpdated.moveToOffset(previous->length() + positionOnlyToBeUpdated.offsetInContainerNode());
            else if (positionOnlyToBeUpdated.containerNode() == previous)
                positionOnlyToBeUpdated.moveToPosition(text, positionOnlyToBeUpdated.offsetInContainerNode());
        } else
            updatePositionForNodeRemoval(positionOnlyToBeUpdated, previous.get());

        removeNode(previous);
    }
    if (text->nextSibling() && text->nextSibling()->isTextNode()) {
        RefPtr<Text> next = toText(text->nextSibling());
        unsigned originalLength = text->length();
        insertTextIntoNode(text, originalLength, next->data());

        if (!positionIsOffsetInAnchor)
            updatePositionForNodeRemoval(position, next.get());

        if (positionOnlyToBeUpdatedIsOffsetInAnchor && positionOnlyToBeUpdated.containerNode() == next)
            positionOnlyToBeUpdated.moveToPosition(text, originalLength + positionOnlyToBeUpdated.offsetInContainerNode());
        else
            updatePositionForNodeRemoval(positionOnlyToBeUpdated, next.get());

        removeNode(next);
    }
}

EditAction ReplaceSelectionCommand::editingAction() const
{
    return m_editAction;
}

// If the user is inserting a list into an existing list, instead of nesting the list,
// we put the list items into the existing list.
Node* ReplaceSelectionCommand::insertAsListItems(PassRefPtr<HTMLElement> prpListElement, Node* insertionBlock, const Position& insertPos, InsertedNodes& insertedNodes)
{
    RefPtr<HTMLElement> listElement = prpListElement;

    while (listElement->hasChildNodes() && isListElement(listElement->firstChild()) && listElement->childNodeCount() == 1)
        listElement = toHTMLElement(listElement->firstChild());

    bool isStart = isStartOfParagraph(insertPos);
    bool isEnd = isEndOfParagraph(insertPos);
    bool isMiddle = !isStart && !isEnd;
    Node* lastNode = insertionBlock;

    // If we're in the middle of a list item, we should split it into two separate
    // list items and insert these nodes between them.
    if (isMiddle) {
        int textNodeOffset = insertPos.offsetInContainerNode();
        if (insertPos.deprecatedNode()->isTextNode() && textNodeOffset > 0)
            splitTextNode(toText(insertPos.deprecatedNode()), textNodeOffset);
        splitTreeToNode(insertPos.deprecatedNode(), lastNode, true);
    }

    while (RefPtr<Node> listItem = listElement->firstChild()) {
        listElement->removeChild(listItem.get(), ASSERT_NO_EXCEPTION);
        if (isStart || isMiddle) {
            insertNodeBefore(listItem, lastNode);
            insertedNodes.respondToNodeInsertion(listItem.get());
        } else if (isEnd) {
            insertNodeAfter(listItem, lastNode);
            insertedNodes.respondToNodeInsertion(listItem.get());
            lastNode = listItem.get();
        } else
            ASSERT_NOT_REACHED();
    }
    if (isStart || isMiddle)
        lastNode = lastNode->previousSibling();
    return lastNode;
}

void ReplaceSelectionCommand::updateNodesInserted(Node *node)
{
    if (!node)
        return;

    if (m_startOfInsertedContent.isNull())
        m_startOfInsertedContent = firstPositionInOrBeforeNode(node);

    m_endOfInsertedContent = lastPositionInOrAfterNode(node->lastDescendant());
}

// During simple pastes, where we're just pasting a text node into a run of text, we insert the text node
// directly into the text node that holds the selection.  This is much faster than the generalized code in
// ReplaceSelectionCommand, and works around <https://bugs.webkit.org/show_bug.cgi?id=6148> since we don't 
// split text nodes.
bool ReplaceSelectionCommand::performTrivialReplace(const ReplacementFragment& fragment)
{
    if (!fragment.firstChild() || fragment.firstChild() != fragment.lastChild() || !fragment.firstChild()->isTextNode())
        return false;

    // FIXME: Would be nice to handle smart replace in the fast path.
    if (m_smartReplace || fragment.hasInterchangeNewlineAtStart() || fragment.hasInterchangeNewlineAtEnd())
        return false;

    // e.g. when "bar" is inserted after "foo" in <div><u>foo</u></div>, "bar" should not be underlined.
    if (nodeToSplitToAvoidPastingIntoInlineNodesWithStyle(endingSelection().start()))
        return false;

    RefPtr<Node> nodeAfterInsertionPos = endingSelection().end().downstream().anchorNode();
    Text* textNode = toText(fragment.firstChild());
    // Our fragment creation code handles tabs, spaces, and newlines, so we don't have to worry about those here.

    Position start = endingSelection().start();
    Position end = replaceSelectedTextInNode(textNode->data());
    if (end.isNull())
        return false;

    if (nodeAfterInsertionPos && nodeAfterInsertionPos->parentNode() && nodeAfterInsertionPos->hasTagName(brTag)
        && shouldRemoveEndBR(nodeAfterInsertionPos.get(), positionBeforeNode(nodeAfterInsertionPos.get())))
        removeNodeAndPruneAncestors(nodeAfterInsertionPos.get());

    VisibleSelection selectionAfterReplace(m_selectReplacement ? start : end, end);

    setEndingSelection(selectionAfterReplace);

    return true;
}

} // namespace WebCore
