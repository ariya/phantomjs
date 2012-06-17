/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#ifndef QHOSTINFO_P_H
#define QHOSTINFO_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QHostInfo class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qcoreapplication.h"
#include "private/qcoreapplication_p.h"
#include "QtNetwork/qhostinfo.h"
#include "QtCore/qmutex.h"
#include "QtCore/qwaitcondition.h"
#include "QtCore/qobject.h"
#include "QtCore/qpointer.h"
#include "QtCore/qthread.h"
#include "QtCore/qthreadpool.h"
#include "QtCore/qmutex.h"
#include "QtCore/qrunnable.h"
#include "QtCore/qlist.h"
#include "QtCore/qqueue.h"
#include <QElapsedTimer>
#include <QCache>

#include <QNetworkSession>
#include <QSharedPointer>

#ifdef Q_OS_SYMBIAN
// Symbian Headers
#include <es_sock.h>
#include <in_sock.h>
#endif


QT_BEGIN_NAMESPACE


class QHostInfoResult : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    inline void emitResultsReady(const QHostInfo &info)
    {
        emit resultsReady(info);
    }

Q_SIGNALS:
    void resultsReady(const QHostInfo &info);
};

// needs to be QObject because fromName calls tr()
class QHostInfoAgent : public QObject
{
    Q_OBJECT
public:
    static QHostInfo fromName(const QString &hostName);
#ifndef QT_NO_BEARERMANAGEMENT
    static QHostInfo fromName(const QString &hostName, QSharedPointer<QNetworkSession> networkSession);
#endif

#ifdef Q_OS_SYMBIAN
    static int lookupHost(const QString &name, QObject *receiver, const char *member);
    static void abortHostLookup(int lookupId);
#endif
};

class QHostInfoPrivate
{
public:
    inline QHostInfoPrivate()
        : err(QHostInfo::NoError),
          errorStr(QLatin1String(QT_TRANSLATE_NOOP("QHostInfo", "Unknown error"))),
          lookupId(0)
    {
    }
#ifndef QT_NO_BEARERMANAGEMENT
    //not a public API yet
    static QHostInfo fromName(const QString &hostName, QSharedPointer<QNetworkSession> networkSession);
#endif

    QHostInfo::HostInfoError err;
    QString errorStr;
    QList<QHostAddress> addrs;
    QString hostName;
    int lookupId;
};

// These functions are outside of the QHostInfo class and strictly internal.
// Do NOT use them outside of QAbstractSocket.
QHostInfo Q_NETWORK_EXPORT qt_qhostinfo_lookup(const QString &name, QObject *receiver, const char *member, bool *valid, int *id);
void Q_AUTOTEST_EXPORT qt_qhostinfo_clear_cache();
void Q_AUTOTEST_EXPORT qt_qhostinfo_enable_cache(bool e);

class QHostInfoCache
{
public:
    QHostInfoCache();
    const int max_age; // seconds

    QHostInfo get(const QString &name, bool *valid);
    void put(const QString &name, const QHostInfo &info);
    void clear();

    bool isEnabled();
    void setEnabled(bool e);
private:
    bool enabled;
    struct QHostInfoCacheElement {
        QHostInfo info;
        QElapsedTimer age;
    };
    QCache<QString,QHostInfoCacheElement> cache;
    QMutex mutex;
};

// the following classes are used for the (normal) case: We use multiple threads to lookup DNS

class QHostInfoRunnable : public QRunnable
{
public:
    QHostInfoRunnable (QString hn, int i);
    void run();

    QString toBeLookedUp;
    int id;
    QHostInfoResult resultEmitter;
};


class QAbstractHostInfoLookupManager : public QObject
{
    Q_OBJECT

public:
    ~QAbstractHostInfoLookupManager() {}
    virtual void clear() = 0;

    QHostInfoCache cache;

protected:
     QAbstractHostInfoLookupManager() {}
     static QAbstractHostInfoLookupManager* globalInstance();

};

#ifndef Q_OS_SYMBIAN
class QHostInfoLookupManager : public QAbstractHostInfoLookupManager
{
    Q_OBJECT
public:
    QHostInfoLookupManager();
    ~QHostInfoLookupManager();

    void clear();
    void work();

    // called from QHostInfo
    void scheduleLookup(QHostInfoRunnable *r);
    void abortLookup(int id);

    // called from QHostInfoRunnable
    void lookupFinished(QHostInfoRunnable *r);
    bool wasAborted(int id);

    friend class QHostInfoRunnable;
protected:
    QList<QHostInfoRunnable*> currentLookups; // in progress
    QList<QHostInfoRunnable*> postponedLookups; // postponed because in progress for same host
    QQueue<QHostInfoRunnable*> scheduledLookups; // not yet started
    QList<QHostInfoRunnable*> finishedLookups; // recently finished
    QList<int> abortedLookups; // ids of aborted lookups

    QThreadPool threadPool;

    QMutex mutex;

    bool wasDeleted;

private slots:
    void waitForThreadPoolDone() { threadPool.waitForDone(); }
};

#else

class QSymbianHostResolver : public CActive
{
public:
    QSymbianHostResolver(const QString &hostName, int id, QSharedPointer<QNetworkSession> networkSession);
    ~QSymbianHostResolver();

    void requestHostLookup();
    void abortHostLookup();
    int id();

    void returnResults();

    QHostInfoResult resultEmitter;

private:
    void DoCancel();
    void RunL();
    void run();
    TInt RunError(TInt aError);

    void processNameResult();
    void nextNameResult();
    void processAddressResult();

private:
    int iId;

    const QString iHostName;
    QString iEncodedHostName;
    TPtrC iHostNamePtr;

    RSocketServ& iSocketServ;
    RHostResolver iHostResolver;
    QSharedPointer<QNetworkSession> iNetworkSession;

    TNameEntry iNameResult;
    TInetAddr IpAdd;

    QHostAddress iAddress;

    QHostInfo iResults;

    QList<QHostAddress> iHostAddresses;

    enum {
        EIdle,
        EGetByName,
        EGetByAddress,
        ECompleteFromCache,
        EError
    } iState;
};

class QSymbianHostInfoLookupManager : public QAbstractHostInfoLookupManager
{
    Q_OBJECT
public:
    QSymbianHostInfoLookupManager();
    ~QSymbianHostInfoLookupManager();

    static QSymbianHostInfoLookupManager* globalInstance();

    int id();
    void clear();

    // called from QHostInfo
    void scheduleLookup(QSymbianHostResolver *r);
    void abortLookup(int id);

    // called from QSymbianHostResolver
    void lookupFinished(QSymbianHostResolver *r);

private:
    void runNextLookup();

    // this is true for single threaded use, with multiple threads the max is ((number of threads) + KMaxConcurrentLookups - 1)
    static const int KMaxConcurrentLookups = 5;

    QList<QSymbianHostResolver*> iCurrentLookups;
    QList<QSymbianHostResolver*> iScheduledLookups;

    QMutex mutex;
};
#endif



QT_END_NAMESPACE

#endif // QHOSTINFO_P_H
