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

#include "qnetworkaccesscache_p.h"
#include "QtCore/qpointer.h"
#include "QtCore/qdatetime.h"
#include "QtCore/qqueue.h"
#include "qnetworkaccessmanager_p.h"
#include "qnetworkreply_p.h"
#include "qnetworkrequest.h"

QT_BEGIN_NAMESPACE

enum ExpiryTimeEnum {
    ExpiryTime = 120
};

namespace {
    struct Receiver
    {
        QPointer<QObject> object;
        const char *member;
    };
}

// idea copied from qcache.h
struct QNetworkAccessCache::Node
{
    QDateTime timestamp;
    QQueue<Receiver> receiverQueue;
    QByteArray key;

    Node *older, *newer;
    CacheableObject *object;

    int useCount;

    Node()
        : older(0), newer(0), object(0), useCount(0)
    { }
};

QNetworkAccessCache::CacheableObject::CacheableObject()
{
    // leave the members uninitialized
    // they must be initialized by the derived class's constructor
}

QNetworkAccessCache::CacheableObject::~CacheableObject()
{
#if 0 //def QT_DEBUG
    if (!key.isEmpty() && Ptr()->hasEntry(key))
        qWarning() << "QNetworkAccessCache: object" << (void*)this << "key" << key
                   << "destroyed without being removed from cache first!";
#endif
}

void QNetworkAccessCache::CacheableObject::setExpires(bool enable)
{
    expires = enable;
}

void QNetworkAccessCache::CacheableObject::setShareable(bool enable)
{
    shareable = enable;
}

QNetworkAccessCache::QNetworkAccessCache()
    : oldest(0), newest(0)
{
}

QNetworkAccessCache::~QNetworkAccessCache()
{
    clear();
}

void QNetworkAccessCache::clear()
{
    NodeHash hashCopy = hash;
    hash.clear();

    // remove all entries
    NodeHash::Iterator it = hashCopy.begin();
    NodeHash::Iterator end = hashCopy.end();
    for ( ; it != end; ++it) {
        it->object->key.clear();
        it->object->dispose();
    }

    // now delete:
    hashCopy.clear();

    timer.stop();

    oldest = newest = 0;
}

/*!
    Appends the entry given by \a key to the end of the linked list.
    (i.e., makes it the newest entry)
 */
void QNetworkAccessCache::linkEntry(const QByteArray &key)
{
    NodeHash::Iterator it = hash.find(key);
    if (it == hash.end())
        return;

    Node *const node = &it.value();
    Q_ASSERT(node != oldest && node != newest);
    Q_ASSERT(node->older == 0 && node->newer == 0);
    Q_ASSERT(node->useCount == 0);

    if (newest) {
        Q_ASSERT(newest->newer == 0);
        newest->newer = node;
        node->older = newest;
    }
    if (!oldest) {
        // there are no entries, so this is the oldest one too
        oldest = node;
    }

    node->timestamp = QDateTime::currentDateTime().addSecs(ExpiryTime);
    newest = node;
}

/*!
    Removes the entry pointed by \a key from the linked list.
    Returns \c true if the entry removed was the oldest one.
 */
bool QNetworkAccessCache::unlinkEntry(const QByteArray &key)
{
    NodeHash::Iterator it = hash.find(key);
    if (it == hash.end())
        return false;

    Node *const node = &it.value();

    bool wasOldest = false;
    if (node == oldest) {
        oldest = node->newer;
        wasOldest = true;
    }
    if (node == newest)
        newest = node->older;
    if (node->older)
        node->older->newer = node->newer;
    if (node->newer)
        node->newer->older = node->older;

    node->newer = node->older = 0;
    return wasOldest;
}

void QNetworkAccessCache::updateTimer()
{
    timer.stop();

    if (!oldest)
        return;

    int interval = QDateTime::currentDateTime().secsTo(oldest->timestamp);
    if (interval <= 0) {
        interval = 0;
    } else {
        // round up the interval
        interval = (interval + 15) & ~16;
    }

    timer.start(interval * 1000, this);
}

bool QNetworkAccessCache::emitEntryReady(Node *node, QObject *target, const char *member)
{
    if (!connect(this, SIGNAL(entryReady(QNetworkAccessCache::CacheableObject*)),
                 target, member, Qt::QueuedConnection))
        return false;

    emit entryReady(node->object);
    disconnect(SIGNAL(entryReady(QNetworkAccessCache::CacheableObject*)));

    return true;
}

void QNetworkAccessCache::timerEvent(QTimerEvent *)
{
    // expire old items
    QDateTime now = QDateTime::currentDateTime();

    while (oldest && oldest->timestamp < now) {
        Node *next = oldest->newer;
        oldest->object->dispose();

        hash.remove(oldest->key); // oldest gets deleted
        oldest = next;
    }

    // fixup the list
    if (oldest)
        oldest->older = 0;
    else
        newest = 0;

    updateTimer();
}

void QNetworkAccessCache::addEntry(const QByteArray &key, CacheableObject *entry)
{
    Q_ASSERT(!key.isEmpty());

    if (unlinkEntry(key))
        updateTimer();

    Node &node = hash[key];     // create the entry in the hash if it didn't exist
    if (node.useCount)
        qWarning("QNetworkAccessCache::addEntry: overriding active cache entry '%s'",
                 key.constData());
    if (node.object)
        node.object->dispose();
    node.object = entry;
    node.object->key = key;
    node.key = key;
    node.useCount = 1;
}

bool QNetworkAccessCache::hasEntry(const QByteArray &key) const
{
    return hash.contains(key);
}

bool QNetworkAccessCache::requestEntry(const QByteArray &key, QObject *target, const char *member)
{
    NodeHash::Iterator it = hash.find(key);
    if (it == hash.end())
        return false;           // no such entry

    Node *node = &it.value();

    if (node->useCount > 0 && !node->object->shareable) {
        // object is not shareable and is in use
        // queue for later use
        Q_ASSERT(node->older == 0 && node->newer == 0);
        Receiver receiver;
        receiver.object = target;
        receiver.member = member;
        node->receiverQueue.enqueue(receiver);

        // request queued
        return true;
    } else {
        // node not in use or is shareable
        if (unlinkEntry(key))
            updateTimer();

        ++node->useCount;
        return emitEntryReady(node, target, member);
    }
}

QNetworkAccessCache::CacheableObject *QNetworkAccessCache::requestEntryNow(const QByteArray &key)
{
    NodeHash::Iterator it = hash.find(key);
    if (it == hash.end())
        return 0;
    if (it->useCount > 0) {
        if (it->object->shareable) {
            ++it->useCount;
            return it->object;
        }

        // object in use and not shareable
        return 0;
    }

    // entry not in use, let the caller have it
    bool wasOldest = unlinkEntry(key);
    ++it->useCount;

    if (wasOldest)
        updateTimer();
    return it->object;
}

void QNetworkAccessCache::releaseEntry(const QByteArray &key)
{
    NodeHash::Iterator it = hash.find(key);
    if (it == hash.end()) {
        qWarning("QNetworkAccessCache::releaseEntry: trying to release key '%s' that is not in cache",
                 key.constData());
        return;
    }

    Node *node = &it.value();
    Q_ASSERT(node->useCount > 0);

    // are there other objects waiting?
    if (!node->receiverQueue.isEmpty()) {
        // queue another activation
        Receiver receiver;
        do {
            receiver = node->receiverQueue.dequeue();
        } while (receiver.object.isNull() && !node->receiverQueue.isEmpty());

        if (!receiver.object.isNull()) {
            emitEntryReady(node, receiver.object, receiver.member);
            return;
        }
    }

    if (!--node->useCount) {
        // no objects waiting; add it back to the expiry list
        if (node->object->expires)
            linkEntry(key);

        if (oldest == node)
            updateTimer();
    }
}

void QNetworkAccessCache::removeEntry(const QByteArray &key)
{
    NodeHash::Iterator it = hash.find(key);
    if (it == hash.end()) {
        qWarning("QNetworkAccessCache::removeEntry: trying to remove key '%s' that is not in cache",
                 key.constData());
        return;
    }

    Node *node = &it.value();
    if (unlinkEntry(key))
        updateTimer();
    if (node->useCount > 1)
        qWarning("QNetworkAccessCache::removeEntry: removing active cache entry '%s'",
                 key.constData());

    node->object->key.clear();
    hash.remove(node->key);
}

QT_END_NAMESPACE
