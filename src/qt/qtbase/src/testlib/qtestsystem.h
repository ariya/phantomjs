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

#ifndef QTESTSYSTEM_H
#define QTESTSYSTEM_H

#include <QtTest/qtestcase.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qelapsedtimer.h>
#ifdef QT_GUI_LIB
#  include <QtGui/QWindow>
#endif
#ifdef QT_WIDGETS_LIB
#  include <QtWidgets/QWidget>
#endif

QT_BEGIN_NAMESPACE

namespace QTest
{
    Q_DECL_UNUSED inline static void qWait(int ms)
    {
        Q_ASSERT(QCoreApplication::instance());

        QElapsedTimer timer;
        timer.start();
        do {
            QCoreApplication::processEvents(QEventLoop::AllEvents, ms);
            QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
            QTest::qSleep(10);
        } while (timer.elapsed() < ms);
    }

#ifdef QT_GUI_LIB
    inline static bool qWaitForWindowActive(QWindow *window, int timeout = 5000)
    {
        QElapsedTimer timer;
        timer.start();
        while (!window->isActive()) {
            int remaining = timeout - int(timer.elapsed());
            if (remaining <= 0)
                break;
            QCoreApplication::processEvents(QEventLoop::AllEvents, remaining);
            QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
            QTest::qSleep(10);
        }
        // Try ensuring the platform window receives the real position.
        // (i.e. that window->pos() reflects reality)
        // isActive() ( == FocusIn in case of X) does not guarantee this. It seems some WMs randomly
        // send the final ConfigureNotify (the one with the non-bogus 0,0 position) after the FocusIn.
        // If we just let things go, every mapTo/FromGlobal call the tests perform directly after
        // qWaitForWindowShown() will generate bogus results.
        if (window->isActive()) {
            int waitNo = 0; // 0, 0 might be a valid position after all, so do not wait for ever
            while (window->position().isNull()) {
                if (waitNo++ > timeout / 10)
                    break;
                qWait(10);
            }
        }
        return window->isActive();
    }

    inline static bool qWaitForWindowExposed(QWindow *window, int timeout = 5000)
    {
        QElapsedTimer timer;
        timer.start();
        while (!window->isExposed()) {
            int remaining = timeout - int(timer.elapsed());
            if (remaining <= 0)
                break;
            QCoreApplication::processEvents(QEventLoop::AllEvents, remaining);
            QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
            QTest::qSleep(10);
        }
        return window->isExposed();
    }
#endif

#ifdef QT_WIDGETS_LIB
    inline static bool qWaitForWindowActive(QWidget *widget, int timeout = 1000)
    {
        if (QWindow *window = widget->windowHandle())
            return qWaitForWindowActive(window, timeout);
        return false;
    }

    inline static bool qWaitForWindowExposed(QWidget *widget, int timeout = 1000)
    {
        if (QWindow *window = widget->windowHandle())
            return qWaitForWindowExposed(window, timeout);
        return false;
    }
#endif

#if QT_DEPRECATED_SINCE(5, 0)
#  ifdef QT_WIDGETS_LIB
    QT_DEPRECATED inline static bool qWaitForWindowShown(QWidget *widget, int timeout = 1000)
    {
        return qWaitForWindowExposed(widget, timeout);
    }
#  endif // QT_WIDGETS_LIB
#endif // QT_DEPRECATED_SINCE(5, 0)
}

QT_END_NAMESPACE

#endif

