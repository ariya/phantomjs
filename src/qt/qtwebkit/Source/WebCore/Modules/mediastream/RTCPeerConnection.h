/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 * 3. Neither the name of Google Inc. nor the names of its contributors
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

#ifndef RTCPeerConnection_h
#define RTCPeerConnection_h

#if ENABLE(MEDIA_STREAM)

#include "ActiveDOMObject.h"
#include "Dictionary.h"
#include "EventTarget.h"
#include "ExceptionBase.h"
#include "MediaStream.h"
#include "RTCIceCandidate.h"
#include "RTCPeerConnectionHandler.h"
#include "RTCPeerConnectionHandlerClient.h"
#include "Timer.h"
#include <wtf/RefCounted.h>

namespace WebCore {

class MediaConstraints;
class MediaStreamTrack;
class RTCConfiguration;
class RTCDTMFSender;
class RTCDataChannel;
class RTCErrorCallback;
class RTCSessionDescription;
class RTCSessionDescriptionCallback;
class RTCStatsCallback;
class VoidCallback;

class RTCPeerConnection : public RefCounted<RTCPeerConnection>, public RTCPeerConnectionHandlerClient, public EventTarget, public ActiveDOMObject {
public:
    static PassRefPtr<RTCPeerConnection> create(ScriptExecutionContext*, const Dictionary& rtcConfiguration, const Dictionary& mediaConstraints, ExceptionCode&);
    ~RTCPeerConnection();

    void createOffer(PassRefPtr<RTCSessionDescriptionCallback>, PassRefPtr<RTCErrorCallback>, const Dictionary& mediaConstraints, ExceptionCode&);

    void createAnswer(PassRefPtr<RTCSessionDescriptionCallback>, PassRefPtr<RTCErrorCallback>, const Dictionary& mediaConstraints, ExceptionCode&);

    void setLocalDescription(PassRefPtr<RTCSessionDescription>, PassRefPtr<VoidCallback>, PassRefPtr<RTCErrorCallback>, ExceptionCode&);
    PassRefPtr<RTCSessionDescription> localDescription(ExceptionCode&);

    void setRemoteDescription(PassRefPtr<RTCSessionDescription>, PassRefPtr<VoidCallback>, PassRefPtr<RTCErrorCallback>, ExceptionCode&);
    PassRefPtr<RTCSessionDescription> remoteDescription(ExceptionCode&);

    String signalingState() const;

    void updateIce(const Dictionary& rtcConfiguration, const Dictionary& mediaConstraints, ExceptionCode&);

    void addIceCandidate(RTCIceCandidate*, ExceptionCode&);

    String iceGatheringState() const;

    String iceConnectionState() const;

    MediaStreamVector getLocalStreams() const;

    MediaStreamVector getRemoteStreams() const;

    MediaStream* getStreamById(const String& streamId);

    void addStream(PassRefPtr<MediaStream>, const Dictionary& mediaConstraints, ExceptionCode&);

    void removeStream(PassRefPtr<MediaStream>, ExceptionCode&);

    void getStats(PassRefPtr<RTCStatsCallback> successCallback, PassRefPtr<MediaStreamTrack> selector);

    PassRefPtr<RTCDataChannel> createDataChannel(String label, const Dictionary& dataChannelDict, ExceptionCode&);

    PassRefPtr<RTCDTMFSender> createDTMFSender(PassRefPtr<MediaStreamTrack>, ExceptionCode&);

    void close(ExceptionCode&);

    DEFINE_ATTRIBUTE_EVENT_LISTENER(negotiationneeded);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(icecandidate);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(signalingstatechange);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(addstream);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(removestream);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(iceconnectionstatechange);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(datachannel);

    // RTCPeerConnectionHandlerClient
    virtual void negotiationNeeded() OVERRIDE;
    virtual void didGenerateIceCandidate(PassRefPtr<RTCIceCandidateDescriptor>) OVERRIDE;
    virtual void didChangeSignalingState(SignalingState) OVERRIDE;
    virtual void didChangeIceGatheringState(IceGatheringState) OVERRIDE;
    virtual void didChangeIceConnectionState(IceConnectionState) OVERRIDE;
    virtual void didAddRemoteStream(PassRefPtr<MediaStreamDescriptor>) OVERRIDE;
    virtual void didRemoveRemoteStream(MediaStreamDescriptor*) OVERRIDE;
    virtual void didAddRemoteDataChannel(PassOwnPtr<RTCDataChannelHandler>) OVERRIDE;

    // EventTarget
    virtual const AtomicString& interfaceName() const OVERRIDE;
    virtual ScriptExecutionContext* scriptExecutionContext() const OVERRIDE;

    // ActiveDOMObject
    virtual void stop() OVERRIDE;

    using RefCounted<RTCPeerConnection>::ref;
    using RefCounted<RTCPeerConnection>::deref;

private:
    RTCPeerConnection(ScriptExecutionContext*, PassRefPtr<RTCConfiguration>, PassRefPtr<MediaConstraints>, ExceptionCode&);

    static PassRefPtr<RTCConfiguration> parseConfiguration(const Dictionary& configuration, ExceptionCode&);
    void scheduleDispatchEvent(PassRefPtr<Event>);
    void scheduledEventTimerFired(Timer<RTCPeerConnection>*);
    bool hasLocalStreamWithTrackId(const String& trackId);

    // EventTarget implementation.
    virtual EventTargetData* eventTargetData();
    virtual EventTargetData* ensureEventTargetData();
    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }
    EventTargetData m_eventTargetData;

    void changeSignalingState(SignalingState);
    void changeIceGatheringState(IceGatheringState);
    void changeIceConnectionState(IceConnectionState);

    SignalingState m_signalingState;
    IceGatheringState m_iceGatheringState;
    IceConnectionState m_iceConnectionState;

    MediaStreamVector m_localStreams;
    MediaStreamVector m_remoteStreams;

    Vector<RefPtr<RTCDataChannel> > m_dataChannels;

    OwnPtr<RTCPeerConnectionHandler> m_peerHandler;

    Timer<RTCPeerConnection> m_scheduledEventTimer;
    Vector<RefPtr<Event> > m_scheduledEvents;

    bool m_stopped;
};

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

#endif // RTCPeerConnection_h
