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

#include "qevent.h"
#include "qwidget.h"
#include "qscroller.h"
#include "private/qflickgesture_p.h"
#include "private/qscroller_p.h"
#include "qscrollerproperties.h"
#include "private/qscrollerproperties_p.h"
#include "qnumeric.h"
#include "math.h"

#include <QTime>
#include <QElapsedTimer>
#include <QMap>
#include <QApplication>
#include <QAbstractScrollArea>
#include <QGraphicsObject>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QDesktopWidget>
#include <QVector2D>
#include <QtCore/qmath.h>
#include <QtGui/qevent.h>
#include <qnumeric.h>

#include <QtDebug>



QT_BEGIN_NAMESPACE

bool qt_sendSpontaneousEvent(QObject *receiver, QEvent *event);

//#define QSCROLLER_DEBUG

#ifdef QSCROLLER_DEBUG
#  define qScrollerDebug  qDebug
#else
#  define qScrollerDebug  while (false) qDebug
#endif

QDebug &operator<<(QDebug &dbg, const QScrollerPrivate::ScrollSegment &s)
{
    dbg << "\n  Time: start:" << s.startTime << " duration:" << s.deltaTime << " stop progress:" << s.stopProgress;
    dbg << "\n  Pos: start:" << s.startPos << " delta:" << s.deltaPos << " stop:" << s.stopPos;
    dbg << "\n  Curve: type:" << s.curve.type() << "\n";
    return dbg;
}


// a few helper operators to make the code below a lot more readable:
// otherwise a lot of ifs would have to be multi-line to check both the x
// and y coordinate separately.

// returns true only if the abs. value of BOTH x and y are <= f
inline bool operator<=(const QPointF &p, qreal f)
{
    return (qAbs(p.x()) <= f) && (qAbs(p.y()) <= f);
}

// returns true only if the abs. value of BOTH x and y are < f
inline bool operator<(const QPointF &p, qreal f)
{
    return (qAbs(p.x()) < f) && (qAbs(p.y()) < f);
}

// returns true if the abs. value of EITHER x or y are >= f
inline bool operator>=(const QPointF &p, qreal f)
{
    return (qAbs(p.x()) >= f) || (qAbs(p.y()) >= f);
}

// returns true if the abs. value of EITHER x or y are > f
inline bool operator>(const QPointF &p, qreal f)
{
    return (qAbs(p.x()) > f) || (qAbs(p.y()) > f);
}

// returns a new point with both coordinates having the abs. value of the original one
inline QPointF qAbs(const QPointF &p)
{
    return QPointF(qAbs(p.x()), qAbs(p.y()));
}

// returns a new point with all components of p1 multiplied by the corresponding components of p2
inline QPointF operator*(const QPointF &p1, const QPointF &p2)
{
    return QPointF(p1.x() * p2.x(), p1.y() * p2.y());
}

// returns a new point with all components of p1 divided by the corresponding components of p2
inline QPointF operator/(const QPointF &p1, const QPointF &p2)
{
    return QPointF(p1.x() / p2.x(), p1.y() / p2.y());
}

inline QPointF clampToRect(const QPointF &p, const QRectF &rect)
{
    qreal x = qBound(rect.left(), p.x(), rect.right());
    qreal y = qBound(rect.top(), p.y(), rect.bottom());
    return QPointF(x, y);
}

// returns -1, 0 or +1 according to r being <0, ==0 or >0
inline int qSign(qreal r)
{
    return (r < 0) ? -1 : ((r > 0) ? 1 : 0);
}

// this version is not mathematically exact, but it just works for every
// easing curve type (even custom ones)

static qreal differentialForProgress(const QEasingCurve &curve, qreal pos)
{
    const qreal dx = 0.01;
    qreal left = (pos < qreal(0.5)) ? pos : pos - qreal(dx);
    qreal right = (pos >= qreal(0.5)) ? pos : pos + qreal(dx);
    qreal d = (curve.valueForProgress(right) - curve.valueForProgress(left)) / qreal(dx);

    //qScrollerDebug() << "differentialForProgress(type: " << curve.type() << ", pos: " << pos << ") = " << d;

    return d;
}

// this version is not mathematically exact, but it just works for every
// easing curve type (even custom ones)

static qreal progressForValue(const QEasingCurve &curve, qreal value)
{
    if (curve.type() >= QEasingCurve::InElastic &&
        curve.type() < QEasingCurve::Custom) {
        qWarning("progressForValue(): QEasingCurves of type %d do not have an inverse, since they are not injective.", curve.type());
        return value;
    }
    if (value < qreal(0) || value > qreal(1))
        return value;

    qreal progress = value, left(0), right(1);
    for (int iterations = 6; iterations; --iterations) {
        qreal v = curve.valueForProgress(progress);
        if (v < value)
            left = progress;
        else if (v > value)
            right = progress;
        else
            break;
        progress = (left + right) / qreal(2);
    }
    return progress;
}


#ifndef QT_NO_ANIMATION
class QScrollTimer : public QAbstractAnimation
{
public:
    QScrollTimer(QScrollerPrivate *_d)
        : d(_d), ignoreUpdate(false), skip(0)
    { }

    int duration() const
    {
        return -1;
    }

    void start()
    {
        // QAbstractAnimation::start() will immediately call
        // updateCurrentTime(), but our state is not set correctly yet
        ignoreUpdate = true;
        QAbstractAnimation::start();
        ignoreUpdate = false;
        skip = 0;
    }

protected:
    void updateCurrentTime(int /*currentTime*/)
   {
        if (!ignoreUpdate) {
            if (++skip >= d->frameRateSkip()) {
                skip = 0;
                d->timerTick();
            }
        }
    }

private:
    QScrollerPrivate *d;
    bool ignoreUpdate;
    int skip;
};
#endif // QT_NO_ANIMATION

/*!
    \class QScroller
    \brief The QScroller class enables kinetic scrolling for any scrolling widget or graphics item.
    \since 5.0

    \inmodule QtWidgets

    With kinetic scrolling, the user can push the widget in a given
    direction and it will continue to scroll in this direction until it is
    stopped either by the user or by friction.  Aspects of inertia, friction
    and other physical concepts can be changed in order to fine-tune an
    intuitive user experience.

    The QScroller object is the object that stores the current position and
    scrolling speed and takes care of updates.
    QScroller can be triggered by a flick gesture

    \code
        QWidget *w = ...;
        QScroller::grabGesture(w, QScroller::LeftMouseButtonGesture);
    \endcode

    or directly like this:

    \code
        QWidget *w = ...;
        QScroller *scroller = QScroller::scroller(w);
        scroller->scrollTo(QPointF(100, 100));
    \endcode

    The scrolled QObjects receive a QScrollPrepareEvent whenever the scroller needs to
    update its geometry information and a QScrollEvent whenever the content of the object should
    actually be scrolled.

    The scroller uses the global QAbstractAnimation timer to generate its QScrollEvents. This
    can be changed with QScrollerProperties::FrameRate on a per-QScroller basis.

    Several examples in the \c scroller examples directory show how QScroller,
    QScrollEvent and the scroller gesture can be used.

    Even though this kinetic scroller has a large number of settings available via
    QScrollerProperties, we recommend that you leave them all at their default, platform optimized
    values. Before changing them you can experiment with the \c plot example in
    the \c scroller examples directory.

    \sa QScrollEvent, QScrollPrepareEvent, QScrollerProperties
*/

typedef QMap<QObject *, QScroller *> ScrollerHash;
typedef QSet<QScroller *> ScrollerSet;

Q_GLOBAL_STATIC(ScrollerHash, qt_allScrollers)
Q_GLOBAL_STATIC(ScrollerSet, qt_activeScrollers)

/*!
    Returns \c true if a QScroller object was already created for \a target; \c false otherwise.

    \sa scroller()
*/
bool QScroller::hasScroller(QObject *target)
{
    return (qt_allScrollers()->value(target));
}

/*!
    Returns the scroller for the given \a target.
    As long as the object exists this function will always return the same QScroller instance.
    If no QScroller exists for the \a target, one will implicitly be created.
    At no point more than one QScroller will be active on an object.

    \sa hasScroller(), target()
*/
QScroller *QScroller::scroller(QObject *target)
{
    if (!target) {
        qWarning("QScroller::scroller() was called with a null target.");
        return 0;
    }

    if (qt_allScrollers()->contains(target))
        return qt_allScrollers()->value(target);

    QScroller *s = new QScroller(target);
    qt_allScrollers()->insert(target, s);
    return s;
}

/*!
    \overload
    This is the const version of scroller().
*/
const QScroller *QScroller::scroller(const QObject *target)
{
    return scroller(const_cast<QObject*>(target));
}

/*!
    Returns an application wide list of currently active QScroller objects.
    Active QScroller objects are in a state() that is not QScroller::Inactive.
    This function is useful when writing your own gesture recognizer.
*/
QList<QScroller *> QScroller::activeScrollers()
{
    return qt_activeScrollers()->toList();
}

/*!
    Returns the target object of this scroller.
    \sa hasScroller(), scroller()
 */
QObject *QScroller::target() const
{
    Q_D(const QScroller);
    return d->target;
}

/*!
    \fn QScroller::scrollerPropertiesChanged(const QScrollerProperties &newProperties);

    QScroller emits this signal whenever its scroller properties change.
    \a newProperties are the new scroller properties.

    \sa scrollerProperties
*/


/*! \property QScroller::scrollerProperties
    \brief The scroller properties of this scroller.
    The properties are used by the QScroller to determine its scrolling behavior.
*/
QScrollerProperties QScroller::scrollerProperties() const
{
    Q_D(const QScroller);
    return d->properties;
}

void QScroller::setScrollerProperties(const QScrollerProperties &sp)
{
    Q_D(QScroller);
    if (d->properties != sp) {
        d->properties = sp;
        emit scrollerPropertiesChanged(sp);

        // we need to force the recalculation here, since the overshootPolicy may have changed and
        // existing segments may include an overshoot animation.
        d->recalcScrollingSegments(true);
    }
}

#ifndef QT_NO_GESTURES

/*!
    Registers a custom scroll gesture recognizer, grabs it for the \a
    target and returns the resulting gesture type.  If \a scrollGestureType is
    set to TouchGesture the gesture triggers on touch events. If it is set to
    one of LeftMouseButtonGesture, RightMouseButtonGesture or
    MiddleMouseButtonGesture it triggers on mouse events of the
    corresponding button.

    Only one scroll gesture can be active on a single object at the same
    time. If you call this function twice on the same object, it will
    ungrab the existing gesture before grabbing the new one.

    \note To avoid unwanted side-effects, mouse events are consumed while
    the gesture is triggered. Since the initial mouse press event is
    not consumed, the gesture sends a fake mouse release event
    at the global position \c{(INT_MIN, INT_MIN)}. This ensures that
    internal states of the widget that received the original mouse press
    are consistent.

    \sa ungrabGesture(), grabbedGesture()
*/
Qt::GestureType QScroller::grabGesture(QObject *target, ScrollerGestureType scrollGestureType)
{
    // ensure that a scroller for target is created
    QScroller *s = scroller(target);
    if (!s)
        return Qt::GestureType(0);

    QScrollerPrivate *sp = s->d_ptr;
    if (sp->recognizer)
        ungrabGesture(target); // ungrab the old gesture

    Qt::MouseButton button;
    switch (scrollGestureType) {
    case LeftMouseButtonGesture  : button = Qt::LeftButton; break;
    case RightMouseButtonGesture : button = Qt::RightButton; break;
    case MiddleMouseButtonGesture: button = Qt::MiddleButton; break;
    default                      :
    case TouchGesture            : button = Qt::NoButton; break; // NoButton == Touch
    }

    sp->recognizer = new QFlickGestureRecognizer(button);
    sp->recognizerType = QGestureRecognizer::registerRecognizer(sp->recognizer);

    if (target->isWidgetType()) {
        QWidget *widget = static_cast<QWidget *>(target);
        widget->grabGesture(sp->recognizerType);
        if (scrollGestureType == TouchGesture)
            widget->setAttribute(Qt::WA_AcceptTouchEvents);
#ifndef QT_NO_GRAPHICSVIEW
    } else if (QGraphicsObject *go = qobject_cast<QGraphicsObject*>(target)) {
        if (scrollGestureType == TouchGesture)
            go->setAcceptTouchEvents(true);
        go->grabGesture(sp->recognizerType);
#endif // QT_NO_GRAPHICSVIEW
    }
    return sp->recognizerType;
}

/*!
    Returns the gesture type currently grabbed for the \a target or 0 if no
    gesture is grabbed.

    \sa grabGesture(), ungrabGesture()
*/
Qt::GestureType QScroller::grabbedGesture(QObject *target)
{
    QScroller *s = scroller(target);
    if (s && s->d_ptr)
        return s->d_ptr->recognizerType;
    else
        return Qt::GestureType(0);
}

/*!
    Ungrabs the gesture for the \a target.
    Does nothing if no gesture is grabbed.

    \sa grabGesture(), grabbedGesture()
*/
void QScroller::ungrabGesture(QObject *target)
{
    QScroller *s = scroller(target);
    if (!s)
        return;

    QScrollerPrivate *sp = s->d_ptr;
    if (!sp->recognizer)
        return; // nothing to do

    if (target->isWidgetType()) {
        QWidget *widget = static_cast<QWidget *>(target);
        widget->ungrabGesture(sp->recognizerType);
#ifndef QT_NO_GRAPHICSVIEW
    } else if (QGraphicsObject *go = qobject_cast<QGraphicsObject*>(target)) {
        go->ungrabGesture(sp->recognizerType);
#endif
    }

    QGestureRecognizer::unregisterRecognizer(sp->recognizerType);
    // do not delete the recognizer. The QGestureManager is doing this.
    sp->recognizer = 0;
}

#endif // QT_NO_GESTURES

/*!
    \internal
*/
QScroller::QScroller(QObject *target)
    : d_ptr(new QScrollerPrivate(this, target))
{
    Q_ASSERT(target); // you can't create a scroller without a target in any normal way
    Q_D(QScroller);
    d->init();
}

/*!
    \internal
*/
QScroller::~QScroller()
{
    Q_D(QScroller);
#ifndef QT_NO_GESTURES
    QGestureRecognizer::unregisterRecognizer(d->recognizerType);
    // do not delete the recognizer. The QGestureManager is doing this.
    d->recognizer = 0;
#endif
    qt_allScrollers()->remove(d->target);
    qt_activeScrollers()->remove(this);

    delete d_ptr;
}


/*!
    \fn QScroller::stateChanged(QScroller::State newState);

    QScroller emits this signal whenever the state changes. \a newState is the new State.

    \sa state
*/

/*!
    \property QScroller::state
    \brief the state of the scroller

    \sa QScroller::State
*/
QScroller::State QScroller::state() const
{
    Q_D(const QScroller);
    return d->state;
}

/*!
    Stops the scroller and resets its state back to Inactive.
*/
void QScroller::stop()
{
    Q_D(QScroller);
    if (d->state != Inactive) {
        QPointF here = clampToRect(d->contentPosition, d->contentPosRange);
        qreal snapX = d->nextSnapPos(here.x(), 0, Qt::Horizontal);
        qreal snapY = d->nextSnapPos(here.y(), 0, Qt::Vertical);
        QPointF snap = here;
        if (!qIsNaN(snapX))
            snap.setX(snapX);
        if (!qIsNaN(snapY))
            snap.setY(snapY);
        d->contentPosition = snap;
        d->overshootPosition = QPointF(0, 0);

        d->setState(Inactive);
    }
}

/*!
    Returns the pixel per meter metric for the scrolled widget.

    The value is reported for both the x and y axis separately by using a QPointF.

    \note Please note that this value should be physically correct. The actual DPI settings
    that Qt returns for the display may be reported wrongly on purpose by the underlying
    windowing system, for example on Mac OS X.
*/
QPointF QScroller::pixelPerMeter() const
{
    Q_D(const QScroller);
    QPointF ppm = d->pixelPerMeter;

#ifndef QT_NO_GRAPHICSVIEW
    if (QGraphicsObject *go = qobject_cast<QGraphicsObject *>(d->target)) {
        QTransform viewtr;
        //TODO: the first view isn't really correct - maybe use an additional field in the prepare event?
        if (go->scene() && !go->scene()->views().isEmpty())
            viewtr = go->scene()->views().first()->viewportTransform();
        QTransform tr = go->deviceTransform(viewtr);
        if (tr.isScaling()) {
            QPointF p0 = tr.map(QPointF(0, 0));
            QPointF px = tr.map(QPointF(1, 0));
            QPointF py = tr.map(QPointF(0, 1));
            ppm.rx() /= QLineF(p0, px).length();
            ppm.ry() /= QLineF(p0, py).length();
        }
    }
#endif // QT_NO_GRAPHICSVIEW
    return ppm;
}

/*!
    Returns the current scrolling velocity in meter per second when the state is Scrolling or Dragging.
    Returns a zero velocity otherwise.

    The velocity is reported for both the x and y axis separately by using a QPointF.

    \sa pixelPerMeter()
*/
QPointF QScroller::velocity() const
{
    Q_D(const QScroller);
    const QScrollerPropertiesPrivate *sp = d->properties.d.data();

     switch (state()) {
     case Dragging:
        return d->releaseVelocity;
     case Scrolling: {
        QPointF vel;
        qint64 now = d->monotonicTimer.elapsed();

        if (!d->xSegments.isEmpty()) {
            const QScrollerPrivate::ScrollSegment &s = d->xSegments.head();
            qreal progress = qreal(now - s.startTime) / qreal(s.deltaTime);
            qreal v = qSign(s.deltaPos) * qreal(s.deltaTime) / qreal(1000) * sp->decelerationFactor * qreal(0.5) * differentialForProgress(s.curve, progress);
            vel.setX(v);
        }

        if (!d->ySegments.isEmpty()) {
            const QScrollerPrivate::ScrollSegment &s = d->ySegments.head();
            qreal progress = qreal(now - s.startTime) / qreal(s.deltaTime);
            qreal v = qSign(s.deltaPos) * qreal(s.deltaTime) / qreal(1000) * sp->decelerationFactor * qreal(0.5) * differentialForProgress(s.curve, progress);
            vel.setY(v);
        }
        return vel;
     }
     default:
         return QPointF(0, 0);
     }
}

/*!
    Returns the estimated final position for the current scroll movement.
    Returns the current position if the scroller state is not Scrolling.
    The result is undefined when the scroller state is Inactive.

    The target position is in pixel.

    \sa pixelPerMeter(), scrollTo()
*/
QPointF QScroller::finalPosition() const
{
    Q_D(const QScroller);
    return QPointF(d->scrollingSegmentsEndPos(Qt::Horizontal),
                   d->scrollingSegmentsEndPos(Qt::Vertical));
}

/*!
    Starts scrolling the widget so that point \a pos is at the top-left position in
    the viewport.

    The behaviour when scrolling outside the valid scroll area is undefined.
    In this case the scroller might or might not overshoot.

    The scrolling speed will be calculated so that the given position will
    be reached after a platform-defined time span.

    \a pos is given in viewport coordinates.

    \sa ensureVisible()
*/
void QScroller::scrollTo(const QPointF &pos)
{
    // we could make this adjustable via QScrollerProperties
    scrollTo(pos, 300);
}

/*! \overload

    This version will reach its destination position in \a scrollTime milliseconds.
*/
void QScroller::scrollTo(const QPointF &pos, int scrollTime)
{
    Q_D(QScroller);

    if (d->state == Pressed || d->state == Dragging )
        return;

    // no need to resend a prepare event if we are already scrolling
    if (d->state == Inactive && !d->prepareScrolling(QPointF()))
        return;

    QPointF newpos = clampToRect(pos, d->contentPosRange);
    qreal snapX = d->nextSnapPos(newpos.x(), 0, Qt::Horizontal);
    qreal snapY = d->nextSnapPos(newpos.y(), 0, Qt::Vertical);
    if (!qIsNaN(snapX))
        newpos.setX(snapX);
    if (!qIsNaN(snapY))
        newpos.setY(snapY);

    qScrollerDebug() << "QScroller::scrollTo(req:" << pos << " [pix] / snap:" << newpos << ", " << scrollTime << " [ms])";

    if (newpos == d->contentPosition + d->overshootPosition)
        return;

    QPointF vel = velocity();

    if (scrollTime < 0)
        scrollTime = 0;
    qreal time = qreal(scrollTime) / 1000;

    d->createScrollToSegments(vel.x(), time, newpos.x(), Qt::Horizontal, QScrollerPrivate::ScrollTypeScrollTo);
    d->createScrollToSegments(vel.y(), time, newpos.y(), Qt::Vertical, QScrollerPrivate::ScrollTypeScrollTo);

    if (!scrollTime)
        d->setContentPositionHelperScrolling();
    d->setState(scrollTime ? Scrolling : Inactive);
}

/*!
    Starts scrolling so that the rectangle \a rect is visible inside the
    viewport with additional margins specified in pixels by \a xmargin and \a ymargin around
    the rect.

    In cases where it is not possible to fit the rect plus margins inside the viewport the contents
    are scrolled so that as much as possible is visible from \a rect.

    The scrolling speed is calculated so that the given position is reached after a platform-defined
    time span.

    This function performs the actual scrolling by calling scrollTo().

    \sa scrollTo()
*/
void QScroller::ensureVisible(const QRectF &rect, qreal xmargin, qreal ymargin)
{
    // we could make this adjustable via QScrollerProperties
    ensureVisible(rect, xmargin, ymargin, 1000);
}

/*! \overload

    This version will reach its destination position in \a scrollTime milliseconds.
*/
void QScroller::ensureVisible(const QRectF &rect, qreal xmargin, qreal ymargin, int scrollTime)
{
    Q_D(QScroller);

    if (d->state == Pressed || d->state == Dragging )
        return;

    if (d->state == Inactive && !d->prepareScrolling(QPointF()))
        return;

    // -- calculate the current pos (or the position after the current scroll)
    QPointF startPos = d->contentPosition + d->overshootPosition;
    startPos = QPointF(d->scrollingSegmentsEndPos(Qt::Horizontal),
                       d->scrollingSegmentsEndPos(Qt::Vertical));

    QRectF marginRect(rect.x() - xmargin, rect.y() - ymargin,
                      rect.width() + 2 * xmargin, rect.height() + 2 * ymargin);

    QSizeF visible = d->viewportSize;
    QRectF visibleRect(startPos, visible);

    qScrollerDebug() << "QScroller::ensureVisible(" << rect << " [pix], " << xmargin << " [pix], " << ymargin << " [pix], " << scrollTime << "[ms])";
    qScrollerDebug() << "  --> content position:" << d->contentPosition;

    if (visibleRect.contains(marginRect))
        return;

    QPointF newPos = startPos;

    if (visibleRect.width() < rect.width()) {
        // at least try to move the rect into view
        if (rect.left() > visibleRect.left())
            newPos.setX(rect.left());
        else if (rect.right() < visibleRect.right())
            newPos.setX(rect.right() - visible.width());

    } else if (visibleRect.width() < marginRect.width()) {
        newPos.setX(rect.center().x() - visibleRect.width() / 2);
    } else if (marginRect.left() > visibleRect.left()) {
        newPos.setX(marginRect.left());
    } else if (marginRect.right() < visibleRect.right()) {
        newPos.setX(marginRect.right() - visible.width());
    }

    if (visibleRect.height() < rect.height()) {
        // at least try to move the rect into view
        if (rect.top() > visibleRect.top())
            newPos.setX(rect.top());
        else if (rect.bottom() < visibleRect.bottom())
            newPos.setX(rect.bottom() - visible.height());

    } else if (visibleRect.height() < marginRect.height()) {
        newPos.setY(rect.center().y() - visibleRect.height() / 2);
    } else if (marginRect.top() > visibleRect.top()) {
        newPos.setY(marginRect.top());
    } else if (marginRect.bottom() < visibleRect.bottom()) {
        newPos.setY(marginRect.bottom() - visible.height());
    }

    // clamp to maximum content position
    newPos = clampToRect(newPos, d->contentPosRange);
    if (newPos == startPos)
        return;

    scrollTo(newPos, scrollTime);
}

/*! This function resends the QScrollPrepareEvent.
    Calling resendPrepareEvent triggers a QScrollPrepareEvent from the scroller.
    This allows the receiver to re-set content position and content size while
    scrolling.
    Calling this function while in the Inactive state is useless as the prepare event
    is sent again before scrolling starts.
 */
void QScroller::resendPrepareEvent()
{
    Q_D(QScroller);
    d->prepareScrolling(d->pressPosition);
}

/*! Set the snap positions for the horizontal axis to a list of \a positions.
    This overwrites all previously set snap positions and also a previously
    set snapping interval.
    Snapping can be deactivated by setting an empty list of positions.
 */
void QScroller::setSnapPositionsX(const QList<qreal> &positions)
{
    Q_D(QScroller);
    d->snapPositionsX = positions;
    d->snapIntervalX = 0.0;

    d->recalcScrollingSegments();
}

/*! Set the snap positions for the horizontal axis to regular spaced intervals.
    The first snap position is at \a first. The next at \a first + \a interval.
    This can be used to implement a list header.
    This overwrites all previously set snap positions and also a previously
    set snapping interval.
    Snapping can be deactivated by setting an interval of 0.0
 */
void QScroller::setSnapPositionsX(qreal first, qreal interval)
{
    Q_D(QScroller);
    d->snapFirstX = first;
    d->snapIntervalX = interval;
    d->snapPositionsX.clear();

    d->recalcScrollingSegments();
}

/*! Set the snap positions for the vertical axis to a list of \a positions.
    This overwrites all previously set snap positions and also a previously
    set snapping interval.
    Snapping can be deactivated by setting an empty list of positions.
 */
void QScroller::setSnapPositionsY(const QList<qreal> &positions)
{
    Q_D(QScroller);
    d->snapPositionsY = positions;
    d->snapIntervalY = 0.0;

    d->recalcScrollingSegments();
}

/*! Set the snap positions for the vertical axis to regular spaced intervals.
    The first snap position is at \a first. The next at \a first + \a interval.
    This overwrites all previously set snap positions and also a previously
    set snapping interval.
    Snapping can be deactivated by setting an interval of 0.0
 */
void QScroller::setSnapPositionsY(qreal first, qreal interval)
{
    Q_D(QScroller);
    d->snapFirstY = first;
    d->snapIntervalY = interval;
    d->snapPositionsY.clear();

    d->recalcScrollingSegments();
}



// -------------- private ------------

QScrollerPrivate::QScrollerPrivate(QScroller *q, QObject *_target)
    : target(_target)
#ifndef QT_NO_GESTURES
    , recognizer(0)
    , recognizerType(Qt::CustomGesture)
#endif
    , state(QScroller::Inactive)
    , firstScroll(true)
    , pressTimestamp(0)
    , lastTimestamp(0)
    , snapFirstX(-1.0)
    , snapIntervalX(0.0)
    , snapFirstY(-1.0)
    , snapIntervalY(0.0)
#ifndef QT_NO_ANIMATION
    , scrollTimer(new QScrollTimer(this))
#endif
    , q_ptr(q)
{
    connect(target, SIGNAL(destroyed(QObject*)), this, SLOT(targetDestroyed()));
}

void QScrollerPrivate::init()
{
    setDpiFromWidget(0);
    monotonicTimer.start();
}

void QScrollerPrivate::sendEvent(QObject *o, QEvent *e)
{
    qt_sendSpontaneousEvent(o, e);
}

const char *QScrollerPrivate::stateName(QScroller::State state)
{
    switch (state) {
    case QScroller::Inactive:  return "inactive";
    case QScroller::Pressed:   return "pressed";
    case QScroller::Dragging:  return "dragging";
    case QScroller::Scrolling: return "scrolling";
    default:                   return "(invalid)";
    }
}

const char *QScrollerPrivate::inputName(QScroller::Input input)
{
    switch (input) {
    case QScroller::InputPress:   return "press";
    case QScroller::InputMove:    return "move";
    case QScroller::InputRelease: return "release";
    default:                      return "(invalid)";
    }
}

void QScrollerPrivate::targetDestroyed()
{
#ifndef QT_NO_ANIMATION
    scrollTimer->stop();
#endif
    delete q_ptr;
}

void QScrollerPrivate::timerTick()
{
    struct timerevent {
        QScroller::State state;
        typedef void (QScrollerPrivate::*timerhandler_t)();
        timerhandler_t handler;
    };

    timerevent timerevents[] = {
        { QScroller::Dragging, &QScrollerPrivate::timerEventWhileDragging },
        { QScroller::Scrolling, &QScrollerPrivate::timerEventWhileScrolling },
    };

    for (int i = 0; i < int(sizeof(timerevents) / sizeof(*timerevents)); ++i) {
        timerevent *te = timerevents + i;

        if (state == te->state) {
            (this->*te->handler)();
            return;
        }
    }

#ifndef QT_NO_ANIMATION
    scrollTimer->stop();
#endif
}

/*!
    This function is used by gesture recognizers to inform the scroller about a new input event.
    The scroller changes its internal state() according to the input event and its attached
    scroller properties. The scroller doesn't distinguish between the kind of input device the
    event came from. Therefore the event needs to be split into the \a input type, a \a position and a
    milli-second \a timestamp.  The \a position needs to be in the target's coordinate system.

    The return value is \c true if the event should be consumed by the calling filter or \c false
    if the event should be forwarded to the control.

    \note Using grabGesture() should be sufficient for most use cases.
*/
bool QScroller::handleInput(Input input, const QPointF &position, qint64 timestamp)
{
    Q_D(QScroller);

    qScrollerDebug() << "QScroller::handleInput(" << input << ", " << d->stateName(d->state) << ", " << position << ", " << timestamp << ")";
    struct statechange {
        State state;
        Input input;
        typedef bool (QScrollerPrivate::*inputhandler_t)(const QPointF &position, qint64 timestamp);
        inputhandler_t handler;
    };

    statechange statechanges[] = {
        { QScroller::Inactive,  InputPress,   &QScrollerPrivate::pressWhileInactive },
        { QScroller::Pressed,   InputMove,    &QScrollerPrivate::moveWhilePressed },
        { QScroller::Pressed,   InputRelease, &QScrollerPrivate::releaseWhilePressed },
        { QScroller::Dragging,  InputMove,    &QScrollerPrivate::moveWhileDragging },
        { QScroller::Dragging,  InputRelease, &QScrollerPrivate::releaseWhileDragging },
        { QScroller::Scrolling, InputPress,   &QScrollerPrivate::pressWhileScrolling }
    };

    for (int i = 0; i < int(sizeof(statechanges) / sizeof(*statechanges)); ++i) {
        statechange *sc = statechanges + i;

         if (d->state == sc->state && input == sc->input)
             return (d->*sc->handler)(position - d->overshootPosition, timestamp);
    }
    return false;
}

#if !defined(Q_WS_MAC)
// the Mac version is implemented in qscroller_mac.mm

QPointF QScrollerPrivate::realDpi(int screen)
{
#  if defined(Q_WS_X11) && !defined(QT_NO_XRANDR)
    if (X11 && X11->use_xrandr && X11->ptrXRRSizes && X11->ptrXRRRootToScreen) {
        int nsizes = 0;
        // QDesktopWidget is based on Xinerama screens, which do not always
        // correspond to RandR screens: NVidia's TwinView e.g.  will show up
        // as 2 screens in QDesktopWidget, but libXRandR will only see 1 screen.
        // (although with the combined size of the Xinerama screens).
        // Additionally, libXrandr will simply crash when calling XRRSizes
        // for (the non-existent) screen 1 in this scenario.
        Window root =  RootWindow(X11->display, screen == -1 ? X11->defaultScreen : screen);
        int randrscreen = (root != XNone) ? X11->ptrXRRRootToScreen(X11->display, root) : -1;

        XRRScreenSize *sizes = X11->ptrXRRSizes(X11->display, randrscreen == -1 ? 0 : randrscreen, &nsizes);
        if (nsizes > 0 && sizes && sizes->width && sizes->height && sizes->mwidth && sizes->mheight) {
            qScrollerDebug() << "XRandR DPI:" << QPointF(qreal(25.4) * qreal(sizes->width) / qreal(sizes->mwidth),
                                                         qreal(25.4) * qreal(sizes->height) / qreal(sizes->mheight));
            return QPointF(qreal(25.4) * qreal(sizes->width) / qreal(sizes->mwidth),
                           qreal(25.4) * qreal(sizes->height) / qreal(sizes->mheight));
        }
    }
#  endif

    QWidget *w = QApplication::desktop()->screen(screen);
    return QPointF(w->physicalDpiX(), w->physicalDpiY());
}

#endif // !Q_WS_MAC


/*! \internal
    Returns the resolution of the used screen.
*/
QPointF QScrollerPrivate::dpi() const
{
    return pixelPerMeter * qreal(0.0254);
}

/*! \internal
    Sets the resolution used for scrolling.
    This resolution is only used by the kinetic scroller. If you change this
    then the scroller will behave quite different as a lot of the values are
    given in physical distances (millimeter).
*/
void QScrollerPrivate::setDpi(const QPointF &dpi)
{
    pixelPerMeter = dpi / qreal(0.0254);
}

/*! \internal
    Sets the dpi used for scrolling to the value of the widget.
*/
void QScrollerPrivate::setDpiFromWidget(QWidget *widget)
{
    QDesktopWidget *dw = QApplication::desktop();
    setDpi(realDpi(widget ? dw->screenNumber(widget) : dw->primaryScreen()));
}

/*! \internal
    Updates the velocity during dragging.
    Sets releaseVelocity.
*/
void QScrollerPrivate::updateVelocity(const QPointF &deltaPixelRaw, qint64 deltaTime)
{
    if (deltaTime <= 0)
        return;

    Q_Q(QScroller);
    QPointF ppm = q->pixelPerMeter();
    const QScrollerPropertiesPrivate *sp = properties.d.data();
    QPointF deltaPixel = deltaPixelRaw;

    qScrollerDebug() << "QScroller::updateVelocity(" << deltaPixelRaw << " [delta pix], " << deltaTime << " [delta ms])";

    // faster than 2.5mm/ms seems bogus (that would be a screen height in ~20 ms)
    if (((deltaPixelRaw / qreal(deltaTime)).manhattanLength() / ((ppm.x() + ppm.y()) / 2) * 1000) > qreal(2.5))
        deltaPixel = deltaPixelRaw * qreal(2.5) * ppm / 1000 / (deltaPixelRaw / qreal(deltaTime)).manhattanLength();

    QPointF newv = -deltaPixel / qreal(deltaTime) * qreal(1000) / ppm;
    // around 95% of all updates are in the [1..50] ms range, so make sure
    // to scale the smoothing factor over that range: this way a 50ms update
    // will have full impact, while 5ms update will only have a 10% impact.
    qreal smoothing = sp->dragVelocitySmoothingFactor * qMin(qreal(deltaTime), qreal(50)) / qreal(50);

    // only smooth if we already have a release velocity and only if the
    // user hasn't stopped to move his finger for more than 100ms
    if ((releaseVelocity != QPointF(0, 0)) && (deltaTime < 100)) {
        qScrollerDebug() << "SMOOTHED from " << newv << " to " << newv * smoothing + releaseVelocity * (qreal(1) - smoothing);
        // smooth x or y only if the new velocity is either 0 or at least in
        // the same direction of the release velocity
        if (!newv.x() || (qSign(releaseVelocity.x()) == qSign(newv.x())))
            newv.setX(newv.x() * smoothing + releaseVelocity.x() * (qreal(1) - smoothing));
        if (!newv.y() || (qSign(releaseVelocity.y()) == qSign(newv.y())))
            newv.setY(newv.y() * smoothing + releaseVelocity.y() * (qreal(1) - smoothing));
    } else
        qScrollerDebug() << "NO SMOOTHING to " << newv;

    releaseVelocity.setX(qBound(-sp->maximumVelocity, newv.x(), sp->maximumVelocity));
    releaseVelocity.setY(qBound(-sp->maximumVelocity, newv.y(), sp->maximumVelocity));

    qScrollerDebug() << "  --> new velocity:" << releaseVelocity;
}

void QScrollerPrivate::pushSegment(ScrollType type, qreal deltaTime, qreal stopProgress, qreal startPos, qreal deltaPos, qreal stopPos, QEasingCurve::Type curve, Qt::Orientation orientation)
{
    if (startPos == stopPos || deltaPos == 0)
        return;

    ScrollSegment s;
    if (orientation == Qt::Horizontal && !xSegments.isEmpty())
        s.startTime = xSegments.last().startTime + xSegments.last().deltaTime * xSegments.last().stopProgress;
    else if (orientation == Qt::Vertical && !ySegments.isEmpty())
        s.startTime = ySegments.last().startTime + ySegments.last().deltaTime * ySegments.last().stopProgress;
    else
        s.startTime = monotonicTimer.elapsed();

    s.startPos = startPos;
    s.deltaPos = deltaPos;
    s.stopPos = stopPos;
    s.deltaTime = deltaTime * 1000;
    s.stopProgress = stopProgress;
    s.curve.setType(curve);
    s.type = type;

    if (orientation == Qt::Horizontal)
        xSegments.enqueue(s);
    else
        ySegments.enqueue(s);

    qScrollerDebug() << "+++ Added a new ScrollSegment: " << s;
}


/*! \internal
    Clears the old segments and recalculates them if the current segments are not longer valid
*/
void QScrollerPrivate::recalcScrollingSegments(bool forceRecalc)
{
    Q_Q(QScroller);
    QPointF ppm = q->pixelPerMeter();

    releaseVelocity = q->velocity();

    if (forceRecalc ||
        !scrollingSegmentsValid(Qt::Horizontal) ||
        !scrollingSegmentsValid(Qt::Vertical))
        createScrollingSegments(releaseVelocity, contentPosition + overshootPosition, ppm);
}

/*! \internal
    Returns the end position after the current scroll has finished.
*/
qreal QScrollerPrivate::scrollingSegmentsEndPos(Qt::Orientation orientation) const
{
    if (orientation == Qt::Horizontal) {
        if (xSegments.isEmpty())
            return contentPosition.x() + overshootPosition.x();
        else
            return xSegments.last().stopPos;
    } else {
        if (ySegments.isEmpty())
            return contentPosition.y() + overshootPosition.y();
        else
            return ySegments.last().stopPos;
    }
}

/*! \internal
    Checks if the scroller segment end in a valid position.
*/
bool QScrollerPrivate::scrollingSegmentsValid(Qt::Orientation orientation)
{
    QQueue<ScrollSegment> *segments;
    qreal minPos;
    qreal maxPos;

    if (orientation == Qt::Horizontal) {
        segments = &xSegments;
        minPos = contentPosRange.left();
        maxPos = contentPosRange.right();
    } else {
        segments = &ySegments;
        minPos = contentPosRange.top();
        maxPos = contentPosRange.bottom();
    }

    if (segments->isEmpty())
        return true;

    const ScrollSegment &last = segments->last();
    qreal stopPos = last.stopPos;

    if (last.type == ScrollTypeScrollTo)
        return true; // scrollTo is always valid

    if (last.type == ScrollTypeOvershoot &&
        (stopPos != minPos && stopPos != maxPos))
        return false;

    if (stopPos < minPos || stopPos > maxPos)
        return false;

    if (stopPos == minPos || stopPos == maxPos) // the begin and the end of the list are always ok
        return true;

    qreal nextSnap = nextSnapPos(stopPos, 0, orientation);
    if (!qIsNaN(nextSnap) && stopPos != nextSnap)
        return false;

    return true;
}

/*! \internal
   Creates the sections needed to scroll to the specific \a endPos to the segments queue.
*/
void QScrollerPrivate::createScrollToSegments(qreal v, qreal deltaTime, qreal endPos, Qt::Orientation orientation, ScrollType type)
{
    Q_UNUSED(v);

    if (orientation == Qt::Horizontal)
        xSegments.clear();
    else
        ySegments.clear();

    qScrollerDebug() << "+++ createScrollToSegments: t:" << deltaTime << "ep:" << endPos << "o:" << int(orientation);

    const QScrollerPropertiesPrivate *sp = properties.d.data();

    qreal startPos = (orientation == Qt::Horizontal) ? contentPosition.x() + overshootPosition.x()
                                                     : contentPosition.y() + overshootPosition.y();
    qreal deltaPos = (endPos - startPos) / 2;

    pushSegment(type, deltaTime * qreal(0.3), qreal(1.0), startPos, deltaPos, startPos + deltaPos, QEasingCurve::InQuad, orientation);
    pushSegment(type, deltaTime * qreal(0.7), qreal(1.0), startPos + deltaPos, deltaPos, endPos, sp->scrollingCurve.type(), orientation);
}

/*! \internal
*/
void QScrollerPrivate::createScrollingSegments(qreal v, qreal startPos,
                                               qreal deltaTime, qreal deltaPos,
                                               Qt::Orientation orientation)
{
    const QScrollerPropertiesPrivate *sp = properties.d.data();

    QScrollerProperties::OvershootPolicy policy;
    qreal minPos;
    qreal maxPos;
    qreal viewSize;

    if (orientation == Qt::Horizontal) {
        xSegments.clear();
        policy = sp->hOvershootPolicy;
        minPos = contentPosRange.left();
        maxPos = contentPosRange.right();
        viewSize = viewportSize.width();
    } else {
        ySegments.clear();
        policy = sp->vOvershootPolicy;
        minPos = contentPosRange.top();
        maxPos = contentPosRange.bottom();
        viewSize = viewportSize.height();
    }

    bool alwaysOvershoot = (policy == QScrollerProperties::OvershootAlwaysOn);
    bool noOvershoot = (policy == QScrollerProperties::OvershootAlwaysOff) || !sp->overshootScrollDistanceFactor;
    bool canOvershoot = !noOvershoot && (alwaysOvershoot || maxPos);

    qScrollerDebug() << "+++ createScrollingSegments: s:" << startPos << "maxPos:" << maxPos << "o:" << int(orientation);

    qScrollerDebug() << "v = " << v << ", decelerationFactor = " << sp->decelerationFactor << ", curveType = " << sp->scrollingCurve.type();

    qreal endPos = startPos + deltaPos;

    qScrollerDebug() << "  Real Delta:" << deltaPos;

    // -- check if are in overshoot and end in overshoot
    if ((startPos < minPos && endPos < minPos) ||
        (startPos > maxPos && endPos > maxPos)) {
        qreal stopPos = endPos < minPos ? minPos : maxPos;
        qreal oDeltaTime = sp->overshootScrollTime;

        pushSegment(ScrollTypeOvershoot, oDeltaTime * qreal(0.7), qreal(1.0), startPos, stopPos - startPos, stopPos, sp->scrollingCurve.type(), orientation);
        return;
    }

    // -- determine snap points
    qreal nextSnap = nextSnapPos(endPos, 0, orientation);
    qreal lowerSnapPos = nextSnapPos(startPos, -1, orientation);
    qreal higherSnapPos = nextSnapPos(startPos, 1, orientation);

    qScrollerDebug() << "  Real Delta:" << lowerSnapPos <<"-"<<nextSnap <<"-"<<higherSnapPos;

    // - check if we can reach another snap point
    if (nextSnap > higherSnapPos || qIsNaN(higherSnapPos))
        higherSnapPos = nextSnap;
    if (nextSnap < lowerSnapPos || qIsNaN(lowerSnapPos))
        lowerSnapPos = nextSnap;

    if (qAbs(v) < sp->minimumVelocity) {

        qScrollerDebug() << "### below minimum Vel" << orientation;

        // - no snap points or already at one
        if (qIsNaN(nextSnap) || nextSnap == startPos)
            return; // nothing to do, no scrolling needed.

        // - decide which point to use

        qreal snapDistance = higherSnapPos - lowerSnapPos;

        qreal pressDistance = (orientation == Qt::Horizontal) ?
            lastPosition.x() - pressPosition.x() :
            lastPosition.y() - pressPosition.y();

        // if not dragged far enough, pick the next snap point.
        if (sp->snapPositionRatio == 0.0 || qAbs(pressDistance / sp->snapPositionRatio) > snapDistance)
            endPos = nextSnap;
        else if (pressDistance < 0.0)
            endPos = lowerSnapPos;
        else
            endPos = higherSnapPos;

        deltaPos = endPos - startPos;
        qreal midPos = startPos + deltaPos * qreal(0.3);
        pushSegment(ScrollTypeFlick, sp->snapTime * qreal(0.3), qreal(1.0), startPos, midPos - startPos, midPos, QEasingCurve::InQuad, orientation);
        pushSegment(ScrollTypeFlick, sp->snapTime * qreal(0.7), qreal(1.0), midPos, endPos - midPos, endPos, sp->scrollingCurve.type(), orientation);
        return;
    }

    // - go to the next snappoint if there is one
    if (v > 0 && !qIsNaN(higherSnapPos)) {
        // change the time in relation to the changed end position
        if (endPos - startPos)
            deltaTime *= qAbs((higherSnapPos - startPos) / (endPos - startPos));
        if (deltaTime > sp->snapTime)
            deltaTime = sp->snapTime;
        endPos = higherSnapPos;

    } else if (v < 0 && !qIsNaN(lowerSnapPos)) {
        // change the time in relation to the changed end position
        if (endPos - startPos)
            deltaTime *= qAbs((lowerSnapPos - startPos) / (endPos - startPos));
        if (deltaTime > sp->snapTime)
            deltaTime = sp->snapTime;
        endPos = lowerSnapPos;

    // -- check if we are overshooting
    } else if (endPos < minPos || endPos > maxPos) {
        qreal stopPos = endPos < minPos ? minPos : maxPos;

        qScrollerDebug() << "Overshoot: delta:" << (stopPos - startPos);

        qreal stopProgress = progressForValue(sp->scrollingCurve, qAbs((stopPos - startPos) / deltaPos));

        if (!canOvershoot) {
            qScrollerDebug() << "Overshoot stopp:" << stopProgress;

            pushSegment(ScrollTypeFlick, deltaTime, stopProgress, startPos, endPos, stopPos, sp->scrollingCurve.type(), orientation);
        } else {
            qreal oDeltaTime = sp->overshootScrollTime;
            qreal oStopProgress = qMin(stopProgress + oDeltaTime * qreal(0.3) / deltaTime, qreal(1));
            qreal oDistance = startPos + deltaPos * sp->scrollingCurve.valueForProgress(oStopProgress) - stopPos;
            qreal oMaxDistance = qSign(oDistance) * (viewSize * sp->overshootScrollDistanceFactor);

            qScrollerDebug() << "1 oDistance:" << oDistance << "Max:" << oMaxDistance << "stopP/oStopP" << stopProgress << oStopProgress;

            if (qAbs(oDistance) > qAbs(oMaxDistance)) {
                oStopProgress = progressForValue(sp->scrollingCurve, qAbs((stopPos + oMaxDistance - startPos) / deltaPos));
                oDistance = oMaxDistance;
                qScrollerDebug() << "2 oDistance:" << oDistance << "Max:" << oMaxDistance << "stopP/oStopP" << stopProgress << oStopProgress;
            }

            pushSegment(ScrollTypeFlick, deltaTime, oStopProgress, startPos, deltaPos, stopPos + oDistance, sp->scrollingCurve.type(), orientation);
            pushSegment(ScrollTypeOvershoot, oDeltaTime * qreal(0.7), qreal(1.0), stopPos + oDistance, -oDistance, stopPos, sp->scrollingCurve.type(), orientation);
        }
        return;
    }

    pushSegment(ScrollTypeFlick, deltaTime, qreal(1.0), startPos, deltaPos, endPos, sp->scrollingCurve.type(), orientation);
}


void QScrollerPrivate::createScrollingSegments(const QPointF &v,
                                               const QPointF &startPos,
                                               const QPointF &ppm)
{
    const QScrollerPropertiesPrivate *sp = properties.d.data();

    // This is only correct for QEasingCurve::OutQuad (linear velocity,
    // constant deceleration), but the results look and feel ok for OutExpo
    // and OutSine as well

    // v(t) = deltaTime * a * 0.5 * differentialForProgress(t / deltaTime)
    // v(0) = vrelease
    // v(deltaTime) = 0
    // deltaTime = (2 * vrelease) / (a * differntial(0))

    // pos(t) = integrate(v(t)dt)
    // pos(t) = vrelease * t - 0.5 * a * t * t
    // pos(t) = deltaTime * a * 0.5 * progress(t / deltaTime) * deltaTime
    // deltaPos = pos(deltaTime)

    QVector2D vel(v);
    qreal deltaTime = (qreal(2) * vel.length()) / (sp->decelerationFactor * differentialForProgress(sp->scrollingCurve, 0));
    QPointF deltaPos = (vel.normalized() * QVector2D(ppm)).toPointF() * deltaTime * deltaTime * qreal(0.5) * sp->decelerationFactor;

    createScrollingSegments(v.x(), startPos.x(), deltaTime, deltaPos.x(),
                            Qt::Horizontal);
    createScrollingSegments(v.y(), startPos.y(), deltaTime, deltaPos.y(),
                            Qt::Vertical);
}

/*! \internal
    Prepares scrolling by sending a QScrollPrepareEvent to the receiver widget.
    Returns \c true if the scrolling was accepted and a target was returned.
*/
bool QScrollerPrivate::prepareScrolling(const QPointF &position)
{
    QScrollPrepareEvent spe(position);
    spe.ignore();
    sendEvent(target, &spe);

    qScrollerDebug() << "QScrollPrepareEvent returned from" << target << "with" << spe.isAccepted() << "mcp:" << spe.contentPosRange() << "cp:" << spe.contentPos();
    if (spe.isAccepted()) {
        QPointF oldContentPos = contentPosition + overshootPosition;
        QPointF contentDelta = spe.contentPos() - oldContentPos;

        viewportSize = spe.viewportSize();
        contentPosRange = spe.contentPosRange();
        if (contentPosRange.width() < 0)
            contentPosRange.setWidth(0);
        if (contentPosRange.height() < 0)
            contentPosRange.setHeight(0);
        contentPosition = clampToRect(spe.contentPos(), contentPosRange);
        overshootPosition = spe.contentPos() - contentPosition;

        // - check if the content position was moved
        if (contentDelta != QPointF(0, 0)) {
            // need to correct all segments
            for (int i = 0; i < xSegments.count(); i++)
                xSegments[i].startPos -= contentDelta.x();

            for (int i = 0; i < ySegments.count(); i++)
                ySegments[i].startPos -= contentDelta.y();
        }

        if (QWidget *w = qobject_cast<QWidget *>(target))
            setDpiFromWidget(w);
#ifndef QT_NO_GRAPHICSVIEW
        if (QGraphicsObject *go = qobject_cast<QGraphicsObject *>(target)) {
            //TODO: the first view isn't really correct - maybe use an additional field in the prepare event?
            if (go->scene() && !go->scene()->views().isEmpty())
                setDpiFromWidget(go->scene()->views().first());
        }
#endif

        if (state == QScroller::Scrolling) {
            recalcScrollingSegments();
        }
        return true;
    }

    return false;
}

void QScrollerPrivate::handleDrag(const QPointF &position, qint64 timestamp)
{
    const QScrollerPropertiesPrivate *sp = properties.d.data();

    QPointF deltaPixel = position - lastPosition;
    qint64 deltaTime = timestamp - lastTimestamp;

    if (sp->axisLockThreshold) {
        int dx = qAbs(deltaPixel.x());
        int dy = qAbs(deltaPixel.y());
        if (dx || dy) {
            bool vertical = (dy > dx);
            qreal alpha = qreal(vertical ? dx : dy) / qreal(vertical ? dy : dx);
            //qScrollerDebug() << "QScroller::handleDrag() -- axis lock:" << alpha << " / " << axisLockThreshold << "- isvertical:" << vertical << "- dx:" << dx << "- dy:" << dy;
            if (alpha <= sp->axisLockThreshold) {
                if (vertical)
                    deltaPixel.setX(0);
                else
                    deltaPixel.setY(0);
            }
        }
    }

    // calculate velocity (if the user would release the mouse NOW)
    updateVelocity(deltaPixel, deltaTime);

    // restrict velocity, if content is not scrollable
    QRectF max = contentPosRange;
    bool canScrollX = (max.width() > 0) || (sp->hOvershootPolicy == QScrollerProperties::OvershootAlwaysOn);
    bool canScrollY = (max.height() > 0) || (sp->vOvershootPolicy == QScrollerProperties::OvershootAlwaysOn);

    if (!canScrollX) {
        deltaPixel.setX(0);
        releaseVelocity.setX(0);
    }
    if (!canScrollY) {
        deltaPixel.setY(0);
        releaseVelocity.setY(0);
    }

//    if (firstDrag) {
//        // Do not delay the first drag
//        setContentPositionHelper(q->contentPosition() - overshootDistance - deltaPixel);
//        dragDistance = QPointF(0, 0);
//    } else {
    dragDistance += deltaPixel;
//    }
//qScrollerDebug() << "######################" << deltaPixel << position.y() << lastPosition.y();

    lastPosition = position;
    lastTimestamp = timestamp;
}

bool QScrollerPrivate::pressWhileInactive(const QPointF &position, qint64 timestamp)
{
    if (prepareScrolling(position)) {
        const QScrollerPropertiesPrivate *sp = properties.d.data();

        if (!contentPosRange.isNull() ||
            (sp->hOvershootPolicy == QScrollerProperties::OvershootAlwaysOn) ||
            (sp->vOvershootPolicy == QScrollerProperties::OvershootAlwaysOn)) {

            lastPosition = pressPosition = position;
            lastTimestamp = pressTimestamp = timestamp;
            setState(QScroller::Pressed);
        }
    }
    return false;
}

bool QScrollerPrivate::releaseWhilePressed(const QPointF &, qint64)
{
    if (overshootPosition != QPointF(0.0, 0.0)) {
        setState(QScroller::Scrolling);
        return true;
    } else {
        setState(QScroller::Inactive);
        return false;
    }
}

bool QScrollerPrivate::moveWhilePressed(const QPointF &position, qint64 timestamp)
{
    Q_Q(QScroller);
    const QScrollerPropertiesPrivate *sp = properties.d.data();
    QPointF ppm = q->pixelPerMeter();

    QPointF deltaPixel = position - pressPosition;

    bool moveAborted = false;
    bool moveStarted = (((deltaPixel / ppm).manhattanLength()) > sp->dragStartDistance);

    // check the direction of the mouse drag and abort if it's too much in the wrong direction.
    if (moveStarted) {
        QRectF max = contentPosRange;
        bool canScrollX = (max.width() > 0);
        bool canScrollY = (max.height() > 0);

        if (sp->hOvershootPolicy == QScrollerProperties::OvershootAlwaysOn)
            canScrollX = true;
        if (sp->vOvershootPolicy == QScrollerProperties::OvershootAlwaysOn)
            canScrollY = true;

        if (qAbs(deltaPixel.x() / ppm.x()) < qAbs(deltaPixel.y() / ppm.y())) {
            if (!canScrollY)
                moveAborted = true;
        } else {
            if (!canScrollX)
                moveAborted = true;
        }
    }

    if (moveAborted) {
        setState(QScroller::Inactive);
        moveStarted = false;

    } else if (moveStarted) {
        setState(QScroller::Dragging);

        // subtract the dragStartDistance
        deltaPixel = deltaPixel - deltaPixel * (sp->dragStartDistance / deltaPixel.manhattanLength());

        if (deltaPixel != QPointF(0, 0)) {
            // handleDrag updates lastPosition, lastTimestamp and velocity
            handleDrag(pressPosition + deltaPixel, timestamp);
        }
    }
    return moveStarted;
}

bool QScrollerPrivate::moveWhileDragging(const QPointF &position, qint64 timestamp)
{
    // handleDrag updates lastPosition, lastTimestamp and velocity
    handleDrag(position, timestamp);
    return true;
}

void QScrollerPrivate::timerEventWhileDragging()
{
    if (dragDistance != QPointF(0, 0)) {
        qScrollerDebug() << "QScroller::timerEventWhileDragging() -- dragDistance:" << dragDistance;

        setContentPositionHelperDragging(-dragDistance);
        dragDistance = QPointF(0, 0);
    }
}

bool QScrollerPrivate::releaseWhileDragging(const QPointF &position, qint64 timestamp)
{
    Q_Q(QScroller);
    const QScrollerPropertiesPrivate *sp = properties.d.data();

    // handleDrag updates lastPosition, lastTimestamp and velocity
    handleDrag(position, timestamp);

    // check if we moved at all - this can happen if you stop a running
    // scroller with a press and release shortly afterwards
    QPointF deltaPixel = position - pressPosition;
    if (((deltaPixel / q->pixelPerMeter()).manhattanLength()) > sp->dragStartDistance) {

        // handle accelerating flicks
        if ((oldVelocity != QPointF(0, 0)) && sp->acceleratingFlickMaximumTime &&
            ((timestamp - pressTimestamp) < qint64(sp->acceleratingFlickMaximumTime * 1000))) {

            // - determine if the direction was changed
            int signX = 0, signY = 0;
            if (releaseVelocity.x())
                signX = (releaseVelocity.x() > 0) == (oldVelocity.x() > 0) ? 1 : -1;
            if (releaseVelocity.y())
                signY = (releaseVelocity.y() > 0) == (oldVelocity.y() > 0) ? 1 : -1;

            if (signX > 0)
                releaseVelocity.setX(qBound(-sp->maximumVelocity,
                                            oldVelocity.x() * sp->acceleratingFlickSpeedupFactor,
                                            sp->maximumVelocity));
            if (signY > 0)
                releaseVelocity.setY(qBound(-sp->maximumVelocity,
                                            oldVelocity.y() * sp->acceleratingFlickSpeedupFactor,
                                            sp->maximumVelocity));
        }
    }

    QPointF ppm = q->pixelPerMeter();
    createScrollingSegments(releaseVelocity, contentPosition + overshootPosition, ppm);

    qScrollerDebug() << "QScroller::releaseWhileDragging() -- velocity:" << releaseVelocity << "-- minimum velocity:" << sp->minimumVelocity << "overshoot" << overshootPosition;

    if (xSegments.isEmpty() && ySegments.isEmpty())
        setState(QScroller::Inactive);
    else
        setState(QScroller::Scrolling);

    return true;
}

void QScrollerPrivate::timerEventWhileScrolling()
{
    qScrollerDebug() << "QScroller::timerEventWhileScrolling()";

    setContentPositionHelperScrolling();
    if (xSegments.isEmpty() && ySegments.isEmpty())
        setState(QScroller::Inactive);
}

bool QScrollerPrivate::pressWhileScrolling(const QPointF &position, qint64 timestamp)
{
    Q_Q(QScroller);

    if ((q->velocity() <= properties.d->maximumClickThroughVelocity) &&
        (overshootPosition == QPointF(0.0, 0.0))) {
        setState(QScroller::Inactive);
        return false;
    } else {
        lastPosition = pressPosition = position;
        lastTimestamp = pressTimestamp = timestamp;
        setState(QScroller::Pressed);
        setState(QScroller::Dragging);
        return true;
    }
}

/*! \internal
    This function handles all state changes of the scroller.
*/
void QScrollerPrivate::setState(QScroller::State newstate)
{
    Q_Q(QScroller);
    bool sendLastScroll = false;

    if (state == newstate)
        return;

    qScrollerDebug() << q << "QScroller::setState(" << stateName(newstate) << ")";

    switch (newstate) {
    case QScroller::Inactive:
#ifndef QT_NO_ANIMATION
        scrollTimer->stop();
#endif

        // send the last scroll event (but only after the current state change was finished)
        if (!firstScroll)
            sendLastScroll = true;

        releaseVelocity = QPointF(0, 0);
        break;

    case QScroller::Pressed:
#ifndef QT_NO_ANIMATION
        scrollTimer->stop();
#endif

        oldVelocity = releaseVelocity;
        releaseVelocity = QPointF(0, 0);
        break;

    case QScroller::Dragging:
        dragDistance = QPointF(0, 0);
#ifndef QT_NO_ANIMATION
        if (state == QScroller::Pressed)
            scrollTimer->start();
#endif
        break;

    case QScroller::Scrolling:
#ifndef QT_NO_ANIMATION
        scrollTimer->start();
#endif
        break;
    }

    qSwap(state, newstate);

    if (sendLastScroll) {
        QScrollEvent se(contentPosition, overshootPosition, QScrollEvent::ScrollFinished);
        sendEvent(target, &se);
        firstScroll = true;
    }
    if (state == QScroller::Dragging || state == QScroller::Scrolling)
        qt_activeScrollers()->insert(q);
    else
        qt_activeScrollers()->remove(q);
    emit q->stateChanged(state);
}


/*! \internal
    Helps when setting the content position.
    It will try to move the content by the requested delta but stop in case
    when we are coming back from an overshoot or a scrollTo.
    It will also indicate a new overshooting condition by the overshootX and oversthootY flags.

    In this cases it will reset the velocity variables and other flags.

    Also keeps track of the current over-shooting value in overshootPosition.

    \a deltaPos is the amount of pixels the current content position should be moved
*/
void QScrollerPrivate::setContentPositionHelperDragging(const QPointF &deltaPos)
{
    const QScrollerPropertiesPrivate *sp = properties.d.data();

    if (sp->overshootDragResistanceFactor)
        overshootPosition /= sp->overshootDragResistanceFactor;

    QPointF oldPos = contentPosition + overshootPosition;
    QPointF newPos = oldPos + deltaPos;

    qScrollerDebug() << "QScroller::setContentPositionHelperDragging(" << deltaPos << " [pix])";
    qScrollerDebug() << "  --> overshoot:" << overshootPosition << "- old pos:" << oldPos << "- new pos:" << newPos;

    QPointF oldClampedPos = clampToRect(oldPos, contentPosRange);
    QPointF newClampedPos = clampToRect(newPos, contentPosRange);

    // --- handle overshooting and stop if the coordinate is going back inside the normal area
    bool alwaysOvershootX = (sp->hOvershootPolicy == QScrollerProperties::OvershootAlwaysOn);
    bool alwaysOvershootY = (sp->vOvershootPolicy == QScrollerProperties::OvershootAlwaysOn);
    bool noOvershootX = (sp->hOvershootPolicy == QScrollerProperties::OvershootAlwaysOff) ||
                        ((state == QScroller::Dragging) && !sp->overshootDragResistanceFactor) ||
                        !sp->overshootDragDistanceFactor;
    bool noOvershootY = (sp->vOvershootPolicy == QScrollerProperties::OvershootAlwaysOff) ||
                        ((state == QScroller::Dragging) && !sp->overshootDragResistanceFactor) ||
                        !sp->overshootDragDistanceFactor;
    bool canOvershootX = !noOvershootX && (alwaysOvershootX || contentPosRange.width());
    bool canOvershootY = !noOvershootY && (alwaysOvershootY || contentPosRange.height());

    qreal oldOvershootX = (canOvershootX) ? oldPos.x() - oldClampedPos.x() : 0;
    qreal oldOvershootY = (canOvershootY) ? oldPos.y() - oldClampedPos.y() : 0;

    qreal newOvershootX = (canOvershootX) ? newPos.x() - newClampedPos.x() : 0;
    qreal newOvershootY = (canOvershootY) ? newPos.y() - newClampedPos.y() : 0;

    qreal maxOvershootX = viewportSize.width() * sp->overshootDragDistanceFactor;
    qreal maxOvershootY = viewportSize.height() * sp->overshootDragDistanceFactor;

    qScrollerDebug() << "  --> noOs:" << noOvershootX << "drf:" << sp->overshootDragResistanceFactor << "mdf:" << sp->overshootScrollDistanceFactor << "ossP:"<<sp->hOvershootPolicy;
    qScrollerDebug() << "  --> canOS:" << canOvershootX << "newOS:" << newOvershootX << "maxOS:" << maxOvershootX;

    if (sp->overshootDragResistanceFactor) {
        oldOvershootX *= sp->overshootDragResistanceFactor;
        oldOvershootY *= sp->overshootDragResistanceFactor;
        newOvershootX *= sp->overshootDragResistanceFactor;
        newOvershootY *= sp->overshootDragResistanceFactor;
    }

    // -- stop at the maximum overshoot distance

    newOvershootX = qBound(-maxOvershootX, newOvershootX, maxOvershootX);
    newOvershootY = qBound(-maxOvershootY, newOvershootY, maxOvershootY);

    overshootPosition.setX(newOvershootX);
    overshootPosition.setY(newOvershootY);
    contentPosition = newClampedPos;

    QScrollEvent se(contentPosition, overshootPosition, firstScroll ? QScrollEvent::ScrollStarted : QScrollEvent::ScrollUpdated);
    sendEvent(target, &se);
    firstScroll = false;

    qScrollerDebug() << "  --> new position:" << newClampedPos << "- new overshoot:" << overshootPosition <<
                        "- overshoot x/y?:" << overshootPosition;
}


qreal QScrollerPrivate::nextSegmentPosition(QQueue<ScrollSegment> &segments, qint64 now, qreal oldPos)
{
    qreal pos = oldPos;

    // check the X segments for new positions
    while (!segments.isEmpty()) {
        const ScrollSegment s = segments.head();

        if ((s.startTime + s.deltaTime * s.stopProgress) <= now) {
            segments.dequeue();
            pos = s.stopPos;
        } else if (s.startTime <= now) {
            qreal progress = qreal(now - s.startTime) / qreal(s.deltaTime);
            pos = s.startPos + s.deltaPos * s.curve.valueForProgress(progress);
            if (s.deltaPos > 0 ? pos > s.stopPos : pos < s.stopPos) {
                segments.dequeue();
                pos = s.stopPos;
            } else {
                break;
            }
        } else {
            break;
        }
    }
    return pos;
}

void QScrollerPrivate::setContentPositionHelperScrolling()
{
    qint64 now = monotonicTimer.elapsed();
    QPointF newPos = contentPosition + overshootPosition;

    newPos.setX(nextSegmentPosition(xSegments, now, newPos.x()));
    newPos.setY(nextSegmentPosition(ySegments, now, newPos.y()));

    // -- set the position and handle overshoot
    qScrollerDebug() << "QScroller::setContentPositionHelperScrolling()";
    qScrollerDebug() << "  --> overshoot:" << overshootPosition << "- new pos:" << newPos;

    QPointF newClampedPos = clampToRect(newPos, contentPosRange);

    overshootPosition = newPos - newClampedPos;
    contentPosition = newClampedPos;

    QScrollEvent se(contentPosition, overshootPosition, firstScroll ? QScrollEvent::ScrollStarted : QScrollEvent::ScrollUpdated);
    sendEvent(target, &se);
    firstScroll = false;

    qScrollerDebug() << "  --> new position:" << newClampedPos << "- new overshoot:" << overshootPosition;
}

/*! \internal
    Returns the next snap point in direction.
    If \a direction >0 it will return the next snap point that is larger than the current position.
    If \a direction <0 it will return the next snap point that is smaller than the current position.
    If \a direction ==0 it will return the nearest snap point (or the current position if we are already
    on a snap point.
    Returns the nearest snap position or NaN if no such point could be found.
 */
qreal QScrollerPrivate::nextSnapPos(qreal p, int dir, Qt::Orientation orientation)
{
    qreal bestSnapPos = Q_QNAN;
    qreal bestSnapPosDist = Q_INFINITY;

    qreal minPos;
    qreal maxPos;

    if (orientation == Qt::Horizontal) {
        minPos = contentPosRange.left();
        maxPos = contentPosRange.right();
    } else {
        minPos = contentPosRange.top();
        maxPos = contentPosRange.bottom();
    }

    if (orientation == Qt::Horizontal) {
        // the snap points in the list
        foreach (qreal snapPos, snapPositionsX) {
            qreal snapPosDist = snapPos - p;
            if ((dir > 0 && snapPosDist < 0) ||
                (dir < 0 && snapPosDist > 0))
                continue; // wrong direction
            if (snapPos < minPos || snapPos > maxPos )
                continue; // invalid

            if (qIsNaN(bestSnapPos) ||
                qAbs(snapPosDist) < bestSnapPosDist ) {
                bestSnapPos = snapPos;
                bestSnapPosDist = qAbs(snapPosDist);
            }
        }

        // the snap point interval
        if (snapIntervalX > 0.0) {
            qreal first = minPos + snapFirstX;
            qreal snapPos;
            if (dir > 0)
                snapPos = qCeil((p - first) / snapIntervalX) * snapIntervalX + first;
            else if (dir < 0)
                snapPos = qFloor((p - first) / snapIntervalX) * snapIntervalX + first;
            else if (p <= first)
                snapPos = first;
            else
            {
                qreal last = qFloor((maxPos - first) / snapIntervalX) * snapIntervalX + first;
                if (p >= last)
                    snapPos = last;
                else
                    snapPos = qRound((p - first) / snapIntervalX) * snapIntervalX + first;
            }

            if (snapPos >= first && snapPos <= maxPos ) {
                qreal snapPosDist = snapPos - p;

                if (qIsNaN(bestSnapPos) ||
                    qAbs(snapPosDist) < bestSnapPosDist ) {
                    bestSnapPos = snapPos;
                    bestSnapPosDist = qAbs(snapPosDist);
                }
            }
        }

    } else { // (orientation == Qt::Vertical)
        // the snap points in the list
        foreach (qreal snapPos, snapPositionsY) {
            qreal snapPosDist = snapPos - p;
            if ((dir > 0 && snapPosDist < 0) ||
                (dir < 0 && snapPosDist > 0))
                continue; // wrong direction
            if (snapPos < minPos || snapPos > maxPos )
                continue; // invalid

            if (qIsNaN(bestSnapPos) ||
                qAbs(snapPosDist) < bestSnapPosDist) {
                bestSnapPos = snapPos;
                bestSnapPosDist = qAbs(snapPosDist);
            }
        }

        // the snap point interval
        if (snapIntervalY > 0.0) {
            qreal first = minPos + snapFirstY;
            qreal snapPos;
            if (dir > 0)
                snapPos = qCeil((p - first) / snapIntervalY) * snapIntervalY + first;
            else if (dir < 0)
                snapPos = qFloor((p - first) / snapIntervalY) * snapIntervalY + first;
            else if (p <= first)
                snapPos = first;
            else
            {
                qreal last = qFloor((maxPos - first) / snapIntervalY) * snapIntervalY + first;
                if (p >= last)
                    snapPos = last;
                else
                    snapPos = qRound((p - first) / snapIntervalY) * snapIntervalY + first;
            }

            if (snapPos >= first && snapPos <= maxPos ) {
                qreal snapPosDist = snapPos - p;

                if (qIsNaN(bestSnapPos) ||
                    qAbs(snapPosDist) < bestSnapPosDist) {
                    bestSnapPos = snapPos;
                    bestSnapPosDist = qAbs(snapPosDist);
                }
            }
        }
    }

    return bestSnapPos;
}

/*!
    \enum QScroller::State

    This enum contains the different QScroller states.

    \value Inactive The scroller is not scrolling and nothing is pressed.
    \value Pressed A touch event was received or the mouse button was pressed but the scroll area is currently not dragged.
    \value Dragging The scroll area is currently following the touch point or mouse.
    \value Scrolling The scroll area is moving on it's own.
*/

/*!
    \enum QScroller::ScrollerGestureType

    This enum contains the different gesture types that are supported by the QScroller gesture recognizer.

    \value TouchGesture The gesture recognizer will only trigger on touch
        events.  Specifically it will react on single touch points when using a
        touch screen and dual touch points when using a touchpad.
    \value LeftMouseButtonGesture The gesture recognizer will only trigger on left mouse button events.
    \value MiddleMouseButtonGesture The gesture recognizer will only trigger on middle mouse button events.
    \value RightMouseButtonGesture The gesture recognizer will only trigger on right mouse button events.
*/

/*!
    \enum QScroller::Input

    This enum contains an input device agnostic view of input events that are relevant for QScroller.

    \value InputPress   The user pressed the input device (e.g.  QEvent::MouseButtonPress,
                        QEvent::GraphicsSceneMousePress, QEvent::TouchBegin)

    \value InputMove    The user moved the input device (e.g.  QEvent::MouseMove,
                        QEvent::GraphicsSceneMouseMove, QEvent::TouchUpdate)

    \value InputRelease The user released the input device (e.g.  QEvent::MouseButtonRelease,
                        QEvent::GraphicsSceneMouseRelease, QEvent::TouchEnd)

*/

QT_END_NAMESPACE
