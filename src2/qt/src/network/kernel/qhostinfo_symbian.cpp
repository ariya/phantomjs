/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

//#define QHOSTINFO_DEBUG

// Qt Headers
#include <QByteArray>
#include <QUrl>
#include <QList>

#include "qplatformdefs.h"

#include "qhostinfo_p.h"
#include <private/qcore_symbian_p.h>
#include <private/qsystemerror_p.h>
#include <private/qnetworksession_p.h>
#include <private/qhostaddress_p.h>

// Header does not exist in the S60 5.0 SDK
//#include <networking/dnd_err.h>
const TInt KErrDndNameNotFound = -5120; // Returned when no data found for GetByName
const TInt KErrDndAddrNotFound = -5121; // Returned when no data found for GetByAddr

QT_BEGIN_NAMESPACE

static void setError_helper(QHostInfo &info, TInt symbianError)
{
    switch (symbianError) {
    case KErrDndNameNotFound:
    case KErrDndAddrNotFound:
    case KErrNotFound:
    case KErrEof:
        // various "no more results" error codes
        info.setError(QHostInfo::HostNotFound);
        info.setErrorString(QObject::tr("Host not found"));
        break;
    default:
        // Unknown error
        info.setError(QHostInfo::UnknownError);
        info.setErrorString(QSystemError(symbianError, QSystemError::NativeError).toString());
        break;
    }
}

QHostInfo QHostInfoAgent::fromName(const QString &hostName, QSharedPointer<QNetworkSession> networkSession)
{
    QHostInfo results;

    // Connect to ESOCK
    RSocketServ socketServ(qt_symbianGetSocketServer());
    RHostResolver hostResolver;


    int err;
    if (networkSession)
        err = QNetworkSessionPrivate::nativeOpenHostResolver(*networkSession, hostResolver, KAfInet, KProtocolInetUdp);
    else
        err = hostResolver.Open(socketServ, KAfInet, KProtocolInetUdp);
    if (err) {
        setError_helper(results, err);
        return results;
    }

    TNameEntry nameResult;

#if defined(QHOSTINFO_DEBUG)
    qDebug("QHostInfoAgent::fromName(%s) looking up...",
           hostName.toLatin1().constData());
#endif

    QHostAddress address;
    if (address.setAddress(hostName)) {
        // Reverse lookup
#if defined(QHOSTINFO_DEBUG)
        qDebug("(reverse lookup)");
#endif
        TInetAddr IpAdd;
        IpAdd.Input(qt_QString2TPtrC(hostName));

        // Synchronous request. nameResult returns Host Name.
        err = hostResolver.GetByAddress(IpAdd, nameResult);
        if (err) {
            //for behavioural compatibility with Qt 4.7 and unix/windows
            //backends: don't report error, return ip address as host name
            results.setHostName(address.toString());
        } else {
            results.setHostName(qt_TDesC2QString(nameResult().iName));
        }
        results.setAddresses(QList<QHostAddress>() << address);
        return results;
    }

    // IDN support
    QByteArray aceHostname = QUrl::toAce(hostName);
    results.setHostName(hostName);
    if (aceHostname.isEmpty()) {
        results.setError(QHostInfo::HostNotFound);
        results.setErrorString(hostName.isEmpty() ?
                               QCoreApplication::translate("QHostInfoAgent", "No host name given") :
                               QCoreApplication::translate("QHostInfoAgent", "Invalid hostname"));
        return results;
    }


    // Call RHostResolver::GetByAddress, and place all IPv4 addresses at the start and
    // the IPv6 addresses at the end of the address list in results.

    // Synchronous request.
    err = hostResolver.GetByName(qt_QString2TPtrC(QString::fromLatin1(aceHostname)), nameResult);
    if (err) {
        setError_helper(results, err);
        return results;
    }

    QList<QHostAddress> hostAddresses;

    TInetAddr hostAdd = nameResult().iAddr;

    if (!(nameResult().iFlags & TNameRecord::EAlias) && !(hostAdd.IsUnspecified()))
        hostAddresses.append(qt_QHostAddressFromTInetAddr(hostAdd));

    // Check if there's more than one IP address linkd to this name
    while (hostResolver.Next(nameResult) == KErrNone) {
        hostAdd = nameResult().iAddr;

        // Ensure that record is valid (not an alias and with length greater than 0)
        if (!(nameResult().iFlags & TNameRecord::EAlias) && !(hostAdd.IsUnspecified()))
            hostAddresses.append(qt_QHostAddressFromTInetAddr(hostAdd));
    }

    hostResolver.Close();

    results.setAddresses(hostAddresses);
    return results;
}

QHostInfo QHostInfoAgent::fromName(const QString &hostName)
{
    // null shared pointer
    QSharedPointer<QNetworkSession> networkSession;
    return fromName(hostName, networkSession);
}

QString QHostInfo::localHostName()
{
    // Connect to ESOCK
    RSocketServ socketServ(qt_symbianGetSocketServer());
    RHostResolver hostResolver;

    // RConnection not required to get the host name
    int err = hostResolver.Open(socketServ, KAfInet, KProtocolInetUdp);
    if (err)
        return QString();

    THostName hostName;
    err = hostResolver.GetHostName(hostName);
    if (err)
        return QString();

    hostResolver.Close();

    return qt_TDesC2QString(hostName);
}

QString QHostInfo::localDomainName()
{
    // This concept does not exist on Symbian OS because the device can be on
    // multiple networks with multiple "local domain" names.
    // For now, return a null string.
    return QString();
}


QSymbianHostResolver::QSymbianHostResolver(const QString &hostName, int identifier, QSharedPointer<QNetworkSession> networkSession)
    : CActive(CActive::EPriorityStandard), iHostName(hostName),
      iSocketServ(qt_symbianGetSocketServer()), iNetworkSession(networkSession), iResults(identifier)
{
    CActiveScheduler::Add(this);
}

QSymbianHostResolver::~QSymbianHostResolver()
{
#if defined(QHOSTINFO_DEBUG)
    qDebug() << "QSymbianHostInfoLookupManager::~QSymbianHostResolver" << id();
#endif
    Cancel();
    iHostResolver.Close();
}

// Async equivalent to QHostInfoAgent::fromName()
void QSymbianHostResolver::requestHostLookup()
{

#if defined(QHOSTINFO_DEBUG)
    qDebug("QSymbianHostResolver::requestHostLookup(%s) looking up... (id = %d)",
        iHostName.toLatin1().constData(), id());
#endif

    QSymbianHostInfoLookupManager *manager = QSymbianHostInfoLookupManager::globalInstance();
    if (manager->cache.isEnabled()) {
        //check if name has been put in the cache while this request was queued
        bool valid;
        QHostInfo cachedResult = manager->cache.get(iHostName, &valid);
        if (valid) {
#if defined(QHOSTINFO_DEBUG)
            qDebug("...found in cache");
#endif
            iResults = cachedResult;
            iState = ECompleteFromCache;
            SetActive();
            TRequestStatus* stat = &iStatus;
            User::RequestComplete(stat, KErrNone);
            return;
        }
    }

    int err;
    if (iNetworkSession) {
        err = QNetworkSessionPrivate::nativeOpenHostResolver(*iNetworkSession, iHostResolver, KAfInet, KProtocolInetUdp);
#if defined(QHOSTINFO_DEBUG)
        qDebug("using resolver from session (err = %d)", err);
#endif
    } else {
        err = iHostResolver.Open(iSocketServ, KAfInet, KProtocolInetUdp);
#if defined(QHOSTINFO_DEBUG)
        qDebug("using default resolver (err = %d)", err);
#endif
    }
    if (err) {
        setError_helper(iResults, err);
    } else {

        if (iAddress.setAddress(iHostName)) {
            // Reverse lookup
            IpAdd.Input(qt_QString2TPtrC(iHostName));

            // Asynchronous request.
            iHostResolver.GetByAddress(IpAdd, iNameResult, iStatus); // <---- ASYNC
            iState = EGetByAddress;

        } else {

            // IDN support
            QByteArray aceHostname = QUrl::toAce(iHostName);
            iResults.setHostName(iHostName);
            if (aceHostname.isEmpty()) {
                iResults.setError(QHostInfo::HostNotFound);
                iResults.setErrorString(iHostName.isEmpty() ?
                                       QCoreApplication::translate("QHostInfoAgent", "No host name given") :
                                       QCoreApplication::translate("QHostInfoAgent", "Invalid hostname"));

                err = KErrArgument;
            } else {
                iEncodedHostName = QString::fromLatin1(aceHostname);
                iHostNamePtr.Set(qt_QString2TPtrC(iEncodedHostName));

                // Asynchronous request.
                iHostResolver.GetByName(iHostNamePtr, iNameResult, iStatus);
                iState = EGetByName;
            }
        }
    }
    SetActive();
    if (err) {
        iHostResolver.Close();

        //self complete so that RunL can inform manager without causing recursion
        iState = EError;
        TRequestStatus* stat = &iStatus;
        User::RequestComplete(stat, err);
    }
}

void QSymbianHostResolver::abortHostLookup()
{
    if (resultEmitter.thread() == QThread::currentThread()) {
#ifdef QHOSTINFO_DEBUG
        qDebug("QSymbianHostResolver::abortHostLookup - deleting %d", id());
#endif
        //normal case, abort from same thread it was started
        delete this; //will cancel outstanding request
    } else {
#ifdef QHOSTINFO_DEBUG
        qDebug("QSymbianHostResolver::abortHostLookup - detaching %d", id());
#endif
        //abort from different thread, carry on but don't report the results
        resultEmitter.disconnect();
    }
}

void QSymbianHostResolver::DoCancel()
{
#if defined(QHOSTINFO_DEBUG)
    qDebug() << "QSymbianHostResolver::DoCancel" << QThread::currentThreadId() << id() << (int)iState << this;
#endif
    if (iState == EGetByAddress || iState == EGetByName) {
        //these states have made an async request to host resolver
        iHostResolver.Cancel();
    } else {
        //for the self completing states there is nothing to cancel
        Q_ASSERT(iState == EError || iState == ECompleteFromCache);
    }
}

void QSymbianHostResolver::RunL()
{
    QT_TRYCATCH_LEAVING(run());
}

void QSymbianHostResolver::run()
{
    switch (iState) {
    case EGetByName:
        processNameResult();
        break;
    case EGetByAddress:
        processAddressResult();
        break;
    case ECompleteFromCache:
    case EError:
        returnResults();
        break;
    default:
        qWarning("QSymbianHostResolver internal error, bad state in run()");
        iResults.setError(QHostInfo::UnknownError);
        iResults.setErrorString(QSystemError(KErrCorrupt,QSystemError::NativeError).toString());
        returnResults();
    }
}

void QSymbianHostResolver::returnResults()
{
#if defined(QHOSTINFO_DEBUG)
    qDebug() << "QSymbianHostResolver::returnResults" << iResults.error() << iResults.errorString();
    foreach (QHostAddress addr, iResults.addresses())
        qDebug() << addr;
#endif
    iState = EIdle;

    QSymbianHostInfoLookupManager *manager = QSymbianHostInfoLookupManager::globalInstance();
    if (manager->cache.isEnabled()) {
        manager->cache.put(iHostName, iResults);
    }
    manager->lookupFinished(this);

    resultEmitter.emitResultsReady(iResults);

    delete this;
}

TInt QSymbianHostResolver::RunError(TInt aError)
{
    QT_TRY {
        iState = EIdle;

        QSymbianHostInfoLookupManager *manager = QSymbianHostInfoLookupManager::globalInstance();
        manager->lookupFinished(this);

        setError_helper(iResults, aError);

        resultEmitter.emitResultsReady(iResults);
    }
    QT_CATCH(...) {}

    delete this;

    return KErrNone;
}

void QSymbianHostResolver::processNameResult()
{
    if (iStatus.Int() == KErrNone) {
        TInetAddr hostAdd = iNameResult().iAddr;

        // Ensure that record is valid (not an alias and with length greater than 0)
        if (!(iNameResult().iFlags & TNameRecord::EAlias) && !(hostAdd.IsUnspecified())) {
            iHostAddresses.append(qt_QHostAddressFromTInetAddr(hostAdd));
        }

        iState = EGetByName;
        iHostResolver.Next(iNameResult, iStatus);
        SetActive();
    }
    else {
        // No more addresses, so return the results (or an error if there aren't any).
#if defined(QHOSTINFO_DEBUG)
        qDebug() << "QSymbianHostResolver::processNameResult with err=" << iStatus.Int() << "count=" << iHostAddresses.count();
#endif
        if (iHostAddresses.count() > 0) {
            iResults.setAddresses(iHostAddresses);
        } else {
            iState = EError;
            setError_helper(iResults, iStatus.Int());
        }
        returnResults();
    }
}

void QSymbianHostResolver::processAddressResult()
{
    TInt err = iStatus.Int();

    if (err < 0) {
        //For behavioural compatibility with Qt 4.7, don't report errors on reverse lookup,
        //return the address as a string (same as unix/windows backends)
        iResults.setHostName(iAddress.toString());
    } else {
        iResults.setHostName(qt_TDesC2QString(iNameResult().iName));
    }
    iResults.setAddresses(QList<QHostAddress>() << iAddress);
    returnResults();
}


int QSymbianHostResolver::id()
{
    return iResults.lookupId();
}

QSymbianHostInfoLookupManager::QSymbianHostInfoLookupManager()
{
}

QSymbianHostInfoLookupManager::~QSymbianHostInfoLookupManager()
{
}

void QSymbianHostInfoLookupManager::clear()
{
    QMutexLocker locker(&mutex);
#if defined(QHOSTINFO_DEBUG)
    qDebug() << "QSymbianHostInfoLookupManager::clear" << QThread::currentThreadId();
#endif
    foreach (QSymbianHostResolver *hr, iCurrentLookups)
        hr->abortHostLookup();
    iCurrentLookups.clear();
    qDeleteAll(iScheduledLookups);
    cache.clear();
}

void QSymbianHostInfoLookupManager::lookupFinished(QSymbianHostResolver *r)
{
    QMutexLocker locker(&mutex);

#if defined(QHOSTINFO_DEBUG)
    qDebug() << "QSymbianHostInfoLookupManager::lookupFinished" << QThread::currentThreadId() << r->id() << "current" << iCurrentLookups.count() << "queued" << iScheduledLookups.count();
#endif
    // remove finished lookup from array and destroy
    TInt count = iCurrentLookups.count();
    for (TInt i = 0; i < count; i++) {
        if (iCurrentLookups[i]->id() == r->id()) {
            iCurrentLookups.removeAt(i);
            break;
        }
    }

    runNextLookup();
}

void QSymbianHostInfoLookupManager::runNextLookup()
{
#if defined(QHOSTINFO_DEBUG)
    qDebug() << "QSymbianHostInfoLookupManager::runNextLookup" << QThread::currentThreadId() << "current" << iCurrentLookups.count() << "queued" << iScheduledLookups.count();
#endif
    // check to see if there are any scheduled lookups
    for (int i=0; i<iScheduledLookups.count(); i++) {
        QSymbianHostResolver* hostResolver = iScheduledLookups.at(i);
        if (hostResolver->resultEmitter.thread() == QThread::currentThread()) {
            // if so, move one to the current lookups and run it
            iCurrentLookups.append(hostResolver);
            iScheduledLookups.removeAt(i);
            hostResolver->requestHostLookup();
            // if spare capacity, try to start another one
            if (iCurrentLookups.count() >= KMaxConcurrentLookups)
                break;
            i--; //compensate for removeAt
        }
    }
}

// called from QHostInfo
void QSymbianHostInfoLookupManager::scheduleLookup(QSymbianHostResolver* r)
{
    QMutexLocker locker(&mutex);

#if defined(QHOSTINFO_DEBUG)
    qDebug() << "QSymbianHostInfoLookupManager::scheduleLookup" << QThread::currentThreadId() << r->id() << "current" << iCurrentLookups.count() << "queued" << iScheduledLookups.count();
#endif
    // Check to see if we have space on the current lookups pool.
    bool defer = false;
    if (iCurrentLookups.count() >= KMaxConcurrentLookups) {
        // busy, defer unless there are no request in this thread
        // at least one active request per thread with queued requests is needed
        for (int i=0; i < iCurrentLookups.count();i++) {
            if (iCurrentLookups.at(i)->resultEmitter.thread() == QThread::currentThread()) {
                defer = true;
                break;
            }
        }
    }
    if (defer) {
        // If no, schedule for later.
        iScheduledLookups.append(r);
#if defined(QHOSTINFO_DEBUG)
    qDebug(" - scheduled");
#endif
        return;
    } else {
        // If yes, add it to the current lookups.
        iCurrentLookups.append(r);

        // ... and trigger the async call.
        r->requestHostLookup();
    }
}

void QSymbianHostInfoLookupManager::abortLookup(int id)
{
    QMutexLocker locker(&mutex);

#if defined(QHOSTINFO_DEBUG)
    qDebug() << "QSymbianHostInfoLookupManager::abortLookup" << QThread::currentThreadId() << id << "current" << iCurrentLookups.count() << "queued" << iScheduledLookups.count();
#endif
    int i = 0;
    // Find the aborted lookup by ID.
    // First in the current lookups.
    for (i = 0; i < iCurrentLookups.count(); i++) {
        if (id == iCurrentLookups[i]->id()) {
            QSymbianHostResolver* r = iCurrentLookups.at(i);
            iCurrentLookups.removeAt(i);
            r->abortHostLookup();
            runNextLookup();
            return;
        }
    }
    // Then in the scheduled lookups.
    for (i = 0; i < iScheduledLookups.count(); i++) {
        if (id == iScheduledLookups[i]->id()) {
            QSymbianHostResolver* r = iScheduledLookups.at(i);
            iScheduledLookups.removeAt(i);
            delete r;
            return;
        }
    }
}

QSymbianHostInfoLookupManager* QSymbianHostInfoLookupManager::globalInstance()
{
    return static_cast<QSymbianHostInfoLookupManager*>
            (QAbstractHostInfoLookupManager::globalInstance());
}

QT_END_NAMESPACE
