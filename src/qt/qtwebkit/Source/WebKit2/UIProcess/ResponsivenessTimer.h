/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef ResponsivenessTimer_h
#define ResponsivenessTimer_h

#include <WebCore/RunLoop.h>

namespace WebKit {

class ResponsivenessTimer {
public:
    class Client {
    public:
        virtual ~Client() { }
        virtual void didBecomeUnresponsive(ResponsivenessTimer*) = 0;
        virtual void interactionOccurredWhileUnresponsive(ResponsivenessTimer*) = 0;
        virtual void didBecomeResponsive(ResponsivenessTimer*) = 0;
    };

    explicit ResponsivenessTimer(ResponsivenessTimer::Client*);
    ~ResponsivenessTimer();
    
    void start();
    void stop();

    void invalidate();
    
    bool isResponsive() { return m_isResponsive; }

private:
    void timerFired();

    ResponsivenessTimer::Client* m_client;
    bool m_isResponsive;

    WebCore::RunLoop::Timer<ResponsivenessTimer> m_timer;
};

} // namespace WebKit

#endif // ResponsivenessTimer_h

