/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DOMTokenList_h
#define DOMTokenList_h

#include <wtf/text/AtomicString.h>
#include <wtf/Vector.h>

namespace WebCore {

class Element;

typedef int ExceptionCode;

class DOMTokenList {
    WTF_MAKE_NONCOPYABLE(DOMTokenList); WTF_MAKE_FAST_ALLOCATED;
public:
    DOMTokenList() { }
    virtual ~DOMTokenList() {};

    virtual void ref() = 0;
    virtual void deref() = 0;

    virtual unsigned length() const = 0;
    virtual const AtomicString item(unsigned index) const = 0;

    bool contains(const AtomicString&, ExceptionCode&) const;
    virtual void add(const Vector<String>&, ExceptionCode&);
    void add(const AtomicString&, ExceptionCode&);
    virtual void remove(const Vector<String>&, ExceptionCode&);
    void remove(const AtomicString&, ExceptionCode&);
    bool toggle(const AtomicString&, ExceptionCode&);
    bool toggle(const AtomicString&, bool force, ExceptionCode&);

    AtomicString toString() const { return value(); }

    virtual Element* element() { return 0; }

protected:
    virtual AtomicString value() const = 0;
    virtual void setValue(const AtomicString&) = 0;

    virtual void addInternal(const AtomicString&);
    virtual bool containsInternal(const AtomicString&) const = 0;
    virtual void removeInternal(const AtomicString&);

    static bool validateToken(const AtomicString&, ExceptionCode&);
    static bool validateTokens(const Vector<String>&, ExceptionCode&);
    static String addToken(const AtomicString&, const AtomicString&);
    static String addTokens(const AtomicString&, const Vector<String>&);
    static String removeToken(const AtomicString&, const AtomicString&);
    static String removeTokens(const AtomicString&, const Vector<String>&);
};

} // namespace WebCore

#endif // DOMTokenList_h
