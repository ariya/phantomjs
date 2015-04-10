/*
 * Copyright (C) 2006 Lars Knoll <lars@trolltech.com>
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"
#include "TextBreakIterator.h"

#include <wtf/Atomics.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/WTFString.h>
#include <wtf/unicode/Unicode.h>

using namespace WTF;
using namespace WTF::Unicode;
using namespace std;

namespace WebCore {

// Hack, not entirely correct
static inline bool isCharStop(UChar c)
{
    CharCategory charCategory = category(c);
    return charCategory != Mark_NonSpacing && (charCategory != Other_Surrogate || (c < 0xd800 || c >= 0xdc00));
}

static inline bool isLineStop(UChar c)
{
    return category(c) != Separator_Line;
}

static inline bool isSentenceStop(UChar c)
{
    return isPunct(c);
}

class TextBreakIterator {
public:
    void reset(const UChar* str, int len)
    {
        string = str;
        length = len;
        currentPos = 0;
    }
    int first()
    {
        currentPos = 0;
        return currentPos;
    }
    int last()
    {
        currentPos = length;
        return currentPos;
    }
    virtual int next() = 0;
    virtual int previous() = 0;
    int following(int position)
    {
        currentPos = position;
        return next();
    }
    int preceding(int position)
    {
        currentPos = position;
        return previous();
    }

    int currentPos;
    const UChar* string;
    int length;
};

struct WordBreakIterator: TextBreakIterator {
    virtual int next();
    virtual int previous();
};

struct CharBreakIterator: TextBreakIterator {
    virtual int next();
    virtual int previous();
};

struct LineBreakIterator: TextBreakIterator {
    virtual int next();
    virtual int previous();
};

struct SentenceBreakIterator : TextBreakIterator {
    virtual int next();
    virtual int previous();
};

int WordBreakIterator::next()
{
    if (currentPos == length) {
        currentPos = -1;
        return currentPos;
    }
    bool haveSpace = false;
    while (currentPos < length) {
        if (haveSpace && !isSpace(string[currentPos]))
            break;
        if (isSpace(string[currentPos]))
            haveSpace = true;
        ++currentPos;
    }
    return currentPos;
}

int WordBreakIterator::previous()
{
    if (!currentPos) {
        currentPos = -1;
        return currentPos;
    }
    bool haveSpace = false;
    while (currentPos > 0) {
        if (haveSpace && !isSpace(string[currentPos]))
            break;
        if (isSpace(string[currentPos]))
            haveSpace = true;
        --currentPos;
    }
    return currentPos;
}

int CharBreakIterator::next()
{
    if (currentPos >= length)
        return -1;
    ++currentPos;
    while (currentPos < length && !isCharStop(string[currentPos]))
        ++currentPos;
    return currentPos;
}

int CharBreakIterator::previous()
{
    if (currentPos <= 0)
        return -1;
    if (currentPos > length)
        currentPos = length;
    --currentPos;
    while (currentPos > 0 && !isCharStop(string[currentPos]))
        --currentPos;
    return currentPos;
}

int LineBreakIterator::next()
{
    if (currentPos == length) {
        currentPos = -1;
        return currentPos;
    }
    bool haveSpace = false;
    while (currentPos < length) {
        if (haveSpace && !isLineStop(string[currentPos]))
            break;
        if (isLineStop(string[currentPos]))
            haveSpace = true;
        ++currentPos;
    }
    return currentPos;
}

int LineBreakIterator::previous()
{
    if (!currentPos) {
        currentPos = -1;
        return currentPos;
    }
    bool haveSpace = false;
    while (currentPos > 0) {
        if (haveSpace && !isLineStop(string[currentPos]))
            break;
        if (isLineStop(string[currentPos]))
            haveSpace = true;
        --currentPos;
    }
    return currentPos;
}

int SentenceBreakIterator::next()
{
    if (currentPos == length) {
        currentPos = -1;
        return currentPos;
    }
    bool haveSpace = false;
    while (currentPos < length) {
        if (haveSpace && !isSentenceStop(string[currentPos]))
            break;
        if (isSentenceStop(string[currentPos]))
            haveSpace = true;
        ++currentPos;
    }
    return currentPos;
}

int SentenceBreakIterator::previous()
{
    if (!currentPos) {
        currentPos = -1;
        return currentPos;
    }
    bool haveSpace = false;
    while (currentPos > 0) {
        if (haveSpace && !isSentenceStop(string[currentPos]))
            break;
        if (isSentenceStop(string[currentPos]))
            haveSpace = true;
        --currentPos;
    }
    return currentPos;
}

TextBreakIterator* wordBreakIterator(const UChar* string, int length)
{
    DEFINE_STATIC_LOCAL(WordBreakIterator, iterator, ());
    iterator.reset(string, length);
    return &iterator;
}

static CharBreakIterator* nonSharedCharacterBreakIterator;

NonSharedCharacterBreakIterator::NonSharedCharacterBreakIterator(const UChar* buffer, int length)
{
    m_iterator = nonSharedCharacterBreakIterator;
    bool createdIterator = m_iterator && weakCompareAndSwap(reinterpret_cast<void**>(&nonSharedCharacterBreakIterator), m_iterator, 0);
    if (!createdIterator)
        m_iterator = new CharBreakIterator;
    m_iterator->reset(buffer, length);
}

NonSharedCharacterBreakIterator::~NonSharedCharacterBreakIterator()
{
    if (!weakCompareAndSwap(reinterpret_cast<void**>(&nonSharedCharacterBreakIterator), 0, m_iterator))
        delete m_iterator;
}

static TextBreakIterator* staticLineBreakIterator;

TextBreakIterator* acquireLineBreakIterator(const UChar* string, int length, const AtomicString& locale, const UChar* priorContext, unsigned priorContextLength)
{
    // This code does not yet support non-empty prior line break context. This
    // needs to be fixed by someone building/testing on a non-ICU platform. See
    // https://bugs.webkit.org/show_bug.cgi?id=105692#c52 for further info.
    ASSERT_UNUSED(priorContext, !priorContext);
    ASSERT_UNUSED(priorContextLength, !priorContextLength);

    TextBreakIterator* lineBreakIterator = 0;
    if (staticLineBreakIterator) {
        staticLineBreakIterator->reset(string, length);
        swap(staticLineBreakIterator, lineBreakIterator);
    }

    if (!lineBreakIterator && string && length) {
        lineBreakIterator = new LineBreakIterator;
        lineBreakIterator->reset(string, length);
    }

    return lineBreakIterator;
}

void releaseLineBreakIterator(TextBreakIterator* iterator)
{
    ASSERT(iterator);

    if (!staticLineBreakIterator)
        staticLineBreakIterator = iterator;
    else
        delete iterator;
}

TextBreakIterator* sentenceBreakIterator(const UChar* string, int length)
{
    DEFINE_STATIC_LOCAL(SentenceBreakIterator, iterator, ());
    iterator.reset(string, length);
    return &iterator;
}

int textBreakFirst(TextBreakIterator* breakIterator)
{
    return breakIterator->first();
}

int textBreakLast(TextBreakIterator* breakIterator)
{
    return breakIterator->last();
}

int textBreakNext(TextBreakIterator* breakIterator)
{
    return breakIterator->next();
}

int textBreakPrevious(TextBreakIterator* breakIterator)
{
    return breakIterator->previous();
}

int textBreakPreceding(TextBreakIterator* breakIterator, int position)
{
    return breakIterator->preceding(position);
}

int textBreakFollowing(TextBreakIterator* breakIterator, int position)
{
    return breakIterator->following(position);
}

int textBreakCurrent(TextBreakIterator* breakIterator)
{
    return breakIterator->currentPos;
}

bool isTextBreak(TextBreakIterator*, int)
{
    return true;
}

bool isWordTextBreak(TextBreakIterator*)
{
    return true;
}

TextBreakIterator* cursorMovementIterator(const UChar* string, int length)
{
    DEFINE_STATIC_LOCAL(CharBreakIterator, iterator, ());
    iterator.reset(string, length);
    return &iterator;
}

} // namespace WebCore
