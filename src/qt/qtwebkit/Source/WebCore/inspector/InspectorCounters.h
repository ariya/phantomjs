/*
* Copyright (C) 2012 Google Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*
*     * Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
* copyright notice, this list of conditions and the following disclaimer
* in the documentation and/or other materials provided with the
* distribution.
*     * Neither the name of Google Inc. nor the names of its
* contributors may be used to endorse or promote products derived from
* this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef InspectorCounters_h
#define InspectorCounters_h

#include <wtf/FastAllocBase.h>

#if !ASSERT_DISABLED
#include <wtf/MainThread.h>
#endif

namespace WebCore {

class InspectorCounters {
public:
    enum CounterType {
        DocumentCounter,
        NodeCounter,
        JSEventListenerCounter,
        CounterTypeLength
    };

    static inline void incrementCounter(CounterType type)
    {
#if ENABLE(INSPECTOR)
        ASSERT(isMainThread());
        ++s_counters[type];
#else
        UNUSED_PARAM(type);
#endif
    }

    static inline void decrementCounter(CounterType type)
    {
#if ENABLE(INSPECTOR)
        ASSERT(isMainThread());
        --s_counters[type];
#else
        UNUSED_PARAM(type);
#endif
    }

#if ENABLE(INSPECTOR)
    static int counterValue(CounterType);
#endif

private:
    InspectorCounters();

#if ENABLE(INSPECTOR)
    static int s_counters[CounterTypeLength];
#endif
};


#if ENABLE(INSPECTOR)
class ThreadLocalInspectorCounters {
    WTF_MAKE_FAST_ALLOCATED;
public:
    enum CounterType {
        JSEventListenerCounter,
        CounterTypeLength
    };
    ThreadLocalInspectorCounters();

    inline void incrementCounter(CounterType type)
    {
        ++m_counters[type];
    }

    inline void decrementCounter(CounterType type)
    {
        --m_counters[type];
    }

    int counterValue(CounterType);

    static ThreadLocalInspectorCounters& current();

private:
    int m_counters[CounterTypeLength];
};
#endif

} // namespace WebCore

#endif // !defined(InspectorCounters_h)
