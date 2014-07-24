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

#ifndef MediaStreamCenter_h
#define MediaStreamCenter_h

#if ENABLE(MEDIA_STREAM)

#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class MediaStreamComponent;
class MediaStreamDescriptor;
class MediaStreamSourcesQueryClient;

class MediaStreamCenter {
public:
    virtual ~MediaStreamCenter();

    static MediaStreamCenter& instance();

    virtual void queryMediaStreamSources(PassRefPtr<MediaStreamSourcesQueryClient>) = 0;

    // FIXME: add a way to mute a MediaStreamSource from the WebKit API layer

    // Calls from the DOM objects to notify the platform
    virtual void didSetMediaStreamTrackEnabled(MediaStreamDescriptor*, MediaStreamComponent*) = 0;
    virtual bool didAddMediaStreamTrack(MediaStreamDescriptor*, MediaStreamComponent*) = 0;
    virtual bool didRemoveMediaStreamTrack(MediaStreamDescriptor*, MediaStreamComponent*) = 0;
    virtual void didStopLocalMediaStream(MediaStreamDescriptor*) = 0;
    virtual void didCreateMediaStream(MediaStreamDescriptor*) = 0;

protected:
    MediaStreamCenter();

    void endLocalMediaStream(MediaStreamDescriptor*);
    void addMediaStreamTrack(MediaStreamDescriptor*, MediaStreamComponent*);
    void removeMediaStreamTrack(MediaStreamDescriptor*, MediaStreamComponent*);
};

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

#endif // MediaStreamCenter_h
