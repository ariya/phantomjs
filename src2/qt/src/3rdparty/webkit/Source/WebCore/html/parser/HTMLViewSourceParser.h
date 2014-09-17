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

#ifndef HTMLViewSourceParser_h
#define HTMLViewSourceParser_h

#include "DecodedDataDocumentParser.h"
#include "HTMLInputStream.h"
#include "HTMLSourceTracker.h"
#include "HTMLToken.h"
#include "HTMLTokenizer.h"
#include "HTMLViewSourceDocument.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class HTMLTokenizer;
class HTMLScriptRunner;
class HTMLTreeBuilder;
class HTMLPreloadScanner;
class ScriptController;
class ScriptSourceCode;

class HTMLViewSourceParser :  public DecodedDataDocumentParser {
public:
    static PassRefPtr<HTMLViewSourceParser> create(HTMLViewSourceDocument* document)
    {
        return adoptRef(new HTMLViewSourceParser(document));
    }
    virtual ~HTMLViewSourceParser();

protected:
    explicit HTMLViewSourceParser(HTMLViewSourceDocument*);

    HTMLTokenizer* tokenizer() const { return m_tokenizer.get(); }

private:
    // DocumentParser
    virtual void insert(const SegmentedString&);
    virtual void append(const SegmentedString&);
    virtual void finish();
    virtual bool finishWasCalled();

    HTMLViewSourceDocument* document() const { return static_cast<HTMLViewSourceDocument*>(DecodedDataDocumentParser::document()); }

    void pumpTokenizer();
    String sourceForToken();
    void updateTokenizerState();

    HTMLInputStream m_input;
    HTMLToken m_token;
    HTMLSourceTracker m_sourceTracker;
    OwnPtr<HTMLTokenizer> m_tokenizer;
};

}

#endif
