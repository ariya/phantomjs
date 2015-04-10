/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef TextTrackCueList_h
#define TextTrackCueList_h

#if ENABLE(VIDEO_TRACK)

#include "TextTrackCue.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {

class TextTrackCueList : public RefCounted<TextTrackCueList> {
public:
    static PassRefPtr<TextTrackCueList> create()
    {
        return adoptRef(new TextTrackCueList);
    }

    ~TextTrackCueList() { }

    unsigned long length() const;
    unsigned long getCueIndex(TextTrackCue*) const;

    TextTrackCue* item(unsigned index) const;
    TextTrackCue* getCueById(const String&) const;
    TextTrackCueList* activeCues();

    bool add(PassRefPtr<TextTrackCue>);
    bool remove(TextTrackCue*);
    bool contains(TextTrackCue*) const;
    
    bool updateCueIndex(TextTrackCue*);

private:
    TextTrackCueList();
    bool add(PassRefPtr<TextTrackCue>, size_t, size_t);
    void clear();
    void invalidateCueIndexes(size_t);

    Vector<RefPtr<TextTrackCue> > m_list;
    RefPtr<TextTrackCueList> m_activeCues;

};

} // namespace WebCore

#endif
#endif
