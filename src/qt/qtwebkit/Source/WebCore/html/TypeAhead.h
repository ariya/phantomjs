/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef TypeAhead_h
#define TypeAhead_h

#include "DOMTimeStamp.h"
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class KeyboardEvent;

class TypeAheadDataSource {
public:
    virtual ~TypeAheadDataSource() { }

    virtual int indexOfSelectedOption() const = 0;
    virtual int optionCount() const = 0;
    virtual String optionAtIndex(int index) const = 0;
};

class TypeAhead {
public:
    TypeAhead(TypeAheadDataSource*);

    enum ModeFlag {
        MatchPrefix = 1 << 0,
        CycleFirstChar = 1 << 1,
        MatchIndex = 1 << 2,
    };
    typedef unsigned MatchModeFlags;

    // Returns the index for the matching option.
    int handleEvent(KeyboardEvent*, MatchModeFlags);

private:
    TypeAheadDataSource* m_dataSource;
    DOMTimeStamp m_lastTypeTime;
    UChar m_repeatingChar;
    StringBuilder m_buffer;
};

} // namespace WebCore

#endif // TypeAhead_h
