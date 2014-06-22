/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
#include "WebDOMEventTarget.h"

#include "DOMApplicationCache.h"
#include "DOMWindow.h"
#include "DedicatedWorkerGlobalScope.h"
#include "EventSource.h"
#include "MessagePort.h"
#include "Node.h"
#include "Notification.h"
#include "SharedWorker.h"
#include "SharedWorkerGlobalScope.h"
#include "ThreadCheck.h"
#include "WebDOMDOMApplicationCache.h"
#include "WebDOMDOMWindow.h"
#include "WebDOMEventSource.h"
#include "WebDOMMessagePort.h"
#include "WebDOMNode.h"
#include "WebDOMXMLHttpRequest.h"
#include "WebDOMXMLHttpRequestUpload.h"
#include "WebExceptionHandler.h"
#include "WebSocket.h"
#include "Worker.h"
#include "XMLHttpRequest.h"
#include "XMLHttpRequestUpload.h"

#include <wtf/RefPtr.h>

#if ENABLE(WORKERS)
#include "WebDOMDedicatedWorkerGlobalScope.h"
#include "WebDOMWorker.h"
#endif

#if ENABLE(SHARED_WORKERS)
#include "WebDOMSharedWorker.h"
#include "WebDOMSharedWorkerGlobalScope.h"
#endif

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
#include "WebDOMNotification.h"
#endif

#if ENABLE(WEB_SOCKETS)
#include "WebDOMWebSocket.h"
#endif

struct WebDOMEventTarget::WebDOMEventTargetPrivate {
    WebDOMEventTargetPrivate(WebCore::EventTarget* object = 0)
        : impl(object)
    {
    }

    RefPtr<WebCore::EventTarget> impl;
};

WebDOMEventTarget::WebDOMEventTarget()
    : WebDOMObject()
    , m_impl(0)
{
}

WebDOMEventTarget::WebDOMEventTarget(WebCore::EventTarget* impl)
    : WebDOMObject()
    , m_impl(new WebDOMEventTargetPrivate(impl))
{
}

WebDOMEventTarget::WebDOMEventTarget(const WebDOMEventTarget& copy)
    : WebDOMObject()
{
    m_impl = copy.impl() ? new WebDOMEventTargetPrivate(copy.impl()) : 0;
}

WebDOMEventTarget::~WebDOMEventTarget()
{
    delete m_impl;
    m_impl = 0;
}

WebCore::EventTarget* WebDOMEventTarget::impl() const
{
    return m_impl ? m_impl->impl.get() : 0;
}

#define ConvertTo(type) \
WebDOM##type WebDOMEventTarget::to##type() \
{ \
    WebCore::EventTarget* target = impl(); \
    return WebDOM##type(target ? target->to##type() : 0); \
}

ConvertTo(Node)
ConvertTo(DOMWindow)

#if ENABLE(WORKERS) && 0
ConvertTo(Worker)
ConvertTo(DedicatedWorkerGlobalScope)
#endif

#if ENABLE(SHARED_WORKERS)
ConvertTo(SharedWorker)
ConvertTo(SharedWorkerGlobalScope)
#endif

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
ConvertTo(Notification)
#endif

#if ENABLE(WEB_SOCKETS)
ConvertTo(WebSocket)
#endif

WebCore::EventTarget* toWebCore(const WebDOMEventTarget& wrapper)
{
    return wrapper.impl();
}

WebDOMEventTarget toWebKit(WebCore::EventTarget* value)
{
    if (WebCore::Node* node = value->toNode())
        return toWebKit(node);

    if (WebCore::DOMWindow* window = value->toDOMWindow())
        return toWebKit(window);

#if ENABLE(SVG) && 0
    // FIXME: Enable once SVG bindings are generated.
    // SVGElementInstance supports both toSVGElementInstance and toNode since so much mouse handling code depends on toNode returning a valid node.
    if (WebCore::SVGElementInstance* instance = value->toSVGElementInstance())
        return toWebKit(instance);
#endif

#if ENABLE(WORKERS) && 0
    if (WebCore::Worker* worker = value->toWorker())
        return toWebKit(worker);

    if (WebCore::DedicatedWorkerGlobalScope* workerGlobalScope = value->toDedicatedWorkerGlobalScope())
        return toWebKit(workerGlobalScope);
#endif

#if ENABLE(SHARED_WORKERS)
    if (WebCore::SharedWorker* sharedWorker = value->toSharedWorker())
        return toWebKit(sharedWorker);

    if (WebCore::SharedWorkerGlobalScope* workerGlobalScope = value->toSharedWorkerGlobalScope())
        return toWebKit(workerGlobalScope);
#endif

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    if (WebCore::Notification* notification = value->toNotification())
        return toWebKit(notification);
#endif

#if ENABLE(WEB_SOCKETS)
    if (WebCore::WebSocket* webSocket = value->toWebSocket())
        return toWebKit(webSocket);
#endif

    ASSERT_NOT_REACHED();
    return WebDOMEventTarget();
}

WebDOMEventTarget& WebDOMEventTarget::operator=(const WebDOMEventTarget& copy)
{
    delete m_impl;
    m_impl = copy.impl() ? new WebDOMEventTargetPrivate(copy.impl()) : 0;
    return *this;
}
