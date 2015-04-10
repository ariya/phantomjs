/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef NotificationPresenterClientQt_h
#define NotificationPresenterClientQt_h

#include "Notification.h"
#include "NotificationClient.h"
#include "QtPlatformPlugin.h"
#include "Timer.h"
#include "qwebkitplatformplugin.h"

#include <QMultiHash>
#include <QScopedPointer>

class QWebFrameAdapter;
class QWebPageAdapter;

namespace WebCore {

class Document;
class Frame;
class ScriptExecutionContext;

class NotificationWrapper : public QObject, public QWebNotificationData {
    Q_OBJECT
public:
    NotificationWrapper();
    ~NotificationWrapper() { }

    void close();
    void close(Timer<NotificationWrapper>*);
    void sendDisplayEvent(Timer<NotificationWrapper>*);
    const QString title() const;
    const QString message() const;
    const QUrl iconUrl() const;
    const QUrl openerPageUrl() const;

public Q_SLOTS:
    void notificationClosed();
    void notificationClicked();

private:
    OwnPtr<QWebNotificationPresenter> m_presenter;
    Timer<NotificationWrapper> m_closeTimer;
    Timer<NotificationWrapper> m_displayEventTimer;

    friend class NotificationPresenterClientQt;
};

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)

typedef QHash <Notification*, NotificationWrapper*> NotificationsQueue;

class NotificationPresenterClientQt : public NotificationClient {
public:
    NotificationPresenterClientQt();
    ~NotificationPresenterClientQt();

    /* WebCore::NotificationClient interface */
    virtual bool show(Notification*);
    virtual void cancel(Notification*);
    virtual void notificationObjectDestroyed(Notification*);
    virtual void notificationControllerDestroyed();
#if ENABLE(LEGACY_NOTIFICATIONS)
    virtual void requestPermission(ScriptExecutionContext*, PassRefPtr<VoidCallback>);
#endif
#if ENABLE(NOTIFICATIONS)
    virtual void requestPermission(ScriptExecutionContext*, PassRefPtr<NotificationPermissionCallback>);
#endif
    virtual NotificationClient::Permission checkPermission(ScriptExecutionContext*);
    virtual void cancelRequestsForPermission(ScriptExecutionContext*);

    void cancel(NotificationWrapper*);

    void setNotificationsAllowedForFrame(Frame*, bool allowed);

    static bool dumpNotification;

    void addClient() { m_clientCount++; }
#ifndef QT_NO_SYSTEMTRAYICON
    bool hasSystemTrayIcon() const { return !m_systemTrayIcon.isNull(); }
    void setSystemTrayIcon(QObject* icon) { m_systemTrayIcon.reset(icon); }
#endif
    void removeClient();
    static NotificationPresenterClientQt* notificationPresenter();

    Notification* notificationForWrapper(const NotificationWrapper*) const;
    void notificationClicked(NotificationWrapper*);
    void notificationClicked(const QString& title);
    void sendDisplayEvent(NotificationWrapper*);

    void clearCachedPermissions();

private:
    void sendEvent(Notification*, const AtomicString& eventName);
    void displayNotification(Notification*);
    void removeReplacedNotificationFromQueue(Notification*);
    void detachNotification(Notification*);
    void dumpReplacedIdText(Notification*);
    void dumpShowText(Notification*);
    QWebPageAdapter* toPage(ScriptExecutionContext*);
    QWebFrameAdapter* toFrame(ScriptExecutionContext*);

    int m_clientCount;
    struct CallbacksInfo {
        QWebFrameAdapter* m_frame;
#if ENABLE(LEGACY_NOTIFICATIONS)
        QList<RefPtr<VoidCallback> > m_voidCallbacks;
#endif
#if ENABLE(NOTIFICATIONS)
        QList<RefPtr<NotificationPermissionCallback> > m_callbacks;
#endif
    };
    QHash<ScriptExecutionContext*,  CallbacksInfo > m_pendingPermissionRequests;
    QHash<ScriptExecutionContext*, NotificationClient::Permission> m_cachedPermissions;

    NotificationsQueue m_notifications;
    QtPlatformPlugin m_platformPlugin;
#ifndef QT_NO_SYSTEMTRAYICON
    QScopedPointer<QObject> m_systemTrayIcon;
#endif
};

#endif // ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)

}

#endif // NotificationPresenterClientQt_h
