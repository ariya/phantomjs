/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#ifndef WTFThreadData_h
#define WTFThreadData_h

#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/StackBounds.h>
#include <wtf/text/StringHash.h>

// This was ENABLE(WORKERS) in WebCore, but this is not defined when compiling JSC.
// However this check was not correct anyway, re this comment:
//    // FIXME: Workers are not necessarily the only feature that make per-thread global data necessary.
//    // We need to check for e.g. database objects manipulating strings on secondary threads.
// Always enabling this is safe, and should be a better option until we can come up
// with a better define.
#define WTFTHREADDATA_MULTITHREADED 1

#if WTFTHREADDATA_MULTITHREADED
#include <wtf/ThreadSpecific.h>
#include <wtf/Threading.h>
#endif

#if USE(JSC)
// FIXME: This is a temporary layering violation while we move more string code to WTF.
namespace JSC {

typedef HashMap<const char*, RefPtr<StringImpl>, PtrHash<const char*> > LiteralIdentifierTable;

class IdentifierTable {
    WTF_MAKE_FAST_ALLOCATED;
public:
    ~IdentifierTable();

    std::pair<HashSet<StringImpl*>::iterator, bool> add(StringImpl* value);
    template<typename U, typename V>
    std::pair<HashSet<StringImpl*>::iterator, bool> add(U value);

    bool remove(StringImpl* r)
    {
        HashSet<StringImpl*>::iterator iter = m_table.find(r);
        if (iter == m_table.end())
            return false;
        m_table.remove(iter);
        return true;
    }

    LiteralIdentifierTable& literalTable() { return m_literalTable; }

private:
    HashSet<StringImpl*> m_table;
    LiteralIdentifierTable m_literalTable;
};

}
#endif

namespace WTF {

class AtomicStringTable;

typedef void (*AtomicStringTableDestructor)(AtomicStringTable*);

class WTFThreadData {
    WTF_MAKE_NONCOPYABLE(WTFThreadData);
public:
    WTFThreadData();
    ~WTFThreadData();

    AtomicStringTable* atomicStringTable()
    {
        return m_atomicStringTable;
    }

#if USE(JSC)
    JSC::IdentifierTable* currentIdentifierTable()
    {
        return m_currentIdentifierTable;
    }

    JSC::IdentifierTable* setCurrentIdentifierTable(JSC::IdentifierTable* identifierTable)
    {
        JSC::IdentifierTable* oldIdentifierTable = m_currentIdentifierTable;
        m_currentIdentifierTable = identifierTable;
        return oldIdentifierTable;
    }

    void resetCurrentIdentifierTable()
    {
        m_currentIdentifierTable = m_defaultIdentifierTable;
    }

    const StackBounds& stack() const
    {
        return m_stackBounds;
    }
#endif

private:
    AtomicStringTable* m_atomicStringTable;
    AtomicStringTableDestructor m_atomicStringTableDestructor;

#if USE(JSC)
    JSC::IdentifierTable* m_defaultIdentifierTable;
    JSC::IdentifierTable* m_currentIdentifierTable;
    StackBounds m_stackBounds;
#endif

#if WTFTHREADDATA_MULTITHREADED
    static JS_EXPORTDATA ThreadSpecific<WTFThreadData>* staticData;
#else
    static JS_EXPORTDATA WTFThreadData* staticData;
#endif
    friend WTFThreadData& wtfThreadData();
    friend class AtomicStringTable;
};

inline WTFThreadData& wtfThreadData()
{
#if WTFTHREADDATA_MULTITHREADED
    // WRT WebCore:
    //    WTFThreadData is used on main thread before it could possibly be used
    //    on secondary ones, so there is no need for synchronization here.
    // WRT JavaScriptCore:
    //    wtfThreadData() is initially called from initializeThreading(), ensuring
    //    this is initially called in a pthread_once locked context.
    if (!WTFThreadData::staticData)
        WTFThreadData::staticData = new ThreadSpecific<WTFThreadData>;
    return **WTFThreadData::staticData;
#else
    if (!WTFThreadData::staticData) {
        WTFThreadData::staticData = static_cast<WTFThreadData*>(fastMalloc(sizeof(WTFThreadData)));
        // WTFThreadData constructor indirectly uses staticData, so we need to set up the memory before invoking it.
        new (WTFThreadData::staticData) WTFThreadData;
    }
    return *WTFThreadData::staticData;
#endif
}

} // namespace WTF

using WTF::WTFThreadData;
using WTF::wtfThreadData;

#endif // WTFThreadData_h
