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



/*!
    \class QButtonGroup
    \brief The QButtonGroup class provides a container to organize groups of
    button widgets.

    \ingroup organizers
    \ingroup geomanagement
    \inmodule QtWidgets

    QButtonGroup provides an abstract container into which button widgets can
    be placed. It does not provide a visual representation of this container
    (see QGroupBox for a container widget), but instead manages the states of
    each of the buttons in the group.

    An \l {QButtonGroup::exclusive} {exclusive} button group switches
    off all checkable (toggle) buttons except the one that has been
    clicked. By default, a button group is exclusive. The buttons in a
    button group are usually checkable \l{QPushButton}s, \l{QCheckBox}es
    (normally for non-exclusive button groups), or \l{QRadioButton}s.
    If you create an exclusive button group, you should ensure that
    one of the buttons in the group is initially checked; otherwise,
    the group will initially be in a state where no buttons are
    checked.

    A button can be added to the group with addButton() and removed
    with removeButton(). If the group is exclusive, the
    currently checked button is available with checkedButton(). If a
    button is clicked, the buttonClicked() signal is emitted; for a
    checkable button in an exclusive group this means that the button
    has been checked. The list of buttons in the group is returned by
    buttons().

    In addition, QButtonGroup can map between integers and buttons.
    You can assign an integer id to a button with setId(), and
    retrieve it with id(). The id of the currently checked button is
    available with checkedId(), and there is an overloaded signal
    buttonClicked() which emits the id of the button. The id \c {-1}
    is reserved by QButtonGroup to mean "no such button". The purpose
    of the mapping mechanism is to simplify the representation of enum
    values in a user interface.

    \sa QGroupBox, QPushButton, QCheckBox, QRadioButton
*/

/*!
    \fn QButtonGroup::QButtonGroup(QObject *parent)

    Constructs a new, empty button group with the given \a parent.

    \sa addButton(), setExclusive()
*/

/*!
    \fn QButtonGroup::~QButtonGroup()

    Destroys the button group.
*/

/*!
    \property QButtonGroup::exclusive
    \brief whether the button group is exclusive

    If this property is \c true, then only one button in the group can be checked
    at any given time. The user can click on any button to check it, and that
    button will replace the existing one as the checked button in the group.

    In an exclusive group, the user cannot uncheck the currently checked button
    by clicking on it; instead, another button in the group must be clicked
    to set the new checked button for that group.

    By default, this property is \c true.
*/

/*!
    \fn void QButtonGroup::buttonClicked(QAbstractButton *button)

    This signal is emitted when the given \a button is clicked. A
    button is clicked when it is first pressed and then released, when
    its shortcut key is typed, or when QAbstractButton::click()
    or QAbstractButton::animateClick() is programmatically called.


    \sa checkedButton(), QAbstractButton::clicked()
*/

/*!
    \fn void QButtonGroup::buttonClicked(int id)

    This signal is emitted when a button with the given \a id is
    clicked.

    \sa checkedButton(), QAbstractButton::clicked()
*/

/*!
    \fn void QButtonGroup::buttonPressed(QAbstractButton *button)
    \since 4.2

    This signal is emitted when the given \a button is pressed down.

    \sa QAbstractButton::pressed()
*/

/*!
    \fn void QButtonGroup::buttonPressed(int id)
    \since 4.2

    This signal is emitted when a button with the given \a id is
    pressed down.

    \sa QAbstractButton::pressed()
*/

/*!
    \fn void QButtonGroup::buttonReleased(QAbstractButton *button)
    \since 4.2

    This signal is emitted when the given \a button is released.

    \sa QAbstractButton::released()
*/

/*!
    \fn void QButtonGroup::buttonReleased(int id)
    \since 4.2

    This signal is emitted when a button with the given \a id is
    released.

    \sa QAbstractButton::released()
*/

/*!
    \fn void QButtonGroup::buttonToggled(QAbstractButton *button, bool checked)
    \since 5.2

    This signal is emitted when the given \a button is toggled.
    \a checked is true if the button is checked, or false if the button is unchecked.

    \sa QAbstractButton::toggled()
*/

/*!
    \fn void QButtonGroup::buttonToggled(int id, bool checked)
    \since 5.2

    This signal is emitted when a button with the given \a id is toggled.
    \a checked is true if the button is checked, or false if the button is unchecked.

    \sa QAbstractButton::toggled()
*/


/*!
    \fn void QButtonGroup::addButton(QAbstractButton *button, int id = -1);

    Adds the given \a button to the button group. If \a id is -1,
    an id will be assigned to the button.
    Automatically assigned ids are guaranteed to be negative,
    starting with -2. If you are assigning your own ids, use
    positive values to avoid conflicts.

    \sa removeButton(), buttons()
*/

/*!
    \fn void QButtonGroup::removeButton(QAbstractButton *button);

    Removes the given \a button from the button group.

    \sa addButton(), buttons()
*/

/*!
    \fn QList<QAbstractButton*> QButtonGroup::buttons() const

    Returns the button group's list of buttons. This may be empty.

    \sa addButton(), removeButton()
*/

/*!
    \fn QAbstractButton *QButtonGroup::checkedButton() const;

    Returns the button group's checked button, or 0 if no buttons are
    checked.

    \sa buttonClicked()
*/

/*!
    \fn QAbstractButton *QButtonGroup::button(int id) const;
    \since 4.1

    Returns the button with the specified \a id, or 0 if no such button
    exists.
*/

/*!
    \fn void QButtonGroup::setId(QAbstractButton *button, int id)
    \since 4.1

    Sets the \a id for the specified \a button. Note that \a id cannot
    be -1.

    \sa id()
*/

/*!
    \fn int QButtonGroup::id(QAbstractButton *button) const;
    \since 4.1

    Returns the id for the specified \a button, or -1 if no such button
    exists.


    \sa setId()
*/

/*!
    \fn int QButtonGroup::checkedId() const;
    \since 4.1

    Returns the id of the checkedButton(), or -1 if no button is checked.

    \sa setId()
*/

