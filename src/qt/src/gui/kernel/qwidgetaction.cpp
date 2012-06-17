/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwidgetaction.h"
#include "qdebug.h"

#ifndef QT_NO_ACTION
#include "qwidgetaction_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QWidgetAction
    \since 4.2
    \brief The QWidgetAction class extends QAction by an interface
    for inserting custom widgets into action based containers, such
    as toolbars.

    \ingroup mainwindow-classes

    Most actions in an application are represented as items in menus or
    buttons in toolbars. However sometimes more complex widgets are
    necessary. For example a zoom action in a word processor may be
    realized using a QComboBox in a QToolBar, presenting a range
    of different zoom levels. QToolBar provides QToolBar::insertWidget()
    as convenience function for inserting a single widget.
    However if you want to implement an action that uses custom
    widgets for visualization in multiple containers then you have to
    subclass QWidgetAction.

    If a QWidgetAction is added for example to a QToolBar then
    QWidgetAction::createWidget() is called. Reimplementations of that
    function should create a new custom widget with the specified parent.

    If the action is removed from a container widget then
    QWidgetAction::deleteWidget() is called with the previously created custom
    widget as argument. The default implementation hides the widget and deletes
    it using QObject::deleteLater().

    If you have only one single custom widget then you can set it as default
    widget using setDefaultWidget(). That widget will then be used if the
    action is added to a QToolBar, or in general to an action container that
    supports QWidgetAction. If a QWidgetAction with only a default widget is
    added to two toolbars at the same time then the default widget is shown
    only in the first toolbar the action was added to. QWidgetAction takes
    over ownership of the default widget.

    Note that it is up to the widget to activate the action, for example by
    reimplementing mouse event handlers and calling QAction::trigger().

    \bold {Mac OS X}: If you add a widget to a menu in the application's menu
    bar on Mac OS X, the widget will be added and it will function but with some
    limitations:
    \list 1
        \o The widget is reparented away from the QMenu to the native menu
            view. If you show the menu in some other place (e.g. as a popup menu),
            the widget will not be there.
        \o Focus/Keyboard handling of the widget is not possible.
        \o Due to Apple's design, mouse tracking on the widget currently does
            not work.
        \o Connecting the triggered() signal to a slot that opens a modal
            dialog will cause a crash in Mac OS X 10.4 (known bug acknowledged
            by Apple), a workaround is to use a QueuedConnection instead of a
            DirectConnection.
    \endlist

    \sa QAction, QActionGroup, QWidget
*/

/*!
    Constructs an action with \a parent.
*/
QWidgetAction::QWidgetAction(QObject *parent)
    : QAction(*(new QWidgetActionPrivate), parent)
{
}

/*!
    Destroys the object and frees allocated resources.
*/
QWidgetAction::~QWidgetAction()
{
    Q_D(QWidgetAction);
    for (int i = 0; i < d->createdWidgets.count(); ++i)
        disconnect(d->createdWidgets.at(i), SIGNAL(destroyed(QObject*)),
                   this, SLOT(_q_widgetDestroyed(QObject*)));
    QList<QWidget *> widgetsToDelete = d->createdWidgets;
    d->createdWidgets.clear();
    qDeleteAll(widgetsToDelete);
    delete d->defaultWidget;
}

/*!
    Sets \a widget to be the default widget. The ownership is
    transferred to QWidgetAction. Unless createWidget() is
    reimplemented by a subclass to return a new widget the default
    widget is used when a container widget requests a widget through
    requestWidget().
*/
void QWidgetAction::setDefaultWidget(QWidget *widget)
{
    Q_D(QWidgetAction);
    if (widget == d->defaultWidget || d->defaultWidgetInUse)
        return;
    delete d->defaultWidget;
    d->defaultWidget = widget;
    if (!widget)
        return;

    setVisible(!(widget->isHidden() && widget->testAttribute(Qt::WA_WState_ExplicitShowHide)));
    d->defaultWidget->hide();
    d->defaultWidget->setParent(0);
    d->defaultWidgetInUse = false;
    if (!isEnabled())
        d->defaultWidget->setEnabled(false);
}

/*!
    Returns the default widget.
*/
QWidget *QWidgetAction::defaultWidget() const
{
    Q_D(const QWidgetAction);
    return d->defaultWidget;
}

/*!
    Returns a widget that represents the action, with the given \a
    parent.

    Container widgets that support actions can call this function to
    request a widget as visual representation of the action.

    \sa releaseWidget(), createWidget(), defaultWidget()
*/
QWidget *QWidgetAction::requestWidget(QWidget *parent)
{
    Q_D(QWidgetAction);

    QWidget *w = createWidget(parent);
    if (!w) {
        if (d->defaultWidgetInUse || !d->defaultWidget)
            return 0;
        d->defaultWidget->setParent(parent);
        d->defaultWidgetInUse = true;
        return d->defaultWidget;
    }

    connect(w, SIGNAL(destroyed(QObject*)),
            this, SLOT(_q_widgetDestroyed(QObject*)));
    d->createdWidgets.append(w);
    return w;
}

/*!
    Releases the specified \a widget.

    Container widgets that support actions call this function when a widget
    action is removed.

    \sa requestWidget(), deleteWidget(), defaultWidget()
*/
void QWidgetAction::releaseWidget(QWidget *widget)
{
    Q_D(QWidgetAction);

    if (widget == d->defaultWidget) {
        d->defaultWidget->hide();
        d->defaultWidget->setParent(0);
        d->defaultWidgetInUse = false;
        return;
    }

    if (!d->createdWidgets.contains(widget))
        return;

    disconnect(widget, SIGNAL(destroyed(QObject*)),
               this, SLOT(_q_widgetDestroyed(QObject*)));
    d->createdWidgets.removeAll(widget);
    deleteWidget(widget);
}

/*!
    \reimp
*/
bool QWidgetAction::event(QEvent *event)
{
    Q_D(QWidgetAction);
    if (event->type() == QEvent::ActionChanged) {
        if (d->defaultWidget)
            d->defaultWidget->setEnabled(isEnabled());
        for (int i = 0; i < d->createdWidgets.count(); ++i)
            d->createdWidgets.at(i)->setEnabled(isEnabled());
    }
    return QAction::event(event);
}

/*!
    \reimp
 */
bool QWidgetAction::eventFilter(QObject *obj, QEvent *event)
{
    return QAction::eventFilter(obj,event);
}

/*!
    This function is called whenever the action is added to a container widget
    that supports custom widgets. If you don't want a custom widget to be
    used as representation of the action in the specified \a parent widget then
    0 should be returned.

    \sa deleteWidget()
*/
QWidget *QWidgetAction::createWidget(QWidget *parent)
{
    Q_UNUSED(parent)
    return 0;
}

/*!
    This function is called whenever the action is removed from a
    container widget that displays the action using a custom \a
    widget previously created using createWidget(). The default
    implementation hides the \a widget and schedules it for deletion
    using QObject::deleteLater().

    \sa createWidget()
*/
void QWidgetAction::deleteWidget(QWidget *widget)
{
    widget->hide();
    widget->deleteLater();
}

/*!
    Returns the list of widgets that have been using createWidget() and
    are currently in use by widgets the action has been added to.
*/
QList<QWidget *> QWidgetAction::createdWidgets() const
{
    Q_D(const QWidgetAction);
    return d->createdWidgets;
}

QT_END_NAMESPACE

#include "moc_qwidgetaction.cpp"

#endif // QT_NO_ACTION
