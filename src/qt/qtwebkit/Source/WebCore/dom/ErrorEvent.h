/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef ErrorEvent_h
#define ErrorEvent_h

#include "Event.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

struct ErrorEventInit : public EventInit {
    ErrorEventInit();

    String message;
    String filename;
    unsigned lineno;
    unsigned colno;
};

class ErrorEvent : public Event {
public:
    static PassRefPtr<ErrorEvent> create()
    {
        return adoptRef(new ErrorEvent);
    }
    static PassRefPtr<ErrorEvent> create(const String& message, const String& fileName, unsigned lineNumber, unsigned columnNumber)
    {
        return adoptRef(new ErrorEvent(message, fileName, lineNumber, columnNumber));
    }
    static PassRefPtr<ErrorEvent> create(const AtomicString& type, const ErrorEventInit& initializer)
    {
        return adoptRef(new ErrorEvent(type, initializer));
    }
    virtual ~ErrorEvent();

    const String& message() const { return m_message; }
    const String& filename() const { return m_fileName; }
    unsigned lineno() const { return m_lineNumber; }
    unsigned colno() const { return m_columnNumber; }

    virtual const AtomicString& interfaceName() const;

private:
    ErrorEvent();
    ErrorEvent(const String& message, const String& fileName, unsigned lineNumber, unsigned columnNumber);
    ErrorEvent(const AtomicString&, const ErrorEventInit&);

    String m_message;
    String m_fileName;
    unsigned m_lineNumber;
    unsigned m_columnNumber;
};

} // namespace WebCore

#endif // ErrorEvent_h
