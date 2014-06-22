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

#ifndef PlatformSpeechSynthesisVoice_h
#define PlatformSpeechSynthesisVoice_h

#if ENABLE(SPEECH_SYNTHESIS)

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class PlatformSpeechSynthesisVoice : public RefCounted<PlatformSpeechSynthesisVoice> {
public:
    static PassRefPtr<PlatformSpeechSynthesisVoice> create(const String& voiceURI, const String& name, const String& lang, bool localService, bool isDefault);
    static PassRefPtr<PlatformSpeechSynthesisVoice> create();

    const String& voiceURI() const { return m_voiceURI; }
    void setVoiceURI(const String& voiceURI) { m_voiceURI = voiceURI; }

    const String& name() const { return m_name; }
    void setName(const String& name) { m_name = name; }

    const String& lang() const { return m_lang; }
    void setLang(const String& lang) { m_lang = lang; }

    bool localService() const { return m_localService; }
    void setLocalService(bool localService) { m_localService = localService; }

    bool isDefault() const { return m_default; }
    void setIsDefault(bool isDefault) { m_default = isDefault; }

private:
    PlatformSpeechSynthesisVoice(const String& voiceURI, const String& name, const String& lang, bool localService, bool isDefault);
    PlatformSpeechSynthesisVoice();

    String m_voiceURI;
    String m_name;
    String m_lang;
    bool m_localService;
    bool m_default;
};

} // namespace WebCore

#endif // ENABLE(SPEECH_SYNTHESIS)

#endif // PlatformSpeechSynthesisVoice_h
