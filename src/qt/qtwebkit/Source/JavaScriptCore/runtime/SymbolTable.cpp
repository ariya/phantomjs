/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "JSDestructibleObject.h"
#include "JSCellInlines.h"
#include "SymbolTable.h"

namespace JSC {

const ClassInfo SharedSymbolTable::s_info = { "SharedSymbolTable", 0, 0, 0, CREATE_METHOD_TABLE(SharedSymbolTable) };

SymbolTableEntry& SymbolTableEntry::copySlow(const SymbolTableEntry& other)
{
    ASSERT(other.isFat());
    FatEntry* newFatEntry = new FatEntry(*other.fatEntry());
    freeFatEntry();
    m_bits = bitwise_cast<intptr_t>(newFatEntry);
    return *this;
}

void SharedSymbolTable::destroy(JSCell* cell)
{
    SharedSymbolTable* thisObject = jsCast<SharedSymbolTable*>(cell);
    thisObject->SharedSymbolTable::~SharedSymbolTable();
}

void SymbolTableEntry::freeFatEntrySlow()
{
    ASSERT(isFat());
    delete fatEntry();
}

bool SymbolTableEntry::couldBeWatched()
{
    if (!isFat())
        return false;
    WatchpointSet* watchpoints = fatEntry()->m_watchpoints.get();
    if (!watchpoints)
        return false;
    return watchpoints->isStillValid();
}

void SymbolTableEntry::attemptToWatch()
{
    FatEntry* entry = inflate();
    if (!entry->m_watchpoints)
        entry->m_watchpoints = adoptRef(new WatchpointSet(InitializedWatching));
}

bool* SymbolTableEntry::addressOfIsWatched()
{
    ASSERT(couldBeWatched());
    return fatEntry()->m_watchpoints->addressOfIsWatched();
}

void SymbolTableEntry::addWatchpoint(Watchpoint* watchpoint)
{
    ASSERT(couldBeWatched());
    fatEntry()->m_watchpoints->add(watchpoint);
}

void SymbolTableEntry::notifyWriteSlow()
{
    WatchpointSet* watchpoints = fatEntry()->m_watchpoints.get();
    if (!watchpoints)
        return;
    watchpoints->notifyWrite();
}

SymbolTableEntry::FatEntry* SymbolTableEntry::inflateSlow()
{
    FatEntry* entry = new FatEntry(m_bits);
    m_bits = bitwise_cast<intptr_t>(entry);
    return entry;
}

} // namespace JSC

