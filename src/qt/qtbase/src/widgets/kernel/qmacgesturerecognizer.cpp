/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qmacgesturerecognizer_p.h"
#include "qgesture.h"
#include "qgesture_p.h"
#include "qevent.h"
#include "qwidget.h"
#include "qdebug.h"

#ifndef QT_NO_GESTURES

QT_BEGIN_NAMESPACE

QMacSwipeGestureRecognizer::QMacSwipeGestureRecognizer()
{
}

QGesture *QMacSwipeGestureRecognizer::create(QObject * /*target*/)
{
    return new QSwipeGesture;
}

QGestureRecognizer::Result
QMacSwipeGestureRecognizer::recognize(QGesture *gesture, QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::NativeGesture && obj->isWidgetType()) {
        QNativeGestureEvent *ev = static_cast<QNativeGestureEvent*>(event);
        switch (ev->gestureType()) {
            case Qt::SwipeNativeGesture: {
                QSwipeGesture *g = static_cast<QSwipeGesture *>(gesture);
                g->setSwipeAngle(ev->value());
                g->setHotSpot(ev->screenPos());
                return QGestureRecognizer::FinishGesture | QGestureRecognizer::ConsumeEventHint;
                break; }
            default:
                break;
        }
    }

    return QGestureRecognizer::Ignore;
}

void QMacSwipeGestureRecognizer::reset(QGesture *gesture)
{
    QSwipeGesture *g = static_cast<QSwipeGesture *>(gesture);
    g->setSwipeAngle(0);
    QGestureRecognizer::reset(gesture);
}

////////////////////////////////////////////////////////////////////////

QMacPinchGestureRecognizer::QMacPinchGestureRecognizer()
{
}

QGesture *QMacPinchGestureRecognizer::create(QObject * /*target*/)
{
    return new QPinchGesture;
}

QGestureRecognizer::Result
QMacPinchGestureRecognizer::recognize(QGesture *gesture, QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::NativeGesture && obj->isWidgetType()) {
        QPinchGesture *g = static_cast<QPinchGesture *>(gesture);
        QNativeGestureEvent *ev = static_cast<QNativeGestureEvent*>(event);
        switch (ev->gestureType()) {
        case Qt::BeginNativeGesture:
            reset(gesture);
            g->setStartCenterPoint(static_cast<QWidget*>(obj)->mapFromGlobal(ev->screenPos().toPoint()));
            g->setCenterPoint(g->startCenterPoint());
            g->setChangeFlags(QPinchGesture::CenterPointChanged);
            g->setTotalChangeFlags(g->totalChangeFlags() | g->changeFlags());
            g->setHotSpot(ev->screenPos());
            return QGestureRecognizer::MayBeGesture | QGestureRecognizer::ConsumeEventHint;
        case Qt::RotateNativeGesture:
            g->setLastScaleFactor(g->scaleFactor());
            g->setLastRotationAngle(g->rotationAngle());
            g->setRotationAngle(g->rotationAngle() + ev->value());
            g->setChangeFlags(QPinchGesture::RotationAngleChanged);
            g->setTotalChangeFlags(g->totalChangeFlags() | g->changeFlags());
            g->setHotSpot(ev->screenPos());
            return QGestureRecognizer::TriggerGesture | QGestureRecognizer::ConsumeEventHint;
        case Qt::ZoomNativeGesture:
            g->setLastScaleFactor(g->scaleFactor());
            g->setLastRotationAngle(g->rotationAngle());
            g->setScaleFactor(g->scaleFactor() * (1 + ev->value()));
            g->setChangeFlags(QPinchGesture::ScaleFactorChanged);
            g->setTotalChangeFlags(g->totalChangeFlags() | g->changeFlags());
            g->setHotSpot(ev->screenPos());
            return QGestureRecognizer::TriggerGesture | QGestureRecognizer::ConsumeEventHint;
        case Qt::SmartZoomNativeGesture:
            g->setLastScaleFactor(g->scaleFactor());
            g->setLastRotationAngle(g->rotationAngle());
            g->setScaleFactor(ev->value() ? 1.7f : 1.0f);
            g->setChangeFlags(QPinchGesture::ScaleFactorChanged);
            g->setTotalChangeFlags(g->totalChangeFlags() | g->changeFlags());
            g->setHotSpot(ev->screenPos());
            return QGestureRecognizer::TriggerGesture | QGestureRecognizer::ConsumeEventHint;
        case Qt::EndNativeGesture:
            return QGestureRecognizer::FinishGesture | QGestureRecognizer::ConsumeEventHint;
        default:
            break;
        }
    }

    return QGestureRecognizer::Ignore;
}

void QMacPinchGestureRecognizer::reset(QGesture *gesture)
{
    QPinchGesture *g = static_cast<QPinchGesture *>(gesture);
    g->setChangeFlags(0);
    g->setTotalChangeFlags(0);
    g->setScaleFactor(1.0f);
    g->setTotalScaleFactor(1.0f);
    g->setLastScaleFactor(1.0f);
    g->setRotationAngle(0.0f);
    g->setTotalRotationAngle(0.0f);
    g->setLastRotationAngle(0.0f);
    g->setCenterPoint(QPointF());
    g->setStartCenterPoint(QPointF());
    g->setLastCenterPoint(QPointF());
    QGestureRecognizer::reset(gesture);
}

////////////////////////////////////////////////////////////////////////

QMacPanGestureRecognizer::QMacPanGestureRecognizer() : _panCanceled(true)
{
}

QGesture *QMacPanGestureRecognizer::create(QObject *target)
{
    if (!target)
        return new QPanGesture;

    if (QWidget *w = qobject_cast<QWidget *>(target)) {
        w->setAttribute(Qt::WA_AcceptTouchEvents);
        w->setAttribute(Qt::WA_TouchPadAcceptSingleTouchEvents);
        return new QPanGesture;
    }
    return 0;
}

QGestureRecognizer::Result
QMacPanGestureRecognizer::recognize(QGesture *gesture, QObject *target, QEvent *event)
{
    const int panBeginDelay = 300;
    const int panBeginRadius = 3;

    QPanGesture *g = static_cast<QPanGesture *>(gesture);

    switch (event->type()) {
    case QEvent::TouchBegin: {
        const QTouchEvent *ev = static_cast<const QTouchEvent*>(event);
        if (ev->touchPoints().size() == 1) {
            reset(gesture);
            _startPos = QCursor::pos();
            _panTimer.start(panBeginDelay, target);
            _panCanceled = false;
            return QGestureRecognizer::MayBeGesture;
        }
        break;}
    case QEvent::TouchEnd: {
        if (_panCanceled)
            break;

        const QTouchEvent *ev = static_cast<const QTouchEvent*>(event);
        if (ev->touchPoints().size() == 1)
            return QGestureRecognizer::FinishGesture;
        break;}
    case QEvent::TouchUpdate: {
        if (_panCanceled)
            break;

        const QTouchEvent *ev = static_cast<const QTouchEvent*>(event);
        if (ev->touchPoints().size() == 1) {
            if (_panTimer.isActive()) {
                // INVARIANT: Still in maybeGesture. Check if the user
                // moved his finger so much that it makes sense to cancel the pan:
                const QPointF p = QCursor::pos();
                if ((p - _startPos).manhattanLength() > panBeginRadius) {
                    _panCanceled = true;
                    _panTimer.stop();
                    return QGestureRecognizer::CancelGesture;
                }
            } else {
                const QPointF p = QCursor::pos();
                const QPointF posOffset = p - _startPos;
                g->setLastOffset(g->offset());
                g->setOffset(QPointF(posOffset.x(), posOffset.y()));
                g->setHotSpot(_startPos);
                return QGestureRecognizer::TriggerGesture;
            }
        } else if (_panTimer.isActive()) {
            // I only want to cancel the pan if the user is pressing
            // more than one finger, and the pan hasn't started yet:
            _panCanceled = true;
            _panTimer.stop();
            return QGestureRecognizer::CancelGesture;
        }
        break;}
    case QEvent::Timer: {
        QTimerEvent *ev = static_cast<QTimerEvent *>(event);
        if (ev->timerId() == _panTimer.timerId()) {
            _panTimer.stop();
            if (_panCanceled)
                break;
            // Begin new pan session!
            _startPos = QCursor::pos();
            g->setHotSpot(_startPos);
            return QGestureRecognizer::TriggerGesture | QGestureRecognizer::ConsumeEventHint;
        }
        break; }
    default:
        break;
    }

    return QGestureRecognizer::Ignore;
}

void QMacPanGestureRecognizer::reset(QGesture *gesture)
{
    QPanGesture *g = static_cast<QPanGesture *>(gesture);
    _startPos = QPointF();
    _panCanceled = true;
    g->setOffset(QPointF(0, 0));
    g->setLastOffset(QPointF(0, 0));
    g->setAcceleration(qreal(1));
    QGestureRecognizer::reset(gesture);
}

QT_END_NAMESPACE

#endif // QT_NO_GESTURES
