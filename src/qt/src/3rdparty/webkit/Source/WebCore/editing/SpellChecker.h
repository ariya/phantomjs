/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef SpellChecker_h
#define SpellChecker_h

#include "DocumentMarker.h"
#include "TextCheckerClient.h"
#include <wtf/Noncopyable.h>

namespace WebCore {

class TextCheckerClient;
class Frame;
class Node;

class SpellChecker {
    WTF_MAKE_NONCOPYABLE(SpellChecker);
public:
    explicit SpellChecker(Frame*);
    ~SpellChecker();

    bool isAsynchronousEnabled() const;
    bool canCheckAsynchronously(Node*) const;
    bool isBusy() const;
    bool isValid(int sequence) const;
    bool isCheckable(Node*) const;
    void requestCheckingFor(TextCheckingTypeMask, Node*);
    void didCheck(int sequence, const Vector<TextCheckingResult>&);

private:
    bool initRequest(Node*);
    void clearRequest();
    TextCheckerClient* client() const;

    Frame* m_frame;

    RefPtr<Node> m_requestNode;
    String m_requestText;
    int m_requestSequence;
};

} // namespace WebCore

#endif // SpellChecker_h
