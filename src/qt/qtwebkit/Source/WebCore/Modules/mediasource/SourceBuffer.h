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

#ifndef SourceBuffer_h
#define SourceBuffer_h

#if ENABLE(MEDIA_SOURCE)

#include "ExceptionCode.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
class MediaSource;
class SourceBufferPrivate;
class TimeRanges;

class SourceBuffer : public RefCounted<SourceBuffer> {
public:
    static PassRefPtr<SourceBuffer> create(PassOwnPtr<SourceBufferPrivate>, PassRefPtr<MediaSource>);

    virtual ~SourceBuffer();

    // SourceBuffer.idl methods
    PassRefPtr<TimeRanges> buffered(ExceptionCode&) const;
    double timestampOffset() const;
    void setTimestampOffset(double, ExceptionCode&);
    void append(PassRefPtr<Uint8Array> data, ExceptionCode&);
    void abort(ExceptionCode&);

    void removedFromMediaSource();

private:
    SourceBuffer(PassOwnPtr<SourceBufferPrivate>, PassRefPtr<MediaSource>);

    bool isRemoved() const;
    bool isOpen() const;
    bool isEnded() const;

    OwnPtr<SourceBufferPrivate> m_private;
    RefPtr<MediaSource> m_source;

    double m_timestampOffset;
};

} // namespace WebCore

#endif
#endif
