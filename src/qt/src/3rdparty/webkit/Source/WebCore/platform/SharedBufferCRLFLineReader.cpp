/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "SharedBufferCRLFLineReader.h"

#include "SharedBuffer.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

SharedBufferCRLFLineReader::SharedBufferCRLFLineReader(SharedBuffer* buffer)
    : m_buffer(buffer)
    , m_bufferPosition(0)
    , m_segment(0)
    , m_segmentLength(0)
    , m_segmentIndex(0)
    , m_reachedEndOfFile(false)
{
}

String SharedBufferCRLFLineReader::nextLine()
{
    if (m_reachedEndOfFile)
        return String();

    bool previousCharacterWasCR = false;
    StringBuilder stringBuilder;
    while (true) {
        bool reachedEndOfLine = false;
        while (m_segmentIndex < m_segmentLength) {
            reachedEndOfLine = false;
            char currentCharacter = m_segment[m_segmentIndex];
            if (previousCharacterWasCR) {
                if (currentCharacter == '\n')
                    reachedEndOfLine = true;
                else
                    stringBuilder.append('\r');
            } else if (currentCharacter != '\r')
                stringBuilder.append(currentCharacter);

            previousCharacterWasCR = (currentCharacter == '\r');
            m_segmentIndex++;
            if (reachedEndOfLine)
                return stringBuilder.toString();
        }

        // Read the next segment.
        m_segmentIndex = 0;
        m_bufferPosition += m_segmentLength;
        m_segmentLength = m_buffer->getSomeData(m_segment, m_bufferPosition);
        if (!m_segmentLength) {
            m_reachedEndOfFile = true;
            if (previousCharacterWasCR)
                stringBuilder.append('\r');
            String line = stringBuilder.toString();
            return (line.isEmpty() && !reachedEndOfLine) ? String() : line;
        }
    }
    return String();
}

}
