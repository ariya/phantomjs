/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdirectfbinput.h"
#include "qdirectfbconvenience.h"

#include <QThread>
#include <QDebug>
#include <qpa/qwindowsysteminterface.h>
#include <QMouseEvent>
#include <QEvent>

#include <directfb.h>

QT_BEGIN_NAMESPACE

QDirectFbInput::QDirectFbInput(IDirectFB *dfb, IDirectFBDisplayLayer *dfbLayer)
    : m_dfbInterface(dfb)
    , m_dfbDisplayLayer(dfbLayer)
    , m_shouldStop(false)
{
    DFBResult ok = m_dfbInterface->CreateEventBuffer(m_dfbInterface, m_eventBuffer.outPtr());
    if (ok != DFB_OK)
        DirectFBError("Failed to initialise eventbuffer", ok);
}

void QDirectFbInput::run()
{
    while (!m_shouldStop) {
        if (m_eventBuffer->WaitForEvent(m_eventBuffer.data()) == DFB_OK)
            handleEvents();
    }
}

void QDirectFbInput::stopInputEventLoop()
{
    m_shouldStop = true;
    m_eventBuffer->WakeUp(m_eventBuffer.data());
}

void QDirectFbInput::addWindow(IDirectFBWindow *window, QWindow *platformWindow)
{
    DFBResult res;
    DFBWindowID id;

    res = window->GetID(window, &id);
    if (res != DFB_OK) {
        DirectFBError("QDirectFbInput::addWindow", res);
        return;
    }

    m_tlwMap.insert(id, platformWindow);
    window->AttachEventBuffer(window, m_eventBuffer.data());
}

void QDirectFbInput::removeWindow(IDirectFBWindow *window)
{
    DFBResult res;
    DFBWindowID id;

    res = window->GetID(window, &id);
    if (res != DFB_OK) {
        DirectFBError("QDirectFbInput::removeWindow", res);
        return;
    }

    window->DetachEventBuffer(window, m_eventBuffer.data());
    m_tlwMap.remove(id);
}

void QDirectFbInput::handleEvents()
{
    DFBResult hasEvent = m_eventBuffer->HasEvent(m_eventBuffer.data());
    while(hasEvent == DFB_OK){
        DFBEvent event;
        DFBResult ok = m_eventBuffer->GetEvent(m_eventBuffer.data(), &event);
        if (ok != DFB_OK)
            DirectFBError("Failed to get event",ok);
        if (event.clazz == DFEC_WINDOW) {
            switch (event.window.type) {
            case DWET_BUTTONDOWN:
            case DWET_BUTTONUP:
            case DWET_MOTION:
                handleMouseEvents(event);
                break;
            case DWET_WHEEL:
                handleWheelEvent(event);
                break;
            case DWET_KEYDOWN:
            case DWET_KEYUP:
                handleKeyEvents(event);
                break;
            case DWET_ENTER:
            case DWET_LEAVE:
                handleEnterLeaveEvents(event);
                break;
            case DWET_GOTFOCUS:
                handleGotFocusEvent(event);
                break;
            case DWET_CLOSE:
                handleCloseEvent(event);
                break;
            case DWET_POSITION_SIZE:
                handleGeometryEvent(event);
                break;
            default:
                break;
            }

        }

        hasEvent = m_eventBuffer->HasEvent(m_eventBuffer.data());
    }
}

void QDirectFbInput::handleMouseEvents(const DFBEvent &event)
{
    QPoint p(event.window.x, event.window.y);
    QPoint globalPos(event.window.cx, event.window.cy);
    Qt::MouseButtons buttons = QDirectFbConvenience::mouseButtons(event.window.buttons);

    QDirectFBPointer<IDirectFBDisplayLayer> layer(QDirectFbConvenience::dfbDisplayLayer());
    QDirectFBPointer<IDirectFBWindow> window;
    layer->GetWindow(layer.data(), event.window.window_id, window.outPtr());

    long timestamp = (event.window.timestamp.tv_sec*1000) + (event.window.timestamp.tv_usec/1000);

    QWindow *tlw = m_tlwMap.value(event.window.window_id);
    QWindowSystemInterface::handleMouseEvent(tlw, timestamp, p, globalPos, buttons);
}

void QDirectFbInput::handleWheelEvent(const DFBEvent &event)
{
    QPoint p(event.window.x, event.window.y);
    QPoint globalPos(event.window.cx, event.window.cy);
    long timestamp = (event.window.timestamp.tv_sec*1000) + (event.window.timestamp.tv_usec/1000);
    QWindow *tlw = m_tlwMap.value(event.window.window_id);
    QWindowSystemInterface::handleWheelEvent(tlw, timestamp, p, globalPos,
                                          event.window.step*120,
                                          Qt::Vertical);
}

void QDirectFbInput::handleKeyEvents(const DFBEvent &event)
{
    QEvent::Type type = QDirectFbConvenience::eventType(event.window.type);
    Qt::Key key = QDirectFbConvenience::keyMap()->value(event.window.key_symbol);
    Qt::KeyboardModifiers modifiers = QDirectFbConvenience::keyboardModifiers(event.window.modifiers);

    long timestamp = (event.window.timestamp.tv_sec*1000) + (event.window.timestamp.tv_usec/1000);

    QChar character;
    if (DFB_KEY_TYPE(event.window.key_symbol) == DIKT_UNICODE)
        character = QChar(event.window.key_symbol);
    QWindow *tlw = m_tlwMap.value(event.window.window_id);
    QWindowSystemInterface::handleKeyEvent(tlw, timestamp, type, key, modifiers, character);
}

void QDirectFbInput::handleEnterLeaveEvents(const DFBEvent &event)
{
    QWindow *tlw = m_tlwMap.value(event.window.window_id);
    switch (event.window.type) {
    case DWET_ENTER:
        QWindowSystemInterface::handleEnterEvent(tlw);
        break;
    case DWET_LEAVE:
        QWindowSystemInterface::handleLeaveEvent(tlw);
        break;
    default:
        break;
    }
}

void QDirectFbInput::handleGotFocusEvent(const DFBEvent &event)
{
    QWindow *tlw = m_tlwMap.value(event.window.window_id);
    QWindowSystemInterface::handleWindowActivated(tlw);
}

void QDirectFbInput::handleCloseEvent(const DFBEvent &event)
{
    QWindow *tlw = m_tlwMap.value(event.window.window_id);
    QWindowSystemInterface::handleCloseEvent(tlw);
}

void QDirectFbInput::handleGeometryEvent(const DFBEvent &event)
{
    QWindow *tlw = m_tlwMap.value(event.window.window_id);
    QRect rect(event.window.x, event.window.y, event.window.w, event.window.h);
    QWindowSystemInterface::handleGeometryChange(tlw, rect);
}

QT_END_NAMESPACE
