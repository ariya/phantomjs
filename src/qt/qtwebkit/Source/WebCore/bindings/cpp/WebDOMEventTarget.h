/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#ifndef WebDOMEventTarget_h
#define WebDOMEventTarget_h

#include <WebDOMObject.h>

namespace WebCore {
class EventTarget;
};

class WebDOMDedicatedWorkerGlobalScope;
class WebDOMDOMApplicationCache;
class WebDOMDOMWindow;
class WebDOMEventSource;
class WebDOMMessagePort;
class WebDOMNode;
class WebDOMNotification;
class WebDOMSharedWorker;
class WebDOMSharedWorkerGlobalScope;
class WebDOMWebSocket;
class WebDOMWorker;
class WebDOMXMLHttpRequest;
class WebDOMXMLHttpRequestUpload;

class WebDOMEventTarget : public WebDOMObject {
public:
    WebDOMEventTarget();
    explicit WebDOMEventTarget(WebCore::EventTarget*);
    WebDOMEventTarget(const WebDOMEventTarget&);
    ~WebDOMEventTarget();

    WebCore::EventTarget* impl() const;

    WebDOMNode toNode();
    WebDOMDOMWindow toDOMWindow();
    WebDOMWorker toWorker();
    WebDOMDedicatedWorkerGlobalScope toDedicatedWorkerGlobalScope();
    WebDOMSharedWorker toSharedWorker();
    WebDOMSharedWorkerGlobalScope toSharedWorkerGlobalScope();
    WebDOMNotification toNotification();
    WebDOMWebSocket toWebSocket();

    WebDOMEventTarget& operator=(const WebDOMEventTarget&);
protected:
    struct WebDOMEventTargetPrivate;
    WebDOMEventTargetPrivate* m_impl;
};

WebCore::EventTarget* toWebCore(const WebDOMEventTarget&);
WebDOMEventTarget toWebKit(WebCore::EventTarget*);

#endif
