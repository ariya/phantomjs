/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "WheelEvent.h"

#include "PlatformMouseEvent.h"
#include "PlatformWheelEvent.h"
#include "Scrollbar.h"

#include <QGraphicsSceneWheelEvent>
#include <QWheelEvent>
#include <qapplication.h>

namespace WebCore {

void PlatformWheelEvent::applyDelta(int delta, Qt::Orientation orientation)
{
    // A delta that is not mod 120 indicates a device that is sending
    // fine-resolution scroll events, so use the delta as number of wheel ticks
    // and number of pixels to scroll.See also webkit.org/b/29601
    bool fullTick = !(delta % 120);

    if (orientation == Qt::Horizontal) {
        m_deltaX = (fullTick) ? delta / 120.0f : delta;
        m_deltaY = 0;
    } else {
        m_deltaX = 0;
        m_deltaY = (fullTick) ? delta / 120.0f : delta;
    }

    m_wheelTicksX = m_deltaX;
    m_wheelTicksY = m_deltaY;

#ifndef QT_NO_WHEELEVENT
    // Use the same single scroll step as QTextEdit
    // (in QTextEditPrivate::init [h,v]bar->setSingleStep)
    static const float cDefaultQtScrollStep = 20.f;
    m_deltaX *= (fullTick) ? QApplication::wheelScrollLines() * cDefaultQtScrollStep : 1;
    m_deltaY *= (fullTick) ? QApplication::wheelScrollLines() * cDefaultQtScrollStep : 1;
#endif
}

PlatformWheelEvent::PlatformWheelEvent(QGraphicsSceneWheelEvent* e)
#ifndef QT_NO_WHEELEVENT
    : m_position(e->pos().toPoint())
    , m_globalPosition(e->screenPos())
    , m_granularity(ScrollByPixelWheelEvent)
    , m_isAccepted(false)
    , m_shiftKey(e->modifiers() & Qt::ShiftModifier)
    , m_ctrlKey(e->modifiers() & Qt::ControlModifier)
    , m_altKey(e->modifiers() & Qt::AltModifier)
    , m_metaKey(e->modifiers() & Qt::MetaModifier)
#endif
{
#ifndef QT_NO_WHEELEVENT
    applyDelta(e->delta(), e->orientation());
#else
    Q_UNUSED(e);
#endif
}

PlatformWheelEvent::PlatformWheelEvent(QWheelEvent* e)
#ifndef QT_NO_WHEELEVENT
    : m_position(e->pos())
    , m_globalPosition(e->globalPos())
    , m_granularity(ScrollByPixelWheelEvent)
    , m_isAccepted(false)
    , m_shiftKey(e->modifiers() & Qt::ShiftModifier)
    , m_ctrlKey(e->modifiers() & Qt::ControlModifier)
    , m_altKey(e->modifiers() & Qt::AltModifier)
    , m_metaKey(e->modifiers() & Qt::MetaModifier)
#endif
{
#ifndef QT_NO_WHEELEVENT
    applyDelta(e->delta(), e->orientation());
#else
    Q_UNUSED(e);
#endif
}

} // namespace WebCore
