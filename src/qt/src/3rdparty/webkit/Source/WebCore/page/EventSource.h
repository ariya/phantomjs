/*
 * Copyright (C) 2009 Ericsson AB
 * All rights reserved.
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Ericsson nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef EventSource_h
#define EventSource_h

#if ENABLE(EVENTSOURCE)

#include "ActiveDOMObject.h"
#include "EventTarget.h"
#include "KURL.h"
#include "ThreadableLoaderClient.h"
#include "Timer.h"
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

    class MessageEvent;
    class ResourceResponse;
    class TextResourceDecoder;
    class ThreadableLoader;

    class EventSource : public RefCounted<EventSource>, public EventTarget, private ThreadableLoaderClient, public ActiveDOMObject {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        static PassRefPtr<EventSource> create(const String& url, ScriptExecutionContext*, ExceptionCode&);
        virtual ~EventSource();

        static const unsigned long long defaultReconnectDelay;

        String url() const;

        enum State {
            CONNECTING = 0,
            OPEN = 1,
            CLOSED = 2,
        };

        State readyState() const;

        DEFINE_ATTRIBUTE_EVENT_LISTENER(open);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(message);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(error);

        void close();

        using RefCounted<EventSource>::ref;
        using RefCounted<EventSource>::deref;

        virtual EventSource* toEventSource() { return this; }
        virtual ScriptExecutionContext* scriptExecutionContext() const;

        virtual void stop();

    private:
        EventSource(const KURL&, ScriptExecutionContext*);

        virtual void refEventTarget() { ref(); }
        virtual void derefEventTarget() { deref(); }
        virtual EventTargetData* eventTargetData();
        virtual EventTargetData* ensureEventTargetData();

        virtual void didReceiveResponse(const ResourceResponse&);
        virtual void didReceiveData(const char*, int);
        virtual void didFinishLoading(unsigned long, double);
        virtual void didFail(const ResourceError&);
        virtual void didFailRedirectCheck();

        void connect();
        void endRequest();
        void scheduleReconnect();
        void reconnectTimerFired(Timer<EventSource>*);
        void parseEventStream();
        void parseEventStreamLine(unsigned int pos, int fieldLength, int lineLength);
        PassRefPtr<MessageEvent> createMessageEvent();

        KURL m_url;
        State m_state;

        RefPtr<TextResourceDecoder> m_decoder;
        RefPtr<ThreadableLoader> m_loader;
        Timer<EventSource> m_reconnectTimer;
        Vector<UChar> m_receiveBuf;
        bool m_discardTrailingNewline;
        bool m_failSilently;
        bool m_requestInFlight;

        String m_eventName;
        Vector<UChar> m_data;
        String m_lastEventId;
        unsigned long long m_reconnectDelay;
        String m_origin;
        
        EventTargetData m_eventTargetData;
    };

} // namespace WebCore

#endif // ENABLE(EVENTSOURCE)

#endif // EventSource_h
