/*
 * Copyright (C) 2008, 2010 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SmallStrings.h"

#include "HeapRootVisitor.h"
#include "JSGlobalObject.h"
#include "JSString.h"
#include "Operations.h"
#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/StringImpl.h>

namespace JSC {

static inline void finalize(JSString*& string)
{
    if (!string || Heap::isMarked(string))
        return;
    string = 0;
}

class SmallStringsStorage {
    WTF_MAKE_NONCOPYABLE(SmallStringsStorage); WTF_MAKE_FAST_ALLOCATED;
public:
    SmallStringsStorage();

    StringImpl* rep(unsigned char character)
    {
        return m_reps[character].get();
    }

private:
    static const unsigned singleCharacterStringCount = maxSingleCharacterString + 1;

    RefPtr<StringImpl> m_reps[singleCharacterStringCount];
};

SmallStringsStorage::SmallStringsStorage()
{
    LChar* characterBuffer = 0;
    RefPtr<StringImpl> baseString = StringImpl::createUninitialized(singleCharacterStringCount, characterBuffer);
    for (unsigned i = 0; i < singleCharacterStringCount; ++i) {
        characterBuffer[i] = i;
        m_reps[i] = StringImpl::create(baseString, i, 1);
    }
}

SmallStrings::SmallStrings()
    : m_emptyString(0)
#define JSC_COMMON_STRINGS_ATTRIBUTE_INITIALIZE(name) , m_##name(0)
    JSC_COMMON_STRINGS_EACH_NAME(JSC_COMMON_STRINGS_ATTRIBUTE_INITIALIZE)
#undef JSC_COMMON_STRINGS_ATTRIBUTE_INITIALIZE
{
    COMPILE_ASSERT(singleCharacterStringCount == sizeof(m_singleCharacterStrings) / sizeof(m_singleCharacterStrings[0]), IsNumCharactersConstInSyncWithClassUsage);

    for (unsigned i = 0; i < singleCharacterStringCount; ++i)
        m_singleCharacterStrings[i] = 0;
}

void SmallStrings::initializeCommonStrings(VM& vm)
{
    createEmptyString(&vm);
#define JSC_COMMON_STRINGS_ATTRIBUTE_INITIALIZE(name) initialize(&vm, m_##name, #name);
    JSC_COMMON_STRINGS_EACH_NAME(JSC_COMMON_STRINGS_ATTRIBUTE_INITIALIZE)
#undef JSC_COMMON_STRINGS_ATTRIBUTE_INITIALIZE
}

void SmallStrings::visitStrongReferences(SlotVisitor& visitor)
{
    visitor.appendUnbarrieredPointer(&m_emptyString);
#define JSC_COMMON_STRINGS_ATTRIBUTE_VISIT(name) visitor.appendUnbarrieredPointer(&m_##name);
    JSC_COMMON_STRINGS_EACH_NAME(JSC_COMMON_STRINGS_ATTRIBUTE_VISIT)
#undef JSC_COMMON_STRINGS_ATTRIBUTE_VISIT
}

SmallStrings::~SmallStrings()
{
}

void SmallStrings::finalizeSmallStrings()
{
    finalize(m_emptyString);
    for (unsigned i = 0; i < singleCharacterStringCount; ++i)
        finalize(m_singleCharacterStrings[i]);
}

void SmallStrings::createEmptyString(VM* vm)
{
    ASSERT(!m_emptyString);
    m_emptyString = JSString::createHasOtherOwner(*vm, StringImpl::empty());
}

void SmallStrings::createSingleCharacterString(VM* vm, unsigned char character)
{
    if (!m_storage)
        m_storage = adoptPtr(new SmallStringsStorage);
    ASSERT(!m_singleCharacterStrings[character]);
    m_singleCharacterStrings[character] = JSString::createHasOtherOwner(*vm, PassRefPtr<StringImpl>(m_storage->rep(character)));
}

StringImpl* SmallStrings::singleCharacterStringRep(unsigned char character)
{
    if (!m_storage)
        m_storage = adoptPtr(new SmallStringsStorage);
    return m_storage->rep(character);
}

void SmallStrings::initialize(VM* vm, JSString*& string, const char* value) const
{
    string = JSString::create(*vm, StringImpl::create(value));
}

} // namespace JSC
