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

#include "qactiongroup.h"

#ifndef QT_NO_ACTION

#include "qaction_p.h"
#include "qapplication.h"
#include "qevent.h"
#include "qlist.h"

QT_BEGIN_NAMESPACE

class QActionGroupPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QActionGroup)
public:
    QActionGroupPrivate() : exclusive(1), enabled(1), visible(1)  { }
    QList<QAction *> actions;
    QPointer<QAction> current;
    uint exclusive : 1;
    uint enabled : 1;
    uint visible : 1;

private:
    void _q_actionTriggered();  //private slot
    void _q_actionChanged();    //private slot
    void _q_actionHovered();    //private slot
};

void QActionGroupPrivate::_q_actionChanged()
{
    Q_Q(QActionGroup);
    QAction *action = qobject_cast<QAction*>(q->sender());
    Q_ASSERT_X(action != 0, "QWidgetGroup::_q_actionChanged", "internal error");
    if(exclusive) {
        if (action->isChecked()) {
            if (action != current) {
                if(current)
                    current->setChecked(false);
                current = action;
            }
        } else if (action == current) {
            current = 0;
        }
    }
}

void QActionGroupPrivate::_q_actionTriggered()
{
    Q_Q(QActionGroup);
    QAction *action = qobject_cast<QAction*>(q->sender());
    Q_ASSERT_X(action != 0, "QWidgetGroup::_q_actionTriggered", "internal error");
    emit q->triggered(action);
}

void QActionGroupPrivate::_q_actionHovered()
{
    Q_Q(QActionGroup);
    QAction *action = qobject_cast<QAction*>(q->sender());
    Q_ASSERT_X(action != 0, "QWidgetGroup::_q_actionHovered", "internal error");
    emit q->hovered(action);
}

/*!
    \class QActionGroup
    \brief The QActionGroup class groups actions together.

    \ingroup mainwindow-classes
    \inmodule QtWidgets

    In some situations it is useful to group QAction objects together.
    For example, if you have a \uicontrol{Left Align} action, a \uicontrol{Right
    Align} action, a \uicontrol{Justify} action, and a \uicontrol{Center} action,
    only one of these actions should be active at any one time. One
    simple way of achieving this is to group the actions together in
    an action group.

    Here's a example (from the \l{mainwindows/menus}{Menus} example):

    \snippet mainwindows/menus/mainwindow.cpp 6

    Here we create a new action group. Since the action group is
    exclusive by default, only one of the actions in the group is
    checked at any one time.

    \image qactiongroup-align.png Alignment options in a QMenu

    A QActionGroup emits an triggered() signal when one of its
    actions is chosen. Each action in an action group emits its
    triggered() signal as usual.

    As stated above, an action group is \l exclusive by default; it
    ensures that only one checkable action is active at any one time.
    If you want to group checkable actions without making them
    exclusive, you can turn of exclusiveness by calling
    setExclusive(false).

    Actions can be added to an action group using addAction(), but it
    is usually more convenient to specify a group when creating
    actions; this ensures that actions are automatically created with
    a parent. Actions can be visually separated from each other by
    adding a separator action to the group; create an action and use
    QAction's \l {QAction::}{setSeparator()} function to make it
    considered a separator. Action groups are added to widgets with
    the QWidget::addActions() function.

    \sa QAction
*/

/*!
    Constructs an action group for the \a parent object.

    The action group is exclusive by default. Call setExclusive(false)
    to make the action group non-exclusive.
*/
QActionGroup::QActionGroup(QObject* parent) : QObject(*new QActionGroupPrivate, parent)
{
}

/*!
    Destroys the action group.
*/
QActionGroup::~QActionGroup()
{
}

/*!
    \fn QAction *QActionGroup::addAction(QAction *action)

    Adds the \a action to this group, and returns it.

    Normally an action is added to a group by creating it with the
    group as its parent, so this function is not usually used.

    \sa QAction::setActionGroup()
*/
QAction *QActionGroup::addAction(QAction* a)
{
    Q_D(QActionGroup);
    if(!d->actions.contains(a)) {
        d->actions.append(a);
        QObject::connect(a, SIGNAL(triggered()), this, SLOT(_q_actionTriggered()));
        QObject::connect(a, SIGNAL(changed()), this, SLOT(_q_actionChanged()));
        QObject::connect(a, SIGNAL(hovered()), this, SLOT(_q_actionHovered()));
    }
    if(!a->d_func()->forceDisabled) {
        a->setEnabled(d->enabled);
        a->d_func()->forceDisabled = false;
    }
    if(!a->d_func()->forceInvisible) {
        a->setVisible(d->visible);
        a->d_func()->forceInvisible = false;
    }
    if(a->isChecked())
        d->current = a;
    QActionGroup *oldGroup = a->d_func()->group;
    if(oldGroup != this) {
        if (oldGroup)
            oldGroup->removeAction(a);
        a->d_func()->group = this;
    }
    return a;
}

/*!
    Creates and returns an action with \a text.  The newly created
    action is a child of this action group.

    Normally an action is added to a group by creating it with the
    group as parent, so this function is not usually used.

    \sa QAction::setActionGroup()
*/
QAction *QActionGroup::addAction(const QString &text)
{
    return new QAction(text, this);
}

/*!
    Creates and returns an action with \a text and an \a icon. The
    newly created action is a child of this action group.

    Normally an action is added to a group by creating it with the
    group as its parent, so this function is not usually used.

    \sa QAction::setActionGroup()
*/
QAction *QActionGroup::addAction(const QIcon &icon, const QString &text)
{
    return new QAction(icon, text, this);
}

/*!
  Removes the \a action from this group. The action will have no
  parent as a result.

  \sa QAction::setActionGroup()
*/
void QActionGroup::removeAction(QAction *action)
{
    Q_D(QActionGroup);
    if (d->actions.removeAll(action)) {
        if (action == d->current)
            d->current = 0;
        QObject::disconnect(action, SIGNAL(triggered()), this, SLOT(_q_actionTriggered()));
        QObject::disconnect(action, SIGNAL(changed()), this, SLOT(_q_actionChanged()));
        QObject::disconnect(action, SIGNAL(hovered()), this, SLOT(_q_actionHovered()));
        action->d_func()->group = 0;
    }
}

/*!
    Returns the list of this groups's actions. This may be empty.
*/
QList<QAction*> QActionGroup::actions() const
{
    Q_D(const QActionGroup);
    return d->actions;
}

/*!
    \property QActionGroup::exclusive
    \brief whether the action group does exclusive checking

    If exclusive is true, only one checkable action in the action group
    can ever be active at any time. If the user chooses another
    checkable action in the group, the one they chose becomes active and
    the one that was active becomes inactive.

    \sa QAction::checkable
*/
void QActionGroup::setExclusive(bool b)
{
    Q_D(QActionGroup);
    d->exclusive = b;
}

bool QActionGroup::isExclusive() const
{
    Q_D(const QActionGroup);
    return d->exclusive;
}

/*!
    \fn void QActionGroup::setDisabled(bool b)

    This is a convenience function for the \l enabled property, that
    is useful for signals--slots connections. If \a b is true the
    action group is disabled; otherwise it is enabled.
*/

/*!
    \property QActionGroup::enabled
    \brief whether the action group is enabled

    Each action in the group will be enabled or disabled unless it
    has been explicitly disabled.

    \sa QAction::setEnabled()
*/
void QActionGroup::setEnabled(bool b)
{
    Q_D(QActionGroup);
    d->enabled = b;
    for(QList<QAction*>::const_iterator it = d->actions.constBegin(); it != d->actions.constEnd(); ++it) {
        if(!(*it)->d_func()->forceDisabled) {
            (*it)->setEnabled(b);
            (*it)->d_func()->forceDisabled = false;
        }
    }
}

bool QActionGroup::isEnabled() const
{
    Q_D(const QActionGroup);
    return d->enabled;
}

/*!
  Returns the currently checked action in the group, or 0 if none
  are checked.
*/
QAction *QActionGroup::checkedAction() const
{
    Q_D(const QActionGroup);
    return d->current;
}

/*!
    \property QActionGroup::visible
    \brief whether the action group is visible

    Each action in the action group will match the visible state of
    this group unless it has been explicitly hidden.

    \sa QAction::setEnabled()
*/
void QActionGroup::setVisible(bool b)
{
    Q_D(QActionGroup);
    d->visible = b;
    for(QList<QAction*>::Iterator it = d->actions.begin(); it != d->actions.end(); ++it) {
        if(!(*it)->d_func()->forceInvisible) {
            (*it)->setVisible(b);
            (*it)->d_func()->forceInvisible = false;
        }
    }
}

bool QActionGroup::isVisible() const
{
    Q_D(const QActionGroup);
    return d->visible;
}

/*!
    \fn void QActionGroup::triggered(QAction *action)

    This signal is emitted when the given \a action in the action
    group is activated by the user; for example, when the user clicks
    a menu option, toolbar button, or presses an action's shortcut key
    combination.

    Connect to this signal for command actions.

    \sa QAction::activate()
*/

/*!
    \fn void QActionGroup::hovered(QAction *action)

    This signal is emitted when the given \a action in the action
    group is highlighted by the user; for example, when the user
    pauses with the cursor over a menu option, toolbar button, or
    presses an action's shortcut key combination.

    \sa QAction::activate()
*/

QT_END_NAMESPACE

#include "moc_qactiongroup.cpp"

#endif // QT_NO_ACTION
