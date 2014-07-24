/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2006 James G. Speth (speth@end.com)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "ObjCEventListener.h"

#import "DOMEventInternal.h"
#import "DOMEventListener.h"
#import "Event.h"
#import "EventListener.h"
#import "JSMainThreadExecState.h"
#import <wtf/HashMap.h>

namespace WebCore {

typedef HashMap<id, ObjCEventListener*> ListenerMap;
static ListenerMap* listenerMap;

ObjCEventListener* ObjCEventListener::find(ObjCListener listener)
{
    ListenerMap* map = listenerMap;
    if (!map)
        return 0;
    return map->get(listener);
}

PassRefPtr<ObjCEventListener> ObjCEventListener::wrap(ObjCListener listener)
{
    RefPtr<ObjCEventListener> wrapper = find(listener);
    if (wrapper)
        return wrapper;
    return adoptRef(new ObjCEventListener(listener));
}

ObjCEventListener::ObjCEventListener(ObjCListener listener)
    : EventListener(ObjCEventListenerType)
    , m_listener(listener)
{
    ListenerMap* map = listenerMap;
    if (!map) {
        map = new ListenerMap;
        listenerMap = map;
    }
    map->set(listener, this);
}

ObjCEventListener::~ObjCEventListener()
{
    listenerMap->remove(m_listener.get());
}

void ObjCEventListener::handleEvent(ScriptExecutionContext*, Event* event)
{
    ObjCListener listener = m_listener.get();
    [listener handleEvent:kit(event)];
}

bool ObjCEventListener::operator==(const EventListener& listener)
{
    if (const ObjCEventListener* objCEventListener = ObjCEventListener::cast(&listener))
        return m_listener == objCEventListener->m_listener;
    return false;
}

} // namespace WebCore
