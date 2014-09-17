/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsymbianevent.h"
#include <qdebug.h>

#include <w32std.h>

QT_BEGIN_NAMESPACE

/*!
    \class QSymbianEvent
    \brief The QSymbianEvent class contains a Symbian event of any type.
    \since 4.6

    The class is used as a generic container type for all types of Symbian
    events.

    \note This class is only available on Symbian.

    \sa QApplication::symbianEventFilter()
*/

/*!
    \enum QSymbianEvent::Type

    \value InvalidEvent        The event is not valid.
    \value WindowServerEvent   Indicates an event of type \c TWsEvent.
    \value CommandEvent        Indicates that the event is a Symbian command.
    \value ResourceChangeEvent Indicates that the event is a Symbian resource change type.
*/

/*!
    \fn QSymbianEvent::type() const

    Returns the event type contained in the QSymbianEvent instance.
*/

/*!
    \fn QSymbianEvent::isValid() const

    Returns whether this QSymbianEvent instance contains a valid event.
*/

/*!
    Constructs a QSymbianEvent containing the given window server event
    \a windowServerEvent.
*/
QSymbianEvent::QSymbianEvent(const TWsEvent *windowServerEvent)
    : m_type(WindowServerEvent)
    , m_eventPtr(windowServerEvent)
{
}

/*!
    Constructs a QSymbianEvent containing the given event value
    \a value. The type of event is controlled by the \a eventType parameter.
*/
QSymbianEvent::QSymbianEvent(QSymbianEvent::Type eventType, int value)
{
    switch (eventType) {
    case CommandEvent:
    case ResourceChangeEvent:
        m_type = eventType;
        m_eventValue = value;
        break;
    default:
        m_type = InvalidEvent;
        m_eventValue = 0;
        break;
    }
}

/*!
    Destroys the QSymbianEvent.
*/
QSymbianEvent::~QSymbianEvent()
{
}

/*!
    Returns the window server event contained in the class instance, or 0 if the event type
    is not \c WindowServerEvent.
*/
const TWsEvent *QSymbianEvent::windowServerEvent() const
{
    return (m_type == WindowServerEvent) ? static_cast<const TWsEvent *>(m_eventPtr) : 0;
}

/*!
    Returns the command contained in the class instance, or 0 if the event type
    is not \c CommandEvent.
*/
int QSymbianEvent::command() const
{
    return (m_type == CommandEvent) ? m_eventValue : 0;
}

/*!
    Returns the resource change type contained in the class instance, or 0 if the event type
    is not \c ResourceChangeEvent.
*/
int QSymbianEvent::resourceChangeType() const
{
    return (m_type == ResourceChangeEvent) ? m_eventValue : 0;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QSymbianEvent *o)
{
    if (!o) {
        dbg << "QSymbianEvent(0x0)";
        return dbg;
    }
    dbg.nospace() << "QSymbianEvent(";
    switch (o->type()) {
    case QSymbianEvent::InvalidEvent:
        dbg << "InvalidEvent";
        break;
    case QSymbianEvent::WindowServerEvent:
        dbg << "WindowServerEvent, Type = " << o->windowServerEvent()->Type();
        break;
    case QSymbianEvent::CommandEvent:
        dbg << "CommandEvent, command = " << o->command();
        break;
    case QSymbianEvent::ResourceChangeEvent:
        dbg << "ResourceChangeEvent, resourceChangeType = " << o->resourceChangeType();
        break;
    default:
        dbg << "Unknown event type";
        break;
    }
    dbg << ")";
    return dbg.space();
}
#endif

QT_END_NAMESPACE
