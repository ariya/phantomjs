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

#include "qstandardgestures_p.h"
#include "qgesture.h"
#include "qgesture_p.h"
#include "qevent.h"
#include "qwidget.h"
#include "qabstractscrollarea.h"
#include <qgraphicssceneevent.h>
#include "qdebug.h"

#ifndef QT_NO_GESTURES

QT_BEGIN_NAMESPACE

QPanGestureRecognizer::QPanGestureRecognizer()
{
}

QGesture *QPanGestureRecognizer::create(QObject *target)
{
    if (target && target->isWidgetType()) {
#if defined(Q_OS_WIN) && !defined(QT_NO_NATIVE_GESTURES)
        // for scroll areas on Windows we want to use native gestures instead
        if (!qobject_cast<QAbstractScrollArea *>(target->parent()))
            static_cast<QWidget *>(target)->setAttribute(Qt::WA_AcceptTouchEvents);
#else
        static_cast<QWidget *>(target)->setAttribute(Qt::WA_AcceptTouchEvents);
#endif
    }
    return new QPanGesture;
}

QGestureRecognizer::Result QPanGestureRecognizer::recognize(QGesture *state,
                                                            QObject *,
                                                            QEvent *event)
{
    QPanGesture *q = static_cast<QPanGesture *>(state);
    QPanGesturePrivate *d = q->d_func();

    const QTouchEvent *ev = static_cast<const QTouchEvent *>(event);

    QGestureRecognizer::Result result;
    switch (event->type()) {
    case QEvent::TouchBegin: {
        result = QGestureRecognizer::MayBeGesture;
        QTouchEvent::TouchPoint p = ev->touchPoints().at(0);
        d->lastOffset = d->offset = QPointF();
        break;
    }
    case QEvent::TouchEnd: {
        if (q->state() != Qt::NoGesture) {
            if (ev->touchPoints().size() == 2) {
                QTouchEvent::TouchPoint p1 = ev->touchPoints().at(0);
                QTouchEvent::TouchPoint p2 = ev->touchPoints().at(1);
                d->lastOffset = d->offset;
                d->offset =
                        QPointF(p1.pos().x() - p1.startPos().x() + p2.pos().x() - p2.startPos().x(),
                              p1.pos().y() - p1.startPos().y() + p2.pos().y() - p2.startPos().y()) / 2;
            }
            result = QGestureRecognizer::FinishGesture;
        } else {
            result = QGestureRecognizer::CancelGesture;
        }
        break;
    }
    case QEvent::TouchUpdate: {
        if (ev->touchPoints().size() >= 2) {
            QTouchEvent::TouchPoint p1 = ev->touchPoints().at(0);
            QTouchEvent::TouchPoint p2 = ev->touchPoints().at(1);
            d->lastOffset = d->offset;
            d->offset =
                    QPointF(p1.pos().x() - p1.startPos().x() + p2.pos().x() - p2.startPos().x(),
                          p1.pos().y() - p1.startPos().y() + p2.pos().y() - p2.startPos().y()) / 2;
            if (d->offset.x() > 10  || d->offset.y() > 10 ||
                d->offset.x() < -10 || d->offset.y() < -10) {
                q->setHotSpot(p1.startScreenPos());
                result = QGestureRecognizer::TriggerGesture;
            } else {
                result = QGestureRecognizer::MayBeGesture;
            }
        }
        break;
    }
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
        result = QGestureRecognizer::Ignore;
        break;
    default:
        result = QGestureRecognizer::Ignore;
        break;
    }
    return result;
}

void QPanGestureRecognizer::reset(QGesture *state)
{
    QPanGesture *pan = static_cast<QPanGesture*>(state);
    QPanGesturePrivate *d = pan->d_func();

    d->lastOffset = d->offset = QPointF();
    d->acceleration = 0;

    QGestureRecognizer::reset(state);
}


//
// QPinchGestureRecognizer
//

QPinchGestureRecognizer::QPinchGestureRecognizer()
{
}

QGesture *QPinchGestureRecognizer::create(QObject *target)
{
    if (target && target->isWidgetType()) {
        static_cast<QWidget *>(target)->setAttribute(Qt::WA_AcceptTouchEvents);
    }
    return new QPinchGesture;
}

QGestureRecognizer::Result QPinchGestureRecognizer::recognize(QGesture *state,
                                                              QObject *,
                                                              QEvent *event)
{
    QPinchGesture *q = static_cast<QPinchGesture *>(state);
    QPinchGesturePrivate *d = q->d_func();

    const QTouchEvent *ev = static_cast<const QTouchEvent *>(event);

    QGestureRecognizer::Result result;

    switch (event->type()) {
    case QEvent::TouchBegin: {
        result = QGestureRecognizer::MayBeGesture;
        break;
    }
    case QEvent::TouchEnd: {
        if (q->state() != Qt::NoGesture) {
            result = QGestureRecognizer::FinishGesture;
        } else {
            result = QGestureRecognizer::CancelGesture;
        }
        break;
    }
    case QEvent::TouchUpdate: {
        d->changeFlags = 0;
        if (ev->touchPoints().size() == 2) {
            QTouchEvent::TouchPoint p1 = ev->touchPoints().at(0);
            QTouchEvent::TouchPoint p2 = ev->touchPoints().at(1);

            d->hotSpot = p1.screenPos();
            d->isHotSpotSet = true;

            QPointF centerPoint = (p1.screenPos() + p2.screenPos()) / 2.0;
            if (d->isNewSequence) {
                d->startPosition[0] = p1.screenPos();
                d->startPosition[1] = p2.screenPos();
                d->lastCenterPoint = centerPoint;
            } else {
                d->lastCenterPoint = d->centerPoint;
            }
            d->centerPoint = centerPoint;

            d->changeFlags |= QPinchGesture::CenterPointChanged;

            if (d->isNewSequence) {
                d->scaleFactor = 1.0;
                d->lastScaleFactor = 1.0;
            } else {
                d->lastScaleFactor = d->scaleFactor;
                QLineF line(p1.screenPos(), p2.screenPos());
                QLineF lastLine(p1.lastScreenPos(),  p2.lastScreenPos());
                d->scaleFactor = line.length() / lastLine.length();
            }
            d->totalScaleFactor = d->totalScaleFactor * d->scaleFactor;
            d->changeFlags |= QPinchGesture::ScaleFactorChanged;

            qreal angle = QLineF(p1.screenPos(), p2.screenPos()).angle();
            if (angle > 180)
                angle -= 360;
            qreal startAngle = QLineF(p1.startScreenPos(), p2.startScreenPos()).angle();
            if (startAngle > 180)
                startAngle -= 360;
            const qreal rotationAngle = startAngle - angle;
            if (d->isNewSequence)
                d->lastRotationAngle = 0.0;
            else
                d->lastRotationAngle = d->rotationAngle;
            d->rotationAngle = rotationAngle;
            d->totalRotationAngle += d->rotationAngle - d->lastRotationAngle;
            d->changeFlags |= QPinchGesture::RotationAngleChanged;

            d->totalChangeFlags |= d->changeFlags;
            d->isNewSequence = false;
            result = QGestureRecognizer::TriggerGesture;
        } else {
            d->isNewSequence = true;
            if (q->state() == Qt::NoGesture)
                result = QGestureRecognizer::Ignore;
            else
                result = QGestureRecognizer::FinishGesture;
        }
        break;
    }
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
        result = QGestureRecognizer::Ignore;
        break;
    default:
        result = QGestureRecognizer::Ignore;
        break;
    }
    return result;
}

void QPinchGestureRecognizer::reset(QGesture *state)
{
    QPinchGesture *pinch = static_cast<QPinchGesture *>(state);
    QPinchGesturePrivate *d = pinch->d_func();

    d->totalChangeFlags = d->changeFlags = 0;

    d->startCenterPoint = d->lastCenterPoint = d->centerPoint = QPointF();
    d->totalScaleFactor = d->lastScaleFactor = d->scaleFactor = 1;
    d->totalRotationAngle = d->lastRotationAngle = d->rotationAngle = 0;

    d->isNewSequence = true;
    d->startPosition[0] = d->startPosition[1] = QPointF();

    QGestureRecognizer::reset(state);
}

//
// QSwipeGestureRecognizer
//

QSwipeGestureRecognizer::QSwipeGestureRecognizer()
{
}

QGesture *QSwipeGestureRecognizer::create(QObject *target)
{
    if (target && target->isWidgetType()) {
        static_cast<QWidget *>(target)->setAttribute(Qt::WA_AcceptTouchEvents);
    }
    return new QSwipeGesture;
}

QGestureRecognizer::Result QSwipeGestureRecognizer::recognize(QGesture *state,
                                                              QObject *,
                                                              QEvent *event)
{
    QSwipeGesture *q = static_cast<QSwipeGesture *>(state);
    QSwipeGesturePrivate *d = q->d_func();

    const QTouchEvent *ev = static_cast<const QTouchEvent *>(event);

    QGestureRecognizer::Result result;

    switch (event->type()) {
    case QEvent::TouchBegin: {
        d->velocityValue = 1;
        d->time.start();
        d->started = true;
        result = QGestureRecognizer::MayBeGesture;
        break;
    }
    case QEvent::TouchEnd: {
        if (q->state() != Qt::NoGesture) {
            result = QGestureRecognizer::FinishGesture;
        } else {
            result = QGestureRecognizer::CancelGesture;
        }
        break;
    }
    case QEvent::TouchUpdate: {
        if (!d->started)
            result = QGestureRecognizer::CancelGesture;
        else if (ev->touchPoints().size() == 3) {
            QTouchEvent::TouchPoint p1 = ev->touchPoints().at(0);
            QTouchEvent::TouchPoint p2 = ev->touchPoints().at(1);
            QTouchEvent::TouchPoint p3 = ev->touchPoints().at(2);

            if (d->lastPositions[0].isNull()) {
                d->lastPositions[0] = p1.startScreenPos().toPoint();
                d->lastPositions[1] = p2.startScreenPos().toPoint();
                d->lastPositions[2] = p3.startScreenPos().toPoint();
            }
            d->hotSpot = p1.screenPos();
            d->isHotSpotSet = true;

            int xDistance = (p1.screenPos().x() - d->lastPositions[0].x() +
                             p2.screenPos().x() - d->lastPositions[1].x() +
                             p3.screenPos().x() - d->lastPositions[2].x()) / 3;
            int yDistance = (p1.screenPos().y() - d->lastPositions[0].y() +
                             p2.screenPos().y() - d->lastPositions[1].y() +
                             p3.screenPos().y() - d->lastPositions[2].y()) / 3;

            const int distance = xDistance >= yDistance ? xDistance : yDistance;
            int elapsedTime = d->time.restart();
            if (!elapsedTime)
                elapsedTime = 1;
            d->velocityValue = 0.9 * d->velocityValue + distance / elapsedTime;
            d->swipeAngle = QLineF(p1.startScreenPos(), p1.screenPos()).angle();

            static const int MoveThreshold = 50;
            if (xDistance > MoveThreshold || yDistance > MoveThreshold) {
                // measure the distance to check if the direction changed
                d->lastPositions[0] = p1.screenPos().toPoint();
                d->lastPositions[1] = p2.screenPos().toPoint();
                d->lastPositions[2] = p3.screenPos().toPoint();
                QSwipeGesture::SwipeDirection horizontal =
                        xDistance > 0 ? QSwipeGesture::Right : QSwipeGesture::Left;
                QSwipeGesture::SwipeDirection vertical =
                        yDistance > 0 ? QSwipeGesture::Down : QSwipeGesture::Up;
                if (d->verticalDirection == QSwipeGesture::NoDirection)
                    d->verticalDirection = vertical;
                if (d->horizontalDirection == QSwipeGesture::NoDirection)
                    d->horizontalDirection = horizontal;
                if (d->verticalDirection != vertical || d->horizontalDirection != horizontal) {
                    // the user has changed the direction!
                    result = QGestureRecognizer::CancelGesture;
                }
                result = QGestureRecognizer::TriggerGesture;
            } else {
                if (q->state() != Qt::NoGesture)
                    result = QGestureRecognizer::TriggerGesture;
                else
                    result = QGestureRecognizer::MayBeGesture;
            }
        } else if (ev->touchPoints().size() > 3) {
            result = QGestureRecognizer::CancelGesture;
        } else { // less than 3 touch points
            if (d->started && (ev->touchPointStates() & Qt::TouchPointPressed))
                result = QGestureRecognizer::CancelGesture;
            else if (d->started)
                result = QGestureRecognizer::Ignore;
            else
                result = QGestureRecognizer::MayBeGesture;
        }
        break;
    }
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
        result = QGestureRecognizer::Ignore;
        break;
    default:
        result = QGestureRecognizer::Ignore;
        break;
    }
    return result;
}

void QSwipeGestureRecognizer::reset(QGesture *state)
{
    QSwipeGesture *q = static_cast<QSwipeGesture *>(state);
    QSwipeGesturePrivate *d = q->d_func();

    d->verticalDirection = d->horizontalDirection = QSwipeGesture::NoDirection;
    d->swipeAngle = 0;

    d->lastPositions[0] = d->lastPositions[1] = d->lastPositions[2] = QPoint();
    d->started = false;
    d->velocityValue = 0;
    d->time.invalidate();

    QGestureRecognizer::reset(state);
}

//
// QTapGestureRecognizer
//

QTapGestureRecognizer::QTapGestureRecognizer()
{
}

QGesture *QTapGestureRecognizer::create(QObject *target)
{
    if (target && target->isWidgetType()) {
        static_cast<QWidget *>(target)->setAttribute(Qt::WA_AcceptTouchEvents);
    }
    return new QTapGesture;
}

QGestureRecognizer::Result QTapGestureRecognizer::recognize(QGesture *state,
                                                            QObject *,
                                                            QEvent *event)
{
    QTapGesture *q = static_cast<QTapGesture *>(state);
    QTapGesturePrivate *d = q->d_func();

    const QTouchEvent *ev = static_cast<const QTouchEvent *>(event);

    QGestureRecognizer::Result result = QGestureRecognizer::CancelGesture;

    switch (event->type()) {
    case QEvent::TouchBegin: {
        d->position = ev->touchPoints().at(0).pos();
        q->setHotSpot(ev->touchPoints().at(0).screenPos());
        result = QGestureRecognizer::TriggerGesture;
        break;
    }
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd: {
        if (q->state() != Qt::NoGesture && ev->touchPoints().size() == 1) {
            QTouchEvent::TouchPoint p = ev->touchPoints().at(0);
            QPoint delta = p.pos().toPoint() - p.startPos().toPoint();
            enum { TapRadius = 40 };
            if (delta.manhattanLength() <= TapRadius) {
                if (event->type() == QEvent::TouchEnd)
                    result = QGestureRecognizer::FinishGesture;
                else
                    result = QGestureRecognizer::TriggerGesture;
            }
        }
        break;
    }
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
        result = QGestureRecognizer::Ignore;
        break;
    default:
        result = QGestureRecognizer::Ignore;
        break;
    }
    return result;
}

void QTapGestureRecognizer::reset(QGesture *state)
{
    QTapGesture *q = static_cast<QTapGesture *>(state);
    QTapGesturePrivate *d = q->d_func();

    d->position = QPointF();

    QGestureRecognizer::reset(state);
}

//
// QTapAndHoldGestureRecognizer
//

QTapAndHoldGestureRecognizer::QTapAndHoldGestureRecognizer()
{
}

QGesture *QTapAndHoldGestureRecognizer::create(QObject *target)
{
    if (target && target->isWidgetType()) {
        static_cast<QWidget *>(target)->setAttribute(Qt::WA_AcceptTouchEvents);
    }
    return new QTapAndHoldGesture;
}

QGestureRecognizer::Result
QTapAndHoldGestureRecognizer::recognize(QGesture *state, QObject *object,
                                        QEvent *event)
{
    QTapAndHoldGesture *q = static_cast<QTapAndHoldGesture *>(state);
    QTapAndHoldGesturePrivate *d = q->d_func();

    if (object == state && event->type() == QEvent::Timer) {
        q->killTimer(d->timerId);
        d->timerId = 0;
        return QGestureRecognizer::FinishGesture | QGestureRecognizer::ConsumeEventHint;
    }

    const QTouchEvent *ev = static_cast<const QTouchEvent *>(event);
    const QMouseEvent *me = static_cast<const QMouseEvent *>(event);
#ifndef QT_NO_GRAPHICSVIEW
    const QGraphicsSceneMouseEvent *gsme = static_cast<const QGraphicsSceneMouseEvent *>(event);
#endif

    enum { TapRadius = 40 };

    switch (event->type()) {
#ifndef QT_NO_GRAPHICSVIEW
    case QEvent::GraphicsSceneMousePress:
        d->position = gsme->screenPos();
        q->setHotSpot(d->position);
        if (d->timerId)
            q->killTimer(d->timerId);
        d->timerId = q->startTimer(QTapAndHoldGesturePrivate::Timeout);
        return QGestureRecognizer::MayBeGesture; // we don't show a sign of life until the timeout
#endif
    case QEvent::MouseButtonPress:
        d->position = me->globalPos();
        q->setHotSpot(d->position);
        if (d->timerId)
            q->killTimer(d->timerId);
        d->timerId = q->startTimer(QTapAndHoldGesturePrivate::Timeout);
        return QGestureRecognizer::MayBeGesture; // we don't show a sign of life until the timeout
    case QEvent::TouchBegin:
        d->position = ev->touchPoints().at(0).startScreenPos();
        q->setHotSpot(d->position);
        if (d->timerId)
            q->killTimer(d->timerId);
        d->timerId = q->startTimer(QTapAndHoldGesturePrivate::Timeout);
        return QGestureRecognizer::MayBeGesture; // we don't show a sign of life until the timeout
#ifndef QT_NO_GRAPHICSVIEW
    case QEvent::GraphicsSceneMouseRelease:
#endif
    case QEvent::MouseButtonRelease:
    case QEvent::TouchEnd:
        return QGestureRecognizer::CancelGesture; // get out of the MayBeGesture state
    case QEvent::TouchUpdate:
        if (d->timerId && ev->touchPoints().size() == 1) {
            QTouchEvent::TouchPoint p = ev->touchPoints().at(0);
            QPoint delta = p.pos().toPoint() - p.startPos().toPoint();
            if (delta.manhattanLength() <= TapRadius)
                return QGestureRecognizer::MayBeGesture;
        }
        return QGestureRecognizer::CancelGesture;
    case QEvent::MouseMove: {
        QPoint delta = me->globalPos() - d->position.toPoint();
        if (d->timerId && delta.manhattanLength() <= TapRadius)
            return QGestureRecognizer::MayBeGesture;
        return QGestureRecognizer::CancelGesture;
    }
#ifndef QT_NO_GRAPHICSVIEW
    case QEvent::GraphicsSceneMouseMove: {
        QPoint delta = gsme->screenPos() - d->position.toPoint();
        if (d->timerId && delta.manhattanLength() <= TapRadius)
            return QGestureRecognizer::MayBeGesture;
        return QGestureRecognizer::CancelGesture;
    }
#endif
    default:
        return QGestureRecognizer::Ignore;
    }
}

void QTapAndHoldGestureRecognizer::reset(QGesture *state)
{
    QTapAndHoldGesture *q = static_cast<QTapAndHoldGesture *>(state);
    QTapAndHoldGesturePrivate *d = q->d_func();

    d->position = QPointF();
    if (d->timerId)
        q->killTimer(d->timerId);
    d->timerId = 0;

    QGestureRecognizer::reset(state);
}

QT_END_NAMESPACE

#endif // QT_NO_GESTURES
