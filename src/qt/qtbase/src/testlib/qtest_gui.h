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

#ifndef QTEST_GUI_H
#define QTEST_GUI_H

// enable GUI features
#ifndef QT_GUI_LIB
#define QT_GUI_LIB
#endif
#if 0
#pragma qt_class(QtTestGui)
#endif

#include <QtTest/qtestassert.h>
#include <QtTest/qtest.h>
#include <QtTest/qtestevent.h>
#include <QtTest/qtestmouse.h>
#include <QtTest/qtesttouch.h>
#include <QtTest/qtestkeyboard.h>

#include <QtGui/qpixmap.h>
#include <QtGui/qimage.h>

#ifdef QT_WIDGETS_LIB
#include <QtGui/qicon.h>
#endif

#if 0
// inform syncqt
#pragma qt_no_master_include
#endif

QT_BEGIN_NAMESPACE


namespace QTest
{

inline bool qCompare(QIcon const &t1, QIcon const &t2, const char *actual, const char *expected,
                    const char *file, int line)
{
    QTEST_ASSERT(sizeof(QIcon) == sizeof(void *));
    return qCompare(*reinterpret_cast<void * const *>(&t1),
                   *reinterpret_cast<void * const *>(&t2), actual, expected, file, line);
}

inline bool qCompare(QImage const &t1, QImage const &t2,
                     const char *actual, const char *expected, const char *file, int line)
{
    char msg[1024];
    msg[0] = '\0';
    const bool t1Null = t1.isNull();
    const bool t2Null = t2.isNull();
    if (t1Null != t2Null) {
        qsnprintf(msg, 1024, "Compared QImages differ.\n"
                  "   Actual   (%s).isNull(): %d\n"
                  "   Expected (%s).isNull(): %d", actual, t1Null, expected, t2Null);
        return compare_helper(false, msg, 0, 0, actual, expected, file, line);
    }
    if (t1Null && t2Null)
        return compare_helper(true, 0, 0, 0, actual, expected, file, line);
    if (t1.width() != t2.width() || t1.height() != t2.height()) {
        qsnprintf(msg, 1024, "Compared QImages differ in size.\n"
                  "   Actual   (%s): %dx%d\n"
                  "   Expected (%s): %dx%d",
                  actual, t1.width(), t1.height(),
                  expected, t2.width(), t2.height());
        return compare_helper(false, msg, 0, 0, actual, expected, file, line);
    }
    if (t1.format() != t2.format()) {
        qsnprintf(msg, 1024, "Compared QImages differ in format.\n"
                  "   Actual   (%s): %d\n"
                  "   Expected (%s): %d",
                  actual, t1.format(), expected, t2.format());
        return compare_helper(false, msg, 0, 0, actual, expected, file, line);
    }
    return compare_helper(t1 == t2, "Compared values are not the same",
                          toString(t1), toString(t2), actual, expected, file, line);
}

inline bool qCompare(QPixmap const &t1, QPixmap const &t2, const char *actual, const char *expected,
                    const char *file, int line)
{
    char msg[1024];
    msg[0] = '\0';
    const bool t1Null = t1.isNull();
    const bool t2Null = t2.isNull();
    if (t1Null != t2Null) {
        qsnprintf(msg, 1024, "Compared QPixmaps differ.\n"
                  "   Actual   (%s).isNull(): %d\n"
                  "   Expected (%s).isNull(): %d", actual, t1Null, expected, t2Null);
        return compare_helper(false, msg, 0, 0, actual, expected, file, line);
    }
    if (t1Null && t2Null)
        return compare_helper(true, 0, 0, 0, actual, expected, file, line);
    if (t1.width() != t2.width() || t1.height() != t2.height()) {
        qsnprintf(msg, 1024, "Compared QPixmaps differ in size.\n"
                  "   Actual   (%s): %dx%d\n"
                  "   Expected (%s): %dx%d",
                  actual, t1.width(), t1.height(),
                  expected, t2.width(), t2.height());
        return compare_helper(false, msg, 0, 0, actual, expected, file, line);
    }
    return qCompare(t1.toImage(), t2.toImage(), actual, expected, file, line);
}

}

QT_END_NAMESPACE

#endif
