/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PageThrottler_h
#define PageThrottler_h

#include "Timer.h"

#include <wtf/HashSet.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class Page;
class PageActivityAssertionToken;

class PageThrottler {
public:
    static PassOwnPtr<PageThrottler> create(Page* page)
    {
        return adoptPtr(new PageThrottler(page));
    }

    bool shouldThrottleAnimations() const { return m_throttleState != PageNotThrottledState; }
    bool shouldThrottleTimers() const { return m_throttleState != PageNotThrottledState; }

    void setThrottled(bool);

    void reportInterestingEvent();

    ~PageThrottler();

private:
    enum PageThrottleState {
        PageNotThrottledState,
        PageWaitingToThrottleState,
        PageThrottledState
    };

    friend class PageActivityAssertionToken;
    void addActivityToken(PageActivityAssertionToken*);
    void removeActivityToken(PageActivityAssertionToken*);

    PageThrottler(Page*);
    void startThrottleHysteresisTimer();
    void stopThrottleHysteresisTimer();
    void throttleHysteresisTimerFired(Timer<PageThrottler>*);

    Page* m_page;

    void throttlePage();
    void unthrottlePage();

    PageThrottleState m_throttleState;
    Timer<PageThrottler> m_throttleHysteresisTimer;
    HashSet<PageActivityAssertionToken*> m_activityTokens;
};

}
#endif
