/*
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef EventSenderQt_h
#define EventSenderQt_h


#include "DumpRenderTreeQt.h"

#include <QApplication>
#include <QBasicTimer>
#include <QEvent>
#include <QEventLoop>
#include <QGesture>
#include <QMimeData>
#include <QMouseEvent>
#include <QObject>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QTouchEvent>
#include <Qt>

#include <qwebpage.h>
#include <qwebframe.h>


class EventSender : public QObject {
    Q_OBJECT
public:
    EventSender(QWebPage* parent);
    virtual bool eventFilter(QObject* watched, QEvent* event);
    void resetClickCount() { m_clickCount = 0; }

public Q_SLOTS:
    void mouseDown(int button = 0, const QStringList& modifiers = QStringList());
    void mouseUp(int button = 0);
    void mouseMoveTo(int x, int y);
#ifndef QT_NO_WHEELEVENT
    void mouseScrollBy(int x, int y);
    void continuousMouseScrollBy(int x, int y);
#endif
    void leapForward(int ms);
    void keyDown(const QString& string, const QStringList& modifiers = QStringList(), unsigned int location = 0);
    void clearKillRing() {}
    QStringList contextClick();
    void scheduleAsynchronousClick();
    void addTouchPoint(int x, int y);
    void updateTouchPoint(int index, int x, int y);
    void setTouchModifier(const QString &modifier, bool enable);
    void setTouchPointRadius(int radiusX, int radiusY);
    void touchStart();
    void touchMove();
    void touchEnd();
    void touchCancel();
    void zoomPageIn();
    void zoomPageOut();
    void textZoomIn();
    void textZoomOut();
    void scalePageBy(float scaleFactor, float x, float y);
    void clearTouchPoints();
    void releaseTouchPoint(int index);
    void cancelTouchPoint(int index);
#ifndef QT_NO_GESTURES
    void gestureTap(int x, int y);
    void gestureLongPress(int x, int y);
#endif
    void beginDragWithFiles(const QStringList& files);

protected:
    void timerEvent(QTimerEvent*);

private:
    bool isGraphicsBased() const { return qobject_cast<WebViewGraphicsBased*>(m_page->view()); }
    QGraphicsSceneMouseEvent* createGraphicsSceneMouseEvent(QEvent::Type, const QPoint& pos, const QPoint& screenPos, Qt::MouseButton, Qt::MouseButtons, Qt::KeyboardModifiers);
    QGraphicsSceneWheelEvent* createGraphicsSceneWheelEvent(QEvent::Type, const QPoint& pos, const QPoint& screenPos, int delta, Qt::KeyboardModifiers, Qt::Orientation);
    void sendEvent(QObject* receiver, QEvent* event);
    void postEvent(QObject* receiver, QEvent* event);

private:
    void sendTouchEvent(QEvent::Type);
    void sendOrQueueEvent(QEvent*);
    void replaySavedEvents(bool flush);
    QPoint m_mousePos;
    QPoint m_clickPos;
    QPoint m_touchPointRadius;
    Qt::MouseButtons m_mouseButtons;
    QWebPage* m_page;
    int m_clickCount;
    int m_currentButton;
    bool m_mouseButtonPressed;
    bool m_drag;
    QMimeData m_currentDragData;
    Qt::DropActions m_currentDragActionsAllowed;
    QEventLoop* m_eventLoop;
    QWebFrame* frameUnderMouse() const;
    QBasicTimer m_clickTimer;
    QList<QTouchEvent::TouchPoint> m_touchPoints;
    Qt::KeyboardModifiers m_touchModifiers;
#ifndef QT_NO_GESTURES
    QTapGesture m_tapGesture;
    QTapAndHoldGesture m_tapAndHoldGesture;
    QList<QGesture*> m_gestures;
#endif
    bool m_touchActive;
};
#endif //  EventSenderQt_h
