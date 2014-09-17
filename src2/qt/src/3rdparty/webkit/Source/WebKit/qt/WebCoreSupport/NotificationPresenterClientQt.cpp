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

#include "config.h"
#include "NotificationPresenterClientQt.h"

#include "Document.h"
#include "DumpRenderTreeSupportQt.h"
#include "EventNames.h"
#include "KURL.h"
#include "Page.h"
#include "QtPlatformPlugin.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"
#include "UserGestureIndicator.h"

#include "qwebframe_p.h"
#include "qwebkitglobal.h"
#include "qwebpage.h"

namespace WebCore {

#if ENABLE(NOTIFICATIONS)

const double notificationTimeout = 10.0;

bool NotificationPresenterClientQt::dumpNotification = false;

NotificationPresenterClientQt* s_notificationPresenter = 0;

NotificationPresenterClientQt* NotificationPresenterClientQt::notificationPresenter()
{
    if (s_notificationPresenter)
        return s_notificationPresenter;

    s_notificationPresenter = new NotificationPresenterClientQt();
    return s_notificationPresenter;
}

#endif

NotificationWrapper::NotificationWrapper()
    : m_closeTimer(this, &NotificationWrapper::close)
{
#if ENABLE(NOTIFICATIONS)

#ifndef QT_NO_SYSTEMTRAYICON
    m_notificationIcon = 0;
#endif
    m_presenter = 0;
#endif
}

void NotificationWrapper::close(Timer<NotificationWrapper>*)
{
#if ENABLE(NOTIFICATIONS)
    NotificationPresenterClientQt::notificationPresenter()->cancel(this);
#endif
}

const QString NotificationWrapper::title() const
{
#if ENABLE(NOTIFICATIONS)
    Notification* notification = NotificationPresenterClientQt::notificationPresenter()->notificationForWrapper(this);
    if (notification)
        return notification->contents().title();
#endif
    return QString();
}

const QString NotificationWrapper::message() const
{
#if ENABLE(NOTIFICATIONS)
    Notification* notification = NotificationPresenterClientQt::notificationPresenter()->notificationForWrapper(this);
    if (notification)
        return notification->contents().body();
#endif
    return QString();
}

const QByteArray NotificationWrapper::iconData() const
{
    QByteArray iconData;
#if ENABLE(NOTIFICATIONS)
    Notification* notification = NotificationPresenterClientQt::notificationPresenter()->notificationForWrapper(this);
    if (notification) {
        if (notification->iconData())
            iconData = QByteArray::fromRawData(notification->iconData()->data(), notification->iconData()->size());
    }
#endif
    return iconData;
}

const QUrl NotificationWrapper::openerPageUrl() const
{
    QUrl url;
#if ENABLE(NOTIFICATIONS)
    Notification* notification = NotificationPresenterClientQt::notificationPresenter()->notificationForWrapper(this);
    if (notification) {
        if (notification->scriptExecutionContext()) 
            url = static_cast<Document*>(notification->scriptExecutionContext())->page()->mainFrame()->document()->url();
    }
#endif
    return url;
}

void NotificationWrapper::notificationClicked()
{
#if ENABLE(NOTIFICATIONS)
    NotificationPresenterClientQt::notificationPresenter()->notificationClicked(this);
#endif
}

void NotificationWrapper::notificationClosed()
{
#if ENABLE(NOTIFICATIONS)
    NotificationPresenterClientQt::notificationPresenter()->cancel(this);
#endif
}

#if ENABLE(NOTIFICATIONS)

NotificationPresenterClientQt::NotificationPresenterClientQt() : m_clientCount(0)
{
}

NotificationPresenterClientQt::~NotificationPresenterClientQt()
{
    while (!m_notifications.isEmpty()) {
        NotificationsQueue::Iterator iter = m_notifications.begin();
        detachNotification(iter.key());
    }
}

void NotificationPresenterClientQt::removeClient()
{
    m_clientCount--;
    if (!m_clientCount) {
        s_notificationPresenter = 0;
        delete this;
    }
}

bool NotificationPresenterClientQt::show(Notification* notification)
{
    // FIXME: workers based notifications are not supported yet.
    if (notification->scriptExecutionContext()->isWorkerContext())
        return false;
    notification->setPendingActivity(notification);
    if (!notification->replaceId().isEmpty())
        removeReplacedNotificationFromQueue(notification);
    if (dumpNotification)
        dumpShowText(notification);
    QByteArray iconData;
    if (notification->iconData())
        iconData = QByteArray::fromRawData(notification->iconData()->data(), notification->iconData()->size());
    displayNotification(notification, iconData);
    notification->releaseIconData();
    return true;
}

void NotificationPresenterClientQt::displayNotification(Notification* notification, const QByteArray& bytes)
{
    NotificationWrapper* wrapper = new NotificationWrapper();
    m_notifications.insert(notification, wrapper);
    QString title;
    QString message;
    // FIXME: download & display HTML notifications
    if (notification->isHTML())
        message = notification->url().string();
    else {
        title = notification->contents().title();
        message = notification->contents().body();
    }

    if (m_platformPlugin.plugin() && m_platformPlugin.plugin()->supportsExtension(QWebKitPlatformPlugin::Notifications))
        wrapper->m_presenter = m_platformPlugin.createNotificationPresenter();

    if (!wrapper->m_presenter) {
#ifndef QT_NO_SYSTEMTRAYICON
        if (!dumpNotification)
            wrapper->m_closeTimer.startOneShot(notificationTimeout);
        QPixmap pixmap;
        if (bytes.length() && pixmap.loadFromData(bytes)) {
            QIcon icon(pixmap);
            wrapper->m_notificationIcon = new QSystemTrayIcon(icon);
        } else
            wrapper->m_notificationIcon = new QSystemTrayIcon();
#endif
    }

    sendEvent(notification, "display");

    // Make sure the notification was not cancelled during handling the display event
    if (m_notifications.find(notification) == m_notifications.end())
        return;

    if (wrapper->m_presenter) {
        wrapper->connect(wrapper->m_presenter.get(), SIGNAL(notificationClosed()), wrapper, SLOT(notificationClosed()), Qt::QueuedConnection);
        wrapper->connect(wrapper->m_presenter.get(), SIGNAL(notificationClicked()), wrapper, SLOT(notificationClicked()));
        wrapper->m_presenter->showNotification(wrapper);
        return;
    }

#ifndef QT_NO_SYSTEMTRAYICON
    wrapper->connect(wrapper->m_notificationIcon.get(), SIGNAL(messageClicked()), wrapper, SLOT(notificationClicked()));
    wrapper->m_notificationIcon->show();
    wrapper->m_notificationIcon->showMessage(notification->contents().title(), notification->contents().body());
#endif
}

void NotificationPresenterClientQt::cancel(Notification* notification)
{
    if (dumpNotification && notification->scriptExecutionContext()) {
        if (notification->isHTML())
            printf("DESKTOP NOTIFICATION CLOSED: %s\n", QString(notification->url().string()).toUtf8().constData());
        else
            printf("DESKTOP NOTIFICATION CLOSED: %s\n", QString(notification->contents().title()).toUtf8().constData());
    }

    NotificationsQueue::Iterator iter = m_notifications.find(notification);
    if (iter != m_notifications.end()) {
        sendEvent(notification, eventNames().closeEvent);
        detachNotification(notification);
    }
}

void NotificationPresenterClientQt::cancel(NotificationWrapper* wrapper)
{
    Notification* notification = notificationForWrapper(wrapper);
    if (notification)
        cancel(notification);
}

void NotificationPresenterClientQt::notificationClicked(NotificationWrapper* wrapper)
{
    Notification* notification =  notificationForWrapper(wrapper);
    if (notification) {
        // Make sure clicks on notifications are treated as user gestures.
        UserGestureIndicator gestureIndicator(DefinitelyProcessingUserGesture);
        sendEvent(notification, eventNames().clickEvent);
    }
}

void NotificationPresenterClientQt::notificationClicked(const QString& title)
{
    if (!dumpNotification)
        return;
    NotificationsQueue::ConstIterator end = m_notifications.end();
    NotificationsQueue::ConstIterator iter = m_notifications.begin();
    Notification* notification = 0;
    while (iter != end) {
        notification = iter.key();
        QString notificationTitle;
        if (notification->isHTML())
            notificationTitle = notification->url().string();
        else
            notificationTitle = notification->contents().title();
        if (notificationTitle == title)
            break;
        iter++;
    }
    if (notification)
        sendEvent(notification, eventNames().clickEvent);
}

Notification* NotificationPresenterClientQt::notificationForWrapper(const NotificationWrapper* wrapper) const
{
    NotificationsQueue::ConstIterator end = m_notifications.end();
    NotificationsQueue::ConstIterator iter = m_notifications.begin();
    while (iter != end && iter.value() != wrapper)
        iter++;
    if (iter != end)
        return iter.key();
    return 0;
}

void NotificationPresenterClientQt::notificationObjectDestroyed(Notification* notification)
{
    // Called from ~Notification(), Remove the entry from the notifications list and delete the icon.
    NotificationsQueue::Iterator iter = m_notifications.find(notification);
    if (iter != m_notifications.end())
        delete m_notifications.take(notification);
}

void NotificationPresenterClientQt::requestPermission(ScriptExecutionContext* context, PassRefPtr<VoidCallback> callback)
{  
    if (dumpNotification)
        printf("DESKTOP NOTIFICATION PERMISSION REQUESTED: %s\n", QString(context->securityOrigin()->toString()).toUtf8().constData());

    QHash<ScriptExecutionContext*, CallbacksInfo >::iterator iter = m_pendingPermissionRequests.find(context);
    if (iter != m_pendingPermissionRequests.end())
        iter.value().m_callbacks.append(callback);
    else {
        RefPtr<VoidCallback> cb = callback;
        CallbacksInfo info;
        info.m_frame = toFrame(context);
        info.m_callbacks.append(cb);
        m_pendingPermissionRequests.insert(context, info);

        if (toPage(context) && toFrame(context)) {
            m_pendingPermissionRequests.insert(context, info);
            emit toPage(context)->featurePermissionRequested(toFrame(context), QWebPage::Notifications);
        }
    }
}

NotificationPresenter::Permission NotificationPresenterClientQt::checkPermission(ScriptExecutionContext* context)
{
    return m_cachedPermissions.value(context, NotificationPresenter::PermissionNotAllowed);
}

void NotificationPresenterClientQt::cancelRequestsForPermission(ScriptExecutionContext* context)
{
    m_cachedPermissions.remove(context);

    QHash<ScriptExecutionContext*, CallbacksInfo >::iterator iter = m_pendingPermissionRequests.find(context);
    if (iter == m_pendingPermissionRequests.end())
        return;

    QWebFrame* frame = iter.value().m_frame;
    if (!frame)
        return;
    QWebPage* page = frame->page();
    m_pendingPermissionRequests.erase(iter);

    if (!page)
        return;

    if (dumpNotification)
        printf("DESKTOP NOTIFICATION PERMISSION REQUEST CANCELLED: %s\n", QString(context->securityOrigin()->toString()).toUtf8().constData());

    emit page->featurePermissionRequestCanceled(frame, QWebPage::Notifications);
}

void NotificationPresenterClientQt::allowNotificationForFrame(Frame* frame)
{
    m_cachedPermissions.insert(frame->document(), NotificationPresenter::PermissionAllowed);

    QHash<ScriptExecutionContext*,  CallbacksInfo>::iterator iter = m_pendingPermissionRequests.begin();
    while (iter != m_pendingPermissionRequests.end()) {
        if (iter.key() == frame->document())
            break;
    }

    if (iter == m_pendingPermissionRequests.end())
        return;

    QList<RefPtr<VoidCallback> >& callbacks = iter.value().m_callbacks;
    for (int i = 0; i < callbacks.size(); i++)
        callbacks.at(i)->handleEvent();
    m_pendingPermissionRequests.remove(iter.key());
}

void NotificationPresenterClientQt::sendEvent(Notification* notification, const AtomicString& eventName)
{
    if (notification->scriptExecutionContext())
        notification->dispatchEvent(Event::create(eventName, false, true));
}

void NotificationPresenterClientQt::removeReplacedNotificationFromQueue(Notification* notification)
{
    Notification* oldNotification = 0;
    NotificationsQueue::Iterator end = m_notifications.end();
    NotificationsQueue::Iterator iter = m_notifications.begin();

    while (iter != end) {
        Notification* existingNotification = iter.key();
        if (existingNotification->replaceId() == notification->replaceId() && existingNotification->url().protocol() == notification->url().protocol() && existingNotification->url().host() == notification->url().host()) {
            oldNotification = iter.key();
            break;
        }
        iter++;
    }

    if (oldNotification) {
        if (dumpNotification)
            dumpReplacedIdText(oldNotification);
        sendEvent(oldNotification, eventNames().closeEvent);
        detachNotification(oldNotification);
    }
}

void NotificationPresenterClientQt::detachNotification(Notification* notification)
{
    delete m_notifications.take(notification);
    notification->detachPresenter();
    notification->unsetPendingActivity(notification);
}

void NotificationPresenterClientQt::dumpReplacedIdText(Notification* notification)
{
    if (notification)
        printf("REPLACING NOTIFICATION %s\n", notification->isHTML() ? QString(notification->url().string()).toUtf8().constData() : QString(notification->contents().title()).toUtf8().constData());
}

void NotificationPresenterClientQt::dumpShowText(Notification* notification)
{
    if (notification->isHTML())
        printf("DESKTOP NOTIFICATION: contents at %s\n", QString(notification->url().string()).toUtf8().constData());
    else {
        printf("DESKTOP NOTIFICATION:%s icon %s, title %s, text %s\n",
                notification->dir() == "rtl" ? "(RTL)" : "",
            QString(notification->contents().icon().string()).toUtf8().constData(), QString(notification->contents().title()).toUtf8().constData(),
            QString(notification->contents().body()).toUtf8().constData());
    }
}

QWebPage* NotificationPresenterClientQt::toPage(ScriptExecutionContext* context)
{
    if (!context || context->isWorkerContext())
        return 0;

    Document* document = static_cast<Document*>(context);

    Page* page = document->page();
    if (!page || !page->mainFrame())
        return 0;

    return QWebFramePrivate::kit(page->mainFrame())->page();
}

QWebFrame* NotificationPresenterClientQt::toFrame(ScriptExecutionContext* context)
{
    if (!context || context->isWorkerContext())
        return 0;

    Document* document = static_cast<Document*>(context);
    if (!document || !document->frame())
        return 0;

    return QWebFramePrivate::kit(document->frame());
}

#endif // ENABLE(NOTIFICATIONS)
}

#include "moc_NotificationPresenterClientQt.cpp"
