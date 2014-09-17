/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qsystemsemaphore.h"
#include "qsystemsemaphore_p.h"
#include "qcoreapplication.h"
#include <qdebug.h>

#include "qcore_symbian_p.h"
#include <e32cmn.h>

#ifndef QT_NO_SYSTEMSEMAPHORE

//#define QSYSTEMSEMAPHORE_DEBUG

QT_BEGIN_NAMESPACE

QSystemSemaphorePrivate::QSystemSemaphorePrivate() :
    error(QSystemSemaphore::NoError)
{
}

void QSystemSemaphorePrivate::setErrorString(const QString &function, int err)
{
    if (err == KErrNone)
        return;

    switch(err){
    case KErrAlreadyExists:
        errorString = QCoreApplication::tr("%1: already exists", "QSystemSemaphore").arg(function);
        error = QSystemSemaphore::AlreadyExists;
        break;
    case KErrNotFound:
        errorString = QCoreApplication::tr("%1: does not exist", "QSystemSemaphore").arg(function);
        error = QSystemSemaphore::NotFound;
        break;
    case KErrNoMemory:
    case KErrInUse:
        errorString = QCoreApplication::tr("%1: out of resources", "QSystemSemaphore").arg(function);
        error = QSystemSemaphore::OutOfResources;
        break;
    case KErrPermissionDenied:
        errorString = QCoreApplication::tr("%1: permission denied", "QSystemSemaphore").arg(function);
        error = QSystemSemaphore::PermissionDenied;
        break;
    default:
        errorString = QCoreApplication::tr("%1: unknown error %2", "QSystemSemaphore").arg(function).arg(err);
        error = QSystemSemaphore::UnknownError;
#ifdef QSYSTEMSEMAPHORE_DEBUG
        qDebug() << errorString << "key" << key;
#endif
        break;
    }
}

int QSystemSemaphorePrivate::handle(QSystemSemaphore::AccessMode)
{
    if (semaphore.Handle())
        return semaphore.Handle();

    // don't allow making handles on empty keys
    if (key.isEmpty())
        return 0;

    TPtrC name(qt_QString2TPtrC(fileName));
    int err = KErrAlreadyExists;
    int tryCount = 10;
    // Sort out race conditions by retrying several times until existing handle is acquired.
    // Sometimes opening can fail inexplicably with KErrPermissionDenied many times in a row.
    while (err != KErrNoMemory && err != KErrNone && tryCount-- >= 0) {
        err = semaphore.CreateGlobal(name, initialValue, EOwnerProcess);
        if (err != KErrNoMemory && err != KErrNone)
            err = semaphore.OpenGlobal(name, EOwnerProcess);
    }
    if (err) {
        setErrorString(QLatin1String("QSystemSemaphore::handle"), err);
        return 0;
    }

    return semaphore.Handle();
}

void QSystemSemaphorePrivate::cleanHandle()
{
    semaphore.Close();
}

bool QSystemSemaphorePrivate::modifySemaphore(int count)
{
    if (0 == handle())
        return false;

    if (count > 0)
        semaphore.Signal(count);
    else
        semaphore.Wait();

    return true;
}

QT_END_NAMESPACE

#endif // QT_NO_SYSTEMSEMAPHORE
