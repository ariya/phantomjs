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

#include <QPointer>
#include <QObject>
#include <QtCore/qmath.h>
#ifdef Q_WS_WIN
#  include <QLibrary>
#endif

#include "qscrollerproperties.h"
#include "private/qscrollerproperties_p.h"

QT_BEGIN_NAMESPACE

static QScrollerPropertiesPrivate *userDefaults = 0;
static QScrollerPropertiesPrivate *systemDefaults = 0;

QScrollerPropertiesPrivate *QScrollerPropertiesPrivate::defaults()
{
    if (!systemDefaults) {
        QScrollerPropertiesPrivate spp;
        spp.mousePressEventDelay = qreal(0.25);
        spp.dragStartDistance = qreal(5.0 / 1000);
        spp.dragVelocitySmoothingFactor = qreal(0.8);
        spp.axisLockThreshold = qreal(0);
        spp.scrollingCurve.setType(QEasingCurve::OutQuad);
        spp.decelerationFactor = qreal(0.125);
        spp.minimumVelocity = qreal(50.0 / 1000);
        spp.maximumVelocity = qreal(500.0 / 1000);
        spp.maximumClickThroughVelocity = qreal(66.5 / 1000);
        spp.acceleratingFlickMaximumTime = qreal(1.25);
        spp.acceleratingFlickSpeedupFactor = qreal(3.0);
        spp.snapPositionRatio = qreal(0.5);
        spp.snapTime = qreal(0.3);
        spp.overshootDragResistanceFactor = qreal(0.5);
        spp.overshootDragDistanceFactor = qreal(1);
        spp.overshootScrollDistanceFactor = qreal(0.5);
        spp.overshootScrollTime = qreal(0.7);
#  ifdef Q_WS_WIN
        if (QLibrary::resolve(QLatin1String("UxTheme"), "BeginPanningFeedback"))
            spp.overshootScrollTime = qreal(0.35);
#  endif
        spp.hOvershootPolicy = QScrollerProperties::OvershootWhenScrollable;
        spp.vOvershootPolicy = QScrollerProperties::OvershootWhenScrollable;
        spp.frameRate = QScrollerProperties::Standard;

        systemDefaults = new QScrollerPropertiesPrivate(spp);
    }
    return new QScrollerPropertiesPrivate(userDefaults ? *userDefaults : *systemDefaults);
}

/*!
    \class QScrollerProperties
    \brief The QScrollerProperties class stores the settings for a QScroller.
    \since 4.8

    \inmodule QtWidgets

    The QScrollerProperties class stores the parameters used by QScroller.

    The default settings are platform dependent so that Qt emulates the
    platform behaviour for kinetic scrolling.

    As a convention the QScrollerProperties are in physical units (meter,
    seconds) and are converted by QScroller using the current DPI.

    \sa QScroller
*/

/*!
    Constructs new scroller properties.
*/
QScrollerProperties::QScrollerProperties()
    : d(QScrollerPropertiesPrivate::defaults())
{
}

/*!
    Constructs a copy of \a sp.
*/
QScrollerProperties::QScrollerProperties(const QScrollerProperties &sp)
    : d(new QScrollerPropertiesPrivate(*sp.d))
{
}

/*!
    Assigns \a sp to these scroller properties and returns a reference to these scroller properties.
*/
QScrollerProperties &QScrollerProperties::operator=(const QScrollerProperties &sp)
{
    *d.data() = *sp.d.data();
    return *this;
}

/*!
    Destroys the scroller properties.
*/
QScrollerProperties::~QScrollerProperties()
{
}

/*!
    Returns \c true if these scroller properties are equal to \a sp; otherwise returns \c false.
*/
bool QScrollerProperties::operator==(const QScrollerProperties &sp) const
{
    return *d.data() == *sp.d.data();
}

/*!
    Returns \c true if these scroller properties are different from \a sp; otherwise returns \c false.
*/
bool QScrollerProperties::operator!=(const QScrollerProperties &sp) const
{
    return !(*d.data() == *sp.d.data());
}

bool QScrollerPropertiesPrivate::operator==(const QScrollerPropertiesPrivate &p) const
{
    bool same = true;
    same &= (mousePressEventDelay == p.mousePressEventDelay);
    same &= (dragStartDistance == p.dragStartDistance);
    same &= (dragVelocitySmoothingFactor == p.dragVelocitySmoothingFactor);
    same &= (axisLockThreshold == p.axisLockThreshold);
    same &= (scrollingCurve == p.scrollingCurve);
    same &= (decelerationFactor == p.decelerationFactor);
    same &= (minimumVelocity == p.minimumVelocity);
    same &= (maximumVelocity == p.maximumVelocity);
    same &= (maximumClickThroughVelocity == p.maximumClickThroughVelocity);
    same &= (acceleratingFlickMaximumTime == p.acceleratingFlickMaximumTime);
    same &= (acceleratingFlickSpeedupFactor == p.acceleratingFlickSpeedupFactor);
    same &= (snapPositionRatio == p.snapPositionRatio);
    same &= (snapTime == p.snapTime);
    same &= (overshootDragResistanceFactor == p.overshootDragResistanceFactor);
    same &= (overshootDragDistanceFactor == p.overshootDragDistanceFactor);
    same &= (overshootScrollDistanceFactor == p.overshootScrollDistanceFactor);
    same &= (overshootScrollTime == p.overshootScrollTime);
    same &= (hOvershootPolicy == p.hOvershootPolicy);
    same &= (vOvershootPolicy == p.vOvershootPolicy);
    same &= (frameRate == p.frameRate);
    return same;
}

/*!
     Sets the scroller properties for all new QScrollerProperties objects to \a sp.

     Use this function to override the platform default properties returned by the default
     constructor. If you only want to change the scroller properties of a single scroller, use
     QScroller::setScrollerProperties()

     \note Calling this function will not change the content of already existing
     QScrollerProperties objects.

     \sa unsetDefaultScrollerProperties()
*/
void QScrollerProperties::setDefaultScrollerProperties(const QScrollerProperties &sp)
{
    if (!userDefaults)
        userDefaults = new QScrollerPropertiesPrivate(*sp.d);
    else
        *userDefaults = *sp.d;
}

/*!
     Sets the scroller properties returned by the default constructor back to the platform default
     properties.

     \sa setDefaultScrollerProperties()
*/
void QScrollerProperties::unsetDefaultScrollerProperties()
{
    delete userDefaults;
    userDefaults = 0;
}

/*!
    Query the \a metric value of the scroller properties.

    \sa setScrollMetric(), ScrollMetric
*/
QVariant QScrollerProperties::scrollMetric(ScrollMetric metric) const
{
    switch (metric) {
    case MousePressEventDelay:          return d->mousePressEventDelay;
    case DragStartDistance:             return d->dragStartDistance;
    case DragVelocitySmoothingFactor:   return d->dragVelocitySmoothingFactor;
    case AxisLockThreshold:             return d->axisLockThreshold;
    case ScrollingCurve:                return d->scrollingCurve;
    case DecelerationFactor:            return d->decelerationFactor;
    case MinimumVelocity:               return d->minimumVelocity;
    case MaximumVelocity:               return d->maximumVelocity;
    case MaximumClickThroughVelocity:   return d->maximumClickThroughVelocity;
    case AcceleratingFlickMaximumTime:  return d->acceleratingFlickMaximumTime;
    case AcceleratingFlickSpeedupFactor:return d->acceleratingFlickSpeedupFactor;
    case SnapPositionRatio:             return d->snapPositionRatio;
    case SnapTime:                      return d->snapTime;
    case OvershootDragResistanceFactor: return d->overshootDragResistanceFactor;
    case OvershootDragDistanceFactor:   return d->overshootDragDistanceFactor;
    case OvershootScrollDistanceFactor: return d->overshootScrollDistanceFactor;
    case OvershootScrollTime:           return d->overshootScrollTime;
    case HorizontalOvershootPolicy:     return QVariant::fromValue(d->hOvershootPolicy);
    case VerticalOvershootPolicy:       return QVariant::fromValue(d->vOvershootPolicy);
    case FrameRate:                     return QVariant::fromValue(d->frameRate);
    case ScrollMetricCount:             break;
    }
    return QVariant();
}

/*!
    Set a specific value of the \a metric ScrollerMetric to \a value.

    \sa scrollMetric(), ScrollMetric
*/
void QScrollerProperties::setScrollMetric(ScrollMetric metric, const QVariant &value)
{
    switch (metric) {
    case MousePressEventDelay:          d->mousePressEventDelay = value.toReal(); break;
    case DragStartDistance:             d->dragStartDistance = value.toReal(); break;
    case DragVelocitySmoothingFactor:   d->dragVelocitySmoothingFactor = qBound(qreal(0), value.toReal(), qreal(1)); break;
    case AxisLockThreshold:             d->axisLockThreshold = qBound(qreal(0), value.toReal(), qreal(1)); break;
    case ScrollingCurve:                d->scrollingCurve = value.toEasingCurve(); break;
    case DecelerationFactor:            d->decelerationFactor = value.toReal(); break;
    case MinimumVelocity:               d->minimumVelocity = value.toReal(); break;
    case MaximumVelocity:               d->maximumVelocity = value.toReal(); break;
    case MaximumClickThroughVelocity:   d->maximumClickThroughVelocity = value.toReal(); break;
    case AcceleratingFlickMaximumTime:  d->acceleratingFlickMaximumTime = value.toReal(); break;
    case AcceleratingFlickSpeedupFactor:d->acceleratingFlickSpeedupFactor = value.toReal(); break;
    case SnapPositionRatio:             d->snapPositionRatio = qBound(qreal(0), value.toReal(), qreal(1)); break;
    case SnapTime:                      d->snapTime = value.toReal(); break;
    case OvershootDragResistanceFactor: d->overshootDragResistanceFactor = value.toReal(); break;
    case OvershootDragDistanceFactor:   d->overshootDragDistanceFactor = qBound(qreal(0), value.toReal(), qreal(1)); break;
    case OvershootScrollDistanceFactor: d->overshootScrollDistanceFactor = qBound(qreal(0), value.toReal(), qreal(1)); break;
    case OvershootScrollTime:           d->overshootScrollTime = value.toReal(); break;
    case HorizontalOvershootPolicy:     d->hOvershootPolicy = value.value<QScrollerProperties::OvershootPolicy>(); break;
    case VerticalOvershootPolicy:       d->vOvershootPolicy = value.value<QScrollerProperties::OvershootPolicy>(); break;
    case FrameRate:                     d->frameRate = value.value<QScrollerProperties::FrameRates>(); break;
    case ScrollMetricCount:             break;
    }
}

/*!
    \enum QScrollerProperties::FrameRates

    This enum describes the available frame rates used while dragging or scrolling.

    \value Fps60 60 frames per second
    \value Fps30 30 frames per second
    \value Fps20 20 frames per second
    \value Standard the default value is 60 frames per second (which corresponds to QAbstractAnimation).
*/

/*!
    \enum QScrollerProperties::OvershootPolicy

    This enum describes the various modes of overshooting.

    \value OvershootWhenScrollable Overshooting is possible when the content is scrollable. This is the
                                   default.

    \value OvershootAlwaysOff Overshooting is never enabled, even when the content is scrollable.

    \value OvershootAlwaysOn Overshooting is always enabled, even when the content is not
                             scrollable.
*/

/*!
    \enum QScrollerProperties::ScrollMetric

    This enum contains the different scroll metric types. When not indicated otherwise the
    setScrollMetric function expects a QVariant of type qreal.

    See the QScroller documentation for further details of the concepts behind the different
    values.

    \value MousePressEventDelay This is the time a mouse press event is delayed when starting
    a flick gesture in \c{[s]}. If the gesture is triggered within that time, no mouse press or
    release is sent to the scrolled object. If it triggers after that delay the delayed
    mouse press plus a faked release event at global position \c{QPoint(-QWIDGETSIZE_MAX,
    -QWIDGETSIZE_MAX)} is sent. If the gesture is canceled, then both the delayed mouse
    press plus the real release event are delivered.

    \value DragStartDistance This is the minimum distance the touch or mouse point needs to be
    moved before the flick gesture is triggered in \c m.

    \value DragVelocitySmoothingFactor A value that describes to which extent new drag velocities are
    included in the final scrolling velocity.  This value should be in the range between \c 0 and
    \c 1.  The lower the value, the more smoothing is applied to the dragging velocity.

    \value AxisLockThreshold Restricts the movement to one axis if the movement is inside an angle
    around the axis. The threshold must be in the range \c 0 to \c 1.

    \value ScrollingCurve The QEasingCurve used when decelerating the scrolling velocity after an
    user initiated flick. Please note that this is the easing curve for the positions, \b{not}
    the velocity: the default is QEasingCurve::OutQuad, which results in a linear decrease in
    velocity (1st derivative) and a constant deceleration (2nd derivative).

    \value DecelerationFactor This factor influences how long it takes the scroller to decelerate
    to 0 velocity. The actual value depends on the chosen ScrollingCurve. For most
    types the value should be in the range from \c 0.1 to \c 2.0

    \value MinimumVelocity The minimum velocity that is needed after ending the touch or releasing
    the mouse to start scrolling in \c{m/s}.

    \value MaximumVelocity This is the maximum velocity that can be reached in \c{m/s}.

    \value MaximumClickThroughVelocity This is the maximum allowed scroll speed for a click-through
    in \c{m/s}. This means that a click on a currently (slowly) scrolling object will not only stop
    the scrolling but the click event will also be delivered to the UI control. This is
    useful when using exponential-type scrolling curves.

    \value AcceleratingFlickMaximumTime This is the maximum time in \c seconds that a flick gesture
    can take to be recognized as an accelerating flick. If set to zero no such gesture is
    detected. An "accelerating flick" is a flick gesture executed on an already scrolling object.
    In such cases the scrolling speed is multiplied by AcceleratingFlickSpeedupFactor in order to
    accelerate it.

    \value AcceleratingFlickSpeedupFactor The current speed is multiplied by this number if an
    accelerating flick is detected. Should be \c{>= 1}.

    \value SnapPositionRatio This is the distance that the user must drag the area beween two snap
    points in order to snap it to the next position. \c{0.33} means that the scroll must only
    reach one third of the distance between two snap points to snap to the next one. The ratio must
    be between \c 0 and \c 1.

    \value SnapTime This is the time factor for the scrolling curve. A lower value means that the
    scrolling will take longer. The scrolling distance is independet of this value.

    \value OvershootDragResistanceFactor This value is the factor between the mouse dragging and
    the actual scroll area movement (during overshoot). The factor must be between \c 0 and \c 1.

    \value OvershootDragDistanceFactor This is the maximum distance for overshoot movements while
    dragging. The actual overshoot distance is calculated by multiplying this value with the
    viewport size of the scrolled object. The factor must be between \c 0 and \c 1.

    \value OvershootScrollDistanceFactor This is the maximum distance for overshoot movements while
    scrolling. The actual overshoot distance is calculated by multiplying this value with the
    viewport size of the scrolled object. The factor must be between \c 0 and \c 1.

    \value OvershootScrollTime This is the time in \c seconds that is used to play the
    complete overshoot animation.

    \value HorizontalOvershootPolicy This is the horizontal overshooting policy (see OvershootPolicy).

    \value VerticalOvershootPolicy This is the horizontal overshooting policy (see OvershootPolicy).

    \value FrameRate This is the frame rate which should be used while dragging or scrolling.
    QScroller uses a QAbstractAnimation timer internally to sync all scrolling operations to other
    animations that might be active at the same time.  If the standard value of 60 frames per
    second is too fast, it can be lowered with this setting,
    while still being in-sync with QAbstractAnimation. Please note that only the values of the
    FrameRates enum are allowed here.

    \value ScrollMetricCount This is always the last entry.
*/

QT_END_NAMESPACE
