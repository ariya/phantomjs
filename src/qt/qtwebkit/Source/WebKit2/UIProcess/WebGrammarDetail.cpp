/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#include "config.h"
#include "WebGrammarDetail.h"

#include "ImmutableArray.h"
#include "WKGrammarDetail.h"
#include "WebString.h"

namespace WebKit {

PassRefPtr<WebGrammarDetail> WebGrammarDetail::create(int location, int length, ImmutableArray* guesses, const String& userDescription)
{
    return adoptRef(new WebGrammarDetail(location, length, guesses, userDescription));
}

PassRefPtr<WebGrammarDetail> WebGrammarDetail::create(const WebCore::GrammarDetail& grammarDetail)
{
    return adoptRef(new WebGrammarDetail(grammarDetail));
}

WebGrammarDetail::WebGrammarDetail(int location, int length, ImmutableArray* guesses, const String& userDescription)
{
    m_grammarDetail.location = location;
    m_grammarDetail.length = length;

    size_t numGuesses = guesses->size();
    m_grammarDetail.guesses.reserveCapacity(numGuesses);
    for (size_t i = 0; i < numGuesses; ++i)
        m_grammarDetail.guesses.uncheckedAppend(guesses->at<WebString>(i)->string());

    m_grammarDetail.userDescription = userDescription;
}

PassRefPtr<ImmutableArray> WebGrammarDetail::guesses() const
{
    size_t numGuesses = m_grammarDetail.guesses.size();
    Vector<RefPtr<APIObject> > wkGuesses(numGuesses);
    for (unsigned i = 0; i < numGuesses; ++i)
        wkGuesses[i] = WebString::create(m_grammarDetail.guesses[i]);
    return ImmutableArray::adopt(wkGuesses);
}

WebGrammarDetail::WebGrammarDetail(const WebCore::GrammarDetail& grammarDetail)
    : m_grammarDetail(grammarDetail)
{
}

} // namespace WebKit
