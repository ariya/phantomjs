/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qwidget.h>

#include "qeventdispatcher_s60_p.h"

QT_BEGIN_NAMESPACE

QEventDispatcherS60::QEventDispatcherS60(QObject *parent)
    : QEventDispatcherSymbian(parent),
      m_noInputEvents(false)
{
}

QEventDispatcherS60::~QEventDispatcherS60()
{
    for (int c = 0; c < m_deferredInputEvents.size(); ++c) {
        delete m_deferredInputEvents[c].event;
    }
}

bool QEventDispatcherS60::processEvents ( QEventLoop::ProcessEventsFlags flags )
{
    bool ret = false;

    QT_TRY {
        bool oldNoInputEventsValue = m_noInputEvents;
        if (flags & QEventLoop::ExcludeUserInputEvents) {
            m_noInputEvents = true;
        } else {
            m_noInputEvents = false;
            ret = sendDeferredInputEvents() || ret;
        }

        ret = QEventDispatcherSymbian::processEvents(flags) || ret;

        m_noInputEvents = oldNoInputEventsValue;
    } QT_CATCH (const std::exception& ex) {
#ifndef QT_NO_EXCEPTIONS
        CActiveScheduler::Current()->Error(qt_symbian_exception2Error(ex));
#endif
    }

    return ret;
}

bool QEventDispatcherS60::hasPendingEvents()
{
    return !m_deferredInputEvents.isEmpty() || QEventDispatcherSymbian::hasPendingEvents();
}

void QEventDispatcherS60::saveInputEvent(QSymbianControl *control, QWidget *widget, QInputEvent *event)
{
    DeferredInputEvent inputEvent = {control, widget, event};
    m_deferredInputEvents.append(inputEvent);
    connect(widget, SIGNAL(destroyed(QObject*)), SLOT(removeInputEventsForWidget(QObject*)));
}

bool QEventDispatcherS60::sendDeferredInputEvents()
{
    bool eventsSent = false;
    while (!m_deferredInputEvents.isEmpty()) {
        DeferredInputEvent inputEvent = m_deferredInputEvents.takeFirst();
#ifndef QT_NO_EXCEPTIONS
        try {
#endif
            inputEvent.control->sendInputEvent(inputEvent.widget, inputEvent.event);
#ifndef QT_NO_EXCEPTIONS
        } catch (...) {
            delete inputEvent.event;
            throw;
        }
#endif
        delete inputEvent.event;
        eventsSent = true;
    }

    return eventsSent;
}

void QEventDispatcherS60::removeInputEventsForWidget(QObject *object)
{
    for (int c = 0; c < m_deferredInputEvents.size(); ++c) {
        if (m_deferredInputEvents[c].widget == object) {
            delete m_deferredInputEvents[c].event;
            m_deferredInputEvents.removeAt(c--);
        }
    }
}

QT_END_NAMESPACE
