/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE, INC. ``AS IS'' AND ANY
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
 *
 */

#ifndef BeforeLoadEvent_h
#define BeforeLoadEvent_h

#include "Event.h"
#include "EventNames.h"

namespace WebCore {

struct BeforeLoadEventInit : public EventInit {
    BeforeLoadEventInit()
    {
    };

    String url;
};

class BeforeLoadEvent : public Event {
public:
    static PassRefPtr<BeforeLoadEvent> create()
    {
        return adoptRef(new BeforeLoadEvent);
    }

    static PassRefPtr<BeforeLoadEvent> create(const String& url)
    {
        return adoptRef(new BeforeLoadEvent(url));
    }

    static PassRefPtr<BeforeLoadEvent> create(const AtomicString& type, const BeforeLoadEventInit& initializer)
    {
        return adoptRef(new BeforeLoadEvent(type, initializer));
    }

    const String& url() const { return m_url; }

    virtual const AtomicString& interfaceName() const { return eventNames().interfaceForBeforeLoadEvent; }

private:
    BeforeLoadEvent()
    {
    }

    explicit BeforeLoadEvent(const String& url)
        : Event(eventNames().beforeloadEvent, false, true)
        , m_url(url)
    {
    }

    BeforeLoadEvent(const AtomicString& type, const BeforeLoadEventInit& initializer)
        : Event(type, initializer)
        , m_url(initializer.url)
    {
    }

    String m_url;
};

} // namespace WebCore

#endif // BeforeLoadEvent_h
