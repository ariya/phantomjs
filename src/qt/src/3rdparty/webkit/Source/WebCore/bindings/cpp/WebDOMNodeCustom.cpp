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

#include "config.h"
#include "WebDOMNode.h"

#include "Node.h"
#include "WebDOMEventListener.h"
#include "WebExceptionHandler.h"
#include "WebNativeEventListener.h"

WebDOMNode WebDOMNode::insertBefore(const WebDOMNode& newChild, const WebDOMNode& refChild)
{
    if (!impl())
        return WebDOMNode();

    WebCore::ExceptionCode ec = 0;
    if (impl()->insertBefore(toWebCore(newChild), toWebCore(refChild), ec, true))
        return newChild;

    webDOMRaiseError(static_cast<WebDOMExceptionCode>(ec));
    return WebDOMNode();
}

WebDOMNode WebDOMNode::replaceChild(const WebDOMNode& newChild, const WebDOMNode& oldChild)
{
    if (!impl())
        return WebDOMNode();

    WebCore::ExceptionCode ec = 0;
    if (impl()->replaceChild(toWebCore(newChild), toWebCore(oldChild), ec, true))
        return oldChild;

    webDOMRaiseError(static_cast<WebDOMExceptionCode>(ec));
    return WebDOMNode();
}

WebDOMNode WebDOMNode::removeChild(const WebDOMNode& oldChild)
{
    if (!impl())
        return WebDOMNode();

    WebCore::ExceptionCode ec = 0;
    if (impl()->removeChild(toWebCore(oldChild), ec))
        return oldChild;

    webDOMRaiseError(static_cast<WebDOMExceptionCode>(ec));
    return WebDOMNode();
}

WebDOMNode WebDOMNode::appendChild(const WebDOMNode& newChild)
{
    if (!impl())
        return WebDOMNode();

    WebCore::ExceptionCode ec = 0;
    if (impl()->appendChild(toWebCore(newChild), ec, true))
        return newChild;

    webDOMRaiseError(static_cast<WebDOMExceptionCode>(ec));
    return WebDOMNode();
}

void WebDOMNode::addEventListener(const WebDOMString& type, const WebDOMEventListener& listener, bool useCapture)
{
    if (!impl())
        return;

    if (toWebCore(listener))
        impl()->addEventListener(type, toWebCore(listener), useCapture);
}

void WebDOMNode::removeEventListener(const WebDOMString& type, const WebDOMEventListener& listener, bool useCapture)
{
    if (!impl())
        return;

    if (toWebCore(listener))
        impl()->removeEventListener(type, toWebCore(listener), useCapture);
}
