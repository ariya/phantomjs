/*
 * Copyright (C) 2011 Ericsson AB. All rights reserved.
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

#if ENABLE(MEDIA_STREAM)

#include "UserMediaRequest.h"

#include "Dictionary.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "LocalMediaStream.h"
#include "MediaConstraintsImpl.h"
#include "MediaStreamCenter.h"
#include "MediaStreamDescriptor.h"
#include "SpaceSplitString.h"
#include "UserMediaController.h"

namespace WebCore {

static PassRefPtr<MediaConstraintsImpl> parseOptions(const Dictionary& options, const String& mediaType, ExceptionCode& ec)
{
    RefPtr<MediaConstraintsImpl> constraints;

    Dictionary constraintsDictionary;
    bool ok = options.get(mediaType, constraintsDictionary);
    if (ok && !constraintsDictionary.isUndefinedOrNull())
        constraints = MediaConstraintsImpl::create(constraintsDictionary, ec);
    else {
        bool mediaRequested = false;
        options.get(mediaType, mediaRequested);
        if (mediaRequested)
            constraints = MediaConstraintsImpl::create();
    }

    return constraints.release();
}

PassRefPtr<UserMediaRequest> UserMediaRequest::create(ScriptExecutionContext* context, UserMediaController* controller, const Dictionary& options, PassRefPtr<NavigatorUserMediaSuccessCallback> successCallback, PassRefPtr<NavigatorUserMediaErrorCallback> errorCallback, ExceptionCode& ec)
{
    RefPtr<MediaConstraintsImpl> audio = parseOptions(options, ASCIILiteral("audio"), ec);
    if (ec)
        return 0;

    RefPtr<MediaConstraintsImpl> video = parseOptions(options, ASCIILiteral("video"), ec);
    if (ec)
        return 0;

    if (!audio && !video)
        return 0;

    return adoptRef(new UserMediaRequest(context, controller, audio.release(), video.release(), successCallback, errorCallback));
}

UserMediaRequest::UserMediaRequest(ScriptExecutionContext* context, UserMediaController* controller, PassRefPtr<MediaConstraintsImpl> audio, PassRefPtr<MediaConstraintsImpl> video, PassRefPtr<NavigatorUserMediaSuccessCallback> successCallback, PassRefPtr<NavigatorUserMediaErrorCallback> errorCallback)
    : ContextDestructionObserver(context)
    , m_audio(audio)
    , m_video(video)
    , m_controller(controller)
    , m_successCallback(successCallback)
    , m_errorCallback(errorCallback)
{
}

UserMediaRequest::~UserMediaRequest()
{
}

bool UserMediaRequest::audio() const
{
    return m_audio;
}

bool UserMediaRequest::video() const
{
    return m_video;
}

MediaConstraints* UserMediaRequest::audioConstraints() const
{
    return m_audio.get();
}

MediaConstraints* UserMediaRequest::videoConstraints() const
{
    return m_video.get();
}

Document* UserMediaRequest::ownerDocument()
{
    if (m_scriptExecutionContext) {
        return toDocument(m_scriptExecutionContext);
    }

    return 0;
}

void UserMediaRequest::start()
{
    MediaStreamCenter::instance().queryMediaStreamSources(this);
}

void UserMediaRequest::didCompleteQuery(const MediaStreamSourceVector& audioSources, const MediaStreamSourceVector& videoSources)
{
    if (m_controller)
        m_controller->requestUserMedia(this, audioSources, videoSources);
}

void UserMediaRequest::succeed(const MediaStreamSourceVector& audioSources, const MediaStreamSourceVector& videoSources)
{
    if (!m_scriptExecutionContext)
        return;

    RefPtr<LocalMediaStream> stream = LocalMediaStream::create(m_scriptExecutionContext, audioSources, videoSources);
    m_successCallback->handleEvent(stream.get());
}

void UserMediaRequest::succeed(PassRefPtr<MediaStreamDescriptor> streamDescriptor)
{
    if (!m_scriptExecutionContext)
        return;

    RefPtr<LocalMediaStream> stream = LocalMediaStream::create(m_scriptExecutionContext, streamDescriptor);
    m_successCallback->handleEvent(stream.get());
}

void UserMediaRequest::fail()
{
    if (!m_scriptExecutionContext)
        return;

    if (m_errorCallback) {
        RefPtr<NavigatorUserMediaError> error = NavigatorUserMediaError::create(NavigatorUserMediaError::PERMISSION_DENIED);
        m_errorCallback->handleEvent(error.get());
    }
}

void UserMediaRequest::contextDestroyed()
{
    RefPtr<UserMediaRequest> protect(this);

    if (m_controller) {
        m_controller->cancelUserMediaRequest(this);
        m_controller = 0;
    }

    ContextDestructionObserver::contextDestroyed();
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
