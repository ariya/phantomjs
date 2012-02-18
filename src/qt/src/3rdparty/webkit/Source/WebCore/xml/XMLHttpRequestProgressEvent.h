/*
 * Copyright (C) 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Julien Chaffraix <jchaffraix@webkit.org>. All rights reserved.
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

#ifndef XMLHttpRequestProgressEvent_h
#define XMLHttpRequestProgressEvent_h

#include "ProgressEvent.h"

namespace WebCore {

class XMLHttpRequestProgressEvent : public ProgressEvent {
public:
    static PassRefPtr<XMLHttpRequestProgressEvent> create()
    {
        return adoptRef(new XMLHttpRequestProgressEvent);
    }
    static PassRefPtr<XMLHttpRequestProgressEvent> create(const AtomicString& type, bool lengthComputable = false, unsigned long long loaded = 0, unsigned long long total = 0)
    {
        return adoptRef(new XMLHttpRequestProgressEvent(type, lengthComputable, loaded, total));
    }

    // Those 2 synonyms are included for compatibility with Firefox.
    unsigned long long position() const { return loaded(); }
    unsigned long long totalSize() const { return total(); }

private:
    virtual bool isXMLHttpRequestProgressEvent() const { return true; }

    XMLHttpRequestProgressEvent() { }
    XMLHttpRequestProgressEvent(const AtomicString& type, bool lengthComputable, unsigned long long loaded, unsigned long long total)
        : ProgressEvent(type, lengthComputable, loaded, total)
    {
    }
};

} // namespace WebCore

#endif // XMLHttpRequestProgressEvent_h
