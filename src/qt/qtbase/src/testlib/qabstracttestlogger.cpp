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

#include <QtTest/private/qabstracttestlogger_p.h>
#include <QtTest/qtestassert.h>

#include <QtCore/qbytearray.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifndef Q_OS_WIN
#include <unistd.h>
#endif

#ifdef Q_OS_ANDROID
#include <sys/stat.h>
#endif

QT_BEGIN_NAMESPACE

QAbstractTestLogger::QAbstractTestLogger(const char *filename)
{
    if (!filename) {
        stream = stdout;
        return;
    }
#if defined(_MSC_VER) && _MSC_VER >= 1400 && !defined(Q_OS_WINCE)
    if (::fopen_s(&stream, filename, "wt")) {
#else
    stream = ::fopen(filename, "wt");
    if (!stream) {
#endif
        fprintf(stderr, "Unable to open file for logging: %s\n", filename);
        ::exit(1);
    }
#ifdef Q_OS_ANDROID
    else {
        // Make sure output is world-readable on Android
        ::chmod(filename, 0666);
    }
#endif
}

QAbstractTestLogger::~QAbstractTestLogger()
{
    QTEST_ASSERT(stream);
    if (stream != stdout) {
        fclose(stream);
    }
    stream = 0;
}

void QAbstractTestLogger::filterUnprintable(char *str) const
{
    unsigned char *idx = reinterpret_cast<unsigned char *>(str);
    while (*idx) {
        if (((*idx < 0x20 && *idx != '\n' && *idx != '\t') || *idx == 0x7f))
            *idx = '?';
        ++idx;
    }
}

void QAbstractTestLogger::outputString(const char *msg)
{
    QTEST_ASSERT(stream);
    QTEST_ASSERT(msg);

    char *filtered = new char[strlen(msg) + 1];
    strcpy(filtered, msg);
    filterUnprintable(filtered);

    ::fputs(filtered, stream);
    ::fflush(stream);

    delete [] filtered;
}

void QAbstractTestLogger::startLogging()
{
}

void QAbstractTestLogger::stopLogging()
{
}

namespace QTest
{

extern void filter_unprintable(char *str);

/*!
    \fn int QTest::qt_asprintf(QTestCharBuffer *buf, const char *format, ...);
    \internal
 */
int qt_asprintf(QTestCharBuffer *str, const char *format, ...)
{
    static const int MAXSIZE = 1024*1024*2;

    Q_ASSERT(str);

    int size = str->size();

    va_list ap;
    int res = 0;

    for (;;) {
        va_start(ap, format);
        res = qvsnprintf(str->data(), size, format, ap);
        va_end(ap);
        str->data()[size - 1] = '\0';
        if (res >= 0 && res < size) {
            // We succeeded
            break;
        }
        // buffer wasn't big enough, try again.
        // Note, we're assuming that a result of -1 is always due to running out of space.
        size *= 2;
        if (size > MAXSIZE) {
            break;
        }
        if (!str->reset(size))
            break; // out of memory - take what we have
    }

    return res;
}

}

QT_END_NAMESPACE
