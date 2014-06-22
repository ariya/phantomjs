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
#include "WebNativeEventListener.h"

#include "WebDOMEvent.h"

WebNativeEventListener::WebNativeEventListener(WebUserEventListener* listener)
    : WebCore::EventListener(CPPEventListenerType),
    m_listener(listener)
{
    ASSERT(m_listener);
    m_listener->ref();
}

WebNativeEventListener::~WebNativeEventListener()
{
    m_listener->deref();
}

void WebNativeEventListener::handleEvent(WebCore::ScriptExecutionContext*, WebCore::Event* event)
{
    m_listener->handleEvent(toWebKit(event));
}

bool WebNativeEventListener::reportError(WebCore::ScriptExecutionContext*, const WTF::String&, const WTF::String&, int)
{
    // FIXME: Implement error handling
    return false;
}

bool WebNativeEventListener::operator==(const WebCore::EventListener& other)
{
    const WebNativeEventListener* ptrOther = cast(&other);
    return ptrOther && m_listener == ptrOther->m_listener;
}
