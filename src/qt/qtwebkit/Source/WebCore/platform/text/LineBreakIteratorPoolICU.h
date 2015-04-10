/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
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

#ifndef LineBreakIteratorPoolICU_h
#define LineBreakIteratorPoolICU_h

#include "TextBreakIteratorInternalICU.h"
#include <unicode/ubrk.h>
#include <wtf/Assertions.h>
#include <wtf/HashMap.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/ThreadSpecific.h>
#include <wtf/text/AtomicString.h>
#include <wtf/text/CString.h>

namespace WebCore {

class LineBreakIteratorPool {
    WTF_MAKE_NONCOPYABLE(LineBreakIteratorPool);
public:
    static LineBreakIteratorPool& sharedPool()
    {
        static WTF::ThreadSpecific<LineBreakIteratorPool>* pool = new WTF::ThreadSpecific<LineBreakIteratorPool>;
        return **pool;
    }

    static PassOwnPtr<LineBreakIteratorPool> create() { return adoptPtr(new LineBreakIteratorPool); }

    UBreakIterator* take(const AtomicString& locale)
    {
        UBreakIterator* iterator = 0;
        for (size_t i = 0; i < m_pool.size(); ++i) {
            if (m_pool[i].first == locale) {
                iterator = m_pool[i].second;
                m_pool.remove(i);
                break;
            }
        }

        if (!iterator) {
            UErrorCode openStatus = U_ZERO_ERROR;
            bool localeIsEmpty = locale.isEmpty();
            iterator = ubrk_open(UBRK_LINE, localeIsEmpty ? currentTextBreakLocaleID() : locale.string().utf8().data(), 0, 0, &openStatus);
            // locale comes from a web page and it can be invalid, leading ICU
            // to fail, in which case we fall back to the default locale.
            if (!localeIsEmpty && U_FAILURE(openStatus)) {
                openStatus = U_ZERO_ERROR;
                iterator = ubrk_open(UBRK_LINE, currentTextBreakLocaleID(), 0, 0, &openStatus);
            }
                
            if (U_FAILURE(openStatus)) {
                LOG_ERROR("ubrk_open failed with status %d", openStatus);
                return 0;
            }
        }

        ASSERT(!m_vendedIterators.contains(iterator));
        m_vendedIterators.set(iterator, locale);
        return iterator;
    }

    void put(UBreakIterator* iterator)
    {
        ASSERT_ARG(iterator, m_vendedIterators.contains(iterator));

        if (m_pool.size() == capacity) {
            ubrk_close(m_pool[0].second);
            m_pool.remove(0);
        }

        m_pool.append(Entry(m_vendedIterators.take(iterator), iterator));
    }

private:
    LineBreakIteratorPool() { }

    static const size_t capacity = 4;

    typedef pair<AtomicString, UBreakIterator*> Entry;
    typedef Vector<Entry, capacity> Pool;
    Pool m_pool;
    HashMap<UBreakIterator*, AtomicString> m_vendedIterators;

    friend WTF::ThreadSpecific<LineBreakIteratorPool>::operator LineBreakIteratorPool*();
};

}

#endif
