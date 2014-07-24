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

#include "WebNotificationPresenter.h"

WebNotificationWidget::WebNotificationWidget()
    : QWidget()
{
}

WebNotificationWidget::~WebNotificationWidget()
{
}

void WebNotificationWidget::showNotification(const QWebNotificationData* data)
{
    QPixmap mask;
    QPainter painter(&mask);
    painter.fillRect(0, 0, 300, 100, Qt::lightGray);
    QBitmap bitmap(mask);
    setMask(bitmap);
    QGridLayout* layout = new QGridLayout(this);
    layout->addWidget(new QLabel(data->title()), 0, 0, 1, 5);
    int messagePosition = 0;

    QLabel* messageLabel = new QLabel(data->message());
    messageLabel->setMask(bitmap);
    messageLabel->setWordWrap(true);
    layout->addWidget(messageLabel, 1, messagePosition, 1, 5 - messagePosition);
    setLayout(layout);
    setFixedSize(300, 100);
    show();
}

bool WebNotificationWidget::event(QEvent* ev)
{
    if (ev->type() == QEvent::MouseButtonRelease) {
        emit notificationClicked();
        return true;
    }
    if (ev->type() == QEvent::Close) {
        emit notificationClosed();
        return true;
    }
    return QWidget::event(ev);
}

