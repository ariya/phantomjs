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

#include "qstatusbar.h"
#ifndef QT_NO_STATUSBAR

#include "qlist.h"
#include "qdebug.h"
#include "qevent.h"
#include "qlayout.h"
#include "qpainter.h"
#include "qtimer.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qsizegrip.h"
#include "qmainwindow.h"

#ifndef QT_NO_ACCESSIBILITY
#include "qaccessible.h"
#endif

#include <private/qlayoutengine_p.h>
#include <private/qwidget_p.h>

QT_BEGIN_NAMESPACE

class QStatusBarPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QStatusBar)
public:
    QStatusBarPrivate() {}

    struct SBItem {
        SBItem(QWidget* widget, int stretch, bool permanent)
            : s(stretch), w(widget), p(permanent) {}
        int s;
        QWidget * w;
        bool p;
    };

    QList<SBItem *> items;
    QString tempItem;

    QBoxLayout * box;
    QTimer * timer;

#ifndef QT_NO_SIZEGRIP
    QSizeGrip * resizer;
    bool showSizeGrip;
#endif

    int savedStrut;

#ifdef Q_WS_MAC
    QPoint dragStart;
#endif

    int indexToLastNonPermanentWidget() const
    {
        int i = items.size() - 1;
        for (; i >= 0; --i) {
            SBItem *item = items.at(i);
            if (!(item && item->p))
                break;
        }
        return i;
    }

#ifndef QT_NO_SIZEGRIP
    void tryToShowSizeGrip()
    {
        if (!showSizeGrip)
            return;
        showSizeGrip = false;
        if (!resizer || resizer->isVisible())
            return;
        resizer->setAttribute(Qt::WA_WState_ExplicitShowHide, false);
        QMetaObject::invokeMethod(resizer, "_q_showIfNotHidden", Qt::DirectConnection);
        resizer->setAttribute(Qt::WA_WState_ExplicitShowHide, false);
    }
#endif

    QRect messageRect() const;
};


QRect QStatusBarPrivate::messageRect() const
{
    Q_Q(const QStatusBar);
    bool rtl = q->layoutDirection() == Qt::RightToLeft;

    int left = 6;
    int right = q->width() - 12;

#ifndef QT_NO_SIZEGRIP
    if (resizer && resizer->isVisible()) {
        if (rtl)
            left = resizer->x() + resizer->width();
        else
            right = resizer->x();
    }
#endif

    for (int i=0; i<items.size(); ++i) {
        QStatusBarPrivate::SBItem* item = items.at(i);
        if (!item)
            break;
        if (item->p && item->w->isVisible()) {
                if (item->p) {
                    if (rtl)
                        left = qMax(left, item->w->x() + item->w->width() + 2);
                    else
                        right = qMin(right, item->w->x() - 2);
                }
                break;
        }
    }
    return QRect(left, 0, right-left, q->height());
}


/*!
    \class QStatusBar
    \brief The QStatusBar class provides a horizontal bar suitable for
    presenting status information.

    \ingroup mainwindow-classes
    \ingroup helpsystem
    \inmodule QtWidgets

    Each status indicator falls into one of three categories:

    \list
    \li \e Temporary - briefly occupies most of the status bar. Used
        to explain tool tip texts or menu entries, for example.
    \li \e Normal - occupies part of the status bar and may be hidden
        by temporary messages. Used to display the page and line
        number in a word processor, for example.
    \li \e Permanent - is never hidden. Used for important mode
        indications, for example, some applications put a Caps Lock
        indicator in the status bar.
    \endlist

    QStatusBar lets you display all three types of indicators.

    Typically, a request for the status bar functionality occurs in
    relation to a QMainWindow object. QMainWindow provides a main
    application window, with a menu bar, tool bars, dock widgets \e
    and a status bar around a large central widget. The status bar can
    be retrieved using the QMainWindow::statusBar() function, and
    replaced using the QMainWindow::setStatusBar() function.

    Use the showMessage() slot to display a \e temporary message:

    \snippet mainwindows/dockwidgets/mainwindow.cpp 8

    To remove a temporary message, use the clearMessage() slot, or set
    a time limit when calling showMessage(). For example:

    \snippet mainwindows/dockwidgets/mainwindow.cpp 3

    Use the currentMessage() function to retrieve the temporary
    message currently shown. The QStatusBar class also provide the
    messageChanged() signal which is emitted whenever the temporary
    status message changes.

    \target permanent message
    \e Normal and \e Permanent messages are displayed by creating a
    small widget (QLabel, QProgressBar or even QToolButton) and then
    adding it to the status bar using the addWidget() or the
    addPermanentWidget() function. Use the removeWidget() function to
    remove such messages from the status bar.

    \snippet code/src_gui_widgets_qstatusbar.cpp 0

    By default QStatusBar provides a QSizeGrip in the lower-right
    corner. You can disable it using the setSizeGripEnabled()
    function. Use the isSizeGripEnabled() function to determine the
    current status of the size grip.

    \image fusion-statusbar-sizegrip.png A status bar shown in the Fusion widget style

    \sa QMainWindow, QStatusTipEvent, {fowler}{GUI Design Handbook:
    Status Bar}, {Application Example}
*/


/*!
    Constructs a status bar with a size grip and the given \a parent.

    \sa setSizeGripEnabled()
*/
QStatusBar::QStatusBar(QWidget * parent)
    : QWidget(*new QStatusBarPrivate, parent, 0)
{
    Q_D(QStatusBar);
    d->box = 0;
    d->timer = 0;

#ifndef QT_NO_SIZEGRIP
    d->resizer = 0;
    setSizeGripEnabled(true); // causes reformat()
#else
    reformat();
#endif
}

/*!
    Destroys this status bar and frees any allocated resources and
    child widgets.
*/
QStatusBar::~QStatusBar()
{
    Q_D(QStatusBar);
    while (!d->items.isEmpty())
        delete d->items.takeFirst();
}


/*!
    Adds the given \a widget to this status bar, reparenting the
    widget if it isn't already a child of this QStatusBar object. The
    \a stretch parameter is used to compute a suitable size for the
    given \a widget as the status bar grows and shrinks. The default
    stretch factor is 0, i.e giving the widget a minimum of space.

    The widget is located to the far left of the first permanent
    widget (see addPermanentWidget()) and may be obscured by temporary
    messages.

    \sa insertWidget(), removeWidget(), addPermanentWidget()
*/

void QStatusBar::addWidget(QWidget * widget, int stretch)
{
    if (!widget)
        return;
    insertWidget(d_func()->indexToLastNonPermanentWidget() + 1, widget, stretch);
}

/*!
    \since 4.2

    Inserts the given \a widget at the given \a index to this status bar,
    reparenting the widget if it isn't already a child of this
    QStatusBar object. If \a index is out of range, the widget is appended
    (in which case it is the actual index of the widget that is returned).

    The \a stretch parameter is used to compute a suitable size for
    the given \a widget as the status bar grows and shrinks. The
    default stretch factor is 0, i.e giving the widget a minimum of
    space.

    The widget is located to the far left of the first permanent
    widget (see addPermanentWidget()) and may be obscured by temporary
    messages.

    \sa addWidget(), removeWidget(), addPermanentWidget()
*/
int QStatusBar::insertWidget(int index, QWidget *widget, int stretch)
{
    if (!widget)
        return -1;

    Q_D(QStatusBar);
    QStatusBarPrivate::SBItem* item = new QStatusBarPrivate::SBItem(widget, stretch, false);

    int idx = d->indexToLastNonPermanentWidget();
    if (index < 0 || index > d->items.size() || (idx >= 0 && index > idx + 1)) {
        qWarning("QStatusBar::insertWidget: Index out of range (%d), appending widget", index);
        index = idx + 1;
    }
    d->items.insert(index, item);

    if (!d->tempItem.isEmpty())
        widget->hide();

    reformat();
    if (!widget->isHidden() || !widget->testAttribute(Qt::WA_WState_ExplicitShowHide))
        widget->show();

    return index;
}

/*!
    Adds the given \a widget permanently to this status bar,
    reparenting the widget if it isn't already a child of this
    QStatusBar object. The \a stretch parameter is used to compute a
    suitable size for the given \a widget as the status bar grows and
    shrinks. The default stretch factor is 0, i.e giving the widget a
    minimum of space.

    Permanently means that the widget may not be obscured by temporary
    messages. It is is located at the far right of the status bar.

    \sa insertPermanentWidget(), removeWidget(), addWidget()
*/

void QStatusBar::addPermanentWidget(QWidget * widget, int stretch)
{
    if (!widget)
        return;
    insertPermanentWidget(d_func()->items.size(), widget, stretch);
}


/*!
    \since 4.2

    Inserts the given \a widget at the given \a index permanently to this status bar,
    reparenting the widget if it isn't already a child of this
    QStatusBar object. If \a index is out of range, the widget is appended
    (in which case it is the actual index of the widget that is returned).

    The \a stretch parameter is used to compute a
    suitable size for the given \a widget as the status bar grows and
    shrinks. The default stretch factor is 0, i.e giving the widget a
    minimum of space.

    Permanently means that the widget may not be obscured by temporary
    messages. It is is located at the far right of the status bar.

    \sa addPermanentWidget(), removeWidget(), addWidget()
*/
int QStatusBar::insertPermanentWidget(int index, QWidget *widget, int stretch)
{
    if (!widget)
        return -1;

    Q_D(QStatusBar);
    QStatusBarPrivate::SBItem* item = new QStatusBarPrivate::SBItem(widget, stretch, true);

    int idx = d->indexToLastNonPermanentWidget();
    if (index < 0 || index > d->items.size() || (idx >= 0 && index <= idx)) {
        qWarning("QStatusBar::insertPermanentWidget: Index out of range (%d), appending widget", index);
        index = d->items.size();
    }
    d->items.insert(index, item);

    reformat();
    if (!widget->isHidden() || !widget->testAttribute(Qt::WA_WState_ExplicitShowHide))
        widget->show();

    return index;
}

/*!
    Removes the specified \a widget from the status bar.

    \note This function does not delete the widget but \e hides it.
    To add the widget again, you must call both the addWidget() and
    show() functions.

    \sa addWidget(), addPermanentWidget(), clearMessage()
*/

void QStatusBar::removeWidget(QWidget *widget)
{
    if (!widget)
        return;

    Q_D(QStatusBar);
    bool found = false;
    QStatusBarPrivate::SBItem* item;
    for (int i=0; i<d->items.size(); ++i) {
        item = d->items.at(i);
        if (!item)
            break;
        if (item->w == widget) {
            d->items.removeAt(i);
            item->w->hide();
            delete item;
            found = true;
            break;
        }
    }

    if (found)
        reformat();
#if defined(QT_DEBUG)
    else
        qDebug("QStatusBar::removeWidget(): Widget not found.");
#endif
}

/*!
    \property QStatusBar::sizeGripEnabled

    \brief whether the QSizeGrip in the bottom-right corner of the
    status bar is enabled

    The size grip is enabled by default.
*/

bool QStatusBar::isSizeGripEnabled() const
{
#ifdef QT_NO_SIZEGRIP
    return false;
#else
    Q_D(const QStatusBar);
    return !!d->resizer;
#endif
}

void QStatusBar::setSizeGripEnabled(bool enabled)
{
#ifdef QT_NO_SIZEGRIP
    Q_UNUSED(enabled);
#else
    Q_D(QStatusBar);
    if (!enabled != !d->resizer) {
        if (enabled) {
            d->resizer = new QSizeGrip(this);
            d->resizer->hide();
            d->resizer->installEventFilter(this);
            d->showSizeGrip = true;
        } else {
            delete d->resizer;
            d->resizer = 0;
            d->showSizeGrip = false;
        }
        reformat();
        if (d->resizer && isVisible())
            d->tryToShowSizeGrip();
    }
#endif
}


/*!
    Changes the status bar's appearance to account for item changes.

    Special subclasses may need this function, but geometry management
    will usually take care of any necessary rearrangements.
*/
void QStatusBar::reformat()
{
    Q_D(QStatusBar);
    if (d->box)
        delete d->box;

    QBoxLayout *vbox;
#ifndef QT_NO_SIZEGRIP
    if (d->resizer) {
        d->box = new QHBoxLayout(this);
        d->box->setMargin(0);
        vbox = new QVBoxLayout;
        d->box->addLayout(vbox);
    } else
#endif
    {
        vbox = d->box = new QVBoxLayout(this);
        d->box->setMargin(0);
    }
    vbox->addSpacing(3);
    QBoxLayout* l = new QHBoxLayout;
    vbox->addLayout(l);
    l->addSpacing(2);
    l->setSpacing(6);

    int maxH = fontMetrics().height();

    int i;
    QStatusBarPrivate::SBItem* item;
    for (i=0,item=0; i<d->items.size(); ++i) {
        item = d->items.at(i);
        if (!item || item->p)
            break;
        l->addWidget(item->w, item->s);
        int itemH = qMin(qSmartMinSize(item->w).height(), item->w->maximumHeight());
        maxH = qMax(maxH, itemH);
    }

    l->addStretch(0);

    for (item=0; i<d->items.size(); ++i) {
        item = d->items.at(i);
        if (!item)
            break;
        l->addWidget(item->w, item->s);
        int itemH = qMin(qSmartMinSize(item->w).height(), item->w->maximumHeight());
        maxH = qMax(maxH, itemH);
    }
#ifndef QT_NO_SIZEGRIP
    if (d->resizer) {
        maxH = qMax(maxH, d->resizer->sizeHint().height());
        d->box->addSpacing(1);
        d->box->addWidget(d->resizer, 0, Qt::AlignBottom);
    }
#endif
    l->addStrut(maxH);
    d->savedStrut = maxH;
    vbox->addSpacing(2);
    d->box->activate();
    update();
}

/*!

  Hides the normal status indications and displays the given \a
  message for the specified number of milli-seconds (\a{timeout}). If
  \a{timeout} is 0 (default), the \a {message} remains displayed until
  the clearMessage() slot is called or until the showMessage() slot is
  called again to change the message.

  Note that showMessage() is called to show temporary explanations of
  tool tip texts, so passing a \a{timeout} of 0 is not sufficient to
  display a \l{permanent message}{permanent message}.

    \sa messageChanged(), currentMessage(), clearMessage()
*/
void QStatusBar::showMessage(const QString &message, int timeout)
{
    Q_D(QStatusBar);

    if (timeout > 0) {
        if (!d->timer) {
            d->timer = new QTimer(this);
            connect(d->timer, SIGNAL(timeout()), this, SLOT(clearMessage()));
        }
        d->timer->start(timeout);
    } else if (d->timer) {
        delete d->timer;
        d->timer = 0;
    }
    if (d->tempItem == message)
        return;
    d->tempItem = message;

    hideOrShow();
}

/*!
    Removes any temporary message being shown.

    \sa currentMessage(), showMessage(), removeWidget()
*/

void QStatusBar::clearMessage()
{
    Q_D(QStatusBar);
    if (d->tempItem.isEmpty())
        return;
    if (d->timer) {
        qDeleteInEventHandler(d->timer);
        d->timer = 0;
    }
    d->tempItem.clear();
    hideOrShow();
}

/*!
    Returns the temporary message currently shown,
    or an empty string if there is no such message.

    \sa showMessage()
*/
QString QStatusBar::currentMessage() const
{
    Q_D(const QStatusBar);
    return d->tempItem;
}

/*!
    \fn QStatusBar::messageChanged(const QString &message)

    This signal is emitted whenever the temporary status message
    changes. The new temporary message is passed in the \a message
    parameter which is a null-string when the message has been
    removed.

    \sa showMessage(), clearMessage()
*/

/*!
    Ensures that the right widgets are visible.

    Used by the showMessage() and clearMessage() functions.
*/
void QStatusBar::hideOrShow()
{
    Q_D(QStatusBar);
    bool haveMessage = !d->tempItem.isEmpty();

    QStatusBarPrivate::SBItem* item = 0;
    for (int i=0; i<d->items.size(); ++i) {
        item = d->items.at(i);
        if (!item || item->p)
            break;
        if (haveMessage && item->w->isVisible()) {
            item->w->hide();
            item->w->setAttribute(Qt::WA_WState_ExplicitShowHide, false);
        } else if (!haveMessage && !item->w->testAttribute(Qt::WA_WState_ExplicitShowHide)) {
            item->w->show();
        }
    }

    emit messageChanged(d->tempItem);

#ifndef QT_NO_ACCESSIBILITY
    if (QAccessible::isActive()) {
        QAccessibleEvent event(this, QAccessible::NameChanged);
        QAccessible::updateAccessibility(&event);
    }
#endif

    repaint(d->messageRect());
}

/*!
  \reimp
 */
void QStatusBar::showEvent(QShowEvent *)
{
#ifndef QT_NO_SIZEGRIP
    Q_D(QStatusBar);
    if (d->resizer && d->showSizeGrip)
        d->tryToShowSizeGrip();
#endif
}

/*!
    \reimp
    \fn void QStatusBar::paintEvent(QPaintEvent *event)

    Shows the temporary message, if appropriate, in response to the
    paint \a event.
*/
void QStatusBar::paintEvent(QPaintEvent *event)
{
    Q_D(QStatusBar);
    bool haveMessage = !d->tempItem.isEmpty();

    QPainter p(this);
    QStyleOption opt;
    opt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_PanelStatusBar, &opt, &p, this);

    for (int i=0; i<d->items.size(); ++i) {
        QStatusBarPrivate::SBItem* item = d->items.at(i);
        if (item && item->w->isVisible() && (!haveMessage || item->p)) {
            QRect ir = item->w->geometry().adjusted(-2, -1, 2, 1);
            if (event->rect().intersects(ir)) {
                QStyleOption opt(0);
                opt.rect = ir;
                opt.palette = palette();
                opt.state = QStyle::State_None;
                style()->drawPrimitive(QStyle::PE_FrameStatusBarItem, &opt, &p, item->w);
            }
        }
    }
    if (haveMessage) {
        p.setPen(palette().foreground().color());
        p.drawText(d->messageRect(), Qt::AlignLeading | Qt::AlignVCenter | Qt::TextSingleLine, d->tempItem);
    }
}

/*!
    \reimp
*/
void QStatusBar::resizeEvent(QResizeEvent * e)
{
    QWidget::resizeEvent(e);
}

/*!
    \reimp
*/

bool QStatusBar::event(QEvent *e)
{
    Q_D(QStatusBar);

    if (e->type() == QEvent::LayoutRequest
        ) {
        // Calculate new strut height and call reformat() if it has changed
        int maxH = fontMetrics().height();

        QStatusBarPrivate::SBItem* item = 0;
        for (int i=0; i<d->items.size(); ++i) {
            item = d->items.at(i);
            if (!item)
                break;
            int itemH = qMin(qSmartMinSize(item->w).height(), item->w->maximumHeight());
            maxH = qMax(maxH, itemH);
        }

#ifndef QT_NO_SIZEGRIP
        if (d->resizer)
            maxH = qMax(maxH, d->resizer->sizeHint().height());
#endif

        if (maxH != d->savedStrut)
            reformat();
        else
            update();
    }
    if (e->type() == QEvent::ChildRemoved) {
        QStatusBarPrivate::SBItem* item = 0;
        for (int i=0; i<d->items.size(); ++i) {
            item = d->items.at(i);
            if (!item)
                break;
            if (item->w == ((QChildEvent*)e)->child()) {
                d->items.removeAt(i);
                delete item;
            }
        }
    }

// On Mac OS X Leopard it is possible to drag the window by clicking
// on the tool bar on most applications.
#ifndef Q_WS_MAC
    return QWidget::event(e);
#else
    if (QSysInfo::MacintoshVersion <= QSysInfo::MV_10_4)
        return QWidget::event(e);

    // Enable drag-click only if the status bar is the status bar for a
    // QMainWindow with a unifed toolbar.
    if (parent() == 0 || qobject_cast<QMainWindow *>(parent()) == 0 ||
        qobject_cast<QMainWindow *>(parent())->unifiedTitleAndToolBarOnMac() == false )
        return QWidget::event(e);

    // Check for mouse events.
    QMouseEvent *mouseEvent;
    if (e->type() == QEvent::MouseButtonPress ||
        e->type() == QEvent::MouseMove ||
        e->type() == QEvent::MouseButtonRelease) {
        mouseEvent = static_cast <QMouseEvent*>(e);
    } else {
        return QWidget::event(e);
    }

    // The following is a standard mouse drag handler.
    if (e->type() == QEvent::MouseButtonPress && (mouseEvent->button() == Qt::LeftButton)) {
        d->dragStart = mouseEvent->pos();
    } else if (e->type() == QEvent::MouseMove){
        if (d->dragStart == QPoint())
            return QWidget::event(e);
        QPoint pos = mouseEvent->pos();
        QPoint delta = (pos - d->dragStart);
        window()->move(window()->pos() + delta);
    } else if (e->type() == QEvent::MouseButtonRelease && (mouseEvent->button() == Qt::LeftButton)){
        d->dragStart = QPoint();
    } else {
        return QWidget::event(e);
    }

    return true;
#endif
}

QT_END_NAMESPACE

#endif
