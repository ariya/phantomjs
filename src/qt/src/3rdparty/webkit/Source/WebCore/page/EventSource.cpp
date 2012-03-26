/*
 * Copyright (C) 2009 Ericsson AB
 * All rights reserved.
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

#if ENABLE(EVENTSOURCE)

#include "EventSource.h"

#include "MemoryCache.h"
#include "DOMWindow.h"
#include "Event.h"
#include "EventException.h"
#include "PlatformString.h"
#include "MessageEvent.h"
#include "ResourceError.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "ScriptCallStack.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"
#include "SerializedScriptValue.h"
#include "TextResourceDecoder.h"
#include "ThreadableLoader.h"

namespace WebCore {

const unsigned long long EventSource::defaultReconnectDelay = 3000;

inline EventSource::EventSource(const KURL& url, ScriptExecutionContext* context)
    : ActiveDOMObject(context, this)
    , m_url(url)
    , m_state(CONNECTING)
    , m_decoder(TextResourceDecoder::create("text/plain", "UTF-8"))
    , m_reconnectTimer(this, &EventSource::reconnectTimerFired)
    , m_discardTrailingNewline(false)
    , m_failSilently(false)
    , m_requestInFlight(false)
    , m_reconnectDelay(defaultReconnectDelay)
    , m_origin(context->securityOrigin()->toString())
{
}

PassRefPtr<EventSource> EventSource::create(const String& url, ScriptExecutionContext* context, ExceptionCode& ec)
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

    // FIXME: Should support at least some cross-origin requests.
    if (!context->securityOrigin()->canRequest(fullURL)) {
        ec = SECURITY_ERR;
        return 0;
    }

    RefPtr<EventSource> source = adoptRef(new EventSource(fullURL, context));

    source->setPendingActivity(source.get());
    source->connect();

    return source.release();
}

EventSource::~EventSource()
{
}

void EventSource::connect()
{
    ResourceRequest request(m_url);
    request.setHTTPMethod("GET");
    request.setHTTPHeaderField("Accept", "text/event-stream");
    request.setHTTPHeaderField("Cache-Control", "no-cache");
    if (!m_lastEventId.isEmpty())
        request.setHTTPHeaderField("Last-Event-ID", m_lastEventId);

    ThreadableLoaderOptions options;
    options.sendLoadCallbacks = true;
    options.sniffContent = false;
    options.allowCredentials = true;

    m_loader = ThreadableLoader::create(scriptExecutionContext(), this, request, options);

    m_requestInFlight = true;
}

void EventSource::endRequest()
{
    if (!m_requestInFlight)
        return;

    m_requestInFlight = false;

    if (!m_failSilently)
        dispatchEvent(Event::create(eventNames().errorEvent, false, false));

    if (m_state != CLOSED)
        scheduleReconnect();
    else
        unsetPendingActivity(this);
}

void EventSource::scheduleReconnect()
{
    m_state = CONNECTING;
    m_reconnectTimer.startOneShot(m_reconnectDelay / 1000);
}

void EventSource::reconnectTimerFired(Timer<EventSource>*)
{
    connect();
}

String EventSource::url() const
{
    return m_url.string();
}

EventSource::State EventSource::readyState() const
{
    return m_state;
}

void EventSource::close()
{
    if (m_state == CLOSED)
        return;

    if (m_reconnectTimer.isActive()) {
        m_reconnectTimer.stop();
        unsetPendingActivity(this);
    }

    m_state = CLOSED;
    m_failSilently = true;

    if (m_requestInFlight)
        m_loader->cancel();
}

ScriptExecutionContext* EventSource::scriptExecutionContext() const
{
    return ActiveDOMObject::scriptExecutionContext();
}

void EventSource::didReceiveResponse(const ResourceResponse& response)
{
    int statusCode = response.httpStatusCode();
    bool mimeTypeIsValid = response.mimeType() == "text/event-stream";
    bool responseIsValid = statusCode == 200 && mimeTypeIsValid;
    if (responseIsValid) {
        const String& charset = response.textEncodingName();
        // If we have a charset, the only allowed value is UTF-8 (case-insensitive). This should match
        // the updated EventSource standard.
        responseIsValid = charset.isEmpty() || equalIgnoringCase(charset, "UTF-8");
        if (!responseIsValid) {
            String message = "EventSource's response has a charset (\"";
            message += charset;
            message += "\") that is not UTF-8. Aborting the connection.";
            // FIXME: We are missing the source line.
            scriptExecutionContext()->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, message, 1, String(), 0);
        }
    } else {
        // To keep the signal-to-noise ratio low, we only log 200-response with an invalid MIME type.
        if (statusCode == 200 && !mimeTypeIsValid) {
            String message = "EventSource's response has a MIME type (\"";
            message += response.mimeType();
            message += "\") that is not \"text/event-stream\". Aborting the connection.";
            // FIXME: We are missing the source line.
            scriptExecutionContext()->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, message, 1, String(), 0);
        }
    }

    if (responseIsValid) {
        m_state = OPEN;
        dispatchEvent(Event::create(eventNames().openEvent, false, false));
    } else {
        if (statusCode <= 200 || statusCode > 299)
            m_state = CLOSED;
        m_loader->cancel();
    }
}

void EventSource::didReceiveData(const char* data, int length)
{
    append(m_receiveBuf, m_decoder->decode(data, length));
    parseEventStream();
}

void EventSource::didFinishLoading(unsigned long, double)
{
    if (m_receiveBuf.size() > 0 || m_data.size() > 0) {
        append(m_receiveBuf, "\n\n");
        parseEventStream();
    }
    m_state = CONNECTING;
    endRequest();
}

void EventSource::didFail(const ResourceError& error)
{
    int canceled = error.isCancellation();
    if (((m_state == CONNECTING) && !canceled) || ((m_state == OPEN) && canceled))
        m_state = CLOSED;
    endRequest();
}

void EventSource::didFailRedirectCheck()
{
    m_state = CLOSED;
    m_loader->cancel();
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
    }

    if (bufPos == bufSize)
        m_receiveBuf.clear();
    else if (bufPos)
        m_receiveBuf.remove(0, bufPos);
}

void EventSource::parseEventStreamLine(unsigned int bufPos, int fieldLength, int lineLength)
{
    if (!lineLength) {
        if (!m_data.isEmpty()) {
            m_data.removeLast();
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
            m_lastEventId = valueLength ? String(&m_receiveBuf[bufPos], valueLength) : "";
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
    event->initMessageEvent(m_eventName.isEmpty() ? eventNames().messageEvent : AtomicString(m_eventName), false, false, SerializedScriptValue::create(String::adopt(m_data)), m_origin, m_lastEventId, 0, 0);
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

#endif // ENABLE(EVENTSOURCE)
