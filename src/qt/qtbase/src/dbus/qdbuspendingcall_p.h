/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtDBus module of the Qt Toolkit.
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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the public API.  This header file may
// change from version to version without notice, or even be
// removed.
//
// We mean it.
//
//

#ifndef QDBUSPENDINGCALL_P_H
#define QDBUSPENDINGCALL_P_H

#include <qshareddata.h>
#include <qpointer.h>
#include <qvector.h>
#include <qmutex.h>
#include <qwaitcondition.h>

#include "qdbusmessage.h"
#include "qdbus_symbols_p.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class QDBusPendingCall;
class QDBusPendingCallWatcher;
class QDBusPendingCallWatcherHelper;
class QDBusConnectionPrivate;

class QDBusPendingCallPrivate: public QSharedData
{
public:
    // {
    //     set only during construction:
    const QDBusMessage sentMessage;
    QDBusConnectionPrivate * const connection;

    // for the callback mechanism (see setReplyCallback and QDBusConnectionPrivate::sendWithReplyAsync)
    QPointer<QObject> receiver;
    QVector<int> metaTypes;
    int methodIdx;

    // }

    mutable QMutex mutex;
    QWaitCondition waitForFinishedCondition;

    // {
    //    protected by the mutex above:
    QDBusPendingCallWatcherHelper *watcherHelper;
    QDBusMessage replyMessage;
    DBusPendingCall *pending;
    QString expectedReplySignature;
    int expectedReplyCount;
    bool waitingForFinished;
    // }

    QDBusPendingCallPrivate(const QDBusMessage &sent, QDBusConnectionPrivate *connection)
        : sentMessage(sent), connection(connection), watcherHelper(0), pending(0), waitingForFinished(false)
    { }
    ~QDBusPendingCallPrivate();
    bool setReplyCallback(QObject *target, const char *member);
    void waitForFinished();
    void setMetaTypes(int count, const int *types);
    void checkReceivedSignature();

    static QDBusPendingCall fromMessage(const QDBusMessage &msg);
};

class QDBusPendingCallWatcherHelper: public QObject
{
    Q_OBJECT
public:
    void add(QDBusPendingCallWatcher *watcher);

    void emitSignals(const QDBusMessage &replyMessage, const QDBusMessage &sentMessage)
    {
        if (replyMessage.type() == QDBusMessage::ReplyMessage)
            emit reply(replyMessage);
        else
            emit error(QDBusError(replyMessage), sentMessage);
        emit finished();
    }

Q_SIGNALS:
    void finished();
    void reply(const QDBusMessage &msg);
    void error(const QDBusError &error, const QDBusMessage &msg);
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
