/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2003, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef WebNativeEventListener_h
#define WebNativeEventListener_h

#include "EventListener.h"
#include "WebDOMEventListener.h"

class WebNativeEventListener : public WebCore::EventListener {
public:
    static PassRefPtr<WebNativeEventListener> create(WebUserEventListener* listener)
    {
        return adoptRef(new WebNativeEventListener(listener));
    }

    static const WebNativeEventListener* cast(const WebCore::EventListener* listener)
    {
        return listener->type() == CPPEventListenerType
                ? static_cast<const WebNativeEventListener*>(listener)
                : 0;
    }

    virtual ~WebNativeEventListener();
    virtual bool operator==(const WebCore::EventListener& other);

private:
    virtual void handleEvent(WebCore::ScriptExecutionContext*, WebCore::Event*);
    virtual bool reportError(WebCore::ScriptExecutionContext*, const WTF::String& message, const WTF::String& url, int lineNumber);

protected:
    WebNativeEventListener(WebUserEventListener*);
    WebUserEventListener* m_listener;
};

#endif
