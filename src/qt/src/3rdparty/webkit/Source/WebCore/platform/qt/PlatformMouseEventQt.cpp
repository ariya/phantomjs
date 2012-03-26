/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PlatformMouseEvent.h"

#include <QGraphicsSceneMouseEvent>
#include <QMouseEvent>
#include <wtf/CurrentTime.h>

namespace WebCore {

#if !defined(QT_NO_GRAPHICSVIEW)
PlatformMouseEvent::PlatformMouseEvent(QGraphicsSceneMouseEvent* event, int clickCount)
{
    m_timestamp = WTF::currentTime();

    switch (event->type()) {
    case QEvent::GraphicsSceneMouseDoubleClick:
    case QEvent::GraphicsSceneMousePress:
        m_eventType = MouseEventPressed;
        break;
    case QEvent::GraphicsSceneMouseRelease:
        m_eventType = MouseEventReleased;
        break;
    case QEvent::GraphicsSceneMouseMove:
    default:
        m_eventType = MouseEventMoved;
    }

    m_position = IntPoint(event->pos().toPoint());
    m_globalPosition = IntPoint(event->screenPos());

    if (event->button() == Qt::LeftButton || (event->buttons() & Qt::LeftButton))
        m_button = LeftButton;
    else if (event->button() == Qt::RightButton || (event->buttons() & Qt::RightButton))
        m_button = RightButton;
    else if (event->button() == Qt::MidButton || (event->buttons() & Qt::MidButton))
        m_button = MiddleButton;
    else
        m_button = NoButton;

    m_clickCount = clickCount;
    m_shiftKey =  (event->modifiers() & Qt::ShiftModifier);
    m_ctrlKey = (event->modifiers() & Qt::ControlModifier);
    m_altKey =  (event->modifiers() & Qt::AltModifier);
    m_metaKey = (event->modifiers() & Qt::MetaModifier);
}
#endif // QT_NO_GRAPHICSVIEW

PlatformMouseEvent::PlatformMouseEvent(QInputEvent* event, int clickCount)
{
    m_timestamp = WTF::currentTime();

    QMouseEvent* me = 0;

    switch (event->type()) {
    case QEvent::MouseMove:
        m_eventType = MouseEventMoved;
        me = static_cast<QMouseEvent *>(event);
        break;
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
        m_eventType = MouseEventPressed;
        me = static_cast<QMouseEvent *>(event);
        break;
    case QEvent::MouseButtonRelease:
        m_eventType = MouseEventReleased;
        me = static_cast<QMouseEvent *>(event);
        break;
#ifndef QT_NO_CONTEXTMENU
    case QEvent::ContextMenu: {
        m_eventType = MouseEventPressed;
        QContextMenuEvent* ce = static_cast<QContextMenuEvent*>(event);
        m_position = IntPoint(ce->pos());
        m_globalPosition = IntPoint(ce->globalPos());
        m_button = RightButton;
        break;
    }
#endif // QT_NO_CONTEXTMENU
    default:
        m_eventType = MouseEventMoved;
    }

    if (me) {
        m_position = IntPoint(me->pos());
        m_globalPosition = IntPoint(me->globalPos());

        if (me->button() == Qt::LeftButton || (me->buttons() & Qt::LeftButton))
            m_button = LeftButton;
        else if (me->button() == Qt::RightButton || (me->buttons() & Qt::RightButton))
            m_button = RightButton;
        else if (me->button() == Qt::MidButton || (me->buttons() & Qt::MidButton))
            m_button = MiddleButton;
        else
            m_button = NoButton;
    }

    m_clickCount = clickCount;
    m_shiftKey =  (event->modifiers() & Qt::ShiftModifier);
    m_ctrlKey = (event->modifiers() & Qt::ControlModifier);
    m_altKey =  (event->modifiers() & Qt::AltModifier);
    m_metaKey = (event->modifiers() & Qt::MetaModifier);
}

}

// vim: ts=4 sw=4 et
