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

#include "config.h"
#include "LocalMediaStream.h"

#if ENABLE(MEDIA_STREAM)

#include "MediaStreamCenter.h"
#include "UUID.h"

namespace WebCore {

PassRefPtr<LocalMediaStream> LocalMediaStream::create(ScriptExecutionContext* context, const MediaStreamSourceVector& audioSources, const MediaStreamSourceVector& videoSources)
{
    return adoptRef(new LocalMediaStream(context, MediaStreamDescriptor::create(createCanonicalUUIDString(), audioSources, videoSources)));
}

PassRefPtr<LocalMediaStream> LocalMediaStream::create(ScriptExecutionContext* context, PassRefPtr<MediaStreamDescriptor> streamDescriptor)
{
    return adoptRef(new LocalMediaStream(context, streamDescriptor));
}

LocalMediaStream::LocalMediaStream(ScriptExecutionContext* context, PassRefPtr<MediaStreamDescriptor> streamDescriptor)
    : MediaStream(context, streamDescriptor)
{
}

void LocalMediaStream::stop()
{
    if (ended())
        return;

    MediaStreamCenter::instance().didStopLocalMediaStream(descriptor());

    streamEnded();
}

LocalMediaStream::~LocalMediaStream()
{
}

const AtomicString& LocalMediaStream::interfaceName() const
{
    return eventNames().interfaceForLocalMediaStream;
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
