/*
 * Copyright (C) 2013 Google, Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HTMLTreeBuilderSimulator_h
#define HTMLTreeBuilderSimulator_h

#if ENABLE(THREADED_HTML_PARSER)

#include "HTMLParserOptions.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class CompactHTMLToken;
class HTMLTokenizer;
class HTMLTreeBuilder;

class HTMLTreeBuilderSimulator {
    WTF_MAKE_FAST_ALLOCATED;
private:
    enum Namespace {
        HTML,
        SVG,
        MathML
    };

public:
    typedef Vector<Namespace, 1> State;

    explicit HTMLTreeBuilderSimulator(const HTMLParserOptions&);

    static State stateFor(HTMLTreeBuilder*);

    const State& state() const { return m_namespaceStack; }
    void setState(const State& state) { m_namespaceStack = state; }

    bool simulate(const CompactHTMLToken&, HTMLTokenizer*);

private:
    explicit HTMLTreeBuilderSimulator(HTMLTreeBuilder*);

    bool inForeignContent() const { return m_namespaceStack.last() != HTML; }

    HTMLParserOptions m_options;
    State m_namespaceStack;
};

}

#endif // ENABLE(THREADED_HTML_PARSER)

#endif
