/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef WebKitTransitionEvent_h
#define WebKitTransitionEvent_h

#include "Event.h"

namespace WebCore {

struct WebKitTransitionEventInit : public EventInit {
    WebKitTransitionEventInit();

    String propertyName;
    double elapsedTime;
    String pseudoElement;
};

class WebKitTransitionEvent : public Event {
public:
    static PassRefPtr<WebKitTransitionEvent> create()
    {
        return adoptRef(new WebKitTransitionEvent);
    }
    static PassRefPtr<WebKitTransitionEvent> create(const AtomicString& type, const String& propertyName, double elapsedTime, const String& pseudoElement)
    {
        return adoptRef(new WebKitTransitionEvent(type, propertyName, elapsedTime, pseudoElement));
    }
    static PassRefPtr<WebKitTransitionEvent> create(const AtomicString& type, const WebKitTransitionEventInit& initializer)
    {
        return adoptRef(new WebKitTransitionEvent(type, initializer));
    }

    virtual ~WebKitTransitionEvent();

    const String& propertyName() const;
    double elapsedTime() const;
    const String& pseudoElement() const;

    virtual const AtomicString& interfaceName() const;

private:
    WebKitTransitionEvent();
    WebKitTransitionEvent(const AtomicString& type, const String& propertyName, double elapsedTime, const String& pseudoElement);
    WebKitTransitionEvent(const AtomicString& type, const WebKitTransitionEventInit& initializer);

    String m_propertyName;
    double m_elapsedTime;
    String m_pseudoElement;
};

} // namespace WebCore

#endif // WebKitTransitionEvent_h
