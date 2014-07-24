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

#include "config.h"

#if ENABLE(MEDIA_STREAM)

#include "RTCPeerConnection.h"

#include "ArrayValue.h"
#include "Document.h"
#include "Event.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "MediaConstraintsImpl.h"
#include "MediaStreamEvent.h"
#include "RTCConfiguration.h"
#include "RTCDTMFSender.h"
#include "RTCDataChannel.h"
#include "RTCDataChannelEvent.h"
#include "RTCDataChannelHandler.h"
#include "RTCErrorCallback.h"
#include "RTCIceCandidate.h"
#include "RTCIceCandidateDescriptor.h"
#include "RTCIceCandidateEvent.h"
#include "RTCSessionDescription.h"
#include "RTCSessionDescriptionCallback.h"
#include "RTCSessionDescriptionDescriptor.h"
#include "RTCSessionDescriptionRequestImpl.h"
#include "RTCStatsCallback.h"
#include "RTCStatsRequestImpl.h"
#include "RTCVoidRequestImpl.h"
#include "ScriptExecutionContext.h"
#include "VoidCallback.h"

namespace WebCore {

PassRefPtr<RTCConfiguration> RTCPeerConnection::parseConfiguration(const Dictionary& configuration, ExceptionCode& ec)
{
    if (configuration.isUndefinedOrNull())
        return 0;

    ArrayValue iceServers;
    bool ok = configuration.get("iceServers", iceServers);
    if (!ok || iceServers.isUndefinedOrNull()) {
        ec = TYPE_MISMATCH_ERR;
        return 0;
    }

    size_t numberOfServers;
    ok = iceServers.length(numberOfServers);
    if (!ok) {
        ec = TYPE_MISMATCH_ERR;
        return 0;
    }

    RefPtr<RTCConfiguration> rtcConfiguration = RTCConfiguration::create();

    for (size_t i = 0; i < numberOfServers; ++i) {
        Dictionary iceServer;
        ok = iceServers.get(i, iceServer);
        if (!ok) {
            ec = TYPE_MISMATCH_ERR;
            return 0;
        }

        String urlString, credential;
        ok = iceServer.get("url", urlString);
        if (!ok) {
            ec = TYPE_MISMATCH_ERR;
            return 0;
        }
        KURL url(KURL(), urlString);
        if (!url.isValid() || !(url.protocolIs("turn") || url.protocolIs("stun"))) {
            ec = TYPE_MISMATCH_ERR;
            return 0;
        }

        iceServer.get("credential", credential);

        rtcConfiguration->appendServer(RTCIceServer::create(url, credential));
    }

    return rtcConfiguration.release();
}

PassRefPtr<RTCPeerConnection> RTCPeerConnection::create(ScriptExecutionContext* context, const Dictionary& rtcConfiguration, const Dictionary& mediaConstraints, ExceptionCode& ec)
{
    RefPtr<RTCConfiguration> configuration = parseConfiguration(rtcConfiguration, ec);
    if (ec)
        return 0;

    RefPtr<MediaConstraints> constraints = MediaConstraintsImpl::create(mediaConstraints, ec);
    if (ec)
        return 0;

    RefPtr<RTCPeerConnection> peerConnection = adoptRef(new RTCPeerConnection(context, configuration.release(), constraints.release(), ec));
    peerConnection->suspendIfNeeded();
    if (ec)
        return 0;

    return peerConnection.release();
}

RTCPeerConnection::RTCPeerConnection(ScriptExecutionContext* context, PassRefPtr<RTCConfiguration> configuration, PassRefPtr<MediaConstraints> constraints, ExceptionCode& ec)
    : ActiveDOMObject(context)
    , m_signalingState(SignalingStateStable)
    , m_iceGatheringState(IceGatheringStateNew)
    , m_iceConnectionState(IceConnectionStateNew)
    , m_scheduledEventTimer(this, &RTCPeerConnection::scheduledEventTimerFired)
    , m_stopped(false)
{
    Document* document = toDocument(m_scriptExecutionContext);

    if (!document->frame()) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }

    m_peerHandler = RTCPeerConnectionHandler::create(this);
    if (!m_peerHandler) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }

    document->frame()->loader()->client()->dispatchWillStartUsingPeerConnectionHandler(m_peerHandler.get());

    if (!m_peerHandler->initialize(configuration, constraints)) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }
}

RTCPeerConnection::~RTCPeerConnection()
{
    stop();
}

void RTCPeerConnection::createOffer(PassRefPtr<RTCSessionDescriptionCallback> successCallback, PassRefPtr<RTCErrorCallback> errorCallback, const Dictionary& mediaConstraints, ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    if (!successCallback) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    RefPtr<MediaConstraints> constraints = MediaConstraintsImpl::create(mediaConstraints, ec);
    if (ec)
        return;

    RefPtr<RTCSessionDescriptionRequestImpl> request = RTCSessionDescriptionRequestImpl::create(scriptExecutionContext(), successCallback, errorCallback);
    m_peerHandler->createOffer(request.release(), constraints);
}

void RTCPeerConnection::createAnswer(PassRefPtr<RTCSessionDescriptionCallback> successCallback, PassRefPtr<RTCErrorCallback> errorCallback, const Dictionary& mediaConstraints, ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    if (!successCallback) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    RefPtr<MediaConstraints> constraints = MediaConstraintsImpl::create(mediaConstraints, ec);
    if (ec)
        return;

    RefPtr<RTCSessionDescriptionRequestImpl> request = RTCSessionDescriptionRequestImpl::create(scriptExecutionContext(), successCallback, errorCallback);
    m_peerHandler->createAnswer(request.release(), constraints.release());
}

void RTCPeerConnection::setLocalDescription(PassRefPtr<RTCSessionDescription> prpSessionDescription, PassRefPtr<VoidCallback> successCallback, PassRefPtr<RTCErrorCallback> errorCallback, ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    RefPtr<RTCSessionDescription> sessionDescription = prpSessionDescription;
    if (!sessionDescription) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    RefPtr<RTCVoidRequestImpl> request = RTCVoidRequestImpl::create(scriptExecutionContext(), successCallback, errorCallback);
    m_peerHandler->setLocalDescription(request.release(), sessionDescription->descriptor());
}

PassRefPtr<RTCSessionDescription> RTCPeerConnection::localDescription(ExceptionCode& ec)
{
    RefPtr<RTCSessionDescriptionDescriptor> descriptor = m_peerHandler->localDescription();
    if (!descriptor)
        return 0;

    RefPtr<RTCSessionDescription> sessionDescription = RTCSessionDescription::create(descriptor.release());
    return sessionDescription.release();
}

void RTCPeerConnection::setRemoteDescription(PassRefPtr<RTCSessionDescription> prpSessionDescription, PassRefPtr<VoidCallback> successCallback, PassRefPtr<RTCErrorCallback> errorCallback, ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    RefPtr<RTCSessionDescription> sessionDescription = prpSessionDescription;
    if (!sessionDescription) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    RefPtr<RTCVoidRequestImpl> request = RTCVoidRequestImpl::create(scriptExecutionContext(), successCallback, errorCallback);
    m_peerHandler->setRemoteDescription(request.release(), sessionDescription->descriptor());
}

PassRefPtr<RTCSessionDescription> RTCPeerConnection::remoteDescription(ExceptionCode& ec)
{
    RefPtr<RTCSessionDescriptionDescriptor> descriptor = m_peerHandler->remoteDescription();
    if (!descriptor)
        return 0;

    RefPtr<RTCSessionDescription> desc = RTCSessionDescription::create(descriptor.release());
    return desc.release();
}

void RTCPeerConnection::updateIce(const Dictionary& rtcConfiguration, const Dictionary& mediaConstraints, ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    RefPtr<RTCConfiguration> configuration = parseConfiguration(rtcConfiguration, ec);
    if (ec)
        return;

    RefPtr<MediaConstraints> constraints = MediaConstraintsImpl::create(mediaConstraints, ec);
    if (ec)
        return;

    bool valid = m_peerHandler->updateIce(configuration, constraints);
    if (!valid)
        ec = SYNTAX_ERR;
}

void RTCPeerConnection::addIceCandidate(RTCIceCandidate* iceCandidate, ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    if (!iceCandidate) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    bool valid = m_peerHandler->addIceCandidate(iceCandidate->descriptor());
    if (!valid)
        ec = SYNTAX_ERR;
}

String RTCPeerConnection::signalingState() const
{
    switch (m_signalingState) {
    case SignalingStateStable:
        return ASCIILiteral("stable");
    case SignalingStateHaveLocalOffer:
        return ASCIILiteral("have-local-offer");
    case SignalingStateHaveRemoteOffer:
        return ASCIILiteral("have-remote-offer");
    case SignalingStateHaveLocalPrAnswer:
        return ASCIILiteral("have-local-pranswer");
    case SignalingStateHaveRemotePrAnswer:
        return ASCIILiteral("have-remote-pranswer");
    case SignalingStateClosed:
        return ASCIILiteral("closed");
    }

    ASSERT_NOT_REACHED();
    return String();
}

String RTCPeerConnection::iceGatheringState() const
{
    switch (m_iceGatheringState) {
    case IceGatheringStateNew:
        return ASCIILiteral("new");
    case IceGatheringStateGathering:
        return ASCIILiteral("gathering");
    case IceGatheringStateComplete:
        return ASCIILiteral("complete");
    }

    ASSERT_NOT_REACHED();
    return String();
}

String RTCPeerConnection::iceConnectionState() const
{
    switch (m_iceConnectionState) {
    case IceConnectionStateNew:
        return ASCIILiteral("new");
    case IceConnectionStateChecking:
        return ASCIILiteral("checking");
    case IceConnectionStateConnected:
        return ASCIILiteral("connected");
    case IceConnectionStateCompleted:
        return ASCIILiteral("completed");
    case IceConnectionStateFailed:
        return ASCIILiteral("failed");
    case IceConnectionStateDisconnected:
        return ASCIILiteral("disconnected");
    case IceConnectionStateClosed:
        return ASCIILiteral("closed");
    }

    ASSERT_NOT_REACHED();
    return String();
}

void RTCPeerConnection::addStream(PassRefPtr<MediaStream> prpStream, const Dictionary& mediaConstraints, ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    RefPtr<MediaStream> stream = prpStream;
    if (!stream) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    if (m_localStreams.contains(stream))
        return;

    RefPtr<MediaConstraints> constraints = MediaConstraintsImpl::create(mediaConstraints, ec);
    if (ec)
        return;

    m_localStreams.append(stream);

    bool valid = m_peerHandler->addStream(stream->descriptor(), constraints);
    if (!valid)
        ec = SYNTAX_ERR;
}

void RTCPeerConnection::removeStream(PassRefPtr<MediaStream> prpStream, ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    if (!prpStream) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    RefPtr<MediaStream> stream = prpStream;

    size_t pos = m_localStreams.find(stream);
    if (pos == notFound)
        return;

    m_localStreams.remove(pos);

    m_peerHandler->removeStream(stream->descriptor());
}

MediaStreamVector RTCPeerConnection::getLocalStreams() const
{
    return m_localStreams;
}

MediaStreamVector RTCPeerConnection::getRemoteStreams() const
{
    return m_remoteStreams;
}

MediaStream* RTCPeerConnection::getStreamById(const String& streamId)
{
    for (MediaStreamVector::iterator iter = m_localStreams.begin(); iter != m_localStreams.end(); ++iter) {
        if ((*iter)->id() == streamId)
            return iter->get();
    }

    for (MediaStreamVector::iterator iter = m_remoteStreams.begin(); iter != m_remoteStreams.end(); ++iter) {
        if ((*iter)->id() == streamId)
            return iter->get();
    }

    return 0;
}

void RTCPeerConnection::getStats(PassRefPtr<RTCStatsCallback> successCallback, PassRefPtr<MediaStreamTrack> selector)
{
    RefPtr<RTCStatsRequestImpl> statsRequest = RTCStatsRequestImpl::create(scriptExecutionContext(), successCallback, selector);
    // FIXME: Add passing selector as part of the statsRequest.
    m_peerHandler->getStats(statsRequest.release());
}

PassRefPtr<RTCDataChannel> RTCPeerConnection::createDataChannel(String label, const Dictionary& options, ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return 0;
    }

    bool reliable = true;
    options.get("reliable", reliable);
    RefPtr<RTCDataChannel> channel = RTCDataChannel::create(scriptExecutionContext(), m_peerHandler.get(), label, reliable, ec);
    if (ec)
        return 0;
    m_dataChannels.append(channel);
    return channel.release();
}

bool RTCPeerConnection::hasLocalStreamWithTrackId(const String& trackId)
{
    for (MediaStreamVector::iterator iter = m_localStreams.begin(); iter != m_localStreams.end(); ++iter) {
        if ((*iter)->getTrackById(trackId))
            return true;
    }
    return false;
}

PassRefPtr<RTCDTMFSender> RTCPeerConnection::createDTMFSender(PassRefPtr<MediaStreamTrack> prpTrack, ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return 0;
    }

    if (!prpTrack) {
        ec = TypeError;
        return 0;
    }

    RefPtr<MediaStreamTrack> track = prpTrack;

    if (!hasLocalStreamWithTrackId(track->id())) {
        ec = SYNTAX_ERR;
        return 0;
    }

    RefPtr<RTCDTMFSender> dtmfSender = RTCDTMFSender::create(scriptExecutionContext(), m_peerHandler.get(), track.release(), ec);
    if (ec)
        return 0;
    return dtmfSender.release();
}

void RTCPeerConnection::close(ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    m_peerHandler->stop();

    changeIceConnectionState(IceConnectionStateClosed);
    changeIceGatheringState(IceGatheringStateComplete);
    changeSignalingState(SignalingStateClosed);
}

void RTCPeerConnection::negotiationNeeded()
{
    scheduleDispatchEvent(Event::create(eventNames().negotiationneededEvent, false, false));
}

void RTCPeerConnection::didGenerateIceCandidate(PassRefPtr<RTCIceCandidateDescriptor> iceCandidateDescriptor)
{
    ASSERT(scriptExecutionContext()->isContextThread());
    if (!iceCandidateDescriptor)
        scheduleDispatchEvent(RTCIceCandidateEvent::create(false, false, 0));
    else {
        RefPtr<RTCIceCandidate> iceCandidate = RTCIceCandidate::create(iceCandidateDescriptor);
        scheduleDispatchEvent(RTCIceCandidateEvent::create(false, false, iceCandidate.release()));
    }
}

void RTCPeerConnection::didChangeSignalingState(SignalingState newState)
{
    ASSERT(scriptExecutionContext()->isContextThread());
    changeSignalingState(newState);
}

void RTCPeerConnection::didChangeIceGatheringState(IceGatheringState newState)
{
    ASSERT(scriptExecutionContext()->isContextThread());
    changeIceGatheringState(newState);
}

void RTCPeerConnection::didChangeIceConnectionState(IceConnectionState newState)
{
    ASSERT(scriptExecutionContext()->isContextThread());
    changeIceConnectionState(newState);
}

void RTCPeerConnection::didAddRemoteStream(PassRefPtr<MediaStreamDescriptor> streamDescriptor)
{
    ASSERT(scriptExecutionContext()->isContextThread());

    if (m_signalingState == SignalingStateClosed)
        return;

    RefPtr<MediaStream> stream = MediaStream::create(scriptExecutionContext(), streamDescriptor);
    m_remoteStreams.append(stream);

    scheduleDispatchEvent(MediaStreamEvent::create(eventNames().addstreamEvent, false, false, stream.release()));
}

void RTCPeerConnection::didRemoveRemoteStream(MediaStreamDescriptor* streamDescriptor)
{
    ASSERT(scriptExecutionContext()->isContextThread());
    ASSERT(streamDescriptor->client());

    RefPtr<MediaStream> stream = static_cast<MediaStream*>(streamDescriptor->client());
    stream->streamEnded();

    if (m_signalingState == SignalingStateClosed)
        return;

    size_t pos = m_remoteStreams.find(stream);
    ASSERT(pos != notFound);
    m_remoteStreams.remove(pos);

    scheduleDispatchEvent(MediaStreamEvent::create(eventNames().removestreamEvent, false, false, stream.release()));
}

void RTCPeerConnection::didAddRemoteDataChannel(PassOwnPtr<RTCDataChannelHandler> handler)
{
    ASSERT(scriptExecutionContext()->isContextThread());

    if (m_signalingState == SignalingStateClosed)
        return;

    RefPtr<RTCDataChannel> channel = RTCDataChannel::create(scriptExecutionContext(), handler);
    m_dataChannels.append(channel);

    scheduleDispatchEvent(RTCDataChannelEvent::create(eventNames().datachannelEvent, false, false, channel.release()));
}

const AtomicString& RTCPeerConnection::interfaceName() const
{
    return eventNames().interfaceForRTCPeerConnection;
}

ScriptExecutionContext* RTCPeerConnection::scriptExecutionContext() const
{
    return ActiveDOMObject::scriptExecutionContext();
}

void RTCPeerConnection::stop()
{
    if (m_stopped)
        return;

    m_stopped = true;
    m_iceConnectionState = IceConnectionStateClosed;
    m_signalingState = SignalingStateClosed;

    Vector<RefPtr<RTCDataChannel> >::iterator i = m_dataChannels.begin();
    for (; i != m_dataChannels.end(); ++i)
        (*i)->stop();
}

EventTargetData* RTCPeerConnection::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* RTCPeerConnection::ensureEventTargetData()
{
    return &m_eventTargetData;
}

void RTCPeerConnection::changeSignalingState(SignalingState signalingState)
{
    if (m_signalingState != SignalingStateClosed && m_signalingState != signalingState) {
        m_signalingState = signalingState;
        scheduleDispatchEvent(Event::create(eventNames().signalingstatechangeEvent, false, false));
    }
}

void RTCPeerConnection::changeIceGatheringState(IceGatheringState iceGatheringState)
{
    m_iceGatheringState = iceGatheringState;
}

void RTCPeerConnection::changeIceConnectionState(IceConnectionState iceConnectionState)
{
    if (m_iceConnectionState != IceConnectionStateClosed && m_iceConnectionState != iceConnectionState) {
        m_iceConnectionState = iceConnectionState;
        scheduleDispatchEvent(Event::create(eventNames().iceconnectionstatechangeEvent, false, false));
    }
}

void RTCPeerConnection::scheduleDispatchEvent(PassRefPtr<Event> event)
{
    m_scheduledEvents.append(event);

    if (!m_scheduledEventTimer.isActive())
        m_scheduledEventTimer.startOneShot(0);
}

void RTCPeerConnection::scheduledEventTimerFired(Timer<RTCPeerConnection>*)
{
    if (m_stopped)
        return;

    Vector<RefPtr<Event> > events;
    events.swap(m_scheduledEvents);

    Vector<RefPtr<Event> >::iterator it = events.begin();
    for (; it != events.end(); ++it)
        dispatchEvent((*it).release());

    events.clear();
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
