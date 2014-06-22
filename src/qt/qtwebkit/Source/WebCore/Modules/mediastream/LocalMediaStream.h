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

#ifndef LocalMediaStream_h
#define LocalMediaStream_h

#if ENABLE(MEDIA_STREAM)

#include "MediaStream.h"

namespace WebCore {

class LocalMediaStream : public MediaStream {
public:
    static PassRefPtr<LocalMediaStream> create(ScriptExecutionContext*, const MediaStreamSourceVector& audioSources, const MediaStreamSourceVector& videoSources);
    static PassRefPtr<LocalMediaStream> create(ScriptExecutionContext*, PassRefPtr<MediaStreamDescriptor>);
    virtual ~LocalMediaStream();

    void stop();

    // MediaStream
    virtual bool isLocal() const OVERRIDE { return true; }

    // EventTarget
    virtual const AtomicString& interfaceName() const OVERRIDE;

private:
    LocalMediaStream(ScriptExecutionContext*, PassRefPtr<MediaStreamDescriptor>);
};

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

#endif // LocalMediaStream_h
