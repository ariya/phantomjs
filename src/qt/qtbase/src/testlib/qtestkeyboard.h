/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTESTKEYBOARD_H
#define QTESTKEYBOARD_H

#if 0
// inform syncqt
#pragma qt_no_master_include
#endif

#include <QtTest/qtestassert.h>
#include <QtTest/qtest_global.h>
#include <QtTest/qtestsystem.h>
#include <QtTest/qtestspontaneevent.h>

#include <QtCore/qpointer.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qwindow.h>
#include <QtGui/qevent.h>

#ifdef QT_WIDGETS_LIB
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qapplication.h>
#endif

QT_BEGIN_NAMESPACE

Q_GUI_EXPORT void qt_handleKeyEvent(QWindow *w, QEvent::Type t, int k, Qt::KeyboardModifiers mods, const QString & text = QString(), bool autorep = false, ushort count = 1);

namespace QTest
{
    enum KeyAction { Press, Release, Click };

    static void simulateEvent(QWindow *window, bool press, int code,
                              Qt::KeyboardModifiers modifier, QString text, bool repeat, int delay=-1)
    {
        QEvent::Type type;
        type = press ? QEvent::KeyPress : QEvent::KeyRelease;
        qt_handleKeyEvent(window, type, code, modifier, text, repeat, delay);
#ifdef QT_MAC_USE_COCOA
        QTest::qWait(20);
#else
        qApp->processEvents();
#endif
    }

    static void sendKeyEvent(KeyAction action, QWindow *window, Qt::Key code,
                             QString text, Qt::KeyboardModifiers modifier, int delay=-1)
    {
        QTEST_ASSERT(qApp);

        if (!window)
            window = QGuiApplication::focusWindow();

        QTEST_ASSERT(window);


        if (action == Click) {
            sendKeyEvent(Press, window, code, text, modifier, delay);
            sendKeyEvent(Release, window, code, text, modifier, delay);
            return;
        }

        bool repeat = false;

        if (action == Press) {
            if (modifier & Qt::ShiftModifier)
                simulateEvent(window, true, Qt::Key_Shift, 0, QString(), false, delay);

            if (modifier & Qt::ControlModifier)
                simulateEvent(window, true, Qt::Key_Control, modifier & Qt::ShiftModifier, QString(), false, delay);

            if (modifier & Qt::AltModifier)
                simulateEvent(window, true, Qt::Key_Alt,
                              modifier & (Qt::ShiftModifier | Qt::ControlModifier), QString(), false, delay);
            if (modifier & Qt::MetaModifier)
                simulateEvent(window, true, Qt::Key_Meta, modifier & (Qt::ShiftModifier
                                                                      | Qt::ControlModifier | Qt::AltModifier), QString(), false, delay);
            simulateEvent(window, true, code, modifier, text, repeat, delay);
        } else if (action == Release) {
            simulateEvent(window, false, code, modifier, text, repeat, delay);

            if (modifier & Qt::MetaModifier)
                simulateEvent(window, false, Qt::Key_Meta, modifier, QString(), false, delay);
            if (modifier & Qt::AltModifier)
                simulateEvent(window, false, Qt::Key_Alt, modifier &
                              (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier), QString(), false, delay);

            if (modifier & Qt::ControlModifier)
                simulateEvent(window, false, Qt::Key_Control,
                              modifier & (Qt::ShiftModifier | Qt::ControlModifier), QString(), false, delay);

            if (modifier & Qt::ShiftModifier)
                simulateEvent(window, false, Qt::Key_Shift, modifier & Qt::ShiftModifier, QString(), false, delay);
        }
    }

    // Convenience function
    static void sendKeyEvent(KeyAction action, QWindow *window, Qt::Key code,
                             char ascii, Qt::KeyboardModifiers modifier, int delay=-1)
    {
        QString text;
        if (ascii)
            text = QString(QChar::fromLatin1(ascii));
        sendKeyEvent(action, window, code, text, modifier, delay);
    }

    inline static void keyEvent(KeyAction action, QWindow *window, char ascii,
                                Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { sendKeyEvent(action, window, asciiToKey(ascii), ascii, modifier, delay); }
    inline static void keyEvent(KeyAction action, QWindow *window, Qt::Key key,
                                Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { sendKeyEvent(action, window, key, keyToAscii(key), modifier, delay); }

    Q_DECL_UNUSED inline static void keyClick(QWindow *window, Qt::Key key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { keyEvent(Click, window, key, modifier, delay); }
    Q_DECL_UNUSED inline static void keyClick(QWindow *window, char key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { keyEvent(Click, window, key, modifier, delay); }
    Q_DECL_UNUSED inline static void keyRelease(QWindow *window, char key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { keyEvent(Release, window, key, modifier, delay); }
    Q_DECL_UNUSED inline static void keyRelease(QWindow *window, Qt::Key key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { keyEvent(Release, window, key, modifier, delay); }
    Q_DECL_UNUSED inline static void keyPress(QWindow *window, char key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { keyEvent(Press, window, key, modifier, delay); }
    Q_DECL_UNUSED inline static void keyPress(QWindow *window, Qt::Key key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { keyEvent(Press, window, key, modifier, delay); }

#ifdef QT_WIDGETS_LIB
    static void simulateEvent(QWidget *widget, bool press, int code,
                              Qt::KeyboardModifiers modifier, QString text, bool repeat, int delay=-1)
    {
        QTEST_ASSERT(widget);
        extern int Q_TESTLIB_EXPORT defaultKeyDelay();

        if (delay == -1 || delay < defaultKeyDelay())
            delay = defaultKeyDelay();
        if (delay > 0)
            QTest::qWait(delay);

        QKeyEvent a(press ? QEvent::KeyPress : QEvent::KeyRelease, code, modifier, text, repeat);
        QSpontaneKeyEvent::setSpontaneous(&a);
        if (!qApp->notify(widget, &a))
            QTest::qWarn("Keyboard event not accepted by receiving widget");
    }

    static void sendKeyEvent(KeyAction action, QWidget *widget, Qt::Key code,
                             QString text, Qt::KeyboardModifiers modifier, int delay=-1)
    {
        QTEST_ASSERT(qApp);

        if (!widget)
            widget = QWidget::keyboardGrabber();
        if (!widget) {
            // Popup widgets stealthily steal the keyboard grab
            if (QWidget *apw = QApplication::activePopupWidget())
                widget = apw->focusWidget() ? apw->focusWidget() : apw;
        }
        if (!widget) {
            QWindow *window = QGuiApplication::focusWindow();
            if (window) {
                sendKeyEvent(action, window, code, text, modifier, delay);
                return;
            }
        }
        if (!widget)
            widget = QApplication::focusWidget();
        if (!widget)
            widget = QApplication::activeWindow();

        QTEST_ASSERT(widget);

        if (action == Click) {
            QPointer<QWidget> ptr(widget);
            sendKeyEvent(Press, widget, code, text, modifier, delay);
            if (!ptr) {
                // if we send key-events to embedded widgets, they might be destroyed
                // when the user presses Return
                return;
            }
            sendKeyEvent(Release, widget, code, text, modifier, delay);
            return;
        }

        bool repeat = false;

        if (action == Press) {
            if (modifier & Qt::ShiftModifier)
                simulateEvent(widget, true, Qt::Key_Shift, 0, QString(), false, delay);

            if (modifier & Qt::ControlModifier)
                simulateEvent(widget, true, Qt::Key_Control, modifier & Qt::ShiftModifier, QString(), false, delay);

            if (modifier & Qt::AltModifier)
                simulateEvent(widget, true, Qt::Key_Alt,
                              modifier & (Qt::ShiftModifier | Qt::ControlModifier), QString(), false, delay);
            if (modifier & Qt::MetaModifier)
                simulateEvent(widget, true, Qt::Key_Meta, modifier & (Qt::ShiftModifier
                                                                      | Qt::ControlModifier | Qt::AltModifier), QString(), false, delay);
            simulateEvent(widget, true, code, modifier, text, repeat, delay);
        } else if (action == Release) {
            simulateEvent(widget, false, code, modifier, text, repeat, delay);

            if (modifier & Qt::MetaModifier)
                simulateEvent(widget, false, Qt::Key_Meta, modifier, QString(), false, delay);
            if (modifier & Qt::AltModifier)
                simulateEvent(widget, false, Qt::Key_Alt, modifier &
                              (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier), QString(), false, delay);

            if (modifier & Qt::ControlModifier)
                simulateEvent(widget, false, Qt::Key_Control,
                              modifier & (Qt::ShiftModifier | Qt::ControlModifier), QString(), false, delay);

            if (modifier & Qt::ShiftModifier)
                simulateEvent(widget, false, Qt::Key_Shift, modifier & Qt::ShiftModifier, QString(), false, delay);
        }
    }

    // Convenience function
    static void sendKeyEvent(KeyAction action, QWidget *widget, Qt::Key code,
                             char ascii, Qt::KeyboardModifiers modifier, int delay=-1)
    {
        QString text;
        if (ascii)
            text = QString(QChar::fromLatin1(ascii));
        sendKeyEvent(action, widget, code, text, modifier, delay);
    }

    inline static void keyEvent(KeyAction action, QWidget *widget, char ascii,
                                Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { sendKeyEvent(action, widget, asciiToKey(ascii), ascii, modifier, delay); }
    inline static void keyEvent(KeyAction action, QWidget *widget, Qt::Key key,
                                Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { sendKeyEvent(action, widget, key, keyToAscii(key), modifier, delay); }

    inline static void keyClicks(QWidget *widget, const QString &sequence,
                                 Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    {
        for (int i=0; i < sequence.length(); i++)
            keyEvent(Click, widget, sequence.at(i).toLatin1(), modifier, delay);
    }

    inline static void keyPress(QWidget *widget, char key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { keyEvent(Press, widget, key, modifier, delay); }
    inline static void keyRelease(QWidget *widget, char key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { keyEvent(Release, widget, key, modifier, delay); }
    inline static void keyClick(QWidget *widget, char key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { keyEvent(Click, widget, key, modifier, delay); }
    inline static void keyPress(QWidget *widget, Qt::Key key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { keyEvent(Press, widget, key, modifier, delay); }
    inline static void keyRelease(QWidget *widget, Qt::Key key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { keyEvent(Release, widget, key, modifier, delay); }
    inline static void keyClick(QWidget *widget, Qt::Key key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { keyEvent(Click, widget, key, modifier, delay); }
#endif // QT_WIDGETS_LIB

}

QT_END_NAMESPACE

#endif // QTESTKEYBOARD_H
