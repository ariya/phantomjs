/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwindowspipewriter_p.h"
#include <string.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_THREAD

QWindowsPipeWriter::QWindowsPipeWriter(HANDLE pipe, QObject * parent)
    : QThread(parent),
      writePipe(INVALID_HANDLE_VALUE),
      quitNow(false),
      hasWritten(false)
{
#if !defined(Q_OS_WINCE) || (_WIN32_WCE >= 0x600)
    DuplicateHandle(GetCurrentProcess(), pipe, GetCurrentProcess(),
                         &writePipe, 0, FALSE, DUPLICATE_SAME_ACCESS);
#else
    Q_UNUSED(pipe);
    writePipe = GetCurrentProcess();
#endif
}

QWindowsPipeWriter::~QWindowsPipeWriter()
{
    lock.lock();
    quitNow = true;
    waitCondition.wakeOne();
    lock.unlock();
    if (!wait(30000))
        terminate();
#if !defined(Q_OS_WINCE) || (_WIN32_WCE >= 0x600)
    CloseHandle(writePipe);
#endif
}

bool QWindowsPipeWriter::waitForWrite(int msecs)
{
    QMutexLocker locker(&lock);
    bool hadWritten = hasWritten;
    hasWritten = false;
    if (hadWritten)
        return true;
    if (!waitCondition.wait(&lock, msecs))
        return false;
    hadWritten = hasWritten;
    hasWritten = false;
    return hadWritten;
}

qint64 QWindowsPipeWriter::write(const char *ptr, qint64 maxlen)
{
    if (!isRunning())
        return -1;

    QMutexLocker locker(&lock);
    data.append(QByteArray(ptr, maxlen));
    waitCondition.wakeOne();
    return maxlen;
}

void QWindowsPipeWriter::run()
{
    OVERLAPPED overl;
    memset(&overl, 0, sizeof overl);
    overl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    forever {
        lock.lock();
        while(data.isEmpty() && (!quitNow)) {
            waitCondition.wakeOne();
            waitCondition.wait(&lock);
        }

        if (quitNow) {
            lock.unlock();
            quitNow = false;
	    break;
        }

        QByteArray copy = data;

        lock.unlock();

        const char *ptrData = copy.data();
        qint64 maxlen = copy.size();
        qint64 totalWritten = 0;
        overl.Offset = 0;
        overl.OffsetHigh = 0;
        while ((!quitNow) && totalWritten < maxlen) {
            DWORD written = 0;
            if (!WriteFile(writePipe, ptrData + totalWritten,
                           maxlen - totalWritten, &written, &overl)) {

                if (GetLastError() == 0xE8/*NT_STATUS_INVALID_USER_BUFFER*/) {
                    // give the os a rest
                    msleep(100);
                    continue;
                }
#ifndef Q_OS_WINCE
                if (GetLastError() == ERROR_IO_PENDING) {
                  if (!GetOverlappedResult(writePipe, &overl, &written, TRUE)) {
                      CloseHandle(overl.hEvent);
                      return;
                  }
                } else {
                    CloseHandle(overl.hEvent);
                    return;
                }
#else
                return;
#endif
            }
            totalWritten += written;
#if defined QPIPEWRITER_DEBUG
            qDebug("QWindowsPipeWriter::run() wrote %d %d/%d bytes",
			    written, int(totalWritten), int(maxlen));
#endif
            lock.lock();
            data.remove(0, written);
            hasWritten = true;
            lock.unlock();
        }
        emit bytesWritten(totalWritten);
        emit canWrite();
    }
    CloseHandle(overl.hEvent);
}

#endif //QT_NO_THREAD

QT_END_NAMESPACE
