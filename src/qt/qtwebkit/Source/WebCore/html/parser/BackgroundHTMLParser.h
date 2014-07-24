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

#ifndef BackgroundHTMLParser_h
#define BackgroundHTMLParser_h

#if ENABLE(THREADED_HTML_PARSER)

#include "BackgroundHTMLInputStream.h"
#include "CompactHTMLToken.h"
#include "HTMLParserOptions.h"
#include "HTMLPreloadScanner.h"
#include "HTMLSourceTracker.h"
#include "HTMLToken.h"
#include "HTMLTokenizer.h"
#include "HTMLTreeBuilderSimulator.h"
#include "XSSAuditorDelegate.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/WeakPtr.h>

namespace WebCore {

class HTMLDocumentParser;
class XSSAuditor;

class BackgroundHTMLParser {
    WTF_MAKE_FAST_ALLOCATED;
public:
    struct Configuration {
        HTMLParserOptions options;
        WeakPtr<HTMLDocumentParser> parser;
        OwnPtr<XSSAuditor> xssAuditor;
        OwnPtr<TokenPreloadScanner> preloadScanner;
    };

    static void create(PassRefPtr<WeakReference<BackgroundHTMLParser> > reference, PassOwnPtr<Configuration> config)
    {
        new BackgroundHTMLParser(reference, config);
        // Caller must free by calling stop().
    }

    struct Checkpoint {
        WeakPtr<HTMLDocumentParser> parser;
        OwnPtr<HTMLToken> token;
        OwnPtr<HTMLTokenizer> tokenizer;
        HTMLTreeBuilderSimulator::State treeBuilderState;
        HTMLInputCheckpoint inputCheckpoint;
        TokenPreloadScannerCheckpoint preloadScannerCheckpoint;
        String unparsedInput;
    };

    void append(const String&);
    void resumeFrom(PassOwnPtr<Checkpoint>);
    void startedChunkWithCheckpoint(HTMLInputCheckpoint);
    void finish();
    void stop();

    void forcePlaintextForTextDocument();

private:
    BackgroundHTMLParser(PassRefPtr<WeakReference<BackgroundHTMLParser> >, PassOwnPtr<Configuration>);

    void markEndOfFile();
    void pumpTokenizer();
    void sendTokensToMainThread();

    WeakPtrFactory<BackgroundHTMLParser> m_weakFactory;
    BackgroundHTMLInputStream m_input;
    HTMLSourceTracker m_sourceTracker;
    OwnPtr<HTMLToken> m_token;
    OwnPtr<HTMLTokenizer> m_tokenizer;
    HTMLTreeBuilderSimulator m_treeBuilderSimulator;
    HTMLParserOptions m_options;
    WeakPtr<HTMLDocumentParser> m_parser;

    OwnPtr<CompactHTMLTokenStream> m_pendingTokens;
    PreloadRequestStream m_pendingPreloads;
    XSSInfoStream m_pendingXSSInfos;

    OwnPtr<XSSAuditor> m_xssAuditor;
    OwnPtr<TokenPreloadScanner> m_preloadScanner;
};

}

#endif // ENABLE(THREADED_HTML_PARSER)

#endif
