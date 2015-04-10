/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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
#include "MediaSource.h"

#if ENABLE(MEDIA_SOURCE)

#include "ContentType.h"
#include "Event.h"
#include "MIMETypeRegistry.h"
#include "SourceBufferPrivate.h"
#include "TimeRanges.h"
#include <wtf/Uint8Array.h>

namespace WebCore {

PassRefPtr<MediaSource> MediaSource::create(ScriptExecutionContext* context)
{
    RefPtr<MediaSource> mediaSource(adoptRef(new MediaSource(context)));
    mediaSource->suspendIfNeeded();
    return mediaSource.release();
}

MediaSource::MediaSource(ScriptExecutionContext* context)
    : ActiveDOMObject(context)
    , m_readyState(closedKeyword())
    , m_asyncEventQueue(GenericEventQueue::create(this))
{
    m_sourceBuffers = SourceBufferList::create(scriptExecutionContext(), m_asyncEventQueue.get());
    m_activeSourceBuffers = SourceBufferList::create(scriptExecutionContext(), m_asyncEventQueue.get());
}

const String& MediaSource::openKeyword()
{
    DEFINE_STATIC_LOCAL(const String, open, (ASCIILiteral("open")));
    return open;
}

const String& MediaSource::closedKeyword()
{
    DEFINE_STATIC_LOCAL(const String, closed, (ASCIILiteral("closed")));
    return closed;
}

const String& MediaSource::endedKeyword()
{
    DEFINE_STATIC_LOCAL(const String, ended, (ASCIILiteral("ended")));
    return ended;
}

SourceBufferList* MediaSource::sourceBuffers()
{
    return m_sourceBuffers.get();
}

SourceBufferList* MediaSource::activeSourceBuffers()
{
    // FIXME(91649): support track selection
    return m_activeSourceBuffers.get();
}

double MediaSource::duration() const
{
    return m_readyState == closedKeyword() ? std::numeric_limits<float>::quiet_NaN() : m_private->duration();
}

void MediaSource::setDuration(double duration, ExceptionCode& ec)
{
    if (duration < 0.0 || std::isnan(duration)) {
        ec = INVALID_ACCESS_ERR;
        return;
    }
    if (m_readyState != openKeyword()) {
        ec = INVALID_STATE_ERR;
        return;
    }
    m_private->setDuration(duration);
}

SourceBuffer* MediaSource::addSourceBuffer(const String& type, ExceptionCode& ec)
{
    // 3.1 http://dvcs.w3.org/hg/html-media/raw-file/tip/media-source/media-source.html#dom-addsourcebuffer
    // 1. If type is null or an empty then throw an INVALID_ACCESS_ERR exception and
    // abort these steps.
    if (type.isNull() || type.isEmpty()) {
        ec = INVALID_ACCESS_ERR;
        return 0;
    }

    // 2. If type contains a MIME type that is not supported ..., then throw a
    // NOT_SUPPORTED_ERR exception and abort these steps.
    if (!isTypeSupported(type)) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    // 4. If the readyState attribute is not in the "open" state then throw an
    // INVALID_STATE_ERR exception and abort these steps.
    if (!m_private || m_readyState != openKeyword()) {
        ec = INVALID_STATE_ERR;
        return 0;
    }

    // 5. Create a new SourceBuffer object and associated resources.
    ContentType contentType(type);
    Vector<String> codecs = contentType.codecs();
    OwnPtr<SourceBufferPrivate> sourceBufferPrivate;
    switch (m_private->addSourceBuffer(contentType.type(), codecs, &sourceBufferPrivate)) {
    case MediaSourcePrivate::Ok: {
        ASSERT(sourceBufferPrivate);
        RefPtr<SourceBuffer> buffer = SourceBuffer::create(sourceBufferPrivate.release(), this);

        // 6. Add the new object to sourceBuffers and fire a addsourcebuffer on that object.
        m_sourceBuffers->add(buffer);
        m_activeSourceBuffers->add(buffer);
        // 7. Return the new object to the caller.
        return buffer.get();
    }
    case MediaSourcePrivate::NotSupported:
        // 2 (cont). If type contains a MIME type ... that is not supported with the types 
        // specified for the other SourceBuffer objects in sourceBuffers, then throw
        // a NOT_SUPPORTED_ERR exception and abort these steps.
        ec = NOT_SUPPORTED_ERR;
        return 0;
    case MediaSourcePrivate::ReachedIdLimit:
        // 3 (cont). If the user agent can't handle any more SourceBuffer objects then throw 
        // a QUOTA_EXCEEDED_ERR exception and abort these steps.
        ec = QUOTA_EXCEEDED_ERR;
        return 0;
    }

    ASSERT_NOT_REACHED();
    return 0;
}

void MediaSource::removeSourceBuffer(SourceBuffer* buffer, ExceptionCode& ec)
{
    // 3.1 http://dvcs.w3.org/hg/html-media/raw-file/tip/media-source/media-source.html#dom-removesourcebuffer
    // 1. If sourceBuffer is null then throw an INVALID_ACCESS_ERR exception and
    // abort these steps.
    if (!buffer) {
        ec = INVALID_ACCESS_ERR;
        return;
    }

    // 2. If sourceBuffers is empty then throw an INVALID_STATE_ERR exception and
    // abort these steps.
    if (!m_private || !m_sourceBuffers->length()) {
        ec = INVALID_STATE_ERR;
        return;
    }

    // 3. If sourceBuffer specifies an object that is not in sourceBuffers then
    // throw a NOT_FOUND_ERR exception and abort these steps.
    // 6. Remove sourceBuffer from sourceBuffers and fire a removesourcebuffer event
    // on that object.
    if (!m_sourceBuffers->remove(buffer)) {
        ec = NOT_FOUND_ERR;
        return;
    }

    // 7. Destroy all resources for sourceBuffer.
    m_activeSourceBuffers->remove(buffer);

    // 4. Remove track information from audioTracks, videoTracks, and textTracks for all tracks 
    // associated with sourceBuffer and fire a simple event named change on the modified lists.
    // FIXME(91649): support track selection

    // 5. If sourceBuffer is in activeSourceBuffers, then remove it from that list and fire a
    // removesourcebuffer event on that object.
    // FIXME(91649): support track selection
}

const String& MediaSource::readyState() const
{
    return m_readyState;
}

void MediaSource::setReadyState(const String& state)
{
    ASSERT(state == openKeyword() || state == closedKeyword() || state == endedKeyword());
    if (m_readyState == state)
        return;

    String oldState = m_readyState;
    m_readyState = state;

    if (m_readyState == closedKeyword()) {
        m_sourceBuffers->clear();
        m_activeSourceBuffers->clear();
        m_private.clear();
        scheduleEvent(eventNames().webkitsourcecloseEvent);
        return;
    }

    if (oldState == openKeyword() && m_readyState == endedKeyword()) {
        scheduleEvent(eventNames().webkitsourceendedEvent);
        return;
    }

    if (m_readyState == openKeyword()) {
        scheduleEvent(eventNames().webkitsourceopenEvent);
        return;
    }
}

void MediaSource::endOfStream(const String& error, ExceptionCode& ec)
{
    // 3.1 http://dvcs.w3.org/hg/html-media/raw-file/tip/media-source/media-source.html#dom-endofstream
    // 1. If the readyState attribute is not in the "open" state then throw an
    // INVALID_STATE_ERR exception and abort these steps.
    if (!m_private || m_readyState != openKeyword()) {
        ec = INVALID_STATE_ERR;
        return;
    }

    MediaSourcePrivate::EndOfStreamStatus eosStatus = MediaSourcePrivate::EosNoError;

    if (error.isNull() || error.isEmpty())
        eosStatus = MediaSourcePrivate::EosNoError;
    else if (error == "network")
        eosStatus = MediaSourcePrivate::EosNetworkError;
    else if (error == "decode")
        eosStatus = MediaSourcePrivate::EosDecodeError;
    else {
        ec = INVALID_ACCESS_ERR;
        return;
    }

    // 2. Change the readyState attribute value to "ended".
    setReadyState(endedKeyword());
    m_private->endOfStream(eosStatus);
}

bool MediaSource::isTypeSupported(const String& type)
{
    // Section 2.1 isTypeSupported() method steps.
    // https://dvcs.w3.org/hg/html-media/raw-file/tip/media-source/media-source.html#widl-MediaSource-isTypeSupported-boolean-DOMString-type
    // 1. If type is an empty string, then return false.
    if (type.isNull() || type.isEmpty())
        return false;

    ContentType contentType(type);
    String codecs = contentType.parameter("codecs");

    // 2. If type does not contain a valid MIME type string, then return false.
    if (contentType.type().isEmpty() || codecs.isEmpty())
        return false;

    // 3. If type contains a media type or media subtype that the MediaSource does not support, then return false.
    // 4. If type contains at a codec that the MediaSource does not support, then return false.
    // 5. If the MediaSource does not support the specified combination of media type, media subtype, and codecs then return false.
    // 6. Return true.
    return MIMETypeRegistry::isSupportedMediaSourceMIMEType(contentType.type(), codecs);
}

void MediaSource::setPrivateAndOpen(PassOwnPtr<MediaSourcePrivate> mediaSourcePrivate)
{
    ASSERT(mediaSourcePrivate);
    ASSERT(!m_private);
    m_private = mediaSourcePrivate;
    setReadyState(openKeyword());
}

const AtomicString& MediaSource::interfaceName() const
{
    return eventNames().interfaceForMediaSource;
}

ScriptExecutionContext* MediaSource::scriptExecutionContext() const
{
    return ActiveDOMObject::scriptExecutionContext();
}

bool MediaSource::hasPendingActivity() const
{
    return m_private || m_asyncEventQueue->hasPendingEvents()
        || ActiveDOMObject::hasPendingActivity();
}

void MediaSource::stop()
{
    m_private.clear();
    m_asyncEventQueue->cancelAllEvents();
}

EventTargetData* MediaSource::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* MediaSource::ensureEventTargetData()
{
    return &m_eventTargetData;
}

void MediaSource::scheduleEvent(const AtomicString& eventName)
{
    ASSERT(m_asyncEventQueue);

    RefPtr<Event> event = Event::create(eventName, false, false);
    event->setTarget(this);

    m_asyncEventQueue->enqueueEvent(event.release());
}

} // namespace WebCore

#endif
