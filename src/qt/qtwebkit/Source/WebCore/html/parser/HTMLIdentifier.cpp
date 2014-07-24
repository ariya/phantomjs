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

#include "HTMLIdentifier.h"

#include "HTMLNames.h"
#include <wtf/HashMap.h>
#include <wtf/MainThread.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

using namespace HTMLNames;

typedef std::pair<unsigned, StringImpl*> IdentifierEntry;
typedef HashMap<unsigned, IdentifierEntry, AlreadyHashed> IdentifierTable;

unsigned HTMLIdentifier::maxNameLength = 0;

static IdentifierTable& identifierTable()
{
    DEFINE_STATIC_LOCAL(IdentifierTable, table, ());
    ASSERT(isMainThread() || !table.isEmpty());
    return table;
}

#ifndef NDEBUG
bool HTMLIdentifier::hasIndex(const StringImpl* string)
{
    const IdentifierTable& table = identifierTable();
    return table.contains(string->hash());
}
#endif

unsigned HTMLIdentifier::findIndex(const UChar* characters, unsigned length)
{
    // We don't need to try hashing if we know the string is too long.
    if (length > maxNameLength)
        return invalidIndex;
    // computeHashAndMaskTop8Bits is the function StringImpl::hash() uses.
    unsigned hash = StringHasher::computeHashAndMaskTop8Bits(characters, length);
    const IdentifierTable& table = identifierTable();
    ASSERT(!table.isEmpty());

    IdentifierTable::const_iterator it = table.find(hash);
    if (it == table.end())
        return invalidIndex;
    // It's possible to have hash collisions between arbitrary strings and
    // known identifiers (e.g. "bvvfg" collides with "script").
    // However ASSERTs in addNames() guard against there ever being collisions
    // between known identifiers.
    if (!equal(it->value.second, characters, length))
        return invalidIndex;
    return it->value.first;
}

const unsigned kHTMLNamesIndexOffset = 0;
const unsigned kHTMLAttrsIndexOffset = 1000;
COMPILE_ASSERT(kHTMLAttrsIndexOffset > HTMLTagsCount, kHTMLAttrsIndexOffset_should_be_larger_than_HTMLTagsCount);

static const String& nameForIndex(unsigned index)
{
    unsigned adjustedIndex;
    QualifiedName** names;
    if (index < kHTMLAttrsIndexOffset) {
        ASSERT(index < kHTMLNamesIndexOffset + HTMLTagsCount);
        adjustedIndex = index - kHTMLNamesIndexOffset;
        names = getHTMLTags();
    } else {
        ASSERT(index < kHTMLAttrsIndexOffset + HTMLAttrsCount);
        adjustedIndex = index - kHTMLAttrsIndexOffset;
        names = getHTMLAttrs();
    }
    // HTMLAttrs and HTMLNames may have collisions, but
    // we shouldn't care which we ended up storing, since their
    // components are all AtomicStrings and should use the same
    // underlying StringImpl*.
    return names[adjustedIndex]->localName().string();
}

const String& HTMLIdentifier::asString() const
{
    ASSERT(isMainThread());
    if (m_index != invalidIndex)
        return nameForIndex(m_index);
    return m_string;
}

const StringImpl* HTMLIdentifier::asStringImpl() const
{
    if (m_index != invalidIndex)
        return nameForIndex(m_index).impl();
    return m_string.impl();
}

void HTMLIdentifier::addNames(QualifiedName** names, unsigned namesCount, unsigned indexOffset)
{
    IdentifierTable& table = identifierTable();
    for (unsigned i = 0; i < namesCount; ++i) {
        StringImpl* name = names[i]->localName().impl();
        unsigned hash = name->hash();
        unsigned index = i + indexOffset;
        IdentifierEntry entry(index, name);
        IdentifierTable::AddResult addResult = table.add(hash, entry);
        maxNameLength = std::max(maxNameLength, name->length());
        // Ensure we're using the same hashing algorithm to get and set.
        ASSERT_UNUSED(addResult, !addResult.isNewEntry || HTMLIdentifier::findIndex(name->characters(), name->length()) == index);
        // We expect some hash collisions, but only for identical strings.
        // Since all of these names are AtomicStrings pointers should be equal.
        // Note: If you hit this ASSERT, then we had a hash collision among
        // HTMLNames strings, and we need to re-design how we use this hash!
        ASSERT_UNUSED(addResult, !addResult.isNewEntry || name == addResult.iterator->value.second);
    }
}

void HTMLIdentifier::init()
{
    ASSERT(isMainThread()); // Not technically necessary, but this is our current expected usage.
    static bool isInitialized = false;
    if (isInitialized)
        return;
    isInitialized = true;

    // FIXME: We should atomize small whitespace (\n, \n\n, etc.)
    addNames(getHTMLTags(), HTMLTagsCount, kHTMLNamesIndexOffset);
    addNames(getHTMLAttrs(), HTMLAttrsCount, kHTMLAttrsIndexOffset);
}

}

#endif // ENABLE(THREADED_HTML_PARSER)
