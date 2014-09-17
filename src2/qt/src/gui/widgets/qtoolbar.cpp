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

#include "qtoolbar.h"

#ifndef QT_NO_TOOLBAR

#include <qapplication.h>
#include <qcombobox.h>
#include <qevent.h>
#include <qlayout.h>
#include <qmainwindow.h>
#include <qmenu.h>
#include <qmenubar.h>
#include <qrubberband.h>
#include <qsignalmapper.h>
#include <qstylepainter.h>
#include <qtoolbutton.h>
#include <qwidgetaction.h>
#include <qtimer.h>
#include <private/qwidgetaction_p.h>
#ifdef Q_WS_MAC
#include <private/qt_mac_p.h>
#include <private/qt_cocoa_helpers_mac_p.h>
#endif

#include <private/qmainwindowlayout_p.h>

#include "qtoolbar_p.h"
#include "qtoolbarseparator_p.h"
#include "qtoolbarlayout_p.h"

#include  "qdebug.h"

#define POPUP_TIMER_INTERVAL 500

QT_BEGIN_NAMESPACE

#ifdef Q_WS_MAC
static void qt_mac_updateToolBarButtonHint(QWidget *parentWidget)
{
    if (!(parentWidget->windowFlags() & Qt::CustomizeWindowHint))
        parentWidget->setWindowFlags(parentWidget->windowFlags() | Qt::MacWindowToolBarButtonHint);
}
#endif

// qmainwindow.cpp
extern QMainWindowLayout *qt_mainwindow_layout(const QMainWindow *window);

/******************************************************************************
** QToolBarPrivate
*/

void QToolBarPrivate::init()
{
    Q_Q(QToolBar);
    q->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
    q->setBackgroundRole(QPalette::Button);
    q->setAttribute(Qt::WA_Hover);
    q->setAttribute(Qt::WA_X11NetWmWindowTypeToolBar);

    QStyle *style = q->style();
    int e = style->pixelMetric(QStyle::PM_ToolBarIconSize, 0, q);
    iconSize = QSize(e, e);

    layout = new QToolBarLayout(q);
    layout->updateMarginAndSpacing();

#ifdef Q_WS_MAC
    if (q->parentWidget() && q->parentWidget()->isWindow()) {
        // Make sure that the window has the "toolbar" button.
        QWidget *parentWidget = q->parentWidget();
        qt_mac_updateToolBarButtonHint(parentWidget);
        reinterpret_cast<QToolBar *>(parentWidget)->d_func()->createWinId(); // Please let me create your winId...
        extern OSWindowRef qt_mac_window_for(const QWidget *); // qwidget_mac.cpp
        macWindowToolbarShow(q->parentWidget(), true);
    }
#endif

    toggleViewAction = new QAction(q);
    toggleViewAction->setCheckable(true);
    q->setMovable(q->style()->styleHint(QStyle::SH_ToolBar_Movable, 0, q ));
    QObject::connect(toggleViewAction, SIGNAL(triggered(bool)), q, SLOT(_q_toggleView(bool)));
}

void QToolBarPrivate::_q_toggleView(bool b)
{
    Q_Q(QToolBar);
    if (b == q->isHidden()) {
        if (b)
            q->show();
        else
            q->close();
    }
}

void QToolBarPrivate::_q_updateIconSize(const QSize &sz)
{
    Q_Q(QToolBar);
    if (!explicitIconSize) {
        // iconSize not explicitly set
        q->setIconSize(sz);
        explicitIconSize = false;
    }
}

void QToolBarPrivate::_q_updateToolButtonStyle(Qt::ToolButtonStyle style)
{
    Q_Q(QToolBar);
    if (!explicitToolButtonStyle) {
        q->setToolButtonStyle(style);
        explicitToolButtonStyle = false;
    }
}

void QToolBarPrivate::updateWindowFlags(bool floating, bool unplug)
{
    Q_Q(QToolBar);
    Qt::WindowFlags flags = floating ? Qt::Tool : Qt::Widget;

    flags |= Qt::FramelessWindowHint;

    if (unplug) {
        flags |= Qt::X11BypassWindowManagerHint;
#ifdef Q_WS_MAC
        flags |= Qt::WindowStaysOnTopHint;
#endif
    }

    q->setWindowFlags(flags);
}

void QToolBarPrivate::setWindowState(bool floating, bool unplug, const QRect &rect)
{
    Q_Q(QToolBar);
    bool visible = !q->isHidden();
    bool wasFloating = q->isFloating(); // ...is also currently using popup menus

    q->hide();

    updateWindowFlags(floating, unplug);

    if (floating != wasFloating)
        layout->checkUsePopupMenu();

    if (!rect.isNull())
        q->setGeometry(rect);

    if (visible)
        q->show();

    if (floating != wasFloating)
        emit q->topLevelChanged(floating);
}

void QToolBarPrivate::initDrag(const QPoint &pos)
{
    Q_Q(QToolBar);

    if (state != 0)
        return;

    QMainWindow *win = qobject_cast<QMainWindow*>(parent);
    Q_ASSERT(win != 0);
    QMainWindowLayout *layout = qt_mainwindow_layout(win);
    Q_ASSERT(layout != 0);
    if (layout->pluggingWidget != 0) // the main window is animating a docking operation
        return;

    state = new DragState;
    state->pressPos = pos;
    state->dragging = false;
    state->moving = false;
    state->widgetItem = 0;

    if (q->isRightToLeft())
        state->pressPos = QPoint(q->width() - state->pressPos.x(), state->pressPos.y());
}

void QToolBarPrivate::startDrag(bool moving)
{
    Q_Q(QToolBar);

    Q_ASSERT(state != 0);

    if ((moving && state->moving) || state->dragging)
        return;

    QMainWindow *win = qobject_cast<QMainWindow*>(parent);
    Q_ASSERT(win != 0);
    QMainWindowLayout *layout = qt_mainwindow_layout(win);
    Q_ASSERT(layout != 0);

    if (!moving) {
        state->widgetItem = layout->unplug(q);
#if defined(Q_WS_MAC) && !defined(QT_MAC_USE_COCOA)
        if (q->isWindow()) {
           setWindowState(true, true); //set it to floating
        }
#endif
        Q_ASSERT(state->widgetItem != 0);
    }
    state->dragging = !moving;
    state->moving = moving;
}

void QToolBarPrivate::endDrag()
{
    Q_Q(QToolBar);
    Q_ASSERT(state != 0);

    q->releaseMouse();

    if (state->dragging) {
        QMainWindowLayout *layout = qt_mainwindow_layout(qobject_cast<QMainWindow *>(q->parentWidget()));
        Q_ASSERT(layout != 0);

        if (!layout->plug(state->widgetItem)) {
            if (q->isFloatable()) {
                layout->restore();
#if defined(Q_WS_X11) || defined(Q_WS_MAC)
                setWindowState(true); // gets rid of the X11BypassWindowManager window flag
                                      // and activates the resizer
#endif
                q->activateWindow();
            } else {
                layout->revert(state->widgetItem);
            }
        }
    }

    delete state;
    state = 0;
}

bool QToolBarPrivate::mousePressEvent(QMouseEvent *event)
{
    Q_Q(QToolBar);
    QStyleOptionToolBar opt;
    q->initStyleOption(&opt);
    if (q->style()->subElementRect(QStyle::SE_ToolBarHandle, &opt, q).contains(event->pos()) == false) {
#ifdef Q_WS_MAC
        // When using the unified toolbar on Mac OS X the user can can click and
        // drag between toolbar contents to move the window. Make this work by
        // implementing the standard mouse-dragging code and then call
        // window->move() in mouseMoveEvent below.
        if (QMainWindow *mainWindow = qobject_cast<QMainWindow *>(parent)) {
            if (mainWindow->toolBarArea(q) == Qt::TopToolBarArea
                    && mainWindow->unifiedTitleAndToolBarOnMac()
                    && q->childAt(event->pos()) == 0) {
                macWindowDragging = true;
                macWindowDragPressPosition = event->pos();
                return true;
            }
        }
#endif
        return false;
    }

    if (event->button() != Qt::LeftButton)
        return true;

    if (!layout->movable())
        return true;

    initDrag(event->pos());
    return true;
}

bool QToolBarPrivate::mouseReleaseEvent(QMouseEvent*)
{
    if (state != 0) {
        endDrag();
        return true;
    } else {
#ifdef Q_WS_MAC
        if (!macWindowDragging)
            return false;
        macWindowDragging = false;
        macWindowDragPressPosition = QPoint();
        return true;
#endif
        return false;
    }
}

bool QToolBarPrivate::mouseMoveEvent(QMouseEvent *event)
{
    Q_Q(QToolBar);

    if (!state) {
#ifdef Q_WS_MAC
        if (!macWindowDragging)
            return false;
        QWidget *w = q->window();
        const QPoint delta = event->pos() - macWindowDragPressPosition;
        w->move(w->pos() + delta);
        return true;
#endif
        return false;
    }

    QMainWindow *win = qobject_cast<QMainWindow*>(parent);
    if (win == 0)
        return true;

    QMainWindowLayout *layout = qt_mainwindow_layout(win);
    Q_ASSERT(layout != 0);

    if (layout->pluggingWidget == 0
        && (event->pos() - state->pressPos).manhattanLength() > QApplication::startDragDistance()) {
            const bool wasDragging = state->dragging;
            const bool moving = !q->isWindow() && (orientation == Qt::Vertical ?
                event->x() >= 0 && event->x() < q->width() :
                event->y() >= 0 && event->y() < q->height());

            startDrag(moving);
            if (!moving && !wasDragging) {
#ifdef Q_OS_WIN
                grabMouseWhileInWindow();
#else
                q->grabMouse();
#endif
            }
    }

    if (state->dragging) {
        QPoint pos = event->globalPos();
        // if we are right-to-left, we move so as to keep the right edge the same distance
        // from the mouse
        if (q->isLeftToRight())
            pos -= state->pressPos;
        else
            pos += QPoint(state->pressPos.x() - q->width(), -state->pressPos.y());

        q->move(pos);
        layout->hover(state->widgetItem, event->globalPos());
    } else if (state->moving) {

        const QPoint rtl(q->width() - state->pressPos.x(), state->pressPos.y()); //for RTL
        const QPoint globalPressPos = q->mapToGlobal(q->isRightToLeft() ? rtl : state->pressPos);
        int pos = 0;

        QPoint delta = event->globalPos() - globalPressPos;
        if (orientation == Qt::Vertical) {
            pos = q->y() + delta.y();
        } else {
            if (q->isRightToLeft()) {
                pos = win->width() - q->width() - q->x()  - delta.x();
            } else {
                pos = q->x() + delta.x();
            }
        }

        layout->moveToolBar(q, pos);
    }
    return true;
}

void QToolBarPrivate::unplug(const QRect &_r)
{
    Q_Q(QToolBar);
    QRect r = _r;
    r.moveTopLeft(q->mapToGlobal(QPoint(0, 0)));
    setWindowState(true, true, r);
    layout->setExpanded(false);
}

void QToolBarPrivate::plug(const QRect &r)
{
    setWindowState(false, false, r);
}

/******************************************************************************
** QToolBar
*/

/*!
    \class QToolBar

    \brief The QToolBar class provides a movable panel that contains a
    set of controls.

    \ingroup mainwindow-classes


    Toolbar buttons are added by adding \e actions, using addAction()
    or insertAction(). Groups of buttons can be separated using
    addSeparator() or insertSeparator(). If a toolbar button is not
    appropriate, a widget can be inserted instead using addWidget() or
    insertWidget(); examples of suitable widgets are QSpinBox,
    QDoubleSpinBox, and QComboBox. When a toolbar button is pressed it
    emits the actionTriggered() signal.

    A toolbar can be fixed in place in a particular area (e.g. at the
    top of the window), or it can be movable (isMovable()) between
    toolbar areas; see allowedAreas() and isAreaAllowed().

    When a toolbar is resized in such a way that it is too small to
    show all the items it contains, an extension button will appear as
    the last item in the toolbar. Pressing the extension button will
    pop up a menu containing the items that does not currently fit in
    the toolbar.

    When a QToolBar is not a child of a QMainWindow, it loses the ability
    to populate the extension pop up with widgets added to the toolbar using
    addWidget(). Please use widget actions created by inheriting QWidgetAction
    and implementing QWidgetAction::createWidget() instead.

    \sa QToolButton, QMenu, QAction, {Application Example}
*/

/*!
    \fn bool QToolBar::isAreaAllowed(Qt::ToolBarArea area) const

    Returns true if this toolbar is dockable in the given \a area;
    otherwise returns false.
*/

/*!
    \fn void QToolBar::addAction(QAction *action)
    \overload

    Appends the action \a action to the toolbar's list of actions.

    \sa QMenu::addAction(), QWidget::addAction()
*/

/*!
    \fn void QToolBar::actionTriggered(QAction *action)

    This signal is emitted when an action in this toolbar is triggered.
    This happens when the action's tool button is pressed, or when the
    action is triggered in some other way outside the tool bar. The parameter
    holds the triggered \a action.
*/

/*!
    \fn void QToolBar::allowedAreasChanged(Qt::ToolBarAreas allowedAreas)

    This signal is emitted when the collection of allowed areas for the
    toolbar is changed. The new areas in which the toolbar can be positioned
    are specified by \a allowedAreas.

    \sa allowedAreas
*/

/*!
    \fn void QToolBar::iconSizeChanged(const QSize &iconSize)

    This signal is emitted when the icon size is changed.  The \a
    iconSize parameter holds the toolbar's new icon size.

    \sa iconSize QMainWindow::iconSize
*/

/*!
    \fn void QToolBar::movableChanged(bool movable)

    This signal is emitted when the toolbar becomes movable or fixed.
    If the toolbar can be moved, \a movable is true; otherwise it is
    false.

    \sa movable
*/

/*!
    \fn void QToolBar::orientationChanged(Qt::Orientation orientation)

    This signal is emitted when the orientation of the toolbar changes.
    The new orientation is specified by the \a orientation given.

    \sa orientation
*/

/*!
    \fn void QToolBar::toolButtonStyleChanged(Qt::ToolButtonStyle toolButtonStyle)

    This signal is emitted when the tool button style is changed. The
    \a toolButtonStyle parameter holds the toolbar's new tool button
    style.

    \sa toolButtonStyle QMainWindow::toolButtonStyle
*/

/*!
    \since 4.6

    \fn void QToolBar::topLevelChanged(bool topLevel)

    This signal is emitted when the \l floating property changes.
    The \a topLevel parameter is true if the toolbar is now floating;
    otherwise it is false.

    \sa isWindow()
*/


/*!
    \fn void QToolBar::visibilityChanged(bool visible)
    \since 4.7

    This signal is emitted when the toolbar becomes \a visible (or
    invisible). This happens when the widget is hidden or shown.
*/

/*!
    Constructs a QToolBar with the given \a parent.
*/
QToolBar::QToolBar(QWidget *parent)
    : QWidget(*new QToolBarPrivate, parent, 0)
{
    Q_D(QToolBar);
    d->init();
}

/*!
    Constructs a QToolBar with the given \a parent.

    The given window \a title identifies the toolbar and is shown in
    the context menu provided by QMainWindow.

    \sa setWindowTitle()
*/
QToolBar::QToolBar(const QString &title, QWidget *parent)
    : QWidget(*new QToolBarPrivate, parent, 0)
{
    Q_D(QToolBar);
    d->init();
    setWindowTitle(title);
}

#ifdef QT3_SUPPORT
/*! \obsolete
    Constructs a QToolBar with the given \a parent and \a name.
*/
QToolBar::QToolBar(QWidget *parent, const char *name)
    : QWidget(*new QToolBarPrivate, parent, 0)
{
    Q_D(QToolBar);
    d->init();
    setObjectName(QString::fromAscii(name));
}
#endif

/*!
    Destroys the toolbar.
*/
QToolBar::~QToolBar()
{
    // Remove the toolbar button if there is nothing left.
    QMainWindow *mainwindow = qobject_cast<QMainWindow *>(parentWidget());
    if (mainwindow) {
#ifdef Q_WS_MAC
        QMainWindowLayout *mainwin_layout = qt_mainwindow_layout(mainwindow);
        if (mainwin_layout && mainwin_layout->layoutState.toolBarAreaLayout.isEmpty()
                && mainwindow->testAttribute(Qt::WA_WState_Created))
            macWindowToolbarShow(mainwindow, false);
#endif
    }
}

/*! \property QToolBar::movable
    \brief whether the user can move the toolbar within the toolbar area,
    or between toolbar areas

    By default, this property is true.

    This property only makes sense if the toolbar is in a
    QMainWindow.

    \sa allowedAreas
*/

void QToolBar::setMovable(bool movable)
{
    Q_D(QToolBar);
    if (!movable == !d->movable)
        return;
    d->movable = movable;
    d->layout->invalidate();
    emit movableChanged(d->movable);
}

bool QToolBar::isMovable() const
{
    Q_D(const QToolBar);
    return d->movable;
}

/*!
    \property QToolBar::floatable
    \brief whether the toolbar can be dragged and dropped as an independent window.

    The default is true.
*/
bool QToolBar::isFloatable() const
{
    Q_D(const QToolBar);
    return d->floatable;
}

void QToolBar::setFloatable(bool floatable)
{
    Q_D(QToolBar);
    d->floatable = floatable;
}

/*!
    \property QToolBar::floating
    \brief whether the toolbar is an independent window.

    By default, this property is true.

    \sa QWidget::isWindow()
*/
bool QToolBar::isFloating() const
{
    return isWindow();
}

/*!
    \property QToolBar::allowedAreas
    \brief areas where the toolbar may be placed

    The default is Qt::AllToolBarAreas.

    This property only makes sense if the toolbar is in a
    QMainWindow.

    \sa movable
*/

void QToolBar::setAllowedAreas(Qt::ToolBarAreas areas)
{
    Q_D(QToolBar);
    areas &= Qt::ToolBarArea_Mask;
    if (areas == d->allowedAreas)
        return;
    d->allowedAreas = areas;
    emit allowedAreasChanged(d->allowedAreas);
}

Qt::ToolBarAreas QToolBar::allowedAreas() const
{
    Q_D(const QToolBar);
#ifdef Q_WS_MAC
    if (QMainWindow *window = qobject_cast<QMainWindow *>(parentWidget())) {
        if (window->unifiedTitleAndToolBarOnMac()) // Don't allow drags to the top (for now).
            return (d->allowedAreas & ~Qt::TopToolBarArea);
    }
#endif
    return d->allowedAreas;
}

/*! \property QToolBar::orientation
    \brief orientation of the toolbar

    The default is Qt::Horizontal.

    This function should not be used when the toolbar is managed
    by QMainWindow. You can use QMainWindow::addToolBar() or
    QMainWindow::insertToolBar() if you wish to move a toolbar (that
    is already added to a main window) to another Qt::ToolBarArea.
*/

void QToolBar::setOrientation(Qt::Orientation orientation)
{
    Q_D(QToolBar);
    if (orientation == d->orientation)
        return;

    d->orientation = orientation;

    if (orientation == Qt::Vertical)
 	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred));
    else
 	setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));

    d->layout->invalidate();
    d->layout->activate();

    emit orientationChanged(d->orientation);
}

Qt::Orientation QToolBar::orientation() const
{ Q_D(const QToolBar); return d->orientation; }

/*!
    \property QToolBar::iconSize
    \brief size of icons in the toolbar.

    The default size is determined by the application's style and is
    derived from the QStyle::PM_ToolBarIconSize pixel metric. It is
    the maximum size an icon can have. Icons of smaller size will not
    be scaled up.
*/

QSize QToolBar::iconSize() const
{ Q_D(const QToolBar); return d->iconSize; }

void QToolBar::setIconSize(const QSize &iconSize)
{
    Q_D(QToolBar);
    QSize sz = iconSize;
    if (!sz.isValid()) {
        QMainWindow *mw = qobject_cast<QMainWindow *>(parentWidget());
        if (mw && mw->layout()) {
            QLayout *layout = mw->layout();
            int i = 0;
            QLayoutItem *item = 0;
            do {
                item = layout->itemAt(i++);
                if (item && (item->widget() == this))
                    sz = mw->iconSize();
            } while (!sz.isValid() && item != 0);
        }
    }
    if (!sz.isValid()) {
        const int metric = style()->pixelMetric(QStyle::PM_ToolBarIconSize, 0, this);
        sz = QSize(metric, metric);
    }
    if (d->iconSize != sz) {
        d->iconSize = sz;
        setMinimumSize(0, 0);
        emit iconSizeChanged(d->iconSize);
    }
    d->explicitIconSize = iconSize.isValid();

    d->layout->invalidate();
}

/*!
    \property QToolBar::toolButtonStyle
    \brief the style of toolbar buttons

    This property defines the style of all tool buttons that are added
    as \l{QAction}s. Note that if you add a QToolButton with the
    addWidget() method, it will not get this button style.

    The default is Qt::ToolButtonIconOnly.
*/

Qt::ToolButtonStyle QToolBar::toolButtonStyle() const
{ Q_D(const QToolBar); return d->toolButtonStyle; }

void QToolBar::setToolButtonStyle(Qt::ToolButtonStyle toolButtonStyle)
{
    Q_D(QToolBar);
    d->explicitToolButtonStyle = true;
    if (d->toolButtonStyle == toolButtonStyle)
        return;
    d->toolButtonStyle = toolButtonStyle;
    setMinimumSize(0, 0);
    emit toolButtonStyleChanged(d->toolButtonStyle);
}

/*!
    Removes all actions from the toolbar.

    \sa removeAction()
*/
void QToolBar::clear()
{
    QList<QAction *> actions = this->actions();
    for(int i = 0; i < actions.size(); i++)
        removeAction(actions.at(i));
}

/*!
    \overload

    Creates a new action with the given \a text. This action is added to
    the end of the toolbar.
*/
QAction *QToolBar::addAction(const QString &text)
{
    QAction *action = new QAction(text, this);
    addAction(action);
    return action;
}

/*!
    \overload

    Creates a new action with the given \a icon and \a text. This
    action is added to the end of the toolbar.
*/
QAction *QToolBar::addAction(const QIcon &icon, const QString &text)
{
    QAction *action = new QAction(icon, text, this);
    addAction(action);
    return action;
}

/*!
    \overload

    Creates a new action with the given \a text. This action is added to
    the end of the toolbar. The action's \link QAction::triggered()
    triggered()\endlink signal is connected to \a member in \a
    receiver.
*/
QAction *QToolBar::addAction(const QString &text,
                             const QObject *receiver, const char* member)
{
    QAction *action = new QAction(text, this);
    QObject::connect(action, SIGNAL(triggered(bool)), receiver, member);
    addAction(action);
    return action;
}

/*!
    \overload

    Creates a new action with the icon \a icon and text \a text. This
    action is added to the end of the toolbar. The action's \link
    QAction::triggered() triggered()\endlink signal is connected to \a
    member in \a receiver.
*/
QAction *QToolBar::addAction(const QIcon &icon, const QString &text,
                             const QObject *receiver, const char* member)
{
    QAction *action = new QAction(icon, text, this);
    QObject::connect(action, SIGNAL(triggered(bool)), receiver, member);
    addAction(action);
    return action;
}

/*!
     Adds a separator to the end of the toolbar.

     \sa insertSeparator()
*/
QAction *QToolBar::addSeparator()
{
    QAction *action = new QAction(this);
    action->setSeparator(true);
    addAction(action);
    return action;
}

/*!
    Inserts a separator into the toolbar in front of the toolbar
    item associated with the \a before action.

    \sa addSeparator()
*/
QAction *QToolBar::insertSeparator(QAction *before)
{
    QAction *action = new QAction(this);
    action->setSeparator(true);
    insertAction(before, action);
    return action;
}

/*!
    Adds the given \a widget to the toolbar as the toolbar's last
    item.

    The toolbar takes ownership of \a widget.

    If you add a QToolButton with this method, the tools bar's
    Qt::ToolButtonStyle will not be respected.

    \note You should use QAction::setVisible() to change the
    visibility of the widget. Using QWidget::setVisible(),
    QWidget::show() and QWidget::hide() does not work.

    \sa insertWidget()
*/
QAction *QToolBar::addWidget(QWidget *widget)
{
    QWidgetAction *action = new QWidgetAction(this);
    action->setDefaultWidget(widget);
    action->d_func()->autoCreated = true;
    addAction(action);
    return action;
}

/*!
    Inserts the given \a widget in front of the toolbar item
    associated with the \a before action.

    Note: You should use QAction::setVisible() to change the
    visibility of the widget. Using QWidget::setVisible(),
    QWidget::show() and QWidget::hide() does not work.

    \sa addWidget()
*/
QAction *QToolBar::insertWidget(QAction *before, QWidget *widget)
{
    QWidgetAction *action = new QWidgetAction(this);
    action->setDefaultWidget(widget);
    action->d_func()->autoCreated = true;
    insertAction(before, action);
    return action;
}

/*!
    \internal

    Returns the geometry of the toolbar item associated with the given
    \a action, or an invalid QRect if no matching item is found.
*/
QRect QToolBar::actionGeometry(QAction *action) const
{
    Q_D(const QToolBar);

    int index = d->layout->indexOf(action);
    if (index == -1)
        return QRect();
    return d->layout->itemAt(index)->widget()->geometry();
}

/*!
    Returns the action at point \a p. This function returns zero if no
    action was found.

    \sa QWidget::childAt()
*/
QAction *QToolBar::actionAt(const QPoint &p) const
{
    Q_D(const QToolBar);
    QWidget *widget = childAt(p);
    int index = d->layout->indexOf(widget);
    if (index == -1)
        return 0;
    QLayoutItem *item = d->layout->itemAt(index);
    return static_cast<QToolBarItem*>(item)->action;
}

/*! \fn QAction *QToolBar::actionAt(int x, int y) const
    \overload

    Returns the action at the point \a x, \a y. This function returns
    zero if no action was found.
*/

/*! \reimp */
void QToolBar::actionEvent(QActionEvent *event)
{
    Q_D(QToolBar);
    QAction *action = event->action();
    QWidgetAction *widgetAction = qobject_cast<QWidgetAction *>(action);

    switch (event->type()) {
        case QEvent::ActionAdded: {
            Q_ASSERT_X(widgetAction == 0 || d->layout->indexOf(widgetAction) == -1,
                        "QToolBar", "widgets cannot be inserted multiple times");

            // reparent the action to this toolbar if it has been created
            // using the addAction(text) etc. convenience functions, to
            // preserve Qt 4.1.x behavior. The widget is already
            // reparented to us due to the createWidget call inside
            // createItem()
            if (widgetAction != 0 && widgetAction->d_func()->autoCreated)
                widgetAction->setParent(this);

            int index = d->layout->count();
            if (event->before()) {
                index = d->layout->indexOf(event->before());
                Q_ASSERT_X(index != -1, "QToolBar::insertAction", "internal error");
            }
            d->layout->insertAction(index, action);
            break;
        }

        case QEvent::ActionChanged:
            d->layout->invalidate();
            break;

        case QEvent::ActionRemoved: {
            int index = d->layout->indexOf(action);
            if (index != -1) {
                delete d->layout->takeAt(index);
            }
            break;
        }

        default:
            Q_ASSERT_X(false, "QToolBar::actionEvent", "internal error");
    }
}

/*! \reimp */
void QToolBar::changeEvent(QEvent *event)
{
    Q_D(QToolBar);
    switch (event->type()) {
    case QEvent::WindowTitleChange:
        d->toggleViewAction->setText(windowTitle());
        break;
    case QEvent::StyleChange:
        d->layout->invalidate();
        if (!d->explicitIconSize)
            setIconSize(QSize());
        d->layout->updateMarginAndSpacing();
        break;
    case QEvent::LayoutDirectionChange:
        d->layout->invalidate();
        break;
    default:
        break;
    }
    QWidget::changeEvent(event);
}

/*! \reimp */
void QToolBar::paintEvent(QPaintEvent *)
{
    Q_D(QToolBar);

    QPainter p(this);
    QStyle *style = this->style();
    QStyleOptionToolBar opt;
    initStyleOption(&opt);

    if (d->layout->expanded || d->layout->animating || isWindow()) {
        //if the toolbar is expended, we need to fill the background with the window color
        //because some styles may expects that.
        p.fillRect(opt.rect, palette().background());
        style->drawControl(QStyle::CE_ToolBar, &opt, &p, this);
        style->drawPrimitive(QStyle::PE_FrameMenu, &opt, &p, this);
    } else {
        style->drawControl(QStyle::CE_ToolBar, &opt, &p, this);
    }

    opt.rect = style->subElementRect(QStyle::SE_ToolBarHandle, &opt, this);
    if (opt.rect.isValid())
        style->drawPrimitive(QStyle::PE_IndicatorToolBarHandle, &opt, &p, this);
}

/*
    Checks if an expanded toolbar has to wait for this popup to close before
    the toolbar collapses. This is true if
    1) the popup has the toolbar in its parent chain,
    2) the popup is a menu whose menuAction is somewhere in the toolbar.
*/
static bool waitForPopup(QToolBar *tb, QWidget *popup)
{
    if (popup == 0 || popup->isHidden())
        return false;

    QWidget *w = popup;
    while (w != 0) {
        if (w == tb)
            return true;
        w = w->parentWidget();
    }

    QMenu *menu = qobject_cast<QMenu*>(popup);
    if (menu == 0)
        return false;

    QAction *action = menu->menuAction();
    QList<QWidget*> widgets = action->associatedWidgets();
    for (int i = 0; i < widgets.count(); ++i) {
        if (waitForPopup(tb, widgets.at(i)))
            return true;
    }

    return false;
}

#if defined(Q_WS_MAC)
static bool toolbarInUnifiedToolBar(QToolBar *toolbar)
{
    const QMainWindow *mainWindow = qobject_cast<const QMainWindow *>(toolbar->parentWidget());
    return mainWindow && mainWindow->unifiedTitleAndToolBarOnMac()
            && mainWindow->toolBarArea(toolbar) == Qt::TopToolBarArea;
}
#endif

/*! \reimp */
bool QToolBar::event(QEvent *event)
{
    Q_D(QToolBar);

    switch (event->type()) {
    case QEvent::Timer:
        if (d->waitForPopupTimer.timerId() == static_cast<QTimerEvent*>(event)->timerId()) {
            QWidget *w = QApplication::activePopupWidget();
            if (!waitForPopup(this, w)) {
                d->waitForPopupTimer.stop();
                if (!this->underMouse())
                    d->layout->setExpanded(false);
            }
        }
        break;
    case QEvent::Hide:
        if (!isHidden())
            break;
        // fallthrough intended
    case QEvent::Show:
        d->toggleViewAction->setChecked(event->type() == QEvent::Show);
        emit visibilityChanged(event->type() == QEvent::Show);
#if defined(Q_WS_MAC)
        if (toolbarInUnifiedToolBar(this)) {
             // I can static_cast because I did the qobject_cast in the if above, therefore
            // we must have a QMainWindowLayout here.
            QMainWindowLayout *mwLayout = qt_mainwindow_layout(qobject_cast<QMainWindow *>(parentWidget()));
            mwLayout->fixSizeInUnifiedToolbar(this);
            mwLayout->syncUnifiedToolbarVisibility();
        }
#  if !defined(QT_MAC_USE_COCOA)
        // Fall through
    case QEvent::LayoutRequest: {
        // There's currently no way to invalidate the size and let
        // HIToolbar know about it. This forces a re-check.
        int earlyResult = -1;
        if (QMainWindow *mainWindow = qobject_cast<QMainWindow *>(parentWidget())) {
            bool needUpdate = true;
            if (event->type() == QEvent::LayoutRequest) {
                QSize oldSizeHint = sizeHint();
                earlyResult = QWidget::event(event) ? 1 : 0;
                needUpdate = oldSizeHint != sizeHint();
            }

            if (needUpdate) {
                OSWindowRef windowRef = qt_mac_window_for(mainWindow);
                if (toolbarInUnifiedToolBar(this)
                        && macWindowToolbarIsVisible(windowRef))   {
                    DisableScreenUpdates();
                    macWindowToolbarShow(this, false);
                    macWindowToolbarShow(this, true);
                    EnableScreenUpdates();
                }
            }

            if (earlyResult != -1)
                return earlyResult;
        }
    }
#  endif // !QT_MAC_USE_COCOA
#endif // Q_WS_MAC
        break;
    case QEvent::ParentChange:
        d->layout->checkUsePopupMenu();
#if defined(Q_WS_MAC)
        if (parentWidget() && parentWidget()->isWindow())
            qt_mac_updateToolBarButtonHint(parentWidget());
#endif
        break;

    case QEvent::MouseButtonPress: {
        if (d->mousePressEvent(static_cast<QMouseEvent*>(event)))
            return true;
        break;
    }
    case QEvent::MouseButtonRelease:
        if (d->mouseReleaseEvent(static_cast<QMouseEvent*>(event)))
            return true;
        break;
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
        // there's nothing special to do here and we don't want to update the whole widget
        return true;
    case QEvent::HoverMove: {
#ifndef QT_NO_CURSOR
        QHoverEvent *e = static_cast<QHoverEvent*>(event);
        QStyleOptionToolBar opt;
        initStyleOption(&opt);
        if (style()->subElementRect(QStyle::SE_ToolBarHandle, &opt, this).contains(e->pos()))
            setCursor(Qt::SizeAllCursor);
        else
            unsetCursor();
#endif
        break;
    }
    case QEvent::MouseMove:
        if (d->mouseMoveEvent(static_cast<QMouseEvent*>(event)))
            return true;
        break;
#ifdef Q_WS_WINCE
    case QEvent::ContextMenu:
        {
            QContextMenuEvent* contextMenuEvent = static_cast<QContextMenuEvent*>(event);
            QWidget* child = childAt(contextMenuEvent->pos());
            QAbstractButton* button = qobject_cast<QAbstractButton*>(child);
            if (button)
                button->setDown(false);
        }
        break;
#endif
    case QEvent::Leave:
        if (d->state != 0 && d->state->dragging) {
#ifdef Q_OS_WIN
            // This is a workaround for loosing the mouse on Vista.
            QPoint pos = QCursor::pos();
            QMouseEvent fake(QEvent::MouseMove, mapFromGlobal(pos), pos, Qt::NoButton,
                             QApplication::mouseButtons(), QApplication::keyboardModifiers());
            d->mouseMoveEvent(&fake);
#endif
        } else {
            if (!d->layout->expanded)
                break;

            QWidget *w = QApplication::activePopupWidget();
            if (waitForPopup(this, w)) {
                d->waitForPopupTimer.start(POPUP_TIMER_INTERVAL, this);
                break;
            }

            d->waitForPopupTimer.stop();
            d->layout->setExpanded(false);
            break;
        }
    default:
        break;
    }
    return QWidget::event(event);
}

/*!
    Returns a checkable action that can be used to show or hide this
    toolbar.

    The action's text is set to the toolbar's window title.

    \sa QAction::text QWidget::windowTitle
*/
QAction *QToolBar::toggleViewAction() const
{ Q_D(const QToolBar); return d->toggleViewAction; }

/*!
    \fn void QToolBar::setLabel(const QString &label)

    Use setWindowTitle() instead.
*/

/*!
    \fn QString QToolBar::label() const

    Use windowTitle() instead.
*/

/*!
    \since 4.2

    Returns the widget associated with the specified \a action.

    \sa addWidget()
*/
QWidget *QToolBar::widgetForAction(QAction *action) const
{
    Q_D(const QToolBar);

    int index = d->layout->indexOf(action);
    if (index == -1)
        return 0;

    return d->layout->itemAt(index)->widget();
}

extern QMainWindowLayout *qt_mainwindow_layout(const QMainWindow *window);

/*!
    \internal
*/
void QToolBar::initStyleOption(QStyleOptionToolBar *option) const
{
    Q_D(const QToolBar);

    if (!option)
        return;

    option->initFrom(this);
    if (orientation() == Qt::Horizontal)
        option->state |= QStyle::State_Horizontal;
    option->lineWidth = style()->pixelMetric(QStyle::PM_ToolBarFrameWidth, 0, this);
    option->features = d->layout->movable()
                        ? QStyleOptionToolBar::Movable
                        : QStyleOptionToolBar::None;
    // if the tool bar is not in a QMainWindow, this will make the painting right
    option->toolBarArea = Qt::NoToolBarArea;

    // Add more styleoptions if the toolbar has been added to a mainwindow.
    QMainWindow *mainWindow = qobject_cast<QMainWindow *>(parentWidget());

    if (!mainWindow)
        return;

    QMainWindowLayout *layout = qt_mainwindow_layout(mainWindow);
    Q_ASSERT_X(layout != 0, "QToolBar::initStyleOption()",
               "QMainWindow->layout() != QMainWindowLayout");

    layout->getStyleOptionInfo(option, const_cast<QToolBar *>(this));
}

/*!
    \reimp
*/
void QToolBar::childEvent(QChildEvent *event) // ### remove me in 5.0
{
    QWidget::childEvent(event);
}

/*!
    \reimp
*/
void QToolBar::resizeEvent(QResizeEvent *event) // ### remove me in 5.0
{
    QWidget::resizeEvent(event);
}

QT_END_NAMESPACE

#include "moc_qtoolbar.cpp"

#endif // QT_NO_TOOLBAR
