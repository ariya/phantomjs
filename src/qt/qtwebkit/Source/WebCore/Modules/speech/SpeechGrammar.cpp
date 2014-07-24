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

#include "SpeechGrammar.h"

#include "Document.h"

namespace WebCore {

PassRefPtr<SpeechGrammar> SpeechGrammar::create()
{
    return adoptRef(new SpeechGrammar);
}

PassRefPtr<SpeechGrammar> SpeechGrammar::create(const KURL& src, double weight)
{
    return adoptRef(new SpeechGrammar(src, weight));
}

void SpeechGrammar::setSrc(ScriptExecutionContext* scriptExecutionContext, const String& src)
{
    Document* document = toDocument(scriptExecutionContext);
    m_src = document->completeURL(src);
}

SpeechGrammar::SpeechGrammar()
    : m_weight(1.0)
{
}

SpeechGrammar::SpeechGrammar(const KURL& src, double weight)
    : m_src(src)
    , m_weight(weight)
{
}

} // namespace WebCore

#endif // ENABLE(SCRIPTED_SPEECH)
