/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef InspectorDOMAgent_h
#define InspectorDOMAgent_h

#include "EventTarget.h"
#include "InjectedScript.h"
#include "InjectedScriptManager.h"
#include "InspectorBaseAgent.h"
#include "InspectorFrontend.h"
#include "InspectorOverlay.h"
#include "InspectorValues.h"
#include "RenderLayer.h"
#include "Timer.h"

#include <wtf/Deque.h>
#include <wtf/ListHashSet.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {
class ContainerNode;
class CharacterData;
class DOMEditor;
class Document;
class Element;
class Event;
class InspectorClient;
class InspectorFrontend;
class InspectorHistory;
class InspectorOverlay;
class InspectorPageAgent;
class HitTestResult;
class HTMLElement;
class InspectorState;
class InstrumentingAgents;
class NameNodeMap;
class Node;
class RevalidateStyleAttributeTask;
class ScriptValue;
class ShadowRoot;

struct HighlightConfig;

typedef String ErrorString;
typedef int BackendNodeId;

#if ENABLE(INSPECTOR)

struct EventListenerInfo {
    EventListenerInfo(Node* node, const AtomicString& eventType, const EventListenerVector& eventListenerVector)
        : node(node)
        , eventType(eventType)
        , eventListenerVector(eventListenerVector)
    {
    }

    Node* node;
    const AtomicString eventType;
    const EventListenerVector eventListenerVector;
};

class InspectorDOMAgent : public InspectorBaseAgent<InspectorDOMAgent>, public InspectorBackendDispatcher::DOMCommandHandler {
    WTF_MAKE_NONCOPYABLE(InspectorDOMAgent);
public:
    struct DOMListener {
        virtual ~DOMListener()
        {
        }
        virtual void didRemoveDocument(Document*) = 0;
        virtual void didRemoveDOMNode(Node*) = 0;
        virtual void didModifyDOMAttr(Element*) = 0;
    };

    static PassOwnPtr<InspectorDOMAgent> create(InstrumentingAgents* instrumentingAgents, InspectorPageAgent* pageAgent, InspectorCompositeState* inspectorState, InjectedScriptManager* injectedScriptManager, InspectorOverlay* overlay, InspectorClient* client)
    {
        return adoptPtr(new InspectorDOMAgent(instrumentingAgents, pageAgent, inspectorState, injectedScriptManager, overlay, client));
    }

    static String toErrorString(const ExceptionCode&);

    ~InspectorDOMAgent();

    virtual void setFrontend(InspectorFrontend*);
    virtual void clearFrontend();
    virtual void restore();

    Vector<Document*> documents();
    void reset();

    // Methods called from the frontend for DOM nodes inspection.
    virtual void querySelector(ErrorString*, int nodeId, const String& selectors, int* elementId);
    virtual void querySelectorAll(ErrorString*, int nodeId, const String& selectors, RefPtr<TypeBuilder::Array<int> >& result);
    virtual void getDocument(ErrorString*, RefPtr<TypeBuilder::DOM::Node>& root);
    virtual void requestChildNodes(ErrorString*, int nodeId, const int* depth);
    virtual void setAttributeValue(ErrorString*, int elementId, const String& name, const String& value);
    virtual void setAttributesAsText(ErrorString*, int elementId, const String& text, const String* name);
    virtual void removeAttribute(ErrorString*, int elementId, const String& name);
    virtual void removeNode(ErrorString*, int nodeId);
    virtual void setNodeName(ErrorString*, int nodeId, const String& name, int* newId);
    virtual void getOuterHTML(ErrorString*, int nodeId, WTF::String* outerHTML);
    virtual void setOuterHTML(ErrorString*, int nodeId, const String& outerHTML);
    virtual void setNodeValue(ErrorString*, int nodeId, const String& value);
    virtual void getEventListenersForNode(ErrorString*, int nodeId, const WTF::String* objectGroup, RefPtr<TypeBuilder::Array<TypeBuilder::DOM::EventListener> >& listenersArray);
    virtual void performSearch(ErrorString*, const String& whitespaceTrimmedQuery, String* searchId, int* resultCount);
    virtual void getSearchResults(ErrorString*, const String& searchId, int fromIndex, int toIndex, RefPtr<TypeBuilder::Array<int> >&);
    virtual void discardSearchResults(ErrorString*, const String& searchId);
    virtual void resolveNode(ErrorString*, int nodeId, const String* objectGroup, RefPtr<TypeBuilder::Runtime::RemoteObject>& result);
    virtual void getAttributes(ErrorString*, int nodeId, RefPtr<TypeBuilder::Array<String> >& result);
    virtual void setInspectModeEnabled(ErrorString*, bool enabled, const RefPtr<InspectorObject>* highlightConfig);
    virtual void requestNode(ErrorString*, const String& objectId, int* nodeId);
    virtual void pushNodeByPathToFrontend(ErrorString*, const String& path, int* nodeId);
    virtual void pushNodeByBackendIdToFrontend(ErrorString*, BackendNodeId, int* nodeId);
    virtual void releaseBackendNodeIds(ErrorString*, const String& nodeGroup);
    virtual void hideHighlight(ErrorString*);
    virtual void highlightRect(ErrorString*, int x, int y, int width, int height, const RefPtr<InspectorObject>* color, const RefPtr<InspectorObject>* outlineColor, const bool* usePageCoordinates);
    virtual void highlightQuad(ErrorString*, const RefPtr<InspectorArray>& quad, const RefPtr<InspectorObject>* color, const RefPtr<InspectorObject>* outlineColor, const bool* usePageCoordinates);
    virtual void highlightNode(ErrorString*, const RefPtr<InspectorObject>& highlightConfig, const int* nodeId, const String* objectId);
    virtual void highlightFrame(ErrorString*, const String& frameId, const RefPtr<InspectorObject>* color, const RefPtr<InspectorObject>* outlineColor);

    virtual void moveTo(ErrorString*, int nodeId, int targetNodeId, const int* anchorNodeId, int* newNodeId);
    virtual void undo(ErrorString*);
    virtual void redo(ErrorString*);
    virtual void markUndoableState(ErrorString*);
    virtual void focus(ErrorString*, int nodeId);
    virtual void setFileInputFiles(ErrorString*, int nodeId, const RefPtr<InspectorArray>& files);

    void getEventListeners(Node*, Vector<EventListenerInfo>& listenersArray, bool includeAncestors);

    // Methods called from the InspectorInstrumentation.
    void setDocument(Document*);
    void releaseDanglingNodes();

    void mainFrameDOMContentLoaded();
    void loadEventFired(Document*);

    void didInsertDOMNode(Node*);
    void didRemoveDOMNode(Node*);
    void willModifyDOMAttr(Element*, const AtomicString& oldValue, const AtomicString& newValue);
    void didModifyDOMAttr(Element*, const AtomicString& name, const AtomicString& value);
    void didRemoveDOMAttr(Element*, const AtomicString& name);
    void styleAttributeInvalidated(const Vector<Element*>& elements);
    void characterDataModified(CharacterData*);
    void didInvalidateStyleAttr(Node*);
    void didPushShadowRoot(Element* host, ShadowRoot*);
    void willPopShadowRoot(Element* host, ShadowRoot*);
    void frameDocumentUpdated(Frame*);

    int pushNodeToFrontend(ErrorString*, int documentNodeId, Node*);
    Node* nodeForId(int nodeId);
    int boundNodeId(Node*);
    void setDOMListener(DOMListener*);
    BackendNodeId backendNodeIdForNode(Node*, const String& nodeGroup);

    static String documentURLString(Document*);

    PassRefPtr<TypeBuilder::Runtime::RemoteObject> resolveNode(Node*, const String& objectGroup);
    bool handleMousePress();
    bool handleTouchEvent(Node*);
    void mouseDidMoveOverElement(const HitTestResult&, unsigned modifierFlags);
    void inspect(Node*);
    void focusNode();

    InspectorHistory* history() { return m_history.get(); }

    // We represent embedded doms as a part of the same hierarchy. Hence we treat children of frame owners differently.
    // We also skip whitespace text nodes conditionally. Following methods encapsulate these specifics.
    static Node* innerFirstChild(Node*);
    static Node* innerNextSibling(Node*);
    static Node* innerPreviousSibling(Node*);
    static unsigned innerChildNodeCount(Node*);
    static Node* innerParentNode(Node*);
    static bool isWhitespace(Node*);

    Node* assertNode(ErrorString*, int nodeId);
    Element* assertElement(ErrorString*, int nodeId);
    Document* assertDocument(ErrorString*, int nodeId);

    // Methods called from other agents.
    InspectorPageAgent* pageAgent() { return m_pageAgent; }

private:
    InspectorDOMAgent(InstrumentingAgents*, InspectorPageAgent*, InspectorCompositeState*, InjectedScriptManager*, InspectorOverlay*, InspectorClient*);

    void setSearchingForNode(ErrorString*, bool enabled, InspectorObject* highlightConfig);
    PassOwnPtr<HighlightConfig> highlightConfigFromInspectorObject(ErrorString*, InspectorObject* highlightInspectorObject);

    // Node-related methods.
    typedef HashMap<RefPtr<Node>, int> NodeToIdMap;
    int bind(Node*, NodeToIdMap*);
    void unbind(Node*, NodeToIdMap*);

    Node* assertEditableNode(ErrorString*, int nodeId);
    Element* assertEditableElement(ErrorString*, int nodeId);

    int pushNodePathToFrontend(Node*);
    void pushChildNodesToFrontend(int nodeId, int depth = 1);

    bool hasBreakpoint(Node*, int type);
    void updateSubtreeBreakpoints(Node* root, uint32_t rootMask, bool value);
    void descriptionForDOMEvent(Node* target, int breakpointType, bool insertion, PassRefPtr<InspectorObject> description);

    PassRefPtr<TypeBuilder::DOM::Node> buildObjectForNode(Node*, int depth, NodeToIdMap*);
    PassRefPtr<TypeBuilder::Array<String> > buildArrayForElementAttributes(Element*);
    PassRefPtr<TypeBuilder::Array<TypeBuilder::DOM::Node> > buildArrayForContainerChildren(Node* container, int depth, NodeToIdMap* nodesMap);
    PassRefPtr<TypeBuilder::DOM::EventListener> buildObjectForEventListener(const RegisteredEventListener&, const AtomicString& eventType, Node*, const String* objectGroupId);

    Node* nodeForPath(const String& path);

    void discardBindings();

    void innerHighlightQuad(PassOwnPtr<FloatQuad>, const RefPtr<InspectorObject>* color, const RefPtr<InspectorObject>* outlineColor, const bool* usePageCoordinates);

    InspectorPageAgent* m_pageAgent;
    InjectedScriptManager* m_injectedScriptManager;
    InspectorOverlay* m_overlay;
    InspectorClient* m_client;
    InspectorFrontend::DOM* m_frontend;
    DOMListener* m_domListener;
    NodeToIdMap m_documentNodeToIdMap;
    typedef HashMap<RefPtr<Node>, BackendNodeId> NodeToBackendIdMap;
    HashMap<String, NodeToBackendIdMap> m_nodeGroupToBackendIdMap;
    // Owns node mappings for dangling nodes.
    Vector<OwnPtr<NodeToIdMap> > m_danglingNodeToIdMaps;
    HashMap<int, Node*> m_idToNode;
    HashMap<int, NodeToIdMap*> m_idToNodesMap;
    HashSet<int> m_childrenRequested;
    HashMap<BackendNodeId, std::pair<Node*, String> > m_backendIdToNode;
    int m_lastNodeId;
    BackendNodeId m_lastBackendNodeId;
    RefPtr<Document> m_document;
    typedef HashMap<String, Vector<RefPtr<Node> > > SearchResults;
    SearchResults m_searchResults;
    OwnPtr<RevalidateStyleAttributeTask> m_revalidateStyleAttrTask;
    RefPtr<Node> m_nodeToFocus;
    bool m_searchingForNode;
    OwnPtr<HighlightConfig> m_inspectModeHighlightConfig;
    OwnPtr<InspectorHistory> m_history;
    OwnPtr<DOMEditor> m_domEditor;
    bool m_suppressAttributeModifiedEvent;
};

#endif // ENABLE(INSPECTOR)

} // namespace WebCore

#endif // !defined(InspectorDOMAgent_h)
