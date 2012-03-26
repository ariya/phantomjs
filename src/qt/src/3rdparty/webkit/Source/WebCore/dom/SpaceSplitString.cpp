/*
 * Copyright (C) 2007 David Smith (catfish.man@gmail.com)
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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
 */

#include "config.h"
#include "SpaceSplitString.h"

#include "HTMLParserIdioms.h"
#include <wtf/ASCIICType.h>
#include <wtf/text/StringBuilder.h>

using namespace WTF;

namespace WebCore {

static bool hasNonASCIIOrUpper(const String& string)
{
    const UChar* characters = string.characters();
    unsigned length = string.length();
    bool hasUpper = false;
    UChar ored = 0;
    for (unsigned i = 0; i < length; i++) {
        UChar c = characters[i];
        hasUpper |= isASCIIUpper(c);
        ored |= c;
    }
    return hasUpper || (ored & ~0x7F);
}

void SpaceSplitStringData::createVector()
{
    ASSERT(!m_createdVector);
    ASSERT(m_vector.isEmpty());

    if (m_shouldFoldCase && hasNonASCIIOrUpper(m_string))
        m_string = m_string.foldCase();

    const UChar* characters = m_string.characters();
    unsigned length = m_string.length();
    unsigned start = 0;
    while (true) {
        while (start < length && isHTMLSpace(characters[start]))
            ++start;
        if (start >= length)
            break;
        unsigned end = start + 1;
        while (end < length && isNotHTMLSpace(characters[end]))
            ++end;

        m_vector.append(AtomicString(characters + start, end - start));

        start = end + 1;
    }

    m_string = String();
    m_createdVector = true;
}

bool SpaceSplitStringData::containsAll(SpaceSplitStringData& other)
{
    ensureVector();
    other.ensureVector();
    size_t thisSize = m_vector.size();
    size_t otherSize = other.m_vector.size();
    for (size_t i = 0; i < otherSize; ++i) {
        const AtomicString& name = other.m_vector[i];
        size_t j;
        for (j = 0; j < thisSize; ++j) {
            if (m_vector[j] == name)
                break;
        }
        if (j == thisSize)
            return false;
    }
    return true;
}

void SpaceSplitStringData::add(const AtomicString& string)
{
    if (contains(string))
        return;

    m_vector.append(string);
}

void SpaceSplitStringData::remove(const AtomicString& string)
{
    ensureVector();

    size_t position = 0;
    while (position < m_vector.size()) {
        if (m_vector[position] == string)
            m_vector.remove(position);
        else
            ++position;
    }
}

void SpaceSplitString::add(const AtomicString& string)
{
    if (m_data)
        m_data->add(string);
}

void SpaceSplitString::remove(const AtomicString& string)
{
    if (m_data)
        m_data->remove(string);
}

} // namespace WebCore
