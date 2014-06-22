/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef WebNotificationPresenter_h  
#define WebNotificationPresenter_h

#include "qwebkitplatformplugin.h"

#include <QBitmap>
#include <QEvent>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QWidget>

class WebNotificationWidget : public QWidget
{
    Q_OBJECT
public:
    WebNotificationWidget();
    virtual ~WebNotificationWidget();

    void showNotification(const QWebNotificationData*);
    bool event(QEvent*);

Q_SIGNALS:
    void notificationClosed();
    void notificationClicked();
};

class WebNotificationPresenter : public QWebNotificationPresenter
{
    Q_OBJECT
public:
    WebNotificationPresenter()
        : QWebNotificationPresenter()
    {
        m_widget = new WebNotificationWidget();
        connect(m_widget, SIGNAL(notificationClosed()), this, SIGNAL(notificationClosed()));
        connect(m_widget, SIGNAL(notificationClicked()), this, SIGNAL(notificationClicked()));
    }
    virtual ~WebNotificationPresenter() { m_widget->close(); delete m_widget; }

    void showNotification(const QWebNotificationData* data) { m_widget->showNotification(data); }
    
private:
    WebNotificationWidget* m_widget;
};

#endif // WebNotificationsUi_h
