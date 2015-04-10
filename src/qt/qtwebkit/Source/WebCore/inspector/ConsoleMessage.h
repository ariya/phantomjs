/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Matt Lilek <webkit@mattlilek.com>
 * Copyright (C) 2009, 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ConsoleMessage_h
#define ConsoleMessage_h

#include "ConsoleAPITypes.h"
#include "ConsoleTypes.h"
#include "InspectorFrontend.h"
#include "ScriptState.h"
#include <wtf/Forward.h>
#include <wtf/Vector.h>

namespace WebCore {

class DOMWindow;
class InjectedScriptManager;
class InspectorFrontend;
class InspectorObject;
class ScriptArguments;
class ScriptCallFrame;
class ScriptCallStack;
class ScriptValue;

class ConsoleMessage {
    WTF_MAKE_NONCOPYABLE(ConsoleMessage); WTF_MAKE_FAST_ALLOCATED;
public:
    ConsoleMessage(bool canGenerateCallStack, MessageSource, MessageType, MessageLevel, const String& message, unsigned long requestIdentifier = 0);
    ConsoleMessage(bool canGenerateCallStack, MessageSource, MessageType, MessageLevel, const String& message, const String& url, unsigned line, unsigned column, ScriptState* = 0, unsigned long requestIdentifier = 0);
    ConsoleMessage(bool canGenerateCallStack, MessageSource, MessageType, MessageLevel, const String& message, PassRefPtr<ScriptCallStack>, unsigned long requestIdentifier = 0);
    ConsoleMessage(bool canGenerateCallStack, MessageSource, MessageType, MessageLevel, const String& message, PassRefPtr<ScriptArguments>, ScriptState*, unsigned long requestIdentifier = 0);
    ~ConsoleMessage();

    void addToFrontend(InspectorFrontend::Console*, InjectedScriptManager*, bool generatePreview);
    void updateRepeatCountInConsole(InspectorFrontend::Console*);
    void incrementCount() { ++m_repeatCount; }
    bool isEqual(ConsoleMessage* msg) const;

    MessageSource source() const { return m_source; }
    const String& message() const { return m_message; }
    MessageType type() const { return m_type; }

    void windowCleared(DOMWindow*);

    unsigned argumentCount();

private:
    void autogenerateMetadata(bool canGenerateCallStack, ScriptState* = 0);

    MessageSource m_source;
    MessageType m_type;
    MessageLevel m_level;
    String m_message;
    RefPtr<ScriptArguments> m_arguments;
    RefPtr<ScriptCallStack> m_callStack;
    String m_url;
    unsigned m_line;
    unsigned m_column;
    unsigned m_repeatCount;
    String m_requestId;
};

} // namespace WebCore

#endif // ConsoleMessage_h
