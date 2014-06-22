/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef IDBVersionChangeEvent_h
#define IDBVersionChangeEvent_h

#if ENABLE(INDEXED_DATABASE)

#include "Event.h"
#include "IDBAny.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class IDBVersionChangeEvent : public Event {
public:
    static PassRefPtr<IDBVersionChangeEvent> create(PassRefPtr<IDBAny> oldVersion = IDBAny::createNull(), PassRefPtr<IDBAny> newVersion = IDBAny::createNull(), const AtomicString& eventType = AtomicString());
    virtual ~IDBVersionChangeEvent();

    virtual PassRefPtr<IDBAny> oldVersion() { return m_oldVersion; }
    virtual PassRefPtr<IDBAny> newVersion() { return m_newVersion; }

    virtual const AtomicString& interfaceName() const;

private:
    IDBVersionChangeEvent(PassRefPtr<IDBAny> oldVersion, PassRefPtr<IDBAny> newVersion, const AtomicString& eventType);

    RefPtr<IDBAny> m_oldVersion;
    RefPtr<IDBAny> m_newVersion;
};

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)

#endif // IDBVersionChangeEvent_h
