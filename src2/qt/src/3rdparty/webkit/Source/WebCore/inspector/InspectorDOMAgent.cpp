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
#include "InspectorDOMAgent.h"

#if ENABLE(INSPECTOR)

#include "Attr.h"
#include "CSSComputedStyleDeclaration.h"
#include "CSSMutableStyleDeclaration.h"
#include "CSSPropertyNames.h"
#include "CSSPropertySourceData.h"
#include "CSSRule.h"
#include "CSSRuleList.h"
#include "CSSStyleRule.h"
#include "CSSStyleSelector.h"
#include "CSSStyleSheet.h"
#include "CharacterData.h"
#include "ContainerNode.h"
#include "Cookie.h"
#include "CookieJar.h"
#include "DOMNodeHighlighter.h"
#include "DOMWindow.h"
#include "Document.h"
#include "DocumentType.h"
#include "Event.h"
#include "EventContext.h"
#include "EventListener.h"
#include "EventNames.h"
#include "EventTarget.h"
#include "Frame.h"
#include "FrameTree.h"
#include "HitTestResult.h"
#include "HTMLElement.h"
#include "HTMLFrameOwnerElement.h"
#include "InjectedScriptManager.h"
#include "InspectorClient.h"
#include "InspectorFrontend.h"
#include "InspectorPageAgent.h"
#include "InspectorState.h"
#include "InstrumentingAgents.h"
#include "MutationEvent.h"
#include "Node.h"
#include "NodeList.h"
#include "Page.h"
#include "Pasteboard.h"
#include "PlatformString.h"
#include "RenderStyle.h"
#include "RenderStyleConstants.h"
#include "ScriptEventListener.h"
#include "ShadowRoot.h"
#include "StyleSheetList.h"
#include "Text.h"

#if ENABLE(XPATH)
#include "XPathResult.h"
#endif

#include "markup.h"

#include <wtf/text/CString.h>
#include <wtf/text/StringConcatenate.h>
#include <wtf/HashSet.h>
#include <wtf/ListHashSet.h>
#include <wtf/OwnPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

namespace DOMAgentState {
static const char documentRequested[] = "documentRequested";
};

class MatchJob {
public:
    virtual void match(ListHashSet<Node*>& resultCollector) = 0;
    virtual ~MatchJob() { }

protected:
    MatchJob(Document* document, const String& query)
        : m_document(document)
        , m_query(query) { }

    void addNodesToResults(PassRefPtr<NodeList> nodes, ListHashSet<Node*>& resultCollector)
    {
        for (unsigned i = 0; nodes && i < nodes->length(); ++i)
            resultCollector.add(nodes->item(i));
    }

    RefPtr<Document> m_document;
    String m_query;
};

class RevalidateStyleAttributeTask {
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

namespace {

class MatchExactIdJob : public WebCore::MatchJob {
public:
    MatchExactIdJob(Document* document, const String& query) : WebCore::MatchJob(document, query) { }
    virtual ~MatchExactIdJob() { }

protected:
    virtual void match(ListHashSet<Node*>& resultCollector)
    {
        if (m_query.isEmpty())
            return;

        Element* element = m_document->getElementById(m_query);
        if (element)
            resultCollector.add(element);
    }
};

class MatchExactClassNamesJob : public WebCore::MatchJob {
public:
    MatchExactClassNamesJob(Document* document, const String& query) : WebCore::MatchJob(document, query) { }
    virtual ~MatchExactClassNamesJob() { }

    virtual void match(ListHashSet<Node*>& resultCollector)
    {
        if (!m_query.isEmpty())
            addNodesToResults(m_document->getElementsByClassName(m_query), resultCollector);
    }
};

class MatchExactTagNamesJob : public WebCore::MatchJob {
public:
    MatchExactTagNamesJob(Document* document, const String& query) : WebCore::MatchJob(document, query) { }
    virtual ~MatchExactTagNamesJob() { }

    virtual void match(ListHashSet<Node*>& resultCollector)
    {
        if (!m_query.isEmpty())
            addNodesToResults(m_document->getElementsByName(m_query), resultCollector);
    }
};

class MatchQuerySelectorAllJob : public WebCore::MatchJob {
public:
    MatchQuerySelectorAllJob(Document* document, const String& query) : WebCore::MatchJob(document, query) { }
    virtual ~MatchQuerySelectorAllJob() { }

    virtual void match(ListHashSet<Node*>& resultCollector)
    {
        if (m_query.isEmpty())
            return;

        ExceptionCode ec = 0;
        RefPtr<NodeList> list = m_document->querySelectorAll(m_query, ec);
        if (!ec)
            addNodesToResults(list, resultCollector);
    }
};

class MatchXPathJob : public WebCore::MatchJob {
public:
    MatchXPathJob(Document* document, const String& query) : WebCore::MatchJob(document, query) { }
    virtual ~MatchXPathJob() { }

    virtual void match(ListHashSet<Node*>& resultCollector)
    {
#if ENABLE(XPATH)
        if (m_query.isEmpty())
            return;

        ExceptionCode ec = 0;
        RefPtr<XPathResult> result = m_document->evaluate(m_query, m_document.get(), 0, XPathResult::ORDERED_NODE_SNAPSHOT_TYPE, 0, ec);
        if (ec || !result)
            return;

        unsigned long size = result->snapshotLength(ec);
        for (unsigned long i = 0; !ec && i < size; ++i) {
            Node* node = result->snapshotItem(i, ec);
            if (ec)
                break;

            if (node->nodeType() == Node::ATTRIBUTE_NODE)
                node = static_cast<Attr*>(node)->ownerElement();
            resultCollector.add(node);
        }
#else
        UNUSED_PARAM(resultCollector);
#endif
    }
};

class MatchPlainTextJob : public MatchXPathJob {
public:
    MatchPlainTextJob(Document* document, const String& query) : MatchXPathJob(document, query)
    {
        m_query = "//text()[contains(., '" + m_query + "')] | //comment()[contains(., '" + m_query + "')]";
    }
    virtual ~MatchPlainTextJob() { }
};

}

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
    for (HashSet<RefPtr<Element> >::iterator it = m_elements.begin(), end = m_elements.end(); it != end; ++it)
        m_domAgent->didModifyDOMAttr(it->get());

    m_elements.clear();
}

InspectorDOMAgent::InspectorDOMAgent(InstrumentingAgents* instrumentingAgents, InspectorPageAgent* pageAgent, InspectorClient* client, InspectorState* inspectorState, InjectedScriptManager* injectedScriptManager)
    : m_instrumentingAgents(instrumentingAgents)
    , m_pageAgent(pageAgent)
    , m_client(client)
    , m_inspectorState(inspectorState)
    , m_injectedScriptManager(injectedScriptManager)
    , m_frontend(0)
    , m_domListener(0)
    , m_lastNodeId(1)
    , m_matchJobsTimer(this, &InspectorDOMAgent::onMatchJobsTimer)
    , m_searchingForNode(false)
{
}

InspectorDOMAgent::~InspectorDOMAgent()
{
    reset();
    ASSERT(!m_highlightedNode);
    ASSERT(!m_searchingForNode);
}

void InspectorDOMAgent::setFrontend(InspectorFrontend* frontend)
{
    ASSERT(!m_frontend);
    m_frontend = frontend->dom();
    m_instrumentingAgents->setInspectorDOMAgent(this);
    m_document = m_pageAgent->mainFrame()->document();

    if (m_nodeToFocus)
        focusNode();
}

void InspectorDOMAgent::clearFrontend()
{
    ASSERT(m_frontend);
    setSearchingForNode(false);

    ErrorString error;
    hideHighlight(&error);

    m_frontend = 0;
    m_instrumentingAgents->setInspectorDOMAgent(0);
    m_inspectorState->setBoolean(DOMAgentState::documentRequested, false);
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
    ErrorString error;
    cancelSearch(&error);
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

    if (!m_inspectorState->getBoolean(DOMAgentState::documentRequested))
        return;

    // Immediately communicate 0 document or document that has finished loading.
    if (!doc || !doc->parsing())
        m_frontend->documentUpdated();
}

void InspectorDOMAgent::releaseDanglingNodes()
{
    deleteAllValues(m_danglingNodeToIdMaps);
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
    if (node->isFrameOwnerElement()) {
        const HTMLFrameOwnerElement* frameOwner = static_cast<const HTMLFrameOwnerElement*>(node);
        if (m_domListener)
            m_domListener->didRemoveDocument(frameOwner->contentDocument());
    }

    int id = nodesMap->get(node);
    if (!id)
        return;
    m_idToNode.remove(id);
    nodesMap->remove(node);
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


HTMLElement* InspectorDOMAgent::assertHTMLElement(ErrorString* errorString, int nodeId)
{
    Element* element = assertElement(errorString, nodeId);
    if (!element)
        return 0;

    if (!element->isHTMLElement()) {
        *errorString = "Node is not an HTML Element";
        return 0;
    }
    return toHTMLElement(element);
}

void InspectorDOMAgent::getDocument(ErrorString*, RefPtr<InspectorObject>* root)
{
    m_inspectorState->setBoolean(DOMAgentState::documentRequested, true);

    if (!m_document)
        return;

    // Reset backend state.
    RefPtr<Document> doc = m_document;
    reset();
    m_document = doc;

    *root = buildObjectForNode(m_document.get(), 2, &m_documentNodeToIdMap);
}

void InspectorDOMAgent::pushChildNodesToFrontend(int nodeId)
{
    Node* node = nodeForId(nodeId);
    if (!node || !isContainerNode(*node))
        return;
    if (m_childrenRequested.contains(nodeId))
        return;

    NodeToIdMap* nodeMap = m_idToNodesMap.get(nodeId);
    RefPtr<InspectorArray> children = buildArrayForContainerChildren(node, 1, nodeMap);
    m_frontend->setChildNodes(nodeId, children.release());
}

void InspectorDOMAgent::discardBindings()
{
    m_documentNodeToIdMap.clear();
    m_idToNode.clear();
    releaseDanglingNodes();
    m_childrenRequested.clear();
}

Node* InspectorDOMAgent::nodeForId(int id)
{
    if (!id)
        return 0;

    HashMap<int, Node*>::iterator it = m_idToNode.find(id);
    if (it != m_idToNode.end())
        return it->second;
    return 0;
}

void InspectorDOMAgent::getChildNodes(ErrorString*, int nodeId)
{
    pushChildNodesToFrontend(nodeId);
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

void InspectorDOMAgent::querySelectorAll(ErrorString* errorString, int nodeId, const String& selectors, RefPtr<InspectorArray>* result)
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

    for (unsigned i = 0; i < nodes->length(); ++i)
        (*result)->pushNumber(pushNodePathToFrontend(nodes->item(i)));
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
            danglingMap = new NodeToIdMap();
            m_danglingNodeToIdMaps.append(danglingMap);
            RefPtr<InspectorArray> children = InspectorArray::create();
            children->pushObject(buildObjectForNode(node, 0, danglingMap));
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

void InspectorDOMAgent::setAttribute(ErrorString* errorString, int elementId, const String& name, const String& value)
{
    Element* element = assertElement(errorString, elementId);
    if (element) {
        ExceptionCode ec = 0;
        element->setAttribute(name, value, ec);
        if (ec)
            *errorString = "Exception while setting attribute value";
    }
}

void InspectorDOMAgent::removeAttribute(ErrorString* errorString, int elementId, const String& name)
{
    Element* element = assertElement(errorString, elementId);
    if (element) {
        ExceptionCode ec = 0;
        element->removeAttribute(name, ec);
        if (ec)
            *errorString = "Exception while removing attribute";
    }
}

void InspectorDOMAgent::removeNode(ErrorString* errorString, int nodeId)
{
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return;

    ContainerNode* parentNode = node->parentNode();
    if (!parentNode) {
        *errorString = "Can not remove detached node";
        return;
    }

    ExceptionCode ec = 0;
    parentNode->removeChild(node, ec);
    if (ec)
        *errorString = "Could not remove node due to DOM exception";
}

void InspectorDOMAgent::setNodeName(ErrorString*, int nodeId, const String& tagName, int* newId)
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
    Element* oldElem = static_cast<Element*>(oldNode);
    newElem->copyNonAttributeProperties(oldElem);
    if (oldElem->attributes())
        newElem->attributes()->setAttributes(*(oldElem->attributes(true)));

    // Copy over the original node's children.
    Node* child;
    while ((child = oldNode->firstChild()))
        newElem->appendChild(child, ec);

    // Replace the old node with the new node
    ContainerNode* parent = oldNode->parentNode();
    parent->insertBefore(newElem, oldNode->nextSibling(), ec);
    parent->removeChild(oldNode, ec);

    if (ec)
        return;

    *newId = pushNodePathToFrontend(newElem.get());
    if (m_childrenRequested.contains(nodeId))
        pushChildNodesToFrontend(*newId);
}

void InspectorDOMAgent::getOuterHTML(ErrorString* errorString, int nodeId, WTF::String* outerHTML)
{
    HTMLElement* element = assertHTMLElement(errorString, nodeId);
    if (element)
        *outerHTML = element->outerHTML();
}

void InspectorDOMAgent::setOuterHTML(ErrorString* errorString, int nodeId, const String& outerHTML, int* newId)
{
    HTMLElement* htmlElement = assertHTMLElement(errorString, nodeId);
    if (!htmlElement)
        return;

    bool requiresTotalUpdate = htmlElement->tagName() == "HTML" || htmlElement->tagName() == "BODY" || htmlElement->tagName() == "HEAD";

    bool childrenRequested = m_childrenRequested.contains(nodeId);
    Node* previousSibling = htmlElement->previousSibling();
    ContainerNode* parentNode = htmlElement->parentNode();

    ExceptionCode ec = 0;
    htmlElement->setOuterHTML(outerHTML, ec);
    if (ec)
        return;

    if (requiresTotalUpdate) {
        RefPtr<Document> document = m_document;
        reset();
        setDocument(document.get());
        *newId = 0;
        return;
    }

    Node* newNode = previousSibling ? previousSibling->nextSibling() : parentNode->firstChild();
    if (!newNode) {
        // The only child node has been deleted.
        *newId = 0;
        return;
    }

    *newId = pushNodePathToFrontend(newNode);
    if (childrenRequested)
        pushChildNodesToFrontend(*newId);
}

void InspectorDOMAgent::setNodeValue(ErrorString* errorString, int nodeId, const String& value)
{
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return;

    if (node->nodeType() != Node::TEXT_NODE) {
        *errorString = "Can only set value of text nodes";
        return;
    }

    Text* textNode = static_cast<Text*>(node);
    ExceptionCode ec = 0;
    textNode->replaceWholeText(value, ec);
    if (ec)
        *errorString = "DOM Error while setting the node value";
}

void InspectorDOMAgent::getEventListenersForNode(ErrorString*, int nodeId, RefPtr<InspectorArray>* listenersArray)
{
    Node* node = nodeForId(nodeId);
    EventTargetData* d;

    // Quick break if a null node or no listeners at all
    if (!node || !(d = node->eventTargetData()))
        return;

    // Get the list of event types this Node is concerned with
    Vector<AtomicString> eventTypes;
    const EventListenerMap& listenerMap = d->eventListenerMap;
    EventListenerMap::const_iterator end = listenerMap.end();
    for (EventListenerMap::const_iterator iter = listenerMap.begin(); iter != end; ++iter)
        eventTypes.append(iter->first);

    // Quick break if no useful listeners
    size_t eventTypesLength = eventTypes.size();
    if (!eventTypesLength)
        return;

    // The Node's Ancestors (not including self)
    Vector<ContainerNode*> ancestors;
    for (ContainerNode* ancestor = node->parentOrHostNode(); ancestor; ancestor = ancestor->parentOrHostNode())
        ancestors.append(ancestor);

    // Nodes and their Listeners for the concerned event types (order is top to bottom)
    Vector<EventListenerInfo> eventInformation;
    for (size_t i = ancestors.size(); i; --i) {
        ContainerNode* ancestor = ancestors[i - 1];
        for (size_t j = 0; j < eventTypesLength; ++j) {
            AtomicString& type = eventTypes[j];
            if (ancestor->hasEventListeners(type))
                eventInformation.append(EventListenerInfo(ancestor, type, ancestor->getEventListeners(type)));
        }
    }

    // Insert the Current Node at the end of that list (last in capturing, first in bubbling)
    for (size_t i = 0; i < eventTypesLength; ++i) {
        const AtomicString& type = eventTypes[i];
        eventInformation.append(EventListenerInfo(node, type, node->getEventListeners(type)));
    }

    // Get Capturing Listeners (in this order)
    size_t eventInformationLength = eventInformation.size();
    for (size_t i = 0; i < eventInformationLength; ++i) {
        const EventListenerInfo& info = eventInformation[i];
        const EventListenerVector& vector = info.eventListenerVector;
        for (size_t j = 0; j < vector.size(); ++j) {
            const RegisteredEventListener& listener = vector[j];
            if (listener.useCapture)
                (*listenersArray)->pushObject(buildObjectForEventListener(listener, info.eventType, info.node));
        }
    }

    // Get Bubbling Listeners (reverse order)
    for (size_t i = eventInformationLength; i; --i) {
        const EventListenerInfo& info = eventInformation[i - 1];
        const EventListenerVector& vector = info.eventListenerVector;
        for (size_t j = 0; j < vector.size(); ++j) {
            const RegisteredEventListener& listener = vector[j];
            if (!listener.useCapture)
                (*listenersArray)->pushObject(buildObjectForEventListener(listener, info.eventType, info.node));
        }
    }
}

void InspectorDOMAgent::performSearch(ErrorString* error, const String& whitespaceTrimmedQuery, const bool* const runSynchronously)
{
    // FIXME: Few things are missing here:
    // 1) Search works with node granularity - number of matches within node is not calculated.
    // 2) There is no need to push all search results to the front-end at a time, pushing next / previous result
    //    is sufficient.

    unsigned queryLength = whitespaceTrimmedQuery.length();
    bool startTagFound = !whitespaceTrimmedQuery.find('<');
    bool endTagFound = whitespaceTrimmedQuery.reverseFind('>') + 1 == queryLength;

    String tagNameQuery = whitespaceTrimmedQuery;
    if (startTagFound || endTagFound)
        tagNameQuery = tagNameQuery.substring(startTagFound ? 1 : 0, endTagFound ? queryLength - 1 : queryLength);
    if (!Document::isValidName(tagNameQuery))
        tagNameQuery = "";

    String attributeNameQuery = whitespaceTrimmedQuery;
    if (!Document::isValidName(attributeNameQuery))
        attributeNameQuery = "";

    String escapedQuery = whitespaceTrimmedQuery;
    escapedQuery.replace("'", "\\'");
    String escapedTagNameQuery = tagNameQuery;
    escapedTagNameQuery.replace("'", "\\'");

    // Clear pending jobs.
    cancelSearch(error);

    // Find all frames, iframes and object elements to search their documents.
    Vector<Document*> docs = documents();
    for (Vector<Document*>::iterator it = docs.begin(); it != docs.end(); ++it) {
        Document* document = *it;

        if (!tagNameQuery.isEmpty() && startTagFound && endTagFound) {
            m_pendingMatchJobs.append(new MatchExactTagNamesJob(document, tagNameQuery));
            m_pendingMatchJobs.append(new MatchPlainTextJob(document, escapedQuery));
            continue;
        }

        if (!tagNameQuery.isEmpty() && startTagFound) {
            m_pendingMatchJobs.append(new MatchXPathJob(document, "//*[starts-with(name(), '" + escapedTagNameQuery + "')]"));
            m_pendingMatchJobs.append(new MatchPlainTextJob(document, escapedQuery));
            continue;
        }

        if (!tagNameQuery.isEmpty() && endTagFound) {
            // FIXME: we should have a matchEndOfTagNames search function if endTagFound is true but not startTagFound.
            // This requires ends-with() support in XPath, WebKit only supports starts-with() and contains().
            m_pendingMatchJobs.append(new MatchXPathJob(document, "//*[contains(name(), '" + escapedTagNameQuery + "')]"));
            m_pendingMatchJobs.append(new MatchPlainTextJob(document, escapedQuery));
            continue;
        }

        bool matchesEveryNode = whitespaceTrimmedQuery == "//*" || whitespaceTrimmedQuery == "*";
        if (matchesEveryNode) {
            // These queries will match every node. Matching everything isn't useful and can be slow for large pages,
            // so limit the search functions list to plain text and attribute matching for these.
            m_pendingMatchJobs.append(new MatchXPathJob(document, "//*[contains(@*, '" + escapedQuery + "')]"));
            m_pendingMatchJobs.append(new MatchPlainTextJob(document, escapedQuery));
            continue;
        }

        m_pendingMatchJobs.append(new MatchExactIdJob(document, whitespaceTrimmedQuery));
        m_pendingMatchJobs.append(new MatchExactClassNamesJob(document, whitespaceTrimmedQuery));
        m_pendingMatchJobs.append(new MatchExactTagNamesJob(document, tagNameQuery));
        m_pendingMatchJobs.append(new MatchQuerySelectorAllJob(document, "[" + attributeNameQuery + "]"));
        m_pendingMatchJobs.append(new MatchQuerySelectorAllJob(document, whitespaceTrimmedQuery));
        m_pendingMatchJobs.append(new MatchXPathJob(document, "//*[contains(@*, '" + escapedQuery + "')]"));
        if (!tagNameQuery.isEmpty())
            m_pendingMatchJobs.append(new MatchXPathJob(document, "//*[contains(name(), '" + escapedTagNameQuery + "')]"));
        m_pendingMatchJobs.append(new MatchPlainTextJob(document, escapedQuery));
        m_pendingMatchJobs.append(new MatchXPathJob(document, whitespaceTrimmedQuery));
    }

    if (runSynchronously && *runSynchronously) {
        // For tests.
        ListHashSet<Node*> resultCollector;
        for (Deque<MatchJob*>::iterator it = m_pendingMatchJobs.begin(); it != m_pendingMatchJobs.end(); ++it)
            (*it)->match(resultCollector);
        reportNodesAsSearchResults(resultCollector);
        cancelSearch(error);
        return;
    }
    m_matchJobsTimer.startOneShot(0);
}

void InspectorDOMAgent::cancelSearch(ErrorString*)
{
    if (m_matchJobsTimer.isActive())
        m_matchJobsTimer.stop();
    deleteAllValues(m_pendingMatchJobs);
    m_pendingMatchJobs.clear();
    m_searchResults.clear();
}

bool InspectorDOMAgent::handleMousePress()
{
    if (!m_searchingForNode)
        return false;

    if (m_highlightedNode) {
        RefPtr<Node> node = m_highlightedNode;
        setSearchingForNode(false);
        inspect(node.get());
    }
    return true;
}

void InspectorDOMAgent::inspect(Node* node)
{
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
    if (node) {
        ErrorString error;
        highlight(&error, node, "all");
    }
}

void InspectorDOMAgent::setSearchingForNode(bool enabled)
{
    if (m_searchingForNode == enabled)
        return;
    m_searchingForNode = enabled;
    if (!enabled) {
        ErrorString error;
        hideHighlight(&error);
    }
}

void InspectorDOMAgent::setInspectModeEnabled(ErrorString*, bool enabled)
{
    setSearchingForNode(enabled);
}

void InspectorDOMAgent::highlight(ErrorString*, Node* node, const String& mode)
{
    ASSERT_ARG(node, node);
    m_highlightedNode = node;
    m_highlightMode = mode;
    m_client->highlight(node);
}

void InspectorDOMAgent::highlightNode(ErrorString* error, int nodeId, String* mode)
{
    if (Node* node = nodeForId(nodeId))
        highlight(error, node, mode && !mode->isEmpty() ? *mode : "all");
}

void InspectorDOMAgent::highlightFrame(ErrorString* error, const String& frameId)
{
    Frame* frame = m_pageAgent->frameForId(frameId);
    if (frame && frame->ownerElement())
        highlight(error, frame->ownerElement(), "all");
}

void InspectorDOMAgent::hideHighlight(ErrorString*)
{
    m_highlightedNode = 0;
    m_client->hideHighlight();
}

void InspectorDOMAgent::resolveNode(ErrorString* error, int nodeId, RefPtr<InspectorObject>* result)
{
    Node* node = nodeForId(nodeId);
    if (!node) {
        *error = "No node with given id found.";
        return;
    }
    *result = resolveNode(node);
}

void InspectorDOMAgent::pushNodeToFrontend(ErrorString*, const String& objectId, int* nodeId)
{
    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptForObjectId(objectId);
    Node* node = injectedScript.nodeForObjectId(objectId);
    if (node)
        *nodeId = pushNodePathToFrontend(node);
    else
        *nodeId = 0;
}

String InspectorDOMAgent::documentURLString(Document* document) const
{
    if (!document || document->url().isNull())
        return "";
    return document->url().string();
}

PassRefPtr<InspectorObject> InspectorDOMAgent::buildObjectForNode(Node* node, int depth, NodeToIdMap* nodesMap)
{
    RefPtr<InspectorObject> value = InspectorObject::create();

    int id = bind(node, nodesMap);
    String nodeName;
    String localName;
    String nodeValue;

    switch (node->nodeType()) {
        case Node::TEXT_NODE:
        case Node::COMMENT_NODE:
        case Node::CDATA_SECTION_NODE:
            nodeValue = node->nodeValue();
            break;
        case Node::ATTRIBUTE_NODE:
            localName = node->localName();
            break;
        case Node::SHADOW_ROOT_NODE:
        case Node::DOCUMENT_FRAGMENT_NODE:
            break;
        case Node::DOCUMENT_NODE:
        case Node::ELEMENT_NODE:
        default:
            nodeName = node->nodeName();
            localName = node->localName();
            break;
    }

    value->setNumber("id", id);
    value->setNumber("nodeType", node->nodeType());
    value->setString("nodeName", nodeName);
    value->setString("localName", localName);
    value->setString("nodeValue", nodeValue);

    if (isContainerNode(*node)) {
        int nodeCount = innerChildNodeCount(node);
        value->setNumber("childNodeCount", nodeCount);
        RefPtr<InspectorArray> children = buildArrayForContainerChildren(node, depth, nodesMap);
        if (children->length() > 0)
            value->setArray("children", children.release());

        if (node->nodeType() == Node::ELEMENT_NODE) {
            Element* element = static_cast<Element*>(node);
            value->setArray("attributes", buildArrayForElementAttributes(element));
            if (node->isFrameOwnerElement()) {
                HTMLFrameOwnerElement* frameOwner = static_cast<HTMLFrameOwnerElement*>(node);
                value->setString("documentURL", documentURLString(frameOwner->contentDocument()));
            }
            if (ShadowRoot* shadowRoot = element->shadowRoot())
                value->setObject("shadowRoot", buildObjectForNode(shadowRoot, depth, nodesMap));
        } else if (node->nodeType() == Node::DOCUMENT_NODE) {
            Document* document = static_cast<Document*>(node);
            value->setString("documentURL", documentURLString(document));
        }
    } else if (node->nodeType() == Node::DOCUMENT_TYPE_NODE) {
        DocumentType* docType = static_cast<DocumentType*>(node);
        value->setString("publicId", docType->publicId());
        value->setString("systemId", docType->systemId());
        value->setString("internalSubset", docType->internalSubset());
    } else if (node->nodeType() == Node::ATTRIBUTE_NODE) {
        Attr* attribute = static_cast<Attr*>(node);
        value->setString("name", attribute->name());
        value->setString("value", attribute->value());
    }
    return value.release();
}

PassRefPtr<InspectorArray> InspectorDOMAgent::buildArrayForElementAttributes(Element* element)
{
    RefPtr<InspectorArray> attributesValue = InspectorArray::create();
    // Go through all attributes and serialize them.
    const NamedNodeMap* attrMap = element->attributes(true);
    if (!attrMap)
        return attributesValue.release();
    unsigned numAttrs = attrMap->length();
    for (unsigned i = 0; i < numAttrs; ++i) {
        // Add attribute pair
        const Attribute *attribute = attrMap->attributeItem(i);
        attributesValue->pushString(attribute->name().toString());
        attributesValue->pushString(attribute->value());
    }
    return attributesValue.release();
}

PassRefPtr<InspectorArray> InspectorDOMAgent::buildArrayForContainerChildren(Node* container, int depth, NodeToIdMap* nodesMap)
{
    RefPtr<InspectorArray> children = InspectorArray::create();
    Node* child = innerFirstChild(container);

    if (depth == 0) {
        // Special-case the only text child - pretend that container's children have been requested.
        if (child && child->nodeType() == Node::TEXT_NODE && !innerNextSibling(child))
            return buildArrayForContainerChildren(container, 1, nodesMap);
        return children.release();
    }

    depth--;
    m_childrenRequested.add(bind(container, nodesMap));

    while (child) {
        children->pushObject(buildObjectForNode(child, depth, nodesMap));
        child = innerNextSibling(child);
    }
    return children.release();
}

PassRefPtr<InspectorObject> InspectorDOMAgent::buildObjectForEventListener(const RegisteredEventListener& registeredEventListener, const AtomicString& eventType, Node* node)
{
    RefPtr<EventListener> eventListener = registeredEventListener.listener;
    RefPtr<InspectorObject> value = InspectorObject::create();
    value->setString("type", eventType);
    value->setBoolean("useCapture", registeredEventListener.useCapture);
    value->setBoolean("isAttribute", eventListener->isAttribute());
    value->setNumber("nodeId", pushNodePathToFrontend(node));
    value->setString("listenerBody", eventListenerHandlerBody(node->document(), eventListener.get()));
    String sourceName;
    int lineNumber;
    if (eventListenerHandlerLocation(node->document(), eventListener.get(), sourceName, lineNumber)) {
        value->setString("sourceName", sourceName);
        value->setNumber("lineNumber", lineNumber);
    }
    return value.release();
}

Node* InspectorDOMAgent::innerFirstChild(Node* node)
{
    if (node->isFrameOwnerElement()) {
        HTMLFrameOwnerElement* frameOwner = static_cast<HTMLFrameOwnerElement*>(node);
        Document* doc = frameOwner->contentDocument();
        if (doc)
            return doc->firstChild();
    }
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
    ContainerNode* parent = node->parentNode();
    if (parent && parent->isDocumentNode())
        return static_cast<Document*>(parent)->ownerElement();
    return parent;
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
    if (m_inspectorState->getBoolean(DOMAgentState::documentRequested))
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

    if (!m_childrenRequested.contains(frameOwnerId)) {
        // No children are mapped yet -> only notify on changes of hasChildren.
        m_frontend->childNodeCountUpdated(frameOwnerId, innerChildNodeCount(frameOwner));
    } else {
        // Re-add frame owner element together with its new children.
        int parentId = m_documentNodeToIdMap.get(innerParentNode(frameOwner));
        m_frontend->childNodeRemoved(parentId, frameOwnerId);
        RefPtr<InspectorObject> value = buildObjectForNode(frameOwner, 0, &m_documentNodeToIdMap);
        Node* previousSibling = innerPreviousSibling(frameOwner);
        int prevId = previousSibling ? m_documentNodeToIdMap.get(previousSibling) : 0;
        m_frontend->childNodeInserted(parentId, prevId, value.release());
        // Invalidate children requested flag for the element.
        m_childrenRequested.remove(m_childrenRequested.find(frameOwnerId));
    }
}

void InspectorDOMAgent::didInsertDOMNode(Node* node)
{
    if (isWhitespace(node))
        return;

    // We could be attaching existing subtree. Forget the bindings.
    unbind(node, &m_documentNodeToIdMap);

    ContainerNode* parent = node->isShadowRoot() ? node->shadowHost() : node->parentNode();
    int parentId = m_documentNodeToIdMap.get(parent);
    // Return if parent is not mapped yet.
    if (!parentId)
        return;

    if (node->isShadowRoot()) {
        RefPtr<InspectorObject> value = buildObjectForNode(node, 0, &m_documentNodeToIdMap);
        m_frontend->shadowRootUpdated(parentId, value.release());
        return;
    }
    if (!m_childrenRequested.contains(parentId)) {
        // No children are mapped yet -> only notify on changes of hasChildren.
        m_frontend->childNodeCountUpdated(parentId, innerChildNodeCount(parent));
    } else {
        // Children have been requested -> return value of a new child.
        Node* prevSibling = innerPreviousSibling(node);
        int prevId = prevSibling ? m_documentNodeToIdMap.get(prevSibling) : 0;
        RefPtr<InspectorObject> value = buildObjectForNode(node, 0, &m_documentNodeToIdMap);
        m_frontend->childNodeInserted(parentId, prevId, value.release());
    }
}

void InspectorDOMAgent::didRemoveDOMNode(Node* node)
{
    if (isWhitespace(node))
        return;

    ContainerNode* parent = node->isShadowRoot() ? node->shadowHost() : node->parentNode();
    int parentId = m_documentNodeToIdMap.get(parent);
    // If parent is not mapped yet -> ignore the event.
    if (!parentId)
        return;

    if (m_domListener)
        m_domListener->didRemoveDOMNode(node);

    if (node->isShadowRoot())
        m_frontend->shadowRootUpdated(parentId, 0);
    else if (!m_childrenRequested.contains(parentId)) {
        // No children are mapped yet -> only notify on changes of hasChildren.
        if (innerChildNodeCount(parent) == 1)
            m_frontend->childNodeCountUpdated(parentId, 0);
    } else
        m_frontend->childNodeRemoved(parentId, m_documentNodeToIdMap.get(node));
    unbind(node, &m_documentNodeToIdMap);
}

void InspectorDOMAgent::didModifyDOMAttr(Element* element)
{
    int id = m_documentNodeToIdMap.get(element);
    // If node is not mapped yet -> ignore the event.
    if (!id)
        return;

    if (m_domListener)
        m_domListener->didModifyDOMAttr(element);

    m_frontend->attributesUpdated(id, buildArrayForElementAttributes(element));
}

void InspectorDOMAgent::characterDataModified(CharacterData* characterData)
{
    int id = m_documentNodeToIdMap.get(characterData);
    if (!id)
        return;
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
    m_revalidateStyleAttrTask->scheduleFor(static_cast<Element*>(node));
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

void InspectorDOMAgent::onMatchJobsTimer(Timer<InspectorDOMAgent>*)
{
    if (!m_pendingMatchJobs.size()) {
        ErrorString error;
        cancelSearch(&error);
        return;
    }

    ListHashSet<Node*> resultCollector;
    MatchJob* job = m_pendingMatchJobs.takeFirst();
    job->match(resultCollector);
    delete job;

    reportNodesAsSearchResults(resultCollector);

    m_matchJobsTimer.startOneShot(0.025);
}

void InspectorDOMAgent::reportNodesAsSearchResults(ListHashSet<Node*>& resultCollector)
{
    RefPtr<InspectorArray> nodeIds = InspectorArray::create();
    for (ListHashSet<Node*>::iterator it = resultCollector.begin(); it != resultCollector.end(); ++it) {
        if (m_searchResults.contains(*it))
            continue;
        m_searchResults.add(*it);
        nodeIds->pushNumber(pushNodePathToFrontend(*it));
    }
    m_frontend->searchResults(nodeIds.release());
}

void InspectorDOMAgent::copyNode(ErrorString*, int nodeId)
{
    Node* node = nodeForId(nodeId);
    if (!node)
        return;
    String markup = createMarkup(node);
    Pasteboard::generalPasteboard()->writePlainText(markup);
}

void InspectorDOMAgent::pushNodeByPathToFrontend(ErrorString*, const String& path, int* nodeId)
{
    if (Node* node = nodeForPath(path))
        *nodeId = pushNodePathToFrontend(node);
}

PassRefPtr<InspectorObject> InspectorDOMAgent::resolveNode(Node* node)
{
    Document* document = node->ownerDocument();
    Frame* frame = document ? document->frame() : 0;
    if (!frame)
        return 0;

    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptFor(mainWorldScriptState(frame));
    if (injectedScript.hasNoValue())
        return 0;

    return injectedScript.wrapNode(node);
}

void InspectorDOMAgent::drawNodeHighlight(GraphicsContext& context) const
{
    if (!m_highlightedNode)
        return;

    DOMNodeHighlighter::HighlightMode mode = DOMNodeHighlighter::HighlightAll;
    if (m_highlightMode == "content")
        mode = DOMNodeHighlighter::HighlightContent;
    else if (m_highlightMode == "padding")
        mode = DOMNodeHighlighter::HighlightPadding;
    else if (m_highlightMode == "border")
        mode = DOMNodeHighlighter::HighlightBorder;
    else if (m_highlightMode == "margin")
        mode = DOMNodeHighlighter::HighlightMargin;
    DOMNodeHighlighter::DrawNodeHighlight(context, m_highlightedNode.get(), mode);
}

bool InspectorDOMAgent::isContainerNode(const Node& node)
{
     Node::NodeType type = node.nodeType();
     return type == Node::ELEMENT_NODE
         || type == Node::DOCUMENT_NODE
         || type == Node::DOCUMENT_FRAGMENT_NODE
         || type == Node::SHADOW_ROOT_NODE;
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
