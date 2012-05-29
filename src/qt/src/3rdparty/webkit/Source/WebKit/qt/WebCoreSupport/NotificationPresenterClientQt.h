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
#include "NotificationPresenter.h"
#include "QtPlatformPlugin.h"
#include "Timer.h"

#include "qwebkitplatformplugin.h"

#include <QMultiHash>
#include <QSystemTrayIcon>

class QWebFrame;
class QWebPage;

namespace WebCore {

class Document;
class Frame;
class ScriptExecutionContext;

class NotificationWrapper : public QObject, public QWebNotificationData {
    Q_OBJECT
public:
    NotificationWrapper();
    ~NotificationWrapper() {}

    void close();
    void close(Timer<NotificationWrapper>*);
    const QString title() const;
    const QString message() const;
    const QByteArray iconData() const;
    const QUrl openerPageUrl() const;

public Q_SLOTS:
    void notificationClosed();
    void notificationClicked();

public:
#ifndef QT_NO_SYSTEMTRAYICON
    OwnPtr<QSystemTrayIcon> m_notificationIcon;
#endif

    OwnPtr<QWebNotificationPresenter> m_presenter;
    Timer<NotificationWrapper> m_closeTimer;
};

#if ENABLE(NOTIFICATIONS)

typedef QHash <Notification*, NotificationWrapper*> NotificationsQueue;

class NotificationPresenterClientQt : public NotificationPresenter {
public:
    NotificationPresenterClientQt();
    ~NotificationPresenterClientQt();

    /* WebCore::NotificationPresenter interface */
    virtual bool show(Notification*);
    virtual void cancel(Notification*);
    virtual void notificationObjectDestroyed(Notification*);
    virtual void requestPermission(ScriptExecutionContext*, PassRefPtr<VoidCallback>);
    virtual NotificationPresenter::Permission checkPermission(ScriptExecutionContext*);
    virtual void cancelRequestsForPermission(ScriptExecutionContext*);

    void cancel(NotificationWrapper*);

    void allowNotificationForFrame(Frame*);

    static bool dumpNotification;

    void addClient() { m_clientCount++; }
    void removeClient();
    static NotificationPresenterClientQt* notificationPresenter();

    Notification* notificationForWrapper(const NotificationWrapper*) const;
    void notificationClicked(NotificationWrapper*);
    void notificationClicked(const QString& title);

private:
    void sendEvent(Notification*, const AtomicString& eventName);
    void displayNotification(Notification*, const QByteArray&);
    void removeReplacedNotificationFromQueue(Notification*);
    void detachNotification(Notification*);
    void dumpReplacedIdText(Notification*);
    void dumpShowText(Notification*);
    QWebPage* toPage(ScriptExecutionContext*);
    QWebFrame* toFrame(ScriptExecutionContext*);

    int m_clientCount;
    struct CallbacksInfo {
        QWebFrame* m_frame;
        QList<RefPtr<VoidCallback> > m_callbacks;
    };
    QHash<ScriptExecutionContext*,  CallbacksInfo > m_pendingPermissionRequests;
    QHash<ScriptExecutionContext*, NotificationPresenter::Permission> m_cachedPermissions;

    NotificationsQueue m_notifications;
    QtPlatformPlugin m_platformPlugin;
};

#endif

}

#endif
