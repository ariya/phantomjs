/*
 * Copyright (C) 2010 Girish Ramakrishnan <girish@forwardbias.in>
 * Copyright (C) 2009, 2011 Nokia Corporation and/or its subsidiary(-ies)
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

#include "config.h"
#include "QtWebComboBox.h"

#ifndef QT_NO_COMBOBOX

#include <QAbstractItemView>
#include <QCoreApplication>
#include <QtGui/QMouseEvent>

namespace WebCore {

QtWebComboBox::QtWebComboBox()
    : QComboBox(), m_hiding(false), m_deleteAfterHiding(false)
{
    // Install an event filter on the view inside the combo box popup to make sure we know
    // when the popup got closed. E.g. QComboBox::hidePopup() won't be called when the popup
    // is closed by a mouse wheel event outside its window.
    view()->installEventFilter(this);
}

void QtWebComboBox::showPopupAtCursorPosition()
{
    QMouseEvent event(QEvent::MouseButtonPress, QCursor::pos(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(this, &event);
}

bool QtWebComboBox::eventFilter(QObject* watched, QEvent* event)
{
    Q_ASSERT(watched == view());
    if (event->type() == QEvent::Hide)
        emit didHide();
    return false;
}

void QtWebComboBox::hidePopup()
{
    m_hiding = true;
    // QComboBox::hidePopup() runs an eventloop, we need to make sure we do not delete ourselves in that loop.
    QComboBox::hidePopup();
    m_hiding = false;
    if (m_deleteAfterHiding)
        deleteLater();
}


void QtWebComboBox::deleteComboBox()
{
    if (!m_hiding)
        deleteLater();
    else
        m_deleteAfterHiding = true;
}

} // namespace

#endif // QT_NO_COMBOBOX
