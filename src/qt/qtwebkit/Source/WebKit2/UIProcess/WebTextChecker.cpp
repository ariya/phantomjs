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
#include "WebTextChecker.h"

#include "TextChecker.h"
#include "WKAPICast.h"
#include "WebContext.h"
#include <wtf/RefPtr.h>

namespace WebKit {

WebTextChecker* WebTextChecker::shared()
{
    static WebTextChecker* textChecker = adoptRef(new WebTextChecker).leakRef();
    return textChecker;
}

WebTextChecker::WebTextChecker()
{
}

void WebTextChecker::setClient(const WKTextCheckerClient* client)
{
    m_client.initialize(client);
}

static void updateStateForAllContexts()
{
    const Vector<WebContext*>& contexts = WebContext::allContexts();
    for (size_t i = 0; i < contexts.size(); ++i)
        contexts[i]->textCheckerStateChanged();
}

void WebTextChecker::continuousSpellCheckingEnabledStateChanged(bool enabled)
{
    TextChecker::continuousSpellCheckingEnabledStateChanged(enabled);
    updateStateForAllContexts();
}

void WebTextChecker::grammarCheckingEnabledStateChanged(bool enabled)
{
    TextChecker::grammarCheckingEnabledStateChanged(enabled);
    updateStateForAllContexts();
}

void WebTextChecker::checkSpelling(const WebPageProxy* page, bool startBeforeSelection)
{
    page->advanceToNextMisspelling(startBeforeSelection);
}

void WebTextChecker::changeSpellingToWord(const WebPageProxy* page, const String& text)
{
    page->changeSpellingToWord(text);
}

} // namespace WebKit
