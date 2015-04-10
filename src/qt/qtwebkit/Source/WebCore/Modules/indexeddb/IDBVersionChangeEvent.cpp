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

#include "config.h"
#include "IDBVersionChangeEvent.h"

#if ENABLE(INDEXED_DATABASE)

#include "EventNames.h"
#include "IDBAny.h"

namespace WebCore {

PassRefPtr<IDBVersionChangeEvent> IDBVersionChangeEvent::create(PassRefPtr<IDBAny> oldVersion, PassRefPtr<IDBAny> newVersion, const AtomicString& eventType)
{
    return adoptRef(new IDBVersionChangeEvent(oldVersion, newVersion, eventType));
}

IDBVersionChangeEvent::IDBVersionChangeEvent(PassRefPtr<IDBAny> oldVersion, PassRefPtr<IDBAny> newVersion, const AtomicString& eventType)
    : Event(eventType, false /*canBubble*/, false /*cancelable*/)
    , m_oldVersion(oldVersion)
    , m_newVersion(newVersion)
{
}

IDBVersionChangeEvent::~IDBVersionChangeEvent()
{
}

const AtomicString& IDBVersionChangeEvent::interfaceName() const
{
    return eventNames().interfaceForIDBVersionChangeEvent;
}

} // namespace WebCore

#endif
