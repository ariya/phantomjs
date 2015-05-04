/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QWINOVERLAPPEDIONOTIFIER_P_H
#define QWINOVERLAPPEDIONOTIFIER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qobject.h>

typedef struct _OVERLAPPED OVERLAPPED;

QT_BEGIN_NAMESPACE

class QWinOverlappedIoNotifierPrivate;

class Q_CORE_EXPORT QWinOverlappedIoNotifier : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(QWinOverlappedIoNotifier)
    Q_DECLARE_PRIVATE(QWinOverlappedIoNotifier)
    Q_PRIVATE_SLOT(d_func(), OVERLAPPED *_q_notified())
    friend class QWinIoCompletionPort;
public:
    QWinOverlappedIoNotifier(QObject *parent = 0);
    ~QWinOverlappedIoNotifier();

    void setHandle(Qt::HANDLE h);
    Qt::HANDLE handle() const;

    void setEnabled(bool enabled);
    OVERLAPPED *waitForAnyNotified(int msecs);
    bool waitForNotified(int msecs, OVERLAPPED *overlapped);

Q_SIGNALS:
    void notified(quint32 numberOfBytes, quint32 errorCode, OVERLAPPED *overlapped);
#if !defined(Q_QDOC)
    void _q_notify();
#endif
};

QT_END_NAMESPACE

#endif // QWINOVERLAPPEDIONOTIFIER_P_H
