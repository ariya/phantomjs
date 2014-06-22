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
 *
 */

#include "config.h"
#include "WTFThreadData.h"

#include <wtf/text/AtomicStringTable.h>

namespace WTF {

ThreadSpecific<WTFThreadData>* WTFThreadData::staticData;

WTFThreadData::WTFThreadData()
    : m_apiData(0)
    , m_atomicStringTable(0)
    , m_atomicStringTableDestructor(0)
    , m_defaultIdentifierTable(new JSC::IdentifierTable())
    , m_currentIdentifierTable(m_defaultIdentifierTable)
    , m_stackBounds(StackBounds::currentThreadStackBounds())
#if ENABLE(STACK_STATS)
    , m_stackStats()
#endif
{
    AtomicStringTable::create(*this);
}

WTFThreadData::~WTFThreadData()
{
    if (m_atomicStringTableDestructor)
        m_atomicStringTableDestructor(m_atomicStringTable);
    delete m_defaultIdentifierTable;
}

} // namespace WTF

namespace JSC {

IdentifierTable::~IdentifierTable()
{
    HashSet<StringImpl*>::iterator end = m_table.end();
    for (HashSet<StringImpl*>::iterator iter = m_table.begin(); iter != end; ++iter)
        (*iter)->setIsIdentifier(false);
}

HashSet<StringImpl*>::AddResult IdentifierTable::add(StringImpl* value)
{
    HashSet<StringImpl*>::AddResult result = m_table.add(value);
    (*result.iterator)->setIsIdentifier(true);
    return result;
}

} // namespace JSC
