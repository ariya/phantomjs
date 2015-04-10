/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qplatforminputcontext.h"
#include <qguiapplication.h>
#include <QRect>
#include "private/qkeymapper_p.h"
#include <qpa/qplatforminputcontext_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformInputContext
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa
    \brief The QPlatformInputContext class abstracts the input method dependent data and composing state.

    An input method is responsible for inputting complex text that cannot
    be inputted via simple keymap. It converts a sequence of input
    events (typically key events) into a text string through the input
    method specific converting process. The class of the processes are
    widely ranging from simple finite state machine to complex text
    translator that pools a whole paragraph of a text with text
    editing capability to perform grammar and semantic analysis.

    To abstract such different input method specific intermediate
    information, Qt offers the QPlatformInputContext as base class. The
    concept is well known as 'input context' in the input method
    domain. An input context is created for a text widget in response
    to a demand. It is ensured that an input context is prepared for
    an input method before input to a text widget.

    QPlatformInputContext provides an interface the actual input methods
    can derive from by reimplementing methods.

    \sa QInputMethod
*/

/*!
    \internal
 */
QPlatformInputContext::QPlatformInputContext()
    : QObject(*(new QPlatformInputContextPrivate))
{
}

/*!
    \internal
 */
QPlatformInputContext::~QPlatformInputContext()
{
}

/*!
    Returns input context validity. Deriving implementations should return true.
 */
bool QPlatformInputContext::isValid() const
{
    return false;
}

/*!
    Method to be called when input method needs to be reset. Called by QInputMethod::reset().
    No further QInputMethodEvents should be sent as response.
 */
void QPlatformInputContext::reset()
{
}

void QPlatformInputContext::commit()
{
}

/*!
    Notification on editor updates. Called by QInputMethod::update().
 */
void QPlatformInputContext::update(Qt::InputMethodQueries)
{
}

/*!
    Called when when the word currently being composed in input item is tapped by
    the user. Input methods often use this information to offer more word
    suggestions to the user.
 */
void QPlatformInputContext::invokeAction(QInputMethod::Action action, int cursorPosition)
{
    Q_UNUSED(cursorPosition)
    // Default behavior for simple ephemeral input contexts. Some
    // complex input contexts should not be reset here.
    if (action == QInputMethod::Click)
        reset();
}

/*!
    This function can be reimplemented to filter input events.
    Return true if the event has been consumed. Otherwise, the unfiltered event will
    be forwarded to widgets as ordinary way. Although the input events have accept()
    and ignore() methods, leave it untouched.
*/
bool QPlatformInputContext::filterEvent(const QEvent *event)
{
    Q_UNUSED(event)
    return false;
}

/*!
    This function can be reimplemented to return virtual keyboard rectangle in currently active
    window coordinates. Default implementation returns invalid rectangle.
 */
QRectF QPlatformInputContext::keyboardRect() const
{
    return QRectF();
}

/*!
    Active QPlatformInputContext is responsible for providing keyboardRectangle property to QInputMethod.
    In addition of providing the value in keyboardRect function, it also needs to call this emit
    function whenever the property changes.
 */
void QPlatformInputContext::emitKeyboardRectChanged()
{
    emit qApp->inputMethod()->keyboardRectangleChanged();
}

/*!
    This function can be reimplemented to return true whenever input method is animating
    shown or hidden. Default implementation returns \c false.
 */
bool QPlatformInputContext::isAnimating() const
{
    return false;
}

/*!
    Active QPlatformInputContext is responsible for providing animating property to QInputMethod.
    In addition of providing the value in isAnimation function, it also needs to call this emit
    function whenever the property changes.
 */
void QPlatformInputContext::emitAnimatingChanged()
{
    emit qApp->inputMethod()->animatingChanged();
}

/*!
    Request to show input panel.
 */
void QPlatformInputContext::showInputPanel()
{
}

/*!
    Request to hide input panel.
 */
void QPlatformInputContext::hideInputPanel()
{
}

/*!
    Returns input panel visibility status. Default implementation returns \c false.
 */
bool QPlatformInputContext::isInputPanelVisible() const
{
    return false;
}

/*!
    Active QPlatformInputContext is responsible for providing visible property to QInputMethod.
    In addition of providing the value in isInputPanelVisible function, it also needs to call this emit
    function whenever the property changes.
 */
void QPlatformInputContext::emitInputPanelVisibleChanged()
{
    emit qApp->inputMethod()->visibleChanged();
}

QLocale QPlatformInputContext::locale() const
{
    return qt_keymapper_private()->keyboardInputLocale;
}

void QPlatformInputContext::emitLocaleChanged()
{
    emit qApp->inputMethod()->localeChanged();
}

Qt::LayoutDirection QPlatformInputContext::inputDirection() const
{
    return qt_keymapper_private()->keyboardInputDirection;
}

void QPlatformInputContext::emitInputDirectionChanged(Qt::LayoutDirection newDirection)
{
    emit qApp->inputMethod()->inputDirectionChanged(newDirection);
}

/*!
    This virtual method gets called to notify updated focus to \a object.
    \warning Input methods must not call this function directly.
 */
void QPlatformInputContext::setFocusObject(QObject *object)
{
    Q_UNUSED(object)
}

/*!
    Returns \c true if current focus object supports input method events.
 */
bool QPlatformInputContext::inputMethodAccepted() const
{
    return QPlatformInputContextPrivate::s_inputMethodAccepted;
}

bool QPlatformInputContextPrivate::s_inputMethodAccepted = false;

void QPlatformInputContextPrivate::setInputMethodAccepted(bool accepted)
{
    QPlatformInputContextPrivate::s_inputMethodAccepted = accepted;
}


QT_END_NAMESPACE
