/*
 * Copyright (C) 2013 Apple Computer, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PlatformSpeechSynthesisVoice.h"

#if ENABLE(SPEECH_SYNTHESIS)

namespace WebCore {

PassRefPtr<PlatformSpeechSynthesisVoice> PlatformSpeechSynthesisVoice::create(const String& voiceURI, const String& name, const String& lang, bool localService, bool isDefault)
{
    return adoptRef(new PlatformSpeechSynthesisVoice(voiceURI, name, lang, localService, isDefault));
}

PassRefPtr<PlatformSpeechSynthesisVoice> PlatformSpeechSynthesisVoice::create()
{
    return adoptRef(new PlatformSpeechSynthesisVoice());
}
    
PlatformSpeechSynthesisVoice::PlatformSpeechSynthesisVoice(const String& voiceURI, const String& name, const String& lang, bool localService, bool isDefault)
    : m_voiceURI(voiceURI)
    , m_name(name)
    , m_lang(lang)
    , m_localService(localService)
    , m_default(isDefault)
{
}

PlatformSpeechSynthesisVoice::PlatformSpeechSynthesisVoice()
    : m_localService(false)
    , m_default(false)
{
}

} // namespace WebCore

#endif // ENABLE(SPEECH_SYNTHESIS)
