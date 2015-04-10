/*
 * Copyright (C) 2011 Ericsson AB. All rights reserved.
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

#ifndef UserMediaRequest_h
#define UserMediaRequest_h

#if ENABLE(MEDIA_STREAM)

#include "ActiveDOMObject.h"
#include "ExceptionBase.h"
#include "MediaStreamSource.h"
#include "MediaStreamSourcesQueryClient.h"
#include "NavigatorUserMediaErrorCallback.h"
#include "NavigatorUserMediaSuccessCallback.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class Dictionary;
class Document;
class MediaConstraints;
class MediaConstraintsImpl;
class MediaStreamDescriptor;
class UserMediaController;

class UserMediaRequest : public MediaStreamSourcesQueryClient, public ContextDestructionObserver {
public:
    static PassRefPtr<UserMediaRequest> create(ScriptExecutionContext*, UserMediaController*, const Dictionary& options, PassRefPtr<NavigatorUserMediaSuccessCallback>, PassRefPtr<NavigatorUserMediaErrorCallback>, ExceptionCode&);
    ~UserMediaRequest();

    NavigatorUserMediaSuccessCallback* successCallback() const { return m_successCallback.get(); }
    NavigatorUserMediaErrorCallback* errorCallback() const { return m_errorCallback.get(); }
    Document* ownerDocument();

    void start();

    void succeed(const MediaStreamSourceVector& audioSources, const MediaStreamSourceVector& videoSources);
    void succeed(PassRefPtr<MediaStreamDescriptor>);
    void fail();

    MediaConstraints* audioConstraints() const;
    MediaConstraints* videoConstraints() const;

    // MediaStreamSourcesQueryClient
    virtual bool audio() const;
    virtual bool video() const;
    virtual void didCompleteQuery(const MediaStreamSourceVector& audioSources, const MediaStreamSourceVector& videoSources);

    // ContextDestructionObserver
    virtual void contextDestroyed();

private:
    UserMediaRequest(ScriptExecutionContext*, UserMediaController*, PassRefPtr<MediaConstraintsImpl> audio, PassRefPtr<MediaConstraintsImpl> video, PassRefPtr<NavigatorUserMediaSuccessCallback>, PassRefPtr<NavigatorUserMediaErrorCallback>);

    RefPtr<MediaConstraintsImpl> m_audio;
    RefPtr<MediaConstraintsImpl> m_video;

    UserMediaController* m_controller;

    RefPtr<NavigatorUserMediaSuccessCallback> m_successCallback;
    RefPtr<NavigatorUserMediaErrorCallback> m_errorCallback;
};

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

#endif // UserMediaRequest_h
