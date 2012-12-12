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

#include "qapplication.h"
#include "qcursor.h"
#include "qevent.h"
#include "qpainter.h"
#include "qscrollbar.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qmenu.h"
#include <QtCore/qelapsedtimer.h>

#ifndef QT_NO_SCROLLBAR

#ifndef QT_NO_ACCESSIBILITY
#include "qaccessible.h"
#endif
#include <limits.h>
#include "qabstractslider_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QScrollBar
    \brief The QScrollBar widget provides a vertical or horizontal scroll bar.

    \ingroup basicwidgets

    A scroll bar is a control that enables the user to access parts of a
    document that is larger than the widget used to display it. It provides
    a visual indication of the user's current position within the document
    and the amount of the document that is visible. Scroll bars are usually
    equipped with other controls that enable more accurate navigation.
    Qt displays scroll bars in a way that is appropriate for each platform.

    If you need to provide a scrolling view onto another widget, it may be
    more convenient to use the QScrollArea class because this provides a
    viewport widget and scroll bars. QScrollBar is useful if you need to
    implement similar functionality for specialized widgets using QAbstractScrollArea;
    for example, if you decide to subclass QAbstractItemView.
    For most other situations where a slider control is used to obtain a value
    within a given range, the QSlider class may be more appropriate for your
    needs.

    \table
    \row \o \image qscrollbar-picture.png
    \o Scroll bars typically include four separate controls: a slider,
    scroll arrows, and a page control.

    \list
    \o a. The slider provides a way to quickly go to any part of the
    document, but does not support accurate navigation within large
    documents.
    \o b. The scroll arrows are push buttons which can be used to accurately
    navigate to a particular place in a document. For a vertical scroll bar
    connected to a text editor, these typically move the current position one
    "line" up or down, and adjust the position of the slider by a small
    amount. In editors and list boxes a "line" might mean one line of text;
    in an image viewer it might mean 20 pixels.
    \o c. The page control is the area over which the slider is dragged (the
    scroll bar's background). Clicking here moves the scroll bar towards
    the click by one "page". This value is usually the same as the length of
    the slider.
    \endlist
    \endtable

    Each scroll bar has a value that indicates how far the slider is from
    the start of the scroll bar; this is obtained with value() and set
    with setValue(). This value always lies within the range of values
    defined for the scroll bar, from \l{QAbstractSlider::minimum()}{minimum()}
    to \l{QAbstractSlider::minimum()}{maximum()} inclusive. The range of
    acceptable values can be set with setMinimum() and setMaximum().
    At the minimum value, the top edge of the slider (for a vertical scroll
    bar) or left edge (for a horizontal scroll bar) will be at the top (or
    left) end of the scroll bar. At the maximum value, the bottom (or right)
    edge of the slider will be at the bottom (or right) end of the scroll bar.

    The length of the slider is usually related to the value of the page step,
    and typically represents the proportion of the document area shown in a
    scrolling view. The page step is the amount that the value changes by
    when the user presses the \key{Page Up} and \key{Page Down} keys, and is
    set with setPageStep(). Smaller changes to the value defined by the
    line step are made using the cursor keys, and this quantity is set with
    \l{QAbstractSlider::}{setSingleStep()}.

    Note that the range of values used is independent of the actual size
    of the scroll bar widget. You do not need to take this into account when
    you choose values for the range and the page step.

    The range of values specified for the scroll bar are often determined
    differently to those for a QSlider because the length of the slider
    needs to be taken into account. If we have a document with 100 lines,
    and we can only show 20 lines in a widget, we may wish to construct a
    scroll bar with a page step of 20, a minimum value of 0, and a maximum
    value of 80. This would give us a scroll bar with five "pages".

    \table
    \row \o \inlineimage qscrollbar-values.png
    \o The relationship between a document length, the range of values used
    in a scroll bar, and the page step is simple in many common situations.
    The scroll bar's range of values is determined by subtracting a
    chosen page step from some value representing the length of the document.
    In such cases, the following equation is useful:
    \e{document length} = maximum() - minimum() + pageStep().
    \endtable

    QScrollBar only provides integer ranges. Note that although
    QScrollBar handles very large numbers, scroll bars on current
    screens cannot usefully represent ranges above about 100,000 pixels.
    Beyond that, it becomes difficult for the user to control the
    slider using either the keyboard or the mouse, and the scroll
    arrows will have limited use.

    ScrollBar inherits a comprehensive set of signals from QAbstractSlider:
    \list
    \o \l{QAbstractSlider::valueChanged()}{valueChanged()} is emitted when the
       scroll bar's value has changed. The tracking() determines whether this
       signal is emitted during user interaction.
    \o \l{QAbstractSlider::rangeChanged()}{rangeChanged()} is emitted when the
       scroll bar's range of values has changed.
    \o \l{QAbstractSlider::sliderPressed()}{sliderPressed()} is emitted when
       the user starts to drag the slider.
    \o \l{QAbstractSlider::sliderMoved()}{sliderMoved()} is emitted when the user
       drags the slider.
    \o \l{QAbstractSlider::sliderReleased()}{sliderReleased()} is emitted when
       the user releases the slider.
    \o \l{QAbstractSlider::actionTriggered()}{actionTriggered()} is emitted
       when the scroll bar is changed by user interaction or via the
       \l{QAbstractSlider::triggerAction()}{triggerAction()} function.
    \endlist

    A scroll bar can be controlled by the keyboard, but it has a
    default focusPolicy() of Qt::NoFocus. Use setFocusPolicy() to
    enable keyboard interaction with the scroll bar:
    \list
         \o Left/Right move a horizontal scroll bar by one single step.
         \o Up/Down move a vertical scroll bar by one single step.
         \o PageUp moves up one page.
         \o PageDown moves down one page.
         \o Home moves to the start (mininum).
         \o End moves to the end (maximum).
     \endlist

    The slider itself can be controlled by using the
    \l{QAbstractSlider::triggerAction()}{triggerAction()} function to simulate
    user interaction with the scroll bar controls. This is useful if you have
    many different widgets that use a common range of values.

    Most GUI styles use the pageStep() value to calculate the size of the
    slider.

    \table 100%
    \row \o \inlineimage macintosh-horizontalscrollbar.png Screenshot of a Macintosh style scroll bar
         \o A scroll bar shown in the \l{Macintosh Style Widget Gallery}{Macintosh widget style}.
    \row \o \inlineimage windowsxp-horizontalscrollbar.png Screenshot of a Windows XP style scroll bar
         \o A scroll bar shown in the \l{Windows XP Style Widget Gallery}{Windows XP widget style}.
    \row \o \inlineimage plastique-horizontalscrollbar.png Screenshot of a Plastique style scroll bar
         \o A scroll bar shown in the \l{Plastique Style Widget Gallery}{Plastique widget style}.
    \endtable

    \sa QScrollArea, QSlider, QDial, QSpinBox, {fowler}{GUI Design Handbook: Scroll Bar}, {Sliders Example}
*/

class QScrollBarPrivate : public QAbstractSliderPrivate
{
    Q_DECLARE_PUBLIC(QScrollBar)
public:
    QStyle::SubControl pressedControl;
    bool pointerOutsidePressedControl;

    int clickOffset, snapBackPosition;

    void activateControl(uint control, int threshold = 500);
    void stopRepeatAction();
    int pixelPosToRangeValue(int pos) const;
    void init();
    bool updateHoverControl(const QPoint &pos);
    QStyle::SubControl newHoverControl(const QPoint &pos);

    QStyle::SubControl hoverControl;
    QRect hoverRect;
};

bool QScrollBarPrivate::updateHoverControl(const QPoint &pos)
{
    Q_Q(QScrollBar);
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

QStyle::SubControl QScrollBarPrivate::newHoverControl(const QPoint &pos)
{
    Q_Q(QScrollBar);
    QStyleOptionSlider opt;
    q->initStyleOption(&opt);
    opt.subControls = QStyle::SC_All;
    hoverControl = q->style()->hitTestComplexControl(QStyle::CC_ScrollBar, &opt, pos, q);
    if (hoverControl == QStyle::SC_None)
        hoverRect = QRect();
    else
        hoverRect = q->style()->subControlRect(QStyle::CC_ScrollBar, &opt, hoverControl, q);
    return hoverControl;
}

void QScrollBarPrivate::activateControl(uint control, int threshold)
{
    QAbstractSlider::SliderAction action = QAbstractSlider::SliderNoAction;
    switch (control) {
    case QStyle::SC_ScrollBarAddPage:
        action = QAbstractSlider::SliderPageStepAdd;
        break;
    case QStyle::SC_ScrollBarSubPage:
        action = QAbstractSlider::SliderPageStepSub;
        break;
    case QStyle::SC_ScrollBarAddLine:
        action = QAbstractSlider::SliderSingleStepAdd;
        break;
    case QStyle::SC_ScrollBarSubLine:
        action = QAbstractSlider::SliderSingleStepSub;
        break;
    case QStyle::SC_ScrollBarFirst:
        action = QAbstractSlider::SliderToMinimum;
        break;
    case QStyle::SC_ScrollBarLast:
        action = QAbstractSlider::SliderToMaximum;
        break;
    default:
        break;
    }

    if (action) {
        q_func()->setRepeatAction(action, threshold);
        q_func()->triggerAction(action);
    }
}

void QScrollBarPrivate::stopRepeatAction()
{
    Q_Q(QScrollBar);
    QStyle::SubControl tmp = pressedControl;
    q->setRepeatAction(QAbstractSlider::SliderNoAction);
    pressedControl = QStyle::SC_None;

    if (tmp == QStyle::SC_ScrollBarSlider)
        q->setSliderDown(false);

    QStyleOptionSlider opt;
    q->initStyleOption(&opt);
    q->repaint(q->style()->subControlRect(QStyle::CC_ScrollBar, &opt, tmp, q));
}

/*!
    Initialize \a option with the values from this QScrollBar. This method
    is useful for subclasses when they need a QStyleOptionSlider, but don't want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QScrollBar::initStyleOption(QStyleOptionSlider *option) const
{
    if (!option)
        return;

    Q_D(const QScrollBar);
    option->initFrom(this);
    option->subControls = QStyle::SC_None;
    option->activeSubControls = QStyle::SC_None;
    option->orientation = d->orientation;
    option->minimum = d->minimum;
    option->maximum = d->maximum;
    option->sliderPosition = d->position;
    option->sliderValue = d->value;
    option->singleStep = d->singleStep;
    option->pageStep = d->pageStep;
    option->upsideDown = d->invertedAppearance;
    if (d->orientation == Qt::Horizontal)
        option->state |= QStyle::State_Horizontal;
}


#define HORIZONTAL (d_func()->orientation == Qt::Horizontal)
#define VERTICAL !HORIZONTAL

/*!
    Constructs a vertical scroll bar.

    The \a parent argument is sent to the QWidget constructor.

    The \l {QAbstractSlider::minimum} {minimum} defaults to 0, the
    \l {QAbstractSlider::maximum} {maximum} to 99, with a
    \l {QAbstractSlider::singleStep} {singleStep} size of 1 and a
    \l {QAbstractSlider::pageStep} {pageStep} size of 10, and an
    initial \l {QAbstractSlider::value} {value} of 0.
*/
QScrollBar::QScrollBar(QWidget *parent)
    : QAbstractSlider(*new QScrollBarPrivate, parent)
{
    d_func()->orientation = Qt::Vertical;
    d_func()->init();
}

/*!
    Constructs a scroll bar with the given \a orientation.

    The \a parent argument is passed to the QWidget constructor.

    The \l {QAbstractSlider::minimum} {minimum} defaults to 0, the
    \l {QAbstractSlider::maximum} {maximum} to 99, with a
    \l {QAbstractSlider::singleStep} {singleStep} size of 1 and a
    \l {QAbstractSlider::pageStep} {pageStep} size of 10, and an
    initial \l {QAbstractSlider::value} {value} of 0.
*/
QScrollBar::QScrollBar(Qt::Orientation orientation, QWidget *parent)
    : QAbstractSlider(*new QScrollBarPrivate, parent)
{
    d_func()->orientation = orientation;
    d_func()->init();
}


#ifdef QT3_SUPPORT
/*!
    Use one of the constructors that doesn't take the \a name
    argument and then use setObjectName() instead.
*/
QScrollBar::QScrollBar(QWidget *parent, const char *name)
    : QAbstractSlider(*new QScrollBarPrivate, parent)
{
    setObjectName(QString::fromAscii(name));
    d_func()->orientation = Qt::Vertical;
    d_func()->init();
}

/*!
    Use one of the constructors that doesn't take the \a name
    argument and then use setObjectName() instead.
*/
QScrollBar::QScrollBar(Qt::Orientation orientation, QWidget *parent, const char *name)
    : QAbstractSlider(*new QScrollBarPrivate, parent)
{
    setObjectName(QString::fromAscii(name));
    d_func()->orientation = orientation;
    d_func()->init();
}

/*!
    Use one of the constructors that doesn't take the \a name
    argument and then use setObjectName() instead.
*/
QScrollBar::QScrollBar(int minimum, int maximum, int lineStep, int pageStep,
                        int value, Qt::Orientation orientation,
                        QWidget *parent, const char *name)
    : QAbstractSlider(*new QScrollBarPrivate, parent)
{
    Q_D(QScrollBar);
    setObjectName(QString::fromAscii(name));
    d->minimum = minimum;
    d->maximum = maximum;
    d->singleStep = lineStep;
    d->pageStep = pageStep;
    d->value = value;
    d->orientation = orientation;
    d->init();
}
#endif // QT3_SUPPORT

/*!
    Destroys the scroll bar.
*/
QScrollBar::~QScrollBar()
{
}

void QScrollBarPrivate::init()
{
    Q_Q(QScrollBar);
    invertedControls = true;
    pressedControl = hoverControl = QStyle::SC_None;
    pointerOutsidePressedControl = false;
    q->setFocusPolicy(Qt::NoFocus);
    QSizePolicy sp(QSizePolicy::Minimum, QSizePolicy::Fixed, QSizePolicy::Slider);
    if (orientation == Qt::Vertical)
        sp.transpose();
    q->setSizePolicy(sp);
    q->setAttribute(Qt::WA_WState_OwnSizePolicy, false);
    q->setAttribute(Qt::WA_OpaquePaintEvent);

#if !defined(QT_NO_CONTEXTMENU) && defined(Q_WS_WINCE)
    if (!q->style()->styleHint(QStyle::SH_ScrollBar_ContextMenu, 0, q)) {
        q->setContextMenuPolicy(Qt::PreventContextMenu);
    }
#endif
}

#ifndef QT_NO_CONTEXTMENU
/*! \reimp */
void QScrollBar::contextMenuEvent(QContextMenuEvent *event)
{
    if (!style()->styleHint(QStyle::SH_ScrollBar_ContextMenu, 0, this)) {
        QAbstractSlider::contextMenuEvent(event);
        return ;
    }

#ifndef QT_NO_MENU
    bool horiz = HORIZONTAL;
    QPointer<QMenu> menu = new QMenu(this);
    QAction *actScrollHere = menu->addAction(tr("Scroll here"));
    menu->addSeparator();
    QAction *actScrollTop =  menu->addAction(horiz ? tr("Left edge") : tr("Top"));
    QAction *actScrollBottom = menu->addAction(horiz ? tr("Right edge") : tr("Bottom"));
    menu->addSeparator();
    QAction *actPageUp = menu->addAction(horiz ? tr("Page left") : tr("Page up"));
    QAction *actPageDn = menu->addAction(horiz ? tr("Page right") : tr("Page down"));
    menu->addSeparator();
    QAction *actScrollUp = menu->addAction(horiz ? tr("Scroll left") : tr("Scroll up"));
    QAction *actScrollDn = menu->addAction(horiz ? tr("Scroll right") : tr("Scroll down"));
    QAction *actionSelected = menu->exec(event->globalPos());
    delete menu;
    if (actionSelected == 0)
        /* do nothing */ ;
    else if (actionSelected == actScrollHere)
        setValue(d_func()->pixelPosToRangeValue(horiz ? event->pos().x() : event->pos().y()));
    else if (actionSelected == actScrollTop)
        triggerAction(QAbstractSlider::SliderToMinimum);
    else if (actionSelected == actScrollBottom)
        triggerAction(QAbstractSlider::SliderToMaximum);
    else if (actionSelected == actPageUp)
        triggerAction(QAbstractSlider::SliderPageStepSub);
    else if (actionSelected == actPageDn)
        triggerAction(QAbstractSlider::SliderPageStepAdd);
    else if (actionSelected == actScrollUp)
        triggerAction(QAbstractSlider::SliderSingleStepSub);
    else if (actionSelected == actScrollDn)
        triggerAction(QAbstractSlider::SliderSingleStepAdd);
#endif // QT_NO_MENU
}
#endif // QT_NO_CONTEXTMENU


/*! \reimp */
QSize QScrollBar::sizeHint() const
{
    ensurePolished();
    QStyleOptionSlider opt;
    initStyleOption(&opt);

    int scrollBarExtent = style()->pixelMetric(QStyle::PM_ScrollBarExtent, &opt, this);
    int scrollBarSliderMin = style()->pixelMetric(QStyle::PM_ScrollBarSliderMin, &opt, this);
    QSize size;
    if (opt.orientation == Qt::Horizontal)
        size = QSize(scrollBarExtent * 2 + scrollBarSliderMin, scrollBarExtent);
    else
        size = QSize(scrollBarExtent, scrollBarExtent * 2 + scrollBarSliderMin);

    return style()->sizeFromContents(QStyle::CT_ScrollBar, &opt, size, this)
        .expandedTo(QApplication::globalStrut());
 }

/*!\reimp */
void QScrollBar::sliderChange(SliderChange change)
{
    QAbstractSlider::sliderChange(change);
}

/*!
    \reimp
*/
bool QScrollBar::event(QEvent *event)
{
    switch(event->type()) {
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
    if (const QHoverEvent *he = static_cast<const QHoverEvent *>(event))
        d_func()->updateHoverControl(he->pos());
        break;
#ifndef QT_NO_WHEELEVENT
    case QEvent::Wheel: {
        event->ignore();
        // override wheel event without adding virtual function override
        if (!isEnabled()) // don't scroll when disabled [QTBUG-27308]
            return false; // normally prevented in QWidget::event()
        QWheelEvent *ev = static_cast<QWheelEvent *>(event);
        int delta = ev->delta();
        // scrollbar is a special case - in vertical mode it reaches minimum
        // value in the upper position, however QSlider's minimum value is on
        // the bottom. So we need to invert a value, but since the scrollbar is
        // inverted by default, we need to inverse the delta value for the
        // horizontal orientation.
        if (ev->orientation() == Qt::Horizontal)
            delta = -delta;
        Q_D(QScrollBar);
        if (d->scrollByDelta(ev->orientation(), ev->modifiers(), delta))
            event->accept();
        return true;
    }
#endif
    default:
        break;
    }
    return QAbstractSlider::event(event);
}

/*!
    \reimp
*/
void QScrollBar::paintEvent(QPaintEvent *)
{
    Q_D(QScrollBar);
    QPainter p(this);
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    opt.subControls = QStyle::SC_All;
    if (d->pressedControl) {
        opt.activeSubControls = (QStyle::SubControl)d->pressedControl;
        if (!d->pointerOutsidePressedControl)
            opt.state |= QStyle::State_Sunken;
    } else {
        opt.activeSubControls = (QStyle::SubControl)d->hoverControl;
    }
    style()->drawComplexControl(QStyle::CC_ScrollBar, &opt, &p, this);
}

/*!
    \reimp
*/
void QScrollBar::mousePressEvent(QMouseEvent *e)
{
    Q_D(QScrollBar);

    if (d->repeatActionTimer.isActive())
        d->stopRepeatAction();

    bool midButtonAbsPos = style()->styleHint(QStyle::SH_ScrollBar_MiddleClickAbsolutePosition,
                                             0, this);
    QStyleOptionSlider opt;
    initStyleOption(&opt);

    if (d->maximum == d->minimum // no range
        || (e->buttons() & (~e->button())) // another button was clicked before
        || !(e->button() == Qt::LeftButton || (midButtonAbsPos && e->button() == Qt::MidButton)))
        return;

    d->pressedControl = style()->hitTestComplexControl(QStyle::CC_ScrollBar, &opt, e->pos(), this);
    d->pointerOutsidePressedControl = false;

    QRect sr = style()->subControlRect(QStyle::CC_ScrollBar, &opt,
                                       QStyle::SC_ScrollBarSlider, this);
    QPoint click = e->pos();
    QPoint pressValue = click - sr.center() + sr.topLeft();
    d->pressValue = d->orientation == Qt::Horizontal ? d->pixelPosToRangeValue(pressValue.x()) :
        d->pixelPosToRangeValue(pressValue.y());
    if (d->pressedControl == QStyle::SC_ScrollBarSlider) {
        d->clickOffset = HORIZONTAL ? (click.x()-sr.x()) : (click.y()-sr.y());
        d->snapBackPosition = d->position;
    }

    if ((d->pressedControl == QStyle::SC_ScrollBarAddPage
          || d->pressedControl == QStyle::SC_ScrollBarSubPage)
        && ((midButtonAbsPos && e->button() == Qt::MidButton)
            || (style()->styleHint(QStyle::SH_ScrollBar_LeftClickAbsolutePosition, &opt, this)
                && e->button() == Qt::LeftButton))) {
        int sliderLength = HORIZONTAL ? sr.width() : sr.height();
        setSliderPosition(d->pixelPosToRangeValue((HORIZONTAL ? e->pos().x()
                                                              : e->pos().y()) - sliderLength / 2));
        d->pressedControl = QStyle::SC_ScrollBarSlider;
        d->clickOffset = sliderLength / 2;
    }
    const int initialDelay = 500; // default threshold
    d->activateControl(d->pressedControl, initialDelay);
    QElapsedTimer time;
    time.start();
    repaint(style()->subControlRect(QStyle::CC_ScrollBar, &opt, d->pressedControl, this));
    if (time.elapsed() >= initialDelay && d->repeatActionTimer.isActive()) {
        // It took more than 500ms (the initial timer delay) to process the repaint(), we
        // therefore need to restart the timer in case we have a pending mouse release event;
        // otherwise we'll get a timer event right before the release event,
        // causing the repeat action to be invoked twice on a single mouse click.
        // 50ms is the default repeat time (see activateControl/setRepeatAction).
        d->repeatActionTimer.start(50, this);
    }
    if (d->pressedControl == QStyle::SC_ScrollBarSlider)
        setSliderDown(true);
}


/*!
    \reimp
*/
void QScrollBar::mouseReleaseEvent(QMouseEvent *e)
{
    Q_D(QScrollBar);
    if (!d->pressedControl)
        return;

    if (e->buttons() & (~e->button())) // some other button is still pressed
        return;

    d->stopRepeatAction();
}


/*!
    \reimp
*/
void QScrollBar::mouseMoveEvent(QMouseEvent *e)
{
    Q_D(QScrollBar);
    if (!d->pressedControl)
        return;

    QStyleOptionSlider opt;
    initStyleOption(&opt);
    if (!(e->buttons() & Qt::LeftButton
          ||  ((e->buttons() & Qt::MidButton)
               && style()->styleHint(QStyle::SH_ScrollBar_MiddleClickAbsolutePosition, &opt, this))))
        return;

    if (d->pressedControl == QStyle::SC_ScrollBarSlider) {
        QPoint click = e->pos();
        int newPosition = d->pixelPosToRangeValue((HORIZONTAL ? click.x() : click.y()) -d->clickOffset);
        int m = style()->pixelMetric(QStyle::PM_MaximumDragDistance, &opt, this);
        if (m >= 0) {
            QRect r = rect();
            r.adjust(-m, -m, m, m);
            if (! r.contains(e->pos()))
                newPosition = d->snapBackPosition;
        }
        setSliderPosition(newPosition);
    } else if (!style()->styleHint(QStyle::SH_ScrollBar_ScrollWhenPointerLeavesControl, &opt, this)) {

        if (style()->styleHint(QStyle::SH_ScrollBar_RollBetweenButtons, &opt, this)
                && d->pressedControl & (QStyle::SC_ScrollBarAddLine | QStyle::SC_ScrollBarSubLine)) {
            QStyle::SubControl newSc = style()->hitTestComplexControl(QStyle::CC_ScrollBar, &opt, e->pos(), this);
            if (newSc == d->pressedControl && !d->pointerOutsidePressedControl)
                return; // nothing to do
            if (newSc & (QStyle::SC_ScrollBarAddLine | QStyle::SC_ScrollBarSubLine)) {
                d->pointerOutsidePressedControl = false;
                QRect scRect = style()->subControlRect(QStyle::CC_ScrollBar, &opt, newSc, this);
                scRect |= style()->subControlRect(QStyle::CC_ScrollBar, &opt, d->pressedControl, this);
                d->pressedControl = newSc;
                d->activateControl(d->pressedControl, 0);
                update(scRect);
                return;
            }
        }

        // stop scrolling when the mouse pointer leaves a control
        // similar to push buttons
        QRect pr = style()->subControlRect(QStyle::CC_ScrollBar, &opt, d->pressedControl, this);
        if (pr.contains(e->pos()) == d->pointerOutsidePressedControl) {
            if ((d->pointerOutsidePressedControl = !d->pointerOutsidePressedControl)) {
                d->pointerOutsidePressedControl = true;
                setRepeatAction(SliderNoAction);
                repaint(pr);
            } else  {
                d->activateControl(d->pressedControl);
            }
        }
    }
}


int QScrollBarPrivate::pixelPosToRangeValue(int pos) const
{
    Q_Q(const QScrollBar);
    QStyleOptionSlider opt;
    q->initStyleOption(&opt);
    QRect gr = q->style()->subControlRect(QStyle::CC_ScrollBar, &opt,
                                          QStyle::SC_ScrollBarGroove, q);
    QRect sr = q->style()->subControlRect(QStyle::CC_ScrollBar, &opt,
                                          QStyle::SC_ScrollBarSlider, q);
    int sliderMin, sliderMax, sliderLength;

    if (orientation == Qt::Horizontal) {
        sliderLength = sr.width();
        sliderMin = gr.x();
        sliderMax = gr.right() - sliderLength + 1;
        if (q->layoutDirection() == Qt::RightToLeft)
            opt.upsideDown = !opt.upsideDown;
    } else {
        sliderLength = sr.height();
        sliderMin = gr.y();
        sliderMax = gr.bottom() - sliderLength + 1;
    }

    return  QStyle::sliderValueFromPosition(minimum, maximum, pos - sliderMin,
                                            sliderMax - sliderMin, opt.upsideDown);
}

/*! \reimp
*/
void QScrollBar::hideEvent(QHideEvent *)
{
    Q_D(QScrollBar);
    if (d->pressedControl) {
        d->pressedControl = QStyle::SC_None;
        setRepeatAction(SliderNoAction);
    }
}

/*!
    \fn bool QScrollBar::draggingSlider()

    Use isSliderDown() instead.
*/

/*! \internal
    Returns the style option for scroll bar.
*/
Q_GUI_EXPORT QStyleOptionSlider qt_qscrollbarStyleOption(QScrollBar *scrollbar)
{
    QStyleOptionSlider opt;
    scrollbar->initStyleOption(&opt);
    return opt;
}

QT_END_NAMESPACE

#endif // QT_NO_SCROLLBAR
