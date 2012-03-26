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

#include "config.h"
#include "HTMLViewSourceParser.h"

#include "HTMLDocumentParser.h"
#include "HTMLNames.h"
#include "HTMLViewSourceDocument.h"

namespace WebCore {

HTMLViewSourceParser::HTMLViewSourceParser(HTMLViewSourceDocument* document)
    : DecodedDataDocumentParser(document)
    , m_tokenizer(HTMLTokenizer::create(HTMLDocumentParser::usePreHTML5ParserQuirks(document)))
{
}

HTMLViewSourceParser::~HTMLViewSourceParser()
{
}

void HTMLViewSourceParser::insert(const SegmentedString&)
{
    ASSERT_NOT_REACHED();
}

void HTMLViewSourceParser::pumpTokenizer()
{
    while (true) {
        m_sourceTracker.start(m_input, m_token);
        if (!m_tokenizer->nextToken(m_input.current(), m_token))
            break;
        m_sourceTracker.end(m_input, m_token);

        document()->addSource(sourceForToken(), m_token);
        updateTokenizerState();
        m_token.clear();
    }
}

void HTMLViewSourceParser::append(const SegmentedString& input)
{
    m_input.appendToEnd(input);
    pumpTokenizer();
}

String HTMLViewSourceParser::sourceForToken()
{
    return m_sourceTracker.sourceForToken(m_token);
}

void HTMLViewSourceParser::updateTokenizerState()
{
    // FIXME: The tokenizer should do this work for us.
    if (m_token.type() != HTMLToken::StartTag)
        return;

    AtomicString tagName(m_token.name().data(), m_token.name().size());
    m_tokenizer->updateStateFor(tagName, document()->frame());
}

void HTMLViewSourceParser::finish()
{
    if (!m_input.haveSeenEndOfFile())
        m_input.markEndOfFile();
    pumpTokenizer();
    document()->finishedParsing();
}

bool HTMLViewSourceParser::finishWasCalled()
{
    return m_input.haveSeenEndOfFile();
}

}
