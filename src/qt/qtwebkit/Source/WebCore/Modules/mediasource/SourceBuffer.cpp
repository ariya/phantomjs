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

#include "config.h"
#include "SourceBuffer.h"

#if ENABLE(MEDIA_SOURCE)

#include "MediaSource.h"
#include "SourceBufferPrivate.h"
#include "TimeRanges.h"
#include <wtf/Uint8Array.h>

namespace WebCore {

PassRefPtr<SourceBuffer> SourceBuffer::create(PassOwnPtr<SourceBufferPrivate> sourceBufferPrivate, PassRefPtr<MediaSource> source)
{
    return adoptRef(new SourceBuffer(sourceBufferPrivate, source));
}

SourceBuffer::SourceBuffer(PassOwnPtr<SourceBufferPrivate> sourceBufferPrivate, PassRefPtr<MediaSource> source)
    : m_private(sourceBufferPrivate)
    , m_source(source)
    , m_timestampOffset(0)
{
    ASSERT(m_private);
    ASSERT(m_source);
}

SourceBuffer::~SourceBuffer()
{
}

PassRefPtr<TimeRanges> SourceBuffer::buffered(ExceptionCode& ec) const
{
    // Section 3.1 buffered attribute steps.
    // 1. If this object has been removed from the sourceBuffers attribute of the parent media source then throw an
    //    INVALID_STATE_ERR exception and abort these steps.
    if (isRemoved()) {
        ec = INVALID_STATE_ERR;
        return 0;
    }

    // 2. Return a new static normalized TimeRanges object for the media segments buffered.
    return m_private->buffered();
}

double SourceBuffer::timestampOffset() const
{
    return m_timestampOffset;
}

void SourceBuffer::setTimestampOffset(double offset, ExceptionCode& ec)
{
    // Section 3.1 timestampOffset attribute setter steps.
    // 1. If this object has been removed from the sourceBuffers attribute of the parent media source then throw an
    //    INVALID_STATE_ERR exception and abort these steps.
    if (isRemoved()) {
        ec = INVALID_STATE_ERR;
        return;
    }

    // 4. If the readyState attribute of the parent media source is in the "ended" state then run the following steps:
    if (isEnded()) {
        // 4.1 Set the readyState attribute of the parent media source to "open"
        // 4.2 Queue a task to fire a simple event named sourceopen at the parent media source.
        m_source->setReadyState(MediaSource::openKeyword());
    }

    // 5. If this object is waiting for the end of a media segment to be appended, then throw an INVALID_STATE_ERR
    // and abort these steps.
    if (!m_private->setTimestampOffset(offset)) {
        ec = INVALID_STATE_ERR;
        return;
    }

    // 6. Update the attribute to the new value.
    m_timestampOffset = offset;
}

void SourceBuffer::append(PassRefPtr<Uint8Array> data, ExceptionCode& ec)
{
    // SourceBuffer.append() steps from October 1st version of the Media Source Extensions spec.
    // https://dvcs.w3.org/hg/html-media/raw-file/7bab66368f2c/media-source/media-source.html#dom-append

    // 2. If data is null then throw an INVALID_ACCESS_ERR exception and abort these steps.
    if (!data) {
        ec = INVALID_ACCESS_ERR;
        return;
    }

    // 3. If this object has been removed from the sourceBuffers attribute of media source then throw
    //    an INVALID_STATE_ERR exception and abort these steps.
    if (isRemoved()) {
        ec = INVALID_STATE_ERR;
        return;
    }

    // 5. If the readyState attribute of media source is in the "ended" state then run the following steps:
    if (isEnded()) {
        // 5.1. Set the readyState attribute of media source to "open"
        // 5.2. Queue a task to fire a simple event named sourceopen at media source.
        m_source->setReadyState(MediaSource::openKeyword());
    }

    // Steps 6 & beyond are handled by the private implementation.
    m_private->append(data->data(), data->length());
}

void SourceBuffer::abort(ExceptionCode& ec)
{
    // Section 3.2 abort() method steps.
    // 1. If this object has been removed from the sourceBuffers attribute of the parent media source
    //    then throw an INVALID_STATE_ERR exception and abort these steps.
    // 2. If the readyState attribute of the parent media source is not in the "open" state
    //    then throw an INVALID_STATE_ERR exception and abort these steps.
    if (isRemoved() || !isOpen()) {
        ec = INVALID_STATE_ERR;
        return;
    }

    // 4. Run the reset parser state algorithm.
    m_private->abort();
}

void SourceBuffer::removedFromMediaSource()
{
    if (isRemoved())
        return;

    m_private->removedFromMediaSource();
    m_source.clear();
}

bool SourceBuffer::isRemoved() const
{
    return !m_source;
}

bool SourceBuffer::isOpen() const
{
    ASSERT(m_source);
    return m_source->readyState() == MediaSource::openKeyword();
}

bool SourceBuffer::isEnded() const
{
    ASSERT(m_source);
    return m_source->readyState() == MediaSource::endedKeyword();
}

} // namespace WebCore

#endif
