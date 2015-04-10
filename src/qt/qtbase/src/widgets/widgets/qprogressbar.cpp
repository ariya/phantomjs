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

#include "qprogressbar.h"
#ifndef QT_NO_PROGRESSBAR
#include <qlocale.h>
#include <qevent.h>
#include <qpainter.h>
#include <qstylepainter.h>
#include <qstyleoption.h>
#include <private/qwidget_p.h>
#ifndef QT_NO_ACCESSIBILITY
#include <qaccessible.h>
#endif
#include <limits.h>

QT_BEGIN_NAMESPACE

class QProgressBarPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QProgressBar)

public:
    QProgressBarPrivate();

    void init();
    void initDefaultFormat();
    inline void resetLayoutItemMargins();

    int minimum;
    int maximum;
    int value;
    Qt::Alignment alignment;
    uint textVisible : 1;
    uint defaultFormat: 1;
    int lastPaintedValue;
    Qt::Orientation orientation;
    bool invertedAppearance;
    QProgressBar::Direction textDirection;
    QString format;
    inline int bound(int val) const { return qMax(minimum-1, qMin(maximum, val)); }
    bool repaintRequired() const;
};

QProgressBarPrivate::QProgressBarPrivate()
    : minimum(0), maximum(100), value(-1), alignment(Qt::AlignLeft), textVisible(true),
      defaultFormat(true), lastPaintedValue(-1), orientation(Qt::Horizontal), invertedAppearance(false),
      textDirection(QProgressBar::TopToBottom)
{
    initDefaultFormat();
}

void QProgressBarPrivate::initDefaultFormat()
{
    if (defaultFormat)
        format = QLatin1String("%p") + locale.percent();
}

void QProgressBarPrivate::init()
{
    Q_Q(QProgressBar);
    QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Fixed);
    if (orientation == Qt::Vertical)
        sp.transpose();
    q->setSizePolicy(sp);
    q->setAttribute(Qt::WA_WState_OwnSizePolicy, false);
    resetLayoutItemMargins();
}

void QProgressBarPrivate::resetLayoutItemMargins()
{
    Q_Q(QProgressBar);
    QStyleOptionProgressBar option;
    q->initStyleOption(&option);
    setLayoutItemMargins(QStyle::SE_ProgressBarLayoutItem, &option);
}

/*!
    Initialize \a option with the values from this QProgressBar. This method is useful
    for subclasses when they need a QStyleOptionProgressBar or QStyleOptionProgressBarV2,
    but don't want to fill in all the information themselves. This function will check the version
    of the QStyleOptionProgressBar and fill in the additional values for a
    QStyleOptionProgressBarV2.

    \sa QStyleOption::initFrom()
*/
void QProgressBar::initStyleOption(QStyleOptionProgressBar *option) const
{
    if (!option)
        return;
    Q_D(const QProgressBar);
    option->initFrom(this);

    if (d->orientation == Qt::Horizontal)
        option->state |= QStyle::State_Horizontal;
    option->minimum = d->minimum;
    option->maximum = d->maximum;
    option->progress = d->value;
    option->textAlignment = d->alignment;
    option->textVisible = d->textVisible;
    option->text = text();

    if (QStyleOptionProgressBarV2 *optionV2
            = qstyleoption_cast<QStyleOptionProgressBarV2 *>(option)) {
        optionV2->orientation = d->orientation;  // ### Qt 5: use State_Horizontal instead
        optionV2->invertedAppearance = d->invertedAppearance;
        optionV2->bottomToTop = (d->textDirection == QProgressBar::BottomToTop);
    }
}

bool QProgressBarPrivate::repaintRequired() const
{
    Q_Q(const QProgressBar);
    if (value == lastPaintedValue)
        return false;

    int valueDifference = qAbs(value - lastPaintedValue);

    // Check if the text needs to be repainted
    if (value == minimum || value == maximum)
        return true;
    if (textVisible) {
        if ((format.contains(QLatin1String("%v"))))
            return true;
        if ((format.contains(QLatin1String("%p"))
             && valueDifference >= qAbs((maximum - minimum) / 100)))
            return true;
    }

    // Check if the bar needs to be repainted
    QStyleOptionProgressBarV2 opt;
    q->initStyleOption(&opt);
    int cw = q->style()->pixelMetric(QStyle::PM_ProgressBarChunkWidth, &opt, q);
    QRect groove  = q->style()->subElementRect(QStyle::SE_ProgressBarGroove, &opt, q);
    // This expression is basically
    // (valueDifference / (maximum - minimum) > cw / groove.width())
    // transformed to avoid integer division.
    int grooveBlock = (q->orientation() == Qt::Horizontal) ? groove.width() : groove.height();
    return (valueDifference * grooveBlock > cw * (maximum - minimum));
}

/*!
    \class QProgressBar
    \brief The QProgressBar widget provides a horizontal or vertical progress bar.

    \ingroup basicwidgets
    \inmodule QtWidgets

    A progress bar is used to give the user an indication of the
    progress of an operation and to reassure them that the application
    is still running.

    The progress bar uses the concept of \e steps. You set it up by
    specifying the minimum and maximum possible step values, and it
    will display the percentage of steps that have been completed
    when you later give it the current step value. The percentage is
    calculated by dividing the progress (value() - minimum()) divided
    by maximum() - minimum().

    You can specify the minimum and maximum number of steps with
    setMinimum() and setMaximum. The current number of steps is set
    with setValue(). The progress bar can be rewound to the
    beginning with reset().

    If minimum and maximum both are set to 0, the bar shows a busy
    indicator instead of a percentage of steps. This is useful, for
    example, when using QNetworkAccessManager to download items when
    they are unable to determine the size of the item being downloaded.

    \table
    \row \li \inlineimage macintosh-progressbar.png Screenshot of a Macintosh style progress bar
         \li A progress bar shown in the Macintosh widget style.
    \row \li \inlineimage windowsvista-progressbar.png Screenshot of a Windows Vista style progress bar
         \li A progress bar shown in the Windows Vista widget style.
    \row \li \inlineimage fusion-progressbar.png Screenshot of a Fusion style progress bar
         \li A progress bar shown in the Fusion widget style.
    \endtable

    \sa QProgressDialog, {fowler}{GUI Design Handbook: Progress Indicator}
*/

/*!
    \since 4.1
    \enum QProgressBar::Direction
    \brief Specifies the reading direction of the \l text for vertical progress bars.

    \value TopToBottom The text is rotated 90 degrees clockwise.
    \value BottomToTop The text is rotated 90 degrees counter-clockwise.

    Note that whether or not the text is drawn is dependent on the style.
    Currently CleanLooks and Plastique draw the text. Mac, Windows
    and WindowsXP style do not.

    \sa textDirection
*/

/*!
    \fn void QProgressBar::valueChanged(int value)

    This signal is emitted when the value shown in the progress bar changes.
    \a value is the new value shown by the progress bar.
*/

/*!
    Constructs a progress bar with the given \a parent.

    By default, the minimum step value is set to 0, and the maximum to 100.

    \sa setRange()
*/

QProgressBar::QProgressBar(QWidget *parent)
    : QWidget(*(new QProgressBarPrivate), parent, 0)
{
    d_func()->init();
}

/*!
    Destructor.
*/
QProgressBar::~QProgressBar()
{
}

/*!
    Reset the progress bar. The progress bar "rewinds" and shows no
    progress.
*/

void QProgressBar::reset()
{
    Q_D(QProgressBar);
    d->value = d->minimum - 1;
    if (d->minimum == INT_MIN)
        d->value = INT_MIN;
    repaint();
}

/*!
    \property QProgressBar::minimum
    \brief the progress bar's minimum value

    When setting this property, the \l maximum is adjusted if
    necessary to ensure that the range remains valid. If the
    current value falls outside the new range, the progress bar is reset
    with reset().
*/
void QProgressBar::setMinimum(int minimum)
{
    setRange(minimum, qMax(d_func()->maximum, minimum));
}

int QProgressBar::minimum() const
{
    return d_func()->minimum;
}


/*!
    \property QProgressBar::maximum
    \brief the progress bar's maximum value

    When setting this property, the \l minimum is adjusted if
    necessary to ensure that the range remains valid. If the
    current value falls outside the new range, the progress bar is reset
    with reset().
*/

void QProgressBar::setMaximum(int maximum)
{
    setRange(qMin(d_func()->minimum, maximum), maximum);
}

int QProgressBar::maximum() const
{
    return d_func()->maximum;
}

/*!
    \property QProgressBar::value
    \brief the progress bar's current value

    Attempting to change the current value to one outside
    the minimum-maximum range has no effect on the current value.
*/
void QProgressBar::setValue(int value)
{
    Q_D(QProgressBar);
    if (d->value == value
            || ((value > d->maximum || value < d->minimum)
                && (d->maximum != 0 || d->minimum != 0)))
        return;
    d->value = value;
    emit valueChanged(value);
#ifndef QT_NO_ACCESSIBILITY
    if (isVisible()) {
        QAccessibleValueChangeEvent event(this, value);
        QAccessible::updateAccessibility(&event);
    }
#endif
    if (d->repaintRequired())
        repaint();
}

int QProgressBar::value() const
{
    return d_func()->value;
}

/*!
    Sets the progress bar's minimum and maximum values to \a minimum and
    \a maximum respectively.

    If \a maximum is smaller than \a minimum, \a minimum becomes the only
    legal value.

    If the current value falls outside the new range, the progress bar is reset
    with reset().

    \sa minimum, maximum
*/
void QProgressBar::setRange(int minimum, int maximum)
{
    Q_D(QProgressBar);
    if (minimum != d->minimum || maximum != d->maximum) {
        d->minimum = minimum;
        d->maximum = qMax(minimum, maximum);

        if (d->value < (d->minimum - 1) || d->value > d->maximum)
            reset();
        else
            update();
    }
}

/*!
    \property QProgressBar::textVisible
    \brief whether the current completed percentage should be displayed

    This property may be ignored by the style (e.g., QMacStyle never draws the text).

    \sa textDirection
*/
void QProgressBar::setTextVisible(bool visible)
{
    Q_D(QProgressBar);
    if (d->textVisible != visible) {
        d->textVisible = visible;
        repaint();
    }
}

bool QProgressBar::isTextVisible() const
{
    return d_func()->textVisible;
}

/*!
    \property QProgressBar::alignment
    \brief the alignment of the progress bar
*/
void QProgressBar::setAlignment(Qt::Alignment alignment)
{
    if (d_func()->alignment != alignment) {
        d_func()->alignment = alignment;
        repaint();
    }
}

Qt::Alignment QProgressBar::alignment() const
{
    return d_func()->alignment;
}

/*!
    \reimp
*/
void QProgressBar::paintEvent(QPaintEvent *)
{
    QStylePainter paint(this);
    QStyleOptionProgressBarV2 opt;
    initStyleOption(&opt);
    paint.drawControl(QStyle::CE_ProgressBar, opt);
    d_func()->lastPaintedValue = d_func()->value;
}

/*!
    \reimp
*/
QSize QProgressBar::sizeHint() const
{
    ensurePolished();
    QFontMetrics fm = fontMetrics();
    QStyleOptionProgressBarV2 opt;
    initStyleOption(&opt);
    int cw = style()->pixelMetric(QStyle::PM_ProgressBarChunkWidth, &opt, this);
    QSize size = QSize(qMax(9, cw) * 7 + fm.width(QLatin1Char('0')) * 4, fm.height() + 8);
    if (opt.orientation == Qt::Vertical)
        size.transpose();
    return style()->sizeFromContents(QStyle::CT_ProgressBar, &opt, size, this);
}

/*!
    \reimp
*/
QSize QProgressBar::minimumSizeHint() const
{
    QSize size;
    if (orientation() == Qt::Horizontal)
        size = QSize(sizeHint().width(), fontMetrics().height() + 2);
    else
        size = QSize(fontMetrics().height() + 2, sizeHint().height());
    return size;
}

/*!
    \property QProgressBar::text
    \brief the descriptive text shown with the progress bar

    The text returned is the same as the text displayed in the center
    (or in some styles, to the left) of the progress bar.

    The progress shown in the text may be smaller than the minimum value,
    indicating that the progress bar is in the "reset" state before any
    progress is set.

    In the default implementation, the text either contains a percentage
    value that indicates the progress so far, or it is blank because the
    progress bar is in the reset state.
*/
QString QProgressBar::text() const
{
    Q_D(const QProgressBar);
    if ((d->maximum == 0 && d->minimum == 0) || d->value < d->minimum
            || (d->value == INT_MIN && d->minimum == INT_MIN))
        return QString();

    qint64 totalSteps = qint64(d->maximum) - d->minimum;

    QString result = d->format;
    QLocale locale = d->locale; // Omit group separators for compatibility with previous versions that were non-localized.
    locale.setNumberOptions(locale.numberOptions() | QLocale::OmitGroupSeparator);
    result.replace(QLatin1String("%m"), locale.toString(totalSteps));
    result.replace(QLatin1String("%v"), locale.toString(d->value));

    // If max and min are equal and we get this far, it means that the
    // progress bar has one step and that we are on that step. Return
    // 100% here in order to avoid division by zero further down.
    if (totalSteps == 0) {
        result.replace(QLatin1String("%p"), locale.toString(int(100)));
        return result;
    }

    int progress = (qreal(d->value) - d->minimum) * 100.0 / totalSteps;
    result.replace(QLatin1String("%p"), locale.toString(progress));
    return result;
}

/*!
    \since 4.1
    \property QProgressBar::orientation
    \brief the orientation of the progress bar

    The orientation must be \l Qt::Horizontal (the default) or \l
    Qt::Vertical.

    \sa invertedAppearance, textDirection
*/

void QProgressBar::setOrientation(Qt::Orientation orientation)
{
    Q_D(QProgressBar);
    if (d->orientation == orientation)
        return;
    d->orientation = orientation;
    if (!testAttribute(Qt::WA_WState_OwnSizePolicy)) {
        QSizePolicy sp = sizePolicy();
        sp.transpose();
        setSizePolicy(sp);
        setAttribute(Qt::WA_WState_OwnSizePolicy, false);
    }
    d->resetLayoutItemMargins();
    update();
    updateGeometry();
}

Qt::Orientation QProgressBar::orientation() const
{
    Q_D(const QProgressBar);
    return d->orientation;
}

/*!
    \since 4.1
    \property QProgressBar::invertedAppearance
    \brief whether or not a progress bar shows its progress inverted

    If this property is \c false, the progress bar grows in the other
    direction (e.g. from right to left). By default, the progress bar
    is not inverted.

    \sa orientation, layoutDirection
*/

void QProgressBar::setInvertedAppearance(bool invert)
{
    Q_D(QProgressBar);
    d->invertedAppearance = invert;
    update();
}

bool QProgressBar::invertedAppearance() const
{
    Q_D(const QProgressBar);
    return d->invertedAppearance;
}

/*!
    \since 4.1
    \property QProgressBar::textDirection
    \brief the reading direction of the \l text for vertical progress bars

    This property has no impact on horizontal progress bars.
    By default, the reading direction is QProgressBar::TopToBottom.

    \sa orientation, textVisible
*/
void QProgressBar::setTextDirection(QProgressBar::Direction textDirection)
{
    Q_D(QProgressBar);
    d->textDirection = textDirection;
    update();
}

QProgressBar::Direction QProgressBar::textDirection() const
{
    Q_D(const QProgressBar);
    return d->textDirection;
}

/*! \reimp */
bool QProgressBar::event(QEvent *e)
{
    Q_D(QProgressBar);
    switch (e->type()) {
    case QEvent::StyleChange:
#ifdef Q_OS_MAC
    case QEvent::MacSizeChange:
#endif
        d->resetLayoutItemMargins();
        break;
    case QEvent::LocaleChange:
        d->initDefaultFormat();
        break;
    default:
        break;
    }
    return QWidget::event(e);
}

/*!
    \since 4.2
    \property QProgressBar::format
    \brief the string used to generate the current text

    %p - is replaced by the percentage completed.
    %v - is replaced by the current value.
    %m - is replaced by the total number of steps.

    The default value is "%p%".

    \sa text()
*/
void QProgressBar::setFormat(const QString &format)
{
    Q_D(QProgressBar);
    if (d->format == format)
        return;
    d->format = format;
    d->defaultFormat = false;
    update();
}

void QProgressBar::resetFormat()
{
    Q_D(QProgressBar);
    d->defaultFormat = true;
    d->initDefaultFormat();
    update();
}

QString QProgressBar::format() const
{
    Q_D(const QProgressBar);
    return d->format;
}

QT_END_NAMESPACE

#endif // QT_NO_PROGRESSBAR
