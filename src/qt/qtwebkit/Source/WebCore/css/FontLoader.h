/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#if ENABLE(FONT_LOAD_EVENTS)

#ifndef FontLoader_h
#define FontLoader_h

#include "ActiveDOMObject.h"
#include "EventListener.h"
#include "EventNames.h"
#include "EventTarget.h"
#include "VoidCallback.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {

class CachedFont;
class CSSFontFaceRule;
class CSSFontFaceSource;
class Dictionary;
class Document;
class Event;
class Font;
class ScriptExecutionContext;

class FontLoader : public RefCounted<FontLoader>, public ActiveDOMObject, public EventTarget {
public:
    static PassRefPtr<FontLoader> create(Document* document)
    {
        return adoptRef<FontLoader>(new FontLoader(document));
    }
    virtual ~FontLoader();

    DEFINE_ATTRIBUTE_EVENT_LISTENER(loading);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(loadingdone);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(loadstart);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(load);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(error);

    bool checkFont(const String&, const String&);
    void loadFont(const Dictionary&);
    void notifyWhenFontsReady(PassRefPtr<VoidCallback>);

    bool loading() const { return m_loadingCount > 0; }

    virtual ScriptExecutionContext* scriptExecutionContext() const;
    virtual const AtomicString& interfaceName() const;

    using RefCounted<FontLoader>::ref;
    using RefCounted<FontLoader>::deref;

    Document* document() const { return m_document; }

    void didLayout();
    void beginFontLoading(CSSFontFaceRule*);
    void fontLoaded(CSSFontFaceRule*);
    void loadError(CSSFontFaceRule*, CSSFontFaceSource*);
    void loadingDone();

private:
    FontLoader(Document*);

    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }
    virtual EventTargetData* eventTargetData();
    virtual EventTargetData* ensureEventTargetData();

    void scheduleEvent(PassRefPtr<Event>);
    void firePendingEvents();
    bool resolveFontStyle(const String&, Font&);

    Document* m_document;
    EventTargetData m_eventTargetData;
    unsigned m_loadingCount;
    Vector<RefPtr<Event> > m_pendingEvents;
    Vector<RefPtr<VoidCallback> > m_callbacks;
    RefPtr<Event> m_loadingDoneEvent;
};

} // namespace WebCore

#endif // FontLoader_h
#endif // ENABLE(FONT_LOAD_EVENTS)
