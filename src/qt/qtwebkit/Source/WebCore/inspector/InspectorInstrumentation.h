/*
* Copyright (C) 2010 Google Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*
*     * Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
* copyright notice, this list of conditions and the following disclaimer
* in the documentation and/or other materials provided with the
* distribution.
*     * Neither the name of Google Inc. nor the names of its
* contributors may be used to endorse or promote products derived from
* this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef InspectorInstrumentation_h
#define InspectorInstrumentation_h

#include "CSSSelector.h"
#include "ConsoleAPITypes.h"
#include "ConsoleTypes.h"
#include "Element.h"
#include "EventContext.h"
#include "FormData.h"
#include "Frame.h"
#include "HitTestResult.h"
#include "Page.h"
#include "ScriptExecutionContext.h"
#include "ScriptState.h"
#include "StorageArea.h"
#include "WebSocketFrame.h"
#include <wtf/RefPtr.h>

namespace WebCore {

class CSSRule;
class CachedResource;
class CharacterData;
class DOMWindow;
class DOMWrapperWorld;
class Database;
class Document;
class Element;
class EventContext;
class DocumentLoader;
class DocumentStyleSheetCollection;
class DeviceOrientationData;
class GeolocationPosition;
class GraphicsContext;
class HTTPHeaderMap;
class InspectorCSSAgent;
class InspectorCSSOMWrappers;
class InspectorInstrumentation;
class InspectorTimelineAgent;
class InstrumentingAgents;
class KURL;
class Node;
class PseudoElement;
class RenderLayer;
class RenderLayerBacking;
class RenderObject;
class ResourceRequest;
class ResourceResponse;
class ScriptArguments;
class ScriptCallStack;
class ScriptExecutionContext;
class ScriptObject;
class ScriptProfile;
class SecurityOrigin;
class ShadowRoot;
class StorageArea;
class StyleResolver;
class StyleRule;
class ThreadableLoaderClient;
class WorkerGlobalScope;
class WorkerGlobalScopeProxy;
class XMLHttpRequest;

#define FAST_RETURN_IF_NO_FRONTENDS(value) if (LIKELY(!hasFrontends())) return value;

class InspectorInstrumentationCookie {
#if ENABLE(INSPECTOR)
public:
    InspectorInstrumentationCookie();
    InspectorInstrumentationCookie(InstrumentingAgents*, int);
    InspectorInstrumentationCookie(const InspectorInstrumentationCookie&);
    InspectorInstrumentationCookie& operator=(const InspectorInstrumentationCookie&);
    ~InspectorInstrumentationCookie();

private:
    friend class InspectorInstrumentation;
    InstrumentingAgents* instrumentingAgents() const { return m_instrumentingAgents.get(); }
    bool isValid() const { return !!m_instrumentingAgents; }
    bool hasMatchingTimelineAgentId(int id) const { return m_timelineAgentId == id; }

    RefPtr<InstrumentingAgents> m_instrumentingAgents;
    int m_timelineAgentId;
#endif
};

class InspectorInstrumentation {
public:
    static void didClearWindowObjectInWorld(Frame*, DOMWrapperWorld*);
    static bool isDebuggerPaused(Frame*);

    static void willInsertDOMNode(Document*, Node* parent);
    static void didInsertDOMNode(Document*, Node*);
    static void willRemoveDOMNode(Document*, Node*);
    static void willModifyDOMAttr(Document*, Element*, const AtomicString& oldValue, const AtomicString& newValue);
    static void didModifyDOMAttr(Document*, Element*, const AtomicString& name, const AtomicString& value);
    static void didRemoveDOMAttr(Document*, Element*, const AtomicString& name);
    static void characterDataModified(Document*, CharacterData*);
    static void didInvalidateStyleAttr(Document*, Node*);
    static void frameWindowDiscarded(Frame*, DOMWindow*);
    static void mediaQueryResultChanged(Document*);
    static void didPushShadowRoot(Element* host, ShadowRoot*);
    static void willPopShadowRoot(Element* host, ShadowRoot*);
    static void didCreateNamedFlow(Document*, WebKitNamedFlow*);
    static void willRemoveNamedFlow(Document*, WebKitNamedFlow*);
    static void didUpdateRegionLayout(Document*, WebKitNamedFlow*);
    static void didChangeRegionOverset(Document*, WebKitNamedFlow*);

    static void mouseDidMoveOverElement(Page*, const HitTestResult&, unsigned modifierFlags);
    static bool handleMousePress(Page*);
    static bool handleTouchEvent(Page*, Node*);
    static bool forcePseudoState(Element*, CSSSelector::PseudoType);

    static void willSendXMLHttpRequest(ScriptExecutionContext*, const String& url);
    static void didScheduleResourceRequest(Document*, const String& url);
    static void didInstallTimer(ScriptExecutionContext*, int timerId, int timeout, bool singleShot);
    static void didRemoveTimer(ScriptExecutionContext*, int timerId);

    static InspectorInstrumentationCookie willCallFunction(ScriptExecutionContext*, const String& scriptName, int scriptLine);
    static void didCallFunction(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willDispatchXHRReadyStateChangeEvent(ScriptExecutionContext*, XMLHttpRequest*);
    static void didDispatchXHRReadyStateChangeEvent(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willDispatchEvent(Document*, const Event&, DOMWindow*, Node*, const EventPath&);
    static void didDispatchEvent(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willHandleEvent(ScriptExecutionContext*, Event*);
    static void didHandleEvent(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willDispatchEventOnWindow(Frame*, const Event& event, DOMWindow* window);
    static void didDispatchEventOnWindow(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willEvaluateScript(Frame*, const String& url, int lineNumber);
    static void didEvaluateScript(const InspectorInstrumentationCookie&);
    static void scriptsEnabled(Page*, bool isEnabled);
    static void didCreateIsolatedContext(Frame*, ScriptState*, SecurityOrigin*);
    static InspectorInstrumentationCookie willFireTimer(ScriptExecutionContext*, int timerId);
    static void didFireTimer(const InspectorInstrumentationCookie&);
    static void didInvalidateLayout(Frame*);
    static InspectorInstrumentationCookie willLayout(Frame*);
    static void didLayout(const InspectorInstrumentationCookie&, RenderObject*);
    static void didScroll(Page*);
    static InspectorInstrumentationCookie willDispatchXHRLoadEvent(ScriptExecutionContext*, XMLHttpRequest*);
    static void didDispatchXHRLoadEvent(const InspectorInstrumentationCookie&);
    static void willScrollLayer(Frame*);
    static void didScrollLayer(Frame*);
    static void willPaint(RenderObject*);
    static void didPaint(RenderObject*, GraphicsContext*, const LayoutRect&);
    static void willComposite(Page*);
    static void didComposite(Page*);
    static InspectorInstrumentationCookie willRecalculateStyle(Document*);
    static void didRecalculateStyle(const InspectorInstrumentationCookie&);
    static void didScheduleStyleRecalculation(Document*);
    static InspectorInstrumentationCookie willMatchRule(Document*, StyleRule*, InspectorCSSOMWrappers&, DocumentStyleSheetCollection*);
    static void didMatchRule(const InspectorInstrumentationCookie&, bool matched);
    static InspectorInstrumentationCookie willProcessRule(Document*, StyleRule*, StyleResolver*);
    static void didProcessRule(const InspectorInstrumentationCookie&);

    static void applyUserAgentOverride(Frame*, String*);
    static void applyScreenWidthOverride(Frame*, long*);
    static void applyScreenHeightOverride(Frame*, long*);
    static void applyEmulatedMedia(Frame*, String*);
    static bool shouldApplyScreenWidthOverride(Frame*);
    static bool shouldApplyScreenHeightOverride(Frame*);
    static void willSendRequest(Frame*, unsigned long identifier, DocumentLoader*, ResourceRequest&, const ResourceResponse& redirectResponse);
    static void continueAfterPingLoader(Frame*, unsigned long identifier, DocumentLoader*, ResourceRequest&, const ResourceResponse&);
    static void markResourceAsCached(Page*, unsigned long identifier);
    static void didLoadResourceFromMemoryCache(Page*, DocumentLoader*, CachedResource*);
    static InspectorInstrumentationCookie willReceiveResourceData(Frame*, unsigned long identifier, int length);
    static void didReceiveResourceData(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willReceiveResourceResponse(Frame*, unsigned long identifier, const ResourceResponse&);
    static void didReceiveResourceResponse(const InspectorInstrumentationCookie&, unsigned long identifier, DocumentLoader*, const ResourceResponse&, ResourceLoader*);
    static void continueAfterXFrameOptionsDenied(Frame*, DocumentLoader*, unsigned long identifier, const ResourceResponse&);
    static void continueWithPolicyDownload(Frame*, DocumentLoader*, unsigned long identifier, const ResourceResponse&);
    static void continueWithPolicyIgnore(Frame*, DocumentLoader*, unsigned long identifier, const ResourceResponse&);
    static void didReceiveData(Frame*, unsigned long identifier, const char* data, int dataLength, int encodedDataLength);
    static void didFinishLoading(Frame*, DocumentLoader*, unsigned long identifier, double finishTime);
    static void didFailLoading(Frame*, DocumentLoader*, unsigned long identifier, const ResourceError&);
    static void documentThreadableLoaderStartedLoadingForClient(ScriptExecutionContext*, unsigned long identifier, ThreadableLoaderClient*);
    static void willLoadXHR(ScriptExecutionContext*, ThreadableLoaderClient*, const String&, const KURL&, bool, PassRefPtr<FormData>, const HTTPHeaderMap&, bool);
    static void didFailXHRLoading(ScriptExecutionContext*, ThreadableLoaderClient*);
    static void didFinishXHRLoading(ScriptExecutionContext*, ThreadableLoaderClient*, unsigned long identifier, const String& sourceString, const String& url, const String& sendURL, unsigned sendLineNumber);
    static void didReceiveXHRResponse(ScriptExecutionContext*, unsigned long identifier);
    static void willLoadXHRSynchronously(ScriptExecutionContext*);
    static void didLoadXHRSynchronously(ScriptExecutionContext*);
    static void scriptImported(ScriptExecutionContext*, unsigned long identifier, const String& sourceString);
    static void scriptExecutionBlockedByCSP(ScriptExecutionContext*, const String& directiveText);
    static void didReceiveScriptResponse(ScriptExecutionContext*, unsigned long identifier);
    static void domContentLoadedEventFired(Frame*);
    static void loadEventFired(Frame*);
    static void frameDetachedFromParent(Frame*);
    static void didCommitLoad(Frame*, DocumentLoader*);
    static void frameDocumentUpdated(Frame*);
    static void loaderDetachedFromFrame(Frame*, DocumentLoader*);
    static void frameStartedLoading(Frame*);
    static void frameStoppedLoading(Frame*);
    static void frameScheduledNavigation(Frame*, double delay);
    static void frameClearedScheduledNavigation(Frame*);
    static InspectorInstrumentationCookie willRunJavaScriptDialog(Page*, const String& message);
    static void didRunJavaScriptDialog(const InspectorInstrumentationCookie&);
    static void willDestroyCachedResource(CachedResource*);

    static InspectorInstrumentationCookie willWriteHTML(Document*, unsigned startLine);
    static void didWriteHTML(const InspectorInstrumentationCookie&, unsigned endLine);

    // FIXME: Remove once we no longer generate stacks outside of Inspector.
    static void addMessageToConsole(Page*, MessageSource, MessageType, MessageLevel, const String& message, ScriptCallStack*, unsigned long requestIdentifier = 0);
    static void addMessageToConsole(Page*, MessageSource, MessageType, MessageLevel, const String& message, ScriptState*, ScriptArguments*, unsigned long requestIdentifier = 0);
    static void addMessageToConsole(Page*, MessageSource, MessageType, MessageLevel, const String& message, const String& scriptId, unsigned lineNumber, unsigned columnNumber, ScriptState* = 0, unsigned long requestIdentifier = 0);
#if ENABLE(WORKERS)
    // FIXME: Convert to ScriptArguments to match non-worker context.
    static void addMessageToConsole(WorkerGlobalScope*, MessageSource, MessageType, MessageLevel, const String& message, ScriptCallStack*, unsigned long requestIdentifier = 0);
    static void addMessageToConsole(WorkerGlobalScope*, MessageSource, MessageType, MessageLevel, const String& message, const String& scriptId, unsigned lineNumber, unsigned columnNumber, ScriptState* = 0, unsigned long requestIdentifier = 0);
#endif
    static void consoleCount(Page*, ScriptState*, PassRefPtr<ScriptArguments>);
    static void startConsoleTiming(Frame*, const String& title);
    static void stopConsoleTiming(Frame*, const String& title, PassRefPtr<ScriptCallStack>);
    static void consoleTimeStamp(Frame*, PassRefPtr<ScriptArguments>);

    static void didRequestAnimationFrame(Document*, int callbackId);
    static void didCancelAnimationFrame(Document*, int callbackId);
    static InspectorInstrumentationCookie willFireAnimationFrame(Document*, int callbackId);
    static void didFireAnimationFrame(const InspectorInstrumentationCookie&);

#if ENABLE(JAVASCRIPT_DEBUGGER)
    static void addStartProfilingMessageToConsole(Page*, const String& title, unsigned lineNumber, unsigned columnNumber, const String& sourceURL);
    static void addProfile(Page*, RefPtr<ScriptProfile>, PassRefPtr<ScriptCallStack>);
    static String getCurrentUserInitiatedProfileName(Page*, bool incrementProfileNumber);
    static bool profilerEnabled(Page*);
#endif

#if ENABLE(SQL_DATABASE)
    static void didOpenDatabase(ScriptExecutionContext*, PassRefPtr<Database>, const String& domain, const String& name, const String& version);
#endif

    static void didDispatchDOMStorageEvent(const String& key, const String& oldValue, const String& newValue, StorageType, SecurityOrigin*, Page*);

#if ENABLE(WORKERS)
    static bool shouldPauseDedicatedWorkerOnStart(ScriptExecutionContext*);
    static void didStartWorkerGlobalScope(ScriptExecutionContext*, WorkerGlobalScopeProxy*, const KURL&);
    static void workerGlobalScopeTerminated(ScriptExecutionContext*, WorkerGlobalScopeProxy*);
    static void willEvaluateWorkerScript(WorkerGlobalScope*, int workerThreadStartMode);
#endif

#if ENABLE(WEB_SOCKETS)
    static void didCreateWebSocket(Document*, unsigned long identifier, const KURL& requestURL, const KURL& documentURL, const String& protocol);
    static void willSendWebSocketHandshakeRequest(Document*, unsigned long identifier, const ResourceRequest&);
    static void didReceiveWebSocketHandshakeResponse(Document*, unsigned long identifier, const ResourceResponse&);
    static void didCloseWebSocket(Document*, unsigned long identifier);
    static void didReceiveWebSocketFrame(Document*, unsigned long identifier, const WebSocketFrame&);
    static void didSendWebSocketFrame(Document*, unsigned long identifier, const WebSocketFrame&);
    static void didReceiveWebSocketFrameError(Document*, unsigned long identifier, const String& errorMessage);
#endif

    static ScriptObject wrapCanvas2DRenderingContextForInstrumentation(Document*, const ScriptObject&);
#if ENABLE(WEBGL)
    static ScriptObject wrapWebGLRenderingContextForInstrumentation(Document*, const ScriptObject&);
#endif

    static void networkStateChanged(Page*);
    static void updateApplicationCacheStatus(Frame*);

#if ENABLE(INSPECTOR)
    static void frontendCreated() { s_frontendCounter += 1; }
    static void frontendDeleted() { s_frontendCounter -= 1; }
    static bool hasFrontends() { return s_frontendCounter; }
    static bool canvasAgentEnabled(ScriptExecutionContext*);
    static bool consoleAgentEnabled(ScriptExecutionContext*);
    static bool timelineAgentEnabled(ScriptExecutionContext*);
    static bool collectingHTMLParseErrors(Page*);
#else
    static bool hasFrontends() { return false; }
    static bool canvasAgentEnabled(ScriptExecutionContext*) { return false; }
    static bool consoleAgentEnabled(ScriptExecutionContext*) { return false; }
    static bool runtimeAgentEnabled(Frame*) { return false; }
    static bool timelineAgentEnabled(ScriptExecutionContext*) { return false; }
    static bool collectingHTMLParseErrors(Page*) { return false; }
#endif

#if ENABLE(GEOLOCATION)
    static GeolocationPosition* overrideGeolocationPosition(Page*, GeolocationPosition*);
#endif

    static void registerInstrumentingAgents(InstrumentingAgents*);
    static void unregisterInstrumentingAgents(InstrumentingAgents*);

    static DeviceOrientationData* overrideDeviceOrientation(Page*, DeviceOrientationData*);

#if USE(ACCELERATED_COMPOSITING)
    static void layerTreeDidChange(Page*);
    static void renderLayerDestroyed(Page*, const RenderLayer*);
    static void pseudoElementDestroyed(Page*, PseudoElement*);
#endif

private:
#if ENABLE(INSPECTOR)
    static void didClearWindowObjectInWorldImpl(InstrumentingAgents*, Frame*, DOMWrapperWorld*);
    static bool isDebuggerPausedImpl(InstrumentingAgents*);

    static void willInsertDOMNodeImpl(InstrumentingAgents*, Node* parent);
    static void didInsertDOMNodeImpl(InstrumentingAgents*, Node*);
    static void willRemoveDOMNodeImpl(InstrumentingAgents*, Node*);
    static void didRemoveDOMNodeImpl(InstrumentingAgents*, Node*);
    static void willModifyDOMAttrImpl(InstrumentingAgents*, Element*, const AtomicString& oldValue, const AtomicString& newValue);
    static void didModifyDOMAttrImpl(InstrumentingAgents*, Element*, const AtomicString& name, const AtomicString& value);
    static void didRemoveDOMAttrImpl(InstrumentingAgents*, Element*, const AtomicString& name);
    static void characterDataModifiedImpl(InstrumentingAgents*, CharacterData*);
    static void didInvalidateStyleAttrImpl(InstrumentingAgents*, Node*);
    static void frameWindowDiscardedImpl(InstrumentingAgents*, DOMWindow*);
    static void mediaQueryResultChangedImpl(InstrumentingAgents*);
    static void didPushShadowRootImpl(InstrumentingAgents*, Element* host, ShadowRoot*);
    static void willPopShadowRootImpl(InstrumentingAgents*, Element* host, ShadowRoot*);
    static void didCreateNamedFlowImpl(InstrumentingAgents*, Document*, WebKitNamedFlow*);
    static void willRemoveNamedFlowImpl(InstrumentingAgents*, Document*, WebKitNamedFlow*);
    static void didUpdateRegionLayoutImpl(InstrumentingAgents*, Document*, WebKitNamedFlow*);
    static void didChangeRegionOversetImpl(InstrumentingAgents*, Document*, WebKitNamedFlow*);

    static void mouseDidMoveOverElementImpl(InstrumentingAgents*, const HitTestResult&, unsigned modifierFlags);
    static bool handleTouchEventImpl(InstrumentingAgents*, Node*);
    static bool handleMousePressImpl(InstrumentingAgents*);
    static bool forcePseudoStateImpl(InstrumentingAgents*, Element*, CSSSelector::PseudoType);

    static void willSendXMLHttpRequestImpl(InstrumentingAgents*, const String& url);
    static void didScheduleResourceRequestImpl(InstrumentingAgents*, const String& url, Frame*);
    static void didInstallTimerImpl(InstrumentingAgents*, int timerId, int timeout, bool singleShot, ScriptExecutionContext*);
    static void didRemoveTimerImpl(InstrumentingAgents*, int timerId, ScriptExecutionContext*);

    static InspectorInstrumentationCookie willCallFunctionImpl(InstrumentingAgents*, const String& scriptName, int scriptLine, ScriptExecutionContext*);
    static void didCallFunctionImpl(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willDispatchXHRReadyStateChangeEventImpl(InstrumentingAgents*, XMLHttpRequest*, ScriptExecutionContext*);
    static void didDispatchXHRReadyStateChangeEventImpl(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willDispatchEventImpl(InstrumentingAgents*, const Event&, DOMWindow*, Node*, const EventPath&, Document*);
    static InspectorInstrumentationCookie willHandleEventImpl(InstrumentingAgents*, Event*);
    static void didHandleEventImpl(const InspectorInstrumentationCookie&);
    static void didDispatchEventImpl(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willDispatchEventOnWindowImpl(InstrumentingAgents*, const Event&, DOMWindow*);
    static void didDispatchEventOnWindowImpl(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willEvaluateScriptImpl(InstrumentingAgents*, const String& url, int lineNumber, Frame*);
    static void didEvaluateScriptImpl(const InspectorInstrumentationCookie&);
    static void scriptsEnabledImpl(InstrumentingAgents*, bool isEnabled);
    static void didCreateIsolatedContextImpl(InstrumentingAgents*, Frame*, ScriptState*, SecurityOrigin*);
    static InspectorInstrumentationCookie willFireTimerImpl(InstrumentingAgents*, int timerId, ScriptExecutionContext*);
    static void didFireTimerImpl(const InspectorInstrumentationCookie&);
    static void didInvalidateLayoutImpl(InstrumentingAgents*, Frame*);
    static InspectorInstrumentationCookie willLayoutImpl(InstrumentingAgents*, Frame*);
    static void didLayoutImpl(const InspectorInstrumentationCookie&, RenderObject*);
    static void didScrollImpl(InstrumentingAgents*);
    static InspectorInstrumentationCookie willDispatchXHRLoadEventImpl(InstrumentingAgents*, XMLHttpRequest*, ScriptExecutionContext*);
    static void didDispatchXHRLoadEventImpl(const InspectorInstrumentationCookie&);
    static void willScrollLayerImpl(InstrumentingAgents*, Frame*);
    static void didScrollLayerImpl(InstrumentingAgents*);
    static void willPaintImpl(InstrumentingAgents*, RenderObject*);
    static void didPaintImpl(InstrumentingAgents*, RenderObject*, GraphicsContext*, const LayoutRect&);
    static InspectorInstrumentationCookie willRecalculateStyleImpl(InstrumentingAgents*, Frame*);
    static void didRecalculateStyleImpl(const InspectorInstrumentationCookie&);
    static void didScheduleStyleRecalculationImpl(InstrumentingAgents*, Document*);
    static InspectorInstrumentationCookie willMatchRuleImpl(InstrumentingAgents*, StyleRule*, InspectorCSSOMWrappers&, DocumentStyleSheetCollection*);
    static void didMatchRuleImpl(const InspectorInstrumentationCookie&, bool matched);
    static InspectorInstrumentationCookie willProcessRuleImpl(InstrumentingAgents*, StyleRule*, StyleResolver*);
    static void didProcessRuleImpl(const InspectorInstrumentationCookie&);

    static void applyUserAgentOverrideImpl(InstrumentingAgents*, String*);
    static void applyScreenWidthOverrideImpl(InstrumentingAgents*, long*);
    static void applyScreenHeightOverrideImpl(InstrumentingAgents*, long*);
    static void applyEmulatedMediaImpl(InstrumentingAgents*, String*);
    static bool shouldApplyScreenWidthOverrideImpl(InstrumentingAgents*);
    static bool shouldApplyScreenHeightOverrideImpl(InstrumentingAgents*);
    static void willSendRequestImpl(InstrumentingAgents*, unsigned long identifier, DocumentLoader*, ResourceRequest&, const ResourceResponse& redirectResponse);
    static void continueAfterPingLoaderImpl(InstrumentingAgents*, unsigned long identifier, DocumentLoader*, ResourceRequest&, const ResourceResponse&);
    static void markResourceAsCachedImpl(InstrumentingAgents*, unsigned long identifier);
    static void didLoadResourceFromMemoryCacheImpl(InstrumentingAgents*, DocumentLoader*, CachedResource*);
    static InspectorInstrumentationCookie willReceiveResourceDataImpl(InstrumentingAgents*, unsigned long identifier, Frame*, int length);
    static void didReceiveResourceDataImpl(const InspectorInstrumentationCookie&);
    static InspectorInstrumentationCookie willReceiveResourceResponseImpl(InstrumentingAgents*, unsigned long identifier, const ResourceResponse&, Frame*);
    static void didReceiveResourceResponseImpl(const InspectorInstrumentationCookie&, unsigned long identifier, DocumentLoader*, const ResourceResponse&, ResourceLoader*);
    static void didReceiveResourceResponseButCanceledImpl(Frame*, DocumentLoader*, unsigned long identifier, const ResourceResponse&);
    static void continueAfterXFrameOptionsDeniedImpl(Frame*, DocumentLoader*, unsigned long identifier, const ResourceResponse&);
    static void continueWithPolicyDownloadImpl(Frame*, DocumentLoader*, unsigned long identifier, const ResourceResponse&);
    static void continueWithPolicyIgnoreImpl(Frame*, DocumentLoader*, unsigned long identifier, const ResourceResponse&);
    static void didReceiveDataImpl(InstrumentingAgents*, unsigned long identifier, const char* data, int dataLength, int encodedDataLength);
    static void didFinishLoadingImpl(InstrumentingAgents*, unsigned long identifier, DocumentLoader*, double finishTime);
    static void didFailLoadingImpl(InstrumentingAgents*, unsigned long identifier, DocumentLoader*, const ResourceError&);
    static void documentThreadableLoaderStartedLoadingForClientImpl(InstrumentingAgents*, unsigned long identifier, ThreadableLoaderClient*);
    static void willLoadXHRImpl(InstrumentingAgents*, ThreadableLoaderClient*, const String&, const KURL&, bool, PassRefPtr<FormData>, const HTTPHeaderMap&, bool);
    static void didFailXHRLoadingImpl(InstrumentingAgents*, ThreadableLoaderClient*);
    static void didFinishXHRLoadingImpl(InstrumentingAgents*, ThreadableLoaderClient*, unsigned long identifier, const String& sourceString, const String& url, const String& sendURL, unsigned sendLineNumber);
    static void didReceiveXHRResponseImpl(InstrumentingAgents*, unsigned long identifier);
    static void willLoadXHRSynchronouslyImpl(InstrumentingAgents*);
    static void didLoadXHRSynchronouslyImpl(InstrumentingAgents*);
    static void scriptImportedImpl(InstrumentingAgents*, unsigned long identifier, const String& sourceString);
    static void scriptExecutionBlockedByCSPImpl(InstrumentingAgents*, const String& directiveText);
    static void didReceiveScriptResponseImpl(InstrumentingAgents*, unsigned long identifier);
    static void domContentLoadedEventFiredImpl(InstrumentingAgents*, Frame*);
    static void loadEventFiredImpl(InstrumentingAgents*, Frame*);
    static void frameDetachedFromParentImpl(InstrumentingAgents*, Frame*);
    static void didCommitLoadImpl(InstrumentingAgents*, Page*, DocumentLoader*);
    static void frameDocumentUpdatedImpl(InstrumentingAgents*, Frame*);
    static void loaderDetachedFromFrameImpl(InstrumentingAgents*, DocumentLoader*);
    static void frameStartedLoadingImpl(InstrumentingAgents*, Frame*);
    static void frameStoppedLoadingImpl(InstrumentingAgents*, Frame*);
    static void frameScheduledNavigationImpl(InstrumentingAgents*, Frame*, double delay);
    static void frameClearedScheduledNavigationImpl(InstrumentingAgents*, Frame*);
    static InspectorInstrumentationCookie willRunJavaScriptDialogImpl(InstrumentingAgents*, const String& message);
    static void didRunJavaScriptDialogImpl(const InspectorInstrumentationCookie&);
    static void willDestroyCachedResourceImpl(CachedResource*);

    static InspectorInstrumentationCookie willWriteHTMLImpl(InstrumentingAgents*, unsigned startLine, Frame*);
    static void didWriteHTMLImpl(const InspectorInstrumentationCookie&, unsigned endLine);

    static void addMessageToConsoleImpl(InstrumentingAgents*, MessageSource, MessageType, MessageLevel, const String& message, ScriptState*, ScriptArguments*, unsigned long requestIdentifier);
    static void addMessageToConsoleImpl(InstrumentingAgents*, MessageSource, MessageType, MessageLevel, const String& message, const String& scriptId, unsigned lineNumber, unsigned columnNumber, ScriptState*, unsigned long requestIdentifier);

    // FIXME: Remove once we no longer generate stacks outside of Inspector.
    static void addMessageToConsoleImpl(InstrumentingAgents*, MessageSource, MessageType, MessageLevel, const String& message, ScriptCallStack*, unsigned long requestIdentifier);

    static void consoleCountImpl(InstrumentingAgents*, ScriptState*, PassRefPtr<ScriptArguments>);
    static void startConsoleTimingImpl(InstrumentingAgents*, Frame*, const String& title);
    static void stopConsoleTimingImpl(InstrumentingAgents*, Frame*, const String& title, PassRefPtr<ScriptCallStack>);
    static void consoleTimeStampImpl(InstrumentingAgents*, Frame*, PassRefPtr<ScriptArguments>);

    static void didRequestAnimationFrameImpl(InstrumentingAgents*, int callbackId, Frame*);
    static void didCancelAnimationFrameImpl(InstrumentingAgents*, int callbackId, Frame*);
    static InspectorInstrumentationCookie willFireAnimationFrameImpl(InstrumentingAgents*, int callbackId, Frame*);
    static void didFireAnimationFrameImpl(const InspectorInstrumentationCookie&);

#if ENABLE(JAVASCRIPT_DEBUGGER)
    static void addStartProfilingMessageToConsoleImpl(InstrumentingAgents*, const String& title, unsigned lineNumber, unsigned columnNumber, const String& sourceURL);
    static void addProfileImpl(InstrumentingAgents*, RefPtr<ScriptProfile>, PassRefPtr<ScriptCallStack>);
    static String getCurrentUserInitiatedProfileNameImpl(InstrumentingAgents*, bool incrementProfileNumber);
    static bool profilerEnabledImpl(InstrumentingAgents*);
#endif

#if ENABLE(SQL_DATABASE)
    static void didOpenDatabaseImpl(InstrumentingAgents*, PassRefPtr<Database>, const String& domain, const String& name, const String& version);
#endif

    static void didDispatchDOMStorageEventImpl(InstrumentingAgents*, const String& key, const String& oldValue, const String& newValue, StorageType, SecurityOrigin*, Page*);

#if ENABLE(WORKERS)
    static bool shouldPauseDedicatedWorkerOnStartImpl(InstrumentingAgents*);
    static void didStartWorkerGlobalScopeImpl(InstrumentingAgents*, WorkerGlobalScopeProxy*, const KURL&);
    static void workerGlobalScopeTerminatedImpl(InstrumentingAgents*, WorkerGlobalScopeProxy*);
#endif

#if ENABLE(WEB_SOCKETS)
    static void didCreateWebSocketImpl(InstrumentingAgents*, unsigned long identifier, const KURL& requestURL, const KURL& documentURL, const String& protocol, Document*);
    static void willSendWebSocketHandshakeRequestImpl(InstrumentingAgents*, unsigned long identifier, const ResourceRequest&, Document*);
    static void didReceiveWebSocketHandshakeResponseImpl(InstrumentingAgents*, unsigned long identifier, const ResourceResponse&, Document*);
    static void didCloseWebSocketImpl(InstrumentingAgents*, unsigned long identifier, Document*);
    static void didReceiveWebSocketFrameImpl(InstrumentingAgents*, unsigned long identifier, const WebSocketFrame&);
    static void didSendWebSocketFrameImpl(InstrumentingAgents*, unsigned long identifier, const WebSocketFrame&);
    static void didReceiveWebSocketFrameErrorImpl(InstrumentingAgents*, unsigned long identifier, const String&);
#endif

    static void networkStateChangedImpl(InstrumentingAgents*);
    static void updateApplicationCacheStatusImpl(InstrumentingAgents*, Frame*);

    static InstrumentingAgents* instrumentingAgentsForPage(Page*);
    static InstrumentingAgents* instrumentingAgentsForFrame(Frame*);
    static InstrumentingAgents* instrumentingAgentsForContext(ScriptExecutionContext*);
    static InstrumentingAgents* instrumentingAgentsForDocument(Document*);
    static InstrumentingAgents* instrumentingAgentsForRenderer(RenderObject*);

#if ENABLE(WORKERS)
    static InstrumentingAgents* instrumentingAgentsForWorkerGlobalScope(WorkerGlobalScope*);
    static InstrumentingAgents* instrumentingAgentsForNonDocumentContext(ScriptExecutionContext*);
#endif

    static bool collectingHTMLParseErrors(InstrumentingAgents*);
    static void pauseOnNativeEventIfNeeded(InstrumentingAgents*, bool isDOMEvent, const String& eventName, bool synchronous);
    static void cancelPauseOnNativeEvent(InstrumentingAgents*);
    static InspectorTimelineAgent* retrieveTimelineAgent(const InspectorInstrumentationCookie&);

#if ENABLE(GEOLOCATION)
    static GeolocationPosition* overrideGeolocationPositionImpl(InstrumentingAgents*, GeolocationPosition*);
#endif

    static DeviceOrientationData* overrideDeviceOrientationImpl(InstrumentingAgents*, DeviceOrientationData*);

#if USE(ACCELERATED_COMPOSITING)
    static void layerTreeDidChangeImpl(InstrumentingAgents*);
    static void renderLayerDestroyedImpl(InstrumentingAgents*, const RenderLayer*);
    static void pseudoElementDestroyedImpl(InstrumentingAgents*, PseudoElement*);
#endif

    static int s_frontendCounter;
#endif
};

namespace InstrumentationEvents {
extern const char PaintLayer[];
extern const char RasterTask[];
extern const char Paint[];
extern const char Layer[];
extern const char BeginFrame[];
};

namespace InstrumentationEventArguments {
extern const char LayerId[];
extern const char PageId[];
};

inline void InspectorInstrumentation::didClearWindowObjectInWorld(Frame* frame, DOMWrapperWorld* world)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        didClearWindowObjectInWorldImpl(instrumentingAgents, frame, world);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(world);
#endif
}

inline bool InspectorInstrumentation::isDebuggerPaused(Frame* frame)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(false);
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        return isDebuggerPausedImpl(instrumentingAgents);
#else
    UNUSED_PARAM(frame);
#endif
    return false;
}

inline void InspectorInstrumentation::willInsertDOMNode(Document* document, Node* parent)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        willInsertDOMNodeImpl(instrumentingAgents, parent);
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(parent);
#endif
}

inline void InspectorInstrumentation::didInsertDOMNode(Document* document, Node* node)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        didInsertDOMNodeImpl(instrumentingAgents, node);
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(node);
#endif
}

inline void InspectorInstrumentation::willRemoveDOMNode(Document* document, Node* node)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document)) {
        willRemoveDOMNodeImpl(instrumentingAgents, node);
        didRemoveDOMNodeImpl(instrumentingAgents, node);
    }
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(node);
#endif
}

inline void InspectorInstrumentation::willModifyDOMAttr(Document* document, Element* element, const AtomicString& oldValue, const AtomicString& newValue)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        willModifyDOMAttrImpl(instrumentingAgents, element, oldValue, newValue);
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(element);
    UNUSED_PARAM(oldValue);
    UNUSED_PARAM(newValue);
#endif
}

inline void InspectorInstrumentation::didModifyDOMAttr(Document* document, Element* element, const AtomicString& name, const AtomicString& value)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        didModifyDOMAttrImpl(instrumentingAgents, element, name, value);
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(element);
    UNUSED_PARAM(name);
    UNUSED_PARAM(value);
#endif
}

inline void InspectorInstrumentation::didRemoveDOMAttr(Document* document, Element* element, const AtomicString& name)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        didRemoveDOMAttrImpl(instrumentingAgents, element, name);
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(element);
    UNUSED_PARAM(name);
#endif
}

inline void InspectorInstrumentation::didInvalidateStyleAttr(Document* document, Node* node)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        didInvalidateStyleAttrImpl(instrumentingAgents, node);
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(node);
#endif
}

inline void InspectorInstrumentation::frameWindowDiscarded(Frame* frame, DOMWindow* domWindow)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        frameWindowDiscardedImpl(instrumentingAgents, domWindow);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(domWindow);
#endif
}

inline void InspectorInstrumentation::mediaQueryResultChanged(Document* document)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        mediaQueryResultChangedImpl(instrumentingAgents);
#else
    UNUSED_PARAM(document);
#endif
}

inline void InspectorInstrumentation::didPushShadowRoot(Element* host, ShadowRoot* root)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(host->ownerDocument()))
        didPushShadowRootImpl(instrumentingAgents, host, root);
#else
    UNUSED_PARAM(host);
    UNUSED_PARAM(root);
#endif
}

inline void InspectorInstrumentation::willPopShadowRoot(Element* host, ShadowRoot* root)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(host->ownerDocument()))
        willPopShadowRootImpl(instrumentingAgents, host, root);
#else
    UNUSED_PARAM(host);
    UNUSED_PARAM(root);
#endif
}

inline void InspectorInstrumentation::didCreateNamedFlow(Document* document, WebKitNamedFlow* namedFlow)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        didCreateNamedFlowImpl(instrumentingAgents, document, namedFlow);
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(namedFlow);
#endif
}

inline void InspectorInstrumentation::willRemoveNamedFlow(Document* document, WebKitNamedFlow* namedFlow)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        willRemoveNamedFlowImpl(instrumentingAgents, document, namedFlow);
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(namedFlow);
#endif
}

inline void InspectorInstrumentation::didUpdateRegionLayout(Document* document, WebKitNamedFlow* namedFlow)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        didUpdateRegionLayoutImpl(instrumentingAgents, document, namedFlow);
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(namedFlow);
#endif
}

inline void InspectorInstrumentation::didChangeRegionOverset(Document* document, WebKitNamedFlow* namedFlow)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        didChangeRegionOversetImpl(instrumentingAgents, document, namedFlow);
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(namedFlow);
#endif
}

inline void InspectorInstrumentation::mouseDidMoveOverElement(Page* page, const HitTestResult& result, unsigned modifierFlags)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        mouseDidMoveOverElementImpl(instrumentingAgents, result, modifierFlags);
#else
    UNUSED_PARAM(page);
    UNUSED_PARAM(result);
    UNUSED_PARAM(modifierFlags);
#endif
}

inline bool InspectorInstrumentation::handleTouchEvent(Page* page, Node* node)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(false);
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        return handleTouchEventImpl(instrumentingAgents, node);
#else
    UNUSED_PARAM(page);
    UNUSED_PARAM(node);
#endif
    return false;
}

inline bool InspectorInstrumentation::handleMousePress(Page* page)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(false);
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        return handleMousePressImpl(instrumentingAgents);
#else
    UNUSED_PARAM(page);
#endif
    return false;
}

inline bool InspectorInstrumentation::forcePseudoState(Element* element, CSSSelector::PseudoType pseudoState)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(false);
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(element->document()))
        return forcePseudoStateImpl(instrumentingAgents, element, pseudoState);
#else
    UNUSED_PARAM(element);
    UNUSED_PARAM(pseudoState);
#endif
    return false;
}

inline void InspectorInstrumentation::characterDataModified(Document* document, CharacterData* characterData)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        characterDataModifiedImpl(instrumentingAgents, characterData);
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(characterData);
#endif
}

inline void InspectorInstrumentation::willSendXMLHttpRequest(ScriptExecutionContext* context, const String& url)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(context))
        willSendXMLHttpRequestImpl(instrumentingAgents, url);
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(url);
#endif
}

inline void InspectorInstrumentation::didScheduleResourceRequest(Document* document, const String& url)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        didScheduleResourceRequestImpl(instrumentingAgents, url, document->frame());
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(url);
#endif
}

inline void InspectorInstrumentation::didInstallTimer(ScriptExecutionContext* context, int timerId, int timeout, bool singleShot)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(context))
        didInstallTimerImpl(instrumentingAgents, timerId, timeout, singleShot, context);
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(timerId);
    UNUSED_PARAM(timeout);
    UNUSED_PARAM(singleShot);
#endif
}

inline void InspectorInstrumentation::didRemoveTimer(ScriptExecutionContext* context, int timerId)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(context))
        didRemoveTimerImpl(instrumentingAgents, timerId, context);
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(timerId);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willCallFunction(ScriptExecutionContext* context, const String& scriptName, int scriptLine)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(InspectorInstrumentationCookie());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(context))
        return willCallFunctionImpl(instrumentingAgents, scriptName, scriptLine, context);
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(scriptName);
    UNUSED_PARAM(scriptLine);
#endif
    return InspectorInstrumentationCookie();
}


inline void InspectorInstrumentation::didCallFunction(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (cookie.isValid())
        didCallFunctionImpl(cookie);
#else
    UNUSED_PARAM(cookie);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willDispatchXHRReadyStateChangeEvent(ScriptExecutionContext* context, XMLHttpRequest* request)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(InspectorInstrumentationCookie());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(context))
        return willDispatchXHRReadyStateChangeEventImpl(instrumentingAgents, request, context);
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(request);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didDispatchXHRReadyStateChangeEvent(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (cookie.isValid())
        didDispatchXHRReadyStateChangeEventImpl(cookie);
#else
    UNUSED_PARAM(cookie);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willDispatchEvent(Document* document, const Event& event, DOMWindow* window, Node* node, const EventPath& eventPath)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(InspectorInstrumentationCookie());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        return willDispatchEventImpl(instrumentingAgents, event, window, node, eventPath, document);
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(event);
    UNUSED_PARAM(window);
    UNUSED_PARAM(node);
    UNUSED_PARAM(eventPath);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didDispatchEvent(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (cookie.isValid())
        didDispatchEventImpl(cookie);
#else
    UNUSED_PARAM(cookie);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willHandleEvent(ScriptExecutionContext* context, Event* event)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(InspectorInstrumentationCookie());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(context))
        return willHandleEventImpl(instrumentingAgents, event);
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(event);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didHandleEvent(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (cookie.isValid())
        didHandleEventImpl(cookie);
#else
    UNUSED_PARAM(cookie);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willDispatchEventOnWindow(Frame* frame, const Event& event, DOMWindow* window)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(InspectorInstrumentationCookie());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        return willDispatchEventOnWindowImpl(instrumentingAgents, event, window);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(event);
    UNUSED_PARAM(window);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didDispatchEventOnWindow(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (cookie.isValid())
        didDispatchEventOnWindowImpl(cookie);
#else
    UNUSED_PARAM(cookie);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willEvaluateScript(Frame* frame, const String& url, int lineNumber)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(InspectorInstrumentationCookie());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        return willEvaluateScriptImpl(instrumentingAgents, url, lineNumber, frame);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(url);
    UNUSED_PARAM(lineNumber);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didEvaluateScript(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (cookie.isValid())
        didEvaluateScriptImpl(cookie);
#else
    UNUSED_PARAM(cookie);
#endif
}

inline void InspectorInstrumentation::scriptsEnabled(Page* page, bool isEnabled)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        return scriptsEnabledImpl(instrumentingAgents, isEnabled);
#else
    UNUSED_PARAM(page);
    UNUSED_PARAM(isEnabled);
#endif
}

inline void InspectorInstrumentation::didCreateIsolatedContext(Frame* frame, ScriptState* scriptState, SecurityOrigin* origin)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        return didCreateIsolatedContextImpl(instrumentingAgents, frame, scriptState, origin);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(scriptState);
    UNUSED_PARAM(origin);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willFireTimer(ScriptExecutionContext* context, int timerId)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(InspectorInstrumentationCookie());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(context))
        return willFireTimerImpl(instrumentingAgents, timerId, context);
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(timerId);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didFireTimer(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (cookie.isValid())
        didFireTimerImpl(cookie);
#else
    UNUSED_PARAM(cookie);
#endif
}

inline void InspectorInstrumentation::didInvalidateLayout(Frame* frame)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        didInvalidateLayoutImpl(instrumentingAgents, frame);
#else
    UNUSED_PARAM(frame);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willLayout(Frame* frame)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(InspectorInstrumentationCookie());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        return willLayoutImpl(instrumentingAgents, frame);
#else
    UNUSED_PARAM(frame);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didLayout(const InspectorInstrumentationCookie& cookie, RenderObject* root)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (cookie.isValid())
        didLayoutImpl(cookie, root);
#else
    UNUSED_PARAM(cookie);
    UNUSED_PARAM(root);
#endif
}

inline void InspectorInstrumentation::didScroll(Page* page)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        didScrollImpl(instrumentingAgents);
#else
    UNUSED_PARAM(page);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willDispatchXHRLoadEvent(ScriptExecutionContext* context, XMLHttpRequest* request)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(InspectorInstrumentationCookie());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(context))
        return willDispatchXHRLoadEventImpl(instrumentingAgents, request, context);
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(request);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didDispatchXHRLoadEvent(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (cookie.isValid())
        didDispatchXHRLoadEventImpl(cookie);
#else
    UNUSED_PARAM(cookie);
#endif
}

inline void InspectorInstrumentation::willPaint(RenderObject* renderer)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForRenderer(renderer))
        return willPaintImpl(instrumentingAgents, renderer);
#else
    UNUSED_PARAM(renderer);
#endif
}

inline void InspectorInstrumentation::didPaint(RenderObject* renderer, GraphicsContext* context, const LayoutRect& rect)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForRenderer(renderer))
        didPaintImpl(instrumentingAgents, renderer, context, rect);
#else
    UNUSED_PARAM(renderer);
    UNUSED_PARAM(context);
    UNUSED_PARAM(rect);
#endif
}

inline void InspectorInstrumentation::willScrollLayer(Frame* frame)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        willScrollLayerImpl(instrumentingAgents, frame);
#else
    UNUSED_PARAM(frame);
#endif
}

inline void InspectorInstrumentation::didScrollLayer(Frame* frame)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        didScrollLayerImpl(instrumentingAgents);
#else
    UNUSED_PARAM(frame);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willRecalculateStyle(Document* document)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(InspectorInstrumentationCookie());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        return willRecalculateStyleImpl(instrumentingAgents, document->frame());
#else
    UNUSED_PARAM(document);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didRecalculateStyle(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (cookie.isValid())
        didRecalculateStyleImpl(cookie);
#else
    UNUSED_PARAM(cookie);
#endif
}

inline void InspectorInstrumentation::didScheduleStyleRecalculation(Document* document)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        didScheduleStyleRecalculationImpl(instrumentingAgents, document);
#else
    UNUSED_PARAM(document);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willMatchRule(Document* document, StyleRule* rule, InspectorCSSOMWrappers& inspectorCSSOMWrappers, DocumentStyleSheetCollection* styleSheetCollection)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(InspectorInstrumentationCookie());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        return willMatchRuleImpl(instrumentingAgents, rule, inspectorCSSOMWrappers, styleSheetCollection);
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(rule);
    UNUSED_PARAM(inspectorCSSOMWrappers);
    UNUSED_PARAM(styleSheetCollection);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didMatchRule(const InspectorInstrumentationCookie& cookie, bool matched)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (cookie.isValid())
        didMatchRuleImpl(cookie, matched);
#else
    UNUSED_PARAM(cookie);
    UNUSED_PARAM(matched);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willProcessRule(Document* document, StyleRule* rule, StyleResolver* styleResolver)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(InspectorInstrumentationCookie());
    if (!rule)
        return InspectorInstrumentationCookie();
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        return willProcessRuleImpl(instrumentingAgents, rule, styleResolver);
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(rule);
    UNUSED_PARAM(styleResolver);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didProcessRule(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (cookie.isValid())
        didProcessRuleImpl(cookie);
#else
    UNUSED_PARAM(cookie);
#endif
}

inline void InspectorInstrumentation::applyUserAgentOverride(Frame* frame, String* userAgent)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        applyUserAgentOverrideImpl(instrumentingAgents, userAgent);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(userAgent);
#endif
}

inline void InspectorInstrumentation::applyScreenWidthOverride(Frame* frame, long* width)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        applyScreenWidthOverrideImpl(instrumentingAgents, width);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(width);
#endif
}

inline void InspectorInstrumentation::applyScreenHeightOverride(Frame* frame, long* height)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        applyScreenHeightOverrideImpl(instrumentingAgents, height);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(height);
#endif
}

inline bool InspectorInstrumentation::shouldApplyScreenWidthOverride(Frame* frame)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(false);
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        return shouldApplyScreenWidthOverrideImpl(instrumentingAgents);
#else
    UNUSED_PARAM(frame);
#endif
    return false;
}

inline void InspectorInstrumentation::applyEmulatedMedia(Frame* frame, String* media)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        applyEmulatedMediaImpl(instrumentingAgents, media);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(media);
#endif
}

inline bool InspectorInstrumentation::shouldApplyScreenHeightOverride(Frame* frame)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(false);
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        return shouldApplyScreenHeightOverrideImpl(instrumentingAgents);
#else
    UNUSED_PARAM(frame);
#endif
    return false;
}

inline void InspectorInstrumentation::willSendRequest(Frame* frame, unsigned long identifier, DocumentLoader* loader, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        willSendRequestImpl(instrumentingAgents, identifier, loader, request, redirectResponse);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(identifier);
    UNUSED_PARAM(loader);
    UNUSED_PARAM(request);
    UNUSED_PARAM(redirectResponse);
#endif
}

inline void InspectorInstrumentation::continueAfterPingLoader(Frame* frame, unsigned long identifier, DocumentLoader* loader, ResourceRequest& request, const ResourceResponse& response)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        InspectorInstrumentation::continueAfterPingLoaderImpl(instrumentingAgents, identifier, loader, request, response);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(identifier);
    UNUSED_PARAM(loader);
    UNUSED_PARAM(request);
    UNUSED_PARAM(response);
#endif
}

inline void InspectorInstrumentation::markResourceAsCached(Page* page, unsigned long identifier)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        markResourceAsCachedImpl(instrumentingAgents, identifier);
#else
    UNUSED_PARAM(page);
    UNUSED_PARAM(identifier);
#endif
}

inline void InspectorInstrumentation::didLoadResourceFromMemoryCache(Page* page, DocumentLoader* loader, CachedResource* resource)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        didLoadResourceFromMemoryCacheImpl(instrumentingAgents, loader, resource);
#else
    UNUSED_PARAM(page);
    UNUSED_PARAM(loader);
    UNUSED_PARAM(resource);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willReceiveResourceData(Frame* frame, unsigned long identifier, int length)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(InspectorInstrumentationCookie());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        return willReceiveResourceDataImpl(instrumentingAgents, identifier, frame, length);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(identifier);
    UNUSED_PARAM(length);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didReceiveResourceData(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (cookie.isValid())
        didReceiveResourceDataImpl(cookie);
#else
    UNUSED_PARAM(cookie);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willReceiveResourceResponse(Frame* frame, unsigned long identifier, const ResourceResponse& response)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        return willReceiveResourceResponseImpl(instrumentingAgents, identifier, response, frame);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(identifier);
    UNUSED_PARAM(response);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didReceiveResourceResponse(const InspectorInstrumentationCookie& cookie, unsigned long identifier, DocumentLoader* loader, const ResourceResponse& response, ResourceLoader* resourceLoader)
{
#if ENABLE(INSPECTOR)
    // Call this unconditionally so that we're able to log to console with no front-end attached.
    if (cookie.isValid())
        didReceiveResourceResponseImpl(cookie, identifier, loader, response, resourceLoader);
#else
    UNUSED_PARAM(cookie);
    UNUSED_PARAM(identifier);
    UNUSED_PARAM(loader);
    UNUSED_PARAM(response);
    UNUSED_PARAM(resourceLoader);
#endif
}

inline void InspectorInstrumentation::continueAfterXFrameOptionsDenied(Frame* frame, DocumentLoader* loader, unsigned long identifier, const ResourceResponse& r)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    InspectorInstrumentation::continueAfterXFrameOptionsDeniedImpl(frame, loader, identifier, r);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(loader);
    UNUSED_PARAM(identifier);
    UNUSED_PARAM(r);
#endif
}

inline void InspectorInstrumentation::continueWithPolicyDownload(Frame* frame, DocumentLoader* loader, unsigned long identifier, const ResourceResponse& r)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    InspectorInstrumentation::continueWithPolicyDownloadImpl(frame, loader, identifier, r);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(loader);
    UNUSED_PARAM(identifier);
    UNUSED_PARAM(r);
#endif
}

inline void InspectorInstrumentation::continueWithPolicyIgnore(Frame* frame, DocumentLoader* loader, unsigned long identifier, const ResourceResponse& r)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    InspectorInstrumentation::continueWithPolicyIgnoreImpl(frame, loader, identifier, r);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(loader);
    UNUSED_PARAM(identifier);
    UNUSED_PARAM(r);
#endif
}

inline void InspectorInstrumentation::didReceiveData(Frame* frame, unsigned long identifier, const char* data, int dataLength, int encodedDataLength)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        didReceiveDataImpl(instrumentingAgents, identifier, data, dataLength, encodedDataLength);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(identifier);
    UNUSED_PARAM(data);
    UNUSED_PARAM(dataLength);
    UNUSED_PARAM(encodedDataLength);
#endif
}

inline void InspectorInstrumentation::didFinishLoading(Frame* frame, DocumentLoader* loader, unsigned long identifier, double finishTime)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        didFinishLoadingImpl(instrumentingAgents, identifier, loader, finishTime);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(loader);
    UNUSED_PARAM(identifier);
    UNUSED_PARAM(finishTime);
#endif
}

inline void InspectorInstrumentation::didFailLoading(Frame* frame, DocumentLoader* loader, unsigned long identifier, const ResourceError& error)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        didFailLoadingImpl(instrumentingAgents, identifier, loader, error);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(loader);
    UNUSED_PARAM(identifier);
    UNUSED_PARAM(error);
#endif
}

inline void InspectorInstrumentation::documentThreadableLoaderStartedLoadingForClient(ScriptExecutionContext* context, unsigned long identifier, ThreadableLoaderClient* client)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(context))
        documentThreadableLoaderStartedLoadingForClientImpl(instrumentingAgents, identifier, client);
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(identifier);
    UNUSED_PARAM(client);
#endif
}

inline void InspectorInstrumentation::willLoadXHR(ScriptExecutionContext* context, ThreadableLoaderClient* client, const String& method, const KURL& url, bool async, PassRefPtr<FormData> formData, const HTTPHeaderMap& headers, bool includeCredentials)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(context))
        willLoadXHRImpl(instrumentingAgents, client, method, url, async, formData, headers, includeCredentials);
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(client);
    UNUSED_PARAM(method);
    UNUSED_PARAM(url);
    UNUSED_PARAM(async);
    UNUSED_PARAM(formData);
    UNUSED_PARAM(headers);
    UNUSED_PARAM(includeCredentials);
#endif
}

inline void InspectorInstrumentation::didFailXHRLoading(ScriptExecutionContext* context, ThreadableLoaderClient* client)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(context))
        didFailXHRLoadingImpl(instrumentingAgents, client);
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(client);
#endif
}


inline void InspectorInstrumentation::didFinishXHRLoading(ScriptExecutionContext* context, ThreadableLoaderClient* client, unsigned long identifier, const String& sourceString, const String& url, const String& sendURL, unsigned sendLineNumber)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(context))
        didFinishXHRLoadingImpl(instrumentingAgents, client, identifier, sourceString, url, sendURL, sendLineNumber);
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(client);
    UNUSED_PARAM(identifier);
    UNUSED_PARAM(sourceString);
    UNUSED_PARAM(url);
    UNUSED_PARAM(sendURL);
    UNUSED_PARAM(sendLineNumber);
#endif
}

inline void InspectorInstrumentation::didReceiveXHRResponse(ScriptExecutionContext* context, unsigned long identifier)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(context))
        didReceiveXHRResponseImpl(instrumentingAgents, identifier);
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(identifier);
#endif
}

inline void InspectorInstrumentation::willLoadXHRSynchronously(ScriptExecutionContext* context)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(context))
        willLoadXHRSynchronouslyImpl(instrumentingAgents);
#else
    UNUSED_PARAM(context);
#endif
}

inline void InspectorInstrumentation::didLoadXHRSynchronously(ScriptExecutionContext* context)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(context))
        didLoadXHRSynchronouslyImpl(instrumentingAgents);
#else
    UNUSED_PARAM(context);
#endif
}

inline void InspectorInstrumentation::scriptImported(ScriptExecutionContext* context, unsigned long identifier, const String& sourceString)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(context))
        scriptImportedImpl(instrumentingAgents, identifier, sourceString);
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(identifier);
    UNUSED_PARAM(sourceString);
#endif
}

inline void InspectorInstrumentation::scriptExecutionBlockedByCSP(ScriptExecutionContext* context, const String& directiveText)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(context))
        scriptExecutionBlockedByCSPImpl(instrumentingAgents, directiveText);
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(directiveText);
#endif
}

inline void InspectorInstrumentation::didReceiveScriptResponse(ScriptExecutionContext* context, unsigned long identifier)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(context))
        didReceiveScriptResponseImpl(instrumentingAgents, identifier);
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(identifier);
#endif
}

inline void InspectorInstrumentation::domContentLoadedEventFired(Frame* frame)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        domContentLoadedEventFiredImpl(instrumentingAgents, frame);
#else
    UNUSED_PARAM(frame);
#endif
}

inline void InspectorInstrumentation::loadEventFired(Frame* frame)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        loadEventFiredImpl(instrumentingAgents, frame);
#else
    UNUSED_PARAM(frame);
#endif
}

inline void InspectorInstrumentation::frameDetachedFromParent(Frame* frame)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        frameDetachedFromParentImpl(instrumentingAgents, frame);
#else
    UNUSED_PARAM(frame);
#endif
}

inline void InspectorInstrumentation::didCommitLoad(Frame* frame, DocumentLoader* loader)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        didCommitLoadImpl(instrumentingAgents, frame->page(), loader);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(loader);
#endif
}

inline void InspectorInstrumentation::frameDocumentUpdated(Frame* frame)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        frameDocumentUpdatedImpl(instrumentingAgents, frame);
#else
    UNUSED_PARAM(frame);
#endif
}

inline void InspectorInstrumentation::loaderDetachedFromFrame(Frame* frame, DocumentLoader* loader)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        loaderDetachedFromFrameImpl(instrumentingAgents, loader);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(loader);
#endif
}

inline void InspectorInstrumentation::frameStartedLoading(Frame* frame)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        frameStartedLoadingImpl(instrumentingAgents, frame);
#else
    UNUSED_PARAM(frame);
#endif
}

inline void InspectorInstrumentation::frameStoppedLoading(Frame* frame)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        frameStoppedLoadingImpl(instrumentingAgents, frame);
#else
    UNUSED_PARAM(frame);
#endif
}

inline void InspectorInstrumentation::frameScheduledNavigation(Frame* frame, double delay)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        frameScheduledNavigationImpl(instrumentingAgents, frame, delay);
#else
    UNUSED_PARAM(frame);
    UNUSED_PARAM(delay);
#endif
}

inline void InspectorInstrumentation::frameClearedScheduledNavigation(Frame* frame)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        frameClearedScheduledNavigationImpl(instrumentingAgents, frame);
#else
    UNUSED_PARAM(frame);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willRunJavaScriptDialog(Page* page, const String& message)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(InspectorInstrumentationCookie());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        return willRunJavaScriptDialogImpl(instrumentingAgents, message);
#else
    UNUSED_PARAM(page);
    UNUSED_PARAM(message);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didRunJavaScriptDialog(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (cookie.isValid())
        didRunJavaScriptDialogImpl(cookie);
#else
    UNUSED_PARAM(cookie);
#endif
}

inline void InspectorInstrumentation::willDestroyCachedResource(CachedResource* cachedResource)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    willDestroyCachedResourceImpl(cachedResource);
#else
    UNUSED_PARAM(cachedResource);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willWriteHTML(Document* document, unsigned startLine)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(InspectorInstrumentationCookie());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        return willWriteHTMLImpl(instrumentingAgents, startLine, document->frame());
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(startLine);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didWriteHTML(const InspectorInstrumentationCookie& cookie, unsigned endLine)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (cookie.isValid())
        didWriteHTMLImpl(cookie, endLine);
#else
    UNUSED_PARAM(cookie);
    UNUSED_PARAM(endLine);
#endif
}

inline void InspectorInstrumentation::didDispatchDOMStorageEvent(const String& key, const String& oldValue, const String& newValue, StorageType storageType, SecurityOrigin* securityOrigin, Page* page)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        didDispatchDOMStorageEventImpl(instrumentingAgents, key, oldValue, newValue, storageType, securityOrigin, page);
#else
    UNUSED_PARAM(key);
    UNUSED_PARAM(oldValue);
    UNUSED_PARAM(newValue);
    UNUSED_PARAM(storageType);
    UNUSED_PARAM(securityOrigin);
    UNUSED_PARAM(page);
#endif // ENABLE(INSPECTOR)
}

#if ENABLE(WORKERS)
inline bool InspectorInstrumentation::shouldPauseDedicatedWorkerOnStart(ScriptExecutionContext* context)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(false);
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(context))
        return shouldPauseDedicatedWorkerOnStartImpl(instrumentingAgents);
#else
    UNUSED_PARAM(context);
#endif
    return false;
}

inline void InspectorInstrumentation::didStartWorkerGlobalScope(ScriptExecutionContext* context, WorkerGlobalScopeProxy* proxy, const KURL& url)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(context))
        didStartWorkerGlobalScopeImpl(instrumentingAgents, proxy, url);
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(proxy);
    UNUSED_PARAM(url);
#endif
}

inline void InspectorInstrumentation::workerGlobalScopeTerminated(ScriptExecutionContext* context, WorkerGlobalScopeProxy* proxy)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(context))
        workerGlobalScopeTerminatedImpl(instrumentingAgents, proxy);
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(proxy);
#endif
}

#endif


#if ENABLE(WEB_SOCKETS)
inline void InspectorInstrumentation::didCreateWebSocket(Document* document, unsigned long identifier, const KURL& requestURL, const KURL& documentURL, const String& protocol)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        didCreateWebSocketImpl(instrumentingAgents, identifier, requestURL, documentURL, protocol, document);
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(identifier);
    UNUSED_PARAM(requestURL);
    UNUSED_PARAM(documentURL);
    UNUSED_PARAM(protocol);
#endif
}

inline void InspectorInstrumentation::willSendWebSocketHandshakeRequest(Document* document, unsigned long identifier, const ResourceRequest& request)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        willSendWebSocketHandshakeRequestImpl(instrumentingAgents, identifier, request, document);
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(identifier);
    UNUSED_PARAM(request);
#endif
}

inline void InspectorInstrumentation::didReceiveWebSocketHandshakeResponse(Document* document, unsigned long identifier, const ResourceResponse& response)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        didReceiveWebSocketHandshakeResponseImpl(instrumentingAgents, identifier, response, document);
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(identifier);
    UNUSED_PARAM(response);
#endif
}

inline void InspectorInstrumentation::didCloseWebSocket(Document* document, unsigned long identifier)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        didCloseWebSocketImpl(instrumentingAgents, identifier, document);
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(identifier);
#endif
}
inline void InspectorInstrumentation::didReceiveWebSocketFrame(Document* document, unsigned long identifier, const WebSocketFrame& frame)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        didReceiveWebSocketFrameImpl(instrumentingAgents, identifier, frame);
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(identifier);
    UNUSED_PARAM(frame);
#endif
}
inline void InspectorInstrumentation::didReceiveWebSocketFrameError(Document* document, unsigned long identifier, const String& errorMessage)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        didReceiveWebSocketFrameErrorImpl(instrumentingAgents, identifier, errorMessage);
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(identifier);
    UNUSED_PARAM(errorMessage);
#endif
}
inline void InspectorInstrumentation::didSendWebSocketFrame(Document* document, unsigned long identifier, const WebSocketFrame& frame)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        didSendWebSocketFrameImpl(instrumentingAgents, identifier, frame);
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(identifier);
    UNUSED_PARAM(frame);
#endif
}
#endif

inline void InspectorInstrumentation::networkStateChanged(Page* page)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        networkStateChangedImpl(instrumentingAgents);
#else
    UNUSED_PARAM(page);
#endif
}

inline void InspectorInstrumentation::updateApplicationCacheStatus(Frame* frame)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        updateApplicationCacheStatusImpl(instrumentingAgents, frame);
#else
    UNUSED_PARAM(frame);
#endif
}

inline void InspectorInstrumentation::didRequestAnimationFrame(Document* document, int callbackId)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        didRequestAnimationFrameImpl(instrumentingAgents, callbackId, document->frame());
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(callbackId);
#endif
}

inline void InspectorInstrumentation::didCancelAnimationFrame(Document* document, int callbackId)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        didCancelAnimationFrameImpl(instrumentingAgents, callbackId, document->frame());
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(callbackId);
#endif
}

inline InspectorInstrumentationCookie InspectorInstrumentation::willFireAnimationFrame(Document* document, int callbackId)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document))
        return willFireAnimationFrameImpl(instrumentingAgents, callbackId, document->frame());
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(callbackId);
#endif
    return InspectorInstrumentationCookie();
}

inline void InspectorInstrumentation::didFireAnimationFrame(const InspectorInstrumentationCookie& cookie)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (cookie.isValid())
        didFireAnimationFrameImpl(cookie);
#else
    UNUSED_PARAM(cookie);
#endif
}

#if ENABLE(GEOLOCATION)
inline GeolocationPosition* InspectorInstrumentation::overrideGeolocationPosition(Page* page, GeolocationPosition* position)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(position);
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        return overrideGeolocationPositionImpl(instrumentingAgents, position);
#else
    UNUSED_PARAM(page);
#endif
    return position;
}
#endif

inline DeviceOrientationData* InspectorInstrumentation::overrideDeviceOrientation(Page* page, DeviceOrientationData* deviceOrientation)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(deviceOrientation);
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        return overrideDeviceOrientationImpl(instrumentingAgents, deviceOrientation);
#else
    UNUSED_PARAM(page);
#endif
    return deviceOrientation;
}

#if USE(ACCELERATED_COMPOSITING)
inline void InspectorInstrumentation::layerTreeDidChange(Page* page)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        layerTreeDidChangeImpl(instrumentingAgents);
#else
    UNUSED_PARAM(page);
#endif
}

inline void InspectorInstrumentation::renderLayerDestroyed(Page* page, const RenderLayer* renderLayer)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        renderLayerDestroyedImpl(instrumentingAgents, renderLayer);
#else
    UNUSED_PARAM(page);
    UNUSED_PARAM(renderLayer);
#endif
}

inline void InspectorInstrumentation::pseudoElementDestroyed(Page* page, PseudoElement* pseudoElement)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        pseudoElementDestroyedImpl(instrumentingAgents, pseudoElement);
#else
    UNUSED_PARAM(page);
    UNUSED_PARAM(pseudoElement);
#endif
}
#endif

#if ENABLE(INSPECTOR)
inline bool InspectorInstrumentation::collectingHTMLParseErrors(Page* page)
{
    FAST_RETURN_IF_NO_FRONTENDS(false);
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        return collectingHTMLParseErrors(instrumentingAgents);
    return false;
}

inline InstrumentingAgents* InspectorInstrumentation::instrumentingAgentsForContext(ScriptExecutionContext* context)
{
    if (!context)
        return 0;
    if (context->isDocument())
        return instrumentingAgentsForPage(toDocument(context)->page());
#if ENABLE(WORKERS)
    return instrumentingAgentsForNonDocumentContext(context);
#else
    return 0;
#endif
}

inline InstrumentingAgents* InspectorInstrumentation::instrumentingAgentsForFrame(Frame* frame)
{
    if (frame)
        return instrumentingAgentsForPage(frame->page());
    return 0;
}

inline InstrumentingAgents* InspectorInstrumentation::instrumentingAgentsForDocument(Document* document)
{
    if (document) {
        Page* page = document->page();
#if ENABLE(TEMPLATE_ELEMENT)
        if (!page && document->templateDocumentHost())
            page = document->templateDocumentHost()->page();
#endif
        return instrumentingAgentsForPage(page);
    }
    return 0;
}
#endif

} // namespace WebCore

#endif // !defined(InspectorInstrumentation_h)
