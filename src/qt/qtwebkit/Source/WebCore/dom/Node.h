/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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
 *
 */

#ifndef Node_h
#define Node_h

#include "EditingBoundary.h"
#include "EventTarget.h"
#include "KURLHash.h"
#include "LayoutRect.h"
#include "MutationObserver.h"
#include "RenderStyleConstants.h"
#include "ScriptWrappable.h"
#include "SimulatedClickOptions.h"
#include "TreeScope.h"
#include "TreeShared.h"
#include <wtf/Forward.h>
#include <wtf/ListHashSet.h>
#include <wtf/text/AtomicString.h>

namespace JSC {
    class VM;
    class SlotVisitor;
}

// This needs to be here because Document.h also depends on it.
#define DUMP_NODE_STATISTICS 0

namespace WebCore {

class Attribute;
class ClassNodeList;
class ContainerNode;
class DOMSettableTokenList;
class Document;
class Element;
class Event;
class EventContext;
class EventDispatchMediator;
class EventListener;
class FloatPoint;
class Frame;
class HTMLInputElement;
class IntRect;
class KeyboardEvent;
class NSResolver;
class NamedNodeMap;
class NameNodeList;
class NodeList;
class NodeListsNodeData;
class NodeRareData;
class NodeRenderingContext;
class PlatformKeyboardEvent;
class PlatformMouseEvent;
class PlatformWheelEvent;
class QualifiedName;
class RadioNodeList;
class RegisteredEventListener;
class RenderArena;
class RenderBox;
class RenderBoxModelObject;
class RenderObject;
class RenderStyle;
class ShadowRoot;
class TagNodeList;

#if ENABLE(GESTURE_EVENTS)
class PlatformGestureEvent;
#endif

#if ENABLE(INDIE_UI)
class UIRequestEvent;
#endif
    
#if ENABLE(TOUCH_EVENTS)
class TouchEvent;
#endif

#if ENABLE(MICRODATA)
class HTMLPropertiesCollection;
class PropertyNodeList;
#endif

typedef int ExceptionCode;

const int nodeStyleChangeShift = 15;

// SyntheticStyleChange means that we need to go through the entire style change logic even though
// no style property has actually changed. It is used to restructure the tree when, for instance,
// RenderLayers are created or destroyed due to animation changes.
enum StyleChangeType { 
    NoStyleChange = 0, 
    InlineStyleChange = 1 << nodeStyleChangeShift, 
    FullStyleChange = 2 << nodeStyleChangeShift, 
    SyntheticStyleChange = 3 << nodeStyleChangeShift
};

class NodeRareDataBase {
public:
    RenderObject* renderer() const { return m_renderer; }
    void setRenderer(RenderObject* renderer) { m_renderer = renderer; }

protected:
    NodeRareDataBase(RenderObject* renderer)
        : m_renderer(renderer)
    { }

private:
    RenderObject* m_renderer;
};

enum AttachBehavior {
    AttachNow,
    AttachLazily,
};

class Node : public EventTarget, public ScriptWrappable, public TreeShared<Node> {
    friend class Document;
    friend class TreeScope;
    friend class TreeScopeAdopter;

public:
    enum NodeType {
        ELEMENT_NODE = 1,
        ATTRIBUTE_NODE = 2,
        TEXT_NODE = 3,
        CDATA_SECTION_NODE = 4,
        ENTITY_REFERENCE_NODE = 5,
        ENTITY_NODE = 6,
        PROCESSING_INSTRUCTION_NODE = 7,
        COMMENT_NODE = 8,
        DOCUMENT_NODE = 9,
        DOCUMENT_TYPE_NODE = 10,
        DOCUMENT_FRAGMENT_NODE = 11,
        NOTATION_NODE = 12,
        XPATH_NAMESPACE_NODE = 13,
    };
    enum DocumentPosition {
        DOCUMENT_POSITION_EQUIVALENT = 0x00,
        DOCUMENT_POSITION_DISCONNECTED = 0x01,
        DOCUMENT_POSITION_PRECEDING = 0x02,
        DOCUMENT_POSITION_FOLLOWING = 0x04,
        DOCUMENT_POSITION_CONTAINS = 0x08,
        DOCUMENT_POSITION_CONTAINED_BY = 0x10,
        DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC = 0x20,
    };

    static bool isSupported(const String& feature, const String& version);

    static void startIgnoringLeaks();
    static void stopIgnoringLeaks();

    static void dumpStatistics();

    enum StyleChange { NoChange, NoInherit, Inherit, Detach, Force };    
    static StyleChange diff(const RenderStyle*, const RenderStyle*, Document*);

    virtual ~Node();
    void willBeDeletedFrom(Document*);

    // DOM methods & attributes for Node

    bool hasTagName(const QualifiedName&) const;
    bool hasLocalName(const AtomicString&) const;
    virtual String nodeName() const = 0;
    virtual String nodeValue() const;
    virtual void setNodeValue(const String&, ExceptionCode&);
    virtual NodeType nodeType() const = 0;
    ContainerNode* parentNode() const;
    Element* parentElement() const;
    Node* previousSibling() const { return m_previous; }
    Node* nextSibling() const { return m_next; }
    PassRefPtr<NodeList> childNodes();
    Node* firstChild() const;
    Node* lastChild() const;
    bool hasAttributes() const;
    NamedNodeMap* attributes() const;
    Node* pseudoAwareNextSibling() const;
    Node* pseudoAwarePreviousSibling() const;
    Node* pseudoAwareFirstChild() const;
    Node* pseudoAwareLastChild() const;

    virtual KURL baseURI() const;
    
    void getSubresourceURLs(ListHashSet<KURL>&) const;

    // These should all actually return a node, but this is only important for language bindings,
    // which will already know and hold a ref on the right node to return. Returning bool allows
    // these methods to be more efficient since they don't need to return a ref
    bool insertBefore(PassRefPtr<Node> newChild, Node* refChild, ExceptionCode&, AttachBehavior = AttachNow);
    bool replaceChild(PassRefPtr<Node> newChild, Node* oldChild, ExceptionCode&, AttachBehavior = AttachNow);
    bool removeChild(Node* child, ExceptionCode&);
    bool appendChild(PassRefPtr<Node> newChild, ExceptionCode&, AttachBehavior = AttachNow);

    void remove(ExceptionCode&);
    bool hasChildNodes() const { return firstChild(); }
    virtual PassRefPtr<Node> cloneNode(bool deep) = 0;
    virtual const AtomicString& localName() const;
    virtual const AtomicString& namespaceURI() const;
    virtual const AtomicString& prefix() const;
    virtual void setPrefix(const AtomicString&, ExceptionCode&);
    void normalize();

    bool isSameNode(Node* other) const { return this == other; }
    bool isEqualNode(Node*) const;
    bool isDefaultNamespace(const AtomicString& namespaceURI) const;
    String lookupPrefix(const AtomicString& namespaceURI) const;
    String lookupNamespaceURI(const String& prefix) const;
    String lookupNamespacePrefix(const AtomicString& namespaceURI, const Element* originalElement) const;
    
    String textContent(bool convertBRsToNewlines = false) const;
    void setTextContent(const String&, ExceptionCode&);
    
    Node* lastDescendant() const;
    Node* firstDescendant() const;

    // Other methods (not part of DOM)

    bool isElementNode() const { return getFlag(IsElementFlag); }
    bool isContainerNode() const { return getFlag(IsContainerFlag); }
    bool isTextNode() const { return getFlag(IsTextFlag); }
    bool isHTMLElement() const { return getFlag(IsHTMLFlag); }
    bool isSVGElement() const { return getFlag(IsSVGFlag); }

    bool isPseudoElement() const { return pseudoId() != NOPSEUDO; }
    bool isBeforePseudoElement() const { return pseudoId() == BEFORE; }
    bool isAfterPseudoElement() const { return pseudoId() == AFTER; }
    PseudoId pseudoId() const { return (isElementNode() && hasCustomStyleCallbacks()) ? customPseudoId() : NOPSEUDO; }

    virtual bool isMediaControlElement() const { return false; }
    virtual bool isMediaControls() const { return false; }
#if ENABLE(VIDEO_TRACK)
    virtual bool isWebVTTElement() const { return false; }
#endif
    bool isStyledElement() const { return getFlag(IsStyledElementFlag); }
    virtual bool isAttributeNode() const { return false; }
    virtual bool isCharacterDataNode() const { return false; }
    virtual bool isFrameOwnerElement() const { return false; }
    virtual bool isPluginElement() const { return false; }
    virtual bool isInsertionPointNode() const { return false; }

    bool isDocumentNode() const;
    bool isTreeScope() const;
    bool isDocumentFragment() const { return getFlag(IsDocumentFragmentFlag); }
    bool isShadowRoot() const { return isDocumentFragment() && isTreeScope(); }
    bool isInsertionPoint() const { return getFlag(NeedsShadowTreeWalkerFlag) && isInsertionPointNode(); }
    // Returns Node rather than InsertionPoint. Should be used only for language bindings.
    Node* insertionParentForBinding() const;

    bool needsShadowTreeWalker() const;
    bool needsShadowTreeWalkerSlow() const;
    void setNeedsShadowTreeWalker() { setFlag(NeedsShadowTreeWalkerFlag); }
    void resetNeedsShadowTreeWalker() { setFlag(needsShadowTreeWalkerSlow(), NeedsShadowTreeWalkerFlag); }

    bool inNamedFlow() const { return getFlag(InNamedFlowFlag); }
    bool hasCustomStyleCallbacks() const { return getFlag(HasCustomStyleCallbacksFlag); }

    bool isRegisteredWithNamedFlow() const;

    bool hasSyntheticAttrChildNodes() const { return getFlag(HasSyntheticAttrChildNodesFlag); }
    void setHasSyntheticAttrChildNodes(bool flag) { setFlag(flag, HasSyntheticAttrChildNodesFlag); }

    // If this node is in a shadow tree, returns its shadow host. Otherwise, returns 0.
    Element* shadowHost() const;
    // If this node is in a shadow tree, returns its shadow host. Otherwise, returns this.
    // Deprecated. Should use shadowHost() and check the return value.
    Node* deprecatedShadowAncestorNode() const;
    ShadowRoot* containingShadowRoot() const;
    ShadowRoot* shadowRoot() const;

    // Returns 0, a child of ShadowRoot, or a legacy shadow root.
    Node* nonBoundaryShadowTreeRootNode();

    // Node's parent, shadow tree host.
    ContainerNode* parentOrShadowHostNode() const;
    Element* parentOrShadowHostElement() const;
    void setParentOrShadowHostNode(ContainerNode*);
    Node* highestAncestor() const;

    // Use when it's guaranteed to that shadowHost is 0.
    ContainerNode* parentNodeGuaranteedHostFree() const;
    // Returns the parent node, but 0 if the parent node is a ShadowRoot.
    ContainerNode* nonShadowBoundaryParentNode() const;

    bool selfOrAncestorHasDirAutoAttribute() const { return getFlag(SelfOrAncestorHasDirAutoFlag); }
    void setSelfOrAncestorHasDirAutoAttribute(bool flag) { setFlag(flag, SelfOrAncestorHasDirAutoFlag); }

    // Returns the enclosing event parent node (or self) that, when clicked, would trigger a navigation.
    Node* enclosingLinkEventParentOrSelf();

    // These low-level calls give the caller responsibility for maintaining the integrity of the tree.
    void setPreviousSibling(Node* previous) { m_previous = previous; }
    void setNextSibling(Node* next) { m_next = next; }

    virtual bool canContainRangeEndPoint() const { return false; }

    bool isRootEditableElement() const;
    Element* rootEditableElement() const;
    Element* rootEditableElement(EditableType) const;

    // Called by the parser when this element's close tag is reached,
    // signaling that all child tags have been parsed and added.
    // This is needed for <applet> and <object> elements, which can't lay themselves out
    // until they know all of their nested <param>s. [Radar 3603191, 4040848].
    // Also used for script elements and some SVG elements for similar purposes,
    // but making parsing a special case in this respect should be avoided if possible.
    virtual void finishParsingChildren() { }
    virtual void beginParsingChildren() { }

    // For <link> and <style> elements.
    virtual bool sheetLoaded() { return true; }
    virtual void notifyLoadedSheetAndAllCriticalSubresources(bool /* error loading subresource */) { }
    virtual void startLoadingDynamicSheet() { ASSERT_NOT_REACHED(); }

    bool isUserActionElement() const { return getFlag(IsUserActionElement); }
    void setUserActionElement(bool flag) { setFlag(flag, IsUserActionElement); }

    bool attached() const { return getFlag(IsAttachedFlag); }
    void setAttached() { setFlag(IsAttachedFlag); }
    bool needsStyleRecalc() const { return styleChangeType() != NoStyleChange; }
    StyleChangeType styleChangeType() const { return static_cast<StyleChangeType>(m_nodeFlags & StyleChangeMask); }
    bool childNeedsStyleRecalc() const { return getFlag(ChildNeedsStyleRecalcFlag); }
    bool isLink() const { return getFlag(IsLinkFlag); }
    bool isEditingText() const { return getFlag(IsEditingTextFlag); }

    void setChildNeedsStyleRecalc() { setFlag(ChildNeedsStyleRecalcFlag); }
    void clearChildNeedsStyleRecalc() { clearFlag(ChildNeedsStyleRecalcFlag); }

    void setNeedsStyleRecalc(StyleChangeType changeType = FullStyleChange);
    void clearNeedsStyleRecalc() { m_nodeFlags &= ~StyleChangeMask; }
    virtual void scheduleSetNeedsStyleRecalc(StyleChangeType changeType = FullStyleChange) { setNeedsStyleRecalc(changeType); }

    void setIsLink(bool f) { setFlag(f, IsLinkFlag); }
    void setIsLink() { setFlag(IsLinkFlag); }
    void clearIsLink() { clearFlag(IsLinkFlag); }

    void setInNamedFlow() { setFlag(InNamedFlowFlag); }
    void clearInNamedFlow() { clearFlag(InNamedFlowFlag); }

    bool hasScopedHTMLStyleChild() const { return getFlag(HasScopedHTMLStyleChildFlag); }
    void setHasScopedHTMLStyleChild(bool flag) { setFlag(flag, HasScopedHTMLStyleChildFlag); }

    bool hasEventTargetData() const { return getFlag(HasEventTargetDataFlag); }
    void setHasEventTargetData(bool flag) { setFlag(flag, HasEventTargetDataFlag); }

    enum ShouldSetAttached {
        SetAttached,
        DoNotSetAttached
    };
    void lazyAttach(ShouldSetAttached = SetAttached);
    void lazyReattach(ShouldSetAttached = SetAttached);

    enum UserSelectAllTreatment {
        UserSelectAllDoesNotAffectEditability,
        UserSelectAllIsAlwaysNonEditable
    };
    bool isContentEditable(UserSelectAllTreatment = UserSelectAllDoesNotAffectEditability);
    bool isContentRichlyEditable();

    void inspect();

    bool rendererIsEditable(EditableType editableType = ContentIsEditable, UserSelectAllTreatment treatment = UserSelectAllIsAlwaysNonEditable) const
    {
        switch (editableType) {
        case ContentIsEditable:
            return rendererIsEditable(Editable, treatment);
        case HasEditableAXRole:
            return isEditableToAccessibility(Editable);
        }
        ASSERT_NOT_REACHED();
        return false;
    }

    bool rendererIsRichlyEditable(EditableType editableType = ContentIsEditable) const
    {
        switch (editableType) {
        case ContentIsEditable:
            return rendererIsEditable(RichlyEditable, UserSelectAllIsAlwaysNonEditable);
        case HasEditableAXRole:
            return isEditableToAccessibility(RichlyEditable);
        }
        ASSERT_NOT_REACHED();
        return false;
    }

    virtual LayoutRect boundingBox() const;
    IntRect pixelSnappedBoundingBox() const { return pixelSnappedIntRect(boundingBox()); }
    LayoutRect renderRect(bool* isReplaced);
    IntRect pixelSnappedRenderRect(bool* isReplaced) { return pixelSnappedIntRect(renderRect(isReplaced)); }

    unsigned nodeIndex() const;

    // Returns the DOM ownerDocument attribute. This method never returns NULL, except in the case 
    // of (1) a Document node or (2) a DocumentType node that is not used with any Document yet. 
    Document* ownerDocument() const;

    // Returns the document associated with this node. This method never returns NULL, except in the case 
    // of a DocumentType node that is not used with any Document yet. A Document node returns itself.
    Document* document() const
    {
        ASSERT(this);
        // FIXME: below ASSERT is useful, but prevents the use of document() in the constructor or destructor
        // due to the virtual function call to nodeType().
        ASSERT(documentInternal() || (nodeType() == DOCUMENT_TYPE_NODE && !inDocument()));
        return documentInternal();
    }

    TreeScope* treeScope() const { return m_treeScope; }

    // Returns true if this node is associated with a document and is in its associated document's
    // node tree, false otherwise.
    bool inDocument() const 
    { 
        ASSERT(documentInternal() || !getFlag(InDocumentFlag));
        return getFlag(InDocumentFlag);
    }
    bool isInShadowTree() const { return getFlag(IsInShadowTreeFlag); }
    bool isInTreeScope() const { return getFlag(static_cast<NodeFlags>(InDocumentFlag | IsInShadowTreeFlag)); }

    bool isReadOnlyNode() const { return nodeType() == ENTITY_REFERENCE_NODE; }
    bool isDocumentTypeNode() const { return nodeType() == DOCUMENT_TYPE_NODE; }
    virtual bool childTypeAllowed(NodeType) const { return false; }
    unsigned childNodeCount() const;
    Node* childNode(unsigned index) const;

    void checkSetPrefix(const AtomicString& prefix, ExceptionCode&);
    bool isDescendantOf(const Node*) const;
    bool contains(const Node*) const;
    bool containsIncludingShadowDOM(const Node*) const;
    bool containsIncludingHostElements(const Node*) const;

    // Used to determine whether range offsets use characters or node indices.
    virtual bool offsetInCharacters() const;
    // Number of DOM 16-bit units contained in node. Note that rendered text length can be different - e.g. because of
    // css-transform:capitalize breaking up precomposed characters and ligatures.
    virtual int maxCharacterOffset() const;

    // Whether or not a selection can be started in this object
    virtual bool canStartSelection() const;

    // Getting points into and out of screen space
    FloatPoint convertToPage(const FloatPoint&) const;
    FloatPoint convertFromPage(const FloatPoint&) const;

    // -----------------------------------------------------------------------------
    // Integration with rendering tree

    // As renderer() includes a branch you should avoid calling it repeatedly in hot code paths.
    RenderObject* renderer() const { return hasRareData() ? m_data.m_rareData->renderer() : m_data.m_renderer; };
    void setRenderer(RenderObject* renderer)
    {
        if (hasRareData())
            m_data.m_rareData->setRenderer(renderer);
        else
            m_data.m_renderer = renderer;
    }

    // Use these two methods with caution.
    RenderBox* renderBox() const;
    RenderBoxModelObject* renderBoxModelObject() const;

    struct AttachContext {
        RenderStyle* resolvedStyle;
        bool performingReattach;

        AttachContext() : resolvedStyle(0), performingReattach(false) { }
    };

    // Attaches this node to the rendering tree. This calculates the style to be applied to the node and creates an
    // appropriate RenderObject which will be inserted into the tree (except when the style has display: none). This
    // makes the node visible in the FrameView.
    virtual void attach(const AttachContext& = AttachContext());

    // Detaches the node from the rendering tree, making it invisible in the rendered view. This method will remove
    // the node's rendering object from the rendering tree and delete it.
    virtual void detach(const AttachContext& = AttachContext());

#ifndef NDEBUG
    bool inDetach() const;
#endif

    void reattach(const AttachContext& = AttachContext());
    void reattachIfAttached(const AttachContext& = AttachContext());
    ContainerNode* parentNodeForRenderingAndStyle();
    
    // Wrapper for nodes that don't have a renderer, but still cache the style (like HTMLOptionElement).
    RenderStyle* renderStyle() const;

    RenderStyle* computedStyle(PseudoId pseudoElementSpecifier = NOPSEUDO) { return virtualComputedStyle(pseudoElementSpecifier); }

    // -----------------------------------------------------------------------------
    // Notification of document structure changes (see ContainerNode.h for more notification methods)
    //
    // At first, WebKit notifies the node that it has been inserted into the document. This is called during document parsing, and also
    // when a node is added through the DOM methods insertBefore(), appendChild() or replaceChild(). The call happens _after_ the node has been added to the tree.
    // This is similar to the DOMNodeInsertedIntoDocument DOM event, but does not require the overhead of event
    // dispatching.
    //
    // WebKit notifies this callback regardless if the subtree of the node is a document tree or a floating subtree.
    // Implementation can determine the type of subtree by seeing insertionPoint->inDocument().
    // For a performance reason, notifications are delivered only to ContainerNode subclasses if the insertionPoint is out of document.
    //
    // There are another callback named didNotifyDescendantInsertions(), which is called after all the descendant is notified.
    // Only a few subclasses actually need this. To utilize this, the node should return InsertionShouldCallDidNotifyDescendantInsertions
    // from insrtedInto().
    //
    enum InsertionNotificationRequest {
        InsertionDone,
        InsertionShouldCallDidNotifySubtreeInsertions
    };

    virtual InsertionNotificationRequest insertedInto(ContainerNode* insertionPoint);
    virtual void didNotifySubtreeInsertions(ContainerNode*) { }

    // Notifies the node that it is no longer part of the tree.
    //
    // This is a dual of insertedInto(), and is similar to the DOMNodeRemovedFromDocument DOM event, but does not require the overhead of event
    // dispatching, and is called _after_ the node is removed from the tree.
    //
    virtual void removedFrom(ContainerNode* insertionPoint);

#ifndef NDEBUG
    virtual void formatForDebugger(char* buffer, unsigned length) const;

    void showNode(const char* prefix = "") const;
    void showTreeForThis() const;
    void showNodePathForThis() const;
    void showTreeAndMark(const Node* markedNode1, const char* markedLabel1, const Node* markedNode2 = 0, const char* markedLabel2 = 0) const;
    void showTreeForThisAcrossFrame() const;
#endif

    void invalidateNodeListCachesInAncestors(const QualifiedName* attrName = 0, Element* attributeOwnerElement = 0);
    NodeListsNodeData* nodeLists();
    void clearNodeLists();

    PassRefPtr<NodeList> getElementsByTagName(const AtomicString&);
    PassRefPtr<NodeList> getElementsByTagNameNS(const AtomicString& namespaceURI, const AtomicString& localName);
    PassRefPtr<NodeList> getElementsByName(const String& elementName);
    PassRefPtr<NodeList> getElementsByClassName(const String& classNames);
    PassRefPtr<RadioNodeList> radioNodeList(const AtomicString&);

    virtual bool willRespondToMouseMoveEvents();
    virtual bool willRespondToMouseClickEvents();
    virtual bool willRespondToTouchEvents();

    PassRefPtr<Element> querySelector(const AtomicString& selectors, ExceptionCode&);
    PassRefPtr<NodeList> querySelectorAll(const AtomicString& selectors, ExceptionCode&);

    unsigned short compareDocumentPosition(Node*);

    virtual Node* toNode();
    virtual HTMLInputElement* toInputElement();

    virtual const AtomicString& interfaceName() const;
    virtual ScriptExecutionContext* scriptExecutionContext() const;

    virtual bool addEventListener(const AtomicString& eventType, PassRefPtr<EventListener>, bool useCapture);
    virtual bool removeEventListener(const AtomicString& eventType, EventListener*, bool useCapture);

    // Handlers to do/undo actions on the target node before an event is dispatched to it and after the event
    // has been dispatched.  The data pointer is handed back by the preDispatch and passed to postDispatch.
    virtual void* preDispatchEventHandler(Event*) { return 0; }
    virtual void postDispatchEventHandler(Event*, void* /*dataFromPreDispatch*/) { }

    using EventTarget::dispatchEvent;
    virtual bool dispatchEvent(PassRefPtr<Event>) OVERRIDE;

    void dispatchScopedEvent(PassRefPtr<Event>);
    void dispatchScopedEventDispatchMediator(PassRefPtr<EventDispatchMediator>);

    virtual void handleLocalEvents(Event*);

    void dispatchSubtreeModifiedEvent();
    bool dispatchDOMActivateEvent(int detail, PassRefPtr<Event> underlyingEvent);

    bool dispatchKeyEvent(const PlatformKeyboardEvent&);
    bool dispatchWheelEvent(const PlatformWheelEvent&);
    bool dispatchMouseEvent(const PlatformMouseEvent&, const AtomicString& eventType, int clickCount = 0, Node* relatedTarget = 0);
#if ENABLE(GESTURE_EVENTS)
    bool dispatchGestureEvent(const PlatformGestureEvent&);
#endif
#if ENABLE(TOUCH_EVENTS)
    bool dispatchTouchEvent(PassRefPtr<TouchEvent>);
#endif
#if ENABLE(INDIE_UI)
    bool dispatchUIRequestEvent(PassRefPtr<UIRequestEvent>);
#endif

    bool dispatchBeforeLoadEvent(const String& sourceURL);

    virtual void dispatchInputEvent();

    // Perform the default action for an event.
    virtual void defaultEventHandler(Event*);

    using TreeShared<Node>::ref;
    using TreeShared<Node>::deref;

    virtual EventTargetData* eventTargetData();
    virtual EventTargetData* ensureEventTargetData();

#if ENABLE(MICRODATA)
    DOMSettableTokenList* itemProp();
    DOMSettableTokenList* itemRef();
    DOMSettableTokenList* itemType();
    PassRefPtr<PropertyNodeList> propertyNodeList(const String&);
#endif

    void getRegisteredMutationObserversOfType(HashMap<MutationObserver*, MutationRecordDeliveryOptions>&, MutationObserver::MutationType, const QualifiedName* attributeName);
    void registerMutationObserver(MutationObserver*, MutationObserverOptions, const HashSet<AtomicString>& attributeFilter);
    void unregisterMutationObserver(MutationObserverRegistration*);
    void registerTransientMutationObserver(MutationObserverRegistration*);
    void unregisterTransientMutationObserver(MutationObserverRegistration*);
    void notifyMutationObserversNodeWillDetach();

    virtual void registerScopedHTMLStyleChild();
    virtual void unregisterScopedHTMLStyleChild();
    size_t numberOfScopedHTMLStyleChildren() const;

    void textRects(Vector<IntRect>&) const;

    unsigned connectedSubframeCount() const;
    void incrementConnectedSubframeCount(unsigned amount = 1);
    void decrementConnectedSubframeCount(unsigned amount = 1);
    void updateAncestorConnectedSubframeCountForRemoval() const;
    void updateAncestorConnectedSubframeCountForInsertion() const;

private:
    enum NodeFlags {
        IsTextFlag = 1,
        IsContainerFlag = 1 << 1,
        IsElementFlag = 1 << 2,
        IsStyledElementFlag = 1 << 3,
        IsHTMLFlag = 1 << 4,
        IsSVGFlag = 1 << 5,
        IsAttachedFlag = 1 << 6,
        ChildNeedsStyleRecalcFlag = 1 << 7,
        InDocumentFlag = 1 << 8,
        IsLinkFlag = 1 << 9,
        IsUserActionElement = 1 << 10,
        HasRareDataFlag = 1 << 11,
        IsDocumentFragmentFlag = 1 << 12,

        // These bits are used by derived classes, pulled up here so they can
        // be stored in the same memory word as the Node bits above.
        IsParsingChildrenFinishedFlag = 1 << 13, // Element
#if ENABLE(SVG)
        HasSVGRareDataFlag = 1 << 14, // SVGElement
#endif

        StyleChangeMask = 1 << nodeStyleChangeShift | 1 << (nodeStyleChangeShift + 1),

        SelfOrAncestorHasDirAutoFlag = 1 << 17,

        IsEditingTextFlag = 1 << 18,

        InNamedFlowFlag = 1 << 19,
        HasSyntheticAttrChildNodesFlag = 1 << 20,
        HasCustomStyleCallbacksFlag = 1 << 21,
        HasScopedHTMLStyleChildFlag = 1 << 22,
        HasEventTargetDataFlag = 1 << 23,
        NeedsShadowTreeWalkerFlag = 1 << 25,
        IsInShadowTreeFlag = 1 << 26,

        DefaultNodeFlags = IsParsingChildrenFinishedFlag
    };

    // 5 bits remaining

    bool getFlag(NodeFlags mask) const { return m_nodeFlags & mask; }
    void setFlag(bool f, NodeFlags mask) const { m_nodeFlags = (m_nodeFlags & ~mask) | (-(int32_t)f & mask); } 
    void setFlag(NodeFlags mask) const { m_nodeFlags |= mask; } 
    void clearFlag(NodeFlags mask) const { m_nodeFlags &= ~mask; } 

protected:
    enum ConstructionType { 
        CreateOther = DefaultNodeFlags,
        CreateText = DefaultNodeFlags | IsTextFlag,
        CreateContainer = DefaultNodeFlags | IsContainerFlag, 
        CreateElement = CreateContainer | IsElementFlag, 
        CreatePseudoElement =  CreateElement | InDocumentFlag | NeedsShadowTreeWalkerFlag,
        CreateShadowRoot = CreateContainer | IsDocumentFragmentFlag | NeedsShadowTreeWalkerFlag | IsInShadowTreeFlag,
        CreateDocumentFragment = CreateContainer | IsDocumentFragmentFlag,
        CreateStyledElement = CreateElement | IsStyledElementFlag, 
        CreateHTMLElement = CreateStyledElement | IsHTMLFlag,
        CreateSVGElement = CreateStyledElement | IsSVGFlag,
        CreateDocument = CreateContainer | InDocumentFlag,
        CreateInsertionPoint = CreateHTMLElement | NeedsShadowTreeWalkerFlag,
        CreateEditingText = CreateText | IsEditingTextFlag,
    };
    Node(Document*, ConstructionType);

    virtual void didMoveToNewDocument(Document* oldDocument);
    
    virtual void addSubresourceAttributeURLs(ListHashSet<KURL>&) const { }

    bool hasRareData() const { return getFlag(HasRareDataFlag); }

    NodeRareData* rareData() const;
    NodeRareData* ensureRareData();
    void clearRareData();

    void clearEventTargetData();

    void setHasCustomStyleCallbacks() { setFlag(true, HasCustomStyleCallbacksFlag); }

    Document* documentInternal() const { return treeScope()->documentScope(); }
    void setTreeScope(TreeScope* scope) { m_treeScope = scope; }

private:
    friend class TreeShared<Node>;

    virtual PseudoId customPseudoId() const
    {
        ASSERT(hasCustomStyleCallbacks());
        return NOPSEUDO;
    }

    void removedLastRef();
    bool hasTreeSharedParent() const { return !!parentOrShadowHostNode(); }

    enum EditableLevel { Editable, RichlyEditable };
    bool rendererIsEditable(EditableLevel, UserSelectAllTreatment = UserSelectAllIsAlwaysNonEditable) const;
    bool isEditableToAccessibility(EditableLevel) const;

    void setStyleChange(StyleChangeType);

    // Used to share code between lazyAttach and setNeedsStyleRecalc.
    void markAncestorsWithChildNeedsStyleRecalc();

    virtual void refEventTarget();
    virtual void derefEventTarget();

    virtual RenderStyle* nonRendererStyle() const { return 0; }
    virtual RenderStyle* virtualComputedStyle(PseudoId = NOPSEUDO);

    Element* ancestorElement() const;

    void trackForDebugging();

    Vector<OwnPtr<MutationObserverRegistration> >* mutationObserverRegistry();
    HashSet<MutationObserverRegistration*>* transientMutationObserverRegistry();

    mutable uint32_t m_nodeFlags;
    ContainerNode* m_parentOrShadowHostNode;
    TreeScope* m_treeScope;
    Node* m_previous;
    Node* m_next;
    // When a node has rare data we move the renderer into the rare data.
    union DataUnion {
        DataUnion() : m_renderer(0) { }
        RenderObject* m_renderer;
        NodeRareDataBase* m_rareData;
    } m_data;

protected:
    bool isParsingChildrenFinished() const { return getFlag(IsParsingChildrenFinishedFlag); }
    void setIsParsingChildrenFinished() { setFlag(IsParsingChildrenFinishedFlag); }
    void clearIsParsingChildrenFinished() { clearFlag(IsParsingChildrenFinishedFlag); }

#if ENABLE(SVG)
    bool hasSVGRareData() const { return getFlag(HasSVGRareDataFlag); }
    void setHasSVGRareData() { setFlag(HasSVGRareDataFlag); }
    void clearHasSVGRareData() { clearFlag(HasSVGRareDataFlag); }
#endif

#if ENABLE(MICRODATA)
    void setItemProp(const String&);
    void setItemRef(const String&);
    void setItemType(const String&);
#endif
};

// Used in Node::addSubresourceAttributeURLs() and in addSubresourceStyleURLs()
inline void addSubresourceURL(ListHashSet<KURL>& urls, const KURL& url)
{
    if (!url.isNull())
        urls.add(url);
}

inline void Node::setParentOrShadowHostNode(ContainerNode* parent)
{
    ASSERT(isMainThread());
    m_parentOrShadowHostNode = parent;
}

inline ContainerNode* Node::parentOrShadowHostNode() const
{
    ASSERT(isMainThreadOrGCThread());
    return m_parentOrShadowHostNode;
}

inline ContainerNode* Node::parentNode() const
{
    return isShadowRoot() ? 0 : parentOrShadowHostNode();
}

inline ContainerNode* Node::parentNodeGuaranteedHostFree() const
{
    ASSERT(!isShadowRoot());
    return parentOrShadowHostNode();
}

inline void Node::reattach(const AttachContext& context)
{
    AttachContext reattachContext(context);
    reattachContext.performingReattach = true;

    if (attached())
        detach(reattachContext);
    attach(reattachContext);
}

inline void Node::reattachIfAttached(const AttachContext& context)
{
    if (attached())
        reattach(context);
}

inline void Node::lazyReattach(ShouldSetAttached shouldSetAttached)
{
    AttachContext context;
    context.performingReattach = true;

    if (attached())
        detach(context);
    lazyAttach(shouldSetAttached);
}

} //namespace

#ifndef NDEBUG
// Outside the WebCore namespace for ease of invocation from gdb.
void showTree(const WebCore::Node*);
void showNodePath(const WebCore::Node*);
#endif

#endif
