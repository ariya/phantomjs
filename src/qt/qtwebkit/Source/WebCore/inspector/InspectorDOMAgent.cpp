/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2009 Joseph Pecoraro
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(INSPECTOR)

#include "InspectorDOMAgent.h"

#include "Attr.h"
#include "CSSComputedStyleDeclaration.h"
#include "CSSPropertyNames.h"
#include "CSSPropertySourceData.h"
#include "CSSRule.h"
#include "CSSRuleList.h"
#include "CSSStyleRule.h"
#include "CSSStyleSheet.h"
#include "CharacterData.h"
#include "ContainerNode.h"
#include "Cookie.h"
#include "CookieJar.h"
#include "DOMEditor.h"
#include "DOMPatchSupport.h"
#include "DOMWindow.h"
#include "Document.h"
#include "DocumentFragment.h"
#include "DocumentType.h"
#include "Element.h"
#include "ElementShadow.h"
#include "Event.h"
#include "EventContext.h"
#include "EventListener.h"
#include "EventNames.h"
#include "EventTarget.h"
#include "File.h"
#include "FileList.h"
#include "Frame.h"
#include "FrameTree.h"
#include "HTMLElement.h"
#include "HTMLFrameOwnerElement.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HTMLTemplateElement.h"
#include "HitTestResult.h"
#include "IdentifiersFactory.h"
#include "InjectedScriptManager.h"
#include "InspectorClient.h"
#include "InspectorFrontend.h"
#include "InspectorHistory.h"
#include "InspectorOverlay.h"
#include "InspectorPageAgent.h"
#include "InspectorState.h"
#include "InstrumentingAgents.h"
#include "IntRect.h"
#include "MutationEvent.h"
#include "Node.h"
#include "NodeList.h"
#include "NodeTraversal.h"
#include "Page.h"
#include "Pasteboard.h"
#include "RenderStyle.h"
#include "RenderStyleConstants.h"
#include "ScriptEventListener.h"
#include "Settings.h"
#include "ShadowRoot.h"
#include "StylePropertySet.h"
#include "StyleResolver.h"
#include "StyleSheetList.h"
#include "Text.h"
#include "XPathResult.h"

#include "htmlediting.h"
#include "markup.h"

#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>
#include <wtf/HashSet.h>
#include <wtf/ListHashSet.h>
#include <wtf/OwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

using namespace HTMLNames;

namespace DOMAgentState {
static const char documentRequested[] = "documentRequested";
};

static const size_t maxTextSize = 10000;
static const UChar ellipsisUChar[] = { 0x2026, 0 };

static Color parseColor(const RefPtr<InspectorObject>* colorObject)
{
    if (!colorObject || !(*colorObject))
        return Color::transparent;

    int r;
    int g;
    int b;
    bool success = (*colorObject)->getNumber("r", &r);
    success |= (*colorObject)->getNumber("g", &g);
    success |= (*colorObject)->getNumber("b", &b);
    if (!success)
        return Color::transparent;

    double a;
    success = (*colorObject)->getNumber("a", &a);
    if (!success)
        return Color(r, g, b);

    // Clamp alpha to the [0..1] range.
    if (a < 0)
        a = 0;
    else if (a > 1)
        a = 1;

    return Color(r, g, b, static_cast<int>(a * 255));
}

static Color parseConfigColor(const String& fieldName, InspectorObject* configObject)
{
    const RefPtr<InspectorObject> colorObject = configObject->getObject(fieldName);
    return parseColor(&colorObject);
}

static bool parseQuad(const RefPtr<InspectorArray>& quadArray, FloatQuad* quad)
{
    if (!quadArray)
        return false;
    const size_t coordinatesInQuad = 8;
    double coordinates[coordinatesInQuad];
    if (quadArray->length() != coordinatesInQuad)
        return false;
    for (size_t i = 0; i < coordinatesInQuad; ++i) {
        if (!quadArray->get(i)->asNumber(coordinates + i))
            return false;
    }
    quad->setP1(FloatPoint(coordinates[0], coordinates[1]));
    quad->setP2(FloatPoint(coordinates[2], coordinates[3]));
    quad->setP3(FloatPoint(coordinates[4], coordinates[5]));
    quad->setP4(FloatPoint(coordinates[6], coordinates[7]));

    return true;
}

class RevalidateStyleAttributeTask {
    WTF_MAKE_FAST_ALLOCATED;
public:
    RevalidateStyleAttributeTask(InspectorDOMAgent*);
    void scheduleFor(Element*);
    void reset() { m_timer.stop(); }
    void onTimer(Timer<RevalidateStyleAttributeTask>*);

private:
    InspectorDOMAgent* m_domAgent;
    Timer<RevalidateStyleAttributeTask> m_timer;
    HashSet<RefPtr<Element> > m_elements;
};

RevalidateStyleAttributeTask::RevalidateStyleAttributeTask(InspectorDOMAgent* domAgent)
    : m_domAgent(domAgent)
    , m_timer(this, &RevalidateStyleAttributeTask::onTimer)
{
}

void RevalidateStyleAttributeTask::scheduleFor(Element* element)
{
    m_elements.add(element);
    if (!m_timer.isActive())
        m_timer.startOneShot(0);
}

void RevalidateStyleAttributeTask::onTimer(Timer<RevalidateStyleAttributeTask>*)
{
    // The timer is stopped on m_domAgent destruction, so this method will never be called after m_domAgent has been destroyed.
    Vector<Element*> elements;
    for (HashSet<RefPtr<Element> >::iterator it = m_elements.begin(), end = m_elements.end(); it != end; ++it)
        elements.append(it->get());
    m_domAgent->styleAttributeInvalidated(elements);

    m_elements.clear();
}

String InspectorDOMAgent::toErrorString(const ExceptionCode& ec)
{
    if (ec) {
        ExceptionCodeDescription description(ec);
        return description.name;
    }
    return "";
}

InspectorDOMAgent::InspectorDOMAgent(InstrumentingAgents* instrumentingAgents, InspectorPageAgent* pageAgent, InspectorCompositeState* inspectorState, InjectedScriptManager* injectedScriptManager, InspectorOverlay* overlay, InspectorClient* client)
    : InspectorBaseAgent<InspectorDOMAgent>("DOM", instrumentingAgents, inspectorState)
    , m_pageAgent(pageAgent)
    , m_injectedScriptManager(injectedScriptManager)
    , m_overlay(overlay)
    , m_client(client)
    , m_frontend(0)
    , m_domListener(0)
    , m_lastNodeId(1)
    , m_lastBackendNodeId(-1)
    , m_searchingForNode(false)
    , m_suppressAttributeModifiedEvent(false)
{
}

InspectorDOMAgent::~InspectorDOMAgent()
{
    reset();
    ASSERT(!m_searchingForNode);
}

void InspectorDOMAgent::setFrontend(InspectorFrontend* frontend)
{
    ASSERT(!m_frontend);
    m_history = adoptPtr(new InspectorHistory());
    m_domEditor = adoptPtr(new DOMEditor(m_history.get()));

    m_frontend = frontend->dom();
    m_instrumentingAgents->setInspectorDOMAgent(this);
    m_document = m_pageAgent->mainFrame()->document();

    if (m_nodeToFocus)
        focusNode();
}

void InspectorDOMAgent::clearFrontend()
{
    ASSERT(m_frontend);

    m_history.clear();
    m_domEditor.clear();

    ErrorString error;
    setSearchingForNode(&error, false, 0);
    hideHighlight(&error);

    m_frontend = 0;
    m_instrumentingAgents->setInspectorDOMAgent(0);
    m_state->setBoolean(DOMAgentState::documentRequested, false);
    reset();
}

void InspectorDOMAgent::restore()
{
    // Reset document to avoid early return from setDocument.
    m_document = 0;
    setDocument(m_pageAgent->mainFrame()->document());
}

Vector<Document*> InspectorDOMAgent::documents()
{
    Vector<Document*> result;
    for (Frame* frame = m_document->frame(); frame; frame = frame->tree()->traverseNext()) {
        Document* document = frame->document();
        if (!document)
            continue;
        result.append(document);
    }
    return result;
}

void InspectorDOMAgent::reset()
{
    if (m_history)
        m_history->reset();
    m_searchResults.clear();
    discardBindings();
    if (m_revalidateStyleAttrTask)
        m_revalidateStyleAttrTask->reset();
    m_document = 0;
}

void InspectorDOMAgent::setDOMListener(DOMListener* listener)
{
    m_domListener = listener;
}

void InspectorDOMAgent::setDocument(Document* doc)
{
    if (doc == m_document.get())
        return;

    reset();

    m_document = doc;

    if (!m_state->getBoolean(DOMAgentState::documentRequested))
        return;

    // Immediately communicate 0 document or document that has finished loading.
    if (!doc || !doc->parsing())
        m_frontend->documentUpdated();
}

void InspectorDOMAgent::releaseDanglingNodes()
{
    m_danglingNodeToIdMaps.clear();
}

int InspectorDOMAgent::bind(Node* node, NodeToIdMap* nodesMap)
{
    int id = nodesMap->get(node);
    if (id)
        return id;
    id = m_lastNodeId++;
    nodesMap->set(node, id);
    m_idToNode.set(id, node);
    m_idToNodesMap.set(id, nodesMap);
    return id;
}

void InspectorDOMAgent::unbind(Node* node, NodeToIdMap* nodesMap)
{
    int id = nodesMap->get(node);
    if (!id)
        return;

    m_idToNode.remove(id);

    if (node->isFrameOwnerElement()) {
        const HTMLFrameOwnerElement* frameOwner = static_cast<const HTMLFrameOwnerElement*>(node);
        Document* contentDocument = frameOwner->contentDocument();
        if (m_domListener)
            m_domListener->didRemoveDocument(contentDocument);
        if (contentDocument)
            unbind(contentDocument, nodesMap);
    }

    if (node->isElementNode()) {
        if (ElementShadow* shadow = toElement(node)->shadow()) {
            if (ShadowRoot* root = shadow->shadowRoot())
                unbind(root, nodesMap);
        }
    }

    nodesMap->remove(node);
    if (m_domListener)
        m_domListener->didRemoveDOMNode(node);

    bool childrenRequested = m_childrenRequested.contains(id);
    if (childrenRequested) {
        // Unbind subtree known to client recursively.
        m_childrenRequested.remove(id);
        Node* child = innerFirstChild(node);
        while (child) {
            unbind(child, nodesMap);
            child = innerNextSibling(child);
        }
    }
}

Node* InspectorDOMAgent::assertNode(ErrorString* errorString, int nodeId)
{
    Node* node = nodeForId(nodeId);
    if (!node) {
        *errorString = "Could not find node with given id";
        return 0;
    }
    return node;
}

Document* InspectorDOMAgent::assertDocument(ErrorString* errorString, int nodeId)
{
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return 0;

    if (!(node->isDocumentNode())) {
        *errorString = "Document is not available";
        return 0;
    }
    return toDocument(node);
}

Element* InspectorDOMAgent::assertElement(ErrorString* errorString, int nodeId)
{
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return 0;

    if (node->nodeType() != Node::ELEMENT_NODE) {
        *errorString = "Node is not an Element";
        return 0;
    }
    return toElement(node);
}

Node* InspectorDOMAgent::assertEditableNode(ErrorString* errorString, int nodeId)
{
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return 0;

    if (node->isInShadowTree()) {
        *errorString = "Can not edit nodes from shadow trees";
        return 0;
    }

    return node;
}

Element* InspectorDOMAgent::assertEditableElement(ErrorString* errorString, int nodeId)
{
    Element* element = assertElement(errorString, nodeId);
    if (!element)
        return 0;

    if (element->isInShadowTree()) {
        *errorString = "Can not edit elements from shadow trees";
        return 0;
    }
    return element;
}

void InspectorDOMAgent::getDocument(ErrorString* errorString, RefPtr<TypeBuilder::DOM::Node>& root)
{
    m_state->setBoolean(DOMAgentState::documentRequested, true);

    if (!m_document) {
        *errorString = "Document is not available";
        return;
    }

    // Reset backend state.
    RefPtr<Document> doc = m_document;
    reset();
    m_document = doc;

    root = buildObjectForNode(m_document.get(), 2, &m_documentNodeToIdMap);
}

void InspectorDOMAgent::pushChildNodesToFrontend(int nodeId, int depth)
{
    Node* node = nodeForId(nodeId);
    if (!node || (node->nodeType() != Node::ELEMENT_NODE && node->nodeType() != Node::DOCUMENT_NODE && node->nodeType() != Node::DOCUMENT_FRAGMENT_NODE))
        return;

    NodeToIdMap* nodeMap = m_idToNodesMap.get(nodeId);

    if (m_childrenRequested.contains(nodeId)) {
        if (depth <= 1)
            return;

        depth--;

        for (node = innerFirstChild(node); node; node = innerNextSibling(node)) {
            int childNodeId = nodeMap->get(node);
            ASSERT(childNodeId);
            pushChildNodesToFrontend(childNodeId, depth);
        }

        return;
    }

    RefPtr<TypeBuilder::Array<TypeBuilder::DOM::Node> > children = buildArrayForContainerChildren(node, depth, nodeMap);
    m_frontend->setChildNodes(nodeId, children.release());
}

void InspectorDOMAgent::discardBindings()
{
    m_documentNodeToIdMap.clear();
    m_idToNode.clear();
    releaseDanglingNodes();
    m_childrenRequested.clear();
    m_backendIdToNode.clear();
    m_nodeGroupToBackendIdMap.clear();
}

int InspectorDOMAgent::pushNodeToFrontend(ErrorString* errorString, int documentNodeId, Node* nodeToPush)
{
    Document* document = assertDocument(errorString, documentNodeId);
    if (!document)
        return 0;
    if (nodeToPush->document() != document) {
        *errorString = "Node is not part of the document with given id";
        return 0;
    }

    return pushNodePathToFrontend(nodeToPush);
}

Node* InspectorDOMAgent::nodeForId(int id)
{
    if (!id)
        return 0;

    HashMap<int, Node*>::iterator it = m_idToNode.find(id);
    if (it != m_idToNode.end())
        return it->value;
    return 0;
}

void InspectorDOMAgent::requestChildNodes(ErrorString* errorString, int nodeId, const int* depth)
{
    int sanitizedDepth;

    if (!depth)
        sanitizedDepth = 1;
    else if (*depth == -1)
        sanitizedDepth = INT_MAX;
    else if (*depth > 0)
        sanitizedDepth = *depth;
    else {
        *errorString = "Please provide a positive integer as a depth or -1 for entire subtree";
        return;
    }

    pushChildNodesToFrontend(nodeId, sanitizedDepth);
}

void InspectorDOMAgent::querySelector(ErrorString* errorString, int nodeId, const String& selectors, int* elementId)
{
    *elementId = 0;
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return;

    ExceptionCode ec = 0;
    RefPtr<Element> element = node->querySelector(selectors, ec);
    if (ec) {
        *errorString = "DOM Error while querying";
        return;
    }

    if (element)
        *elementId = pushNodePathToFrontend(element.get());
}

void InspectorDOMAgent::querySelectorAll(ErrorString* errorString, int nodeId, const String& selectors, RefPtr<TypeBuilder::Array<int> >& result)
{
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return;

    ExceptionCode ec = 0;
    RefPtr<NodeList> nodes = node->querySelectorAll(selectors, ec);
    if (ec) {
        *errorString = "DOM Error while querying";
        return;
    }

    result = TypeBuilder::Array<int>::create();

    for (unsigned i = 0; i < nodes->length(); ++i)
        result->addItem(pushNodePathToFrontend(nodes->item(i)));
}

int InspectorDOMAgent::pushNodePathToFrontend(Node* nodeToPush)
{
    ASSERT(nodeToPush);  // Invalid input

    if (!m_document)
        return 0;
    if (!m_documentNodeToIdMap.contains(m_document))
        return 0;

    // Return id in case the node is known.
    int result = m_documentNodeToIdMap.get(nodeToPush);
    if (result)
        return result;

    Node* node = nodeToPush;
    Vector<Node*> path;
    NodeToIdMap* danglingMap = 0;

    while (true) {
        Node* parent = innerParentNode(node);
        if (!parent) {
            // Node being pushed is detached -> push subtree root.
            OwnPtr<NodeToIdMap> newMap = adoptPtr(new NodeToIdMap);
            danglingMap = newMap.get();
            m_danglingNodeToIdMaps.append(newMap.release());
            RefPtr<TypeBuilder::Array<TypeBuilder::DOM::Node> > children = TypeBuilder::Array<TypeBuilder::DOM::Node>::create();
            children->addItem(buildObjectForNode(node, 0, danglingMap));
            m_frontend->setChildNodes(0, children);
            break;
        } else {
            path.append(parent);
            if (m_documentNodeToIdMap.get(parent))
                break;
            else
                node = parent;
        }
    }

    NodeToIdMap* map = danglingMap ? danglingMap : &m_documentNodeToIdMap;
    for (int i = path.size() - 1; i >= 0; --i) {
        int nodeId = map->get(path.at(i));
        ASSERT(nodeId);
        pushChildNodesToFrontend(nodeId);
    }
    return map->get(nodeToPush);
}

int InspectorDOMAgent::boundNodeId(Node* node)
{
    return m_documentNodeToIdMap.get(node);
}

BackendNodeId InspectorDOMAgent::backendNodeIdForNode(Node* node, const String& nodeGroup)
{
    if (!node)
        return 0;

    if (!m_nodeGroupToBackendIdMap.contains(nodeGroup))
        m_nodeGroupToBackendIdMap.set(nodeGroup, NodeToBackendIdMap());

    NodeToBackendIdMap& map = m_nodeGroupToBackendIdMap.find(nodeGroup)->value;
    BackendNodeId id = map.get(node);
    if (!id) {
        id = --m_lastBackendNodeId;
        map.set(node, id);
        m_backendIdToNode.set(id, std::make_pair(node, nodeGroup));
    }

    return id;
}

void InspectorDOMAgent::releaseBackendNodeIds(ErrorString* errorString, const String& nodeGroup)
{
    if (m_nodeGroupToBackendIdMap.contains(nodeGroup)) {
        NodeToBackendIdMap& map = m_nodeGroupToBackendIdMap.find(nodeGroup)->value;
        for (NodeToBackendIdMap::iterator it = map.begin(); it != map.end(); ++it)
            m_backendIdToNode.remove(it->value);
        m_nodeGroupToBackendIdMap.remove(nodeGroup);
        return;
    }
    *errorString = "Group name not found";
}

void InspectorDOMAgent::setAttributeValue(ErrorString* errorString, int elementId, const String& name, const String& value)
{
    Element* element = assertEditableElement(errorString, elementId);
    if (!element)
        return;

    m_domEditor->setAttribute(element, name, value, errorString);
}

void InspectorDOMAgent::setAttributesAsText(ErrorString* errorString, int elementId, const String& text, const String* const name)
{
    Element* element = assertEditableElement(errorString, elementId);
    if (!element)
        return;

    RefPtr<HTMLElement> parsedElement = createHTMLElement(element->document(), spanTag);
    ExceptionCode ec = 0;
    parsedElement.get()->setInnerHTML("<span " + text + "></span>", ec);
    if (ec) {
        *errorString = InspectorDOMAgent::toErrorString(ec);
        return;
    }

    Node* child = parsedElement->firstChild();
    if (!child) {
        *errorString = "Could not parse value as attributes";
        return;
    }

    Element* childElement = toElement(child);
    if (!childElement->hasAttributes() && name) {
        m_domEditor->removeAttribute(element, *name, errorString);
        return;
    }

    bool foundOriginalAttribute = false;
    unsigned numAttrs = childElement->attributeCount();
    for (unsigned i = 0; i < numAttrs; ++i) {
        // Add attribute pair
        const Attribute* attribute = childElement->attributeItem(i);
        foundOriginalAttribute = foundOriginalAttribute || (name && attribute->name().toString() == *name);
        if (!m_domEditor->setAttribute(element, attribute->name().toString(), attribute->value(), errorString))
            return;
    }

    if (!foundOriginalAttribute && name && !name->stripWhiteSpace().isEmpty())
        m_domEditor->removeAttribute(element, *name, errorString);
}

void InspectorDOMAgent::removeAttribute(ErrorString* errorString, int elementId, const String& name)
{
    Element* element = assertEditableElement(errorString, elementId);
    if (!element)
        return;

    m_domEditor->removeAttribute(element, name, errorString);
}

void InspectorDOMAgent::removeNode(ErrorString* errorString, int nodeId)
{
    Node* node = assertEditableNode(errorString, nodeId);
    if (!node)
        return;

    ContainerNode* parentNode = node->parentNode();
    if (!parentNode) {
        *errorString = "Can not remove detached node";
        return;
    }

    m_domEditor->removeChild(parentNode, node, errorString);
}

void InspectorDOMAgent::setNodeName(ErrorString* errorString, int nodeId, const String& tagName, int* newId)
{
    *newId = 0;

    Node* oldNode = nodeForId(nodeId);
    if (!oldNode || !oldNode->isElementNode())
        return;

    ExceptionCode ec = 0;
    RefPtr<Element> newElem = oldNode->document()->createElement(tagName, ec);
    if (ec)
        return;

    // Copy over the original node's attributes.
    newElem->cloneAttributesFromElement(*toElement(oldNode));

    // Copy over the original node's children.
    Node* child;
    while ((child = oldNode->firstChild())) {
        if (!m_domEditor->insertBefore(newElem.get(), child, 0, errorString))
            return;
    }

    // Replace the old node with the new node
    ContainerNode* parent = oldNode->parentNode();
    if (!m_domEditor->insertBefore(parent, newElem.get(), oldNode->nextSibling(), errorString))
        return;
    if (!m_domEditor->removeChild(parent, oldNode, errorString))
        return;

    *newId = pushNodePathToFrontend(newElem.get());
    if (m_childrenRequested.contains(nodeId))
        pushChildNodesToFrontend(*newId);
}

void InspectorDOMAgent::getOuterHTML(ErrorString* errorString, int nodeId, WTF::String* outerHTML)
{
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return;

    *outerHTML = createMarkup(node);
}

void InspectorDOMAgent::setOuterHTML(ErrorString* errorString, int nodeId, const String& outerHTML)
{
    if (!nodeId) {
        DOMPatchSupport domPatchSupport(m_domEditor.get(), m_document.get());
        domPatchSupport.patchDocument(outerHTML);
        return;
    }

    Node* node = assertEditableNode(errorString, nodeId);
    if (!node)
        return;

    Document* document = node->isDocumentNode() ? toDocument(node) : node->ownerDocument();
    if (!document || (!document->isHTMLDocument() && !document->isXHTMLDocument()
#if ENABLE(SVG)
        && !document->isSVGDocument()
#endif
    )) {
        *errorString = "Not an HTML/XML document";
        return;
    }

    Node* newNode = 0;
    if (!m_domEditor->setOuterHTML(node, outerHTML, &newNode, errorString))
        return;

    if (!newNode) {
        // The only child node has been deleted.
        return;
    }

    int newId = pushNodePathToFrontend(newNode);

    bool childrenRequested = m_childrenRequested.contains(nodeId);
    if (childrenRequested)
        pushChildNodesToFrontend(newId);
}

void InspectorDOMAgent::setNodeValue(ErrorString* errorString, int nodeId, const String& value)
{
    Node* node = assertEditableNode(errorString, nodeId);
    if (!node)
        return;

    if (node->nodeType() != Node::TEXT_NODE) {
        *errorString = "Can only set value of text nodes";
        return;
    }

    m_domEditor->replaceWholeText(toText(node), value, errorString);
}

void InspectorDOMAgent::getEventListenersForNode(ErrorString* errorString, int nodeId, const String* objectGroup, RefPtr<TypeBuilder::Array<TypeBuilder::DOM::EventListener> >& listenersArray)
{
    listenersArray = TypeBuilder::Array<TypeBuilder::DOM::EventListener>::create();
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return;
    Vector<EventListenerInfo> eventInformation;
    getEventListeners(node, eventInformation, true);

    // Get Capturing Listeners (in this order)
    size_t eventInformationLength = eventInformation.size();
    for (size_t i = 0; i < eventInformationLength; ++i) {
        const EventListenerInfo& info = eventInformation[i];
        const EventListenerVector& vector = info.eventListenerVector;
        for (size_t j = 0; j < vector.size(); ++j) {
            const RegisteredEventListener& listener = vector[j];
            if (listener.useCapture)
                listenersArray->addItem(buildObjectForEventListener(listener, info.eventType, info.node, objectGroup));
        }
    }

    // Get Bubbling Listeners (reverse order)
    for (size_t i = eventInformationLength; i; --i) {
        const EventListenerInfo& info = eventInformation[i - 1];
        const EventListenerVector& vector = info.eventListenerVector;
        for (size_t j = 0; j < vector.size(); ++j) {
            const RegisteredEventListener& listener = vector[j];
            if (!listener.useCapture)
                listenersArray->addItem(buildObjectForEventListener(listener, info.eventType, info.node, objectGroup));
        }
    }
}

void InspectorDOMAgent::getEventListeners(Node* node, Vector<EventListenerInfo>& eventInformation, bool includeAncestors)
{
    // The Node's Ancestors including self.
    Vector<Node*> ancestors;
    // Push this node as the firs element.
    ancestors.append(node);
    if (includeAncestors) {
        for (ContainerNode* ancestor = node->parentOrShadowHostNode(); ancestor; ancestor = ancestor->parentOrShadowHostNode())
            ancestors.append(ancestor);
    }

    // Nodes and their Listeners for the concerned event types (order is top to bottom)
    for (size_t i = ancestors.size(); i; --i) {
        Node* ancestor = ancestors[i - 1];
        EventTargetData* d = ancestor->eventTargetData();
        if (!d)
            continue;
        // Get the list of event types this Node is concerned with
        Vector<AtomicString> eventTypes = d->eventListenerMap.eventTypes();
        for (size_t j = 0; j < eventTypes.size(); ++j) {
            AtomicString& type = eventTypes[j];
            const EventListenerVector& listeners = ancestor->getEventListeners(type);
            EventListenerVector filteredListeners;
            filteredListeners.reserveCapacity(listeners.size());
            for (size_t k = 0; k < listeners.size(); ++k) {
                if (listeners[k].listener->type() == EventListener::JSEventListenerType)
                    filteredListeners.append(listeners[k]);
            }
            if (!filteredListeners.isEmpty())
                eventInformation.append(EventListenerInfo(ancestor, type, filteredListeners));
        }
    }
}

void InspectorDOMAgent::performSearch(ErrorString*, const String& whitespaceTrimmedQuery, String* searchId, int* resultCount)
{
    // FIXME: Few things are missing here:
    // 1) Search works with node granularity - number of matches within node is not calculated.
    // 2) There is no need to push all search results to the front-end at a time, pushing next / previous result
    //    is sufficient.

    unsigned queryLength = whitespaceTrimmedQuery.length();
    bool startTagFound = !whitespaceTrimmedQuery.find('<');
    bool endTagFound = whitespaceTrimmedQuery.reverseFind('>') + 1 == queryLength;
    bool startQuoteFound = !whitespaceTrimmedQuery.find('"');
    bool endQuoteFound = whitespaceTrimmedQuery.reverseFind('"') + 1 == queryLength;
    bool exactAttributeMatch = startQuoteFound && endQuoteFound;

    String tagNameQuery = whitespaceTrimmedQuery;
    String attributeQuery = whitespaceTrimmedQuery;
    if (startTagFound)
        tagNameQuery = tagNameQuery.right(tagNameQuery.length() - 1);
    if (endTagFound)
        tagNameQuery = tagNameQuery.left(tagNameQuery.length() - 1);
    if (startQuoteFound)
        attributeQuery = attributeQuery.right(attributeQuery.length() - 1);
    if (endQuoteFound)
        attributeQuery = attributeQuery.left(attributeQuery.length() - 1);

    Vector<Document*> docs = documents();
    ListHashSet<Node*> resultCollector;

    for (Vector<Document*>::iterator it = docs.begin(); it != docs.end(); ++it) {
        Document* document = *it;
        Node* node = document->documentElement();
        if (!node)
            continue;

        // Manual plain text search.
        while ((node = NodeTraversal::next(node, document->documentElement()))) {
            switch (node->nodeType()) {
            case Node::TEXT_NODE:
            case Node::COMMENT_NODE:
            case Node::CDATA_SECTION_NODE: {
                String text = node->nodeValue();
                if (text.findIgnoringCase(whitespaceTrimmedQuery) != notFound)
                    resultCollector.add(node);
                break;
            }
            case Node::ELEMENT_NODE: {
                if ((!startTagFound && !endTagFound && (node->nodeName().findIgnoringCase(tagNameQuery) != notFound))
                    || (startTagFound && endTagFound && equalIgnoringCase(node->nodeName(), tagNameQuery))
                    || (startTagFound && !endTagFound && node->nodeName().startsWith(tagNameQuery, false))
                    || (!startTagFound && endTagFound && node->nodeName().endsWith(tagNameQuery, false))) {
                    resultCollector.add(node);
                    break;
                }
                // Go through all attributes and serialize them.
                const Element* element = toElement(node);
                if (!element->hasAttributes())
                    break;

                unsigned numAttrs = element->attributeCount();
                for (unsigned i = 0; i < numAttrs; ++i) {
                    // Add attribute pair
                    const Attribute* attribute = element->attributeItem(i);
                    if (attribute->localName().find(whitespaceTrimmedQuery) != notFound) {
                        resultCollector.add(node);
                        break;
                    }
                    size_t foundPosition = attribute->value().find(attributeQuery);
                    if (foundPosition != notFound) {
                        if (!exactAttributeMatch || (!foundPosition && attribute->value().length() == attributeQuery.length())) {
                            resultCollector.add(node);
                            break;
                        }
                    }
                }
                break;
            }
            default:
                break;
            }
        }

        // XPath evaluation
        for (Vector<Document*>::iterator it = docs.begin(); it != docs.end(); ++it) {
            Document* document = *it;
            ExceptionCode ec = 0;
            RefPtr<XPathResult> result = document->evaluate(whitespaceTrimmedQuery, document, 0, XPathResult::ORDERED_NODE_SNAPSHOT_TYPE, 0, ec);
            if (ec || !result)
                continue;

            unsigned long size = result->snapshotLength(ec);
            for (unsigned long i = 0; !ec && i < size; ++i) {
                Node* node = result->snapshotItem(i, ec);
                if (ec)
                    break;

                if (node->nodeType() == Node::ATTRIBUTE_NODE)
                    node = static_cast<Attr*>(node)->ownerElement();
                resultCollector.add(node);
            }
        }

        // Selector evaluation
        for (Vector<Document*>::iterator it = docs.begin(); it != docs.end(); ++it) {
            Document* document = *it;
            ExceptionCode ec = 0;
            RefPtr<NodeList> nodeList = document->querySelectorAll(whitespaceTrimmedQuery, ec);
            if (ec || !nodeList)
                continue;

            unsigned size = nodeList->length();
            for (unsigned i = 0; i < size; ++i)
                resultCollector.add(nodeList->item(i));
        }
    }

    *searchId = IdentifiersFactory::createIdentifier();
    SearchResults::iterator resultsIt = m_searchResults.add(*searchId, Vector<RefPtr<Node> >()).iterator;

    for (ListHashSet<Node*>::iterator it = resultCollector.begin(); it != resultCollector.end(); ++it)
        resultsIt->value.append(*it);

    *resultCount = resultsIt->value.size();
}

void InspectorDOMAgent::getSearchResults(ErrorString* errorString, const String& searchId, int fromIndex, int toIndex, RefPtr<TypeBuilder::Array<int> >& nodeIds)
{
    SearchResults::iterator it = m_searchResults.find(searchId);
    if (it == m_searchResults.end()) {
        *errorString = "No search session with given id found";
        return;
    }

    int size = it->value.size();
    if (fromIndex < 0 || toIndex > size || fromIndex >= toIndex) {
        *errorString = "Invalid search result range";
        return;
    }

    nodeIds = TypeBuilder::Array<int>::create();
    for (int i = fromIndex; i < toIndex; ++i)
        nodeIds->addItem(pushNodePathToFrontend((it->value)[i].get()));
}

void InspectorDOMAgent::discardSearchResults(ErrorString*, const String& searchId)
{
    m_searchResults.remove(searchId);
}

bool InspectorDOMAgent::handleMousePress()
{
    if (!m_searchingForNode)
        return false;

    if (Node* node = m_overlay->highlightedNode()) {
        inspect(node);
        return true;
    }
    return false;
}

bool InspectorDOMAgent::handleTouchEvent(Node* node)
{
    if (!m_searchingForNode)
        return false;
    if (node && m_inspectModeHighlightConfig) {
        m_overlay->highlightNode(node, *m_inspectModeHighlightConfig);
        inspect(node);
        return true;
    }
    return false;
}

void InspectorDOMAgent::inspect(Node* inspectedNode)
{
    ErrorString error;
    RefPtr<Node> node = inspectedNode;
    setSearchingForNode(&error, false, 0);

    if (node->nodeType() != Node::ELEMENT_NODE && node->nodeType() != Node::DOCUMENT_NODE)
        node = node->parentNode();
    m_nodeToFocus = node;

    focusNode();
}

void InspectorDOMAgent::focusNode()
{
    if (!m_frontend)
        return;

    ASSERT(m_nodeToFocus);

    RefPtr<Node> node = m_nodeToFocus.get();
    m_nodeToFocus = 0;

    Document* document = node->ownerDocument();
    if (!document)
        return;
    Frame* frame = document->frame();
    if (!frame)
        return;

    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptFor(mainWorldScriptState(frame));
    if (injectedScript.hasNoValue())
        return;

    injectedScript.inspectNode(node.get());
}

void InspectorDOMAgent::mouseDidMoveOverElement(const HitTestResult& result, unsigned)
{
    if (!m_searchingForNode)
        return;

    Node* node = result.innerNode();
    while (node && node->nodeType() == Node::TEXT_NODE)
        node = node->parentNode();
    if (node && m_inspectModeHighlightConfig)
        m_overlay->highlightNode(node, *m_inspectModeHighlightConfig);
}

void InspectorDOMAgent::setSearchingForNode(ErrorString* errorString, bool enabled, InspectorObject* highlightInspectorObject)
{
    if (m_searchingForNode == enabled)
        return;
    m_searchingForNode = enabled;
    if (enabled) {
        m_inspectModeHighlightConfig = highlightConfigFromInspectorObject(errorString, highlightInspectorObject);
        if (!m_inspectModeHighlightConfig)
            return;
    } else
        hideHighlight(errorString);
}

PassOwnPtr<HighlightConfig> InspectorDOMAgent::highlightConfigFromInspectorObject(ErrorString* errorString, InspectorObject* highlightInspectorObject)
{
    if (!highlightInspectorObject) {
        *errorString = "Internal error: highlight configuration parameter is missing";
        return nullptr;
    }

    OwnPtr<HighlightConfig> highlightConfig = adoptPtr(new HighlightConfig());
    bool showInfo = false; // Default: false (do not show a tooltip).
    highlightInspectorObject->getBoolean("showInfo", &showInfo);
    highlightConfig->showInfo = showInfo;
    bool showRulers = false; // Default: false (do not show rulers).
    highlightInspectorObject->getBoolean("showRulers", &showRulers);
    highlightConfig->showRulers = showRulers;
    highlightConfig->content = parseConfigColor("contentColor", highlightInspectorObject);
    highlightConfig->contentOutline = parseConfigColor("contentOutlineColor", highlightInspectorObject);
    highlightConfig->padding = parseConfigColor("paddingColor", highlightInspectorObject);
    highlightConfig->border = parseConfigColor("borderColor", highlightInspectorObject);
    highlightConfig->margin = parseConfigColor("marginColor", highlightInspectorObject);
    return highlightConfig.release();
}

void InspectorDOMAgent::setInspectModeEnabled(ErrorString* errorString, bool enabled, const RefPtr<InspectorObject>* highlightConfig)
{
    setSearchingForNode(errorString, enabled, highlightConfig ? highlightConfig->get() : 0);
}

void InspectorDOMAgent::highlightRect(ErrorString*, int x, int y, int width, int height, const RefPtr<InspectorObject>* color, const RefPtr<InspectorObject>* outlineColor, const bool* usePageCoordinates)
{
    OwnPtr<FloatQuad> quad = adoptPtr(new FloatQuad(FloatRect(x, y, width, height)));
    innerHighlightQuad(quad.release(), color, outlineColor, usePageCoordinates);
}

void InspectorDOMAgent::highlightQuad(ErrorString* errorString, const RefPtr<InspectorArray>& quadArray, const RefPtr<InspectorObject>* color, const RefPtr<InspectorObject>* outlineColor, const bool* usePageCoordinates)
{
    OwnPtr<FloatQuad> quad = adoptPtr(new FloatQuad());
    if (!parseQuad(quadArray, quad.get())) {
        *errorString = "Invalid Quad format";
        return;
    }
    innerHighlightQuad(quad.release(), color, outlineColor, usePageCoordinates);
}

void InspectorDOMAgent::innerHighlightQuad(PassOwnPtr<FloatQuad> quad, const RefPtr<InspectorObject>* color, const RefPtr<InspectorObject>* outlineColor, const bool* usePageCoordinates)
{
    OwnPtr<HighlightConfig> highlightConfig = adoptPtr(new HighlightConfig());
    highlightConfig->content = parseColor(color);
    highlightConfig->contentOutline = parseColor(outlineColor);
    highlightConfig->usePageCoordinates = usePageCoordinates ? *usePageCoordinates : false;
    m_overlay->highlightQuad(quad, *highlightConfig);
}

void InspectorDOMAgent::highlightNode(ErrorString* errorString, const RefPtr<InspectorObject>& highlightInspectorObject, const int* nodeId, const String* objectId)
{
    Node* node = 0;
    if (nodeId) {
        node = assertNode(errorString, *nodeId);
    } else if (objectId) {
        InjectedScript injectedScript = m_injectedScriptManager->injectedScriptForObjectId(*objectId);
        node = injectedScript.nodeForObjectId(*objectId);
        if (!node)
            *errorString = "Node for given objectId not found";
    } else
        *errorString = "Either nodeId or objectId must be specified";

    if (!node)
        return;

    OwnPtr<HighlightConfig> highlightConfig = highlightConfigFromInspectorObject(errorString, highlightInspectorObject.get());
    if (!highlightConfig)
        return;

    m_overlay->highlightNode(node, *highlightConfig);
}

void InspectorDOMAgent::highlightFrame(
    ErrorString*,
    const String& frameId,
    const RefPtr<InspectorObject>* color,
    const RefPtr<InspectorObject>* outlineColor)
{
    Frame* frame = m_pageAgent->frameForId(frameId);
    if (frame && frame->ownerElement()) {
        OwnPtr<HighlightConfig> highlightConfig = adoptPtr(new HighlightConfig());
        highlightConfig->showInfo = true; // Always show tooltips for frames.
        highlightConfig->content = parseColor(color);
        highlightConfig->contentOutline = parseColor(outlineColor);
        m_overlay->highlightNode(frame->ownerElement(), *highlightConfig);
    }
}

void InspectorDOMAgent::hideHighlight(ErrorString*)
{
    m_overlay->hideHighlight();
}

void InspectorDOMAgent::moveTo(ErrorString* errorString, int nodeId, int targetElementId, const int* const anchorNodeId, int* newNodeId)
{
    Node* node = assertEditableNode(errorString, nodeId);
    if (!node)
        return;

    Element* targetElement = assertEditableElement(errorString, targetElementId);
    if (!targetElement)
        return;

    Node* anchorNode = 0;
    if (anchorNodeId && *anchorNodeId) {
        anchorNode = assertEditableNode(errorString, *anchorNodeId);
        if (!anchorNode)
            return;
        if (anchorNode->parentNode() != targetElement) {
            *errorString = "Anchor node must be child of the target element";
            return;
        }
    }

    if (!m_domEditor->insertBefore(targetElement, node, anchorNode, errorString))
        return;

    *newNodeId = pushNodePathToFrontend(node);
}

void InspectorDOMAgent::undo(ErrorString* errorString)
{
    ExceptionCode ec = 0;
    m_history->undo(ec);
    *errorString = InspectorDOMAgent::toErrorString(ec);
}

void InspectorDOMAgent::redo(ErrorString* errorString)
{
    ExceptionCode ec = 0;
    m_history->redo(ec);
    *errorString = InspectorDOMAgent::toErrorString(ec);
}

void InspectorDOMAgent::markUndoableState(ErrorString*)
{
    m_history->markUndoableState();
}

void InspectorDOMAgent::focus(ErrorString* errorString, int nodeId)
{
    Element* element = assertElement(errorString, nodeId);
    if (!element)
        return;
    if (!element->isFocusable()) {
        *errorString = "Element is not focusable";
        return;
    }
    element->focus();
}

void InspectorDOMAgent::setFileInputFiles(ErrorString* errorString, int nodeId, const RefPtr<InspectorArray>& files)
{
    if (!m_client->canSetFileInputFiles()) {
        *errorString = "Cannot set file input files";
        return;
    }

    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return;
    HTMLInputElement* element = node->toInputElement();
    if (!element || !element->isFileUpload()) {
        *errorString = "Node is not a file input element";
        return;
    }

    RefPtr<FileList> fileList = FileList::create();
    for (InspectorArray::const_iterator iter = files->begin(); iter != files->end(); ++iter) {
        String path;
        if (!(*iter)->asString(&path)) {
            *errorString = "Files must be strings";
            return;
        }
        fileList->append(File::create(path));
    }
    element->setFiles(fileList);
}

void InspectorDOMAgent::resolveNode(ErrorString* errorString, int nodeId, const String* const objectGroup, RefPtr<TypeBuilder::Runtime::RemoteObject>& result)
{
    String objectGroupName = objectGroup ? *objectGroup : "";
    Node* node = nodeForId(nodeId);
    if (!node) {
        *errorString = "No node with given id found";
        return;
    }
    RefPtr<TypeBuilder::Runtime::RemoteObject> object = resolveNode(node, objectGroupName);
    if (!object) {
        *errorString = "Node with given id does not belong to the document";
        return;
    }
    result = object;
}

void InspectorDOMAgent::getAttributes(ErrorString* errorString, int nodeId, RefPtr<TypeBuilder::Array<String> >& result)
{
    Element* element = assertElement(errorString, nodeId);
    if (!element)
        return;

    result = buildArrayForElementAttributes(element);
}

void InspectorDOMAgent::requestNode(ErrorString*, const String& objectId, int* nodeId)
{
    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptForObjectId(objectId);
    Node* node = injectedScript.nodeForObjectId(objectId);
    if (node)
        *nodeId = pushNodePathToFrontend(node);
    else
        *nodeId = 0;
}

// static
String InspectorDOMAgent::documentURLString(Document* document)
{
    if (!document || document->url().isNull())
        return "";
    return document->url().string();
}

static String documentBaseURLString(Document* document)
{
    return document->completeURL("").string();
}

PassRefPtr<TypeBuilder::DOM::Node> InspectorDOMAgent::buildObjectForNode(Node* node, int depth, NodeToIdMap* nodesMap)
{
    int id = bind(node, nodesMap);
    String nodeName;
    String localName;
    String nodeValue;

    switch (node->nodeType()) {
    case Node::TEXT_NODE:
    case Node::COMMENT_NODE:
    case Node::CDATA_SECTION_NODE:
        nodeValue = node->nodeValue();
        if (nodeValue.length() > maxTextSize) {
            nodeValue = nodeValue.left(maxTextSize);
            nodeValue.append(ellipsisUChar);
        }
        break;
    case Node::ATTRIBUTE_NODE:
        localName = node->localName();
        break;
    case Node::DOCUMENT_FRAGMENT_NODE:
    case Node::DOCUMENT_NODE:
    case Node::ELEMENT_NODE:
    default:
        nodeName = node->nodeName();
        localName = node->localName();
        break;
    }

    RefPtr<TypeBuilder::DOM::Node> value = TypeBuilder::DOM::Node::create()
        .setNodeId(id)
        .setNodeType(static_cast<int>(node->nodeType()))
        .setNodeName(nodeName)
        .setLocalName(localName)
        .setNodeValue(nodeValue);

    if (node->isContainerNode()) {
        int nodeCount = innerChildNodeCount(node);
        value->setChildNodeCount(nodeCount);
        RefPtr<TypeBuilder::Array<TypeBuilder::DOM::Node> > children = buildArrayForContainerChildren(node, depth, nodesMap);
        if (children->length() > 0)
            value->setChildren(children.release());
    }

    if (node->isElementNode()) {
        Element* element = toElement(node);
        value->setAttributes(buildArrayForElementAttributes(element));
        if (node->isFrameOwnerElement()) {
            HTMLFrameOwnerElement* frameOwner = static_cast<HTMLFrameOwnerElement*>(node);
            Frame* frame = frameOwner->contentFrame();
            if (frame)
                value->setFrameId(m_pageAgent->frameId(frame));
            Document* doc = frameOwner->contentDocument();
            if (doc)
                value->setContentDocument(buildObjectForNode(doc, 0, nodesMap));
        }

        ElementShadow* shadow = element->shadow();
        if (shadow) {
            RefPtr<TypeBuilder::Array<TypeBuilder::DOM::Node> > shadowRoots = TypeBuilder::Array<TypeBuilder::DOM::Node>::create();
            if (ShadowRoot* root = shadow->shadowRoot())
                shadowRoots->addItem(buildObjectForNode(root, 0, nodesMap));
            value->setShadowRoots(shadowRoots);
        }

#if ENABLE(TEMPLATE_ELEMENT)
        if (element->hasTagName(HTMLNames::templateTag))
            value->setTemplateContent(buildObjectForNode(static_cast<HTMLTemplateElement*>(element)->content(), 0, nodesMap));
#endif

    } else if (node->isDocumentNode()) {
        Document* document = toDocument(node);
        value->setDocumentURL(documentURLString(document));
        value->setBaseURL(documentBaseURLString(document));
        value->setXmlVersion(document->xmlVersion());
    } else if (node->nodeType() == Node::DOCUMENT_TYPE_NODE) {
        DocumentType* docType = static_cast<DocumentType*>(node);
        value->setPublicId(docType->publicId());
        value->setSystemId(docType->systemId());
        value->setInternalSubset(docType->internalSubset());
    } else if (node->isAttributeNode()) {
        Attr* attribute = static_cast<Attr*>(node);
        value->setName(attribute->name());
        value->setValue(attribute->value());
    }
    return value.release();
}

PassRefPtr<TypeBuilder::Array<String> > InspectorDOMAgent::buildArrayForElementAttributes(Element* element)
{
    RefPtr<TypeBuilder::Array<String> > attributesValue = TypeBuilder::Array<String>::create();
    // Go through all attributes and serialize them.
    if (!element->hasAttributes())
        return attributesValue.release();
    unsigned numAttrs = element->attributeCount();
    for (unsigned i = 0; i < numAttrs; ++i) {
        // Add attribute pair
        const Attribute* attribute = element->attributeItem(i);
        attributesValue->addItem(attribute->name().toString());
        attributesValue->addItem(attribute->value());
    }
    return attributesValue.release();
}

PassRefPtr<TypeBuilder::Array<TypeBuilder::DOM::Node> > InspectorDOMAgent::buildArrayForContainerChildren(Node* container, int depth, NodeToIdMap* nodesMap)
{
    RefPtr<TypeBuilder::Array<TypeBuilder::DOM::Node> > children = TypeBuilder::Array<TypeBuilder::DOM::Node>::create();
    if (depth == 0) {
        // Special-case the only text child - pretend that container's children have been requested.
        Node* firstChild = container->firstChild();
        if (firstChild && firstChild->nodeType() == Node::TEXT_NODE && !firstChild->nextSibling()) {
            children->addItem(buildObjectForNode(firstChild, 0, nodesMap));
            m_childrenRequested.add(bind(container, nodesMap));
        }
        return children.release();
    }

    Node* child = innerFirstChild(container);
    depth--;
    m_childrenRequested.add(bind(container, nodesMap));

    while (child) {
        children->addItem(buildObjectForNode(child, depth, nodesMap));
        child = innerNextSibling(child);
    }
    return children.release();
}

PassRefPtr<TypeBuilder::DOM::EventListener> InspectorDOMAgent::buildObjectForEventListener(const RegisteredEventListener& registeredEventListener, const AtomicString& eventType, Node* node, const String* objectGroupId)
{
    RefPtr<EventListener> eventListener = registeredEventListener.listener;
    Document* document = node->document();
    RefPtr<TypeBuilder::DOM::EventListener> value = TypeBuilder::DOM::EventListener::create()
        .setType(eventType)
        .setUseCapture(registeredEventListener.useCapture)
        .setIsAttribute(eventListener->isAttribute())
        .setNodeId(pushNodePathToFrontend(node))
        .setHandlerBody(eventListenerHandlerBody(document, eventListener.get()));
    if (objectGroupId) {
        ScriptValue functionValue = eventListenerHandler(document, eventListener.get());
        if (!functionValue.hasNoValue()) {
            Frame* frame = document->frame();
            if (frame) {
                ScriptState* scriptState = eventListenerHandlerScriptState(frame, eventListener.get());
                if (scriptState) {
                    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptFor(scriptState);
                    if (!injectedScript.hasNoValue()) {
                        RefPtr<TypeBuilder::Runtime::RemoteObject> valueJson = injectedScript.wrapObject(functionValue, *objectGroupId);
                        value->setHandler(valueJson);
                    }
                }
            }
        }
    }
    String sourceName;
    String scriptId;
    int lineNumber;
    if (eventListenerHandlerLocation(node->document(), eventListener.get(), sourceName, scriptId, lineNumber)) {
        RefPtr<TypeBuilder::Debugger::Location> location = TypeBuilder::Debugger::Location::create()
            .setScriptId(scriptId)
            .setLineNumber(lineNumber);
        value->setLocation(location);
        if (!sourceName.isEmpty())
            value->setSourceName(sourceName);
    }
    return value.release();
}

Node* InspectorDOMAgent::innerFirstChild(Node* node)
{
    node = node->firstChild();
    while (isWhitespace(node))
        node = node->nextSibling();
    return node;
}

Node* InspectorDOMAgent::innerNextSibling(Node* node)
{
    do {
        node = node->nextSibling();
    } while (isWhitespace(node));
    return node;
}

Node* InspectorDOMAgent::innerPreviousSibling(Node* node)
{
    do {
        node = node->previousSibling();
    } while (isWhitespace(node));
    return node;
}

unsigned InspectorDOMAgent::innerChildNodeCount(Node* node)
{
    unsigned count = 0;
    Node* child = innerFirstChild(node);
    while (child) {
        count++;
        child = innerNextSibling(child);
    }
    return count;
}

Node* InspectorDOMAgent::innerParentNode(Node* node)
{
    if (node->isDocumentNode()) {
        Document* document = toDocument(node);
        return document->ownerElement();
    }
    return node->parentNode();
}

bool InspectorDOMAgent::isWhitespace(Node* node)
{
    //TODO: pull ignoreWhitespace setting from the frontend and use here.
    return node && node->nodeType() == Node::TEXT_NODE && node->nodeValue().stripWhiteSpace().length() == 0;
}

void InspectorDOMAgent::mainFrameDOMContentLoaded()
{
    // Re-push document once it is loaded.
    discardBindings();
    if (m_state->getBoolean(DOMAgentState::documentRequested))
        m_frontend->documentUpdated();
}

void InspectorDOMAgent::loadEventFired(Document* document)
{
    Element* frameOwner = document->ownerElement();
    if (!frameOwner)
        return;

    int frameOwnerId = m_documentNodeToIdMap.get(frameOwner);
    if (!frameOwnerId)
        return;

    // Re-add frame owner element together with its new children.
    int parentId = m_documentNodeToIdMap.get(innerParentNode(frameOwner));
    m_frontend->childNodeRemoved(parentId, frameOwnerId);
    unbind(frameOwner, &m_documentNodeToIdMap);

    RefPtr<TypeBuilder::DOM::Node> value = buildObjectForNode(frameOwner, 0, &m_documentNodeToIdMap);
    Node* previousSibling = innerPreviousSibling(frameOwner);
    int prevId = previousSibling ? m_documentNodeToIdMap.get(previousSibling) : 0;
    m_frontend->childNodeInserted(parentId, prevId, value.release());
}

void InspectorDOMAgent::didInsertDOMNode(Node* node)
{
    if (isWhitespace(node))
        return;

    // We could be attaching existing subtree. Forget the bindings.
    unbind(node, &m_documentNodeToIdMap);

    ContainerNode* parent = node->parentNode();
    if (!parent)
        return;

    int parentId = m_documentNodeToIdMap.get(parent);
    // Return if parent is not mapped yet.
    if (!parentId)
        return;

    if (!m_childrenRequested.contains(parentId)) {
        // No children are mapped yet -> only notify on changes of hasChildren.
        m_frontend->childNodeCountUpdated(parentId, innerChildNodeCount(parent));
    } else {
        // Children have been requested -> return value of a new child.
        Node* prevSibling = innerPreviousSibling(node);
        int prevId = prevSibling ? m_documentNodeToIdMap.get(prevSibling) : 0;
        RefPtr<TypeBuilder::DOM::Node> value = buildObjectForNode(node, 0, &m_documentNodeToIdMap);
        m_frontend->childNodeInserted(parentId, prevId, value.release());
    }
}

void InspectorDOMAgent::didRemoveDOMNode(Node* node)
{
    if (isWhitespace(node))
        return;

    ContainerNode* parent = node->parentNode();

    // If parent is not mapped yet -> ignore the event.
    if (!m_documentNodeToIdMap.contains(parent))
        return;

    int parentId = m_documentNodeToIdMap.get(parent);

    if (!m_childrenRequested.contains(parentId)) {
        // No children are mapped yet -> only notify on changes of hasChildren.
        if (innerChildNodeCount(parent) == 1)
            m_frontend->childNodeCountUpdated(parentId, 0);
    } else
        m_frontend->childNodeRemoved(parentId, m_documentNodeToIdMap.get(node));
    unbind(node, &m_documentNodeToIdMap);
}

void InspectorDOMAgent::willModifyDOMAttr(Element*, const AtomicString& oldValue, const AtomicString& newValue)
{
    m_suppressAttributeModifiedEvent = (oldValue == newValue);
}

void InspectorDOMAgent::didModifyDOMAttr(Element* element, const AtomicString& name, const AtomicString& value)
{
    bool shouldSuppressEvent = m_suppressAttributeModifiedEvent;
    m_suppressAttributeModifiedEvent = false;
    if (shouldSuppressEvent)
        return;

    int id = boundNodeId(element);
    // If node is not mapped yet -> ignore the event.
    if (!id)
        return;

    if (m_domListener)
        m_domListener->didModifyDOMAttr(element);

    m_frontend->attributeModified(id, name, value);
}

void InspectorDOMAgent::didRemoveDOMAttr(Element* element, const AtomicString& name)
{
    int id = boundNodeId(element);
    // If node is not mapped yet -> ignore the event.
    if (!id)
        return;

    if (m_domListener)
        m_domListener->didModifyDOMAttr(element);

    m_frontend->attributeRemoved(id, name);
}

void InspectorDOMAgent::styleAttributeInvalidated(const Vector<Element*>& elements)
{
    RefPtr<TypeBuilder::Array<int> > nodeIds = TypeBuilder::Array<int>::create();
    for (unsigned i = 0, size = elements.size(); i < size; ++i) {
        Element* element = elements.at(i);
        int id = boundNodeId(element);
        // If node is not mapped yet -> ignore the event.
        if (!id)
            continue;

        if (m_domListener)
            m_domListener->didModifyDOMAttr(element);
        nodeIds->addItem(id);
    }
    m_frontend->inlineStyleInvalidated(nodeIds.release());
}

void InspectorDOMAgent::characterDataModified(CharacterData* characterData)
{
    int id = m_documentNodeToIdMap.get(characterData);
    if (!id) {
        // Push text node if it is being created.
        didInsertDOMNode(characterData);
        return;
    }
    m_frontend->characterDataModified(id, characterData->data());
}

void InspectorDOMAgent::didInvalidateStyleAttr(Node* node)
{
    int id = m_documentNodeToIdMap.get(node);
    // If node is not mapped yet -> ignore the event.
    if (!id)
        return;

    if (!m_revalidateStyleAttrTask)
        m_revalidateStyleAttrTask = adoptPtr(new RevalidateStyleAttributeTask(this));
    m_revalidateStyleAttrTask->scheduleFor(toElement(node));
}

void InspectorDOMAgent::didPushShadowRoot(Element* host, ShadowRoot* root)
{
    int hostId = m_documentNodeToIdMap.get(host);
    if (hostId)
        m_frontend->shadowRootPushed(hostId, buildObjectForNode(root, 0, &m_documentNodeToIdMap));
}

void InspectorDOMAgent::willPopShadowRoot(Element* host, ShadowRoot* root)
{
    int hostId = m_documentNodeToIdMap.get(host);
    int rootId = m_documentNodeToIdMap.get(root);
    if (hostId && rootId)
        m_frontend->shadowRootPopped(hostId, rootId);
}

void InspectorDOMAgent::frameDocumentUpdated(Frame* frame)
{
    Document* document = frame->document();
    if (!document)
        return;

    Page* page = frame->page();
    ASSERT(page);
    if (frame != page->mainFrame())
        return;

    // Only update the main frame document, nested frame document updates are not required
    // (will be handled by loadEventFired()).
    setDocument(document);
}

Node* InspectorDOMAgent::nodeForPath(const String& path)
{
    // The path is of form "1,HTML,2,BODY,1,DIV"
    if (!m_document)
        return 0;

    Node* node = m_document.get();
    Vector<String> pathTokens;
    path.split(",", false, pathTokens);
    if (!pathTokens.size())
        return 0;
    for (size_t i = 0; i < pathTokens.size() - 1; i += 2) {
        bool success = true;
        unsigned childNumber = pathTokens[i].toUInt(&success);
        if (!success)
            return 0;
        if (childNumber >= innerChildNodeCount(node))
            return 0;

        Node* child = innerFirstChild(node);
        String childName = pathTokens[i + 1];
        for (size_t j = 0; child && j < childNumber; ++j)
            child = innerNextSibling(child);

        if (!child || child->nodeName() != childName)
            return 0;
        node = child;
    }
    return node;
}

void InspectorDOMAgent::pushNodeByPathToFrontend(ErrorString* errorString, const String& path, int* nodeId)
{
    if (Node* node = nodeForPath(path))
        *nodeId = pushNodePathToFrontend(node);
    else
        *errorString = "No node with given path found";
}

void InspectorDOMAgent::pushNodeByBackendIdToFrontend(ErrorString* errorString, BackendNodeId backendNodeId, int* nodeId)
{
    if (!m_backendIdToNode.contains(backendNodeId)) {
        *errorString = "No node with given backend id found";
        return;
    }

    Node* node = m_backendIdToNode.get(backendNodeId).first;
    String nodeGroup = m_backendIdToNode.get(backendNodeId).second;
    *nodeId = pushNodePathToFrontend(node);

    if (nodeGroup == "") {
        m_backendIdToNode.remove(backendNodeId);
        m_nodeGroupToBackendIdMap.find(nodeGroup)->value.remove(node);
    }
}

PassRefPtr<TypeBuilder::Runtime::RemoteObject> InspectorDOMAgent::resolveNode(Node* node, const String& objectGroup)
{
    Document* document = node->isDocumentNode() ? node->document() : node->ownerDocument();
    Frame* frame = document ? document->frame() : 0;
    if (!frame)
        return 0;

    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptFor(mainWorldScriptState(frame));
    if (injectedScript.hasNoValue())
        return 0;

    return injectedScript.wrapNode(node, objectGroup);
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
