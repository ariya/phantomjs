/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(VIDEO_TRACK)

#include "TextTrackCueList.h"

namespace WebCore {

TextTrackCueList::TextTrackCueList()
{
}

unsigned long TextTrackCueList::length() const
{
    return m_list.size();
}

unsigned long TextTrackCueList::getCueIndex(TextTrackCue* cue) const
{
    return m_list.find(cue);
}

TextTrackCue* TextTrackCueList::item(unsigned index) const
{
    if (index < m_list.size())
        return m_list[index].get();
    return 0;
}

TextTrackCue* TextTrackCueList::getCueById(const String& id) const
{
    for (size_t i = 0; i < m_list.size(); ++i) {
        if (m_list[i]->id() == id)
            return m_list[i].get();
    }
    return 0;
}

TextTrackCueList* TextTrackCueList::activeCues()
{
    if (!m_activeCues)
        m_activeCues = create();

    m_activeCues->clear();
    for (size_t i = 0; i < m_list.size(); ++i) {
        RefPtr<TextTrackCue> cue = m_list[i];
        if (cue->isActive())
            m_activeCues->add(cue);
    }
    return m_activeCues.get();
}

bool TextTrackCueList::add(PassRefPtr<TextTrackCue> cue)
{
    ASSERT(cue->startTime() >= 0);
    ASSERT(cue->endTime() >= 0);

    return add(cue, 0, m_list.size());
}

bool TextTrackCueList::add(PassRefPtr<TextTrackCue> prpCue, size_t start, size_t end)
{
    ASSERT_WITH_SECURITY_IMPLICATION(start <= m_list.size());
    ASSERT_WITH_SECURITY_IMPLICATION(end <= m_list.size());

    // Maintain text track cue order:
    // http://www.whatwg.org/specs/web-apps/current-work/#text-track-cue-order
    RefPtr<TextTrackCue> cue = prpCue;
    if (start == end) {
        if (!m_list.isEmpty() && (start > 0) && (m_list[start - 1].get() == cue.get()))
            return false;

        m_list.insert(start, cue);
        invalidateCueIndexes(start);
        return true;
    }

    size_t index = (start + end) / 2;
    if (cue->isOrderedBefore(m_list[index].get()))
        return add(cue.release(), start, index);

    return add(cue.release(), index + 1, end);
}

bool TextTrackCueList::remove(TextTrackCue* cue)
{
    size_t index = m_list.find(cue);
    if (index == notFound)
        return false;

    cue->setIsActive(false);
    m_list.remove(index);
    return true;
}

bool TextTrackCueList::contains(TextTrackCue* cue) const
{
    return m_list.contains(cue);
}

bool TextTrackCueList::updateCueIndex(TextTrackCue* cue)
{
    if (!contains(cue))
        return false;
    
    remove(cue);
    return add(cue);
}

void TextTrackCueList::clear()
{
    m_list.clear();
}

void TextTrackCueList::invalidateCueIndexes(size_t start)
{
    for (size_t i = start; i < m_list.size(); ++i)
        m_list[i]->invalidateCueIndex();
}

} // namespace WebCore

#endif
