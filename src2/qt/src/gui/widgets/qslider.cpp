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

#include "qslider.h"
#ifndef QT_NO_SLIDER
#ifndef QT_NO_ACCESSIBILITY
#include "qaccessible.h"
#endif
#include "qapplication.h"
#include "qevent.h"
#include "qpainter.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "private/qabstractslider_p.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

class QSliderPrivate : public QAbstractSliderPrivate
{
    Q_DECLARE_PUBLIC(QSlider)
public:
    QStyle::SubControl pressedControl;
    int tickInterval;
    QSlider::TickPosition tickPosition;
    int clickOffset;
    void init();
	void resetLayoutItemMargins();
    int pixelPosToRangeValue(int pos) const;
    inline int pick(const QPoint &pt) const;

    QStyle::SubControl newHoverControl(const QPoint &pos);
    bool updateHoverControl(const QPoint &pos);
    QStyle::SubControl hoverControl;
    QRect hoverRect;
};

void QSliderPrivate::init()
{
    Q_Q(QSlider);
    pressedControl = QStyle::SC_None;
    tickInterval = 0;
    tickPosition = QSlider::NoTicks;
    hoverControl = QStyle::SC_None;
    q->setFocusPolicy(Qt::FocusPolicy(q->style()->styleHint(QStyle::SH_Button_FocusPolicy)));
    QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Fixed, QSizePolicy::Slider);
    if (orientation == Qt::Vertical)
        sp.transpose();
    q->setSizePolicy(sp);
    q->setAttribute(Qt::WA_WState_OwnSizePolicy, false);
	resetLayoutItemMargins();
}

void QSliderPrivate::resetLayoutItemMargins()
{
    Q_Q(QSlider);
    QStyleOptionSlider opt;
    q->initStyleOption(&opt);
    setLayoutItemMargins(QStyle::SE_SliderLayoutItem, &opt);
}

int QSliderPrivate::pixelPosToRangeValue(int pos) const
{
    Q_Q(const QSlider);
    QStyleOptionSlider opt;
    q->initStyleOption(&opt);
    QRect gr = q->style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, q);
    QRect sr = q->style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, q);
    int sliderMin, sliderMax, sliderLength;

    if (orientation == Qt::Horizontal) {
        sliderLength = sr.width();
        sliderMin = gr.x();
        sliderMax = gr.right() - sliderLength + 1;
    } else {
        sliderLength = sr.height();
        sliderMin = gr.y();
        sliderMax = gr.bottom() - sliderLength + 1;
    }
    return QStyle::sliderValueFromPosition(minimum, maximum, pos - sliderMin,
                                           sliderMax - sliderMin, opt.upsideDown);
}

inline int QSliderPrivate::pick(const QPoint &pt) const
{
    return orientation == Qt::Horizontal ? pt.x() : pt.y();
}

/*!
    Initialize \a option with the values from this QSlider. This method
    is useful for subclasses when they need a QStyleOptionSlider, but don't want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QSlider::initStyleOption(QStyleOptionSlider *option) const
{
    if (!option)
        return;

    Q_D(const QSlider);
    option->initFrom(this);
    option->subControls = QStyle::SC_None;
    option->activeSubControls = QStyle::SC_None;
    option->orientation = d->orientation;
    option->maximum = d->maximum;
    option->minimum = d->minimum;
    option->tickPosition = (QSlider::TickPosition)d->tickPosition;
    option->tickInterval = d->tickInterval;
    option->upsideDown = (d->orientation == Qt::Horizontal) ?
                     (d->invertedAppearance != (option->direction == Qt::RightToLeft))
                     : (!d->invertedAppearance);
    option->direction = Qt::LeftToRight; // we use the upsideDown option instead
    option->sliderPosition = d->position;
    option->sliderValue = d->value;
    option->singleStep = d->singleStep;
    option->pageStep = d->pageStep;
    if (d->orientation == Qt::Horizontal)
        option->state |= QStyle::State_Horizontal;
}

bool QSliderPrivate::updateHoverControl(const QPoint &pos)
{
    Q_Q(QSlider);
    QRect lastHoverRect = hoverRect;
    QStyle::SubControl lastHoverControl = hoverControl;
    bool doesHover = q->testAttribute(Qt::WA_Hover);
    if (lastHoverControl != newHoverControl(pos) && doesHover) {
        q->update(lastHoverRect);
        q->update(hoverRect);
        return true;
    }
    return !doesHover;
}

QStyle::SubControl QSliderPrivate::newHoverControl(const QPoint &pos)
{
    Q_Q(QSlider);
    QStyleOptionSlider opt;
    q->initStyleOption(&opt);
    opt.subControls = QStyle::SC_All;
    QRect handleRect = q->style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, q);
    QRect grooveRect = q->style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, q);
    QRect tickmarksRect = q->style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderTickmarks, q);

    if (handleRect.contains(pos)) {
        hoverRect = handleRect;
        hoverControl = QStyle::SC_SliderHandle;
    } else if (grooveRect.contains(pos)) {
        hoverRect = grooveRect;
        hoverControl = QStyle::SC_SliderGroove;
    } else if (tickmarksRect.contains(pos)) {
        hoverRect = tickmarksRect;
        hoverControl = QStyle::SC_SliderTickmarks;
    } else {
        hoverRect = QRect();
        hoverControl = QStyle::SC_None;
    }

    return hoverControl;
}

/*!
    \class QSlider
    \brief The QSlider widget provides a vertical or horizontal slider.

    \ingroup basicwidgets


    The slider is the classic widget for controlling a bounded value.
    It lets the user move a slider handle along a horizontal or vertical
    groove and translates the handle's position into an integer value
    within the legal range.

    QSlider has very few of its own functions; most of the functionality is in
    QAbstractSlider. The most useful functions are setValue() to set
    the slider directly to some value; triggerAction() to simulate
    the effects of clicking (useful for shortcut keys);
    setSingleStep(), setPageStep() to set the steps; and setMinimum()
    and setMaximum() to define the range of the scroll bar.

    QSlider provides methods for controlling tickmarks.  You can use
    setTickPosition() to indicate where you want the tickmarks to be,
    setTickInterval() to indicate how many of them you want. the
    currently set tick position and interval can be queried using the
    tickPosition() and tickInterval() functions, respectively.

    QSlider inherits a comprehensive set of signals:
    \table
    \header \o Signal \o Description
    \row \o \l valueChanged()
    \o Emitted when the slider's value has changed. The tracking()
       determines whether this signal is emitted during user
       interaction.
    \row \o \l sliderPressed()
    \o Emitted when the user starts to drag the slider.
    \row \o \l sliderMoved()
    \o Emitted when the user drags the slider.
    \row \o \l sliderReleased()
    \o Emitted when the user releases the slider.
    \endtable

    QSlider only provides integer ranges. Note that although
    QSlider handles very large numbers, it becomes difficult for users
    to use a slider accurately for very large ranges.

    A slider accepts focus on Tab and provides both a mouse wheel and a
    keyboard interface. The keyboard interface is the following:

    \list
        \o Left/Right move a horizontal slider by one single step.
        \o Up/Down move a vertical slider by one single step.
        \o PageUp moves up one page.
        \o PageDown moves down one page.
        \o Home moves to the start (mininum).
        \o End moves to the end (maximum).
    \endlist

    \table 100%
    \row \o \inlineimage macintosh-slider.png Screenshot of a Macintosh slider
         \o A slider shown in the \l{Macintosh Style Widget Gallery}{Macintosh widget style}.
    \row \o \inlineimage windows-slider.png Screenshot of a Windows XP slider
         \o A slider shown in the \l{Windows XP Style Widget Gallery}{Windows XP widget style}.
    \row \o \inlineimage plastique-slider.png Screenshot of a Plastique slider
         \o A slider shown in the \l{Plastique Style Widget Gallery}{Plastique widget style}.
    \endtable

    \sa QScrollBar, QSpinBox, QDial, {fowler}{GUI Design Handbook: Slider}, {Sliders Example}
*/


/*!
    \enum QSlider::TickPosition

    This enum specifies where the tick marks are to be drawn relative
    to the slider's groove and the handle the user moves.

    \value NoTicks Do not draw any tick marks.
    \value TicksBothSides Draw tick marks on both sides of the groove.
    \value TicksAbove Draw tick marks above the (horizontal) slider
    \value TicksBelow Draw tick marks below the (horizontal) slider
    \value TicksLeft Draw tick marks to the left of the (vertical) slider
    \value TicksRight Draw tick marks to the right of the (vertical) slider

    \omitvalue NoMarks
    \omitvalue Above
    \omitvalue Left
    \omitvalue Below
    \omitvalue Right
    \omitvalue Both
*/


/*!
    Constructs a vertical slider with the given \a parent.
*/
QSlider::QSlider(QWidget *parent)
    : QAbstractSlider(*new QSliderPrivate, parent)
{
    d_func()->orientation = Qt::Vertical;
    d_func()->init();
}

/*!
    Constructs a slider with the given \a parent. The \a orientation
    parameter determines whether the slider is horizontal or vertical;
    the valid values are Qt::Vertical and Qt::Horizontal.
*/

QSlider::QSlider(Qt::Orientation orientation, QWidget *parent)
    : QAbstractSlider(*new QSliderPrivate, parent)
{
    d_func()->orientation = orientation;
    d_func()->init();
}

#ifdef QT3_SUPPORT
/*!
    Use QSlider() and QObject::setObjectName() instead.

    \oldcode
        QSlider *mySlider = new QSlider(parent, name);
    \newcode
        QSlider *mySlider = new QSlider(parent);
        mySlider->setObjectName(name);
    \endcode
*/
QSlider::QSlider(QWidget *parent, const char *name)
    : QAbstractSlider(*new QSliderPrivate, parent)
{
    setObjectName(QString::fromAscii(name));
    d_func()->orientation = Qt::Vertical;
    d_func()->init();
}

/*!
    Use QSlider() and QObject::setObjectName() instead.

    \oldcode
        QSlider *mySlider = new QSlider(orientation, parent, name);
    \newcode
        QSlider *mySlider = new QSlider(orientation, parent);
        mySlider->setObjectName(name);
    \endcode
*/
QSlider::QSlider(Qt::Orientation orientation, QWidget *parent, const char *name)
    : QAbstractSlider(*new QSliderPrivate, parent)
{
    setObjectName(QString::fromAscii(name));
    d_func()->orientation = orientation;
    d_func()->init();
}

/*!
    Use QSlider(), QObject::setObjectName() and the functionality
    inherited from QAbstractSlider instead.

    \oldcode
        QSlider *mySlider = new QSlider(minValue, maxValue, pageStep,
                                        value, orientation, parent, name);
    \newcode
        QSlider *mySlider = new QSlider(orientation, parent);
        mySlider->setObjectName(name);
        mySlider->setMinimum(minValue);
        mySlider->setMaximum(maxValue);
        mySlider->setPageStep(pageStep);
        mySlider->setValue(value);
    \endcode
*/
QSlider::QSlider(int minValue, int maxValue, int pageStep, int value, Qt::Orientation orientation,
                 QWidget *parent, const char *name)
    : QAbstractSlider(*new QSliderPrivate, parent)
{
    Q_D(QSlider);
    setObjectName(QString::fromAscii(name));
    d->minimum = minValue;
    d->maximum = maxValue;
    d->pageStep = pageStep;
    d->position = d->value = value;
    d->orientation = orientation;
    d->init();
}
#endif

/*!
    Destroys this slider.
*/
QSlider::~QSlider()
{
}

/*!
    \reimp
*/
void QSlider::paintEvent(QPaintEvent *)
{
    Q_D(QSlider);
    QPainter p(this);
    QStyleOptionSlider opt;
    initStyleOption(&opt);

    opt.subControls = QStyle::SC_SliderGroove | QStyle::SC_SliderHandle;
    if (d->tickPosition != NoTicks)
        opt.subControls |= QStyle::SC_SliderTickmarks;
    if (d->pressedControl) {
        opt.activeSubControls = d->pressedControl;
        opt.state |= QStyle::State_Sunken;
    } else {
        opt.activeSubControls = d->hoverControl;
    }

    style()->drawComplexControl(QStyle::CC_Slider, &opt, &p, this);
}

/*!
    \reimp
*/

bool QSlider::event(QEvent *event)
{
    Q_D(QSlider);

    switch(event->type()) {
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
        if (const QHoverEvent *he = static_cast<const QHoverEvent *>(event))
            d->updateHoverControl(he->pos());
        break;
    case QEvent::StyleChange:
    case QEvent::MacSizeChange:
        d->resetLayoutItemMargins();
        break;
    default:
        break;
    }
    return QAbstractSlider::event(event);
}

/*!
    \reimp
*/
void QSlider::mousePressEvent(QMouseEvent *ev)
{
    Q_D(QSlider);
    if (d->maximum == d->minimum || (ev->buttons() ^ ev->button())) {
        ev->ignore();
        return;
    }
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::keypadNavigationEnabled())
        setEditFocus(true);
#endif
    ev->accept();
    if ((ev->button() & style()->styleHint(QStyle::SH_Slider_AbsoluteSetButtons)) == ev->button()) {
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        const QRect sliderRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
        const QPoint center = sliderRect.center() - sliderRect.topLeft();
        // to take half of the slider off for the setSliderPosition call we use the center - topLeft

        setSliderPosition(d->pixelPosToRangeValue(d->pick(ev->pos() - center)));
        triggerAction(SliderMove);
        setRepeatAction(SliderNoAction);
        d->pressedControl = QStyle::SC_SliderHandle;
        update();
    } else if ((ev->button() & style()->styleHint(QStyle::SH_Slider_PageSetButtons)) == ev->button()) {
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        d->pressedControl = style()->hitTestComplexControl(QStyle::CC_Slider,
                                                           &opt, ev->pos(), this);
        SliderAction action = SliderNoAction;
        if (d->pressedControl == QStyle::SC_SliderGroove) {
            const QRect sliderRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
            int pressValue = d->pixelPosToRangeValue(d->pick(ev->pos() - sliderRect.center() + sliderRect.topLeft()));
            d->pressValue = pressValue;
            if (pressValue > d->value)
                action = SliderPageStepAdd;
            else if (pressValue < d->value)
                action = SliderPageStepSub;
            if (action) {
                triggerAction(action);
                setRepeatAction(action);
            }
        }
    } else {
        ev->ignore();
        return;
    }

    if (d->pressedControl == QStyle::SC_SliderHandle) {
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        setRepeatAction(SliderNoAction);
        QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
        d->clickOffset = d->pick(ev->pos() - sr.topLeft());
        update(sr);
        setSliderDown(true);
    }
}

/*!
    \reimp
*/
void QSlider::mouseMoveEvent(QMouseEvent *ev)
{
    Q_D(QSlider);
    if (d->pressedControl != QStyle::SC_SliderHandle) {
        ev->ignore();
        return;
    }
    ev->accept();
    int newPosition = d->pixelPosToRangeValue(d->pick(ev->pos()) - d->clickOffset);
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    setSliderPosition(newPosition);
}


/*!
    \reimp
*/
void QSlider::mouseReleaseEvent(QMouseEvent *ev)
{
    Q_D(QSlider);
    if (d->pressedControl == QStyle::SC_None || ev->buttons()) {
        ev->ignore();
        return;
    }
    ev->accept();
    QStyle::SubControl oldPressed = QStyle::SubControl(d->pressedControl);
    d->pressedControl = QStyle::SC_None;
    setRepeatAction(SliderNoAction);
    if (oldPressed == QStyle::SC_SliderHandle)
        setSliderDown(false);
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    opt.subControls = oldPressed;
    update(style()->subControlRect(QStyle::CC_Slider, &opt, oldPressed, this));
}

/*!
    \reimp
*/
QSize QSlider::sizeHint() const
{
    Q_D(const QSlider);
    ensurePolished();
    const int SliderLength = 84, TickSpace = 5;
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    int thick = style()->pixelMetric(QStyle::PM_SliderThickness, &opt, this);
    if (d->tickPosition & TicksAbove)
        thick += TickSpace;
    if (d->tickPosition & TicksBelow)
        thick += TickSpace;
    int w = thick, h = SliderLength;
    if (d->orientation == Qt::Horizontal) {
        w = SliderLength;
        h = thick;
    }
    return style()->sizeFromContents(QStyle::CT_Slider, &opt, QSize(w, h), this).expandedTo(QApplication::globalStrut());
}

/*!
    \reimp
*/
QSize QSlider::minimumSizeHint() const
{
    Q_D(const QSlider);
    QSize s = sizeHint();
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    int length = style()->pixelMetric(QStyle::PM_SliderLength, &opt, this);
    if (d->orientation == Qt::Horizontal)
        s.setWidth(length);
    else
        s.setHeight(length);
    return s;
}

/*!
    \property QSlider::tickPosition
    \brief the tickmark position for this slider

    The valid values are described by the QSlider::TickPosition enum.

    The default value is \l QSlider::NoTicks.

    \sa tickInterval
*/

void QSlider::setTickPosition(TickPosition position)
{
    Q_D(QSlider);
    d->tickPosition = position;
    d->resetLayoutItemMargins();
    update();
    updateGeometry();
}

QSlider::TickPosition QSlider::tickPosition() const
{
    return d_func()->tickPosition;
}

/*!
    \fn TickPosition QSlider::tickmarks() const
    \compat

    Use tickPosition() instead.
*/

/*!
    \fn QSlider::setTickmarks(TickPosition position)
    \compat

    Use setTickPosition() instead.
*/

/*!
    \property QSlider::tickInterval
    \brief the interval between tickmarks

    This is a value interval, not a pixel interval. If it is 0, the
    slider will choose between singleStep() and pageStep().

    The default value is 0.

    \sa tickPosition, lineStep(), pageStep()
*/

void QSlider::setTickInterval(int ts)
{
    d_func()->tickInterval = qMax(0, ts);
    update();
}

int QSlider::tickInterval() const
{
    return d_func()->tickInterval;
}

/*!
    \fn void QSlider::addStep()

    Use setValue() instead.
*/

/*!
    \fn void QSlider::subtractStep()

    Use setValue() instead.
*/

/*! \internal
    Returns the style option for slider.
*/
Q_GUI_EXPORT QStyleOptionSlider qt_qsliderStyleOption(QSlider *slider)
{
    QStyleOptionSlider sliderOption;
    slider->initStyleOption(&sliderOption);
    return sliderOption;
}

#endif

QT_END_NAMESPACE
