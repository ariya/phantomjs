/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(SCRIPTED_SPEECH)

#include "SpeechGrammarList.h"

#include "Document.h"

namespace WebCore {

PassRefPtr<SpeechGrammarList> SpeechGrammarList::create()
{
    return adoptRef(new SpeechGrammarList);
}

SpeechGrammar* SpeechGrammarList::item(unsigned long index) const
{
    if (index >= m_grammars.size())
        return 0;

    return m_grammars[index].get();
}

void SpeechGrammarList::addFromUri(ScriptExecutionContext* scriptExecutionContext, const String& src, double weight)
{
    Document* document = toDocument(scriptExecutionContext);
    m_grammars.append(SpeechGrammar::create(document->completeURL(src), weight));
}

void SpeechGrammarList::addFromString(const String& string, double weight)
{
    String urlString = String("data:application/xml,") + encodeWithURLEscapeSequences(string);
    m_grammars.append(SpeechGrammar::create(KURL(KURL(), urlString), weight));
}

SpeechGrammarList::SpeechGrammarList()
{
}

} // namespace WebCore

#endif // ENABLE(SCRIPTED_SPEECH)
