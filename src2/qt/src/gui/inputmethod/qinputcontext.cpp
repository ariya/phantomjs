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

/****************************************************************************
**
** Implementation of QInputContext class
**
** Copyright (C) 2003-2004 immodule for Qt Project.  All rights reserved.
**
** This file is written to contribute to Nokia Corporation and/or its subsidiary(-ies) under their own
** license. You may use this file under your Qt license. Following
** description is copied from their original file headers. Contact
** immodule-qt@freedesktop.org if any conditions of this licensing are
** not clear to you.
**
****************************************************************************/

//#define QT_NO_IM_PREEDIT_RELOCATION

#include "qinputcontext.h"
#include "qinputcontext_p.h"

#ifndef QT_NO_IM

#include "qplatformdefs.h"

#include "qapplication.h"
#include "qmenu.h"
#include "qtextformat.h"
#include "qpalette.h"

#include <stdlib.h>
#include <limits.h>

QT_BEGIN_NAMESPACE

/*!
    \class QInputContext
    \brief The QInputContext class abstracts the input method dependent data and composing state.

    \ingroup i18n

    An input method is responsible for inputting complex text that cannot
    be inputted via simple keymap. It converts a sequence of input
    events (typically key events) into a text string through the input
    method specific converting process. The class of the processes are
    widely ranging from simple finite state machine to complex text
    translator that pools a whole paragraph of a text with text
    editing capability to perform grammar and semantic analysis.

    To abstract such different input method specific intermediate
    information, Qt offers the QInputContext as base class. The
    concept is well known as 'input context' in the input method
    domain. An input context is created for a text widget in response
    to a demand. It is ensured that an input context is prepared for
    an input method before input to a text widget.

    Multiple input contexts that belong to a single input method
    may concurrently coexist. Suppose multi-window text editor. Each
    text widget of window A and B holds different QInputContext
    instance which contains different state information such as
    partially composed text.

    \section1 Groups of Functions

    \table
    \header \o Context \o Functions

    \row \o Receiving information \o
        x11FilterEvent(),
        filterEvent(),
        mouseHandler()

    \row \o Sending back composed text \o
        sendEvent()

    \row \o State change notification \o
        setFocusWidget(),
        reset()

    \row \o Context information \o
        identifierName(),
        language(),
        font(),
        isComposing()

    \endtable

    \section1 Licensing Information

    \legalese
    Copyright (C) 2003-2004 immodule for Qt Project.  All rights reserved.

    This file is written to contribute to Nokia Corporation and/or its subsidiary(-ies) under their own
    license. You may use this file under your Qt license. Following
    description is copied from their original file headers. Contact
    immodule-qt@freedesktop.org if any conditions of this licensing are
    not clear to you.
    \endlegalese

    \sa QInputContextPlugin, QInputContextFactory, QApplication::setInputContext()
*/

/*!
    Constructs an input context with the given \a parent.
*/
QInputContext::QInputContext(QObject* parent)
    : QObject(*new QInputContextPrivate, parent)
{
}


/*!
    Destroys the input context.
*/
QInputContext::~QInputContext()
{
}

/*!
    Returns the widget that has an input focus for this input
    context.

    The return value may differ from holderWidget() if the input
    context is shared between several text widgets.

    \warning To ensure platform independence and support flexible
    configuration of widgets, ordinary input methods should not call
    this function directly.

    \sa setFocusWidget()
*/
QWidget *QInputContext::focusWidget() const
{
    Q_D(const QInputContext);
    return d->focusWidget;
}


/*!
    Sets the \a widget that has an input focus for this input context.

    \warning Ordinary input methods must not call this function
    directly.

    \sa focusWidget()
*/
void QInputContext::setFocusWidget(QWidget *widget)
{
    Q_ASSERT(!widget || widget->testAttribute(Qt::WA_InputMethodEnabled));
    Q_D(QInputContext);
    d->focusWidget = widget;
}

/*!
    \fn bool QInputContext::isComposing() const

    This function indicates whether InputMethodStart event had been
    sent to the current focus widget. It is ensured that an input
    context can send InputMethodCompose or InputMethodEnd event safely
    if this function returned true.

    The state is automatically being tracked through sendEvent().

    \sa sendEvent()
*/

/*!
    This function can be reimplemented in a subclass to filter input
    events.

    Return true if the \a event has been consumed. Otherwise, the
    unfiltered \a event will be forwarded to widgets as ordinary
    way. Although the input events have accept() and ignore()
    methods, leave it untouched.

    \a event is currently restricted to events of these types:

    \list
        \i CloseSoftwareInputPanel
        \i KeyPress
        \i KeyRelease
        \i MouseButtonDblClick
        \i MouseButtonPress
        \i MouseButtonRelease
        \i MouseMove
        \i RequestSoftwareInputPanel
    \endlist

    But some input method related events such as QWheelEvent or
    QTabletEvent may be added in future.

    The filtering opportunity is always given to the input context as
    soon as possible. It has to be taken place before any other key
    event consumers such as eventfilters and accelerators because some
    input methods require quite various key combination and
    sequences. It often conflicts with accelerators and so on, so we
    must give the input context the filtering opportunity first to
    ensure all input methods work properly regardless of application
    design.

    Ordinary input methods require discrete key events to work
    properly, so Qt's key compression is always disabled for any input
    contexts.

    \sa QKeyEvent, x11FilterEvent()
*/
bool QInputContext::filterEvent(const QEvent * /*event*/)
{
    return false;
}

/*!
  Sends an input method event specified by \a event to the current focus
  widget. Implementations of QInputContext should call this method to
  send the generated input method events and not
  QApplication::sendEvent(), as the events might have to get dispatched
  to a different application on some platforms.

  Some complex input methods route the handling to several child
  contexts (e.g. to enable language switching). To account for this,
  QInputContext will check if the parent object is a QInputContext. If
  yes, it will call the parents sendEvent() implementation instead of
  sending the event directly.

  \sa QInputMethodEvent
*/
void QInputContext::sendEvent(const QInputMethodEvent &event)
{
    // route events over input context parents to make chaining possible.
    QInputContext *p = qobject_cast<QInputContext *>(parent());
    if (p) {
        p->sendEvent(event);
        return;
    }

    QWidget *focus = focusWidget();
    if (!focus)
	return;

    QInputMethodEvent e(event);
    QApplication::sendEvent(focus, &e);
}


/*!
    This function can be reimplemented in a subclass to handle mouse
    press, release, double-click, and move events within the preedit
    text. You can use the function to implement mouse-oriented user
    interface such as text selection or popup menu for candidate
    selection.

    The \a x parameter is the offset within the string that was sent
    with the InputMethodCompose event. The alteration boundary of \a
    x is ensured as character boundary of preedit string accurately.

    The \a event parameter is the event that was sent to the editor
    widget. The event type is QEvent::MouseButtonPress,
    QEvent::MouseButtonRelease, QEvent::MouseButtonDblClick or
    QEvent::MouseMove. The event's button and state indicate
    the kind of operation that was performed.
*/
void QInputContext::mouseHandler(int /*x*/, QMouseEvent *event)
{
    // Default behavior for simple ephemeral input contexts. Some
    // complex input contexts should not be reset here.
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick)
	reset();
}


/*!
    Returns the font of the current input widget
*/
QFont QInputContext::font() const
{
    Q_D(const QInputContext);
    if (!d->focusWidget)
        return QApplication::font();

    return qvariant_cast<QFont>(d->focusWidget->inputMethodQuery(Qt::ImFont));
}

/*!
    This virtual function is called when a state in the focus widget
    has changed. QInputContext can then use
    QWidget::inputMethodQuery() to query the new state of the widget.
*/
void QInputContext::update()
{
}

/*!
    This virtual function is called when the specified \a widget is
    destroyed. The \a widget is a widget on which this input context
    is installed.
*/
void QInputContext::widgetDestroyed(QWidget *widget)
{
    Q_D(QInputContext);
    if (widget == d->focusWidget)
        setFocusWidget(0);
}

/*!
    \fn void QInputContext::reset()

    This function can be reimplemented in a subclass to reset the
    state of the input method.

    This function is called by several widgets to reset input
    state. For example, a text widget call this function before
    inserting a text to make widget ready to accept a text.

    Default implementation is sufficient for simple input method. You
    can override this function to reset external input method engines
    in complex input method. In the case, call QInputContext::reset()
    to ensure proper termination of inputting.

    In a reimplementation of reset(), you must not send any
    QInputMethodEvent containing preedit text. You can only commit
    string and attributes; otherwise, you risk breaking input state
    consistency.
*/


/*!
  \fn QString QInputContext::identifierName()

    This function must be implemented in any subclasses to return the
    identifier name of the input method.

    Return value is the name to identify and specify input methods for
    the input method switching mechanism and so on. The name has to be
    consistent with QInputContextPlugin::keys(). The name has to
    consist of ASCII characters only.

    There are two different names with different responsibility in the
    input method domain. This function returns one of them. Another
    name is called 'display name' that stands for the name for
    endusers appeared in a menu and so on.

    \sa QInputContextPlugin::keys(), QInputContextPlugin::displayName()
*/


/*!
    \fn QString QInputContext::language()

    This function must be implemented in any subclasses to return a
    language code (e.g. "zh_CN", "zh_TW", "zh_HK", "ja", "ko", ...)
    of the input context. If the input context can handle multiple
    languages, return the currently used one. The name has to be
    consistent with QInputContextPlugin::language().

    This information will be used by language tagging feature in
    QInputMethodEvent. It is required to distinguish unified han characters
    correctly. It enables proper font and character code
    handling. Suppose CJK-awared multilingual web browser
    (that automatically modifies fonts in CJK-mixed text) and XML editor
    (that automatically inserts lang attr).
*/


/*!
    This is a preliminary interface for Qt 4.
*/
QList<QAction *> QInputContext::actions()
{
    return QList<QAction *>();
}

/*!
    \enum QInputContext::StandardFormat

    \value PreeditFormat  The preedit text.
    \value SelectionFormat  The selection text.

    \sa standardFormat()
*/

/*!
    Returns a QTextFormat object that specifies the format for
    component \a s.
*/
QTextFormat QInputContext::standardFormat(StandardFormat s) const
{
    QWidget *focus = focusWidget();
    const QPalette &pal = focus ? focus->palette() : QApplication::palette();

    QTextCharFormat fmt;
    QColor bg;
    switch (s) {
    case QInputContext::PreeditFormat: {
        fmt.setUnderlineStyle(QTextCharFormat::DashUnderline);
        break;
    }
    case QInputContext::SelectionFormat: {
        bg = pal.text().color();
        fmt.setBackground(QBrush(bg));
        fmt.setForeground(pal.background());
        break;
    }
    }
    return fmt;
}

#ifdef Q_WS_X11
/*!
    This function may be overridden only if input method is depending
    on X11 and you need raw XEvent. Otherwise, this function must not.

    This function is designed to filter raw key events for XIM, but
    other input methods may use this to implement some special
    features such as distinguishing Shift_L and Shift_R.

    Return true if the \a event has been consumed. Otherwise, the
    unfiltered \a event will be translated into QEvent and forwarded
    to filterEvent(). Filtering at both x11FilterEvent() and
    filterEvent() in single input method is allowed.

    \a keywidget is a client widget into which a text is inputted. \a
    event is inputted XEvent.

    \sa filterEvent()
*/
bool QInputContext::x11FilterEvent(QWidget * /*keywidget*/, XEvent * /*event*/)
{
    return false;
}
#endif // Q_WS_X11

#ifdef Q_OS_SYMBIAN
/*!
    \since 4.6

    This function may be overridden only if input method is depending
    on Symbian and you need raw Symbian events. Otherwise, this function must not.

    This function is designed to filter raw key events on Symbian, but
    other input methods may use this to implement some special
    features.

    Return true if the \a event has been consumed. Otherwise, the
    unfiltered \a event will be translated into QEvent and forwarded
    to filterEvent(). Filtering at both symbianFilterEvent() and
    filterEvent() in single input method is allowed.

    \a keywidget is a client widget into which a text is inputted. \a
    event is inputted QSymbianEvent.

    \sa filterEvent()
*/
bool QInputContext::symbianFilterEvent(QWidget * /*keywidget*/, const QSymbianEvent * /*event*/)
{
    return false;
}
#endif // Q_OS_SYMBIAN

QT_END_NAMESPACE

#endif //Q_NO_IM
