/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
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

#ifndef HTMLParserScheduler_h
#define HTMLParserScheduler_h

#include "NestingLevelIncrementer.h"
#include "Timer.h"
#include <wtf/CurrentTime.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class Document;
class HTMLDocumentParser;

class ActiveParserSession {
public:
    explicit ActiveParserSession(Document*);
    ~ActiveParserSession();

private:
    RefPtr<Document> m_document;
};

class PumpSession : public NestingLevelIncrementer, public ActiveParserSession {
public:
    PumpSession(unsigned& nestingLevel, Document*);
    ~PumpSession();

    int processedTokens;
    double startTime;
    bool needsYield;
    bool didSeeScript;
};

class HTMLParserScheduler {
    WTF_MAKE_NONCOPYABLE(HTMLParserScheduler); WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<HTMLParserScheduler> create(HTMLDocumentParser* parser)
    {
        return adoptPtr(new HTMLParserScheduler(parser));
    }
    ~HTMLParserScheduler();

    // Inline as this is called after every token in the parser.
    void checkForYieldBeforeToken(PumpSession& session)
    {
        if (session.processedTokens > m_parserChunkSize || session.didSeeScript) {
            // currentTime() can be expensive.  By delaying, we avoided calling
            // currentTime() when constructing non-yielding PumpSessions.
            if (!session.startTime)
                session.startTime = currentTime();

            session.processedTokens = 0;
            session.didSeeScript = false;

            double elapsedTime = currentTime() - session.startTime;
            if (elapsedTime > m_parserTimeLimit)
                session.needsYield = true;
        }
        ++session.processedTokens;
    }
    void checkForYieldBeforeScript(PumpSession&);

    void scheduleForResume();
    bool isScheduledForResume() const { return m_isSuspendedWithActiveTimer || m_continueNextChunkTimer.isActive(); }

    void suspend();
    void resume();

private:
    HTMLParserScheduler(HTMLDocumentParser*);

    void continueNextChunkTimerFired(Timer<HTMLParserScheduler>*);

    HTMLDocumentParser* m_parser;

    double m_parserTimeLimit;
    int m_parserChunkSize;
    Timer<HTMLParserScheduler> m_continueNextChunkTimer;
    bool m_isSuspendedWithActiveTimer;
};

}

#endif
