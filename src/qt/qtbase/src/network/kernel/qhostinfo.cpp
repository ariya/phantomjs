/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qhostinfo.h"
#include "qhostinfo_p.h"

#include "QtCore/qscopedpointer.h"
#include <qabstracteventdispatcher.h>
#include <qcoreapplication.h>
#include <qmetaobject.h>
#include <qstringlist.h>
#include <qthread.h>
#include <qurl.h>
#include <private/qnetworksession_p.h>

#ifdef Q_OS_UNIX
#  include <unistd.h>
#endif

QT_BEGIN_NAMESPACE

//#define QHOSTINFO_DEBUG

Q_GLOBAL_STATIC(QHostInfoLookupManager, theHostInfoLookupManager)

/*!
    \class QHostInfo
    \brief The QHostInfo class provides static functions for host name lookups.

    \reentrant
    \inmodule QtNetwork
    \ingroup network

    QHostInfo uses the lookup mechanisms provided by the operating
    system to find the IP address(es) associated with a host name,
    or the host name associated with an IP address.
    The class provides two static convenience functions: one that
    works asynchronously and emits a signal once the host is found,
    and one that blocks and returns a QHostInfo object.

    To look up a host's IP addresses asynchronously, call lookupHost(),
    which takes the host name or IP address, a receiver object, and a slot
    signature as arguments and returns an ID. You can abort the
    lookup by calling abortHostLookup() with the lookup ID.

    Example:

    \snippet code/src_network_kernel_qhostinfo.cpp 0


    The slot is invoked when the results are ready. The results are
    stored in a QHostInfo object. Call
    addresses() to get the list of IP addresses for the host, and
    hostName() to get the host name that was looked up.

    If the lookup failed, error() returns the type of error that
    occurred. errorString() gives a human-readable description of the
    lookup error.

    If you want a blocking lookup, use the QHostInfo::fromName() function:

    \snippet code/src_network_kernel_qhostinfo.cpp 1

    QHostInfo supports Internationalized Domain Names (IDNs) through the
    IDNA and Punycode standards.

    To retrieve the name of the local host, use the static
    QHostInfo::localHostName() function.

    \note Since Qt 4.6.1 QHostInfo is using multiple threads for DNS lookup
    instead of one dedicated DNS thread. This improves performance,
    but also changes the order of signal emissions when using lookupHost()
    compared to previous versions of Qt.
    \note Since Qt 4.6.3 QHostInfo is using a small internal 60 second DNS cache
    for performance improvements.

    \sa QAbstractSocket, {http://www.rfc-editor.org/rfc/rfc3492.txt}{RFC 3492}
*/

static QBasicAtomicInt theIdCounter = Q_BASIC_ATOMIC_INITIALIZER(1);

/*!
    Looks up the IP address(es) associated with host name \a name, and
    returns an ID for the lookup. When the result of the lookup is
    ready, the slot or signal \a member in \a receiver is called with
    a QHostInfo argument. The QHostInfo object can then be inspected
    to get the results of the lookup.

    The lookup is performed by a single function call, for example:

    \snippet code/src_network_kernel_qhostinfo.cpp 2

    The implementation of the slot prints basic information about the
    addresses returned by the lookup, or reports an error if it failed:

    \snippet code/src_network_kernel_qhostinfo.cpp 3

    If you pass a literal IP address to \a name instead of a host name,
    QHostInfo will search for the domain name for the IP (i.e., QHostInfo will
    perform a \e reverse lookup). On success, the resulting QHostInfo will
    contain both the resolved domain name and IP addresses for the host
    name. Example:

    \snippet code/src_network_kernel_qhostinfo.cpp 4

    \note There is no guarantee on the order the signals will be emitted
    if you start multiple requests with lookupHost().

    \sa abortHostLookup(), addresses(), error(), fromName()
*/
int QHostInfo::lookupHost(const QString &name, QObject *receiver,
                          const char *member)
{
#if defined QHOSTINFO_DEBUG
    qDebug("QHostInfo::lookupHost(\"%s\", %p, %s)",
           name.toLatin1().constData(), receiver, member ? member + 1 : 0);
#endif

    if (!QAbstractEventDispatcher::instance(QThread::currentThread())) {
        qWarning("QHostInfo::lookupHost() called with no event dispatcher");
        return -1;
    }

    qRegisterMetaType<QHostInfo>();

    int id = theIdCounter.fetchAndAddRelaxed(1); // generate unique ID

    if (name.isEmpty()) {
        if (!receiver)
            return -1;

        QHostInfo hostInfo(id);
        hostInfo.setError(QHostInfo::HostNotFound);
        hostInfo.setErrorString(QCoreApplication::translate("QHostInfo", "No host name given"));
        QScopedPointer<QHostInfoResult> result(new QHostInfoResult);
        QObject::connect(result.data(), SIGNAL(resultsReady(QHostInfo)),
                         receiver, member, Qt::QueuedConnection);
        result.data()->emitResultsReady(hostInfo);
        return id;
    }

    QHostInfoLookupManager *manager = theHostInfoLookupManager();

    if (manager) {
        // the application is still alive
        if (manager->cache.isEnabled()) {
            // check cache first
            bool valid = false;
            QHostInfo info = manager->cache.get(name, &valid);
            if (valid) {
                if (!receiver)
                    return -1;

                info.setLookupId(id);
                QHostInfoResult result;
                QObject::connect(&result, SIGNAL(resultsReady(QHostInfo)), receiver, member, Qt::QueuedConnection);
                result.emitResultsReady(info);
                return id;
            }
        }

        // cache is not enabled or it was not in the cache, do normal lookup
        QHostInfoRunnable* runnable = new QHostInfoRunnable(name, id);
        if (receiver)
            QObject::connect(&runnable->resultEmitter, SIGNAL(resultsReady(QHostInfo)), receiver, member, Qt::QueuedConnection);
        manager->scheduleLookup(runnable);
    }
    return id;
}

/*!
    Aborts the host lookup with the ID \a id, as returned by lookupHost().

    \sa lookupHost(), lookupId()
*/
void QHostInfo::abortHostLookup(int id)
{
    theHostInfoLookupManager()->abortLookup(id);
}

/*!
    Looks up the IP address(es) for the given host \a name. The
    function blocks during the lookup which means that execution of
    the program is suspended until the results of the lookup are
    ready. Returns the result of the lookup in a QHostInfo object.

    If you pass a literal IP address to \a name instead of a host name,
    QHostInfo will search for the domain name for the IP (i.e., QHostInfo will
    perform a \e reverse lookup). On success, the returned QHostInfo will
    contain both the resolved domain name and IP addresses for the host name.

    \sa lookupHost()
*/
QHostInfo QHostInfo::fromName(const QString &name)
{
#if defined QHOSTINFO_DEBUG
    qDebug("QHostInfo::fromName(\"%s\")",name.toLatin1().constData());
#endif

    QHostInfo hostInfo = QHostInfoAgent::fromName(name);
    QAbstractHostInfoLookupManager* manager = theHostInfoLookupManager();
    manager->cache.put(name, hostInfo);
    return hostInfo;
}

#ifndef QT_NO_BEARERMANAGEMENT
QHostInfo QHostInfoPrivate::fromName(const QString &name, QSharedPointer<QNetworkSession> session)
{
#if defined QHOSTINFO_DEBUG
    qDebug("QHostInfoPrivate::fromName(\"%s\") with session %p",name.toLatin1().constData(), session.data());
#endif

    QHostInfo hostInfo = QHostInfoAgent::fromName(name, session);
    QAbstractHostInfoLookupManager* manager = theHostInfoLookupManager();
    manager->cache.put(name, hostInfo);
    return hostInfo;
}
#endif

#ifndef QT_NO_BEARERMANAGEMENT
QHostInfo QHostInfoAgent::fromName(const QString &hostName, QSharedPointer<QNetworkSession>)
{
    return QHostInfoAgent::fromName(hostName);
}
#endif


/*!
    \enum QHostInfo::HostInfoError

    This enum describes the various errors that can occur when trying
    to resolve a host name.

    \value NoError The lookup was successful.
    \value HostNotFound No IP addresses were found for the host.
    \value UnknownError An unknown error occurred.

    \sa error(), setError()
*/

/*!
    Constructs an empty host info object with lookup ID \a id.

    \sa lookupId()
*/
QHostInfo::QHostInfo(int id)
    : d(new QHostInfoPrivate)
{
    d->lookupId = id;
}

/*!
    Constructs a copy of \a other.
*/
QHostInfo::QHostInfo(const QHostInfo &other)
    : d(new QHostInfoPrivate(*other.d.data()))
{
}

/*!
    Assigns the data of the \a other object to this host info object,
    and returns a reference to it.
*/
QHostInfo &QHostInfo::operator=(const QHostInfo &other)
{
    *d.data() = *other.d.data();
    return *this;
}

/*!
    Destroys the host info object.
*/
QHostInfo::~QHostInfo()
{
}

/*!
    Returns the list of IP addresses associated with hostName(). This
    list may be empty.

    Example:

    \snippet code/src_network_kernel_qhostinfo.cpp 5

    \sa hostName(), error()
*/
QList<QHostAddress> QHostInfo::addresses() const
{
    return d->addrs;
}

/*!
    Sets the list of addresses in this QHostInfo to \a addresses.

    \sa addresses()
*/
void QHostInfo::setAddresses(const QList<QHostAddress> &addresses)
{
    d->addrs = addresses;
}

/*!
    Returns the name of the host whose IP addresses were looked up.

    \sa localHostName()
*/
QString QHostInfo::hostName() const
{
    return d->hostName;
}

/*!
    Sets the host name of this QHostInfo to \a hostName.

    \sa hostName()
*/
void QHostInfo::setHostName(const QString &hostName)
{
    d->hostName = hostName;
}

/*!
    Returns the type of error that occurred if the host name lookup
    failed; otherwise returns NoError.

    \sa setError(), errorString()
*/
QHostInfo::HostInfoError QHostInfo::error() const
{
    return d->err;
}

/*!
    Sets the error type of this QHostInfo to \a error.

    \sa error(), errorString()
*/
void QHostInfo::setError(HostInfoError error)
{
    d->err = error;
}

/*!
    Returns the ID of this lookup.

    \sa setLookupId(), abortHostLookup(), hostName()
*/
int QHostInfo::lookupId() const
{
    return d->lookupId;
}

/*!
    Sets the ID of this lookup to \a id.

    \sa lookupId(), lookupHost()
*/
void QHostInfo::setLookupId(int id)
{
    d->lookupId = id;
}

/*!
    If the lookup failed, this function returns a human readable
    description of the error; otherwise "Unknown error" is returned.

    \sa setErrorString(), error()
*/
QString QHostInfo::errorString() const
{
    return d->errorStr;
}

/*!
    Sets the human readable description of the error that occurred to \a str
    if the lookup failed.

    \sa errorString(), setError()
*/
void QHostInfo::setErrorString(const QString &str)
{
    d->errorStr = str;
}

/*!
    \fn QString QHostInfo::localHostName()

    Returns the host name of this machine.

    \sa hostName()
*/

/*!
    \fn QString QHostInfo::localDomainName()

    Returns the DNS domain of this machine.

    \note DNS domains are not related to domain names found in
    Windows networks.

    \sa hostName()
*/

QHostInfoRunnable::QHostInfoRunnable(QString hn, int i) : toBeLookedUp(hn), id(i)
{
    setAutoDelete(true);
}

// the QHostInfoLookupManager will at some point call this via a QThreadPool
void QHostInfoRunnable::run()
{
    QHostInfoLookupManager *manager = theHostInfoLookupManager();
    // check aborted
    if (manager->wasAborted(id)) {
        manager->lookupFinished(this);
        return;
    }

    QHostInfo hostInfo;

    // QHostInfo::lookupHost already checks the cache. However we need to check
    // it here too because it might have been cache saved by another QHostInfoRunnable
    // in the meanwhile while this QHostInfoRunnable was scheduled but not running
    if (manager->cache.isEnabled()) {
        // check the cache first
        bool valid = false;
        hostInfo = manager->cache.get(toBeLookedUp, &valid);
        if (!valid) {
            // not in cache, we need to do the lookup and store the result in the cache
            hostInfo = QHostInfoAgent::fromName(toBeLookedUp);
            manager->cache.put(toBeLookedUp, hostInfo);
        }
    } else {
        // cache is not enabled, just do the lookup and continue
        hostInfo = QHostInfoAgent::fromName(toBeLookedUp);
    }

    // check aborted again
    if (manager->wasAborted(id)) {
        manager->lookupFinished(this);
        return;
    }

    // signal emission
    hostInfo.setLookupId(id);
    resultEmitter.emitResultsReady(hostInfo);

    // now also iterate through the postponed ones
    {
        QMutexLocker locker(&manager->mutex);
        QMutableListIterator<QHostInfoRunnable*> iterator(manager->postponedLookups);
        while (iterator.hasNext()) {
            QHostInfoRunnable* postponed = iterator.next();
            if (toBeLookedUp == postponed->toBeLookedUp) {
                // we can now emit
                iterator.remove();
                hostInfo.setLookupId(postponed->id);
                postponed->resultEmitter.emitResultsReady(hostInfo);
                delete postponed;
            }
        }
    }

    manager->lookupFinished(this);

    // thread goes back to QThreadPool
}

QHostInfoLookupManager::QHostInfoLookupManager() : mutex(QMutex::Recursive), wasDeleted(false)
{
    moveToThread(QCoreApplicationPrivate::mainThread());
    connect(QCoreApplication::instance(), SIGNAL(destroyed()), SLOT(waitForThreadPoolDone()), Qt::DirectConnection);
    threadPool.setMaxThreadCount(20); // do up to 20 DNS lookups in parallel
}

QHostInfoLookupManager::~QHostInfoLookupManager()
{
    wasDeleted = true;

    // don't qDeleteAll currentLookups, the QThreadPool has ownership
    clear();
}

void QHostInfoLookupManager::clear()
{
    {
        QMutexLocker locker(&mutex);
        qDeleteAll(postponedLookups);
        qDeleteAll(scheduledLookups);
        qDeleteAll(finishedLookups);
        postponedLookups.clear();
        scheduledLookups.clear();
        finishedLookups.clear();
    }

    threadPool.waitForDone();
    cache.clear();
}

void QHostInfoLookupManager::work()
{
    if (wasDeleted)
        return;

    // goals of this function:
    //  - launch new lookups via the thread pool
    //  - make sure only one lookup per host/IP is in progress

    QMutexLocker locker(&mutex);

    if (!finishedLookups.isEmpty()) {
        // remove ID from aborted if it is in there
        for (int i = 0; i < finishedLookups.length(); i++) {
           abortedLookups.removeAll(finishedLookups.at(i)->id);
        }

        finishedLookups.clear();
    }

    if (!postponedLookups.isEmpty()) {
        // try to start the postponed ones

        QMutableListIterator<QHostInfoRunnable*> iterator(postponedLookups);
        while (iterator.hasNext()) {
            QHostInfoRunnable* postponed = iterator.next();

            // check if none of the postponed hostnames is currently running
            bool alreadyRunning = false;
            for (int i = 0; i < currentLookups.length(); i++) {
                if (currentLookups.at(i)->toBeLookedUp == postponed->toBeLookedUp) {
                    alreadyRunning = true;
                    break;
                }
            }
            if (!alreadyRunning) {
                iterator.remove();
                scheduledLookups.prepend(postponed); // prepend! we want to finish it ASAP
            }
        }
    }

    if (!scheduledLookups.isEmpty()) {
        // try to start the new ones
        QMutableListIterator<QHostInfoRunnable*> iterator(scheduledLookups);
        while (iterator.hasNext()) {
            QHostInfoRunnable *scheduled = iterator.next();

            // check if a lookup for this host is already running, then postpone
            for (int i = 0; i < currentLookups.size(); i++) {
                if (currentLookups.at(i)->toBeLookedUp == scheduled->toBeLookedUp) {
                    iterator.remove();
                    postponedLookups.append(scheduled);
                    scheduled = 0;
                    break;
                }
            }

            if (scheduled && currentLookups.size() < threadPool.maxThreadCount()) {
                // runnable now running in new thread, track this in currentLookups
                threadPool.start(scheduled);
                iterator.remove();
                currentLookups.append(scheduled);
            } else {
                // was postponed, continue iterating
                continue;
            }
        };
    }
}

// called by QHostInfo
void QHostInfoLookupManager::scheduleLookup(QHostInfoRunnable *r)
{
    if (wasDeleted)
        return;

    QMutexLocker locker(&this->mutex);
    scheduledLookups.enqueue(r);
    work();
}

// called by QHostInfo
void QHostInfoLookupManager::abortLookup(int id)
{
    if (wasDeleted)
        return;

    QMutexLocker locker(&this->mutex);

    // is postponed? delete and return
    for (int i = 0; i < postponedLookups.length(); i++) {
        if (postponedLookups.at(i)->id == id) {
            delete postponedLookups.takeAt(i);
            return;
        }
    }

    // is scheduled? delete and return
    for (int i = 0; i < scheduledLookups.length(); i++) {
        if (scheduledLookups.at(i)->id == id) {
            delete scheduledLookups.takeAt(i);
            return;
        }
    }

    if (!abortedLookups.contains(id))
        abortedLookups.append(id);
}

// called from QHostInfoRunnable
bool QHostInfoLookupManager::wasAborted(int id)
{
    if (wasDeleted)
        return true;

    QMutexLocker locker(&this->mutex);
    return abortedLookups.contains(id);
}

// called from QHostInfoRunnable
void QHostInfoLookupManager::lookupFinished(QHostInfoRunnable *r)
{
    if (wasDeleted)
        return;

    QMutexLocker locker(&this->mutex);
    currentLookups.removeOne(r);
    finishedLookups.append(r);
    work();
}

// This function returns immediately when we had a result in the cache, else it will later emit a signal
QHostInfo qt_qhostinfo_lookup(const QString &name, QObject *receiver, const char *member, bool *valid, int *id)
{
    *valid = false;
    *id = -1;

    // check cache
    QAbstractHostInfoLookupManager* manager = theHostInfoLookupManager();
    if (manager && manager->cache.isEnabled()) {
        QHostInfo info = manager->cache.get(name, valid);
        if (*valid) {
            return info;
        }
    }

    // was not in cache, trigger lookup
    *id = QHostInfo::lookupHost(name, receiver, member);

    // return empty response, valid==false
    return QHostInfo();
}

void qt_qhostinfo_clear_cache()
{
    QAbstractHostInfoLookupManager* manager = theHostInfoLookupManager();
    if (manager) {
        manager->clear();
    }
}

void Q_AUTOTEST_EXPORT qt_qhostinfo_enable_cache(bool e)
{
    QAbstractHostInfoLookupManager* manager = theHostInfoLookupManager();
    if (manager) {
        manager->cache.setEnabled(e);
    }
}

// cache for 60 seconds
// cache 128 items
QHostInfoCache::QHostInfoCache() : max_age(60), enabled(true), cache(128)
{
#ifdef QT_QHOSTINFO_CACHE_DISABLED_BY_DEFAULT
    enabled = false;
#endif
}

bool QHostInfoCache::isEnabled()
{
    return enabled;
}

// this function is currently only used for the auto tests
// and not usable by public API
void QHostInfoCache::setEnabled(bool e)
{
    enabled = e;
}


QHostInfo QHostInfoCache::get(const QString &name, bool *valid)
{
    QMutexLocker locker(&this->mutex);

    *valid = false;
    if (cache.contains(name)) {
        QHostInfoCacheElement *element = cache.object(name);
        if (element->age.elapsed() < max_age*1000)
            *valid = true;
        return element->info;

        // FIXME idea:
        // if too old but not expired, trigger a new lookup
        // to freshen our cache
    }

    return QHostInfo();
}

void QHostInfoCache::put(const QString &name, const QHostInfo &info)
{
    // if the lookup failed, don't cache
    if (info.error() != QHostInfo::NoError)
        return;

    QHostInfoCacheElement* element = new QHostInfoCacheElement();
    element->info = info;
    element->age = QElapsedTimer();
    element->age.start();

    QMutexLocker locker(&this->mutex);
    cache.insert(name, element); // cache will take ownership
}

void QHostInfoCache::clear()
{
    QMutexLocker locker(&this->mutex);
    cache.clear();
}

QAbstractHostInfoLookupManager* QAbstractHostInfoLookupManager::globalInstance()
{
    return theHostInfoLookupManager();
}

QT_END_NAMESPACE
