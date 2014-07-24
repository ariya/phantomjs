/*
 * Copyright (C) 2013 Google, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(THREADED_HTML_PARSER)

#include "BackgroundHTMLInputStream.h"

namespace WebCore {

BackgroundHTMLInputStream::BackgroundHTMLInputStream()
    : m_firstValidCheckpointIndex(0)
    , m_firstValidSegmentIndex(0)
{
}

void BackgroundHTMLInputStream::append(const String& input)
{
    m_current.append(SegmentedString(input));
    m_segments.append(input);
}

void BackgroundHTMLInputStream::close()
{
    m_current.close();
}

HTMLInputCheckpoint BackgroundHTMLInputStream::createCheckpoint()
{
    HTMLInputCheckpoint checkpoint = m_checkpoints.size();
    m_checkpoints.append(Checkpoint(m_current, m_segments.size()));
    return checkpoint;
}

void BackgroundHTMLInputStream::invalidateCheckpointsBefore(HTMLInputCheckpoint newFirstValidCheckpointIndex)
{
    ASSERT(newFirstValidCheckpointIndex < m_checkpoints.size());
    // There is nothing to do for the first valid checkpoint.
    if (m_firstValidCheckpointIndex == newFirstValidCheckpointIndex)
        return;

    ASSERT(newFirstValidCheckpointIndex > m_firstValidCheckpointIndex);
    const Checkpoint& lastInvalidCheckpoint = m_checkpoints[newFirstValidCheckpointIndex - 1];

    ASSERT(m_firstValidSegmentIndex <= lastInvalidCheckpoint.numberOfSegmentsAlreadyAppended);
    for (size_t i = m_firstValidSegmentIndex; i < lastInvalidCheckpoint.numberOfSegmentsAlreadyAppended; ++i)
        m_segments[i] = String();
    m_firstValidSegmentIndex = lastInvalidCheckpoint.numberOfSegmentsAlreadyAppended;

    for (size_t i = m_firstValidCheckpointIndex; i < newFirstValidCheckpointIndex; ++i)
        m_checkpoints[i].clear();
    m_firstValidCheckpointIndex = newFirstValidCheckpointIndex;
}

void BackgroundHTMLInputStream::rewindTo(HTMLInputCheckpoint checkpointIndex, const String& unparsedInput)
{
    ASSERT(checkpointIndex < m_checkpoints.size()); // If this ASSERT fires, checkpointIndex is invalid.
    const Checkpoint& checkpoint = m_checkpoints[checkpointIndex];
    ASSERT(!checkpoint.isNull());

    bool isClosed = m_current.isClosed();

    m_current = checkpoint.input;

    for (size_t i = checkpoint.numberOfSegmentsAlreadyAppended; i < m_segments.size(); ++i) {
        ASSERT(!m_segments[i].isNull());
        m_current.append(SegmentedString(m_segments[i]));
    }

    if (!unparsedInput.isEmpty())
        m_current.prepend(SegmentedString(unparsedInput));

    if (isClosed && !m_current.isClosed())
        m_current.close();

    ASSERT(m_current.isClosed() == isClosed);

    m_segments.clear();
    m_checkpoints.clear();
    m_firstValidCheckpointIndex = 0;
    m_firstValidSegmentIndex = 0;
}

}

#endif // ENABLE(THREADED_HTML_PARSER)
