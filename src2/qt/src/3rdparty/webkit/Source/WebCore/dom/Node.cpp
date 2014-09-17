/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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
 */

#include "config.h"
#include "Node.h"

#include "AXObjectCache.h"
#include "Attr.h"
#include "Attribute.h"
#include "CSSParser.h"
#include "CSSRule.h"
#include "CSSRuleList.h"
#include "CSSSelector.h"
#include "CSSSelectorList.h"
#include "CSSStyleRule.h"
#include "CSSStyleSelector.h"
#include "CSSStyleSheet.h"
#include "ChildNodeList.h"
#include "ClassNodeList.h"
#include "ContextMenuController.h"
#include "DOMImplementation.h"
#include "Document.h"
#include "DocumentType.h"
#include "DynamicNodeList.h"
#include "Element.h"
#include "Event.h"
#include "EventContext.h"
#include "EventDispatcher.h"
#include "EventException.h"
#include "EventHandler.h"
#include "EventListener.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "FrameView.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "InspectorInstrumentation.h"
#include "KeyboardEvent.h"
#include "LabelsNodeList.h"
#include "Logging.h"
#include "MouseEvent.h"
#include "MutationEvent.h"
#include "NameNodeList.h"
#include "NamedNodeMap.h"
#include "NodeRareData.h"
#include "Page.h"
#include "PlatformMouseEvent.h"
#include "PlatformWheelEvent.h"
#include "ProcessingInstruction.h"
#include "ProgressEvent.h"
#include "RegisteredEventListener.h"
#include "RenderBlock.h"
#include "RenderBox.h"
#include "RenderFullScreen.h"
#include "RenderTextControl.h"
#include "RenderView.h"
#include "ScopedEventQueue.h"
#include "SelectorNodeList.h"
#include "ShadowRoot.h"
#include "StaticNodeList.h"
#include "TagNodeList.h"
#include "Text.h"
#include "TextEvent.h"
#include "UIEvent.h"
#include "UIEventWithKeyState.h"
#include "WebKitAnimationEvent.h"
#include "WebKitTransitionEvent.h"
#include "WheelEvent.h"
#include "WindowEventContext.h"
#include "XMLNames.h"
#include "htmlediting.h"
#include <wtf/HashSet.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/UnusedParam.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

#if ENABLE(DOM_STORAGE)
#include "StorageEvent.h"
#endif

#if ENABLE(SVG)
#include "SVGElementInstance.h"
#include "SVGUseElement.h"
#endif

#if ENABLE(XHTMLMP)
#include "HTMLNoScriptElement.h"
#endif

#if USE(JSC)
#include <runtime/JSGlobalData.h>
#endif

#define DUMP_NODE_STATISTICS 0

using namespace std;

namespace WebCore {

using namespace HTMLNames;

bool Node::isSupported(const String& feature, const String& version)
{
    return DOMImplementation::hasFeature(feature, version);
}

#if DUMP_NODE_STATISTICS
static HashSet<Node*> liveNodeSet;
#endif

void Node::dumpStatistics()
{
#if DUMP_NODE_STATISTICS
    size_t nodesWithRareData = 0;

    size_t elementNodes = 0;
    size_t attrNodes = 0;
    size_t textNodes = 0;
    size_t cdataNodes = 0;
    size_t commentNodes = 0;
    size_t entityReferenceNodes = 0;
    size_t entityNodes = 0;
    size_t piNodes = 0;
    size_t documentNodes = 0;
    size_t docTypeNodes = 0;
    size_t fragmentNodes = 0;
    size_t notationNodes = 0;
    size_t xpathNSNodes = 0;
    size_t shadowRootNodes = 0;

    HashMap<String, size_t> perTagCount;

    size_t attributes = 0;
    size_t mappedAttributes = 0;
    size_t mappedAttributesWithStyleDecl = 0;
    size_t attributesWithAttr = 0;
    size_t attrMaps = 0;

    for (HashSet<Node*>::iterator it = liveNodeSet.begin(); it != liveNodeSet.end(); ++it) {
        Node* node = *it;

        if (node->hasRareData())
            ++nodesWithRareData;

        switch (node->nodeType()) {
            case ELEMENT_NODE: {
                ++elementNodes;

                // Tag stats
                Element* element = static_cast<Element*>(node);
                pair<HashMap<String, size_t>::iterator, bool> result = perTagCount.add(element->tagName(), 1);
                if (!result.second)
                    result.first->second++;

                // AttributeMap stats
                if (NamedNodeMap* attrMap = element->attributes(true)) {
                    attributes += attrMap->length();
                    ++attrMaps;
                    for (unsigned i = 0; i < attrMap->length(); ++i) {
                        Attribute* attr = attrMap->attributeItem(i);
                        if (attr->attr())
                            ++attributesWithAttr;
                        if (attr->isMappedAttribute()) {
                            ++mappedAttributes;
                            if (attr->style())
                                ++mappedAttributesWithStyleDecl;
                        }
                    }
                }
                break;
            }
            case ATTRIBUTE_NODE: {
                ++attrNodes;
                break;
            }
            case TEXT_NODE: {
                ++textNodes;
                break;
            }
            case CDATA_SECTION_NODE: {
                ++cdataNodes;
                break;
            }
            case COMMENT_NODE: {
                ++commentNodes;
                break;
            }
            case ENTITY_REFERENCE_NODE: {
                ++entityReferenceNodes;
                break;
            }
            case ENTITY_NODE: {
                ++entityNodes;
                break;
            }
            case PROCESSING_INSTRUCTION_NODE: {
                ++piNodes;
                break;
            }
            case DOCUMENT_NODE: {
                ++documentNodes;
                break;
            }
            case DOCUMENT_TYPE_NODE: {
                ++docTypeNodes;
                break;
            }
            case DOCUMENT_FRAGMENT_NODE: {
                ++fragmentNodes;
                break;
            }
            case NOTATION_NODE: {
                ++notationNodes;
                break;
            }
            case XPATH_NAMESPACE_NODE: {
                ++xpathNSNodes;
                break;
            }
            case SHADOW_ROOT_NODE: {
                ++shadowRootNodes;
                break;
            }
        }
    }

    printf("Number of Nodes: %d\n\n", liveNodeSet.size());
    printf("Number of Nodes with RareData: %zu\n\n", nodesWithRareData);

    printf("NodeType distrubution:\n");
    printf("  Number of Element nodes: %zu\n", elementNodes);
    printf("  Number of Attribute nodes: %zu\n", attrNodes);
    printf("  Number of Text nodes: %zu\n", textNodes);
    printf("  Number of CDATASection nodes: %zu\n", cdataNodes);
    printf("  Number of Comment nodes: %zu\n", commentNodes);
    printf("  Number of EntityReference nodes: %zu\n", entityReferenceNodes);
    printf("  Number of Entity nodes: %zu\n", entityNodes);
    printf("  Number of ProcessingInstruction nodes: %zu\n", piNodes);
    printf("  Number of Document nodes: %zu\n", documentNodes);
    printf("  Number of DocumentType nodes: %zu\n", docTypeNodes);
    printf("  Number of DocumentFragment nodes: %zu\n", fragmentNodes);
    printf("  Number of Notation nodes: %zu\n", notationNodes);
    printf("  Number of XPathNS nodes: %zu\n", xpathNSNodes);
    printf("  Number of ShadowRoot nodes: %zu\n", shadowRootNodes);

    printf("Element tag name distibution:\n");
    for (HashMap<String, size_t>::iterator it = perTagCount.begin(); it != perTagCount.end(); ++it)
        printf("  Number of <%s> tags: %zu\n", it->first.utf8().data(), it->second);

    printf("Attribute Maps:\n");
    printf("  Number of Attributes (non-Node and Node): %zu [%zu]\n", attributes, sizeof(Attribute));
    printf("  Number of Attributes that are mapped: %zu\n", mappedAttributes);
    printf("  Number of Attributes with a StyleDeclaration: %zu\n", mappedAttributesWithStyleDecl);
    printf("  Number of Attributes with an Attr: %zu\n", attributesWithAttr);
    printf("  Number of NamedNodeMaps: %zu [%zu]\n", attrMaps, sizeof(NamedNodeMap));
#endif
}

#ifndef NDEBUG
static WTF::RefCountedLeakCounter nodeCounter("WebCoreNode");

static bool shouldIgnoreLeaks = false;
static HashSet<Node*> ignoreSet;
#endif

void Node::startIgnoringLeaks()
{
#ifndef NDEBUG
    shouldIgnoreLeaks = true;
#endif
}

void Node::stopIgnoringLeaks()
{
#ifndef NDEBUG
    shouldIgnoreLeaks = false;
#endif
}

Node::StyleChange Node::diff(const RenderStyle* s1, const RenderStyle* s2)
{
    // FIXME: The behavior of this function is just totally wrong.  It doesn't handle
    // explicit inheritance of non-inherited properties and so you end up not re-resolving
    // style in cases where you need to.
    StyleChange ch = NoInherit;
    EDisplay display1 = s1 ? s1->display() : NONE;
    bool fl1 = s1 && s1->hasPseudoStyle(FIRST_LETTER);
    EDisplay display2 = s2 ? s2->display() : NONE;
    bool fl2 = s2 && s2->hasPseudoStyle(FIRST_LETTER);
    
    // We just detach if a renderer acquires or loses a column-span, since spanning elements
    // typically won't contain much content.
    bool colSpan1 = s1 && s1->columnSpan();
    bool colSpan2 = s2 && s2->columnSpan();
    
    if (display1 != display2 || fl1 != fl2 || colSpan1 != colSpan2 || (s1 && s2 && !s1->contentDataEquivalent(s2)))
        ch = Detach;
    else if (!s1 || !s2)
        ch = Inherit;
    else if (*s1 == *s2)
        ch = NoChange;
    else if (s1->inheritedNotEqual(s2))
        ch = Inherit;
    
    // For nth-child and other positional rules, treat styles as different if they have
    // changed positionally in the DOM. This way subsequent sibling resolutions won't be confused
    // by the wrong child index and evaluate to incorrect results.
    if (ch == NoChange && s1->childIndex() != s2->childIndex())
        ch = NoInherit;

    // If the pseudoStyles have changed, we want any StyleChange that is not NoChange
    // because setStyle will do the right thing with anything else.
    if (ch == NoChange && s1->hasAnyPublicPseudoStyles()) {
        for (PseudoId pseudoId = FIRST_PUBLIC_PSEUDOID; ch == NoChange && pseudoId < FIRST_INTERNAL_PSEUDOID; pseudoId = static_cast<PseudoId>(pseudoId + 1)) {
            if (s1->hasPseudoStyle(pseudoId)) {
                RenderStyle* ps2 = s2->getCachedPseudoStyle(pseudoId);
                if (!ps2)
                    ch = NoInherit;
                else {
                    RenderStyle* ps1 = s1->getCachedPseudoStyle(pseudoId);
                    ch = ps1 && *ps1 == *ps2 ? NoChange : NoInherit;
                }
            }
        }
    }

    // When text-combine property has been changed, we need to prepare a separate renderer object.
    // When text-combine is on, we use RenderCombineText, otherwise RenderText.
    // https://bugs.webkit.org/show_bug.cgi?id=55069
    if ((s1 && s2) && (s1->hasTextCombine() != s2->hasTextCombine()))
        ch = Detach;

    return ch;
}

void Node::trackForDebugging()
{
#ifndef NDEBUG
    if (shouldIgnoreLeaks)
        ignoreSet.add(this);
    else
        nodeCounter.increment();
#endif

#if DUMP_NODE_STATISTICS
    liveNodeSet.add(this);
#endif
}

Node::~Node()
{
#ifndef NDEBUG
    HashSet<Node*>::iterator it = ignoreSet.find(this);
    if (it != ignoreSet.end())
        ignoreSet.remove(it);
    else
        nodeCounter.decrement();
#endif

#if DUMP_NODE_STATISTICS
    liveNodeSet.remove(this);
#endif

    if (!hasRareData())
        ASSERT(!NodeRareData::rareDataMap().contains(this));
    else {
        if (m_document && rareData()->nodeLists())
            m_document->removeNodeListCache();
        
        NodeRareData::NodeRareDataMap& dataMap = NodeRareData::rareDataMap();
        NodeRareData::NodeRareDataMap::iterator it = dataMap.find(this);
        ASSERT(it != dataMap.end());
        delete it->second;
        dataMap.remove(it);
    }

    if (renderer())
        detach();

    if (AXObjectCache::accessibilityEnabled() && m_document && m_document->axObjectCacheExists())
        m_document->axObjectCache()->removeNodeForUse(this);
    
    if (m_previous)
        m_previous->setNextSibling(0);
    if (m_next)
        m_next->setPreviousSibling(0);

    if (m_document)
        m_document->guardDeref();
}

#ifdef NDEBUG

static inline void setWillMoveToNewOwnerDocumentWasCalled(bool)
{
}

static inline void setDidMoveToNewOwnerDocumentWasCalled(bool)
{
}

#else
    
static bool willMoveToNewOwnerDocumentWasCalled;
static bool didMoveToNewOwnerDocumentWasCalled;

static void setWillMoveToNewOwnerDocumentWasCalled(bool wasCalled)
{
    willMoveToNewOwnerDocumentWasCalled = wasCalled;
}

static void setDidMoveToNewOwnerDocumentWasCalled(bool wasCalled)
{
    didMoveToNewOwnerDocumentWasCalled = wasCalled;
}
    
#endif
    
void Node::setDocument(Document* document)
{
    ASSERT(!inDocument() || m_document == document);
    if (inDocument() || m_document == document)
        return;

    document->guardRef();

    setWillMoveToNewOwnerDocumentWasCalled(false);
    willMoveToNewOwnerDocument();
    ASSERT(willMoveToNewOwnerDocumentWasCalled);

    if (hasRareData() && rareData()->nodeLists()) {
        if (m_document)
            m_document->removeNodeListCache();
        document->addNodeListCache();
    }

    if (m_document) {
        m_document->moveNodeIteratorsToNewDocument(this, document);
        m_document->guardDeref();
    }

    m_document = document;

    setDidMoveToNewOwnerDocumentWasCalled(false);
    didMoveToNewOwnerDocument();
    ASSERT(didMoveToNewOwnerDocumentWasCalled);
}

TreeScope* Node::treeScope() const
{
    // FIXME: Using m_document directly is not good -> see comment with document() in the header file.
    if (!hasRareData())
        return m_document;
    TreeScope* scope = rareData()->treeScope();
    return scope ? scope : m_document;
}

void Node::setTreeScopeRecursively(TreeScope* newTreeScope, bool includeRoot)
{
    ASSERT(this);
    ASSERT(!includeRoot || !isDocumentNode());
    ASSERT(newTreeScope);
    ASSERT(!m_deletionHasBegun);

    TreeScope* currentTreeScope = treeScope();
    if (currentTreeScope == newTreeScope)
        return;

    Document* currentDocument = document();
    Document* newDocument = newTreeScope->document();
    // If an element is moved from a document and then eventually back again the collection cache for
    // that element may contain stale data as changes made to it will have updated the DOMTreeVersion
    // of the document it was moved to. By increasing the DOMTreeVersion of the donating document here
    // we ensure that the collection cache will be invalidated as needed when the element is moved back.
    if (currentDocument && currentDocument != newDocument)
        currentDocument->incDOMTreeVersion();

    for (Node* node = includeRoot ? this : traverseNextNode(this); node; node = node->traverseNextNode(this)) {
        if (newTreeScope == newDocument) {
            if (node->hasRareData())
                node->rareData()->setTreeScope(0);
            // Setting the new document tree scope will be handled implicitly
            // by setDocument() below.
        } else
            node->ensureRareData()->setTreeScope(newTreeScope);

        node->setDocument(newDocument);

        if (!node->isElementNode())
            continue;
        if (ShadowRoot* shadowRoot = toElement(node)->shadowRoot()) {
            shadowRoot->setParentTreeScope(newTreeScope);
            if (currentDocument != newDocument)
                shadowRoot->setDocumentRecursively(newDocument);
        }
    }
}

NodeRareData* Node::rareData() const
{
    ASSERT(hasRareData());
    return NodeRareData::rareDataFromMap(this);
}

NodeRareData* Node::ensureRareData()
{
    if (hasRareData())
        return rareData();
    
    ASSERT(!NodeRareData::rareDataMap().contains(this));
    NodeRareData* data = createRareData();
    NodeRareData::rareDataMap().set(this, data);
    setFlag(HasRareDataFlag);
    return data;
}
    
NodeRareData* Node::createRareData()
{
    return new NodeRareData;
}

Element* Node::shadowHost() const
{
    return toElement(getFlag(IsShadowRootFlag) ? parent() : 0);
}

void Node::setShadowHost(Element* host)
{
    ASSERT(!parentNode() && !isSVGShadowRoot());
    if (host)
        setFlag(IsShadowRootFlag);
    else
        clearFlag(IsShadowRootFlag);

    setParent(host);
}

InputElement* Node::toInputElement()
{
    // If one of the below ASSERTs trigger, you are calling this function
    // directly or indirectly from a constructor or destructor of this object.
    // Don't do this!
    ASSERT(!(isHTMLElement() && hasTagName(inputTag)));
    return 0;
}

short Node::tabIndex() const
{
    return hasRareData() ? rareData()->tabIndex() : 0;
}
    
void Node::setTabIndexExplicitly(short i)
{
    ensureRareData()->setTabIndexExplicitly(i);
}

void Node::clearTabIndexExplicitly()
{
    ensureRareData()->clearTabIndexExplicitly();
}

String Node::nodeValue() const
{
    return String();
}

void Node::setNodeValue(const String& /*nodeValue*/, ExceptionCode& ec)
{
    // NO_MODIFICATION_ALLOWED_ERR: Raised when the node is readonly
    if (isReadOnlyNode()) {
        ec = NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    // By default, setting nodeValue has no effect.
}

PassRefPtr<NodeList> Node::childNodes()
{
    NodeRareData* data = ensureRareData();
    if (!data->nodeLists()) {
        data->setNodeLists(NodeListsNodeData::create());
        if (document())
            document()->addNodeListCache();
    }

    return ChildNodeList::create(this, data->nodeLists()->m_childNodeListCaches.get());
}

Node *Node::lastDescendant() const
{
    Node *n = const_cast<Node *>(this);
    while (n && n->lastChild())
        n = n->lastChild();
    return n;
}

Node* Node::firstDescendant() const
{
    Node *n = const_cast<Node *>(this);
    while (n && n->firstChild())
        n = n->firstChild();
    return n;
}

bool Node::insertBefore(PassRefPtr<Node> newChild, Node* refChild, ExceptionCode& ec, bool shouldLazyAttach)
{
    if (!isContainerNode()) {
        ec = HIERARCHY_REQUEST_ERR;
        return false;
    }
    return toContainerNode(this)->insertBefore(newChild, refChild, ec, shouldLazyAttach);
}

bool Node::replaceChild(PassRefPtr<Node> newChild, Node* oldChild, ExceptionCode& ec, bool shouldLazyAttach)
{
    if (!isContainerNode()) {
        ec = HIERARCHY_REQUEST_ERR;
        return false;
    }
    return toContainerNode(this)->replaceChild(newChild, oldChild, ec, shouldLazyAttach);
}

bool Node::removeChild(Node* oldChild, ExceptionCode& ec)
{
    if (!isContainerNode()) {
        ec = NOT_FOUND_ERR;
        return false;
    }
    return toContainerNode(this)->removeChild(oldChild, ec);
}

bool Node::appendChild(PassRefPtr<Node> newChild, ExceptionCode& ec, bool shouldLazyAttach)
{
    if (!isContainerNode()) {
        ec = HIERARCHY_REQUEST_ERR;
        return false;
    }
    return toContainerNode(this)->appendChild(newChild, ec, shouldLazyAttach);
}

void Node::remove(ExceptionCode& ec)
{
    if (ContainerNode* parent = parentNode())
        parent->removeChild(this, ec);
    else
        ec = HIERARCHY_REQUEST_ERR;
}

void Node::normalize()
{
    // Go through the subtree beneath us, normalizing all nodes. This means that
    // any two adjacent text nodes are merged and any empty text nodes are removed.

    RefPtr<Node> node = this;
    while (Node* firstChild = node->firstChild())
        node = firstChild;
    while (node) {
        NodeType type = node->nodeType();
        if (type == ELEMENT_NODE)
            static_cast<Element*>(node.get())->normalizeAttributes();

        if (node == this)
            break;

        if (type != TEXT_NODE) {
            node = node->traverseNextNodePostOrder();
            continue;
        }

        Text* text = static_cast<Text*>(node.get());

        // Remove empty text nodes.
        if (!text->length()) {
            // Care must be taken to get the next node before removing the current node.
            node = node->traverseNextNodePostOrder();
            ExceptionCode ec;
            text->remove(ec);
            continue;
        }

        // Merge text nodes.
        while (Node* nextSibling = node->nextSibling()) {
            if (nextSibling->nodeType() != TEXT_NODE)
                break;
            RefPtr<Text> nextText = static_cast<Text*>(nextSibling);

            // Remove empty text nodes.
            if (!nextText->length()) {
                ExceptionCode ec;
                nextText->remove(ec);
                continue;
            }

            // Both non-empty text nodes. Merge them.
            unsigned offset = text->length();
            ExceptionCode ec;
            text->appendData(nextText->data(), ec);
            document()->textNodesMerged(nextText.get(), offset);
            nextText->remove(ec);
        }

        node = node->traverseNextNodePostOrder();
    }
}

const AtomicString& Node::virtualPrefix() const
{
    // For nodes other than elements and attributes, the prefix is always null
    return nullAtom;
}

void Node::setPrefix(const AtomicString& /*prefix*/, ExceptionCode& ec)
{
    // The spec says that for nodes other than elements and attributes, prefix is always null.
    // It does not say what to do when the user tries to set the prefix on another type of
    // node, however Mozilla throws a NAMESPACE_ERR exception.
    ec = NAMESPACE_ERR;
}

const AtomicString& Node::virtualLocalName() const
{
    return nullAtom;
}

const AtomicString& Node::virtualNamespaceURI() const
{
    return nullAtom;
}

void Node::deprecatedParserAddChild(PassRefPtr<Node>)
{
}

bool Node::isContentEditable() const
{
    document()->updateLayoutIgnorePendingStylesheets();
    return rendererIsEditable(Editable);
}

bool Node::rendererIsEditable(EditableLevel editableLevel) const
{
    if (document()->frame() && document()->frame()->page() && document()->frame()->page()->isEditable())
        return true;

    // Ideally we'd call ASSERT(!needsStyleRecalc()) here, but
    // ContainerNode::setFocus() calls setNeedsStyleRecalc(), so the assertion
    // would fire in the middle of Document::setFocusedNode().

    for (const Node* node = this; node; node = node->parentNode()) {
        if ((node->isHTMLElement() || node->isDocumentNode()) && node->renderer()) {
            switch (node->renderer()->style()->userModify()) {
            case READ_ONLY:
                return false;
            case READ_WRITE:
                return true;
            case READ_WRITE_PLAINTEXT_ONLY:
                return editableLevel != RichlyEditable;
            }
            ASSERT_NOT_REACHED();
            return false;
        }
    }

    return false;
}

bool Node::shouldUseInputMethod() const
{
    return isContentEditable();
}

RenderBox* Node::renderBox() const
{
    return m_renderer && m_renderer->isBox() ? toRenderBox(m_renderer) : 0;
}

RenderBoxModelObject* Node::renderBoxModelObject() const
{
    return m_renderer && m_renderer->isBoxModelObject() ? toRenderBoxModelObject(m_renderer) : 0;
}

IntRect Node::getRect() const
{
    if (renderer())
        return renderer()->absoluteBoundingBoxRect(true);
    return IntRect();
}
    
IntRect Node::renderRect(bool* isReplaced)
{    
    RenderObject* hitRenderer = this->renderer();
    ASSERT(hitRenderer);
    RenderObject* renderer = hitRenderer;
    while (renderer && !renderer->isBody() && !renderer->isRoot()) {
        if (renderer->isRenderBlock() || renderer->isInlineBlockOrInlineTable() || renderer->isReplaced()) {
            *isReplaced = renderer->isReplaced();
            return renderer->absoluteBoundingBoxRect(true);
        }
        renderer = renderer->parent();
    }
    return IntRect();    
}

bool Node::hasNonEmptyBoundingBox() const
{
    // Before calling absoluteRects, check for the common case where the renderer
    // is non-empty, since this is a faster check and almost always returns true.
    RenderBoxModelObject* box = renderBoxModelObject();
    if (!box)
        return false;
    if (!box->borderBoundingBox().isEmpty())
        return true;

    Vector<IntRect> rects;
    FloatPoint absPos = renderer()->localToAbsolute();
    renderer()->absoluteRects(rects, absPos.x(), absPos.y());
    size_t n = rects.size();
    for (size_t i = 0; i < n; ++i)
        if (!rects[i].isEmpty())
            return true;

    return false;
}

inline static ShadowRoot* shadowRoot(Node* node)
{
    return node->isElementNode() ? toElement(node)->shadowRoot() : 0;
}

void Node::setDocumentRecursively(Document* newDocument)
{
    ASSERT(document() != newDocument);

    for (Node* node = this; node; node = node->traverseNextNode(this)) {
        node->setDocument(newDocument);
        if (!node->isElementNode())
            continue;
        if (ShadowRoot* shadow = shadowRoot(node))
            shadow->setDocumentRecursively(newDocument);
    }
}

inline void Node::setStyleChange(StyleChangeType changeType)
{
    m_nodeFlags = (m_nodeFlags & ~StyleChangeMask) | changeType;
}

inline void Node::markAncestorsWithChildNeedsStyleRecalc()
{
    for (ContainerNode* p = parentOrHostNode(); p && !p->childNeedsStyleRecalc(); p = p->parentOrHostNode())
        p->setChildNeedsStyleRecalc();

    if (document()->childNeedsStyleRecalc())
        document()->scheduleStyleRecalc();
}

void Node::refEventTarget()
{
    ref();
}

void Node::derefEventTarget()
{
    deref();
}

void Node::setNeedsStyleRecalc(StyleChangeType changeType)
{
    ASSERT(changeType != NoStyleChange);
    if (!attached()) // changed compared to what?
        return;

    StyleChangeType existingChangeType = styleChangeType();
    if (changeType > existingChangeType)
        setStyleChange(changeType);

    if (existingChangeType == NoStyleChange)
        markAncestorsWithChildNeedsStyleRecalc();
}

void Node::lazyAttach(ShouldSetAttached shouldSetAttached)
{
    for (Node* n = this; n; n = n->traverseNextNode(this)) {
        if (n->firstChild())
            n->setChildNeedsStyleRecalc();
        n->setStyleChange(FullStyleChange);
        if (shouldSetAttached == SetAttached)
            n->setAttached();
    }
    markAncestorsWithChildNeedsStyleRecalc();
}

void Node::setFocus(bool b)
{ 
    if (b || hasRareData())
        ensureRareData()->setFocused(b);
}

bool Node::rareDataFocused() const
{
    ASSERT(hasRareData());
    return rareData()->isFocused();
}

bool Node::supportsFocus() const
{
    return hasRareData() && rareData()->tabIndexSetExplicitly();
}
    
bool Node::isFocusable() const
{
    if (!inDocument() || !supportsFocus())
        return false;
    
    if (renderer())
        ASSERT(!renderer()->needsLayout());
    else
        // If the node is in a display:none tree it might say it needs style recalc but
        // the whole document is actually up to date.
        ASSERT(!document()->childNeedsStyleRecalc());
    
    // FIXME: Even if we are not visible, we might have a child that is visible.
    // Hyatt wants to fix that some day with a "has visible content" flag or the like.
    if (!renderer() || renderer()->style()->visibility() != VISIBLE)
        return false;

    return true;
}

bool Node::isKeyboardFocusable(KeyboardEvent*) const
{
    return isFocusable() && tabIndex() >= 0;
}

bool Node::isMouseFocusable() const
{
    return isFocusable();
}

unsigned Node::nodeIndex() const
{
    Node *_tempNode = previousSibling();
    unsigned count=0;
    for ( count=0; _tempNode; count++ )
        _tempNode = _tempNode->previousSibling();
    return count;
}

void Node::registerDynamicNodeList(DynamicNodeList* list)
{
    NodeRareData* data = ensureRareData();
    if (!data->nodeLists()) {
        data->setNodeLists(NodeListsNodeData::create());
        document()->addNodeListCache();
    } else if (!m_document || !m_document->hasNodeListCaches()) {
        // We haven't been receiving notifications while there were no registered lists, so the cache is invalid now.
        data->nodeLists()->invalidateCaches();
    }

    if (list->hasOwnCaches())
        data->nodeLists()->m_listsWithCaches.add(list);
}

void Node::unregisterDynamicNodeList(DynamicNodeList* list)
{
    ASSERT(rareData());
    ASSERT(rareData()->nodeLists());
    if (list->hasOwnCaches()) {
        NodeRareData* data = rareData();
        data->nodeLists()->m_listsWithCaches.remove(list);
        if (data->nodeLists()->isEmpty()) {
            data->clearNodeLists();
            if (document())
                document()->removeNodeListCache();
        }
    }
}

void Node::notifyLocalNodeListsAttributeChanged()
{
    if (!hasRareData())
        return;
    NodeRareData* data = rareData();
    if (!data->nodeLists())
        return;

    if (!isAttributeNode())
        data->nodeLists()->invalidateCachesThatDependOnAttributes();
    else
        data->nodeLists()->invalidateCaches();

    if (data->nodeLists()->isEmpty()) {
        data->clearNodeLists();
        document()->removeNodeListCache();
    }
}

void Node::notifyNodeListsAttributeChanged()
{
    for (Node *n = this; n; n = n->parentNode())
        n->notifyLocalNodeListsAttributeChanged();
}

void Node::notifyLocalNodeListsChildrenChanged()
{
    if (!hasRareData())
        return;
    NodeRareData* data = rareData();
    if (!data->nodeLists())
        return;

    data->nodeLists()->invalidateCaches();

    NodeListsNodeData::NodeListSet::iterator end = data->nodeLists()->m_listsWithCaches.end();
    for (NodeListsNodeData::NodeListSet::iterator i = data->nodeLists()->m_listsWithCaches.begin(); i != end; ++i)
        (*i)->invalidateCache();

    if (data->nodeLists()->isEmpty()) {
        data->clearNodeLists();
        document()->removeNodeListCache();
    }
}

void Node::notifyNodeListsChildrenChanged()
{
    for (Node* n = this; n; n = n->parentNode())
        n->notifyLocalNodeListsChildrenChanged();
}

void Node::notifyLocalNodeListsLabelChanged()
{
    if (!hasRareData())
        return;
    NodeRareData* data = rareData();
    if (!data->nodeLists())
        return;

    if (data->nodeLists()->m_labelsNodeListCache)
        data->nodeLists()->m_labelsNodeListCache->invalidateCache();
}

void Node::removeCachedClassNodeList(ClassNodeList* list, const String& className)
{
    ASSERT(rareData());
    ASSERT(rareData()->nodeLists());
    ASSERT_UNUSED(list, list->hasOwnCaches());

    NodeListsNodeData* data = rareData()->nodeLists();
    ASSERT_UNUSED(list, list == data->m_classNodeListCache.get(className));
    data->m_classNodeListCache.remove(className);
}

void Node::removeCachedNameNodeList(NameNodeList* list, const String& nodeName)
{
    ASSERT(rareData());
    ASSERT(rareData()->nodeLists());
    ASSERT_UNUSED(list, list->hasOwnCaches());

    NodeListsNodeData* data = rareData()->nodeLists();
    ASSERT_UNUSED(list, list == data->m_nameNodeListCache.get(nodeName));
    data->m_nameNodeListCache.remove(nodeName);
}

void Node::removeCachedTagNodeList(TagNodeList* list, const AtomicString& name)
{
    ASSERT(rareData());
    ASSERT(rareData()->nodeLists());
    ASSERT_UNUSED(list, list->hasOwnCaches());

    NodeListsNodeData* data = rareData()->nodeLists();
    ASSERT_UNUSED(list, list == data->m_tagNodeListCache.get(name.impl()));
    data->m_tagNodeListCache.remove(name.impl());
}

void Node::removeCachedTagNodeList(TagNodeList* list, const QualifiedName& name)
{
    ASSERT(rareData());
    ASSERT(rareData()->nodeLists());
    ASSERT_UNUSED(list, list->hasOwnCaches());

    NodeListsNodeData* data = rareData()->nodeLists();
    ASSERT_UNUSED(list, list == data->m_tagNodeListCacheNS.get(name.impl()));
    data->m_tagNodeListCacheNS.remove(name.impl());
}

void Node::removeCachedLabelsNodeList(DynamicNodeList* list)
{
    ASSERT(rareData());
    ASSERT(rareData()->nodeLists());
    ASSERT_UNUSED(list, list->hasOwnCaches());
    
    NodeListsNodeData* data = rareData()->nodeLists();
    data->m_labelsNodeListCache = 0;
}

Node* Node::traverseNextNode(const Node* stayWithin) const
{
    if (firstChild())
        return firstChild();
    if (this == stayWithin)
        return 0;
    if (nextSibling())
        return nextSibling();
    const Node *n = this;
    while (n && !n->nextSibling() && (!stayWithin || n->parentNode() != stayWithin))
        n = n->parentNode();
    if (n)
        return n->nextSibling();
    return 0;
}

Node* Node::traverseNextSibling(const Node* stayWithin) const
{
    if (this == stayWithin)
        return 0;
    if (nextSibling())
        return nextSibling();
    const Node *n = this;
    while (n && !n->nextSibling() && (!stayWithin || n->parentNode() != stayWithin))
        n = n->parentNode();
    if (n)
        return n->nextSibling();
    return 0;
}

Node* Node::traverseNextNodePostOrder() const
{
    Node* next = nextSibling();
    if (!next)
        return parentNode();
    while (Node* firstChild = next->firstChild())
        next = firstChild;
    return next;
}

Node* Node::traversePreviousNode(const Node* stayWithin) const
{
    if (this == stayWithin)
        return 0;
    if (previousSibling()) {
        Node *n = previousSibling();
        while (n->lastChild())
            n = n->lastChild();
        return n;
    }
    return parentNode();
}

Node* Node::traversePreviousNodePostOrder(const Node* stayWithin) const
{
    if (lastChild())
        return lastChild();
    if (this == stayWithin)
        return 0;
    if (previousSibling())
        return previousSibling();
    const Node *n = this;
    while (n && !n->previousSibling() && (!stayWithin || n->parentNode() != stayWithin))
        n = n->parentNode();
    if (n)
        return n->previousSibling();
    return 0;
}

Node* Node::traversePreviousSiblingPostOrder(const Node* stayWithin) const
{
    if (this == stayWithin)
        return 0;
    if (previousSibling())
        return previousSibling();
    const Node *n = this;
    while (n && !n->previousSibling() && (!stayWithin || n->parentNode() != stayWithin))
        n = n->parentNode();
    if (n)
        return n->previousSibling();
    return 0;
}

void Node::checkSetPrefix(const AtomicString& prefix, ExceptionCode& ec)
{
    // Perform error checking as required by spec for setting Node.prefix. Used by
    // Element::setPrefix() and Attr::setPrefix()

    // FIXME: Implement support for INVALID_CHARACTER_ERR: Raised if the specified prefix contains an illegal character.
    
    if (isReadOnlyNode()) {
        ec = NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    // FIXME: Raise NAMESPACE_ERR if prefix is malformed per the Namespaces in XML specification.

    const AtomicString& nodeNamespaceURI = namespaceURI();
    if ((nodeNamespaceURI.isEmpty() && !prefix.isEmpty())
        || (prefix == xmlAtom && nodeNamespaceURI != XMLNames::xmlNamespaceURI)) {
        ec = NAMESPACE_ERR;
        return;
    }
    // Attribute-specific checks are in Attr::setPrefix().
}

static bool isChildTypeAllowed(Node* newParent, Node* child)
{
    if (child->nodeType() != Node::DOCUMENT_FRAGMENT_NODE) {
        if (!newParent->childTypeAllowed(child->nodeType()))
            return false;
        return true;
    }
    
    for (Node *n = child->firstChild(); n; n = n->nextSibling()) {
        if (!newParent->childTypeAllowed(n->nodeType()))
            return false;
    }
    return true;
}

bool Node::canReplaceChild(Node* newChild, Node*)
{
    return isChildTypeAllowed(this, newChild);
}

static void checkAcceptChild(Node* newParent, Node* newChild, ExceptionCode& ec)
{
    // Not mentioned in spec: throw NOT_FOUND_ERR if newChild is null
    if (!newChild) {
        ec = NOT_FOUND_ERR;
        return;
    }
    
    if (newParent->isReadOnlyNode()) {
        ec = NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    if (newChild->inDocument() && newChild->nodeType() == Node::DOCUMENT_TYPE_NODE) {
        ec = HIERARCHY_REQUEST_ERR;
        return;
    }

    // HIERARCHY_REQUEST_ERR: Raised if this node is of a type that does not allow children of the type of the
    // newChild node, or if the node to append is one of this node's ancestors.

    if (newChild == newParent || newParent->isDescendantOf(newChild)) {
        ec = HIERARCHY_REQUEST_ERR;
        return;
    }
}

void Node::checkReplaceChild(Node* newChild, Node* oldChild, ExceptionCode& ec)
{
    if (!oldChild) {
        ec = NOT_FOUND_ERR;
        return;
    }

    checkAcceptChild(this, newChild, ec);
    if (ec)
        return;

    if (!canReplaceChild(newChild, oldChild)) {
        ec = HIERARCHY_REQUEST_ERR;
        return;
    }
}

void Node::checkAddChild(Node *newChild, ExceptionCode& ec)
{
    checkAcceptChild(this, newChild, ec);
    if (ec)
        return;
    
    if (!isChildTypeAllowed(this, newChild)) {
        ec = HIERARCHY_REQUEST_ERR;
        return;
    }
}

bool Node::isDescendantOf(const Node *other) const
{
    // Return true if other is an ancestor of this, otherwise false
    if (!other)
        return false;
    for (const ContainerNode* n = parentNode(); n; n = n->parentNode()) {
        if (n == other)
            return true;
    }
    return false;
}

bool Node::contains(const Node* node) const
{
    if (!node)
        return false;
    return this == node || node->isDescendantOf(this);
}

bool Node::containsIncludingShadowDOM(Node* node)
{
    if (!node)
        return false;
    for (Node* n = node; n; n = n->parentOrHostNode()) {
        if (n == this)
            return true;
    }
    return false;
}

void Node::attach()
{
    ASSERT(!attached());
    ASSERT(!renderer() || (renderer()->style() && renderer()->parent()));

    // FIXME: This is O(N^2) for the innerHTML case, where all children are replaced at once (and not attached).
    // If this node got a renderer it may be the previousRenderer() of sibling text nodes and thus affect the
    // result of Text::rendererIsNeeded() for those nodes.
    if (renderer()) {
        for (Node* next = nextSibling(); next; next = next->nextSibling()) {
            if (next->renderer())
                break;
            if (!next->attached())
                break;  // Assume this means none of the following siblings are attached.
            if (next->isTextNode())
                next->createRendererIfNeeded();
        }
    }

    setAttached();
    clearNeedsStyleRecalc();
}

void Node::willRemove()
{
}

void Node::detach()
{
    setFlag(InDetachFlag);

    if (renderer())
        renderer()->destroy();
    setRenderer(0);

    Document* doc = document();
    if (hovered())
        doc->hoveredNodeDetached(this);
    if (inActiveChain())
        doc->activeChainNodeDetached(this);

    clearFlag(IsActiveFlag);
    clearFlag(IsHoveredFlag);
    clearFlag(InActiveChainFlag);
    clearFlag(IsAttachedFlag);

    clearFlag(InDetachFlag);
}

RenderObject* Node::previousRenderer()
{
    // FIXME: We should have the same O(N^2) avoidance as nextRenderer does
    // however, when I tried adding it, several tests failed.
    for (Node* n = previousSibling(); n; n = n->previousSibling()) {
        if (n->renderer())
            return n->renderer();
    }
    return 0;
}

RenderObject* Node::nextRenderer()
{
    // Avoid an O(n^2) problem with this function by not checking for
    // nextRenderer() when the parent element hasn't attached yet.
    if (parentOrHostNode() && !parentOrHostNode()->attached())
        return 0;

    for (Node* n = nextSibling(); n; n = n->nextSibling()) {
        if (n->renderer())
            return n->renderer();
    }
    return 0;
}

// FIXME: This code is used by editing.  Seems like it could move over there and not pollute Node.
Node *Node::previousNodeConsideringAtomicNodes() const
{
    if (previousSibling()) {
        Node *n = previousSibling();
        while (!isAtomicNode(n) && n->lastChild())
            n = n->lastChild();
        return n;
    }
    else if (parentNode()) {
        return parentNode();
    }
    else {
        return 0;
    }
}

Node *Node::nextNodeConsideringAtomicNodes() const
{
    if (!isAtomicNode(this) && firstChild())
        return firstChild();
    if (nextSibling())
        return nextSibling();
    const Node *n = this;
    while (n && !n->nextSibling())
        n = n->parentNode();
    if (n)
        return n->nextSibling();
    return 0;
}

Node *Node::previousLeafNode() const
{
    Node *node = previousNodeConsideringAtomicNodes();
    while (node) {
        if (isAtomicNode(node))
            return node;
        node = node->previousNodeConsideringAtomicNodes();
    }
    return 0;
}

Node *Node::nextLeafNode() const
{
    Node *node = nextNodeConsideringAtomicNodes();
    while (node) {
        if (isAtomicNode(node))
            return node;
        node = node->nextNodeConsideringAtomicNodes();
    }
    return 0;
}


class NodeRendererFactory {
public:
    enum Type {
        NotFound,
        AsLightChild,
        AsShadowChild,
        AsContentChild
    };

    NodeRendererFactory(Node* node)
        : m_type(NotFound)
        , m_node(node)
        , m_visualParentShadowRoot(0)
    {
        m_parentNodeForRenderingAndStyle = findVisualParent();
    }
  
    ContainerNode* parentNodeForRenderingAndStyle() const { return m_parentNodeForRenderingAndStyle; }
    void createRendererIfNeeded();

private:
    Document* document() { return m_node->document(); }
    ContainerNode* findVisualParent();
    RenderObject* nextRenderer() const { return m_node->nextRenderer(); }
    RenderObject* createRendererAndStyle();
    bool shouldCreateRenderer() const;

    Type m_type;
    Node* m_node;
    ContainerNode* m_parentNodeForRenderingAndStyle;
    ShadowRoot* m_visualParentShadowRoot;
};

ContainerNode* NodeRendererFactory::findVisualParent()
{
    ContainerNode* parent = m_node->parentOrHostNode();
    if (!parent)
        return 0;

    if (parent->isShadowBoundary()) {
        m_type = AsShadowChild;
        return parent->shadowHost();
    }

    if (parent->isElementNode()) {
        m_visualParentShadowRoot = toElement(parent)->shadowRoot();
        if (m_visualParentShadowRoot) {
            if (ContainerNode* contentContainer = m_visualParentShadowRoot->contentContainerFor(m_node)) {
                m_type = AsContentChild;
                return NodeRendererFactory(contentContainer).parentNodeForRenderingAndStyle();
            }

            // FIXME: should be not found once light/shadow is mutual exclusive.
        }
    }

    m_type = AsLightChild;
    return parent;
}

bool NodeRendererFactory::shouldCreateRenderer() const
{
    ASSERT(m_parentNodeForRenderingAndStyle);

    RenderObject* parentRenderer = m_parentNodeForRenderingAndStyle->renderer();
    if (!parentRenderer)
        return false;

    if (m_type == AsLightChild) {
        // FIXME: Ignoring canHaveChildren() in a case of shadow children might be wrong.
        // See https://bugs.webkit.org/show_bug.cgi?id=52423
        if (!parentRenderer->canHaveChildren())
            return false;
    
        if (m_visualParentShadowRoot && !m_parentNodeForRenderingAndStyle->canHaveLightChildRendererWithShadow())
            return false;
    }

    if (!m_parentNodeForRenderingAndStyle->childShouldCreateRenderer(m_node))
        return false;

    return true;
}

RenderObject* NodeRendererFactory::createRendererAndStyle()
{
    ASSERT(!m_node->renderer());
    ASSERT(document()->shouldCreateRenderers());

    if (!shouldCreateRenderer())
        return 0;

    RefPtr<RenderStyle> style = m_node->styleForRenderer();
    if (!m_node->rendererIsNeeded(style.get()))
        return 0;

    RenderObject* newRenderer = m_node->createRenderer(document()->renderArena(), style.get());
    if (!newRenderer)
        return 0;

    if (!m_parentNodeForRenderingAndStyle->renderer()->isChildAllowed(newRenderer, style.get())) {
        newRenderer->destroy();
        return 0;
    }

    m_node->setRenderer(newRenderer);
    newRenderer->setAnimatableStyle(style.release()); // setAnimatableStyle() can depend on renderer() already being set.
    return newRenderer;
}

#if ENABLE(FULLSCREEN_API)
static RenderFullScreen* wrapWithRenderFullScreen(RenderObject* object, Document* document)
{
    RenderFullScreen* fullscreenRenderer = new (document->renderArena()) RenderFullScreen(document);
    fullscreenRenderer->setStyle(RenderFullScreen::createFullScreenStyle());
    // It's possible that we failed to create the new render and end up wrapping nothing.
    // We'll end up displaying a black screen, but Jer says this is expected.
    if (object)
        fullscreenRenderer->addChild(object);
    document->setFullScreenRenderer(fullscreenRenderer);
    return fullscreenRenderer;
}
#endif

void NodeRendererFactory::createRendererIfNeeded()
{
    if (!document()->shouldCreateRenderers())
        return;

    ASSERT(!m_node->renderer());

    RenderObject* newRenderer = createRendererAndStyle();

#if ENABLE(FULLSCREEN_API)
    if (document()->webkitIsFullScreen() && document()->webkitCurrentFullScreenElement() == m_node)
        newRenderer = wrapWithRenderFullScreen(newRenderer, document());
#endif

    if (!newRenderer)
        return;

    // Note: Adding newRenderer instead of renderer(). renderer() may be a child of newRenderer.
    m_parentNodeForRenderingAndStyle->renderer()->addChild(newRenderer, nextRenderer());
}

ContainerNode* Node::parentNodeForRenderingAndStyle()
{
    return NodeRendererFactory(this).parentNodeForRenderingAndStyle();
}

void Node::createRendererIfNeeded()
{
    NodeRendererFactory(this).createRendererIfNeeded();
}

PassRefPtr<RenderStyle> Node::styleForRenderer()
{
    if (isElementNode()) {
        bool allowSharing = true;
#if ENABLE(XHTMLMP)
        // noscript needs the display property protected - it's a special case
        allowSharing = localName() != HTMLNames::noscriptTag.localName();
#endif
        return document()->styleSelector()->styleForElement(static_cast<Element*>(this), 0, allowSharing);
    }
    return parentNode() && parentNode()->renderer() ? parentNode()->renderer()->style() : 0;
}

bool Node::rendererIsNeeded(RenderStyle *style)
{
    return (document()->documentElement() == this) || (style->display() != NONE);
}

RenderObject* Node::createRenderer(RenderArena*, RenderStyle*)
{
    ASSERT(false);
    return 0;
}
    
RenderStyle* Node::nonRendererRenderStyle() const
{ 
    return 0; 
}   

void Node::setRenderStyle(PassRefPtr<RenderStyle> s)
{
    if (m_renderer)
        m_renderer->setAnimatableStyle(s); 
}

RenderStyle* Node::virtualComputedStyle(PseudoId pseudoElementSpecifier)
{
    return parentOrHostNode() ? parentOrHostNode()->computedStyle(pseudoElementSpecifier) : 0;
}

int Node::maxCharacterOffset() const
{
    ASSERT_NOT_REACHED();
    return 0;
}

// FIXME: Shouldn't these functions be in the editing code?  Code that asks questions about HTML in the core DOM class
// is obviously misplaced.
bool Node::canStartSelection() const
{
    if (rendererIsEditable())
        return true;

    if (renderer()) {
        RenderStyle* style = renderer()->style();
        // We allow selections to begin within an element that has -webkit-user-select: none set,
        // but if the element is draggable then dragging should take priority over selection.
        if (style->userDrag() == DRAG_ELEMENT && style->userSelect() == SELECT_NONE)
            return false;
    }
    return parentOrHostNode() ? parentOrHostNode()->canStartSelection() : true;
}

#if ENABLE(SVG)
SVGUseElement* Node::svgShadowHost() const
{
    return isSVGShadowRoot() ? static_cast<SVGUseElement*>(parent()) : 0;
}
#endif

Node* Node::shadowAncestorNode()
{
#if ENABLE(SVG)
    // SVG elements living in a shadow tree only occur when <use> created them.
    // For these cases we do NOT want to return the shadowParentNode() here
    // but the actual shadow tree element - as main difference to the HTML forms
    // shadow tree concept. (This function _could_ be made virtual - opinions?)
    if (isSVGElement())
        return this;
#endif

    Node* root = shadowTreeRootNode();
    if (root)
        return root->shadowHost();
    return this;
}

Node* Node::shadowTreeRootNode()
{
    Node* root = this;
    while (root) {
        if (root->isShadowRoot() || root->isSVGShadowRoot())
            return root;
        root = root->parentNodeGuaranteedHostFree();
    }
    return 0;
}

bool Node::isInShadowTree()
{
    for (Node* n = this; n; n = n->parentNode())
        if (n->isShadowRoot())
            return true;
    return false;
}

Element* Node::parentOrHostElement() const
{
    ContainerNode* parent = parentOrHostNode();
    if (!parent)
        return 0;

    if (parent->isShadowRoot())
        parent = parent->shadowHost();

    if (!parent->isElementNode())
        return 0;

    return toElement(parent);
}


bool Node::isBlockFlow() const
{
    return renderer() && renderer()->isBlockFlow();
}

bool Node::isBlockFlowOrBlockTable() const
{
    return renderer() && (renderer()->isBlockFlow() || (renderer()->isTable() && !renderer()->isInline()));
}

Element *Node::enclosingBlockFlowElement() const
{
    Node *n = const_cast<Node *>(this);
    if (isBlockFlow())
        return static_cast<Element *>(n);

    while (1) {
        n = n->parentNode();
        if (!n)
            break;
        if (n->isBlockFlow() || n->hasTagName(bodyTag))
            return static_cast<Element *>(n);
    }
    return 0;
}

Element* Node::rootEditableElement() const
{
    Element* result = 0;
    for (Node* n = const_cast<Node*>(this); n && n->rendererIsEditable(); n = n->parentNode()) {
        if (n->isElementNode())
            result = static_cast<Element*>(n);
        if (n->hasTagName(bodyTag))
            break;
    }
    return result;
}

bool Node::inSameContainingBlockFlowElement(Node *n)
{
    return n ? enclosingBlockFlowElement() == n->enclosingBlockFlowElement() : false;
}

// FIXME: End of obviously misplaced HTML editing functions.  Try to move these out of Node.

PassRefPtr<NodeList> Node::getElementsByTagName(const AtomicString& localName)
{
    if (localName.isNull())
        return 0;

    NodeRareData* data = ensureRareData();
    if (!data->nodeLists()) {
        data->setNodeLists(NodeListsNodeData::create());
        document()->addNodeListCache();
    }

    String name = localName;
    if (document()->isHTMLDocument())
        name = localName.lower();

    AtomicString localNameAtom = name;

    pair<NodeListsNodeData::TagNodeListCache::iterator, bool> result = data->nodeLists()->m_tagNodeListCache.add(localNameAtom, 0);
    if (!result.second)
        return PassRefPtr<TagNodeList>(result.first->second);

    RefPtr<TagNodeList> list = TagNodeList::create(this, starAtom, localNameAtom);
    result.first->second = list.get();
    return list.release();   
}

PassRefPtr<NodeList> Node::getElementsByTagNameNS(const AtomicString& namespaceURI, const AtomicString& localName)
{
    if (localName.isNull())
        return 0;

    if (namespaceURI == starAtom)
        return getElementsByTagName(localName);

    NodeRareData* data = ensureRareData();
    if (!data->nodeLists()) {
        data->setNodeLists(NodeListsNodeData::create());
        document()->addNodeListCache();
    }

    String name = localName;
    if (document()->isHTMLDocument())
        name = localName.lower();

    AtomicString localNameAtom = name;

    pair<NodeListsNodeData::TagNodeListCacheNS::iterator, bool> result = data->nodeLists()->m_tagNodeListCacheNS.add(QualifiedName(nullAtom, localNameAtom, namespaceURI).impl(), 0);
    if (!result.second)
        return PassRefPtr<TagNodeList>(result.first->second);

    RefPtr<TagNodeList> list = TagNodeList::create(this, namespaceURI.isEmpty() ? nullAtom : namespaceURI, localNameAtom);
    result.first->second = list.get();
    return list.release();
}

PassRefPtr<NodeList> Node::getElementsByName(const String& elementName)
{
    NodeRareData* data = ensureRareData();
    if (!data->nodeLists()) {
        data->setNodeLists(NodeListsNodeData::create());
        document()->addNodeListCache();
    }

    pair<NodeListsNodeData::NameNodeListCache::iterator, bool> result = data->nodeLists()->m_nameNodeListCache.add(elementName, 0);
    if (!result.second)
        return PassRefPtr<NodeList>(result.first->second);

    RefPtr<NameNodeList> list = NameNodeList::create(this, elementName);
    result.first->second = list.get();
    return list.release();
}

PassRefPtr<NodeList> Node::getElementsByClassName(const String& classNames)
{
    NodeRareData* data = ensureRareData();
    if (!data->nodeLists()) {
        data->setNodeLists(NodeListsNodeData::create());
        document()->addNodeListCache();
    }

    pair<NodeListsNodeData::ClassNodeListCache::iterator, bool> result = data->nodeLists()->m_classNodeListCache.add(classNames, 0);
    if (!result.second)
        return PassRefPtr<NodeList>(result.first->second);

    RefPtr<ClassNodeList> list = ClassNodeList::create(this, classNames);
    result.first->second = list.get();
    return list.release();
}

PassRefPtr<Element> Node::querySelector(const String& selectors, ExceptionCode& ec)
{
    if (selectors.isEmpty()) {
        ec = SYNTAX_ERR;
        return 0;
    }
    bool strictParsing = !document()->inQuirksMode();
    CSSParser p(strictParsing);

    CSSSelectorList querySelectorList;
    p.parseSelector(selectors, document(), querySelectorList);

    if (!querySelectorList.first() || querySelectorList.hasUnknownPseudoElements()) {
        ec = SYNTAX_ERR;
        return 0;
    }

    // throw a NAMESPACE_ERR if the selector includes any namespace prefixes.
    if (querySelectorList.selectorsNeedNamespaceResolution()) {
        ec = NAMESPACE_ERR;
        return 0;
    }

    CSSStyleSelector::SelectorChecker selectorChecker(document(), strictParsing);

    // FIXME: we could also optimize for the the [id="foo"] case
    if (strictParsing && inDocument() && querySelectorList.hasOneSelector() && querySelectorList.first()->m_match == CSSSelector::Id) {
        Element* element = treeScope()->getElementById(querySelectorList.first()->value());
        if (element && (isDocumentNode() || element->isDescendantOf(this)) && selectorChecker.checkSelector(querySelectorList.first(), element))
            return element;
        return 0;
    }

    // FIXME: We can speed this up by implementing caching similar to the one use by getElementById
    for (Node* n = firstChild(); n; n = n->traverseNextNode(this)) {
        if (n->isElementNode()) {
            Element* element = static_cast<Element*>(n);
            for (CSSSelector* selector = querySelectorList.first(); selector; selector = CSSSelectorList::next(selector)) {
                if (selectorChecker.checkSelector(selector, element))
                    return element;
            }
        }
    }
    
    return 0;
}

PassRefPtr<NodeList> Node::querySelectorAll(const String& selectors, ExceptionCode& ec)
{
    if (selectors.isEmpty()) {
        ec = SYNTAX_ERR;
        return 0;
    }
    bool strictParsing = !document()->inQuirksMode();
    CSSParser p(strictParsing);

    CSSSelectorList querySelectorList;
    p.parseSelector(selectors, document(), querySelectorList);

    if (!querySelectorList.first() || querySelectorList.hasUnknownPseudoElements()) {
        ec = SYNTAX_ERR;
        return 0;
    }

    // Throw a NAMESPACE_ERR if the selector includes any namespace prefixes.
    if (querySelectorList.selectorsNeedNamespaceResolution()) {
        ec = NAMESPACE_ERR;
        return 0;
    }

    return createSelectorNodeList(this, querySelectorList);
}

Document *Node::ownerDocument() const
{
    Document *doc = document();
    return doc == this ? 0 : doc;
}

KURL Node::baseURI() const
{
    return parentNode() ? parentNode()->baseURI() : KURL();
}

bool Node::isEqualNode(Node* other) const
{
    if (!other)
        return false;
    
    NodeType nodeType = this->nodeType();
    if (nodeType != other->nodeType())
        return false;
    
    if (nodeName() != other->nodeName())
        return false;
    
    if (localName() != other->localName())
        return false;
    
    if (namespaceURI() != other->namespaceURI())
        return false;
    
    if (prefix() != other->prefix())
        return false;
    
    if (nodeValue() != other->nodeValue())
        return false;
    
    NamedNodeMap* attributes = this->attributes();
    NamedNodeMap* otherAttributes = other->attributes();
    
    if (!attributes && otherAttributes)
        return false;
    
    if (attributes && !attributes->mapsEquivalent(otherAttributes))
        return false;
    
    Node* child = firstChild();
    Node* otherChild = other->firstChild();
    
    while (child) {
        if (!child->isEqualNode(otherChild))
            return false;
        
        child = child->nextSibling();
        otherChild = otherChild->nextSibling();
    }
    
    if (otherChild)
        return false;
    
    if (nodeType == DOCUMENT_TYPE_NODE) {
        const DocumentType* documentTypeThis = static_cast<const DocumentType*>(this);
        const DocumentType* documentTypeOther = static_cast<const DocumentType*>(other);
        
        if (documentTypeThis->publicId() != documentTypeOther->publicId())
            return false;

        if (documentTypeThis->systemId() != documentTypeOther->systemId())
            return false;

        if (documentTypeThis->internalSubset() != documentTypeOther->internalSubset())
            return false;

        NamedNodeMap* entities = documentTypeThis->entities();
        NamedNodeMap* otherEntities = documentTypeOther->entities();
        if (!entities && otherEntities)
            return false;
        if (entities && !entities->mapsEquivalent(otherEntities))
            return false;

        NamedNodeMap* notations = documentTypeThis->notations();
        NamedNodeMap* otherNotations = documentTypeOther->notations();
        if (!notations && otherNotations)
            return false;
        if (notations && !notations->mapsEquivalent(otherNotations))
            return false;
    }
    
    return true;
}

bool Node::isDefaultNamespace(const AtomicString& namespaceURIMaybeEmpty) const
{
    const AtomicString& namespaceURI = namespaceURIMaybeEmpty.isEmpty() ? nullAtom : namespaceURIMaybeEmpty;

    switch (nodeType()) {
        case ELEMENT_NODE: {
            const Element* elem = static_cast<const Element*>(this);
            
            if (elem->prefix().isNull())
                return elem->namespaceURI() == namespaceURI;

            if (elem->hasAttributes()) {
                NamedNodeMap* attrs = elem->attributes();
                
                for (unsigned i = 0; i < attrs->length(); i++) {
                    Attribute* attr = attrs->attributeItem(i);
                    
                    if (attr->localName() == xmlnsAtom)
                        return attr->value() == namespaceURI;
                }
            }

            if (Element* ancestor = ancestorElement())
                return ancestor->isDefaultNamespace(namespaceURI);

            return false;
        }
        case DOCUMENT_NODE:
            if (Element* de = static_cast<const Document*>(this)->documentElement())
                return de->isDefaultNamespace(namespaceURI);
            return false;
        case ENTITY_NODE:
        case NOTATION_NODE:
        case DOCUMENT_TYPE_NODE:
        case DOCUMENT_FRAGMENT_NODE:
        case SHADOW_ROOT_NODE:
            return false;
        case ATTRIBUTE_NODE: {
            const Attr* attr = static_cast<const Attr*>(this);
            if (attr->ownerElement())
                return attr->ownerElement()->isDefaultNamespace(namespaceURI);
            return false;
        }
        default:
            if (Element* ancestor = ancestorElement())
                return ancestor->isDefaultNamespace(namespaceURI);
            return false;
    }
}

String Node::lookupPrefix(const AtomicString &namespaceURI) const
{
    // Implemented according to
    // http://www.w3.org/TR/2004/REC-DOM-Level-3-Core-20040407/namespaces-algorithms.html#lookupNamespacePrefixAlgo
    
    if (namespaceURI.isEmpty())
        return String();
    
    switch (nodeType()) {
        case ELEMENT_NODE:
            return lookupNamespacePrefix(namespaceURI, static_cast<const Element *>(this));
        case DOCUMENT_NODE:
            if (Element* de = static_cast<const Document*>(this)->documentElement())
                return de->lookupPrefix(namespaceURI);
            return String();
        case ENTITY_NODE:
        case NOTATION_NODE:
        case DOCUMENT_FRAGMENT_NODE:
        case DOCUMENT_TYPE_NODE:
        case SHADOW_ROOT_NODE:
            return String();
        case ATTRIBUTE_NODE: {
            const Attr *attr = static_cast<const Attr *>(this);
            if (attr->ownerElement())
                return attr->ownerElement()->lookupPrefix(namespaceURI);
            return String();
        }
        default:
            if (Element* ancestor = ancestorElement())
                return ancestor->lookupPrefix(namespaceURI);
            return String();
    }
}

String Node::lookupNamespaceURI(const String &prefix) const
{
    // Implemented according to
    // http://www.w3.org/TR/2004/REC-DOM-Level-3-Core-20040407/namespaces-algorithms.html#lookupNamespaceURIAlgo
    
    if (!prefix.isNull() && prefix.isEmpty())
        return String();
    
    switch (nodeType()) {
        case ELEMENT_NODE: {
            const Element *elem = static_cast<const Element *>(this);
            
            if (!elem->namespaceURI().isNull() && elem->prefix() == prefix)
                return elem->namespaceURI();
            
            if (elem->hasAttributes()) {
                NamedNodeMap *attrs = elem->attributes();
                
                for (unsigned i = 0; i < attrs->length(); i++) {
                    Attribute *attr = attrs->attributeItem(i);
                    
                    if (attr->prefix() == xmlnsAtom && attr->localName() == prefix) {
                        if (!attr->value().isEmpty())
                            return attr->value();
                        
                        return String();
                    } else if (attr->localName() == xmlnsAtom && prefix.isNull()) {
                        if (!attr->value().isEmpty())
                            return attr->value();
                        
                        return String();
                    }
                }
            }
            if (Element* ancestor = ancestorElement())
                return ancestor->lookupNamespaceURI(prefix);
            return String();
        }
        case DOCUMENT_NODE:
            if (Element* de = static_cast<const Document*>(this)->documentElement())
                return de->lookupNamespaceURI(prefix);
            return String();
        case ENTITY_NODE:
        case NOTATION_NODE:
        case DOCUMENT_TYPE_NODE:
        case DOCUMENT_FRAGMENT_NODE:
        case SHADOW_ROOT_NODE:
            return String();
        case ATTRIBUTE_NODE: {
            const Attr *attr = static_cast<const Attr *>(this);
            
            if (attr->ownerElement())
                return attr->ownerElement()->lookupNamespaceURI(prefix);
            else
                return String();
        }
        default:
            if (Element* ancestor = ancestorElement())
                return ancestor->lookupNamespaceURI(prefix);
            return String();
    }
}

String Node::lookupNamespacePrefix(const AtomicString &_namespaceURI, const Element *originalElement) const
{
    if (_namespaceURI.isNull())
        return String();
            
    if (originalElement->lookupNamespaceURI(prefix()) == _namespaceURI)
        return prefix();
    
    if (hasAttributes()) {
        NamedNodeMap *attrs = attributes();
        
        for (unsigned i = 0; i < attrs->length(); i++) {
            Attribute *attr = attrs->attributeItem(i);
            
            if (attr->prefix() == xmlnsAtom &&
                attr->value() == _namespaceURI &&
                originalElement->lookupNamespaceURI(attr->localName()) == _namespaceURI)
                return attr->localName();
        }
    }
    
    if (Element* ancestor = ancestorElement())
        return ancestor->lookupNamespacePrefix(_namespaceURI, originalElement);
    return String();
}

static void appendTextContent(const Node* node, bool convertBRsToNewlines, bool& isNullString, StringBuilder& content)
{
    switch (node->nodeType()) {
    case Node::TEXT_NODE:
    case Node::CDATA_SECTION_NODE:
    case Node::COMMENT_NODE:
        isNullString = false;
        content.append(static_cast<const CharacterData*>(node)->data());
        break;

    case Node::PROCESSING_INSTRUCTION_NODE:
        isNullString = false;
        content.append(static_cast<const ProcessingInstruction*>(node)->data());
        break;
    
    case Node::ELEMENT_NODE:
        if (node->hasTagName(brTag) && convertBRsToNewlines) {
            isNullString = false;
            content.append('\n');
            break;
        }
    // Fall through.
    case Node::ATTRIBUTE_NODE:
    case Node::ENTITY_NODE:
    case Node::ENTITY_REFERENCE_NODE:
    case Node::DOCUMENT_FRAGMENT_NODE:
    case Node::SHADOW_ROOT_NODE:
        isNullString = false;
        for (Node* child = node->firstChild(); child; child = child->nextSibling()) {
            if (child->nodeType() == Node::COMMENT_NODE || child->nodeType() == Node::PROCESSING_INSTRUCTION_NODE)
                continue;
            appendTextContent(child, convertBRsToNewlines, isNullString, content);
        }
        break;

    case Node::DOCUMENT_NODE:
    case Node::DOCUMENT_TYPE_NODE:
    case Node::NOTATION_NODE:
    case Node::XPATH_NAMESPACE_NODE:
        break;
    }
}

String Node::textContent(bool convertBRsToNewlines) const
{
    StringBuilder content;
    bool isNullString = true;
    appendTextContent(this, convertBRsToNewlines, isNullString, content);
    return isNullString ? String() : content.toString();
}

void Node::setTextContent(const String& text, ExceptionCode& ec)
{           
    switch (nodeType()) {
        case TEXT_NODE:
        case CDATA_SECTION_NODE:
        case COMMENT_NODE:
        case PROCESSING_INSTRUCTION_NODE:
            setNodeValue(text, ec);
            return;
        case ELEMENT_NODE:
        case ATTRIBUTE_NODE:
        case ENTITY_NODE:
        case ENTITY_REFERENCE_NODE:
        case DOCUMENT_FRAGMENT_NODE:
        case SHADOW_ROOT_NODE: {
            ContainerNode* container = toContainerNode(this);
            container->removeChildren();
            if (!text.isEmpty())
                container->appendChild(document()->createTextNode(text), ec);
            return;
        }
        case DOCUMENT_NODE:
        case DOCUMENT_TYPE_NODE:
        case NOTATION_NODE:
        case XPATH_NAMESPACE_NODE:
            // Do nothing.
            return;
    }
    ASSERT_NOT_REACHED();
}

Element* Node::ancestorElement() const
{
    // In theory, there can be EntityReference nodes between elements, but this is currently not supported.
    for (ContainerNode* n = parentNode(); n; n = n->parentNode()) {
        if (n->isElementNode())
            return static_cast<Element*>(n);
    }
    return 0;
}

bool Node::offsetInCharacters() const
{
    return false;
}

unsigned short Node::compareDocumentPosition(Node* otherNode)
{
    // It is not clear what should be done if |otherNode| is 0.
    if (!otherNode)
        return DOCUMENT_POSITION_DISCONNECTED;

    if (otherNode == this)
        return DOCUMENT_POSITION_EQUIVALENT;
    
    Attr* attr1 = nodeType() == ATTRIBUTE_NODE ? static_cast<Attr*>(this) : 0;
    Attr* attr2 = otherNode->nodeType() == ATTRIBUTE_NODE ? static_cast<Attr*>(otherNode) : 0;
    
    Node* start1 = attr1 ? attr1->ownerElement() : this;
    Node* start2 = attr2 ? attr2->ownerElement() : otherNode;
    
    // If either of start1 or start2 is null, then we are disconnected, since one of the nodes is
    // an orphaned attribute node.
    if (!start1 || !start2)
        return DOCUMENT_POSITION_DISCONNECTED | DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC;

    Vector<Node*, 16> chain1;
    Vector<Node*, 16> chain2;
    if (attr1)
        chain1.append(attr1);
    if (attr2)
        chain2.append(attr2);
    
    if (attr1 && attr2 && start1 == start2 && start1) {
        // We are comparing two attributes on the same node.  Crawl our attribute map
        // and see which one we hit first.
        NamedNodeMap* map = attr1->ownerElement()->attributes(true);
        unsigned length = map->length();
        for (unsigned i = 0; i < length; ++i) {
            // If neither of the two determining nodes is a child node and nodeType is the same for both determining nodes, then an 
            // implementation-dependent order between the determining nodes is returned. This order is stable as long as no nodes of
            // the same nodeType are inserted into or removed from the direct container. This would be the case, for example, 
            // when comparing two attributes of the same element, and inserting or removing additional attributes might change 
            // the order between existing attributes.
            Attribute* attr = map->attributeItem(i);
            if (attr1->attr() == attr)
                return DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC | DOCUMENT_POSITION_FOLLOWING;
            if (attr2->attr() == attr)
                return DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC | DOCUMENT_POSITION_PRECEDING;
        }
        
        ASSERT_NOT_REACHED();
        return DOCUMENT_POSITION_DISCONNECTED;
    }

    // If one node is in the document and the other is not, we must be disconnected.
    // If the nodes have different owning documents, they must be disconnected.  Note that we avoid
    // comparing Attr nodes here, since they return false from inDocument() all the time (which seems like a bug).
    if (start1->inDocument() != start2->inDocument() ||
        start1->document() != start2->document())
        return DOCUMENT_POSITION_DISCONNECTED | DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC;

    // We need to find a common ancestor container, and then compare the indices of the two immediate children.
    Node* current;
    for (current = start1; current; current = current->parentNode())
        chain1.append(current);
    for (current = start2; current; current = current->parentNode())
        chain2.append(current);
   
    // Walk the two chains backwards and look for the first difference.
    unsigned index1 = chain1.size();
    unsigned index2 = chain2.size();
    for (unsigned i = min(index1, index2); i; --i) {
        Node* child1 = chain1[--index1];
        Node* child2 = chain2[--index2];
        if (child1 != child2) {
            // If one of the children is an attribute, it wins.
            if (child1->nodeType() == ATTRIBUTE_NODE)
                return DOCUMENT_POSITION_FOLLOWING;
            if (child2->nodeType() == ATTRIBUTE_NODE)
                return DOCUMENT_POSITION_PRECEDING;
            
            if (!child2->nextSibling())
                return DOCUMENT_POSITION_FOLLOWING;
            if (!child1->nextSibling())
                return DOCUMENT_POSITION_PRECEDING;

            // Otherwise we need to see which node occurs first.  Crawl backwards from child2 looking for child1.
            for (Node* child = child2->previousSibling(); child; child = child->previousSibling()) {
                if (child == child1)
                    return DOCUMENT_POSITION_FOLLOWING;
            }
            return DOCUMENT_POSITION_PRECEDING;
        }
    }
    
    // There was no difference between the two parent chains, i.e., one was a subset of the other.  The shorter
    // chain is the ancestor.
    return index1 < index2 ? 
               DOCUMENT_POSITION_FOLLOWING | DOCUMENT_POSITION_CONTAINED_BY :
               DOCUMENT_POSITION_PRECEDING | DOCUMENT_POSITION_CONTAINS;
}

FloatPoint Node::convertToPage(const FloatPoint& p) const
{
    // If there is a renderer, just ask it to do the conversion
    if (renderer())
        return renderer()->localToAbsolute(p, false, true);
    
    // Otherwise go up the tree looking for a renderer
    Element *parent = ancestorElement();
    if (parent)
        return parent->convertToPage(p);

    // No parent - no conversion needed
    return p;
}

FloatPoint Node::convertFromPage(const FloatPoint& p) const
{
    // If there is a renderer, just ask it to do the conversion
    if (renderer())
        return renderer()->absoluteToLocal(p, false, true);

    // Otherwise go up the tree looking for a renderer
    Element *parent = ancestorElement();
    if (parent)
        return parent->convertFromPage(p);

    // No parent - no conversion needed
    return p;
}

#ifndef NDEBUG

static void appendAttributeDesc(const Node* node, String& string, const QualifiedName& name, const char* attrDesc)
{
    if (node->isElementNode()) {
        String attr = static_cast<const Element*>(node)->getAttribute(name);
        if (!attr.isEmpty()) {
            string += attrDesc;
            string += attr;
        }
    }
}

void Node::showNode(const char* prefix) const
{
    if (!prefix)
        prefix = "";
    if (isTextNode()) {
        String value = nodeValue();
        value.replace('\\', "\\\\");
        value.replace('\n', "\\n");
        fprintf(stderr, "%s%s\t%p \"%s\"\n", prefix, nodeName().utf8().data(), this, value.utf8().data());
    } else {
        String attrs = "";
        appendAttributeDesc(this, attrs, classAttr, " CLASS=");
        appendAttributeDesc(this, attrs, styleAttr, " STYLE=");
        fprintf(stderr, "%s%s\t%p%s\n", prefix, nodeName().utf8().data(), this, attrs.utf8().data());
    }
}

void Node::showTreeForThis() const
{
    showTreeAndMark(this, "*");
}

static void traverseTreeAndMark(const String& baseIndent, const Node* rootNode, const Node* markedNode1, const char* markedLabel1, const Node* markedNode2, const char* markedLabel2)
{
    for (const Node* node = rootNode; node; node = node->traverseNextNode()) {
        if (node == markedNode1)
            fprintf(stderr, "%s", markedLabel1);
        if (node == markedNode2)
            fprintf(stderr, "%s", markedLabel2);

        String indent = baseIndent;
        for (const Node* tmpNode = node; tmpNode && tmpNode != rootNode; tmpNode = tmpNode->parentOrHostNode())
            indent += "\t";
        fprintf(stderr, "%s", indent.utf8().data());
        node->showNode();

        ContainerNode* shadow = shadowRoot(const_cast<Node*>(node));

        if (!shadow && node->renderer() && node->renderer()->isTextControl())
            shadow = static_cast<RenderTextControl*>(node->renderer())->innerTextElement();

        if (shadow) {
            indent += "\t";
            traverseTreeAndMark(indent, shadow, markedNode1, markedLabel1, markedNode2, markedLabel2);
        }
    }
}

void Node::showTreeAndMark(const Node* markedNode1, const char* markedLabel1, const Node* markedNode2, const char* markedLabel2) const
{
    const Node* rootNode;
    const Node* node = this;
    while (node->parentOrHostNode() && !node->hasTagName(bodyTag))
        node = node->parentOrHostNode();
    rootNode = node;

    String startingIndent;
    traverseTreeAndMark(startingIndent, rootNode, markedNode1, markedLabel1, markedNode2, markedLabel2);
}

void Node::formatForDebugger(char* buffer, unsigned length) const
{
    String result;
    String s;
    
    s = nodeName();
    if (s.length() == 0)
        result += "<none>";
    else
        result += s;
          
    strncpy(buffer, result.utf8().data(), length - 1);
}

#endif

// --------

void NodeListsNodeData::invalidateCaches()
{
    m_childNodeListCaches->reset();

    if (m_labelsNodeListCache)
        m_labelsNodeListCache->invalidateCache();
    TagNodeListCache::const_iterator tagCacheEnd = m_tagNodeListCache.end();
    for (TagNodeListCache::const_iterator it = m_tagNodeListCache.begin(); it != tagCacheEnd; ++it)
        it->second->invalidateCache();
    TagNodeListCacheNS::const_iterator tagCacheNSEnd = m_tagNodeListCacheNS.end();
    for (TagNodeListCacheNS::const_iterator it = m_tagNodeListCacheNS.begin(); it != tagCacheNSEnd; ++it)
        it->second->invalidateCache();
    invalidateCachesThatDependOnAttributes();
}

void NodeListsNodeData::invalidateCachesThatDependOnAttributes()
{
    ClassNodeListCache::iterator classCacheEnd = m_classNodeListCache.end();
    for (ClassNodeListCache::iterator it = m_classNodeListCache.begin(); it != classCacheEnd; ++it)
        it->second->invalidateCache();

    NameNodeListCache::iterator nameCacheEnd = m_nameNodeListCache.end();
    for (NameNodeListCache::iterator it = m_nameNodeListCache.begin(); it != nameCacheEnd; ++it)
        it->second->invalidateCache();
    if (m_labelsNodeListCache)
        m_labelsNodeListCache->invalidateCache();
}

bool NodeListsNodeData::isEmpty() const
{
    if (!m_listsWithCaches.isEmpty())
        return false;

    if (m_childNodeListCaches->refCount())
        return false;
    
    TagNodeListCache::const_iterator tagCacheEnd = m_tagNodeListCache.end();
    for (TagNodeListCache::const_iterator it = m_tagNodeListCache.begin(); it != tagCacheEnd; ++it) {
        if (it->second->refCount())
            return false;
    }

    TagNodeListCacheNS::const_iterator tagCacheNSEnd = m_tagNodeListCacheNS.end();
    for (TagNodeListCacheNS::const_iterator it = m_tagNodeListCacheNS.begin(); it != tagCacheNSEnd; ++it) {
        if (it->second->refCount())
            return false;
    }

    ClassNodeListCache::const_iterator classCacheEnd = m_classNodeListCache.end();
    for (ClassNodeListCache::const_iterator it = m_classNodeListCache.begin(); it != classCacheEnd; ++it) {
        if (it->second->refCount())
            return false;
    }

    NameNodeListCache::const_iterator nameCacheEnd = m_nameNodeListCache.end();
    for (NameNodeListCache::const_iterator it = m_nameNodeListCache.begin(); it != nameCacheEnd; ++it) {
        if (it->second->refCount())
            return false;
    }

    if (m_labelsNodeListCache)
        return false;

    return true;
}

void Node::getSubresourceURLs(ListHashSet<KURL>& urls) const
{
    addSubresourceAttributeURLs(urls);
}

Node* Node::enclosingLinkEventParentOrSelf()
{
    for (Node* node = this; node; node = node->parentOrHostNode()) {
        // For imagemaps, the enclosing link node is the associated area element not the image itself.
        // So we don't let images be the enclosingLinkNode, even though isLink sometimes returns true
        // for them.
        if (node->isLink() && !node->hasTagName(imgTag))
            return node;
    }

    return 0;
}

// --------

ScriptExecutionContext* Node::scriptExecutionContext() const
{
    return document();
}

void Node::insertedIntoDocument()
{
    setInDocument();
}

void Node::removedFromDocument()
{
    clearInDocument();
}

void Node::willMoveToNewOwnerDocument()
{
    ASSERT(!willMoveToNewOwnerDocumentWasCalled);
    setWillMoveToNewOwnerDocumentWasCalled(true);
}

void Node::didMoveToNewOwnerDocument()
{
    ASSERT(!didMoveToNewOwnerDocumentWasCalled);
    setDidMoveToNewOwnerDocumentWasCalled(true);
}

#if ENABLE(SVG)
static inline HashSet<SVGElementInstance*> instancesForSVGElement(Node* node)
{
    HashSet<SVGElementInstance*> instances;
 
    ASSERT(node);
    if (!node->isSVGElement() || node->shadowTreeRootNode())
        return HashSet<SVGElementInstance*>();

    SVGElement* element = static_cast<SVGElement*>(node);
    if (!element->isStyled())
        return HashSet<SVGElementInstance*>();

    SVGStyledElement* styledElement = static_cast<SVGStyledElement*>(element);
    ASSERT(!styledElement->instanceUpdatesBlocked());

    return styledElement->instancesForElement();
}
#endif

static inline bool tryAddEventListener(Node* targetNode, const AtomicString& eventType, PassRefPtr<EventListener> listener, bool useCapture)
{
    if (!targetNode->EventTarget::addEventListener(eventType, listener, useCapture))
        return false;

    if (Document* document = targetNode->document())
        document->addListenerTypeIfNeeded(eventType);

    return true;
}

bool Node::addEventListener(const AtomicString& eventType, PassRefPtr<EventListener> listener, bool useCapture)
{
#if !ENABLE(SVG)
    return tryAddEventListener(this, eventType, listener, useCapture);
#else
    if (!isSVGElement())
        return tryAddEventListener(this, eventType, listener, useCapture);

    HashSet<SVGElementInstance*> instances = instancesForSVGElement(this);
    if (instances.isEmpty())
        return tryAddEventListener(this, eventType, listener, useCapture);

    RefPtr<EventListener> listenerForRegularTree = listener;
    RefPtr<EventListener> listenerForShadowTree = listenerForRegularTree;

    // Add event listener to regular DOM element
    if (!tryAddEventListener(this, eventType, listenerForRegularTree.release(), useCapture))
        return false;

    // Add event listener to all shadow tree DOM element instances
    const HashSet<SVGElementInstance*>::const_iterator end = instances.end();
    for (HashSet<SVGElementInstance*>::const_iterator it = instances.begin(); it != end; ++it) {
        ASSERT((*it)->shadowTreeElement());
        ASSERT((*it)->correspondingElement() == this);

        RefPtr<EventListener> listenerForCurrentShadowTreeElement = listenerForShadowTree;
        bool result = tryAddEventListener((*it)->shadowTreeElement(), eventType, listenerForCurrentShadowTreeElement.release(), useCapture);
        ASSERT_UNUSED(result, result);
    }

    return true;
#endif
}

static inline bool tryRemoveEventListener(Node* targetNode, const AtomicString& eventType, EventListener* listener, bool useCapture)
{
    if (!targetNode->EventTarget::removeEventListener(eventType, listener, useCapture))
        return false;

    // FIXME: Notify Document that the listener has vanished. We need to keep track of a number of
    // listeners for each type, not just a bool - see https://bugs.webkit.org/show_bug.cgi?id=33861

    return true;
}

bool Node::removeEventListener(const AtomicString& eventType, EventListener* listener, bool useCapture)
{
#if !ENABLE(SVG)
    return tryRemoveEventListener(this, eventType, listener, useCapture);
#else
    if (!isSVGElement())
        return tryRemoveEventListener(this, eventType, listener, useCapture);

    HashSet<SVGElementInstance*> instances = instancesForSVGElement(this);
    if (instances.isEmpty())
        return tryRemoveEventListener(this, eventType, listener, useCapture);

    // EventTarget::removeEventListener creates a PassRefPtr around the given EventListener
    // object when creating a temporary RegisteredEventListener object used to look up the
    // event listener in a cache. If we want to be able to call removeEventListener() multiple
    // times on different nodes, we have to delay its immediate destruction, which would happen
    // after the first call below.
    RefPtr<EventListener> protector(listener);

    // Remove event listener from regular DOM element
    if (!tryRemoveEventListener(this, eventType, listener, useCapture))
        return false;

    // Remove event listener from all shadow tree DOM element instances
    const HashSet<SVGElementInstance*>::const_iterator end = instances.end();
    for (HashSet<SVGElementInstance*>::const_iterator it = instances.begin(); it != end; ++it) {
        ASSERT((*it)->correspondingElement() == this);

        SVGElement* shadowTreeElement = (*it)->shadowTreeElement();
        ASSERT(shadowTreeElement);

        if (tryRemoveEventListener(shadowTreeElement, eventType, listener, useCapture))
            continue;

        // This case can only be hit for event listeners created from markup
        ASSERT(listener->wasCreatedFromMarkup());

        // If the event listener 'listener' has been created from markup and has been fired before
        // then JSLazyEventListener::parseCode() has been called and m_jsFunction of that listener
        // has been created (read: it's not 0 anymore). During shadow tree creation, the event
        // listener DOM attribute has been cloned, and another event listener has been setup in
        // the shadow tree. If that event listener has not been used yet, m_jsFunction is still 0,
        // and tryRemoveEventListener() above will fail. Work around that very seldom problem.
        EventTargetData* data = shadowTreeElement->eventTargetData();
        ASSERT(data);

        EventListenerMap::iterator result = data->eventListenerMap.find(eventType);
        ASSERT(result != data->eventListenerMap.end());

        EventListenerVector* entry = result->second;
        ASSERT(entry);

        unsigned int index = 0;
        bool foundListener = false;

        EventListenerVector::iterator end = entry->end();
        for (EventListenerVector::iterator it = entry->begin(); it != end; ++it) {
            if (!(*it).listener->wasCreatedFromMarkup()) {
                ++index;
                continue;
            }

            foundListener = true;
            entry->remove(index);
            break;
        }

        ASSERT_UNUSED(foundListener, foundListener);

        if (entry->isEmpty()) {                
            delete entry;
            data->eventListenerMap.remove(result);
        }
    }

    return true;
#endif
}

EventTargetData* Node::eventTargetData()
{
    return hasRareData() ? rareData()->eventTargetData() : 0;
}

EventTargetData* Node::ensureEventTargetData()
{
    return ensureRareData()->ensureEventTargetData();
}

void Node::handleLocalEvents(Event* event)
{
    if (!hasRareData() || !rareData()->eventTargetData())
        return;

    if (disabled() && event->isMouseEvent())
        return;

    fireEventListeners(event);
}

void Node::dispatchScopedEvent(PassRefPtr<Event> event)
{
    EventDispatcher::dispatchScopedEvent(this, event);
}

bool Node::dispatchEvent(PassRefPtr<Event> event)
{
    return EventDispatcher::dispatchEvent(this, EventDispatchMediator(event));
}

void Node::dispatchSubtreeModifiedEvent()
{
    ASSERT(!eventDispatchForbidden());
    
    document()->incDOMTreeVersion();

    notifyNodeListsAttributeChanged(); // FIXME: Can do better some day. Really only care about the name attribute changing.
    
    if (!document()->hasListenerType(Document::DOMSUBTREEMODIFIED_LISTENER))
        return;

    dispatchScopedEvent(MutationEvent::create(eventNames().DOMSubtreeModifiedEvent, true));
}

void Node::dispatchUIEvent(const AtomicString& eventType, int detail, PassRefPtr<Event> underlyingEvent)
{
    ASSERT(!eventDispatchForbidden());
    ASSERT(eventType == eventNames().focusinEvent || eventType == eventNames().focusoutEvent || 
           eventType == eventNames().DOMFocusInEvent || eventType == eventNames().DOMFocusOutEvent || eventType == eventNames().DOMActivateEvent);
    
    bool cancelable = eventType == eventNames().DOMActivateEvent;

    RefPtr<UIEvent> event = UIEvent::create(eventType, true, cancelable, document()->defaultView(), detail);
    event->setUnderlyingEvent(underlyingEvent);
    dispatchScopedEvent(event.release());
}

bool Node::dispatchKeyEvent(const PlatformKeyboardEvent& event)
{
    return EventDispatcher::dispatchEvent(this, KeyboardEventDispatchMediator(KeyboardEvent::create(event, document()->defaultView())));
}

bool Node::dispatchMouseEvent(const PlatformMouseEvent& event, const AtomicString& eventType,
    int detail, Node* relatedTarget)
{
    return EventDispatcher::dispatchEvent(this, MouseEventDispatchMediator(MouseEvent::create(eventType, document()->defaultView(), event, detail, relatedTarget)));
}

void Node::dispatchSimulatedClick(PassRefPtr<Event> event, bool sendMouseEvents, bool showPressedLook)
{
    EventDispatcher::dispatchSimulatedClick(this, event, sendMouseEvents, showPressedLook);
}

bool Node::dispatchWheelEvent(const PlatformWheelEvent& event)
{
    return EventDispatcher::dispatchEvent(this, WheelEventDispatchMediator(event, document()->defaultView()));
}

void Node::dispatchFocusEvent()
{
    dispatchEvent(Event::create(eventNames().focusEvent, false, false));
}

void Node::dispatchBlurEvent()
{
    dispatchEvent(Event::create(eventNames().blurEvent, false, false));
}

void Node::dispatchChangeEvent()
{
    dispatchEvent(Event::create(eventNames().changeEvent, true, false));
}

void Node::dispatchInputEvent()
{
    dispatchEvent(Event::create(eventNames().inputEvent, true, false));
}

bool Node::disabled() const
{
    return false;
}

void Node::defaultEventHandler(Event* event)
{
    if (event->target() != this)
        return;
    const AtomicString& eventType = event->type();
    if (eventType == eventNames().keydownEvent || eventType == eventNames().keypressEvent) {
        if (event->isKeyboardEvent())
            if (Frame* frame = document()->frame())
                frame->eventHandler()->defaultKeyboardEventHandler(static_cast<KeyboardEvent*>(event));
    } else if (eventType == eventNames().clickEvent) {
        int detail = event->isUIEvent() ? static_cast<UIEvent*>(event)->detail() : 0;
        dispatchUIEvent(eventNames().DOMActivateEvent, detail, event);
#if ENABLE(CONTEXT_MENUS)
    } else if (eventType == eventNames().contextmenuEvent) {
        if (Frame* frame = document()->frame())
            if (Page* page = frame->page())
                page->contextMenuController()->handleContextMenuEvent(event);
#endif
    } else if (eventType == eventNames().textInputEvent) {
        if (event->isTextEvent())
            if (Frame* frame = document()->frame())
                frame->eventHandler()->defaultTextInputEventHandler(static_cast<TextEvent*>(event));
#if ENABLE(PAN_SCROLLING)
    } else if (eventType == eventNames().mousedownEvent && event->isMouseEvent()) {
        MouseEvent* mouseEvent = static_cast<MouseEvent*>(event);
        if (mouseEvent->button() == MiddleButton) {
            if (enclosingLinkEventParentOrSelf())
                return;

            RenderObject* renderer = this->renderer();
            while (renderer && (!renderer->isBox() || !toRenderBox(renderer)->canBeScrolledAndHasScrollableArea()))
                renderer = renderer->parent();

            if (renderer) {
                if (Frame* frame = document()->frame())
                    frame->eventHandler()->startPanScrolling(renderer);
            }
        }
#endif
    } else if (eventType == eventNames().mousewheelEvent && event->isWheelEvent()) {
        WheelEvent* wheelEvent = static_cast<WheelEvent*>(event);
        
        // If we don't have a renderer, send the wheel event to the first node we find with a renderer.
        // This is needed for <option> and <optgroup> elements so that <select>s get a wheel scroll.
        Node* startNode = this;
        while (startNode && !startNode->renderer())
            startNode = startNode->parentOrHostNode();
        
        if (startNode && startNode->renderer())
            if (Frame* frame = document()->frame())
                frame->eventHandler()->defaultWheelEventHandler(startNode, wheelEvent);
    } else if (event->type() == eventNames().webkitEditableContentChangedEvent) {
        dispatchInputEvent();
    }
}

} // namespace WebCore

#ifndef NDEBUG

void showTree(const WebCore::Node* node)
{
    if (node)
        node->showTreeForThis();
}

#endif
