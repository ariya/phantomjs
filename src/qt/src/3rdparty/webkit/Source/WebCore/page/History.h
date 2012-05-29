/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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

#ifndef History_h
#define History_h

#include "KURL.h"
#include <wtf/Forward.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class Frame;
class ScriptExecutionContext;
class SerializedScriptValue;
typedef int ExceptionCode;

class History : public RefCounted<History> {
public:
    static PassRefPtr<History> create(Frame* frame) { return adoptRef(new History(frame)); }
    
    Frame* frame() const;
    void disconnectFrame();

    unsigned length() const;
    void back();
    void forward();
    void go(int distance);

    void back(ScriptExecutionContext*);
    void forward(ScriptExecutionContext*);
    void go(ScriptExecutionContext*, int distance);

    enum StateObjectType {
        StateObjectPush,
        StateObjectReplace
    };
    void stateObjectAdded(PassRefPtr<SerializedScriptValue>, const String& title, const String& url, StateObjectType, ExceptionCode&);

private:
    History(Frame*);

    KURL urlForState(const String& url);

    Frame* m_frame;
};

} // namespace WebCore

#endif // History_h
