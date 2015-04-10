/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DOMWindowSpeechSynthesis.h"

#if ENABLE(SPEECH_SYNTHESIS)

#include "DOMWindow.h"
#include <wtf/PassRefPtr.h>

namespace WebCore {
    
DOMWindowSpeechSynthesis::DOMWindowSpeechSynthesis(DOMWindow* window)
    : DOMWindowProperty(window->frame())
{
}

DOMWindowSpeechSynthesis::~DOMWindowSpeechSynthesis()
{
}

const char* DOMWindowSpeechSynthesis::supplementName()
{
    return "DOMWindowSpeechSynthesis";
}

// static
DOMWindowSpeechSynthesis* DOMWindowSpeechSynthesis::from(DOMWindow* window)
{
    DOMWindowSpeechSynthesis* supplement = static_cast<DOMWindowSpeechSynthesis*>(Supplement<DOMWindow>::from(window, supplementName()));
    if (!supplement) {
        supplement = new DOMWindowSpeechSynthesis(window);
        provideTo(window, supplementName(), adoptPtr(supplement));
    }
    return supplement;
}

// static
SpeechSynthesis* DOMWindowSpeechSynthesis::speechSynthesis(DOMWindow* window)
{
    return DOMWindowSpeechSynthesis::from(window)->speechSynthesis();
}

SpeechSynthesis* DOMWindowSpeechSynthesis::speechSynthesis()
{
    if (!m_speechSynthesis && frame())
        m_speechSynthesis = SpeechSynthesis::create();
    return m_speechSynthesis.get();
}
    
} // namespace WebCore

#endif // ENABLE(SPEECH_SYNTHESIS)
