/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Samuel Weinig <sam@webkit.org>
 *  Copyright (C) 2008 Luke Kenneth Casson Leighton <lkcl@lkcl.net>
 *  Copyright (C) 2008 Martin Soto <soto@freedesktop.org>
 *  Copyright (C) 2009, 2010 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "WebKitDOMBinding.h"

#include "DOMObjectCache.h"
#include "Element.h"
#include "Event.h"
#include "EventException.h"
#include "HTMLNames.h"
#include "MouseEvent.h"
#include "UIEvent.h"
#include "WebKitDOMDOMWindowPrivate.h"
#include "WebKitDOMElementPrivate.h"
#include "WebKitDOMEventPrivate.h"
#include "WebKitDOMNode.h"
#include "WebKitDOMNodePrivate.h"
#include "WebKitHTMLElementWrapperFactory.h"
#include "webkit/WebKitDOMMouseEventPrivate.h"
#include "webkit/WebKitDOMUIEventPrivate.h"

namespace WebKit {

using namespace WebCore;
using namespace WebCore::HTMLNames;

// kit methods

static gpointer createWrapper(Node* node)
{
    ASSERT(node);
    ASSERT(node->nodeType());

    gpointer wrappedNode = 0;

    switch (node->nodeType()) {
    case Node::ELEMENT_NODE:
        if (node->isHTMLElement())
            wrappedNode = createHTMLElementWrapper(toHTMLElement(node));
        else
            wrappedNode = wrapElement(static_cast<Element*>(node));
        break;
    default:
        wrappedNode = wrapNode(node);
        break;
    }

    return DOMObjectCache::put(node, wrappedNode);
}

WebKitDOMNode* kit(Node* node)
{
    if (!node)
        return 0;

    gpointer kitNode = DOMObjectCache::get(node);
    if (kitNode)
        return static_cast<WebKitDOMNode*>(kitNode);

    return static_cast<WebKitDOMNode*>(createWrapper(node));
}

WebKitDOMElement* kit(Element* element)
{
    if (!element)
        return 0;

    gpointer kitElement = DOMObjectCache::get(element);
    if (kitElement)
        return static_cast<WebKitDOMElement*>(kitElement);

    gpointer wrappedElement;

    if (element->isHTMLElement())
        wrappedElement = createHTMLElementWrapper(toHTMLElement(element));
    else
        wrappedElement = wrapElement(element);

    return static_cast<WebKitDOMElement*>(DOMObjectCache::put(element, wrappedElement));
}

WebKitDOMEvent* kit(Event* event)
{
    if (!event)
        return 0;

    gpointer kitEvent = DOMObjectCache::get(event);
    if (kitEvent)
        return static_cast<WebKitDOMEvent*>(kitEvent);

    gpointer wrappedEvent;

    if (event->isMouseEvent())
        wrappedEvent = wrapMouseEvent(static_cast<MouseEvent*>(event));
    else if (event->isUIEvent())
        wrappedEvent = wrapUIEvent(static_cast<UIEvent*>(event));
    else
        wrappedEvent = wrapEvent(event);

    return static_cast<WebKitDOMEvent*>(DOMObjectCache::put(event, wrappedEvent));
}

static gpointer wrapEventTarget(EventTarget* target)
{
    ASSERT(target);

    gpointer wrappedTarget = 0;

    if (target->toNode()) {
        Node* node = target->toNode();
        wrappedTarget = wrapNode(node);
    } else if (target->toDOMWindow()) {
        DOMWindow* window = target->toDOMWindow();
        wrappedTarget = wrapDOMWindow(window);
    }

    return DOMObjectCache::put(target, wrappedTarget);
}

WebKitDOMEventTarget* kit(WebCore::EventTarget* obj)
{
    g_return_val_if_fail(obj, 0);

    if (gpointer ret = DOMObjectCache::get(obj))
        return static_cast<WebKitDOMEventTarget*>(ret);

    return static_cast<WebKitDOMEventTarget*>(DOMObjectCache::put(obj, WebKit::wrapEventTarget(obj)));
}

} // namespace WebKit
