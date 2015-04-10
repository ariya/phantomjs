/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2009, 2011, 2012 Apple Inc. All rights reserved.
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

#ifndef Notification_h  
#define Notification_h

#include "ActiveDOMObject.h"
#include "EventNames.h"
#include "EventTarget.h"
#include "KURL.h"
#include "NotificationClient.h"
#include "SharedBuffer.h"
#include "TextDirection.h"
#include "ThreadableLoaderClient.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/text/AtomicStringHash.h>

#if ENABLE(NOTIFICATIONS)
#include "Timer.h"
#endif

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
namespace WebCore {

class Dictionary;
class NotificationCenter;
class NotificationPermissionCallback;
class ResourceError;
class ResourceResponse;
class ScriptExecutionContext;
class ThreadableLoader;

typedef int ExceptionCode;

class Notification : public RefCounted<Notification>, public ActiveDOMObject, public EventTarget {
    WTF_MAKE_FAST_ALLOCATED;
public:
    Notification();
#if ENABLE(LEGACY_NOTIFICATIONS)
    static PassRefPtr<Notification> create(const String& title, const String& body, const String& iconURI, ScriptExecutionContext*, ExceptionCode&, PassRefPtr<NotificationCenter> provider);
#endif
#if ENABLE(NOTIFICATIONS)
    static PassRefPtr<Notification> create(ScriptExecutionContext*, const String& title, const Dictionary& options);
#endif
    
    virtual ~Notification();

    void show();
#if ENABLE(LEGACY_NOTIFICATIONS)
    void cancel() { close(); }
#endif
    void close();

    KURL iconURL() const { return m_icon; }
    void setIconURL(const KURL& url) { m_icon = url; }

    String title() const { return m_title; }
    String body() const { return m_body; }

    String lang() const { return m_lang; }
    void setLang(const String& lang) { m_lang = lang; }

    String dir() const { return m_direction; }
    void setDir(const String& dir) { m_direction = dir; }

#if ENABLE(LEGACY_NOTIFICATIONS)
    String replaceId() const { return tag(); }
    void setReplaceId(const String& replaceId) { setTag(replaceId); }
#endif

    String tag() const { return m_tag; }
    void setTag(const String& tag) { m_tag = tag; }

    TextDirection direction() const { return dir() == "rtl" ? RTL : LTR; }

#if ENABLE(LEGACY_NOTIFICATIONS)
    EventListener* ondisplay() { return getAttributeEventListener(eventNames().showEvent); }
    void setOndisplay(PassRefPtr<EventListener> listener) { setAttributeEventListener(eventNames().showEvent, listener); }
#endif
    DEFINE_ATTRIBUTE_EVENT_LISTENER(show);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(error);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(close);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(click);
    
    void dispatchClickEvent();
    void dispatchCloseEvent();
    void dispatchErrorEvent();
    void dispatchShowEvent();

    using RefCounted<Notification>::ref;
    using RefCounted<Notification>::deref;

    // EventTarget interface
    virtual const AtomicString& interfaceName() const;
    virtual ScriptExecutionContext* scriptExecutionContext() const { return ActiveDOMObject::scriptExecutionContext(); }

    // ActiveDOMObject interface
    virtual void contextDestroyed();

    void stopLoadingIcon();

    // Deprecated. Use functions from NotificationCenter.
    void detachPresenter() { }

    void finalize();

#if ENABLE(NOTIFICATIONS)
    static const String permission(ScriptExecutionContext*);
    static const String permissionString(NotificationClient::Permission);
    static void requestPermission(ScriptExecutionContext*, PassRefPtr<NotificationPermissionCallback> = 0);
#endif

private:
#if ENABLE(LEGACY_NOTIFICATIONS)
    Notification(const String& title, const String& body, const String& iconURI, ScriptExecutionContext*, ExceptionCode&, PassRefPtr<NotificationCenter>);
#endif
#if ENABLE(NOTIFICATIONS)
    Notification(ScriptExecutionContext*, const String& title);
#endif

    void setBody(const String& body) { m_body = body; }

    // EventTarget interface
    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }
    virtual EventTargetData* eventTargetData();
    virtual EventTargetData* ensureEventTargetData();

    void startLoadingIcon();
    void finishLoadingIcon();

#if ENABLE(NOTIFICATIONS)
    void taskTimerFired(Timer<Notification>*);
#endif

    // Text notifications.
    KURL m_icon;
    String m_title;
    String m_body;
    String m_direction;
    String m_lang;
    String m_tag;

    enum NotificationState {
        Idle = 0,
        Showing = 1,
        Closed = 2,
    };

    NotificationState m_state;

    RefPtr<NotificationCenter> m_notificationCenter;
    
    EventTargetData m_eventTargetData;

#if ENABLE(NOTIFICATIONS)
    OwnPtr<Timer<Notification> > m_taskTimer;
#endif
};

} // namespace WebCore

#endif // ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)

#endif // Notifications_h
