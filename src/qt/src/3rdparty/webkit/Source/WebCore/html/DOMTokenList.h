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

#include "ExceptionCode.h"
#include <wtf/text/AtomicString.h>
#include <wtf/Vector.h>

namespace WebCore {

class Element;

class DOMTokenList {
    WTF_MAKE_NONCOPYABLE(DOMTokenList); WTF_MAKE_FAST_ALLOCATED;
public:
    DOMTokenList() { }
    virtual ~DOMTokenList() {};

    virtual void ref() = 0;
    virtual void deref() = 0;

    virtual unsigned length() const = 0;
    virtual const AtomicString item(unsigned index) const = 0;
    virtual bool contains(const AtomicString&, ExceptionCode&) const = 0;
    virtual void add(const AtomicString&, ExceptionCode&) = 0;
    virtual void remove(const AtomicString&, ExceptionCode&) = 0;
    virtual bool toggle(const AtomicString&, ExceptionCode&) = 0;
    virtual String toString() const = 0;

    virtual Element* element() { return 0; }

protected:
    static bool validateToken(const AtomicString&, ExceptionCode&);
    static String addToken(const AtomicString&, const AtomicString&);
    static String removeToken(const AtomicString&, const AtomicString&);
};

} // namespace WebCore

#endif // DOMTokenList_h
