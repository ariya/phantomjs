/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef AbstractWorker_h
#define AbstractWorker_h

#if ENABLE(WORKERS)

#include "ActiveDOMObject.h"
#include "EventListener.h"
#include "EventNames.h"
#include "EventTarget.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

    class KURL;
    class ScriptExecutionContext;

    class AbstractWorker : public RefCounted<AbstractWorker>, public ActiveDOMObject, public EventTarget {
    public:
        // EventTarget APIs
        virtual ScriptExecutionContext* scriptExecutionContext() const OVERRIDE { return ActiveDOMObject::scriptExecutionContext(); }

        DEFINE_ATTRIBUTE_EVENT_LISTENER(error);

        using RefCounted<AbstractWorker>::ref;
        using RefCounted<AbstractWorker>::deref;

        virtual void contextDestroyed() OVERRIDE;
        AbstractWorker(ScriptExecutionContext*);
        virtual ~AbstractWorker();

    protected:
        // Helper function that converts a URL to an absolute URL and checks the result for validity.
        KURL resolveURL(const String& url, ExceptionCode& ec);
        intptr_t asID() const { return reinterpret_cast<intptr_t>(this); }

    private:
        virtual void refEventTarget() OVERRIDE { ref(); }
        virtual void derefEventTarget() OVERRIDE { deref(); }
        virtual EventTargetData* eventTargetData() OVERRIDE;
        virtual EventTargetData* ensureEventTargetData() OVERRIDE;
        
        EventTargetData m_eventTargetData;
    };

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // AbstractWorker_h
