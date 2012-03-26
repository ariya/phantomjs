/*
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "SegmentedString.h"

namespace WebCore {

SegmentedString::SegmentedString(const SegmentedString& other)
    : m_pushedChar1(other.m_pushedChar1)
    , m_pushedChar2(other.m_pushedChar2)
    , m_currentString(other.m_currentString)
    , m_substrings(other.m_substrings)
    , m_closed(other.m_closed)
{
    if (other.m_currentChar == &other.m_pushedChar1)
        m_currentChar = &m_pushedChar1;
    else if (other.m_currentChar == &other.m_pushedChar2)
        m_currentChar = &m_pushedChar2;
    else
        m_currentChar = other.m_currentChar;
}

const SegmentedString& SegmentedString::operator=(const SegmentedString& other)
{
    m_pushedChar1 = other.m_pushedChar1;
    m_pushedChar2 = other.m_pushedChar2;
    m_currentString = other.m_currentString;
    m_substrings = other.m_substrings;
    if (other.m_currentChar == &other.m_pushedChar1)
        m_currentChar = &m_pushedChar1;
    else if (other.m_currentChar == &other.m_pushedChar2)
        m_currentChar = &m_pushedChar2;
    else
        m_currentChar = other.m_currentChar;
    m_closed = other.m_closed;
    m_numberOfCharactersConsumedPriorToCurrentString = other.m_numberOfCharactersConsumedPriorToCurrentString;
    m_numberOfCharactersConsumedPriorToCurrentLine = other.m_numberOfCharactersConsumedPriorToCurrentLine;
    m_currentLine = other.m_currentLine;

    return *this;
}

unsigned SegmentedString::length() const
{
    unsigned length = m_currentString.m_length;
    if (m_pushedChar1) {
        ++length;
        if (m_pushedChar2)
            ++length;
    }
    if (isComposite()) {
        Deque<SegmentedSubstring>::const_iterator it = m_substrings.begin();
        Deque<SegmentedSubstring>::const_iterator e = m_substrings.end();
        for (; it != e; ++it)
            length += it->m_length;
    }
    return length;
}

void SegmentedString::setExcludeLineNumbers()
{
    m_currentString.setExcludeLineNumbers();
    if (isComposite()) {
        Deque<SegmentedSubstring>::iterator it = m_substrings.begin();
        Deque<SegmentedSubstring>::iterator e = m_substrings.end();
        for (; it != e; ++it)
            it->setExcludeLineNumbers();
    }
}

void SegmentedString::clear()
{
    m_pushedChar1 = 0;
    m_pushedChar2 = 0;
    m_currentChar = 0;
    m_currentString.clear();
    m_substrings.clear();
    m_closed = false;
}

void SegmentedString::append(const SegmentedSubstring& s)
{
    ASSERT(!m_closed);
    if (!s.m_length)
        return;

    if (!m_currentString.m_length) {
        m_numberOfCharactersConsumedPriorToCurrentString += m_currentString.numberOfCharactersConsumed();
        m_currentString = s;
    } else
        m_substrings.append(s);
}

void SegmentedString::prepend(const SegmentedSubstring& s)
{
    ASSERT(!escaped());
    ASSERT(!s.numberOfCharactersConsumed());
    if (!s.m_length)
        return;

    // FIXME: We're assuming that the prepend were originally consumed by
    //        this SegmentedString.  We're also ASSERTing that s is a fresh
    //        SegmentedSubstring.  These assumptions are sufficient for our
    //        current use, but we might need to handle the more elaborate
    //        cases in the future.
    m_numberOfCharactersConsumedPriorToCurrentString += m_currentString.numberOfCharactersConsumed();
    m_numberOfCharactersConsumedPriorToCurrentString -= s.m_length;
    if (!m_currentString.m_length)
        m_currentString = s;
    else {
        // Shift our m_currentString into our list.
        m_substrings.prepend(m_currentString);
        m_currentString = s;
    }
}

void SegmentedString::close()
{
    // Closing a stream twice is likely a coding mistake.
    ASSERT(!m_closed);
    m_closed = true;
}

void SegmentedString::append(const SegmentedString& s)
{
    ASSERT(!m_closed);
    ASSERT(!s.escaped());
    append(s.m_currentString);
    if (s.isComposite()) {
        Deque<SegmentedSubstring>::const_iterator it = s.m_substrings.begin();
        Deque<SegmentedSubstring>::const_iterator e = s.m_substrings.end();
        for (; it != e; ++it)
            append(*it);
    }
    m_currentChar = m_pushedChar1 ? &m_pushedChar1 : m_currentString.m_current;
}

void SegmentedString::prepend(const SegmentedString& s)
{
    ASSERT(!escaped());
    ASSERT(!s.escaped());
    if (s.isComposite()) {
        Deque<SegmentedSubstring>::const_reverse_iterator it = s.m_substrings.rbegin();
        Deque<SegmentedSubstring>::const_reverse_iterator e = s.m_substrings.rend();
        for (; it != e; ++it)
            prepend(*it);
    }
    prepend(s.m_currentString);
    m_currentChar = m_pushedChar1 ? &m_pushedChar1 : m_currentString.m_current;
}

void SegmentedString::advanceSubstring()
{
    if (isComposite()) {
        m_numberOfCharactersConsumedPriorToCurrentString += m_currentString.numberOfCharactersConsumed();
        m_currentString = m_substrings.takeFirst();
        // If we've previously consumed some characters of the non-current
        // string, we now account for those characters as part of the current
        // string, not as part of "prior to current string."
        m_numberOfCharactersConsumedPriorToCurrentString -= m_currentString.numberOfCharactersConsumed();
    } else
        m_currentString.clear();
}

String SegmentedString::toString() const
{
    String result;
    if (m_pushedChar1) {
        result.append(m_pushedChar1);
        if (m_pushedChar2)
            result.append(m_pushedChar2);
    }
    m_currentString.appendTo(result);
    if (isComposite()) {
        Deque<SegmentedSubstring>::const_iterator it = m_substrings.begin();
        Deque<SegmentedSubstring>::const_iterator e = m_substrings.end();
        for (; it != e; ++it)
            it->appendTo(result);
    }
    return result;
}

void SegmentedString::advance(unsigned count, UChar* consumedCharacters)
{
    ASSERT(count <= length());
    for (unsigned i = 0; i < count; ++i) {
        consumedCharacters[i] = *current();
        advance();
    }
}

void SegmentedString::advanceSlowCase()
{
    if (m_pushedChar1) {
        m_pushedChar1 = m_pushedChar2;
        m_pushedChar2 = 0;
    } else if (m_currentString.m_current) {
        ++m_currentString.m_current;
        if (--m_currentString.m_length == 0)
            advanceSubstring();
    }
    m_currentChar = m_pushedChar1 ? &m_pushedChar1 : m_currentString.m_current;
}

void SegmentedString::advanceSlowCase(int& lineNumber)
{
    if (m_pushedChar1) {
        m_pushedChar1 = m_pushedChar2;
        m_pushedChar2 = 0;
    } else if (m_currentString.m_current) {
        if (*m_currentString.m_current++ == '\n' && m_currentString.doNotExcludeLineNumbers()) {
            ++lineNumber;
            ++m_currentLine;
            // Plus 1 because numberOfCharactersConsumed value hasn't incremented yet; it does with m_length decrement below.
            m_numberOfCharactersConsumedPriorToCurrentLine = numberOfCharactersConsumed() + 1;
        }
        if (--m_currentString.m_length == 0)
            advanceSubstring();
    }
    m_currentChar = m_pushedChar1 ? &m_pushedChar1 : m_currentString.m_current;
}

WTF::ZeroBasedNumber SegmentedString::currentLine() const
{
    return WTF::ZeroBasedNumber::fromZeroBasedInt(m_currentLine);
}

WTF::ZeroBasedNumber SegmentedString::currentColumn() const
{
    int zeroBasedColumn = numberOfCharactersConsumed() - m_numberOfCharactersConsumedPriorToCurrentLine;
    return WTF::ZeroBasedNumber::fromZeroBasedInt(zeroBasedColumn);
}

void SegmentedString::setCurrentPosition(WTF::ZeroBasedNumber line, WTF::ZeroBasedNumber columnAftreProlog, int prologLength)
{
    m_currentLine = line.zeroBasedInt();
    m_numberOfCharactersConsumedPriorToCurrentLine = numberOfCharactersConsumed() + prologLength - columnAftreProlog.zeroBasedInt();
}

}
