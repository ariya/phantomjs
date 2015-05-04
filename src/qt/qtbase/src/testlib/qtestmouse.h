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

#ifndef QTESTMOUSE_H
#define QTESTMOUSE_H

#if 0
// inform syncqt
#pragma qt_no_master_include
#endif

#include <QtTest/qtest_global.h>
#include <QtTest/qtestassert.h>
#include <QtTest/qtestsystem.h>
#include <QtTest/qtestspontaneevent.h>
#include <QtCore/qpoint.h>
#include <QtCore/qstring.h>
#include <QtGui/qevent.h>
#include <QtGui/qwindow.h>

#ifdef QT_WIDGETS_LIB
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qwidget.h>
#endif

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

Q_GUI_EXPORT void qt_handleMouseEvent(QWindow *w, const QPointF & local, const QPointF & global, Qt::MouseButtons b, Qt::KeyboardModifiers mods = Qt::NoModifier);

namespace QTest
{
    enum MouseAction { MousePress, MouseRelease, MouseClick, MouseDClick, MouseMove };

    static void waitForEvents()
    {
#ifdef Q_OS_MAC
        QTest::qWait(20);
#else
        qApp->processEvents();
#endif
    }

    static void mouseEvent(MouseAction action, QWindow *window, Qt::MouseButton button,
                           Qt::KeyboardModifiers stateKey, QPoint pos, int delay=-1)
    {
        QTEST_ASSERT(window);
        extern int Q_TESTLIB_EXPORT defaultMouseDelay();

        // pos is in window local coordinates
        if (window->geometry().width() <= pos.x() || window->geometry().height() <= pos.y()) {
            QTest::qWarn("Mouse event occurs outside of target window.");
        }

         static Qt::MouseButton lastButton = Qt::NoButton;

        if (delay == -1 || delay < defaultMouseDelay())
            delay = defaultMouseDelay();
        if (delay > 0)
            QTest::qWait(delay);

        if (pos.isNull())
            pos = window->geometry().center();

        if (action == MouseClick) {
            mouseEvent(MousePress, window, button, stateKey, pos);
            mouseEvent(MouseRelease, window, button, stateKey, pos);
            return;
        }
        QTEST_ASSERT(stateKey == 0 || stateKey & Qt::KeyboardModifierMask);

        stateKey &= static_cast<unsigned int>(Qt::KeyboardModifierMask);


        switch (action)
        {
            case MousePress:
                qt_handleMouseEvent(window,pos,window->mapToGlobal(pos),button,stateKey);
                lastButton = button;
                break;
            case MouseRelease:
                qt_handleMouseEvent(window,pos,window->mapToGlobal(pos),Qt::NoButton,stateKey);
                lastButton = Qt::NoButton;
                break;
            case MouseDClick:
                qt_handleMouseEvent(window,pos,window->mapToGlobal(pos),button,stateKey);
                qWait(10);
                qt_handleMouseEvent(window,pos,window->mapToGlobal(pos),Qt::NoButton,stateKey);
                qWait(20);
                qt_handleMouseEvent(window,pos,window->mapToGlobal(pos),button,stateKey);
                qWait(10);
                qt_handleMouseEvent(window,pos,window->mapToGlobal(pos),Qt::NoButton,stateKey);
                break;
            case MouseMove:
                qt_handleMouseEvent(window,pos,window->mapToGlobal(pos),lastButton,stateKey);
                // No QCursor::setPos() call here. That could potentially result in mouse events sent by the windowing system
                // which is highly undesired here. Tests must avoid relying on QCursor.
                break;
            default:
                QTEST_ASSERT(false);
        }
        waitForEvents();
    }

    inline void mousePress(QWindow *window, Qt::MouseButton button, Qt::KeyboardModifiers stateKey = 0,
                           QPoint pos = QPoint(), int delay=-1)
    { mouseEvent(MousePress, window, button, stateKey, pos, delay); }
    inline void mouseRelease(QWindow *window, Qt::MouseButton button, Qt::KeyboardModifiers stateKey = 0,
                             QPoint pos = QPoint(), int delay=-1)
    { mouseEvent(MouseRelease, window, button, stateKey, pos, delay); }
    inline void mouseClick(QWindow *window, Qt::MouseButton button, Qt::KeyboardModifiers stateKey = 0,
                           QPoint pos = QPoint(), int delay=-1)
    { mouseEvent(MouseClick, window, button, stateKey, pos, delay); }
    inline void mouseDClick(QWindow *window, Qt::MouseButton button, Qt::KeyboardModifiers stateKey = 0,
                            QPoint pos = QPoint(), int delay=-1)
    { mouseEvent(MouseDClick, window, button, stateKey, pos, delay); }
    inline void mouseMove(QWindow *window, QPoint pos = QPoint(), int delay=-1)
    { mouseEvent(MouseMove, window, Qt::NoButton, 0, pos, delay); }

#ifdef QT_WIDGETS_LIB
    static void mouseEvent(MouseAction action, QWidget *widget, Qt::MouseButton button,
                           Qt::KeyboardModifiers stateKey, QPoint pos, int delay=-1)
    {
        QTEST_ASSERT(widget);
        extern int Q_TESTLIB_EXPORT defaultMouseDelay();

        if (delay == -1 || delay < defaultMouseDelay())
            delay = defaultMouseDelay();
        if (delay > 0)
            QTest::qWait(delay);

        if (pos.isNull())
            pos = widget->rect().center();

        if (action == MouseClick) {
            mouseEvent(MousePress, widget, button, stateKey, pos);
            mouseEvent(MouseRelease, widget, button, stateKey, pos);
            return;
        }

        QTEST_ASSERT(stateKey == 0 || stateKey & Qt::KeyboardModifierMask);

        stateKey &= static_cast<unsigned int>(Qt::KeyboardModifierMask);

        QMouseEvent me(QEvent::User, QPoint(), Qt::LeftButton, button, stateKey);
        switch (action)
        {
            case MousePress:
                me = QMouseEvent(QEvent::MouseButtonPress, pos, widget->mapToGlobal(pos), button, button, stateKey);
                break;
            case MouseRelease:
                me = QMouseEvent(QEvent::MouseButtonRelease, pos, widget->mapToGlobal(pos), button, 0, stateKey);
                break;
            case MouseDClick:
                me = QMouseEvent(QEvent::MouseButtonDblClick, pos, widget->mapToGlobal(pos), button, button, stateKey);
                break;
            case MouseMove:
                QCursor::setPos(widget->mapToGlobal(pos));
#ifdef Q_OS_MAC
                QTest::qWait(20);
#else
                qApp->processEvents();
#endif
                return;
            default:
                QTEST_ASSERT(false);
        }
        QSpontaneKeyEvent::setSpontaneous(&me);
        if (!qApp->notify(widget, &me)) {
            static const char *mouseActionNames[] =
                { "MousePress", "MouseRelease", "MouseClick", "MouseDClick", "MouseMove" };
            QString warning = QString::fromLatin1("Mouse event \"%1\" not accepted by receiving widget");
            QTest::qWarn(warning.arg(QString::fromLatin1(mouseActionNames[static_cast<int>(action)])).toLatin1().data());
        }

    }

    inline void mousePress(QWidget *widget, Qt::MouseButton button, Qt::KeyboardModifiers stateKey = 0,
                           QPoint pos = QPoint(), int delay=-1)
    { mouseEvent(MousePress, widget, button, stateKey, pos, delay); }
    inline void mouseRelease(QWidget *widget, Qt::MouseButton button, Qt::KeyboardModifiers stateKey = 0,
                             QPoint pos = QPoint(), int delay=-1)
    { mouseEvent(MouseRelease, widget, button, stateKey, pos, delay); }
    inline void mouseClick(QWidget *widget, Qt::MouseButton button, Qt::KeyboardModifiers stateKey = 0,
                           QPoint pos = QPoint(), int delay=-1)
    { mouseEvent(MouseClick, widget, button, stateKey, pos, delay); }
    inline void mouseDClick(QWidget *widget, Qt::MouseButton button, Qt::KeyboardModifiers stateKey = 0,
                            QPoint pos = QPoint(), int delay=-1)
    { mouseEvent(MouseDClick, widget, button, stateKey, pos, delay); }
    inline void mouseMove(QWidget *widget, QPoint pos = QPoint(), int delay=-1)
    { mouseEvent(MouseMove, widget, Qt::NoButton, 0, pos, delay); }
#endif // QT_WIDGETS_LIB
}

QT_END_NAMESPACE

#endif // QTESTMOUSE_H
