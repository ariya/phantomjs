/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MediaKeySession.h"

#if ENABLE(ENCRYPTED_MEDIA_V2)

#include "CDM.h"
#include "Event.h"
#include "GenericEventQueue.h"
#include "MediaKeyError.h"
#include "MediaKeyMessageEvent.h"
#include "MediaKeys.h"

namespace WebCore {

PassRefPtr<MediaKeySession> MediaKeySession::create(ScriptExecutionContext* context, MediaKeys* keys, const String& keySystem)
{
    return adoptRef(new MediaKeySession(context, keys, keySystem));
}

MediaKeySession::MediaKeySession(ScriptExecutionContext* context, MediaKeys* keys, const String& keySystem)
    : ContextDestructionObserver(context)
    , m_keys(keys)
    , m_keySystem(keySystem)
    , m_asyncEventQueue(GenericEventQueue::create(this))
    , m_session(keys->cdm()->createSession())
    , m_keyRequestTimer(this, &MediaKeySession::keyRequestTimerFired)
    , m_addKeyTimer(this, &MediaKeySession::addKeyTimerFired)
{
}

MediaKeySession::~MediaKeySession()
{
    close();
}

void MediaKeySession::setError(MediaKeyError* error)
{
    m_error = error;
}

void MediaKeySession::close()
{
    if (m_session)
        m_session->releaseKeys();
    m_session = 0;
    m_asyncEventQueue->cancelAllEvents();
}

const String& MediaKeySession::sessionId() const
{
    return m_session->sessionId();
}

void MediaKeySession::generateKeyRequest(const String& mimeType, Uint8Array* initData)
{
    m_pendingKeyRequests.append(PendingKeyRequest(mimeType, initData));
    m_keyRequestTimer.startOneShot(0);
}

void MediaKeySession::keyRequestTimerFired(Timer<MediaKeySession>*)
{
    ASSERT(m_pendingKeyRequests.size());
    if (!m_session)
        return;

    while (!m_pendingKeyRequests.isEmpty()) {
        PendingKeyRequest request = m_pendingKeyRequests.takeFirst();

        // NOTE: Continued from step 5 in MediaKeys::createSession().
        // The user agent will asynchronously execute the following steps in the task:

        // 1. Let cdm be the cdm loaded in the MediaKeys constructor.
        // 2. Let destinationURL be null.
        String destinationURL;
        MediaKeyError::Code errorCode = 0;
        unsigned long systemCode = 0;

        // 3. Use cdm to generate a key request and follow the steps for the first matching condition from the following list:

        RefPtr<Uint8Array> keyRequest = m_session->generateKeyRequest(request.mimeType, request.initData.get(), destinationURL, errorCode, systemCode);

        // Otherwise [if a request is not successfully generated]:
        if (!keyRequest) {
            // 3.1. Create a new MediaKeyError object with the following attributes:
            //      code = the appropriate MediaKeyError code
            //      systemCode = a Key System-specific value, if provided, and 0 otherwise
            RefPtr<MediaKeyError> error = MediaKeyError::create(errorCode, systemCode).get();

            // 3.2. Set the MediaKeySession object's error attribute to the error object created in the previous step.
            setError(error.get());

            // 3.3. queue a task to fire a simple event named keyerror at the MediaKeySession object.
            RefPtr<Event> event = Event::create(eventNames().webkitkeyerrorEvent, false, false);
            event->setTarget(this);
            m_asyncEventQueue->enqueueEvent(event.release());

            // 3.4. Abort the task.
            continue;
        }

        // 4. queue a task to fire a simple event named keymessage at the new object
        //    The event is of type MediaKeyMessageEvent and has:
        //    message = key request
        //    destinationURL = destinationURL
        MediaKeyMessageEventInit init;
        init.bubbles = false;
        init.cancelable = false;
        init.message = keyRequest;
        init.destinationURL = destinationURL;
        RefPtr<MediaKeyMessageEvent> event = MediaKeyMessageEvent::create(eventNames().webkitkeymessageEvent, init);
        event->setTarget(this);
        m_asyncEventQueue->enqueueEvent(event);
    }
}

void MediaKeySession::update(Uint8Array* key, ExceptionCode& ec)
{
    // From <http://dvcs.w3.org/hg/html-media/raw-file/tip/encrypted-media/encrypted-media.html#dom-addkey>:
    // The addKey(key) method must run the following steps:
    // 1. If the first or second argument [sic] is null or an empty array, throw an INVALID_ACCESS_ERR.
    // NOTE: the reference to a "second argument" is a spec bug.
    if (!key || !key->length()) {
        ec = INVALID_ACCESS_ERR;
        return;
    }

    // 2. Schedule a task to handle the call, providing key.
    m_pendingKeys.append(key);
    m_addKeyTimer.startOneShot(0);
}

void MediaKeySession::addKeyTimerFired(Timer<MediaKeySession>*)
{
    ASSERT(m_pendingKeys.size());
    if (!m_session)
        return;

    while (!m_pendingKeys.isEmpty()) {
        RefPtr<Uint8Array> pendingKey = m_pendingKeys.takeFirst();
        unsigned short errorCode = 0;
        unsigned long systemCode = 0;

        // NOTE: Continued from step 2. of MediaKeySession::update()
        // 2.1. Let cdm be the cdm loaded in the MediaKeys constructor.
        // NOTE: This is m_session.
        // 2.2. Let 'did store key' be false.
        bool didStoreKey = false;
        // 2.3. Let 'next message' be null.
        RefPtr<Uint8Array> nextMessage;
        // 2.4. Use cdm to handle key.
        didStoreKey = m_session->update(pendingKey.get(), nextMessage, errorCode, systemCode);
        // 2.5. If did store key is true and the media element is waiting for a key, queue a task to attempt to resume playback.
        // TODO: Find and restart the media element

        // 2.6. If next message is not null, queue a task to fire a simple event named keymessage at the MediaKeySession object.
        //      The event is of type MediaKeyMessageEvent and has:
        //      message = next message
        //      destinationURL = null
        if (nextMessage) {
            MediaKeyMessageEventInit init;
            init.bubbles = false;
            init.cancelable = false;
            init.message = nextMessage;
            RefPtr<MediaKeyMessageEvent> event = MediaKeyMessageEvent::create(eventNames().webkitkeymessageEvent, init);
            event->setTarget(this);
            m_asyncEventQueue->enqueueEvent(event);
        }

        // 2.7. If did store key is true, queue a task to fire a simple event named keyadded at the MediaKeySession object.
        if (didStoreKey) {
            RefPtr<Event> keyaddedEvent = Event::create(eventNames().webkitkeyaddedEvent, false, false);
            keyaddedEvent->setTarget(this);
            m_asyncEventQueue->enqueueEvent(keyaddedEvent);
        }

        // 2.8. If any of the preceding steps in the task failed
        if (!didStoreKey) {
            // 2.8.1. Create a new MediaKeyError object with the following attributes:
            //        code = the appropriate MediaKeyError code
            //        systemCode = a Key System-specific value, if provided, and 0 otherwise
            RefPtr<MediaKeyError> error = MediaKeyError::create(errorCode, systemCode).get();

            // 2.8.2. Set the MediaKeySession object's error attribute to the error object created in the previous step.
            setError(error.get());

            // 2.8.3. queue a task to fire a simple event named keyerror at the MediaKeySession object.
            RefPtr<Event> keyerrorEvent = Event::create(eventNames().webkitkeyerrorEvent, false, false);
            keyerrorEvent->setTarget(this);
            m_asyncEventQueue->enqueueEvent(keyerrorEvent.release());
            
            // 2.8.4. Abort the task.
            // NOTE: no-op
        }
    }
}

const AtomicString& MediaKeySession::interfaceName() const
{
    return eventNames().interfaceForMediaKeySession;
}

ScriptExecutionContext* MediaKeySession::scriptExecutionContext() const
{
    return ContextDestructionObserver::scriptExecutionContext();
}

}

#endif
