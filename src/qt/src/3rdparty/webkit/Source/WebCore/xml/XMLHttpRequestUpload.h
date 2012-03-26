/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef XMLHttpRequestUpload_h
#define XMLHttpRequestUpload_h

#include "EventListener.h"
#include "EventNames.h"
#include "EventTarget.h"
#include "XMLHttpRequest.h"
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

    class ScriptExecutionContext;
    class XMLHttpRequest;

    class XMLHttpRequestUpload : public EventTarget {
    public:
        static PassOwnPtr<XMLHttpRequestUpload> create(XMLHttpRequest* xmlHttpRequest)
        {
            return adoptPtr(new XMLHttpRequestUpload(xmlHttpRequest));
        }

        void ref() { m_xmlHttpRequest->ref(); }
        void deref() { m_xmlHttpRequest->deref(); }
        XMLHttpRequest* xmlHttpRequest() const { return m_xmlHttpRequest; }

        virtual XMLHttpRequestUpload* toXMLHttpRequestUpload() { return this; }

        ScriptExecutionContext* scriptExecutionContext() const;

        DEFINE_ATTRIBUTE_EVENT_LISTENER(abort);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(error);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(load);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(loadstart);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(progress);

    private:
        XMLHttpRequestUpload(XMLHttpRequest*);

        virtual void refEventTarget() { ref(); }
        virtual void derefEventTarget() { deref(); }
        virtual EventTargetData* eventTargetData();
        virtual EventTargetData* ensureEventTargetData();

        XMLHttpRequest* m_xmlHttpRequest;
        EventTargetData m_eventTargetData;
    };
    
} // namespace WebCore

#endif // XMLHttpRequestUpload_h
