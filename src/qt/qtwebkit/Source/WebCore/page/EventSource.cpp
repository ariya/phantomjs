/*
 * Copyright (C) 2009, 2012 Ericsson AB. All rights reserved.
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011, Code Aurora Forum. All rights reserved.
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

#include "config.h"
#include "EventSource.h"

#include "ContentSecurityPolicy.h"
#include "DOMWindow.h"
#include "Dictionary.h"
#include "Document.h"
#include "Event.h"
#include "EventException.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "MemoryCache.h"
#include "MessageEvent.h"
#include "ResourceError.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "ScriptCallStack.h"
#include "ScriptController.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"
#include "SerializedScriptValue.h"
#include "TextResourceDecoder.h"
#include "ThreadableLoader.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

const unsigned long long EventSource::defaultReconnectDelay = 3000;

inline EventSource::EventSource(ScriptExecutionContext* context, const KURL& url, const Dictionary& eventSourceInit)
    : ActiveDOMObject(context)
    , m_url(url)
    , m_withCredentials(false)
    , m_state(CONNECTING)
    , m_decoder(TextResourceDecoder::create("text/plain", "UTF-8"))
    , m_connectTimer(this, &EventSource::connectTimerFired)
    , m_discardTrailingNewline(false)
    , m_requestInFlight(false)
    , m_reconnectDelay(defaultReconnectDelay)
{
    eventSourceInit.get("withCredentials", m_withCredentials);
}

PassRefPtr<EventSource> EventSource::create(ScriptExecutionContext* context, const String& url, const Dictionary& eventSourceInit, ExceptionCode& ec)
{
    if (url.isEmpty()) {
        ec = SYNTAX_ERR;
        return 0;
    }

    KURL fullURL = context->completeURL(url);
    if (!fullURL.isValid()) {
        ec = SYNTAX_ERR;
        return 0;
    }

    // FIXME: Convert this to check the isolated world's Content Security Policy once webkit.org/b/104520 is solved.
    bool shouldBypassMainWorldContentSecurityPolicy = false;
    if (context->isDocument()) {
        Document* document = toDocument(context);
        shouldBypassMainWorldContentSecurityPolicy = document->frame()->script()->shouldBypassMainWorldContentSecurityPolicy();
    }
    if (!shouldBypassMainWorldContentSecurityPolicy && !context->contentSecurityPolicy()->allowConnectToSource(fullURL)) {
        // FIXME: Should this be throwing an exception?
        ec = SECURITY_ERR;
        return 0;
    }

    RefPtr<EventSource> source = adoptRef(new EventSource(context, fullURL, eventSourceInit));

    source->setPendingActivity(source.get());
    source->scheduleInitialConnect();
    source->suspendIfNeeded();

    return source.release();
}

EventSource::~EventSource()
{
    ASSERT(m_state == CLOSED);
    ASSERT(!m_requestInFlight);
}

void EventSource::connect()
{
    ASSERT(m_state == CONNECTING);
    ASSERT(!m_requestInFlight);

    ResourceRequest request(m_url);
    request.setHTTPMethod("GET");
    request.setHTTPHeaderField("Accept", "text/event-stream");
    request.setHTTPHeaderField("Cache-Control", "no-cache");
    if (!m_lastEventId.isEmpty())
        request.setHTTPHeaderField("Last-Event-ID", m_lastEventId);

    SecurityOrigin* origin = scriptExecutionContext()->securityOrigin();

    ThreadableLoaderOptions options;
    options.sendLoadCallbacks = SendCallbacks;
    options.sniffContent = DoNotSniffContent;
    options.allowCredentials = (origin->canRequest(m_url) || m_withCredentials) ? AllowStoredCredentials : DoNotAllowStoredCredentials;
    options.preflightPolicy = PreventPreflight;
    options.crossOriginRequestPolicy = UseAccessControl;
    options.dataBufferingPolicy = DoNotBufferData;
    options.securityOrigin = origin;

    m_loader = ThreadableLoader::create(scriptExecutionContext(), this, request, options);

    if (m_loader)
        m_requestInFlight = true;
}

void EventSource::networkRequestEnded()
{
    if (!m_requestInFlight)
        return;

    m_requestInFlight = false;

    if (m_state != CLOSED)
        scheduleReconnect();
    else
        unsetPendingActivity(this);
}

void EventSource::scheduleInitialConnect()
{
    ASSERT(m_state == CONNECTING);
    ASSERT(!m_requestInFlight);

    m_connectTimer.startOneShot(0);
}

void EventSource::scheduleReconnect()
{
    m_state = CONNECTING;
    m_connectTimer.startOneShot(m_reconnectDelay / 1000.0);
    dispatchEvent(Event::create(eventNames().errorEvent, false, false));
}

void EventSource::connectTimerFired(Timer<EventSource>*)
{
    connect();
}

String EventSource::url() const
{
    return m_url.string();
}

bool EventSource::withCredentials() const
{
    return m_withCredentials;
}

EventSource::State EventSource::readyState() const
{
    return m_state;
}

void EventSource::close()
{
    if (m_state == CLOSED) {
        ASSERT(!m_requestInFlight);
        return;
    }

    // Stop trying to connect/reconnect if EventSource was explicitly closed or if ActiveDOMObject::stop() was called.
    if (m_connectTimer.isActive())
        m_connectTimer.stop();

    if (m_requestInFlight)
        m_loader->cancel();
    else {
        m_state = CLOSED;
        unsetPendingActivity(this);
    }
}

const AtomicString& EventSource::interfaceName() const
{
    return eventNames().interfaceForEventSource;
}

ScriptExecutionContext* EventSource::scriptExecutionContext() const
{
    return ActiveDOMObject::scriptExecutionContext();
}

void EventSource::didReceiveResponse(unsigned long, const ResourceResponse& response)
{
    ASSERT(m_state == CONNECTING);
    ASSERT(m_requestInFlight);

    m_eventStreamOrigin = SecurityOrigin::create(response.url())->toString();
    int statusCode = response.httpStatusCode();
    bool mimeTypeIsValid = response.mimeType() == "text/event-stream";
    bool responseIsValid = statusCode == 200 && mimeTypeIsValid;
    if (responseIsValid) {
        const String& charset = response.textEncodingName();
        // If we have a charset, the only allowed value is UTF-8 (case-insensitive).
        responseIsValid = charset.isEmpty() || equalIgnoringCase(charset, "UTF-8");
        if (!responseIsValid) {
            StringBuilder message;
            message.appendLiteral("EventSource's response has a charset (\"");
            message.append(charset);
            message.appendLiteral("\") that is not UTF-8. Aborting the connection.");
            // FIXME: We are missing the source line.
            scriptExecutionContext()->addConsoleMessage(JSMessageSource, ErrorMessageLevel, message.toString());
        }
    } else {
        // To keep the signal-to-noise ratio low, we only log 200-response with an invalid MIME type.
        if (statusCode == 200 && !mimeTypeIsValid) {
            StringBuilder message;
            message.appendLiteral("EventSource's response has a MIME type (\"");
            message.append(response.mimeType());
            message.appendLiteral("\") that is not \"text/event-stream\". Aborting the connection.");
            // FIXME: We are missing the source line.
            scriptExecutionContext()->addConsoleMessage(JSMessageSource, ErrorMessageLevel, message.toString());
        }
    }

    if (responseIsValid) {
        m_state = OPEN;
        dispatchEvent(Event::create(eventNames().openEvent, false, false));
    } else {
        m_loader->cancel();
        dispatchEvent(Event::create(eventNames().errorEvent, false, false));
    }
}

void EventSource::didReceiveData(const char* data, int length)
{
    ASSERT(m_state == OPEN);
    ASSERT(m_requestInFlight);

    append(m_receiveBuf, m_decoder->decode(data, length));
    parseEventStream();
}

void EventSource::didFinishLoading(unsigned long, double)
{
    ASSERT(m_state == OPEN);
    ASSERT(m_requestInFlight);

    if (m_receiveBuf.size() > 0 || m_data.size() > 0) {
        parseEventStream();

        // Discard everything that has not been dispatched by now.
        m_receiveBuf.clear();
        m_data.clear();
        m_eventName = "";
        m_currentlyParsedEventId = String();
    }
    networkRequestEnded();
}

void EventSource::didFail(const ResourceError& error)
{
    ASSERT(m_state != CLOSED);
    ASSERT(m_requestInFlight);

    if (error.isCancellation())
        m_state = CLOSED;
    networkRequestEnded();
}

void EventSource::didFailAccessControlCheck(const ResourceError& error)
{
    String message = makeString("EventSource cannot load ", error.failingURL(), ". ", error.localizedDescription());
    scriptExecutionContext()->addConsoleMessage(JSMessageSource, ErrorMessageLevel, message);

    abortConnectionAttempt();
}

void EventSource::didFailRedirectCheck()
{
    abortConnectionAttempt();
}

void EventSource::abortConnectionAttempt()
{
    ASSERT(m_state == CONNECTING);

    if (m_requestInFlight)
        m_loader->cancel();
    else {
        m_state = CLOSED;
        unsetPendingActivity(this);
    }

    ASSERT(m_state == CLOSED);
    dispatchEvent(Event::create(eventNames().errorEvent, false, false));
}

void EventSource::parseEventStream()
{
    unsigned int bufPos = 0;
    unsigned int bufSize = m_receiveBuf.size();
    while (bufPos < bufSize) {
        if (m_discardTrailingNewline) {
            if (m_receiveBuf[bufPos] == '\n')
                bufPos++;
            m_discardTrailingNewline = false;
        }

        int lineLength = -1;
        int fieldLength = -1;
        for (unsigned int i = bufPos; lineLength < 0 && i < bufSize; i++) {
            switch (m_receiveBuf[i]) {
            case ':':
                if (fieldLength < 0)
                    fieldLength = i - bufPos;
                break;
            case '\r':
                m_discardTrailingNewline = true;
            case '\n':
                lineLength = i - bufPos;
                break;
            }
        }

        if (lineLength < 0)
            break;

        parseEventStreamLine(bufPos, fieldLength, lineLength);
        bufPos += lineLength + 1;

        // EventSource.close() might've been called by one of the message event handlers.
        // Per spec, no further messages should be fired after that.
        if (m_state == CLOSED)
            break;
    }

    if (bufPos == bufSize)
        m_receiveBuf.clear();
    else if (bufPos)
        m_receiveBuf.remove(0, bufPos);
}

void EventSource::parseEventStreamLine(unsigned bufPos, int fieldLength, int lineLength)
{
    if (!lineLength) {
        if (!m_data.isEmpty()) {
            m_data.removeLast();
            if (!m_currentlyParsedEventId.isNull()) {
                m_lastEventId.swap(m_currentlyParsedEventId);
                m_currentlyParsedEventId = String();
            }
            dispatchEvent(createMessageEvent());
        }
        if (!m_eventName.isEmpty())
            m_eventName = "";
    } else if (fieldLength) {
        bool noValue = fieldLength < 0;

        String field(&m_receiveBuf[bufPos], noValue ? lineLength : fieldLength);
        int step;
        if (noValue)
            step = lineLength;
        else if (m_receiveBuf[bufPos + fieldLength + 1] != ' ')
            step = fieldLength + 1;
        else
            step = fieldLength + 2;
        bufPos += step;
        int valueLength = lineLength - step;

        if (field == "data") {
            if (valueLength)
                m_data.append(&m_receiveBuf[bufPos], valueLength);
            m_data.append('\n');
        } else if (field == "event")
            m_eventName = valueLength ? String(&m_receiveBuf[bufPos], valueLength) : "";
        else if (field == "id")
            m_currentlyParsedEventId = valueLength ? String(&m_receiveBuf[bufPos], valueLength) : "";
        else if (field == "retry") {
            if (!valueLength)
                m_reconnectDelay = defaultReconnectDelay;
            else {
                String value(&m_receiveBuf[bufPos], valueLength);
                bool ok;
                unsigned long long retry = value.toUInt64(&ok);
                if (ok)
                    m_reconnectDelay = retry;
            }
        }
    }
}

void EventSource::stop()
{
    close();
}

PassRefPtr<MessageEvent> EventSource::createMessageEvent()
{
    RefPtr<MessageEvent> event = MessageEvent::create();
    event->initMessageEvent(m_eventName.isEmpty() ? eventNames().messageEvent : AtomicString(m_eventName), false, false, SerializedScriptValue::create(String::adopt(m_data)), m_eventStreamOrigin, m_lastEventId, 0, 0);
    return event.release();
}

EventTargetData* EventSource::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* EventSource::ensureEventTargetData()
{
    return &m_eventTargetData;
}

} // namespace WebCore
