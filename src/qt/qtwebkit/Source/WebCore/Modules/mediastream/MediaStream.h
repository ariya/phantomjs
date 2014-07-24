/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2011 Ericsson AB. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MediaStream_h
#define MediaStream_h

#if ENABLE(MEDIA_STREAM)

#include "ContextDestructionObserver.h"
#include "EventTarget.h"
#include "ExceptionBase.h"
#include "MediaStreamDescriptor.h"
#include "MediaStreamTrack.h"
#include "Timer.h"
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class MediaStream : public RefCounted<MediaStream>, public MediaStreamDescriptorClient, public EventTarget, public ContextDestructionObserver {
public:
    static PassRefPtr<MediaStream> create(ScriptExecutionContext*);
    static PassRefPtr<MediaStream> create(ScriptExecutionContext*, PassRefPtr<MediaStream>);
    static PassRefPtr<MediaStream> create(ScriptExecutionContext*, const MediaStreamTrackVector&);
    static PassRefPtr<MediaStream> create(ScriptExecutionContext*, PassRefPtr<MediaStreamDescriptor>);
    virtual ~MediaStream();

    // DEPRECATED
    String label() const { return m_descriptor->id(); }

    String id() const { return m_descriptor->id(); }

    void addTrack(PassRefPtr<MediaStreamTrack>, ExceptionCode&);
    void removeTrack(PassRefPtr<MediaStreamTrack>, ExceptionCode&);
    MediaStreamTrack* getTrackById(String);

    MediaStreamTrackVector getAudioTracks() const { return m_audioTracks; }
    MediaStreamTrackVector getVideoTracks() const { return m_videoTracks; }

    bool ended() const;

    DEFINE_ATTRIBUTE_EVENT_LISTENER(ended);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(addtrack);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(removetrack);

    // MediaStreamDescriptorClient
    virtual void streamEnded() OVERRIDE;

    virtual bool isLocal() const { return false; }

    MediaStreamDescriptor* descriptor() const { return m_descriptor.get(); }

    // EventTarget
    virtual const AtomicString& interfaceName() const OVERRIDE;
    virtual ScriptExecutionContext* scriptExecutionContext() const OVERRIDE;

    using RefCounted<MediaStream>::ref;
    using RefCounted<MediaStream>::deref;

protected:
    MediaStream(ScriptExecutionContext*, PassRefPtr<MediaStreamDescriptor>);

    // EventTarget
    virtual EventTargetData* eventTargetData() OVERRIDE;
    virtual EventTargetData* ensureEventTargetData() OVERRIDE;

    // ContextDestructionObserver
    virtual void contextDestroyed();

private:
    // EventTarget
    virtual void refEventTarget() OVERRIDE { ref(); }
    virtual void derefEventTarget() OVERRIDE { deref(); }

    // MediaStreamDescriptorClient
    virtual void addRemoteTrack(MediaStreamComponent*) OVERRIDE;
    virtual void removeRemoteTrack(MediaStreamComponent*) OVERRIDE;

    void scheduleDispatchEvent(PassRefPtr<Event>);
    void scheduledEventTimerFired(Timer<MediaStream>*);

    bool m_stopped;

    EventTargetData m_eventTargetData;

    MediaStreamTrackVector m_audioTracks;
    MediaStreamTrackVector m_videoTracks;
    RefPtr<MediaStreamDescriptor> m_descriptor;

    Timer<MediaStream> m_scheduledEventTimer;
    Vector<RefPtr<Event> > m_scheduledEvents;
};

typedef Vector<RefPtr<MediaStream> > MediaStreamVector;

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

#endif // MediaStream_h
