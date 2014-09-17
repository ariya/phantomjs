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

#include "qprogressdialog.h"

#ifndef QT_NO_PROGRESSDIALOG

#include "qshortcut.h"
#include "qpainter.h"
#include "qdrawutil.h"
#include "qlabel.h"
#include "qprogressbar.h"
#include "qapplication.h"
#include "qstyle.h"
#include "qpushbutton.h"
#include "qcursor.h"
#include "qtimer.h"
#include "qelapsedtimer.h"
#include <private/qdialog_p.h>
#include <limits.h>

#if defined(QT_SOFTKEYS_ENABLED)
#include <qaction.h>
#endif
#ifdef Q_WS_S60
#include <QtGui/qdesktopwidget.h>
#endif


QT_BEGIN_NAMESPACE

// If the operation is expected to take this long (as predicted by
// progress time), show the progress dialog.
static const int defaultShowTime = 4000;
// Wait at least this long before attempting to make a prediction.
static const int minWaitTime = 50;

class QProgressDialogPrivate : public QDialogPrivate
{
    Q_DECLARE_PUBLIC(QProgressDialog)

public:
    QProgressDialogPrivate() : label(0), cancel(0), bar(0),
        shown_once(false),
        cancellation_flag(false),
        showTime(defaultShowTime),
#ifndef QT_NO_SHORTCUT
        escapeShortcut(0),
#endif
#ifdef QT_SOFTKEYS_ENABLED
        cancelAction(0),
#endif
        useDefaultCancelText(false)
    {
    }

    void init(const QString &labelText, const QString &cancelText, int min, int max);
    void layout();
    void retranslateStrings();
    void _q_disconnectOnClose();

    QLabel *label;
    QPushButton *cancel;
    QProgressBar *bar;
    QTimer *forceTimer;
    bool shown_once;
    bool cancellation_flag;
    QElapsedTimer starttime;
#ifndef QT_NO_CURSOR
    QCursor parentCursor;
#endif
    int showTime;
    bool autoClose;
    bool autoReset;
    bool forceHide;
#ifndef QT_NO_SHORTCUT
    QShortcut *escapeShortcut;
#endif
#ifdef QT_SOFTKEYS_ENABLED
    QAction *cancelAction;
#endif
    bool useDefaultCancelText;
    QPointer<QObject> receiverToDisconnectOnClose;
    QByteArray memberToDisconnectOnClose;
};

void QProgressDialogPrivate::init(const QString &labelText, const QString &cancelText,
                                  int min, int max)
{
    Q_Q(QProgressDialog);
    label = new QLabel(labelText, q);
    int align = q->style()->styleHint(QStyle::SH_ProgressDialog_TextLabelAlignment, 0, q);
    label->setAlignment(Qt::Alignment(align));
    bar = new QProgressBar(q);
    bar->setRange(min, max);
    autoClose = true;
    autoReset = true;
    forceHide = false;
    QObject::connect(q, SIGNAL(canceled()), q, SLOT(cancel()));
    forceTimer = new QTimer(q);
    QObject::connect(forceTimer, SIGNAL(timeout()), q, SLOT(forceShow()));
    if (useDefaultCancelText) {
        retranslateStrings();
    } else {
        q->setCancelButtonText(cancelText);
    }
}

void QProgressDialogPrivate::layout()
{
    Q_Q(QProgressDialog);
    int sp = q->style()->pixelMetric(QStyle::PM_DefaultLayoutSpacing);
    int mtb = q->style()->pixelMetric(QStyle::PM_DefaultTopLevelMargin);
    int mlr = qMin(q->width() / 10, mtb);
    const bool centered =
        bool(q->style()->styleHint(QStyle::SH_ProgressDialog_CenterCancelButton, 0, q));

    int additionalSpacing = 0;
#ifdef Q_OS_SYMBIAN
    //In Symbian, we need to have wider margins for dialog borders, as the actual border is some pixels
    //inside the dialog area (to enable transparent borders)
    additionalSpacing = mlr;
#endif

    QSize cs = cancel ? cancel->sizeHint() : QSize(0,0);
    QSize bh = bar->sizeHint();
    int cspc;
    int lh = 0;

    // Find spacing and sizes that fit.  It is important that a progress
    // dialog can be made very small if the user demands it so.
    for (int attempt=5; attempt--;) {
        cspc = cancel ? cs.height() + sp : 0;
        lh = qMax(0, q->height() - mtb - bh.height() - sp - cspc);

        if (lh < q->height()/4) {
            // Getting cramped
            sp /= 2;
            mtb /= 2;
            if (cancel) {
                cs.setHeight(qMax(4,cs.height()-sp-2));
            }
            bh.setHeight(qMax(4,bh.height()-sp-1));
        } else {
            break;
        }
    }

    if (cancel) {
        cancel->setGeometry(
            centered ? q->width()/2 - cs.width()/2 : q->width() - mlr - cs.width(),
            q->height() - mtb - cs.height(),
            cs.width(), cs.height());
    }

    if (label)
        label->setGeometry(mlr, additionalSpacing, q->width() - mlr * 2, lh);
    bar->setGeometry(mlr, lh + sp + additionalSpacing, q->width() - mlr * 2, bh.height());
}

void QProgressDialogPrivate::retranslateStrings()
{
    Q_Q(QProgressDialog);
    if (useDefaultCancelText)
        q->setCancelButtonText(QProgressDialog::tr("Cancel"));
}

void QProgressDialogPrivate::_q_disconnectOnClose()
{
    Q_Q(QProgressDialog);
    if (receiverToDisconnectOnClose) {
        QObject::disconnect(q, SIGNAL(canceled()), receiverToDisconnectOnClose,
                            memberToDisconnectOnClose);
        receiverToDisconnectOnClose = 0;
    }
    memberToDisconnectOnClose.clear();
}

/*!
  \class QProgressDialog
  \brief The QProgressDialog class provides feedback on the progress of a slow operation.
  \ingroup standard-dialogs


  A progress dialog is used to give the user an indication of how long
  an operation is going to take, and to demonstrate that the
  application has not frozen. It can also give the user an opportunity
  to abort the operation.

  A common problem with progress dialogs is that it is difficult to know
  when to use them; operations take different amounts of time on different
  hardware.  QProgressDialog offers a solution to this problem:
  it estimates the time the operation will take (based on time for
  steps), and only shows itself if that estimate is beyond minimumDuration()
  (4 seconds by default).

  Use setMinimum() and setMaximum() or the constructor to set the number of
  "steps" in the operation and call setValue() as the operation
  progresses. The number of steps can be chosen arbitrarily. It can be the
  number of files copied, the number of bytes received, the number of
  iterations through the main loop of your algorithm, or some other
  suitable unit. Progress starts at the value set by setMinimum(),
  and the progress dialog shows that the operation has finished when
  you call setValue() with the value set by setMaximum() as its argument.

  The dialog automatically resets and hides itself at the end of the
  operation.  Use setAutoReset() and setAutoClose() to change this
  behavior. Note that if you set a new maximum (using setMaximum() or
  setRange()) that equals your current value(), the dialog will not
  close regardless.

  There are two ways of using QProgressDialog: modal and modeless.

  Compared to a modeless QProgressDialog, a modal QProgressDialog is simpler
  to use for the programmer. Do the operation in a loop, call \l setValue() at
  intervals, and check for cancellation with wasCanceled(). For example:

  \snippet doc/src/snippets/dialogs/dialogs.cpp 3

  A modeless progress dialog is suitable for operations that take
  place in the background, where the user is able to interact with the
  application. Such operations are typically based on QTimer (or
  QObject::timerEvent()), QSocketNotifier, or QUrlOperator; or performed
  in a separate thread. A QProgressBar in the status bar of your main window
  is often an alternative to a modeless progress dialog.

  You need to have an event loop to be running, connect the
  canceled() signal to a slot that stops the operation, and call \l
  setValue() at intervals. For example:

  \snippet doc/src/snippets/dialogs/dialogs.cpp 4
  \codeline
  \snippet doc/src/snippets/dialogs/dialogs.cpp 5
  \codeline
  \snippet doc/src/snippets/dialogs/dialogs.cpp 6

  In both modes the progress dialog may be customized by
  replacing the child widgets with custom widgets by using setLabel(),
  setBar(), and setCancelButton().
  The functions setLabelText() and setCancelButtonText()
  set the texts shown.

  \image plastique-progressdialog.png A progress dialog shown in the Plastique widget style.

  \sa QDialog, QProgressBar, {fowler}{GUI Design Handbook: Progress Indicator},
      {Find Files Example}, {Pixelator Example}
*/


/*!
  Constructs a progress dialog.

  Default settings:
  \list
  \i The label text is empty.
  \i The cancel button text is (translated) "Cancel".
  \i minimum is 0;
  \i maximum is 100
  \endlist

  The \a parent argument is dialog's parent widget. The widget flags, \a f, are
  passed to the QDialog::QDialog() constructor.

  \sa setLabelText(), setCancelButtonText(), setCancelButton(),
  setMinimum(), setMaximum()
*/

QProgressDialog::QProgressDialog(QWidget *parent, Qt::WindowFlags f)
    : QDialog(*(new QProgressDialogPrivate), parent, f)
{
    Q_D(QProgressDialog);
    d->useDefaultCancelText = true;
    d->init(QString::fromLatin1(""), QString(), 0, 100);
}

/*!
  Constructs a progress dialog.

   The \a labelText is the text used to remind the user what is progressing.

   The \a cancelButtonText is the text to display on the cancel button.  If 
   QString() is passed then no cancel button is shown.

   The \a minimum and \a maximum is the number of steps in the operation for
   which this progress dialog shows progress.  For example, if the
   operation is to examine 50 files, this value minimum value would be 0,
   and the maximum would be 50. Before examining the first file, call
   setValue(0). As each file is processed call setValue(1), setValue(2),
   etc., finally calling setValue(50) after examining the last file.

   The \a parent argument is the dialog's parent widget. The parent, \a parent, and
   widget flags, \a f, are passed to the QDialog::QDialog() constructor.

  \sa setLabelText(), setLabel(), setCancelButtonText(), setCancelButton(),
  setMinimum(), setMaximum()
*/

QProgressDialog::QProgressDialog(const QString &labelText,
                                 const QString &cancelButtonText,
                                 int minimum, int maximum,
                                 QWidget *parent, Qt::WindowFlags f)
    : QDialog(*(new QProgressDialogPrivate), parent, f)
{
    Q_D(QProgressDialog);
    d->init(labelText, cancelButtonText, minimum, maximum);
}


/*!
  Destroys the progress dialog.
*/

QProgressDialog::~QProgressDialog()
{
}

/*!
  \fn void QProgressDialog::canceled()

  This signal is emitted when the cancel button is clicked.
  It is connected to the cancel() slot by default.

  \sa wasCanceled()
*/


/*!
  Sets the label to \a label. The progress dialog resizes to fit. The
  label becomes owned by the progress dialog and will be deleted when
  necessary, so do not pass the address of an object on the stack.

  \sa setLabelText()
*/

void QProgressDialog::setLabel(QLabel *label)
{
    Q_D(QProgressDialog);
    delete d->label;
    d->label = label;
    if (label) {
        if (label->parentWidget() == this) {
            label->hide(); // until we resize
        } else {
            label->setParent(this, 0);
        }
    }
    int w = qMax(isVisible() ? width() : 0, sizeHint().width());
    int h = qMax(isVisible() ? height() : 0, sizeHint().height());
    resize(w, h);
    if (label)
        label->show();
}


/*!
  \property QProgressDialog::labelText
  \brief the label's text

  The default text is an empty string.
*/

QString QProgressDialog::labelText() const
{
    Q_D(const QProgressDialog);
    if (d->label)
        return d->label->text();
    return QString();
}

void QProgressDialog::setLabelText(const QString &text)
{
    Q_D(QProgressDialog);
    if (d->label) {
        d->label->setText(text);
        int w = qMax(isVisible() ? width() : 0, sizeHint().width());
        int h = qMax(isVisible() ? height() : 0, sizeHint().height());
        resize(w, h);
    }
}


/*!
  Sets the cancel button to the push button, \a cancelButton. The
  progress dialog takes ownership of this button which will be deleted
  when necessary, so do not pass the address of an object that is on
  the stack, i.e. use new() to create the button.  If 0 is passed then
  no cancel button will be shown.

  \sa setCancelButtonText()
*/

void QProgressDialog::setCancelButton(QPushButton *cancelButton)
{
    Q_D(QProgressDialog);
    delete d->cancel;
    d->cancel = cancelButton;
    if (cancelButton) {
        if (cancelButton->parentWidget() == this) {
            cancelButton->hide(); // until we resize
        } else {
            cancelButton->setParent(this, 0);
        }
        connect(d->cancel, SIGNAL(clicked()), this, SIGNAL(canceled()));
#ifndef QT_NO_SHORTCUT
        d->escapeShortcut = new QShortcut(Qt::Key_Escape, this, SIGNAL(canceled()));
#endif
    } else {
#ifndef QT_NO_SHORTCUT
        delete d->escapeShortcut;
        d->escapeShortcut = 0;
#endif
    }
    int w = qMax(isVisible() ? width() : 0, sizeHint().width());
    int h = qMax(isVisible() ? height() : 0, sizeHint().height());
    resize(w, h);
    if (cancelButton)
#if !defined(QT_SOFTKEYS_ENABLED)
        cancelButton->show();
#else
    {
        d->cancelAction = new QAction(cancelButton->text(), cancelButton);
        d->cancelAction->setSoftKeyRole(QAction::NegativeSoftKey);
        connect(d->cancelAction, SIGNAL(triggered()), this, SIGNAL(canceled()));
        addAction(d->cancelAction);
    }
#endif
}

/*!
  Sets the cancel button's text to \a cancelButtonText.  If the text
  is set to QString() then it will cause the cancel button to be 
  hidden and deleted.

  \sa setCancelButton()
*/

void QProgressDialog::setCancelButtonText(const QString &cancelButtonText)
{
    Q_D(QProgressDialog);
    d->useDefaultCancelText = false;

    if (!cancelButtonText.isNull()) {
        if (d->cancel) {
            d->cancel->setText(cancelButtonText);
#ifdef QT_SOFTKEYS_ENABLED
            d->cancelAction->setText(cancelButtonText);
#endif
        } else {
            setCancelButton(new QPushButton(cancelButtonText, this));
        }
    } else {
        setCancelButton(0);
    }
    int w = qMax(isVisible() ? width() : 0, sizeHint().width());
    int h = qMax(isVisible() ? height() : 0, sizeHint().height());
    resize(w, h);
}


/*!
  Sets the progress bar widget to \a bar. The progress dialog resizes to
  fit. The progress dialog takes ownership of the progress \a bar which
  will be deleted when necessary, so do not use a progress bar
  allocated on the stack.
*/

void QProgressDialog::setBar(QProgressBar *bar)
{
    Q_D(QProgressDialog);
    if (!bar) {
        qWarning("QProgressDialog::setBar: Cannot set a null progress bar");
        return;
    }
#ifndef QT_NO_DEBUG
    if (value() > 0)
        qWarning("QProgressDialog::setBar: Cannot set a new progress bar "
                  "while the old one is active");
#endif
    delete d->bar;
    d->bar = bar;
    int w = qMax(isVisible() ? width() : 0, sizeHint().width());
    int h = qMax(isVisible() ? height() : 0, sizeHint().height());
    resize(w, h);
}


/*!
  \property QProgressDialog::wasCanceled
  \brief whether the dialog was canceled
*/

bool QProgressDialog::wasCanceled() const
{
    Q_D(const QProgressDialog);
    return d->cancellation_flag;
}


/*!
    \property QProgressDialog::maximum
    \brief the highest value represented by the progress bar

    The default is 0.

    \sa minimum, setRange()
*/

int QProgressDialog::maximum() const
{
    Q_D(const QProgressDialog);
    return d->bar->maximum();
}

void QProgressDialog::setMaximum(int maximum)
{
    Q_D(QProgressDialog);
    d->bar->setMaximum(maximum);
}

/*!
    \property QProgressDialog::minimum
    \brief the lowest value represented by the progress bar

    The default is 0.

    \sa maximum, setRange()
*/

int QProgressDialog::minimum() const
{
    Q_D(const QProgressDialog);
    return d->bar->minimum();
}

void QProgressDialog::setMinimum(int minimum)
{
    Q_D(QProgressDialog);
    d->bar->setMinimum(minimum);
}

/*!
    Sets the progress dialog's minimum and maximum values
    to \a minimum and \a maximum, respectively.

    If \a maximum is smaller than \a minimum, \a minimum becomes the only
    legal value.

    If the current value falls outside the new range, the progress
    dialog is reset with reset().

    \sa minimum, maximum
*/
void QProgressDialog::setRange(int minimum, int maximum)
{
    Q_D(QProgressDialog);
    d->bar->setRange(minimum, maximum);
}


/*!
  Resets the progress dialog.
  The progress dialog becomes hidden if autoClose() is true.

  \sa setAutoClose(), setAutoReset()
*/

void QProgressDialog::reset()
{
    Q_D(QProgressDialog);
#ifndef QT_NO_CURSOR
    if (value() >= 0) {
        if (parentWidget())
            parentWidget()->setCursor(d->parentCursor);
    }
#endif
    if (d->autoClose || d->forceHide)
        hide();
    d->bar->reset();
    d->cancellation_flag = false;
    d->shown_once = false;
    d->forceTimer->stop();

    /*
        I wish we could disconnect the user slot provided to open() here but
        unfortunately reset() is usually called before the slot has been invoked.
        (reset() is itself invoked when canceled() is emitted.)
    */
    if (d->receiverToDisconnectOnClose)
        QMetaObject::invokeMethod(this, "_q_disconnectOnClose", Qt::QueuedConnection);
}

/*!
  Resets the progress dialog. wasCanceled() becomes true until
  the progress dialog is reset.
  The progress dialog becomes hidden.
*/

void QProgressDialog::cancel()
{
    Q_D(QProgressDialog);
    d->forceHide = true;
    reset();
    d->forceHide = false;
    d->cancellation_flag = true;
}


int QProgressDialog::value() const
{
    Q_D(const QProgressDialog);
    return d->bar->value();
}

/*!
  \property QProgressDialog::value
  \brief the current amount of progress made.

  For the progress dialog to work as expected, you should initially set
  this property to 0 and finally set it to
  QProgressDialog::maximum(); you can call setValue() any number of times
  in-between.

  \warning If the progress dialog is modal
    (see QProgressDialog::QProgressDialog()),
    setValue() calls QApplication::processEvents(), so take care that
    this does not cause undesirable re-entrancy in your code. For example,
    don't use a QProgressDialog inside a paintEvent()!

  \sa minimum, maximum
*/
void QProgressDialog::setValue(int progress)
{
    Q_D(QProgressDialog);
    if (progress == d->bar->value()
        || (d->bar->value() == -1 && progress == d->bar->maximum()))
        return;

    d->bar->setValue(progress);

    if (d->shown_once) {
        if (isModal())
            QApplication::processEvents();
    } else {
        if (progress == 0) {
            d->starttime.start();
            d->forceTimer->start(d->showTime);
            return;
        } else {
            bool need_show;
            int elapsed = d->starttime.elapsed();
            if (elapsed >= d->showTime) {
                need_show = true;
            } else {
                if (elapsed > minWaitTime) {
                    int estimate;
                    int totalSteps = maximum() - minimum();
                    int myprogress = progress - minimum();
                    if ((totalSteps - myprogress) >= INT_MAX / elapsed)
                        estimate = (totalSteps - myprogress) / myprogress * elapsed;
                    else
                        estimate = elapsed * (totalSteps - myprogress) / myprogress;
                    need_show = estimate >= d->showTime;
                } else {
                    need_show = false;
                }
            }
            if (need_show) {
                int w = qMax(isVisible() ? width() : 0, sizeHint().width());
                int h = qMax(isVisible() ? height() : 0, sizeHint().height());
                resize(w, h);
                show();
                d->shown_once = true;
            }
        }
#ifdef Q_WS_MAC
        QApplication::flush();
#endif
    }

    if (progress == d->bar->maximum() && d->autoReset)
        reset();
}

/*!
  Returns a size that fits the contents of the progress dialog.
  The progress dialog resizes itself as required, so you should not
  need to call this yourself.
*/

QSize QProgressDialog::sizeHint() const
{
    Q_D(const QProgressDialog);
    QSize sh = d->label ? d->label->sizeHint() : QSize(0, 0);
    QSize bh = d->bar->sizeHint();
    int margin = style()->pixelMetric(QStyle::PM_DefaultTopLevelMargin);
    int spacing = style()->pixelMetric(QStyle::PM_DefaultLayoutSpacing);
    int h = margin * 2 + bh.height() + sh.height() + spacing;
    if (d->cancel)
        h += d->cancel->sizeHint().height() + spacing;
#ifdef Q_WS_S60
    if (QApplication::desktop()->size().height() > QApplication::desktop()->size().width())
        return QSize(qMax(QApplication::desktop()->size().width(), sh.width() + 2 * margin), h);
    else
        return QSize(qMax(QApplication::desktop()->size().height(), sh.width() + 2 * margin), h);
#else
    return QSize(qMax(200, sh.width() + 2 * margin), h);
#endif
}

/*!\reimp
*/
void QProgressDialog::resizeEvent(QResizeEvent *)
{
    Q_D(QProgressDialog);
    d->layout();
}

/*!
  \reimp
*/
void QProgressDialog::changeEvent(QEvent *ev)
{
    Q_D(QProgressDialog);
    if (ev->type() == QEvent::StyleChange) {
        d->layout();
    } else if (ev->type() == QEvent::LanguageChange) {
        d->retranslateStrings();
    }
    QDialog::changeEvent(ev);
}

/*!
    \property QProgressDialog::minimumDuration
    \brief the time that must pass before the dialog appears

    If the expected duration of the task is less than the
    minimumDuration, the dialog will not appear at all. This prevents
    the dialog popping up for tasks that are quickly over. For tasks
    that are expected to exceed the minimumDuration, the dialog will
    pop up after the minimumDuration time or as soon as any progress
    is set.

    If set to 0, the dialog is always shown as soon as any progress is
    set. The default is 4000 milliseconds.
*/
void QProgressDialog::setMinimumDuration(int ms)
{
    Q_D(QProgressDialog);
    d->showTime = ms;
    if (d->bar->value() == 0) {
        d->forceTimer->stop();
        d->forceTimer->start(ms);
    }
}

int QProgressDialog::minimumDuration() const
{
    Q_D(const QProgressDialog);
    return d->showTime;
}


/*!
  \reimp
*/

void QProgressDialog::closeEvent(QCloseEvent *e)
{
    emit canceled();
    QDialog::closeEvent(e);
}

/*!
  \property QProgressDialog::autoReset
  \brief whether the progress dialog calls reset() as soon as value() equals maximum()

  The default is true.

  \sa setAutoClose()
*/

void QProgressDialog::setAutoReset(bool b)
{
    Q_D(QProgressDialog);
    d->autoReset = b;
}

bool QProgressDialog::autoReset() const
{
    Q_D(const QProgressDialog);
    return d->autoReset;
}

/*!
  \property QProgressDialog::autoClose
  \brief whether the dialog gets hidden by reset()

  The default is true.

  \sa setAutoReset()
*/

void QProgressDialog::setAutoClose(bool close)
{
    Q_D(QProgressDialog);
    d->autoClose = close;
}

bool QProgressDialog::autoClose() const
{
    Q_D(const QProgressDialog);
    return d->autoClose;
}

/*!
  \reimp
*/

void QProgressDialog::showEvent(QShowEvent *e)
{
    Q_D(QProgressDialog);
    QDialog::showEvent(e);
    int w = qMax(isVisible() ? width() : 0, sizeHint().width());
    int h = qMax(isVisible() ? height() : 0, sizeHint().height());
    resize(w, h);
    d->forceTimer->stop();
}

/*!
  Shows the dialog if it is still hidden after the algorithm has been started
  and minimumDuration milliseconds have passed.

  \sa setMinimumDuration()
*/

void QProgressDialog::forceShow()
{
    Q_D(QProgressDialog);
    d->forceTimer->stop();
    if (d->shown_once || d->cancellation_flag)
        return;

    show();
    d->shown_once = true;
}

/*!
    \since 4.5
    \overload

    Opens the dialog and connects its accepted() signal to the slot specified
    by \a receiver and \a member.

    The signal will be disconnected from the slot when the dialog is closed.
*/
void QProgressDialog::open(QObject *receiver, const char *member)
{
    Q_D(QProgressDialog);
    connect(this, SIGNAL(canceled()), receiver, member);
    d->receiverToDisconnectOnClose = receiver;
    d->memberToDisconnectOnClose = member;
    QDialog::open();
}

QT_END_NAMESPACE

#include "moc_qprogressdialog.cpp"

#endif // QT_NO_PROGRESSDIALOG
