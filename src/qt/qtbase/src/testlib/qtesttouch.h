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

#ifndef QTESTTOUCH_H
#define QTESTTOUCH_H

#if 0
// inform syncqt
#pragma qt_no_master_include
#endif

#include <QtTest/qtest_global.h>
#include <QtTest/qtestassert.h>
#include <QtTest/qtestsystem.h>
#include <QtTest/qtestspontaneevent.h>
#include <QtCore/qmap.h>
#include <QtGui/qevent.h>
#include <QtGui/qwindow.h>
#ifdef QT_WIDGETS_LIB
#include <QtWidgets/qwidget.h>
#endif

QT_BEGIN_NAMESPACE

Q_GUI_EXPORT  void qt_handleTouchEvent(QWindow *w, QTouchDevice *device,
                                const QList<QTouchEvent::TouchPoint> &points,
                                Qt::KeyboardModifiers mods = Qt::NoModifier);


namespace QTest
{

    class QTouchEventSequence
    {
    public:
        ~QTouchEventSequence()
        {
            if (commitWhenDestroyed)
                commit();
        }
        QTouchEventSequence& press(int touchId, const QPoint &pt, QWindow *window = 0)
        {
            QTouchEvent::TouchPoint &p = point(touchId);
            p.setScreenPos(mapToScreen(window, pt));
            p.setState(Qt::TouchPointPressed);
            return *this;
        }
        QTouchEventSequence& move(int touchId, const QPoint &pt, QWindow *window = 0)
        {
            QTouchEvent::TouchPoint &p = point(touchId);
            p.setScreenPos(mapToScreen(window, pt));
            p.setState(Qt::TouchPointMoved);
            return *this;
        }
        QTouchEventSequence& release(int touchId, const QPoint &pt, QWindow *window = 0)
        {
            QTouchEvent::TouchPoint &p = point(touchId);
            p.setScreenPos(mapToScreen(window, pt));
            p.setState(Qt::TouchPointReleased);
            return *this;
        }
        QTouchEventSequence& stationary(int touchId)
        {
            QTouchEvent::TouchPoint &p = pointOrPreviousPoint(touchId);
            p.setState(Qt::TouchPointStationary);
            return *this;
        }

#ifdef QT_WIDGETS_LIB
        QTouchEventSequence& press(int touchId, const QPoint &pt, QWidget *widget = 0)
        {
            QTouchEvent::TouchPoint &p = point(touchId);
            p.setScreenPos(mapToScreen(widget, pt));
            p.setState(Qt::TouchPointPressed);
            return *this;
        }
        QTouchEventSequence& move(int touchId, const QPoint &pt, QWidget *widget = 0)
        {
            QTouchEvent::TouchPoint &p = point(touchId);
            p.setScreenPos(mapToScreen(widget, pt));
            p.setState(Qt::TouchPointMoved);
            return *this;
        }
        QTouchEventSequence& release(int touchId, const QPoint &pt, QWidget *widget = 0)
        {
            QTouchEvent::TouchPoint &p = point(touchId);
            p.setScreenPos(mapToScreen(widget, pt));
            p.setState(Qt::TouchPointReleased);
            return *this;
        }
#endif

        void commit(bool processEvents = true)
        {
            if (!points.isEmpty()) {
                if (targetWindow)
                {
                    qt_handleTouchEvent(targetWindow, device, points.values());
                }
#ifdef QT_WIDGETS_LIB
                else if (targetWidget)
                {
                    qt_handleTouchEvent(targetWidget->windowHandle(), device, points.values());
                }
#endif
            }
            if (processEvents)
                QCoreApplication::processEvents();
            previousPoints = points;
            points.clear();
        }

private:
#ifdef QT_WIDGETS_LIB
        QTouchEventSequence(QWidget *widget, QTouchDevice *aDevice, bool autoCommit)
            : targetWidget(widget), targetWindow(0), device(aDevice), commitWhenDestroyed(autoCommit)
        {
        }
#endif
        QTouchEventSequence(QWindow *window, QTouchDevice *aDevice, bool autoCommit)
            :
#ifdef QT_WIDGETS_LIB
              targetWidget(0),
#endif
              targetWindow(window), device(aDevice), commitWhenDestroyed(autoCommit)
        {
        }

        QTouchEvent::TouchPoint &point(int touchId)
        {
            if (!points.contains(touchId))
                points[touchId] = QTouchEvent::TouchPoint(touchId);
            return points[touchId];
        }

        QTouchEvent::TouchPoint &pointOrPreviousPoint(int touchId)
        {
            if (!points.contains(touchId)) {
                if (previousPoints.contains(touchId))
                    points[touchId] = previousPoints.value(touchId);
                else
                    points[touchId] = QTouchEvent::TouchPoint(touchId);
            }
            return points[touchId];
        }

#ifdef QT_WIDGETS_LIB
        QPoint mapToScreen(QWidget *widget, const QPoint &pt)
        {
            if (widget)
                return widget->mapToGlobal(pt);
            return targetWidget ? targetWidget->mapToGlobal(pt) : pt;
        }
#endif
        QPoint mapToScreen(QWindow *window, const QPoint &pt)
        {
            if(window)
                return window->mapToGlobal(pt);
            return targetWindow ? targetWindow->mapToGlobal(pt) : pt;
        }

        QMap<int, QTouchEvent::TouchPoint> previousPoints;
        QMap<int, QTouchEvent::TouchPoint> points;
#ifdef QT_WIDGETS_LIB
        QWidget *targetWidget;
#endif
        QWindow *targetWindow;
        QTouchDevice *device;
        bool commitWhenDestroyed;
#ifdef QT_WIDGETS_LIB
        friend QTouchEventSequence touchEvent(QWidget *, QTouchDevice*, bool);
#endif
        friend QTouchEventSequence touchEvent(QWindow *, QTouchDevice*, bool);
    };

#ifdef QT_WIDGETS_LIB
    inline
    QTouchEventSequence touchEvent(QWidget *widget,
                                   QTouchDevice *device,
                                   bool autoCommit = true)
    {
        return QTouchEventSequence(widget, device, autoCommit);
    }
#endif
    inline
    QTouchEventSequence touchEvent(QWindow *window,
                                   QTouchDevice *device,
                                   bool autoCommit = true)
    {
        return QTouchEventSequence(window, device, autoCommit);
    }

}

QT_END_NAMESPACE

#endif // QTESTTOUCH_H
