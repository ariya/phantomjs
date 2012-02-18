/*
 * Copyright (C) 2008, 2009 Apple Inc. All Rights Reserved.
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

#ifndef DOMApplicationCache_h
#define DOMApplicationCache_h

#if ENABLE(OFFLINE_WEB_APPLICATIONS)

#include "ApplicationCacheHost.h"
#include "EventListener.h"
#include "EventNames.h"
#include "EventTarget.h"
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

class Frame;
class KURL;

class DOMApplicationCache : public RefCounted<DOMApplicationCache>, public EventTarget {
public:
    static PassRefPtr<DOMApplicationCache> create(Frame* frame) { return adoptRef(new DOMApplicationCache(frame)); }
    ~DOMApplicationCache() { ASSERT(!m_frame); }

    Frame* frame() const { return m_frame; }
    void disconnectFrame();

    unsigned short status() const;
    void update(ExceptionCode&);
    void swapCache(ExceptionCode&);

    // EventTarget impl

    using RefCounted<DOMApplicationCache>::ref;
    using RefCounted<DOMApplicationCache>::deref;

    // Explicitly named attribute event listener helpers

    DEFINE_ATTRIBUTE_EVENT_LISTENER(checking);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(error);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(noupdate);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(downloading);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(progress);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(updateready);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(cached);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(obsolete);

    virtual ScriptExecutionContext* scriptExecutionContext() const;
    DOMApplicationCache* toDOMApplicationCache() { return this; }

    static const AtomicString& toEventType(ApplicationCacheHost::EventID id);

private:
    DOMApplicationCache(Frame*);

    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }
    virtual EventTargetData* eventTargetData();
    virtual EventTargetData* ensureEventTargetData();

    ApplicationCacheHost* applicationCacheHost() const;

    Frame* m_frame;
    EventTargetData m_eventTargetData;
};

} // namespace WebCore

#endif // ENABLE(OFFLINE_WEB_APPLICATIONS)

#endif // DOMApplicationCache_h
