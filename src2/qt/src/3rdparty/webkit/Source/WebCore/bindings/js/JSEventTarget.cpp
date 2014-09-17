/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "JSEventTarget.h"

#include "DOMWindow.h"
#include "Document.h"
#include "JSDOMWindow.h"
#include "JSDOMWindowShell.h"
#include "JSEventListener.h"
#include "JSMessagePort.h"
#include "JSNode.h"
#if ENABLE(SHARED_WORKERS)

#include "JSSharedWorker.h"
#include "JSSharedWorkerContext.h"
#endif

#include "JSXMLHttpRequest.h"
#include "JSXMLHttpRequestUpload.h"
#include "MessagePort.h"

#if ENABLE(SHARED_WORKERS)
#include "SharedWorker.h"
#include "SharedWorkerContext.h"
#endif

#include "XMLHttpRequest.h"
#include "XMLHttpRequestUpload.h"

#if ENABLE(EVENTSOURCE)
#include "EventSource.h"
#include "JSEventSource.h"
#endif

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
#include "DOMApplicationCache.h"
#include "JSDOMApplicationCache.h"
#endif

#if ENABLE(SVG)
#include "SVGElementInstance.h"
#include "JSSVGElementInstance.h"
#endif

#if ENABLE(WORKERS)
#include "DedicatedWorkerContext.h"
#include "JSDedicatedWorkerContext.h"
#include "JSWorker.h"
#include "Worker.h"
#endif

#if ENABLE(NOTIFICATIONS)
#include "JSNotification.h"
#include "Notification.h"
#endif

#if ENABLE(INDEXED_DATABASE)
#include "IDBRequest.h"
#include "JSIDBRequest.h"
#endif

#if ENABLE(WEB_AUDIO)
#include "AudioContext.h"
#include "JSAudioContext.h"
#include "JSJavaScriptAudioNode.h"
#include "JavaScriptAudioNode.h"
#endif

#if ENABLE(WEB_SOCKETS)
#include "JSWebSocket.h"
#include "WebSocket.h"
#endif

#if ENABLE(BLOB)
#include "JSFileReader.h"
#include "FileReader.h"
#endif

using namespace JSC;

namespace WebCore {

JSValue toJS(ExecState* exec, JSDOMGlobalObject* globalObject, EventTarget* target)
{
    if (!target)
        return jsNull();
    
#if ENABLE(EVENTSOURCE)
    if (EventSource* eventSource = target->toEventSource())
        return toJS(exec, globalObject, eventSource);
#endif

#if ENABLE(SVG)
    // SVGElementInstance supports both toSVGElementInstance and toNode since so much mouse handling code depends on toNode returning a valid node.
    if (SVGElementInstance* instance = target->toSVGElementInstance())
        return toJS(exec, globalObject, instance);
#endif
    
    if (Node* node = target->toNode())
        return toJS(exec, globalObject, node);

    if (DOMWindow* domWindow = target->toDOMWindow())
        return toJS(exec, globalObject, domWindow);

    if (XMLHttpRequest* xhr = target->toXMLHttpRequest())
        return toJS(exec, globalObject, xhr);

    if (XMLHttpRequestUpload* upload = target->toXMLHttpRequestUpload())
        return toJS(exec, globalObject, upload);

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    if (DOMApplicationCache* cache = target->toDOMApplicationCache())
        return toJS(exec, globalObject, cache);
#endif

    if (MessagePort* messagePort = target->toMessagePort())
        return toJS(exec, globalObject, messagePort);

#if ENABLE(WORKERS)
    if (Worker* worker = target->toWorker())
        return toJS(exec, globalObject, worker);

    if (DedicatedWorkerContext* workerContext = target->toDedicatedWorkerContext())
        return toJSDOMGlobalObject(workerContext, exec);
#endif

#if ENABLE(SHARED_WORKERS)
    if (SharedWorker* sharedWorker = target->toSharedWorker())
        return toJS(exec, globalObject, sharedWorker);

    if (SharedWorkerContext* workerContext = target->toSharedWorkerContext())
        return toJSDOMGlobalObject(workerContext, exec);
#endif

#if ENABLE(NOTIFICATIONS)
    if (Notification* notification = target->toNotification())
        return toJS(exec, globalObject, notification);
#endif

#if ENABLE(INDEXED_DATABASE)
    if (IDBDatabase* idbDatabase = target->toIDBDatabase())
        return toJS(exec, globalObject, idbDatabase);

    if (IDBRequest* idbRequest = target->toIDBRequest())
        return toJS(exec, globalObject, idbRequest);

    if (IDBTransaction* idbTransaction = target->toIDBTransaction())
        return toJS(exec, globalObject, idbTransaction);
#endif

#if ENABLE(WEB_AUDIO)
    if (JavaScriptAudioNode* jsAudioNode = target->toJavaScriptAudioNode())
        return toJS(exec, globalObject, jsAudioNode);
    if (AudioContext* audioContext = target->toAudioContext())
        return toJS(exec, globalObject, audioContext);
#endif

#if ENABLE(WEB_SOCKETS)
    if (WebSocket* webSocket = target->toWebSocket())
        return toJS(exec, globalObject, webSocket);
#endif

#if ENABLE(BLOB)
    if (FileReader* fileReader = target->toFileReader())
        return toJS(exec, globalObject, fileReader);
#endif

    ASSERT_NOT_REACHED();
    return jsNull();
}

EventTarget* toEventTarget(JSC::JSValue value)
{
    #define CONVERT_TO_EVENT_TARGET(type) \
        if (value.inherits(&JS##type::s_info)) \
            return static_cast<JS##type*>(asObject(value))->impl();

    CONVERT_TO_EVENT_TARGET(Node)
    CONVERT_TO_EVENT_TARGET(XMLHttpRequest)
    CONVERT_TO_EVENT_TARGET(XMLHttpRequestUpload)
    CONVERT_TO_EVENT_TARGET(MessagePort)

    if (value.inherits(&JSDOMWindowShell::s_info))
        return static_cast<JSDOMWindowShell*>(asObject(value))->impl();

#if ENABLE(EVENTSOURCE)
    CONVERT_TO_EVENT_TARGET(EventSource)
#endif

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    CONVERT_TO_EVENT_TARGET(DOMApplicationCache)
#endif

#if ENABLE(SVG)
    CONVERT_TO_EVENT_TARGET(SVGElementInstance)
#endif

#if ENABLE(WORKERS)
    CONVERT_TO_EVENT_TARGET(Worker)
    CONVERT_TO_EVENT_TARGET(DedicatedWorkerContext)
#endif

#if ENABLE(SHARED_WORKERS)
    CONVERT_TO_EVENT_TARGET(SharedWorker)
    CONVERT_TO_EVENT_TARGET(SharedWorkerContext)
#endif

#if ENABLE(NOTIFICATIONS)
    CONVERT_TO_EVENT_TARGET(Notification)
#endif

#if ENABLE(WEB_SOCKETS)
    CONVERT_TO_EVENT_TARGET(WebSocket)
#endif

    return 0;
}

} // namespace WebCore
