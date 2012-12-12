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

#include "qobject.h"
#include "qobject_p.h"
#include "qmetaobject_p.h"

#include "qabstracteventdispatcher.h"
#include "qcoreapplication.h"
#include "qcoreapplication_p.h"
#include "qvariant.h"
#include "qmetaobject.h"
#include <qregexp.h>
#include <qthread.h>
#include <private/qthread_p.h>
#include <qdebug.h>
#include <qhash.h>
#include <qpair.h>
#include <qset.h>
#include <qsemaphore.h>
#include <qsharedpointer.h>

#include <private/qorderedmutexlocker_p.h>
#include <private/qmutexpool_p.h>

#include <new>

#include <ctype.h>
#include <limits.h>

QT_BEGIN_NAMESPACE

static int DIRECT_CONNECTION_ONLY = 0;

static int *queuedConnectionTypes(const QList<QByteArray> &typeNames)
{
    int *types = new int [typeNames.count() + 1];
    Q_CHECK_PTR(types);
    for (int i = 0; i < typeNames.count(); ++i) {
        const QByteArray typeName = typeNames.at(i);
        if (typeName.endsWith('*'))
            types[i] = QMetaType::VoidStar;
        else
            types[i] = QMetaType::type(typeName);

        if (!types[i]) {
            qWarning("QObject::connect: Cannot queue arguments of type '%s'\n"
                     "(Make sure '%s' is registered using qRegisterMetaType().)",
                     typeName.constData(), typeName.constData());
            delete [] types;
            return 0;
        }
    }
    types[typeNames.count()] = 0;

    return types;
}

static QBasicAtomicPointer<QMutexPool> signalSlotMutexes = Q_BASIC_ATOMIC_INITIALIZER(0);
static QBasicAtomicInt objectCount = Q_BASIC_ATOMIC_INITIALIZER(0);

/** \internal
 * mutex to be locked when accessing the connectionlists or the senders list
 */
static inline QMutex *signalSlotLock(const QObject *o)
{
    if (!signalSlotMutexes) {
        QMutexPool *mp = new QMutexPool;
        if (!signalSlotMutexes.testAndSetOrdered(0, mp)) {
            delete mp;
        }
    }
    return signalSlotMutexes->get(o);
}

extern "C" Q_CORE_EXPORT void qt_addObject(QObject *)
{
    objectCount.ref();
}

extern "C" Q_CORE_EXPORT void qt_removeObject(QObject *)
{
    if(!objectCount.deref()) {
        QMutexPool *old = signalSlotMutexes.fetchAndStoreAcquire(0);
        delete old;
    }
}

void (*QAbstractDeclarativeData::destroyed)(QAbstractDeclarativeData *, QObject *) = 0;
void (*QAbstractDeclarativeData::parentChanged)(QAbstractDeclarativeData *, QObject *, QObject *) = 0;
void (*QAbstractDeclarativeData::objectNameChanged)(QAbstractDeclarativeData *, QObject *) = 0;

QObjectData::~QObjectData() {}

QObjectPrivate::QObjectPrivate(int version)
    : threadData(0), connectionLists(0), senders(0), currentSender(0), currentChildBeingDeleted(0)
{
    if (version != QObjectPrivateVersion)
        qFatal("Cannot mix incompatible Qt library (version 0x%x) with this library (version 0x%x)",
                version, QObjectPrivateVersion);

    // QObjectData initialization
    q_ptr = 0;
    parent = 0;                                 // no parent yet. It is set by setParent()
    isWidget = false;                           // assume not a widget object
    pendTimer = false;                          // no timers yet
    blockSig = false;                           // not blocking signals
    wasDeleted = false;                         // double-delete catcher
    sendChildEvents = true;                     // if we should send ChildInsert and ChildRemove events to parent
    receiveChildEvents = true;
    postedEvents = 0;
    extraData = 0;
    connectedSignals[0] = connectedSignals[1] = 0;
    inThreadChangeEvent = false;
#ifdef QT_JAMBI_BUILD
    inEventHandler = false;
    deleteWatch = 0;
#endif
    metaObject = 0;
    hasGuards = false;
}

QObjectPrivate::~QObjectPrivate()
{
    if (pendTimer) {
        // unregister pending timers
        if (threadData && threadData->eventDispatcher)
            threadData->eventDispatcher->unregisterTimers(q_ptr);
    }

    if (postedEvents)
        QCoreApplication::removePostedEvents(q_ptr, 0);

    if (threadData)
        threadData->deref();

    delete static_cast<QAbstractDynamicMetaObject*>(metaObject);
#ifdef QT_JAMBI_BUILD
    if (deleteWatch)
        *deleteWatch = 1;
#endif
#ifndef QT_NO_USERDATA
    if (extraData)
        qDeleteAll(extraData->userData);
    delete extraData;
#endif
}


#ifdef QT_JAMBI_BUILD
int *QObjectPrivate::setDeleteWatch(QObjectPrivate *d, int *w) {
    int *old = d->deleteWatch;
    d->deleteWatch = w;
    return old;
}


void QObjectPrivate::resetDeleteWatch(QObjectPrivate *d, int *oldWatch, int deleteWatch) {
    if (!deleteWatch)
        d->deleteWatch = oldWatch;

    if (oldWatch)
        *oldWatch = deleteWatch;
}
#endif

#ifdef QT3_SUPPORT
void QObjectPrivate::sendPendingChildInsertedEvents()
{
    Q_Q(QObject);
    for (int i = 0; i < pendingChildInsertedEvents.size(); ++i) {
        QObject *c = pendingChildInsertedEvents.at(i).data();
        if (!c || c->parent() != q)
            continue;
        QChildEvent childEvent(QEvent::ChildInserted, c);
        QCoreApplication::sendEvent(q, &childEvent);
    }
    pendingChildInsertedEvents.clear();
}

#endif


/*!\internal
  For a given metaobject, compute the signal offset, and the method offset (including signals)
*/
static void computeOffsets(const QMetaObject *metaobject, int *signalOffset, int *methodOffset)
{
    *signalOffset = *methodOffset = 0;
    const QMetaObject *m = metaobject->d.superdata;
    while (m) {
        const QMetaObjectPrivate *d = QMetaObjectPrivate::get(m);
        *methodOffset += d->methodCount;
        *signalOffset += (d->revision >= 4) ? d->signalCount : d->methodCount;
        /*Before Qt 4.6 (revision 4), the signalCount information was not generated by moc.
           so for compatibility we consider all the method as slot for old moc output*/
        m = m->d.superdata;
    }
}

/*
    This vector contains the all connections from an object.

    Each object may have one vector containing the lists of
    connections for a given signal. The index in the vector correspond
    to the signal index. The signal index is the one returned by
    QObjectPrivate::signalIndex (not QMetaObject::indexOfSignal).
    Negative index means connections to all signals.

    This vector is protected by the object mutex (signalSlotMutexes())

    Each Connection is also part of a 'senders' linked list. The mutex
    of the receiver must be locked when touching the pointers of this
    linked list.
*/
class QObjectConnectionListVector : public QVector<QObjectPrivate::ConnectionList>
{
public:
    bool orphaned; //the QObject owner of this vector has been destroyed while the vector was inUse
    bool dirty; //some Connection have been disconnected (their receiver is 0) but not removed from the list yet
    int inUse; //number of functions that are currently accessing this object or its connections
    QObjectPrivate::ConnectionList allsignals;

    QObjectConnectionListVector()
        : QVector<QObjectPrivate::ConnectionList>(), orphaned(false), dirty(false), inUse(0)
    { }

    QObjectPrivate::ConnectionList &operator[](int at)
    {
        if (at < 0)
            return allsignals;
        return QVector<QObjectPrivate::ConnectionList>::operator[](at);
    }
};

// Used by QAccessibleWidget
bool QObjectPrivate::isSender(const QObject *receiver, const char *signal) const
{
    Q_Q(const QObject);
    int signal_index = signalIndex(signal);
    if (signal_index < 0)
        return false;
    QMutexLocker locker(signalSlotLock(q));
    if (connectionLists) {
        if (signal_index < connectionLists->count()) {
            const QObjectPrivate::Connection *c =
                connectionLists->at(signal_index).first;

            while (c) {
                if (c->receiver == receiver)
                    return true;
                c = c->nextConnectionList;
            }
        }
    }
    return false;
}

// Used by QAccessibleWidget
QObjectList QObjectPrivate::receiverList(const char *signal) const
{
    Q_Q(const QObject);
    QObjectList returnValue;
    int signal_index = signalIndex(signal);
    if (signal_index < 0)
        return returnValue;
    QMutexLocker locker(signalSlotLock(q));
    if (connectionLists) {
        if (signal_index < connectionLists->count()) {
            const QObjectPrivate::Connection *c = connectionLists->at(signal_index).first;

            while (c) {
                if (c->receiver)
                    returnValue << c->receiver;
                c = c->nextConnectionList;
            }
        }
    }
    return returnValue;
}

// Used by QAccessibleWidget
QObjectList QObjectPrivate::senderList() const
{
    QObjectList returnValue;
    QMutexLocker locker(signalSlotLock(q_func()));
    for (Connection *c = senders; c; c = c->next)
        returnValue << c->sender;
    return returnValue;
}

void QObjectPrivate::addConnection(int signal, Connection *c)
{
    if (!connectionLists)
        connectionLists = new QObjectConnectionListVector();
    if (signal >= connectionLists->count())
        connectionLists->resize(signal + 1);

    ConnectionList &connectionList = (*connectionLists)[signal];
    if (connectionList.last) {
        connectionList.last->nextConnectionList = c;
    } else {
        connectionList.first = c;
    }
    connectionList.last = c;

    cleanConnectionLists();
}

void QObjectPrivate::cleanConnectionLists()
{
    if (connectionLists->dirty && !connectionLists->inUse) {
        // remove broken connections
        for (int signal = -1; signal < connectionLists->count(); ++signal) {
            QObjectPrivate::ConnectionList &connectionList =
                (*connectionLists)[signal];

            // Set to the last entry in the connection list that was *not*
            // deleted.  This is needed to update the list's last pointer
            // at the end of the cleanup.
            QObjectPrivate::Connection *last = 0;

            QObjectPrivate::Connection **prev = &connectionList.first;
            QObjectPrivate::Connection *c = *prev;
            while (c) {
                if (c->receiver) {
                    last = c;
                    prev = &c->nextConnectionList;
                    c = *prev;
                } else {
                    QObjectPrivate::Connection *next = c->nextConnectionList;
                    *prev = next;
                    delete c;
                    c = next;
                }
            }

            // Correct the connection list's last pointer.
            // As conectionList.last could equal last, this could be a noop
            connectionList.last = last;
        }
        connectionLists->dirty = false;
    }
}

typedef QMultiHash<QObject *, QObject **> GuardHash;
Q_GLOBAL_STATIC(GuardHash, guardHash)
Q_GLOBAL_STATIC(QMutex, guardHashLock)

/*!\internal
 */
void QMetaObject::addGuard(QObject **ptr)
{
    if (!*ptr)
        return;
    GuardHash *hash = guardHash();
    if (!hash) {
        *ptr = 0;
        return;
    }
    QMutexLocker locker(guardHashLock());
    QObjectPrivate::get(*ptr)->hasGuards = true;
    hash->insert(*ptr, ptr);
}

/*!\internal
 */
void QMetaObject::removeGuard(QObject **ptr)
{
    if (!*ptr)
        return;
    GuardHash *hash = guardHash();
    /* check that the hash is empty - otherwise we might detach
       the shared_null hash, which will alloc, which is not nice */
    if (!hash || hash->isEmpty())
        return;
    QMutexLocker locker(guardHashLock());
    if (!*ptr) //check again, under the lock
        return;
    GuardHash::iterator it = hash->find(*ptr);
    const GuardHash::iterator end = hash->end();
    bool more = false; //if the QObject has more pointer attached to it.
    for (; it.key() == *ptr && it != end; ++it) {
        if (it.value() == ptr) {
            it = hash->erase(it);
            if (!more) more = (it != end && it.key() == *ptr);
            break;
        }
        more = true;
    }
    if (!more)
        QObjectPrivate::get(*ptr)->hasGuards = false;
}

/*!\internal
 */
void QMetaObject::changeGuard(QObject **ptr, QObject *o)
{
    GuardHash *hash = guardHash();
    if (!hash) {
        *ptr = 0;
        return;
    }
    QMutexLocker locker(guardHashLock());
    if (o) {
        hash->insert(o, ptr);
        QObjectPrivate::get(o)->hasGuards = true;
    }
    if (*ptr) {
        bool more = false; //if the QObject has more pointer attached to it.
        GuardHash::iterator it = hash->find(*ptr);
        const GuardHash::iterator end = hash->end();
        for (; it.key() == *ptr && it != end; ++it) {
            if (it.value() == ptr) {
                it = hash->erase(it);
                if (!more) more = (it != end && it.key() == *ptr);
                break;
            }
            more = true;
        }
        if (!more)
            QObjectPrivate::get(*ptr)->hasGuards = false;
    }
    *ptr = o;
}

/*! \internal
 */
void QObjectPrivate::clearGuards(QObject *object)
{
    GuardHash *hash = 0;
    QMutex *mutex = 0;
    QT_TRY {
        hash = guardHash();
        mutex = guardHashLock();
    } QT_CATCH(const std::bad_alloc &) {
        // do nothing in case of OOM - code below is safe
    }

    /* check that the hash is empty - otherwise we might detach
       the shared_null hash, which will alloc, which is not nice */
    if (hash && !hash->isEmpty()) {
        QMutexLocker locker(mutex);
        GuardHash::iterator it = hash->find(object);
        const GuardHash::iterator end = hash->end();
        while (it.key() == object && it != end) {
            *it.value() = 0;
            it = hash->erase(it);
        }
    }
}

/*! \internal
 */
QMetaCallEvent::QMetaCallEvent(ushort method_offset, ushort method_relative, QObjectPrivate::StaticMetaCallFunction callFunction,
                               const QObject *sender, int signalId,
                               int nargs, int *types, void **args, QSemaphore *semaphore)
    : QEvent(MetaCall), sender_(sender), signalId_(signalId),
      nargs_(nargs), types_(types), args_(args), semaphore_(semaphore),
      callFunction_(callFunction), method_offset_(method_offset), method_relative_(method_relative)
{ }

/*! \internal
 */
QMetaCallEvent::~QMetaCallEvent()
{
    if (types_) {
        for (int i = 0; i < nargs_; ++i) {
            if (types_[i] && args_[i])
                QMetaType::destroy(types_[i], args_[i]);
        }
        qFree(types_);
        qFree(args_);
    }
#ifndef QT_NO_THREAD
    if (semaphore_)
        semaphore_->release();
#endif
}

/*! \internal
 */
void QMetaCallEvent::placeMetaCall(QObject *object)
{
    if (callFunction_) {
        callFunction_(object, QMetaObject::InvokeMetaMethod, method_relative_, args_);
    } else {
        QMetaObject::metacall(object, QMetaObject::InvokeMetaMethod, method_offset_ + method_relative_, args_);
    }
}

/*!
    \class QObject
    \brief The QObject class is the base class of all Qt objects.

    \ingroup objectmodel

    \reentrant

    QObject is the heart of the Qt \l{Object Model}. The central
    feature in this model is a very powerful mechanism for seamless
    object communication called \l{signals and slots}. You can
    connect a signal to a slot with connect() and destroy the
    connection with disconnect(). To avoid never ending notification
    loops you can temporarily block signals with blockSignals(). The
    protected functions connectNotify() and disconnectNotify() make
    it possible to track connections.

    QObjects organize themselves in \l {Object Trees & Ownership}
    {object trees}. When you create a QObject with another object as
    parent, the object will automatically add itself to the parent's
    children() list. The parent takes ownership of the object; i.e.,
    it will automatically delete its children in its destructor. You
    can look for an object by name and optionally type using
    findChild() or findChildren().

    Every object has an objectName() and its class name can be found
    via the corresponding metaObject() (see QMetaObject::className()).
    You can determine whether the object's class inherits another
    class in the QObject inheritance hierarchy by using the
    inherits() function.

    When an object is deleted, it emits a destroyed() signal. You can
    catch this signal to avoid dangling references to QObjects.

    QObjects can receive events through event() and filter the events
    of other objects. See installEventFilter() and eventFilter() for
    details. A convenience handler, childEvent(), can be reimplemented
    to catch child events.

    Events are delivered in the thread in which the object was
    created; see \l{Thread Support in Qt} and thread() for details.
    Note that event processing is not done at all for QObjects with no
    thread affinity (thread() returns zero). Use the moveToThread()
    function to change the thread affinity for an object and its
    children (the object cannot be moved if it has a parent).

    Last but not least, QObject provides the basic timer support in
    Qt; see QTimer for high-level support for timers.

    Notice that the Q_OBJECT macro is mandatory for any object that
    implements signals, slots or properties. You also need to run the
    \l{moc}{Meta Object Compiler} on the source file. We strongly
    recommend the use of this macro in all subclasses of QObject
    regardless of whether or not they actually use signals, slots and
    properties, since failure to do so may lead certain functions to
    exhibit strange behavior.

    All Qt widgets inherit QObject. The convenience function
    isWidgetType() returns whether an object is actually a widget. It
    is much faster than
    \l{qobject_cast()}{qobject_cast}<QWidget *>(\e{obj}) or
    \e{obj}->\l{inherits()}{inherits}("QWidget").

    Some QObject functions, e.g. children(), return a QObjectList.
    QObjectList is a typedef for QList<QObject *>.

    \target No copy constructor
    \section1 No copy constructor or assignment operator

    QObject has neither a copy constructor nor an assignment operator.
    This is by design. Actually, they are declared, but in a
    \c{private} section with the macro Q_DISABLE_COPY(). In fact, all
    Qt classes derived from QObject (direct or indirect) use this
    macro to declare their copy constructor and assignment operator to
    be private. The reasoning is found in the discussion on
    \l{Identity vs Value} {Identity vs Value} on the Qt \l{Object
    Model} page.

    The main consequence is that you should use pointers to QObject
    (or to your QObject subclass) where you might otherwise be tempted
    to use your QObject subclass as a value. For example, without a
    copy constructor, you can't use a subclass of QObject as the value
    to be stored in one of the container classes. You must store
    pointers.

    \section1 Auto-Connection

    Qt's meta-object system provides a mechanism to automatically connect
    signals and slots between QObject subclasses and their children. As long
    as objects are defined with suitable object names, and slots follow a
    simple naming convention, this connection can be performed at run-time
    by the QMetaObject::connectSlotsByName() function.

    \l uic generates code that invokes this function to enable
    auto-connection to be performed between widgets on forms created
    with \QD. More information about using auto-connection with \QD is
    given in the \l{Using a Designer UI File in Your Application} section of
    the \QD manual.

    \section1 Dynamic Properties

    From Qt 4.2, dynamic properties can be added to and removed from QObject
    instances at run-time. Dynamic properties do not need to be declared at
    compile-time, yet they provide the same advantages as static properties
    and are manipulated using the same API - using property() to read them
    and setProperty() to write them.

    From Qt 4.3, dynamic properties are supported by
    \l{Qt Designer's Widget Editing Mode#The Property Editor}{Qt Designer},
    and both standard Qt widgets and user-created forms can be given dynamic
    properties.

    \section1 Internationalization (i18n)

    All QObject subclasses support Qt's translation features, making it possible
    to translate an application's user interface into different languages.

    To make user-visible text translatable, it must be wrapped in calls to
    the tr() function. This is explained in detail in the
    \l{Writing Source Code for Translation} document.

    \sa QMetaObject, QPointer, QObjectCleanupHandler, Q_DISABLE_COPY()
    \sa {Object Trees & Ownership}
*/

/*!
    \relates QObject

    Returns a pointer to the object named \a name that inherits \a
    type and with a given \a parent.

    Returns 0 if there is no such child.

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 0
*/

void *qt_find_obj_child(QObject *parent, const char *type, const QString &name)
{
    QObjectList list = parent->children();
    if (list.size() == 0) return 0;
    for (int i = 0; i < list.size(); ++i) {
        QObject *obj = list.at(i);
        if (name == obj->objectName() && obj->inherits(type))
            return obj;
    }
    return 0;
}


/*****************************************************************************
  QObject member functions
 *****************************************************************************/

// check the constructor's parent thread argument
static bool check_parent_thread(QObject *parent,
                                QThreadData *parentThreadData,
                                QThreadData *currentThreadData)
{
    if (parent && parentThreadData != currentThreadData) {
        QThread *parentThread = parentThreadData->thread;
        QThread *currentThread = currentThreadData->thread;
        qWarning("QObject: Cannot create children for a parent that is in a different thread.\n"
                 "(Parent is %s(%p), parent's thread is %s(%p), current thread is %s(%p)",
                 parent->metaObject()->className(),
                 parent,
                 parentThread ? parentThread->metaObject()->className() : "QThread",
                 parentThread,
                 currentThread ? currentThread->metaObject()->className() : "QThread",
                 currentThread);
        return false;
    }
    return true;
}

/*!
    Constructs an object with parent object \a parent.

    The parent of an object may be viewed as the object's owner. For
    instance, a \l{QDialog}{dialog box} is the parent of the \gui OK
    and \gui Cancel buttons it contains.

    The destructor of a parent object destroys all child objects.

    Setting \a parent to 0 constructs an object with no parent. If the
    object is a widget, it will become a top-level window.

    \sa parent(), findChild(), findChildren()
*/

QObject::QObject(QObject *parent)
    : d_ptr(new QObjectPrivate)
{
    Q_D(QObject);
    d_ptr->q_ptr = this;
    d->threadData = (parent && !parent->thread()) ? parent->d_func()->threadData : QThreadData::current();
    d->threadData->ref();
    if (parent) {
        QT_TRY {
            if (!check_parent_thread(parent, parent ? parent->d_func()->threadData : 0, d->threadData))
                parent = 0;
            setParent(parent);
        } QT_CATCH(...) {
            d->threadData->deref();
            QT_RETHROW;
        }
    }
    qt_addObject(this);
}

#ifdef QT3_SUPPORT
/*!
    \overload QObject()
    \obsolete

    Creates a new QObject with the given \a parent and object \a name.
 */
QObject::QObject(QObject *parent, const char *name)
    : d_ptr(new QObjectPrivate)
{
    Q_D(QObject);
    qt_addObject(d_ptr->q_ptr = this);
    d->threadData = (parent && !parent->thread()) ? parent->d_func()->threadData : QThreadData::current();
    d->threadData->ref();
    if (parent) {
        if (!check_parent_thread(parent, parent ? parent->d_func()->threadData : 0, d->threadData))
            parent = 0;
        setParent(parent);
    }
    setObjectName(QString::fromAscii(name));
}
#endif

/*! \internal
 */
QObject::QObject(QObjectPrivate &dd, QObject *parent)
    : d_ptr(&dd)
{
    Q_D(QObject);
    d_ptr->q_ptr = this;
    d->threadData = (parent && !parent->thread()) ? parent->d_func()->threadData : QThreadData::current();
    d->threadData->ref();
    if (parent) {
        QT_TRY {
            if (!check_parent_thread(parent, parent ? parent->d_func()->threadData : 0, d->threadData))
                parent = 0;
            if (d->isWidget) {
                if (parent) {
                    d->parent = parent;
                    d->parent->d_func()->children.append(this);
                }
                // no events sent here, this is done at the end of the QWidget constructor
            } else {
                setParent(parent);
            }
        } QT_CATCH(...) {
            d->threadData->deref();
            QT_RETHROW;
        }
    }
    qt_addObject(this);
}

/*!
    Destroys the object, deleting all its child objects.

    All signals to and from the object are automatically disconnected, and
    any pending posted events for the object are removed from the event
    queue. However, it is often safer to use deleteLater() rather than
    deleting a QObject subclass directly.

    \warning All child objects are deleted. If any of these objects
    are on the stack or global, sooner or later your program will
    crash. We do not recommend holding pointers to child objects from
    outside the parent. If you still do, the destroyed() signal gives
    you an opportunity to detect when an object is destroyed.

    \warning Deleting a QObject while pending events are waiting to
    be delivered can cause a crash. You must not delete the QObject
    directly if it exists in a different thread than the one currently
    executing. Use deleteLater() instead, which will cause the event
    loop to delete the object after all pending events have been
    delivered to it.

    \sa deleteLater()
*/

QObject::~QObject()
{
    Q_D(QObject);
    d->wasDeleted = true;
    d->blockSig = 0; // unblock signals so we always emit destroyed()

    if (d->hasGuards && !d->isWidget) {
        // set all QPointers for this object to zero - note that
        // ~QWidget() does this for us, so we don't have to do it twice
        QObjectPrivate::clearGuards(this);
    }

    if (d->sharedRefcount) {
        if (d->sharedRefcount->strongref > 0) {
            qWarning("QObject: shared QObject was deleted directly. The program is malformed and may crash.");
            // but continue deleting, it's too late to stop anyway
        }

        // indicate to all QWeakPointers that this QObject has now been deleted
        d->sharedRefcount->strongref = 0;
        if (!d->sharedRefcount->weakref.deref())
            delete d->sharedRefcount;
    }


    if (d->isSignalConnected(0)) {
        QT_TRY {
            emit destroyed(this);
        } QT_CATCH(...) {
            // all the signal/slots connections are still in place - if we don't
            // quit now, we will crash pretty soon.
            qWarning("Detected an unexpected exception in ~QObject while emitting destroyed().");
            QT_RETHROW;
        }
    }

    if (d->declarativeData)
        QAbstractDeclarativeData::destroyed(d->declarativeData, this);

    // set ref to zero to indicate that this object has been deleted
    if (d->currentSender != 0)
        d->currentSender->ref = 0;
    d->currentSender = 0;

    if (d->connectionLists || d->senders) {
        QMutex *signalSlotMutex = signalSlotLock(this);
        QMutexLocker locker(signalSlotMutex);

        // disconnect all receivers
        if (d->connectionLists) {
            ++d->connectionLists->inUse;
            int connectionListsCount = d->connectionLists->count();
            for (int signal = -1; signal < connectionListsCount; ++signal) {
                QObjectPrivate::ConnectionList &connectionList =
                    (*d->connectionLists)[signal];

                while (QObjectPrivate::Connection *c = connectionList.first) {
                    if (!c->receiver) {
                        connectionList.first = c->nextConnectionList;
                        delete c;
                        continue;
                    }

                    QMutex *m = signalSlotLock(c->receiver);
                    bool needToUnlock = QOrderedMutexLocker::relock(signalSlotMutex, m);

                    if (c->receiver) {
                        *c->prev = c->next;
                        if (c->next) c->next->prev = c->prev;
                    }
                    if (needToUnlock)
                        m->unlockInline();

                    connectionList.first = c->nextConnectionList;
                    delete c;
                }
            }

            if (!--d->connectionLists->inUse) {
                delete d->connectionLists;
            } else {
                d->connectionLists->orphaned = true;
            }
            d->connectionLists = 0;
        }

        // disconnect all senders
        QObjectPrivate::Connection *node = d->senders;
        while (node) {
            QObject *sender = node->sender;
            QMutex *m = signalSlotLock(sender);
            node->prev = &node;
            bool needToUnlock = QOrderedMutexLocker::relock(signalSlotMutex, m);
            //the node has maybe been removed while the mutex was unlocked in relock?
            if (!node || node->sender != sender) {
                m->unlockInline();
                continue;
            }
            node->receiver = 0;
            QObjectConnectionListVector *senderLists = sender->d_func()->connectionLists;
            if (senderLists)
                senderLists->dirty = true;

            node = node->next;
            if (needToUnlock)
                m->unlockInline();
        }
    }

    if (!d->children.isEmpty())
        d->deleteChildren();

    qt_removeObject(this);

    if (d->parent)        // remove it from parent object
        d->setParent_helper(0);

#ifdef QT_JAMBI_BUILD
    if (d->inEventHandler) {
        qWarning("QObject: Do not delete object, '%s', during its event handler!",
                 objectName().isNull() ? "unnamed" : qPrintable(objectName()));
    }
#endif
}

QObjectPrivate::Connection::~Connection()
{
    if (argumentTypes != &DIRECT_CONNECTION_ONLY)
        delete [] static_cast<int *>(argumentTypes);
}


/*!
    \fn QMetaObject *QObject::metaObject() const

    Returns a pointer to the meta-object of this object.

    A meta-object contains information about a class that inherits
    QObject, e.g. class name, superclass name, properties, signals and
    slots. Every QObject subclass that contains the Q_OBJECT macro will have a
    meta-object.

    The meta-object information is required by the signal/slot
    connection mechanism and the property system. The inherits()
    function also makes use of the meta-object.

    If you have no pointer to an actual object instance but still
    want to access the meta-object of a class, you can use \l
    staticMetaObject.

    Example:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 1

    \sa staticMetaObject
*/

/*!
    \variable QObject::staticMetaObject

    This variable stores the meta-object for the class.

    A meta-object contains information about a class that inherits
    QObject, e.g. class name, superclass name, properties, signals and
    slots. Every class that contains the Q_OBJECT macro will also have
    a meta-object.

    The meta-object information is required by the signal/slot
    connection mechanism and the property system. The inherits()
    function also makes use of the meta-object.

    If you have a pointer to an object, you can use metaObject() to
    retrieve the meta-object associated with that object.

    Example:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 2

    \sa metaObject()
*/

/*! \fn T *qobject_cast<T *>(QObject *object)
    \relates QObject

    Returns the given \a object cast to type T if the object is of type
    T (or of a subclass); otherwise returns 0.  If \a object is 0 then 
    it will also return 0.

    The class T must inherit (directly or indirectly) QObject and be
    declared with the \l Q_OBJECT macro.

    A class is considered to inherit itself.

    Example:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 3

    The qobject_cast() function behaves similarly to the standard C++
    \c dynamic_cast(), with the advantages that it doesn't require
    RTTI support and it works across dynamic library boundaries.

    qobject_cast() can also be used in conjunction with interfaces;
    see the \l{tools/plugandpaint}{Plug & Paint} example for details.

    \warning If T isn't declared with the Q_OBJECT macro, this
    function's return value is undefined.

    \sa QObject::inherits()
*/

/*!
    \fn bool QObject::inherits(const char *className) const

    Returns true if this object is an instance of a class that
    inherits \a className or a QObject subclass that inherits \a
    className; otherwise returns false.

    A class is considered to inherit itself.

    Example:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 4

    If you need to determine whether an object is an instance of a particular
    class for the purpose of casting it, consider using qobject_cast<Type *>(object)
    instead.

    \sa metaObject(), qobject_cast()
*/

/*!
    \property QObject::objectName

    \brief the name of this object

    You can find an object by name (and type) using findChild(). You can
    find a set of objects with findChildren().

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 5

    By default, this property contains an empty string.

    \sa metaObject(), QMetaObject::className()
*/

QString QObject::objectName() const
{
    Q_D(const QObject);
    return d->objectName;
}

/*
    Sets the object's name to \a name.
*/
void QObject::setObjectName(const QString &name)
{
    Q_D(QObject);
    bool objectNameChanged = d->declarativeData && d->objectName != name;

    d->objectName = name;

    if (objectNameChanged) 
        d->declarativeData->objectNameChanged(d->declarativeData, this);
}


#ifdef QT3_SUPPORT
/*! \internal
    QObject::child is compat but needs to call itself recursively,
    that's why we need this helper.
*/
static QObject *qChildHelper(const char *objName, const char *inheritsClass,
                             bool recursiveSearch, const QObjectList &children)
{
    if (children.isEmpty())
        return 0;

    bool onlyWidgets = (inheritsClass && qstrcmp(inheritsClass, "QWidget") == 0);
    const QLatin1String oName(objName);
    for (int i = 0; i < children.size(); ++i) {
        QObject *obj = children.at(i);
        if (onlyWidgets) {
            if (obj->isWidgetType() && (!objName || obj->objectName() == oName))
                return obj;
        } else if ((!inheritsClass || obj->inherits(inheritsClass))
                   && (!objName || obj->objectName() == oName))
            return obj;
        if (recursiveSearch && (obj = qChildHelper(objName, inheritsClass,
                                                   recursiveSearch, obj->children())))
            return obj;
    }
    return 0;
}


/*!
    Searches the children and optionally grandchildren of this object,
    and returns a child that is called \a objName that inherits \a
    inheritsClass. If \a inheritsClass is 0 (the default), any class
    matches.

    If \a recursiveSearch is true (the default), child() performs a
    depth-first search of the object's children.

    If there is no such object, this function returns 0. If there are
    more than one, the first one found is returned.
*/
QObject* QObject::child(const char *objName, const char *inheritsClass,
                        bool recursiveSearch) const
{
    Q_D(const QObject);
    return qChildHelper(objName, inheritsClass, recursiveSearch, d->children);
}
#endif

/*!
    \fn bool QObject::isWidgetType() const

    Returns true if the object is a widget; otherwise returns false.

    Calling this function is equivalent to calling
    inherits("QWidget"), except that it is much faster.
*/


/*!
    This virtual function receives events to an object and should
    return true if the event \a e was recognized and processed.

    The event() function can be reimplemented to customize the
    behavior of an object.

    \sa installEventFilter(), timerEvent(), QApplication::sendEvent(),
    QApplication::postEvent(), QWidget::event()
*/

bool QObject::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::Timer:
        timerEvent((QTimerEvent*)e);
        break;

#ifdef QT3_SUPPORT
    case QEvent::ChildInsertedRequest:
        d_func()->sendPendingChildInsertedEvents();
        break;
#endif

    case QEvent::ChildAdded:
    case QEvent::ChildPolished:
#ifdef QT3_SUPPORT
    case QEvent::ChildInserted:
#endif
    case QEvent::ChildRemoved:
        childEvent((QChildEvent*)e);
        break;

    case QEvent::DeferredDelete:
        qDeleteInEventHandler(this);
        break;

    case QEvent::MetaCall:
        {
#ifdef QT_JAMBI_BUILD
            d_func()->inEventHandler = false;
#endif
            QMetaCallEvent *mce = static_cast<QMetaCallEvent*>(e);
            QObjectPrivate::Sender currentSender;
            currentSender.sender = const_cast<QObject*>(mce->sender());
            currentSender.signal = mce->signalId();
            currentSender.ref = 1;
            QObjectPrivate::Sender * const previousSender =
                QObjectPrivate::setCurrentSender(this, &currentSender);
#if defined(QT_NO_EXCEPTIONS)
            mce->placeMetaCall(this);
#else
            QT_TRY {
                mce->placeMetaCall(this);
            } QT_CATCH(...) {
                QObjectPrivate::resetCurrentSender(this, &currentSender, previousSender);
                QT_RETHROW;
            }
#endif
            QObjectPrivate::resetCurrentSender(this, &currentSender, previousSender);
            break;
        }

    case QEvent::ThreadChange: {
        Q_D(QObject);
        QThreadData *threadData = d->threadData;
        QAbstractEventDispatcher *eventDispatcher = threadData->eventDispatcher;
        if (eventDispatcher) {
            QList<QPair<int, int> > timers = eventDispatcher->registeredTimers(this);
            if (!timers.isEmpty()) {
                // set inThreadChangeEvent to true to tell the dispatcher not to release out timer ids
                // back to the pool (since the timer ids are moving to a new thread).
                d->inThreadChangeEvent = true;
                eventDispatcher->unregisterTimers(this);
                d->inThreadChangeEvent = false;
                QMetaObject::invokeMethod(this, "_q_reregisterTimers", Qt::QueuedConnection,
                                          Q_ARG(void*, (new QList<QPair<int, int> >(timers))));
            }
        }
        break;
    }

    default:
        if (e->type() >= QEvent::User) {
            customEvent(e);
            break;
        }
        return false;
    }
    return true;
}

/*!
    \fn void QObject::timerEvent(QTimerEvent *event)

    This event handler can be reimplemented in a subclass to receive
    timer events for the object.

    QTimer provides a higher-level interface to the timer
    functionality, and also more general information about timers. The
    timer event is passed in the \a event parameter.

    \sa startTimer(), killTimer(), event()
*/

void QObject::timerEvent(QTimerEvent *)
{
}


/*!
    This event handler can be reimplemented in a subclass to receive
    child events. The event is passed in the \a event parameter.

    QEvent::ChildAdded and QEvent::ChildRemoved events are sent to
    objects when children are added or removed. In both cases you can
    only rely on the child being a QObject, or if isWidgetType()
    returns true, a QWidget. (This is because, in the
    \l{QEvent::ChildAdded}{ChildAdded} case, the child is not yet
    fully constructed, and in the \l{QEvent::ChildRemoved}{ChildRemoved}
    case it might have been destructed already).

    QEvent::ChildPolished events are sent to widgets when children
    are polished, or when polished children are added. If you receive
    a child polished event, the child's construction is usually
    completed. However, this is not guaranteed, and multiple polish
    events may be delivered during the execution of a widget's
    constructor.

    For every child widget, you receive one
    \l{QEvent::ChildAdded}{ChildAdded} event, zero or more
    \l{QEvent::ChildPolished}{ChildPolished} events, and one
    \l{QEvent::ChildRemoved}{ChildRemoved} event.

    The \l{QEvent::ChildPolished}{ChildPolished} event is omitted if
    a child is removed immediately after it is added. If a child is
    polished several times during construction and destruction, you
    may receive several child polished events for the same child,
    each time with a different virtual table.

    \sa event()
*/

void QObject::childEvent(QChildEvent * /* event */)
{
}


/*!
    This event handler can be reimplemented in a subclass to receive
    custom events. Custom events are user-defined events with a type
    value at least as large as the QEvent::User item of the
    QEvent::Type enum, and is typically a QEvent subclass. The event
    is passed in the \a event parameter.

    \sa event(), QEvent
*/
void QObject::customEvent(QEvent * /* event */)
{
}



/*!
    Filters events if this object has been installed as an event
    filter for the \a watched object.

    In your reimplementation of this function, if you want to filter
    the \a event out, i.e. stop it being handled further, return
    true; otherwise return false.

    Example:
    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 6

    Notice in the example above that unhandled events are passed to
    the base class's eventFilter() function, since the base class
    might have reimplemented eventFilter() for its own internal
    purposes.

    \warning If you delete the receiver object in this function, be
    sure to return true. Otherwise, Qt will forward the event to the
    deleted object and the program might crash.

    \sa installEventFilter()
*/

bool QObject::eventFilter(QObject * /* watched */, QEvent * /* event */)
{
    return false;
}

/*!
    \fn bool QObject::signalsBlocked() const

    Returns true if signals are blocked; otherwise returns false.

    Signals are not blocked by default.

    \sa blockSignals()
*/

/*!
    If \a block is true, signals emitted by this object are blocked
    (i.e., emitting a signal will not invoke anything connected to it).
    If \a block is false, no such blocking will occur.

    The return value is the previous value of signalsBlocked().

    Note that the destroyed() signal will be emitted even if the signals
    for this object have been blocked.

    \sa signalsBlocked()
*/

bool QObject::blockSignals(bool block)
{
    Q_D(QObject);
    bool previous = d->blockSig;
    d->blockSig = block;
    return previous;
}

/*!
    Returns the thread in which the object lives.

    \sa moveToThread()
*/
QThread *QObject::thread() const
{
    return d_func()->threadData->thread;
}

/*!
    Changes the thread affinity for this object and its children. The
    object cannot be moved if it has a parent. Event processing will
    continue in the \a targetThread.

    To move an object to the main thread, use QApplication::instance()
    to retrieve a pointer to the current application, and then use
    QApplication::thread() to retrieve the thread in which the
    application lives. For example:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 7

    If \a targetThread is zero, all event processing for this object
    and its children stops.

    Note that all active timers for the object will be reset. The
    timers are first stopped in the current thread and restarted (with
    the same interval) in the \a targetThread. As a result, constantly
    moving an object between threads can postpone timer events
    indefinitely.

    A QEvent::ThreadChange event is sent to this object just before
    the thread affinity is changed. You can handle this event to
    perform any special processing. Note that any new events that are
    posted to this object will be handled in the \a targetThread.

    \warning This function is \e not thread-safe; the current thread
    must be same as the current thread affinity. In other words, this
    function can only "push" an object from the current thread to
    another thread, it cannot "pull" an object from any arbitrary
    thread to the current thread.

    \sa thread()
 */
void QObject::moveToThread(QThread *targetThread)
{
    Q_D(QObject);

    if (d->threadData->thread == targetThread) {
        // object is already in this thread
        return;
    }

    if (d->parent != 0) {
        qWarning("QObject::moveToThread: Cannot move objects with a parent");
        return;
    }
    if (d->isWidget) {
        qWarning("QObject::moveToThread: Widgets cannot be moved to a new thread");
        return;
    }

    QThreadData *currentData = QThreadData::current();
    QThreadData *targetData = targetThread ? QThreadData::get2(targetThread) : new QThreadData(0);
    if (d->threadData->thread == 0 && currentData == targetData) {
        // one exception to the rule: we allow moving objects with no thread affinity to the current thread
        currentData = d->threadData;
    } else if (d->threadData != currentData) {
        qWarning("QObject::moveToThread: Current thread (%p) is not the object's thread (%p).\n"
                 "Cannot move to target thread (%p)\n",
                 currentData->thread, d->threadData->thread, targetData->thread);

#ifdef Q_WS_MAC
        qWarning("On Mac OS X, you might be loading two sets of Qt binaries into the same process. "
                 "Check that all plugins are compiled against the right Qt binaries. Export "
                 "DYLD_PRINT_LIBRARIES=1 and check that only one set of binaries are being loaded.");
#endif

        return;
    }

    // prepare to move
    d->moveToThread_helper();

    QOrderedMutexLocker locker(&currentData->postEventList.mutex,
                               &targetData->postEventList.mutex);

    // keep currentData alive (since we've got it locked)
    currentData->ref();

    // move the object
    d_func()->setThreadData_helper(currentData, targetData);

    locker.unlock();

    // now currentData can commit suicide if it wants to
    currentData->deref();
}

void QObjectPrivate::moveToThread_helper()
{
    Q_Q(QObject);
    QEvent e(QEvent::ThreadChange);
    QCoreApplication::sendEvent(q, &e);
    for (int i = 0; i < children.size(); ++i) {
        QObject *child = children.at(i);
        child->d_func()->moveToThread_helper();
    }
}

void QObjectPrivate::setThreadData_helper(QThreadData *currentData, QThreadData *targetData)
{
    Q_Q(QObject);

    // move posted events
    int eventsMoved = 0;
    for (int i = 0; i < currentData->postEventList.size(); ++i) {
        const QPostEvent &pe = currentData->postEventList.at(i);
        if (!pe.event)
            continue;
        if (pe.receiver == q) {
            // move this post event to the targetList
            targetData->postEventList.addEvent(pe);
            const_cast<QPostEvent &>(pe).event = 0;
            ++eventsMoved;
        }
    }
    if (eventsMoved > 0 && targetData->eventDispatcher) {
        targetData->canWait = false;
        targetData->eventDispatcher->wakeUp();
    }

    // the current emitting thread shouldn't restore currentSender after calling moveToThread()
    if (currentSender)
        currentSender->ref = 0;
    currentSender = 0;

#ifdef QT_JAMBI_BUILD
    // the current event thread also shouldn't restore the delete watch
    inEventHandler = false;

    if (deleteWatch)
        *deleteWatch = 1;
    deleteWatch = 0;
#endif

    // set new thread data
    targetData->ref();
    threadData->deref();
    threadData = targetData;

    for (int i = 0; i < children.size(); ++i) {
        QObject *child = children.at(i);
        child->d_func()->setThreadData_helper(currentData, targetData);
    }
}

void QObjectPrivate::_q_reregisterTimers(void *pointer)
{
    Q_Q(QObject);
    QList<QPair<int, int> > *timerList = reinterpret_cast<QList<QPair<int, int> > *>(pointer);
    QAbstractEventDispatcher *eventDispatcher = threadData->eventDispatcher;
    for (int i = 0; i < timerList->size(); ++i) {
        const QPair<int, int> &pair = timerList->at(i);
        eventDispatcher->registerTimer(pair.first, pair.second, q);
    }
    delete timerList;
}


//
// The timer flag hasTimer is set when startTimer is called.
// It is not reset when killing the timer because more than
// one timer might be active.
//

/*!
    Starts a timer and returns a timer identifier, or returns zero if
    it could not start a timer.

    A timer event will occur every \a interval milliseconds until
    killTimer() is called. If \a interval is 0, then the timer event
    occurs once every time there are no more window system events to
    process.

    The virtual timerEvent() function is called with the QTimerEvent
    event parameter class when a timer event occurs. Reimplement this
    function to get timer events.

    If multiple timers are running, the QTimerEvent::timerId() can be
    used to find out which timer was activated.

    Example:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 8

    Note that QTimer's accuracy depends on the underlying operating
    system and hardware. Most platforms support an accuracy of 20
    milliseconds; some provide more. If Qt is unable to deliver the
    requested number of timer events, it will silently discard some.

    The QTimer class provides a high-level programming interface with
    single-shot timers and timer signals instead of events. There is
    also a QBasicTimer class that is more lightweight than QTimer and
    less clumsy than using timer IDs directly.

    \sa timerEvent(), killTimer(), QTimer::singleShot()
*/

int QObject::startTimer(int interval)
{
    Q_D(QObject);

    if (interval < 0) {
        qWarning("QObject::startTimer: QTimer cannot have a negative interval");
        return 0;
    }

    d->pendTimer = true;                                // set timer flag

    if (!d->threadData->eventDispatcher) {
        qWarning("QObject::startTimer: QTimer can only be used with threads started with QThread");
        return 0;
    }
    return d->threadData->eventDispatcher->registerTimer(interval, this);
}

/*!
    Kills the timer with timer identifier, \a id.

    The timer identifier is returned by startTimer() when a timer
    event is started.

    \sa timerEvent(), startTimer()
*/

void QObject::killTimer(int id)
{
    Q_D(QObject);
    if (d->threadData->eventDispatcher)
        d->threadData->eventDispatcher->unregisterTimer(id);
}


/*!
    \fn QObject *QObject::parent() const

    Returns a pointer to the parent object.

    \sa children()
*/

/*!
    \fn const QObjectList &QObject::children() const

    Returns a list of child objects.
    The QObjectList class is defined in the \c{<QObject>} header
    file as the following:

    \quotefromfile src/corelib/kernel/qobject.h
    \skipto /typedef .*QObjectList/
    \printuntil QObjectList

    The first child added is the \l{QList::first()}{first} object in
    the list and the last child added is the \l{QList::last()}{last}
    object in the list, i.e. new children are appended at the end.

    Note that the list order changes when QWidget children are
    \l{QWidget::raise()}{raised} or \l{QWidget::lower()}{lowered}. A
    widget that is raised becomes the last object in the list, and a
    widget that is lowered becomes the first object in the list.

    \sa findChild(), findChildren(), parent(), setParent()
*/

#ifdef QT3_SUPPORT
static void objSearch(QObjectList &result,
                      const QObjectList &list,
                      const char *inheritsClass,
                      bool onlyWidgets,
                      const char *objName,
                      QRegExp *rx,
                      bool recurse)
{
    for (int i = 0; i < list.size(); ++i) {
        QObject *obj = list.at(i);
        if (!obj)
            continue;
        bool ok = true;
        if (onlyWidgets)
            ok = obj->isWidgetType();
        else if (inheritsClass && !obj->inherits(inheritsClass))
            ok = false;
        if (ok) {
            if (objName)
                ok = (obj->objectName() == QLatin1String(objName));
#ifndef QT_NO_REGEXP
            else if (rx)
                ok = (rx->indexIn(obj->objectName()) != -1);
#endif
        }
        if (ok)                                // match!
            result.append(obj);
        if (recurse) {
            QObjectList clist = obj->children();
            if (!clist.isEmpty())
                objSearch(result, clist, inheritsClass,
                           onlyWidgets, objName, rx, recurse);
        }
    }
}

/*!
    \internal

    Searches the children and optionally grandchildren of this object,
    and returns a list of those objects that are named or that match
    \a objName and inherit \a inheritsClass. If \a inheritsClass is 0
    (the default), all classes match. If \a objName is 0 (the
    default), all object names match.

    If \a regexpMatch is true (the default), \a objName is a regular
    expression that the objects's names must match. The syntax is that
    of a QRegExp. If \a regexpMatch is false, \a objName is a string
    and object names must match it exactly.

    Note that \a inheritsClass uses single inheritance from QObject,
    the way inherits() does. According to inherits(), QWidget
    inherits QObject but not QPaintDevice. This does not quite match
    reality, but is the best that can be done on the wide variety of
    compilers Qt supports.

    Finally, if \a recursiveSearch is true (the default), queryList()
    searches \e{n}th-generation as well as first-generation children.

    If all this seems a bit complex for your needs, the simpler
    child() function may be what you want.

    This somewhat contrived example disables all the buttons in this
    window:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 9

    \warning Delete the list as soon you have finished using it. The
    list contains pointers that may become invalid at almost any time
    without notice (as soon as the user closes a window you may have
    dangling pointers, for example).

    \sa child() children(), parent(), inherits(), objectName(), QRegExp
*/

QObjectList QObject::queryList(const char *inheritsClass,
                               const char *objName,
                               bool regexpMatch,
                               bool recursiveSearch) const
{
    Q_D(const QObject);
    QObjectList list;
    bool onlyWidgets = (inheritsClass && qstrcmp(inheritsClass, "QWidget") == 0);
#ifndef QT_NO_REGEXP
    if (regexpMatch && objName) {                // regexp matching
        QRegExp rx(QString::fromLatin1(objName));
        objSearch(list, d->children, inheritsClass, onlyWidgets, 0, &rx, recursiveSearch);
    } else
#endif
    {
        objSearch(list, d->children, inheritsClass, onlyWidgets, objName, 0, recursiveSearch);
    }
    return list;
}
#endif

/*!
    \fn T *QObject::findChild(const QString &name) const

    Returns the child of this object that can be cast into type T and
    that is called \a name, or 0 if there is no such object.
    Omitting the \a name argument causes all object names to be matched.
    The search is performed recursively.

    If there is more than one child matching the search, the most
    direct ancestor is returned. If there are several direct
    ancestors, it is undefined which one will be returned. In that
    case, findChildren() should be used.

    This example returns a child \l{QPushButton} of \c{parentWidget}
    named \c{"button1"}:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 10

    This example returns a \l{QListWidget} child of \c{parentWidget}:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 11

    \sa findChildren()
*/

/*!
    \fn QList<T> QObject::findChildren(const QString &name) const

    Returns all children of this object with the given \a name that can be
    cast to type T, or an empty list if there are no such objects.
    Omitting the \a name argument causes all object names to be matched.
    The search is performed recursively.

    The following example shows how to find a list of child \l{QWidget}s of
    the specified \c{parentWidget} named \c{widgetname}:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 12

    This example returns all \c{QPushButton}s that are children of \c{parentWidget}:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 13

    \sa findChild()
*/

/*!
    \fn QList<T> QObject::findChildren(const QRegExp &regExp) const
    \overload findChildren()

    Returns the children of this object that can be cast to type T
    and that have names matching the regular expression \a regExp,
    or an empty list if there are no such objects.
    The search is performed recursively.
*/

/*!
    \fn T qFindChild(const QObject *obj, const QString &name)
    \relates QObject
    \overload qFindChildren()
    \obsolete

    This function is equivalent to
    \a{obj}->\l{QObject::findChild()}{findChild}<T>(\a name).

    \note This function was provided as a workaround for MSVC 6
    which did not support member template functions. It is advised
    to use the other form in new code.

    \sa QObject::findChild()
*/

/*!
    \fn QList<T> qFindChildren(const QObject *obj, const QString &name)
    \relates QObject
    \overload qFindChildren()
    \obsolete

    This function is equivalent to
    \a{obj}->\l{QObject::findChildren()}{findChildren}<T>(\a name).

    \note This function was provided as a workaround for MSVC 6
    which did not support member template functions. It is advised
    to use the other form in new code.

    \sa QObject::findChildren()
*/

/*!
    \fn QList<T> qFindChildren(const QObject *obj, const QRegExp &regExp)
    \relates QObject
    \overload qFindChildren()

    This function is equivalent to
    \a{obj}->\l{QObject::findChildren()}{findChildren}<T>(\a regExp).

    \note This function was provided as a workaround for MSVC 6
    which did not support member template functions. It is advised
    to use the other form in new code.

    \sa QObject::findChildren()
*/

/*!
    \internal
*/
void qt_qFindChildren_helper(const QObject *parent, const QString &name, const QRegExp *re,
                             const QMetaObject &mo, QList<void*> *list)
{
    if (!parent || !list)
        return;
    const QObjectList &children = parent->children();
    QObject *obj;
    for (int i = 0; i < children.size(); ++i) {
        obj = children.at(i);
        if (mo.cast(obj)) {
            if (re) {
                if (re->indexIn(obj->objectName()) != -1)
                    list->append(obj);
            } else {
                if (name.isNull() || obj->objectName() == name)
                    list->append(obj);
            }
        }
        qt_qFindChildren_helper(obj, name, re, mo, list);
    }
}

/*! \internal
 */
QObject *qt_qFindChild_helper(const QObject *parent, const QString &name, const QMetaObject &mo)
{
    if (!parent)
        return 0;
    const QObjectList &children = parent->children();
    QObject *obj;
    int i;
    for (i = 0; i < children.size(); ++i) {
        obj = children.at(i);
        if (mo.cast(obj) && (name.isNull() || obj->objectName() == name))
            return obj;
    }
    for (i = 0; i < children.size(); ++i) {
        obj = qt_qFindChild_helper(children.at(i), name, mo);
        if (obj)
            return obj;
    }
    return 0;
}

/*!
    Makes the object a child of \a parent.

    \sa QWidget::setParent()
*/

void QObject::setParent(QObject *parent)
{
    Q_D(QObject);
    Q_ASSERT(!d->isWidget);
    d->setParent_helper(parent);
}

void QObjectPrivate::deleteChildren()
{
    const bool reallyWasDeleted = wasDeleted;
    wasDeleted = true;
    // delete children objects
    // don't use qDeleteAll as the destructor of the child might
    // delete siblings
    for (int i = 0; i < children.count(); ++i) {
        currentChildBeingDeleted = children.at(i);
        children[i] = 0;
        delete currentChildBeingDeleted;
    }
    children.clear();
    currentChildBeingDeleted = 0;
    wasDeleted = reallyWasDeleted;
}

void QObjectPrivate::setParent_helper(QObject *o)
{
    Q_Q(QObject);
    if (o == parent)
        return;
    if (parent) {
        QObjectPrivate *parentD = parent->d_func();
        if (parentD->wasDeleted && wasDeleted
            && parentD->currentChildBeingDeleted == q) {
            // don't do anything since QObjectPrivate::deleteChildren() already
            // cleared our entry in parentD->children.
        } else {
            const int index = parentD->children.indexOf(q);
            if (parentD->wasDeleted) {
                parentD->children[index] = 0;
            } else {
                parentD->children.removeAt(index);
                if (sendChildEvents && parentD->receiveChildEvents) {
                    QChildEvent e(QEvent::ChildRemoved, q);
                    QCoreApplication::sendEvent(parent, &e);
                }
            }
        }
    }
    parent = o;
    if (parent) {
        // object hierarchies are constrained to a single thread
        if (threadData != parent->d_func()->threadData) {
            qWarning("QObject::setParent: Cannot set parent, new parent is in a different thread");
            parent = 0;
            return;
        }
        parent->d_func()->children.append(q);
        if(sendChildEvents && parent->d_func()->receiveChildEvents) {
            if (!isWidget) {
                QChildEvent e(QEvent::ChildAdded, q);
                QCoreApplication::sendEvent(parent, &e);
#ifdef QT3_SUPPORT
                if (QCoreApplicationPrivate::useQt3Support) {
                    if (parent->d_func()->pendingChildInsertedEvents.isEmpty()) {
                        QCoreApplication::postEvent(parent,
                                                    new QEvent(QEvent::ChildInsertedRequest),
                                                    Qt::HighEventPriority);
                    }
                    parent->d_func()->pendingChildInsertedEvents.append(q);
                }
#endif
            }
        }
    }
    if (!wasDeleted && declarativeData)
        QAbstractDeclarativeData::parentChanged(declarativeData, q, o);
}

/*!
    \fn void QObject::installEventFilter(QObject *filterObj)

    Installs an event filter \a filterObj on this object. For example:
    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 14

    An event filter is an object that receives all events that are
    sent to this object. The filter can either stop the event or
    forward it to this object. The event filter \a filterObj receives
    events via its eventFilter() function. The eventFilter() function
    must return true if the event should be filtered, (i.e. stopped);
    otherwise it must return false.

    If multiple event filters are installed on a single object, the
    filter that was installed last is activated first.

    Here's a \c KeyPressEater class that eats the key presses of its
    monitored objects:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 15

    And here's how to install it on two widgets:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 16

    The QShortcut class, for example, uses this technique to intercept
    shortcut key presses.

    \warning If you delete the receiver object in your eventFilter()
    function, be sure to return true. If you return false, Qt sends
    the event to the deleted object and the program will crash.

    Note that the filtering object must be in the same thread as this
    object. If \a filterObj is in a different thread, this function does
    nothing. If either \a filterObj or this object are moved to a different
    thread after calling this function, the event filter will not be
    called until both objects have the same thread affinity again (it
    is \e not removed).

    \sa removeEventFilter(), eventFilter(), event()
*/

void QObject::installEventFilter(QObject *obj)
{
    Q_D(QObject);
    if (!obj)
        return;
    if (d->threadData != obj->d_func()->threadData) {
        qWarning("QObject::installEventFilter(): Cannot filter events for objects in a different thread.");
        return;
    }

    // clean up unused items in the list
    d->eventFilters.removeAll((QObject*)0);
    d->eventFilters.removeAll(obj);
    d->eventFilters.prepend(obj);
}

/*!
    Removes an event filter object \a obj from this object. The
    request is ignored if such an event filter has not been installed.

    All event filters for this object are automatically removed when
    this object is destroyed.

    It is always safe to remove an event filter, even during event
    filter activation (i.e. from the eventFilter() function).

    \sa installEventFilter(), eventFilter(), event()
*/

void QObject::removeEventFilter(QObject *obj)
{
    Q_D(QObject);
    for (int i = 0; i < d->eventFilters.count(); ++i) {
        if (d->eventFilters.at(i) == obj)
            d->eventFilters[i] = 0;
    }
}


/*!
    \fn QObject::destroyed(QObject *obj)

    This signal is emitted immediately before the object \a obj is
    destroyed, and can not be blocked.

    All the objects's children are destroyed immediately after this
    signal is emitted.

    \sa deleteLater(), QPointer
*/

/*!
    Schedules this object for deletion.

    The object will be deleted when control returns to the event
    loop. If the event loop is not running when this function is
    called (e.g. deleteLater() is called on an object before
    QCoreApplication::exec()), the object will be deleted once the
    event loop is started.

    Note that entering and leaving a new event loop (e.g., by opening a modal
    dialog) will \e not perform the deferred deletion; for the object to be
    deleted, the control must return to the event loop from which
    deleteLater() was called.

    \bold{Note:} It is safe to call this function more than once; when the
    first deferred deletion event is delivered, any pending events for the
    object are removed from the event queue.

    \sa destroyed(), QPointer
*/
void QObject::deleteLater()
{
    QCoreApplication::postEvent(this, new QEvent(QEvent::DeferredDelete));
}

/*!
    \fn QString QObject::tr(const char *sourceText, const char *disambiguation, int n)
    \reentrant

    Returns a translated version of \a sourceText, optionally based on a
    \a disambiguation string and value of \a n for strings containing plurals;
    otherwise returns \a sourceText itself if no appropriate translated string
    is available.

    Example:
    \snippet mainwindows/sdi/mainwindow.cpp implicit tr context
    \dots

    If the same \a sourceText is used in different roles within the
    same context, an additional identifying string may be passed in
    \a disambiguation (0 by default). In Qt 4.4 and earlier, this was
    the preferred way to pass comments to translators.

    Example:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 17
    \dots

    See \l{Writing Source Code for Translation} for a detailed description of
    Qt's translation mechanisms in general, and the
    \l{Writing Source Code for Translation#Disambiguation}{Disambiguation}
    section for information on disambiguation.

    \warning This method is reentrant only if all translators are
    installed \e before calling this method. Installing or removing
    translators while performing translations is not supported. Doing
    so will probably result in crashes or other undesirable behavior.

    \sa trUtf8(), QApplication::translate(), QTextCodec::setCodecForTr(), {Internationalization with Qt}
*/

/*!
    \fn QString QObject::trUtf8(const char *sourceText, const char *disambiguation, int n)
    \reentrant

    Returns a translated version of \a sourceText, or
    QString::fromUtf8(\a sourceText) if there is no appropriate
    version. It is otherwise identical to tr(\a sourceText, \a
    disambiguation, \a n).

    Note that using the Utf8 variants of the translation functions
    is not required if \c CODECFORTR is already set to UTF-8 in the
    qmake project file and QTextCodec::setCodecForTr("UTF-8") is
    used.

    \warning This method is reentrant only if all translators are
    installed \e before calling this method. Installing or removing
    translators while performing translations is not supported. Doing
    so will probably result in crashes or other undesirable behavior.

    \warning For portability reasons, we recommend that you use
    escape sequences for specifying non-ASCII characters in string
    literals to trUtf8(). For example:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 20

    \sa tr(), QApplication::translate(), {Internationalization with Qt}
*/




/*****************************************************************************
  Signals and slots
 *****************************************************************************/


const int flagged_locations_count = 2;
static const char* flagged_locations[flagged_locations_count] = {0};

const char *qFlagLocation(const char *method)
{
    static int idx = 0;
    flagged_locations[idx] = method;
    idx = (idx+1) % flagged_locations_count;
    return method;
}

static int extract_code(const char *member)
{
    // extract code, ensure QMETHOD_CODE <= code <= QSIGNAL_CODE
    return (((int)(*member) - '0') & 0x3);
}

static const char * extract_location(const char *member)
{
    for (int i = 0; i < flagged_locations_count; ++i) {
        if (member == flagged_locations[i]) {
            // signature includes location information after the first null-terminator
            const char *location = member + qstrlen(member) + 1;
            if (*location != '\0')
                return location;
            return 0;
        }
    }
    return 0;
}

static bool check_signal_macro(const QObject *sender, const char *signal,
                                const char *func, const char *op)
{
    int sigcode = extract_code(signal);
    if (sigcode != QSIGNAL_CODE) {
        if (sigcode == QSLOT_CODE)
            qWarning("Object::%s: Attempt to %s non-signal %s::%s",
                     func, op, sender->metaObject()->className(), signal+1);
        else
            qWarning("Object::%s: Use the SIGNAL macro to %s %s::%s",
                     func, op, sender->metaObject()->className(), signal);
        return false;
    }
    return true;
}

static bool check_method_code(int code, const QObject *object,
                               const char *method, const char *func)
{
    if (code != QSLOT_CODE && code != QSIGNAL_CODE) {
        qWarning("Object::%s: Use the SLOT or SIGNAL macro to "
                 "%s %s::%s", func, func, object->metaObject()->className(), method);
        return false;
    }
    return true;
}

static void err_method_notfound(const QObject *object,
                                const char *method, const char *func)
{
    const char *type = "method";
    switch (extract_code(method)) {
        case QSLOT_CODE:   type = "slot";   break;
        case QSIGNAL_CODE: type = "signal"; break;
    }
    const char *loc = extract_location(method);
    if (strchr(method,')') == 0)                // common typing mistake
        qWarning("Object::%s: Parentheses expected, %s %s::%s%s%s",
                 func, type, object->metaObject()->className(), method+1,
                 loc ? " in ": "", loc ? loc : "");
    else
        qWarning("Object::%s: No such %s %s::%s%s%s",
                 func, type, object->metaObject()->className(), method+1,
                 loc ? " in ": "", loc ? loc : "");

}


static void err_info_about_objects(const char * func,
                                    const QObject * sender,
                                    const QObject * receiver)
{
    QString a = sender ? sender->objectName() : QString();
    QString b = receiver ? receiver->objectName() : QString();
    if (!a.isEmpty())
        qWarning("Object::%s:  (sender name:   '%s')", func, a.toLocal8Bit().data());
    if (!b.isEmpty())
        qWarning("Object::%s:  (receiver name: '%s')", func, b.toLocal8Bit().data());
}

/*!
    Returns a pointer to the object that sent the signal, if called in
    a slot activated by a signal; otherwise it returns 0. The pointer
    is valid only during the execution of the slot that calls this
    function from this object's thread context.

    The pointer returned by this function becomes invalid if the
    sender is destroyed, or if the slot is disconnected from the
    sender's signal.

    \warning This function violates the object-oriented principle of
    modularity. However, getting access to the sender might be useful
    when many signals are connected to a single slot.

    \warning As mentioned above, the return value of this function is
    not valid when the slot is called via a Qt::DirectConnection from
    a thread different from this object's thread. Do not use this
    function in this type of scenario.

    \sa senderSignalIndex(), QSignalMapper
*/

QObject *QObject::sender() const
{
    Q_D(const QObject);

    QMutexLocker locker(signalSlotLock(this));
    if (!d->currentSender)
        return 0;

    for (QObjectPrivate::Connection *c = d->senders; c; c = c->next) {
        if (c->sender == d->currentSender->sender)
            return d->currentSender->sender;
    }

    return 0;
}

/*!
    \since 4.8

    Returns the meta-method index of the signal that called the currently
    executing slot, which is a member of the class returned by sender().
    If called outside of a slot activated by a signal, -1 is returned.

    For signals with default parameters, this function will always return
    the index with all parameters, regardless of which was used with
    connect(). For example, the signal \c {destroyed(QObject *obj = 0)}
    will have two different indexes (with and without the parameter), but
    this function will always return the index with a parameter. This does
    not apply when overloading signals with different parameters.

    \warning This function violates the object-oriented principle of
    modularity. However, getting access to the signal index might be useful
    when many signals are connected to a single slot.

    \warning The return value of this function is not valid when the slot
    is called via a Qt::DirectConnection from a thread different from this
    object's thread. Do not use this function in this type of scenario.

    \sa sender(), QMetaObject::indexOfSignal(), QMetaObject::method()
*/

int QObject::senderSignalIndex() const
{
    Q_D(const QObject);

    QMutexLocker locker(signalSlotLock(this));
    if (!d->currentSender)
        return -1;

    for (QObjectPrivate::Connection *c = d->senders; c; c = c->next) {
        if (c->sender == d->currentSender->sender)
            return d->currentSender->signal;
    }

    return -1;
}

/*!
    Returns the number of receivers connected to the \a signal.

    Since both slots and signals can be used as receivers for signals,
    and the same connections can be made many times, the number of
    receivers is the same as the number of connections made from this
    signal.

    When calling this function, you can use the \c SIGNAL() macro to
    pass a specific signal:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 21

    As the code snippet above illustrates, you can use this function
    to avoid emitting a signal that nobody listens to.

    \warning This function violates the object-oriented principle of
    modularity. However, it might be useful when you need to perform
    expensive initialization only if something is connected to a
    signal.
*/

int QObject::receivers(const char *signal) const
{
    Q_D(const QObject);
    int receivers = 0;
    if (signal) {
        QByteArray signal_name = QMetaObject::normalizedSignature(signal);
        signal = signal_name;
#ifndef QT_NO_DEBUG
        if (!check_signal_macro(this, signal, "receivers", "bind"))
            return 0;
#endif
        signal++; // skip code
        int signal_index = d->signalIndex(signal);
        if (signal_index < 0) {
#ifndef QT_NO_DEBUG
            err_method_notfound(this, signal-1, "receivers");
#endif
            return false;
        }

        Q_D(const QObject);
        QMutexLocker locker(signalSlotLock(this));
        if (d->connectionLists) {
            if (signal_index < d->connectionLists->count()) {
                const QObjectPrivate::Connection *c =
                    d->connectionLists->at(signal_index).first;
                while (c) {
                    receivers += c->receiver ? 1 : 0;
                    c = c->nextConnectionList;
                }
            }
        }
    }
    return receivers;
}

/*!
    \internal

    This helper function calculates signal and method index for the given
    member in the specified class.

    \list
    \o If member.mobj is 0 then both signalIndex and methodIndex are set to -1.

    \o If specified member is not a member of obj instance class (or one of
    its parent classes) then both signalIndex and methodIndex are set to -1.
    \endlist

    This function is used by QObject::connect and QObject::disconnect which
    are working with QMetaMethod.

    \a signalIndex is set to the signal index of member. If the member
    specified is not signal this variable is set to -1.

    \a methodIndex is set to the method index of the member. If the
    member is not a method of the object specified by the \a obj argument this
    variable is set to -1.
*/
void QMetaObjectPrivate::memberIndexes(const QObject *obj,
                                       const QMetaMethod &member,
                                       int *signalIndex, int *methodIndex)
{
    *signalIndex = -1;
    *methodIndex = -1;
    if (!obj || !member.mobj)
        return;
    const QMetaObject *m = obj->metaObject();
    // Check that member is member of obj class
    while (m != 0 && m != member.mobj)
        m = m->d.superdata;
    if (!m)
        return;
    *signalIndex = *methodIndex = (member.handle - get(member.mobj)->methodData)/5;

    int signalOffset;
    int methodOffset;
    computeOffsets(m, &signalOffset, &methodOffset);

    *methodIndex += methodOffset;
    if (member.methodType() == QMetaMethod::Signal) {
        *signalIndex = originalClone(m, *signalIndex);
        *signalIndex += signalOffset;
    } else {
        *signalIndex = -1;
    }
}

static inline void check_and_warn_compat(const QMetaObject *sender, const QMetaMethod &signal,
                                         const QMetaObject *receiver, const QMetaMethod &method)
{
    if (signal.attributes() & QMetaMethod::Compatibility) {
        if (!(method.attributes() & QMetaMethod::Compatibility))
            qWarning("QObject::connect: Connecting from COMPAT signal (%s::%s)",
                     sender->className(), signal.signature());
    } else if ((method.attributes() & QMetaMethod::Compatibility) &&
               method.methodType() == QMetaMethod::Signal) {
        qWarning("QObject::connect: Connecting from %s::%s to COMPAT slot (%s::%s)",
                 sender->className(), signal.signature(),
                 receiver->className(), method.signature());
    }
}

/*!
    \threadsafe

    Creates a connection of the given \a type from the \a signal in
    the \a sender object to the \a method in the \a receiver object.
    Returns true if the connection succeeds; otherwise returns false.

    You must use the \c SIGNAL() and \c SLOT() macros when specifying
    the \a signal and the \a method, for example:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 22

    This example ensures that the label always displays the current
    scroll bar value. Note that the signal and slots parameters must not
    contain any variable names, only the type. E.g. the following would
    not work and return false:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 23

    A signal can also be connected to another signal:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 24

    In this example, the \c MyWidget constructor relays a signal from
    a private member variable, and makes it available under a name
    that relates to \c MyWidget.

    A signal can be connected to many slots and signals. Many signals
    can be connected to one slot.

    If a signal is connected to several slots, the slots are activated
    in the same order as the order the connection was made, when the
    signal is emitted.

    The function returns true if it successfully connects the signal
    to the slot. It will return false if it cannot create the
    connection, for example, if QObject is unable to verify the
    existence of either \a signal or \a method, or if their signatures
    aren't compatible.

    By default, a signal is emitted for every connection you make;
    two signals are emitted for duplicate connections. You can break
    all of these connections with a single disconnect() call.
    If you pass the Qt::UniqueConnection \a type, the connection will only
    be made if it is not a duplicate. If there is already a duplicate
    (exact same signal to the exact same slot on the same objects),
    the connection will fail and connect will return false.

    The optional \a type parameter describes the type of connection
    to establish. In particular, it determines whether a particular
    signal is delivered to a slot immediately or queued for delivery
    at a later time. If the signal is queued, the parameters must be
    of types that are known to Qt's meta-object system, because Qt
    needs to copy the arguments to store them in an event behind the
    scenes. If you try to use a queued connection and get the error
    message

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 25

    call qRegisterMetaType() to register the data type before you
    establish the connection.

    \sa disconnect(), sender(), qRegisterMetaType(), Q_DECLARE_METATYPE()
*/

bool QObject::connect(const QObject *sender, const char *signal,
                      const QObject *receiver, const char *method,
                      Qt::ConnectionType type)
{
    {
        const void *cbdata[] = { sender, signal, receiver, method, &type };
        if (QInternal::activateCallbacks(QInternal::ConnectCallback, (void **) cbdata))
            return true;
    }

#ifndef QT_NO_DEBUG
    bool warnCompat = true;
#endif
    if (type == Qt::AutoCompatConnection) {
        type = Qt::AutoConnection;
#ifndef QT_NO_DEBUG
        warnCompat = false;
#endif
    }

    if (sender == 0 || receiver == 0 || signal == 0 || method == 0) {
        qWarning("QObject::connect: Cannot connect %s::%s to %s::%s",
                 sender ? sender->metaObject()->className() : "(null)",
                 (signal && *signal) ? signal+1 : "(null)",
                 receiver ? receiver->metaObject()->className() : "(null)",
                 (method && *method) ? method+1 : "(null)");
        return false;
    }
    QByteArray tmp_signal_name;

    if (!check_signal_macro(sender, signal, "connect", "bind"))
        return false;
    const QMetaObject *smeta = sender->metaObject();
    const char *signal_arg = signal;
    ++signal; //skip code
    int signal_index = QMetaObjectPrivate::indexOfSignalRelative(&smeta, signal, false);
    if (signal_index < 0) {
        // check for normalized signatures
        tmp_signal_name = QMetaObject::normalizedSignature(signal - 1);
        signal = tmp_signal_name.constData() + 1;

        smeta = sender->metaObject();
        signal_index = QMetaObjectPrivate::indexOfSignalRelative(&smeta, signal, false);
    }
    if (signal_index < 0) {
        // re-use tmp_signal_name and signal from above

        smeta = sender->metaObject();
        signal_index = QMetaObjectPrivate::indexOfSignalRelative(&smeta, signal, true);
    }
    if (signal_index < 0) {
        err_method_notfound(sender, signal_arg, "connect");
        err_info_about_objects("connect", sender, receiver);
        return false;
    }
    signal_index = QMetaObjectPrivate::originalClone(smeta, signal_index);
    int signalOffset, methodOffset;
    computeOffsets(smeta, &signalOffset, &methodOffset);
    int signal_absolute_index = signal_index + methodOffset;
    signal_index += signalOffset;

    QByteArray tmp_method_name;
    int membcode = extract_code(method);

    if (!check_method_code(membcode, receiver, method, "connect"))
        return false;
    const char *method_arg = method;
    ++method; // skip code

    const QMetaObject *rmeta = receiver->metaObject();
    int method_index_relative = -1;
    switch (membcode) {
    case QSLOT_CODE:
        method_index_relative = QMetaObjectPrivate::indexOfSlotRelative(&rmeta, method, false);
        break;
    case QSIGNAL_CODE:
        method_index_relative = QMetaObjectPrivate::indexOfSignalRelative(&rmeta, method, false);
        break;
    }

    if (method_index_relative < 0) {
        // check for normalized methods
        tmp_method_name = QMetaObject::normalizedSignature(method);
        method = tmp_method_name.constData();

        // rmeta may have been modified above
        rmeta = receiver->metaObject();
        switch (membcode) {
        case QSLOT_CODE:
            method_index_relative = QMetaObjectPrivate::indexOfSlotRelative(&rmeta, method, false);
            if (method_index_relative < 0)
                method_index_relative = QMetaObjectPrivate::indexOfSlotRelative(&rmeta, method, true);
            break;
        case QSIGNAL_CODE:
            method_index_relative = QMetaObjectPrivate::indexOfSignalRelative(&rmeta, method, false);
            if (method_index_relative < 0)
                method_index_relative = QMetaObjectPrivate::indexOfSignalRelative(&rmeta, method, true);
            break;
        }
    }

    if (method_index_relative < 0) {
        err_method_notfound(receiver, method_arg, "connect");
        err_info_about_objects("connect", sender, receiver);
        return false;
    }

    if (!QMetaObject::checkConnectArgs(signal, method)) {
        qWarning("QObject::connect: Incompatible sender/receiver arguments"
                 "\n        %s::%s --> %s::%s",
                 sender->metaObject()->className(), signal,
                 receiver->metaObject()->className(), method);
        return false;
    }

    int *types = 0;
    if ((type == Qt::QueuedConnection)
            && !(types = queuedConnectionTypes(smeta->method(signal_absolute_index).parameterTypes())))
        return false;

#ifndef QT_NO_DEBUG
    if (warnCompat) {
        QMetaMethod smethod = smeta->method(signal_absolute_index);
        QMetaMethod rmethod = rmeta->method(method_index_relative + rmeta->methodOffset());
        check_and_warn_compat(smeta, smethod, rmeta, rmethod);
    }
#endif
    if (!QMetaObjectPrivate::connect(sender, signal_index, receiver, method_index_relative, rmeta ,type, types))
        return false;
    const_cast<QObject*>(sender)->connectNotify(signal - 1);
    return true;
}

/*!
    \since 4.8

    Creates a connection of the given \a type from the \a signal in
    the \a sender object to the \a method in the \a receiver object.
    Returns true if the connection succeeds; otherwise returns false.

    This function works in the same way as
    connect(const QObject *sender, const char *signal,
            const QObject *receiver, const char *method,
            Qt::ConnectionType type)
    but it uses QMetaMethod to specify signal and method.

    \sa connect(const QObject *sender, const char *signal,
                const QObject *receiver, const char *method,
                Qt::ConnectionType type)
 */
bool QObject::connect(const QObject *sender, const QMetaMethod &signal,
                      const QObject *receiver, const QMetaMethod &method,
                      Qt::ConnectionType type)
{
#ifndef QT_NO_DEBUG
    bool warnCompat = true;
#endif
    if (type == Qt::AutoCompatConnection) {
        type = Qt::AutoConnection;
#ifndef QT_NO_DEBUG
        warnCompat = false;
#endif
    }

    if (sender == 0
            || receiver == 0
            || signal.methodType() != QMetaMethod::Signal
            || method.methodType() == QMetaMethod::Constructor) {
        qWarning("QObject::connect: Cannot connect %s::%s to %s::%s",
                 sender ? sender->metaObject()->className() : "(null)",
                 signal.signature(),
                 receiver ? receiver->metaObject()->className() : "(null)",
                 method.signature() );
        return false;
    }

    QVarLengthArray<char> signalSignature;
    QObjectPrivate::signalSignature(signal, &signalSignature);

    {
        QByteArray methodSignature;
        methodSignature.reserve(qstrlen(method.signature())+1);
        methodSignature.append((char)(method.methodType() == QMetaMethod::Slot ? QSLOT_CODE
                                    : method.methodType() == QMetaMethod::Signal ? QSIGNAL_CODE : 0  + '0'));
        methodSignature.append(method.signature());
        const void *cbdata[] = { sender, signalSignature.constData(), receiver, methodSignature.constData(), &type };
        if (QInternal::activateCallbacks(QInternal::ConnectCallback, (void **) cbdata))
            return true;
    }


    int signal_index;
    int method_index;
    {
        int dummy;
        QMetaObjectPrivate::memberIndexes(sender, signal, &signal_index, &dummy);
        QMetaObjectPrivate::memberIndexes(receiver, method, &dummy, &method_index);
    }

    const QMetaObject *smeta = sender->metaObject();
    const QMetaObject *rmeta = receiver->metaObject();
    if (signal_index == -1) {
        qWarning("QObject::connect: Can't find signal %s on instance of class %s",
                 signal.signature(), smeta->className());
        return false;
    }
    if (method_index == -1) {
        qWarning("QObject::connect: Can't find method %s on instance of class %s",
                 method.signature(), rmeta->className());
        return false;
    }
    
    if (!QMetaObject::checkConnectArgs(signal.signature(), method.signature())) {
        qWarning("QObject::connect: Incompatible sender/receiver arguments"
                 "\n        %s::%s --> %s::%s",
                 smeta->className(), signal.signature(),
                 rmeta->className(), method.signature());
        return false;
    }

    int *types = 0;
    if ((type == Qt::QueuedConnection)
            && !(types = queuedConnectionTypes(signal.parameterTypes())))
        return false;

#ifndef QT_NO_DEBUG
    if (warnCompat)
        check_and_warn_compat(smeta, signal, rmeta, method);
#endif
    if (!QMetaObjectPrivate::connect(sender, signal_index, receiver, method_index, 0, type, types))
        return false;

    const_cast<QObject*>(sender)->connectNotify(signalSignature.constData());
    return true;
}

/*!
    \fn bool QObject::connect(const QObject *sender, const char *signal, const char *method, Qt::ConnectionType type) const
    \overload connect()
    \threadsafe

    Connects \a signal from the \a sender object to this object's \a
    method.

    Equivalent to connect(\a sender, \a signal, \c this, \a method, \a type).

    Every connection you make emits a signal, so duplicate connections emit
    two signals. You can break a connection using disconnect().

    \sa disconnect()
*/

/*!
    \threadsafe

    Disconnects \a signal in object \a sender from \a method in object
    \a receiver. Returns true if the connection is successfully broken;
    otherwise returns false.

    A signal-slot connection is removed when either of the objects
    involved are destroyed.

    disconnect() is typically used in three ways, as the following
    examples demonstrate.
    \list 1
    \i Disconnect everything connected to an object's signals:

       \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 26

       equivalent to the non-static overloaded function

       \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 27

    \i Disconnect everything connected to a specific signal:

       \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 28

       equivalent to the non-static overloaded function

       \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 29

    \i Disconnect a specific receiver:

       \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 30

       equivalent to the non-static overloaded function

       \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 31

    \endlist

    0 may be used as a wildcard, meaning "any signal", "any receiving
    object", or "any slot in the receiving object", respectively.

    The \a sender may never be 0. (You cannot disconnect signals from
    more than one object in a single call.)

    If \a signal is 0, it disconnects \a receiver and \a method from
    any signal. If not, only the specified signal is disconnected.

    If \a receiver is 0, it disconnects anything connected to \a
    signal. If not, slots in objects other than \a receiver are not
    disconnected.

    If \a method is 0, it disconnects anything that is connected to \a
    receiver. If not, only slots named \a method will be disconnected,
    and all other slots are left alone. The \a method must be 0 if \a
    receiver is left out, so you cannot disconnect a
    specifically-named slot on all objects.

    \sa connect()
*/
bool QObject::disconnect(const QObject *sender, const char *signal,
                         const QObject *receiver, const char *method)
{
    if (sender == 0 || (receiver == 0 && method != 0)) {
        qWarning("Object::disconnect: Unexpected null parameter");
        return false;
    }

    {
        const void *cbdata[] = { sender, signal, receiver, method };
        if (QInternal::activateCallbacks(QInternal::DisconnectCallback, (void **) cbdata))
            return true;
    }

    const char *signal_arg = signal;
    QByteArray signal_name;
    bool signal_found = false;
    if (signal) {
        QT_TRY {
            signal_name = QMetaObject::normalizedSignature(signal);
            signal = signal_name.constData();
        } QT_CATCH (const std::bad_alloc &) {
            // if the signal is already normalized, we can continue.
            if (sender->metaObject()->indexOfSignal(signal + 1) == -1)
                QT_RETHROW;
        }

        if (!check_signal_macro(sender, signal, "disconnect", "unbind"))
            return false;
        signal++; // skip code
    }

    QByteArray method_name;
    const char *method_arg = method;
    int membcode = -1;
    bool method_found = false;
    if (method) {
        QT_TRY {
            method_name = QMetaObject::normalizedSignature(method);
            method = method_name.constData();
        } QT_CATCH(const std::bad_alloc &) {
            // if the method is already normalized, we can continue.
            if (receiver->metaObject()->indexOfMethod(method + 1) == -1)
                QT_RETHROW;
        }

        membcode = extract_code(method);
        if (!check_method_code(membcode, receiver, method, "disconnect"))
            return false;
        method++; // skip code
    }

    /* We now iterate through all the sender's and receiver's meta
     * objects in order to also disconnect possibly shadowed signals
     * and slots with the same signature.
    */
    bool res = false;
    const QMetaObject *smeta = sender->metaObject();
    do {
        int signal_index = -1;
        if (signal) {
            signal_index = QMetaObjectPrivate::indexOfSignalRelative(&smeta, signal, false);
            if (signal_index < 0)
                signal_index = QMetaObjectPrivate::indexOfSignalRelative(&smeta, signal, true);
            if (signal_index < 0)
                break;
            signal_index = QMetaObjectPrivate::originalClone(smeta, signal_index);
            int signalOffset, methodOffset;
            computeOffsets(smeta, &signalOffset, &methodOffset);
            signal_index += signalOffset;
            signal_found = true;
        }

        if (!method) {
            res |= QMetaObjectPrivate::disconnect(sender, signal_index, receiver, -1);
        } else {
            const QMetaObject *rmeta = receiver->metaObject();
            do {
                int method_index = rmeta->indexOfMethod(method);
                if (method_index >= 0)
                    while (method_index < rmeta->methodOffset())
                            rmeta = rmeta->superClass();
                if (method_index < 0)
                    break;
                res |= QMetaObjectPrivate::disconnect(sender, signal_index, receiver, method_index);
                method_found = true;
            } while ((rmeta = rmeta->superClass()));
        }
    } while (signal && (smeta = smeta->superClass()));

    if (signal && !signal_found) {
        err_method_notfound(sender, signal_arg, "disconnect");
        err_info_about_objects("disconnect", sender, receiver);
    } else if (method && !method_found) {
        err_method_notfound(receiver, method_arg, "disconnect");
        err_info_about_objects("disconnect", sender, receiver);
    }
    if (res)
        const_cast<QObject*>(sender)->disconnectNotify(signal ? (signal - 1) : 0);
    return res;
}

/*!
    \since 4.8

    Disconnects \a signal in object \a sender from \a method in object
    \a receiver. Returns true if the connection is successfully broken;
    otherwise returns false.

    This function provides the same possibilities like
    disconnect(const QObject *sender, const char *signal, const QObject *receiver, const char *method)
    but uses QMetaMethod to represent the signal and the method to be disconnected.

    Additionally this function returnsfalse and no signals and slots disconnected
    if:
    \list 1

        \i \a signal is not a member of sender class or one of its parent classes.

        \i \a method is not a member of receiver class or one of its parent classes.

        \i \a signal instance represents not a signal.

    \endlist

    QMetaMethod() may be used as wildcard in the meaning "any signal" or "any slot in receiving object".
    In the same way 0 can be used for \a receiver in the meaning "any receiving object". In this case
    method should also be QMetaMethod(). \a sender parameter should be never 0.

    \sa disconnect(const QObject *sender, const char *signal, const QObject *receiver, const char *method)
 */
bool QObject::disconnect(const QObject *sender, const QMetaMethod &signal,
                         const QObject *receiver, const QMetaMethod &method)
{
    if (sender == 0 || (receiver == 0 && method.mobj != 0)) {
        qWarning("Object::disconnect: Unexpected null parameter");
        return false;
    }
    if (signal.mobj) {
        if(signal.methodType() != QMetaMethod::Signal) {
            qWarning("Object::%s: Attempt to %s non-signal %s::%s",
                     "disconnect","unbind",
                     sender->metaObject()->className(), signal.signature());
            return false;
        }
    }
    if (method.mobj) {
        if(method.methodType() == QMetaMethod::Constructor) {
            qWarning("QObject::disconect: cannot use constructor as argument %s::%s",
                     receiver->metaObject()->className(), method.signature());
            return false;
        }
    }

    QVarLengthArray<char> signalSignature;
    if (signal.mobj)
        QObjectPrivate::signalSignature(signal, &signalSignature);

    {
        QByteArray methodSignature;
        if (method.mobj) {
            methodSignature.reserve(qstrlen(method.signature())+1);
            methodSignature.append((char)(method.methodType() == QMetaMethod::Slot ? QSLOT_CODE
                                        : method.methodType() == QMetaMethod::Signal ? QSIGNAL_CODE : 0  + '0'));
            methodSignature.append(method.signature());
        }
        const void *cbdata[] = { sender, signal.mobj ? signalSignature.constData() : 0,
                                 receiver, method.mobj ? methodSignature.constData() : 0 };
        if (QInternal::activateCallbacks(QInternal::DisconnectCallback, (void **) cbdata))
            return true;
    }

    int signal_index;
    int method_index;
    {
        int dummy;
        QMetaObjectPrivate::memberIndexes(sender, signal, &signal_index, &dummy);
        QMetaObjectPrivate::memberIndexes(receiver, method, &dummy, &method_index);
    }
    // If we are here sender is not null. If signal is not null while signal_index
    // is -1 then this signal is not a member of sender.
    if (signal.mobj && signal_index == -1) {
        qWarning("QObject::disconect: signal %s not found on class %s",
                 signal.signature(), sender->metaObject()->className());
        return false;
    }
    // If this condition is true then method is not a member of receeiver.
    if (receiver && method.mobj && method_index == -1) {
        qWarning("QObject::disconect: method %s not found on class %s",
                 method.signature(), receiver->metaObject()->className());
        return false;
    }

    if (!QMetaObjectPrivate::disconnect(sender, signal_index, receiver, method_index))
        return false;

    const_cast<QObject*>(sender)->disconnectNotify(method.mobj ? signalSignature.constData() : 0);
    return true;
}

/*!
    \threadsafe

    \fn bool QObject::disconnect(const char *signal, const QObject *receiver, const char *method)
    \overload disconnect()

    Disconnects \a signal from \a method of \a receiver.

    A signal-slot connection is removed when either of the objects
    involved are destroyed.
*/

/*!
    \fn bool QObject::disconnect(const QObject *receiver, const char *method)
    \overload disconnect()

    Disconnects all signals in this object from \a receiver's \a
    method.

    A signal-slot connection is removed when either of the objects
    involved are destroyed.
*/


/*!
    \fn void QObject::connectNotify(const char *signal)

    This virtual function is called when something has been connected
    to \a signal in this object.

    If you want to compare \a signal with a specific signal, use
    QLatin1String and the \c SIGNAL() macro as follows:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 32

    If the signal contains multiple parameters or parameters that
    contain spaces, call QMetaObject::normalizedSignature() on
    the result of the \c SIGNAL() macro.

    \warning This function violates the object-oriented principle of
    modularity. However, it might be useful when you need to perform
    expensive initialization only if something is connected to a
    signal.

    \sa connect(), disconnectNotify()
*/

void QObject::connectNotify(const char *)
{
}

/*!
    \fn void QObject::disconnectNotify(const char *signal)

    This virtual function is called when something has been
    disconnected from \a signal in this object.

    See connectNotify() for an example of how to compare
    \a signal with a specific signal.

    \warning This function violates the object-oriented principle of
    modularity. However, it might be useful for optimizing access to
    expensive resources.

    \sa disconnect(), connectNotify()
*/

void QObject::disconnectNotify(const char *)
{
}

/* \internal
    convert a signal index from the method range to the signal range
 */
static int methodIndexToSignalIndex(const QMetaObject *metaObject, int signal_index)
{
    if (signal_index < 0)
        return signal_index;
    while (metaObject && metaObject->methodOffset() > signal_index)
        metaObject = metaObject->superClass();

    if (metaObject) {
        int signalOffset, methodOffset;
        computeOffsets(metaObject, &signalOffset, &methodOffset);
        if (signal_index < metaObject->methodCount())
            signal_index = QMetaObjectPrivate::originalClone(metaObject, signal_index - methodOffset) + signalOffset;
        else
            signal_index = signal_index - methodOffset + signalOffset;
    }
    return signal_index;
}

/*!\internal
   \a types is a 0-terminated vector of meta types for queued
   connections.

   if \a signal_index is -1, then we effectively connect *all* signals
   from the sender to the receiver's slot
 */
bool QMetaObject::connect(const QObject *sender, int signal_index,
                          const QObject *receiver, int method_index, int type, int *types)
{
    signal_index = methodIndexToSignalIndex(sender->metaObject(), signal_index);
    return QMetaObjectPrivate::connect(sender, signal_index,
                                       receiver, method_index,
                                       0, //FIXME, we could speed this connection up by computing the relative index
                                       type, types);
}

/*! \internal
   Same as the QMetaObject::connect, but \a signal_index must be the result of QObjectPrivate::signalIndex

    method_index is relative to the rmeta metaobject, if rmeta is null, then it is absolute index
 */
bool QMetaObjectPrivate::connect(const QObject *sender, int signal_index,
                                 const QObject *receiver, int method_index,
                                 const QMetaObject *rmeta, int type, int *types)
{
    QObject *s = const_cast<QObject *>(sender);
    QObject *r = const_cast<QObject *>(receiver);

    int method_offset = rmeta ? rmeta->methodOffset() : 0;
    QObjectPrivate::StaticMetaCallFunction callFunction =
        (rmeta && QMetaObjectPrivate::get(rmeta)->revision >= 6 && rmeta->d.extradata)
        ? reinterpret_cast<const QMetaObjectExtraData *>(rmeta->d.extradata)->static_metacall : 0;

    QOrderedMutexLocker locker(signalSlotLock(sender),
                               signalSlotLock(receiver));

    if (type & Qt::UniqueConnection) {
        QObjectConnectionListVector *connectionLists = QObjectPrivate::get(s)->connectionLists;
        if (connectionLists && connectionLists->count() > signal_index) {
            const QObjectPrivate::Connection *c2 =
                (*connectionLists)[signal_index].first;

            int method_index_absolute = method_index + method_offset;

            while (c2) {
                if (c2->receiver == receiver && c2->method() == method_index_absolute)
                    return false;
                c2 = c2->nextConnectionList;
            }
        }
        type &= Qt::UniqueConnection - 1;
    }

    QObjectPrivate::Connection *c = new QObjectPrivate::Connection;
    c->sender = s;
    c->receiver = r;
    c->method_relative = method_index;
    c->method_offset = method_offset;
    c->connectionType = type;
    c->argumentTypes = types;
    c->nextConnectionList = 0;
    c->callFunction = callFunction;

    QT_TRY {
        QObjectPrivate::get(s)->addConnection(signal_index, c);
    } QT_CATCH(...) {
        delete c;
        QT_RETHROW;
    }

    c->prev = &(QObjectPrivate::get(r)->senders);
    c->next = *c->prev;
    *c->prev = c;
    if (c->next)
        c->next->prev = &c->next;

    QObjectPrivate *const sender_d = QObjectPrivate::get(s);
    if (signal_index < 0) {
        sender_d->connectedSignals[0] = sender_d->connectedSignals[1] = ~0;
    } else if (signal_index < (int)sizeof(sender_d->connectedSignals) * 8) {
        sender_d->connectedSignals[signal_index >> 5] |= (1 << (signal_index & 0x1f));
    }

    return true;
}

/*!\internal
 */
bool QMetaObject::disconnect(const QObject *sender, int signal_index,
                             const QObject *receiver, int method_index)
{
    signal_index = methodIndexToSignalIndex(sender->metaObject(), signal_index);
    return QMetaObjectPrivate::disconnect(sender, signal_index,
                                          receiver, method_index);
}

/*!\internal

Disconnect a single signal connection.  If QMetaObject::connect() has been called 
multiple times for the same sender, signal_index, receiver and method_index only 
one of these connections will be removed.
 */
bool QMetaObject::disconnectOne(const QObject *sender, int signal_index,
                                const QObject *receiver, int method_index)
{
    signal_index = methodIndexToSignalIndex(sender->metaObject(), signal_index);
    return QMetaObjectPrivate::disconnect(sender, signal_index,
                                          receiver, method_index,
                                          QMetaObjectPrivate::DisconnectOne);
}

/*! \internal
    Helper function to remove the connection from the senders list and setting the receivers to 0
 */
bool QMetaObjectPrivate::disconnectHelper(QObjectPrivate::Connection *c,
                                          const QObject *receiver, int method_index,
                                          QMutex *senderMutex, DisconnectType disconnectType)
{
    bool success = false;
    while (c) {
        if (c->receiver
            && (receiver == 0 || (c->receiver == receiver
                           && (method_index < 0 || c->method() == method_index)))) {
            bool needToUnlock = false;
            QMutex *receiverMutex = 0;
            if (!receiver) {
                receiverMutex = signalSlotLock(c->receiver);
                // need to relock this receiver and sender in the correct order
                needToUnlock = QOrderedMutexLocker::relock(senderMutex, receiverMutex);
            }
            if (c->receiver) {
                *c->prev = c->next;
                if (c->next)
                    c->next->prev = c->prev;
            }

            if (needToUnlock)
                receiverMutex->unlockInline();

            c->receiver = 0;

            success = true;

            if (disconnectType == DisconnectOne)
                return success;
        }
        c = c->nextConnectionList;
    }
    return success;
}

/*! \internal
    Same as the QMetaObject::disconnect, but \a signal_index must be the result of QObjectPrivate::signalIndex
 */
bool QMetaObjectPrivate::disconnect(const QObject *sender, int signal_index,
                                    const QObject *receiver, int method_index,
                                    DisconnectType disconnectType)
{
    if (!sender)
        return false;

    QObject *s = const_cast<QObject *>(sender);

    QMutex *senderMutex = signalSlotLock(sender);
    QMutex *receiverMutex = receiver ? signalSlotLock(receiver) : 0;
    QOrderedMutexLocker locker(senderMutex, receiverMutex);

    QObjectConnectionListVector *connectionLists = QObjectPrivate::get(s)->connectionLists;
    if (!connectionLists)
        return false;

    // prevent incoming connections changing the connectionLists while unlocked
    ++connectionLists->inUse;

    bool success = false;
    if (signal_index < 0) {
        // remove from all connection lists
        for (signal_index = -1; signal_index < connectionLists->count(); ++signal_index) {
            QObjectPrivate::Connection *c =
                (*connectionLists)[signal_index].first;
            if (disconnectHelper(c, receiver, method_index, senderMutex, disconnectType)) {
                success = true;
                connectionLists->dirty = true;
            }
        }
    } else if (signal_index < connectionLists->count()) {
        QObjectPrivate::Connection *c =
            (*connectionLists)[signal_index].first;
        if (disconnectHelper(c, receiver, method_index, senderMutex, disconnectType)) {
            success = true;
            connectionLists->dirty = true;
        }
    }

    --connectionLists->inUse;
    Q_ASSERT(connectionLists->inUse >= 0);
    if (connectionLists->orphaned && !connectionLists->inUse)
        delete connectionLists;

    return success;
}

/*!
    \fn void QMetaObject::connectSlotsByName(QObject *object)

    Searches recursively for all child objects of the given \a object, and connects
    matching signals from them to slots of \a object that follow the following form:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 33

    Let's assume our object has a child object of type QPushButton with
    the \l{QObject::objectName}{object name} \c{button1}. The slot to catch the
    button's \c{clicked()} signal would be:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 34

    \sa QObject::setObjectName()
 */
void QMetaObject::connectSlotsByName(QObject *o)
{
    if (!o)
        return;
    const QMetaObject *mo = o->metaObject();
    Q_ASSERT(mo);
    const QObjectList list = o->findChildren<QObject *>(QString());
    for (int i = 0; i < mo->methodCount(); ++i) {
        const char *slot = mo->method(i).signature();
        Q_ASSERT(slot);
        if (slot[0] != 'o' || slot[1] != 'n' || slot[2] != '_')
            continue;
        bool foundIt = false;
        for(int j = 0; j < list.count(); ++j) {
            const QObject *co = list.at(j);
            QByteArray objName = co->objectName().toAscii();
            int len = objName.length();
            if (!len || qstrncmp(slot + 3, objName.data(), len) || slot[len+3] != '_')
                continue;
            int sigIndex = co->d_func()->signalIndex(slot + len + 4);
            if (sigIndex < 0) { // search for compatible signals
                const QMetaObject *smo = co->metaObject();
                int slotlen = qstrlen(slot + len + 4) - 1;
                for (int k = 0; k < co->metaObject()->methodCount(); ++k) {
                    QMetaMethod method = smo->method(k);
                    if (method.methodType() != QMetaMethod::Signal)
                        continue;

                    if (!qstrncmp(method.signature(), slot + len + 4, slotlen)) {
                        int signalOffset, methodOffset;
                        computeOffsets(method.enclosingMetaObject(), &signalOffset, &methodOffset);
                        sigIndex = k + - methodOffset + signalOffset;
                        break;
                    }
                }
            }
            if (sigIndex < 0)
                continue;
            if (QMetaObjectPrivate::connect(co, sigIndex, o, i)) {
                foundIt = true;
                break;
            }
        }
        if (foundIt) {
            // we found our slot, now skip all overloads
            while (mo->method(i + 1).attributes() & QMetaMethod::Cloned)
                  ++i;
        } else if (!(mo->method(i).attributes() & QMetaMethod::Cloned)) {
            qWarning("QMetaObject::connectSlotsByName: No matching signal for %s", slot);
        }
    }
}

static void queued_activate(QObject *sender, int signal, QObjectPrivate::Connection *c, void **argv)
{
    if (!c->argumentTypes && c->argumentTypes != &DIRECT_CONNECTION_ONLY) {
        QMetaMethod m = sender->metaObject()->method(signal);
        int *tmp = queuedConnectionTypes(m.parameterTypes());
        if (!tmp) // cannot queue arguments
            tmp = &DIRECT_CONNECTION_ONLY;
        if (!c->argumentTypes.testAndSetOrdered(0, tmp)) {
            if (tmp != &DIRECT_CONNECTION_ONLY)
                delete [] tmp;
        }
    }
    if (c->argumentTypes == &DIRECT_CONNECTION_ONLY) // cannot activate
        return;
    int nargs = 1; // include return type
    while (c->argumentTypes[nargs-1])
        ++nargs;
    int *types = (int *) qMalloc(nargs*sizeof(int));
    Q_CHECK_PTR(types);
    void **args = (void **) qMalloc(nargs*sizeof(void *));
    Q_CHECK_PTR(args);
    types[0] = 0; // return type
    args[0] = 0; // return value
    for (int n = 1; n < nargs; ++n)
        args[n] = QMetaType::construct((types[n] = c->argumentTypes[n-1]), argv[n]);
    QCoreApplication::postEvent(c->receiver, new QMetaCallEvent(c->method_offset,
                                                                c->method_relative,
                                                                c->callFunction,
                                                                sender, signal, nargs,
                                                                types, args));
}


/*!\internal
   \obsolete.
   Used to be called from QMetaObject::activate(QObject *, QMetaObject *, int, int, void **) before Qt 4.6
 */
void QMetaObject::activate(QObject *sender, int from_signal_index, int to_signal_index, void **argv)
{
    Q_UNUSED(to_signal_index);
    activate(sender, from_signal_index, argv);
}

/*!\internal
 */
void QMetaObject::activate(QObject *sender, const QMetaObject *m, int local_signal_index,
                           void **argv)
{
    int signalOffset;
    int methodOffset;
    computeOffsets(m, &signalOffset, &methodOffset);

    int signal_index = signalOffset + local_signal_index;

    if (!sender->d_func()->isSignalConnected(signal_index))
        return; // nothing connected to these signals, and no spy

    if (sender->d_func()->blockSig)
        return;

    int signal_absolute_index = methodOffset + local_signal_index;

    void *empty_argv[] = { 0 };
    if (qt_signal_spy_callback_set.signal_begin_callback != 0) {
        qt_signal_spy_callback_set.signal_begin_callback(sender, signal_absolute_index,
                                                         argv ? argv : empty_argv);
    }

    Qt::HANDLE currentThreadId = QThread::currentThreadId();

    QMutexLocker locker(signalSlotLock(sender));
    QObjectConnectionListVector *connectionLists = sender->d_func()->connectionLists;
    if (!connectionLists) {
        locker.unlock();
        if (qt_signal_spy_callback_set.signal_end_callback != 0)
            qt_signal_spy_callback_set.signal_end_callback(sender, signal_absolute_index);
        return;
    }
    ++connectionLists->inUse;


    const QObjectPrivate::ConnectionList *list;
    if (signal_index < connectionLists->count())
        list = &connectionLists->at(signal_index);
    else
        list = &connectionLists->allsignals;

    do {
        QObjectPrivate::Connection *c = list->first;
        if (!c) continue;
        // We need to check against last here to ensure that signals added
        // during the signal emission are not emitted in this emission.
        QObjectPrivate::Connection *last = list->last;

        do {
            if (!c->receiver)
                continue;

            QObject * const receiver = c->receiver;
            const bool receiverInSameThread = currentThreadId == receiver->d_func()->threadData->threadId;

            // determine if this connection should be sent immediately or
            // put into the event queue
            if ((c->connectionType == Qt::AutoConnection && !receiverInSameThread)
                || (c->connectionType == Qt::QueuedConnection)) {
                queued_activate(sender, signal_absolute_index, c, argv ? argv : empty_argv);
                continue;
#ifndef QT_NO_THREAD
            } else if (c->connectionType == Qt::BlockingQueuedConnection) {
                locker.unlock();
                if (receiverInSameThread) {
                    qWarning("Qt: Dead lock detected while activating a BlockingQueuedConnection: "
                    "Sender is %s(%p), receiver is %s(%p)",
                    sender->metaObject()->className(), sender,
                    receiver->metaObject()->className(), receiver);
                }
                QSemaphore semaphore;
                QCoreApplication::postEvent(receiver, new QMetaCallEvent(c->method_offset, c->method_relative,
                                                                         c->callFunction,
                                                                         sender, signal_absolute_index,
                                                                         0, 0,
                                                                         argv ? argv : empty_argv,
                                                                         &semaphore));
                semaphore.acquire();
                locker.relock();
                continue;
#endif
            }

            QObjectPrivate::Sender currentSender;
            QObjectPrivate::Sender *previousSender = 0;
            if (receiverInSameThread) {
                currentSender.sender = sender;
                currentSender.signal = signal_absolute_index;
                currentSender.ref = 1;
                previousSender = QObjectPrivate::setCurrentSender(receiver, &currentSender);
            }
            const QObjectPrivate::StaticMetaCallFunction callFunction = c->callFunction;
            const int method_relative = c->method_relative;
            if (callFunction && c->method_offset <= receiver->metaObject()->methodOffset()) {
                //we compare the vtable to make sure we are not in the destructor of the object.
                locker.unlock();
                if (qt_signal_spy_callback_set.slot_begin_callback != 0)
                    qt_signal_spy_callback_set.slot_begin_callback(receiver, c->method(), argv ? argv : empty_argv);

                callFunction(receiver, QMetaObject::InvokeMetaMethod, method_relative, argv ? argv : empty_argv);

                if (qt_signal_spy_callback_set.slot_end_callback != 0)
                    qt_signal_spy_callback_set.slot_end_callback(receiver, c->method());
                locker.relock();
            } else {
                const int method = method_relative + c->method_offset;
                locker.unlock();

                if (qt_signal_spy_callback_set.slot_begin_callback != 0) {
                    qt_signal_spy_callback_set.slot_begin_callback(receiver,
                                                                method,
                                                                argv ? argv : empty_argv);
                }

#if defined(QT_NO_EXCEPTIONS)
                metacall(receiver, QMetaObject::InvokeMetaMethod, method, argv ? argv : empty_argv);
#else
                QT_TRY {
                    metacall(receiver, QMetaObject::InvokeMetaMethod, method, argv ? argv : empty_argv);
                } QT_CATCH(...) {
                    locker.relock();
                    if (receiverInSameThread)
                        QObjectPrivate::resetCurrentSender(receiver, &currentSender, previousSender);

                    --connectionLists->inUse;
                    Q_ASSERT(connectionLists->inUse >= 0);
                    if (connectionLists->orphaned && !connectionLists->inUse)
                        delete connectionLists;
                    QT_RETHROW;
                }
#endif

                if (qt_signal_spy_callback_set.slot_end_callback != 0)
                    qt_signal_spy_callback_set.slot_end_callback(receiver, method);

                locker.relock();
            }

            if (receiverInSameThread)
                QObjectPrivate::resetCurrentSender(receiver, &currentSender, previousSender);

            if (connectionLists->orphaned)
                break;
        } while (c != last && (c = c->nextConnectionList) != 0);

        if (connectionLists->orphaned)
            break;
    } while (list != &connectionLists->allsignals &&
        //start over for all signals;
        ((list = &connectionLists->allsignals), true));

    --connectionLists->inUse;
    Q_ASSERT(connectionLists->inUse >= 0);
    if (connectionLists->orphaned) {
        if (!connectionLists->inUse)
            delete connectionLists;
    } else if (connectionLists->dirty) {
        sender->d_func()->cleanConnectionLists();
    }

    locker.unlock();

    if (qt_signal_spy_callback_set.signal_end_callback != 0)
        qt_signal_spy_callback_set.signal_end_callback(sender, signal_absolute_index);

}

/*!\internal
   Obsolete.  (signal_index comes from indexOfMethod())
*/
void QMetaObject::activate(QObject *sender, int signal_index, void **argv)
{
    const QMetaObject *mo = sender->metaObject();
    while (mo->methodOffset() > signal_index)
        mo = mo->superClass();
    activate(sender, mo, signal_index - mo->methodOffset(), argv);
}

/*!\internal
   Obsolete, called by moc generated code before Qt 4.6 for cloned signals
   But since Qt 4.6, all clones are connected to their original
 */
void QMetaObject::activate(QObject *sender, const QMetaObject *m,
                           int from_local_signal_index, int to_local_signal_index, void **argv)
{
    Q_UNUSED(to_local_signal_index);
    Q_ASSERT(from_local_signal_index == QMetaObjectPrivate::originalClone(m, to_local_signal_index));
    activate(sender, m, from_local_signal_index, argv);
}

/*! \internal
    Returns the signal index used in the internal connectionLists vector.

    It is different from QMetaObject::indexOfSignal():  indexOfSignal is the same as indexOfMethod
    while QObjectPrivate::signalIndex is smaller because it doesn't give index to slots.
*/
int QObjectPrivate::signalIndex(const char *signalName) const
{
    Q_Q(const QObject);
    const QMetaObject *base = q->metaObject();
    int relative_index = QMetaObjectPrivate::indexOfSignalRelative(&base, signalName, false);
    if (relative_index < 0)
        relative_index = QMetaObjectPrivate::indexOfSignalRelative(&base, signalName, true);
    if (relative_index < 0)
        return relative_index;
    relative_index = QMetaObjectPrivate::originalClone(base, relative_index);
    int signalOffset, methodOffset;
    computeOffsets(base, &signalOffset, &methodOffset);
    return relative_index + signalOffset;
}

/*****************************************************************************
  Properties
 *****************************************************************************/

#ifndef QT_NO_PROPERTIES

/*!
  Sets the value of the object's \a name property to \a value.

  If the property is defined in the class using Q_PROPERTY then
  true is returned on success and false otherwise. If the property
  is not defined using Q_PROPERTY, and therefore not listed in the
  meta-object, it is added as a dynamic property and false is returned.

  Information about all available properties is provided through the
  metaObject() and dynamicPropertyNames().

  Dynamic properties can be queried again using property() and can be
  removed by setting the property value to an invalid QVariant.
  Changing the value of a dynamic property causes a QDynamicPropertyChangeEvent
  to be sent to the object.

  \bold{Note:} Dynamic properties starting with "_q_" are reserved for internal
  purposes.

  \sa property(), metaObject(), dynamicPropertyNames()
*/
bool QObject::setProperty(const char *name, const QVariant &value)
{
    Q_D(QObject);
    const QMetaObject* meta = metaObject();
    if (!name || !meta)
        return false;

    int id = meta->indexOfProperty(name);
    if (id < 0) {
        if (!d->extraData)
            d->extraData = new QObjectPrivate::ExtraData;

        const int idx = d->extraData->propertyNames.indexOf(name);

        if (!value.isValid()) {
            if (idx == -1)
                return false;
            d->extraData->propertyNames.removeAt(idx);
            d->extraData->propertyValues.removeAt(idx);
        } else {
            if (idx == -1) {
                d->extraData->propertyNames.append(name);
                d->extraData->propertyValues.append(value);
            } else {
                d->extraData->propertyValues[idx] = value;
            }
        }

        QDynamicPropertyChangeEvent ev(name);
        QCoreApplication::sendEvent(this, &ev);

        return false;
    }
    QMetaProperty p = meta->property(id);
#ifndef QT_NO_DEBUG
    if (!p.isWritable())
        qWarning("%s::setProperty: Property \"%s\" invalid,"
                 " read-only or does not exist", metaObject()->className(), name);
#endif
    return p.write(this, value);
}

/*!
  Returns the value of the object's \a name property.

  If no such property exists, the returned variant is invalid.

  Information about all available properties is provided through the
  metaObject() and dynamicPropertyNames().

  \sa setProperty(), QVariant::isValid(), metaObject(), dynamicPropertyNames()
*/
QVariant QObject::property(const char *name) const
{
    Q_D(const QObject);
    const QMetaObject* meta = metaObject();
    if (!name || !meta)
        return QVariant();

    int id = meta->indexOfProperty(name);
    if (id < 0) {
        if (!d->extraData)
            return QVariant();
        const int i = d->extraData->propertyNames.indexOf(name);
        return d->extraData->propertyValues.value(i);
    }
    QMetaProperty p = meta->property(id);
#ifndef QT_NO_DEBUG
    if (!p.isReadable())
        qWarning("%s::property: Property \"%s\" invalid or does not exist",
                 metaObject()->className(), name);
#endif
    return p.read(this);
}

/*!
    \since 4.2

    Returns the names of all properties that were dynamically added to
    the object using setProperty().
*/
QList<QByteArray> QObject::dynamicPropertyNames() const
{
    Q_D(const QObject);
    if (d->extraData)
        return d->extraData->propertyNames;
    return QList<QByteArray>();
}

#endif // QT_NO_PROPERTIES


/*****************************************************************************
  QObject debugging output routines.
 *****************************************************************************/

static void dumpRecursive(int level, QObject *object)
{
#if defined(QT_DEBUG)
    if (object) {
        QByteArray buf;
        buf.fill(' ', level / 2 * 8);
        if (level % 2)
            buf += "    ";
        QString name = object->objectName();
        QString flags = QLatin1String("");
#if 0
        if (qApp->focusWidget() == object)
            flags += 'F';
        if (object->isWidgetType()) {
            QWidget * w = (QWidget *)object;
            if (w->isVisible()) {
                QString t("<%1,%2,%3,%4>");
                flags += t.arg(w->x()).arg(w->y()).arg(w->width()).arg(w->height());
            } else {
                flags += 'I';
            }
        }
#endif
        qDebug("%s%s::%s %s", (const char*)buf, object->metaObject()->className(), name.toLocal8Bit().data(),
               flags.toLatin1().data());
        QObjectList children = object->children();
        if (!children.isEmpty()) {
            for (int i = 0; i < children.size(); ++i)
                dumpRecursive(level+1, children.at(i));
        }
    }
#else
    Q_UNUSED(level)
        Q_UNUSED(object)
#endif
}

/*!
    Dumps a tree of children to the debug output.

    This function is useful for debugging, but does nothing if the
    library has been compiled in release mode (i.e. without debugging
    information).

    \sa dumpObjectInfo()
*/

void QObject::dumpObjectTree()
{
    dumpRecursive(0, this);
}

/*!
    Dumps information about signal connections, etc. for this object
    to the debug output.

    This function is useful for debugging, but does nothing if the
    library has been compiled in release mode (i.e. without debugging
    information).

    \sa dumpObjectTree()
*/

void QObject::dumpObjectInfo()
{
#if defined(QT_DEBUG)
    qDebug("OBJECT %s::%s", metaObject()->className(),
           objectName().isEmpty() ? "unnamed" : objectName().toLocal8Bit().data());

    Q_D(QObject);
    QMutexLocker locker(signalSlotLock(this));

    // first, look for connections where this object is the sender
    qDebug("  SIGNALS OUT");

    if (d->connectionLists) {
        int offset = 0;
        int offsetToNextMetaObject = 0;
        for (int signal_index = 0; signal_index < d->connectionLists->count(); ++signal_index) {
            if (signal_index >= offsetToNextMetaObject) {
                const QMetaObject *mo = metaObject();
                int signalOffset, methodOffset;
                computeOffsets(mo, &signalOffset, &methodOffset);
                while (signalOffset > signal_index) {
                    mo = mo->superClass();
                    offsetToNextMetaObject = signalOffset;
                    computeOffsets(mo, &signalOffset, &methodOffset);
                }
                offset = methodOffset - signalOffset;
            }
            const QMetaMethod signal = metaObject()->method(signal_index + offset);
            qDebug("        signal: %s", signal.signature());

            // receivers
            const QObjectPrivate::Connection *c =
                d->connectionLists->at(signal_index).first;
            while (c) {
                if (!c->receiver) {
                    qDebug("          <Disconnected receiver>");
                    c = c->nextConnectionList;
                    continue;
                }
                const QMetaObject *receiverMetaObject = c->receiver->metaObject();
                const QMetaMethod method = receiverMetaObject->method(c->method());
                qDebug("          --> %s::%s %s",
                       receiverMetaObject->className(),
                       c->receiver->objectName().isEmpty() ? "unnamed" : qPrintable(c->receiver->objectName()),
                       method.signature());
                c = c->nextConnectionList;
            }
        }
    } else {
        qDebug( "        <None>" );
    }

    // now look for connections where this object is the receiver
    qDebug("  SIGNALS IN");

    if (d->senders) {
        for (QObjectPrivate::Connection *s = d->senders; s; s = s->next) {
            const QMetaMethod slot = metaObject()->method(s->method());
            qDebug("          <-- %s::%s  %s",
                   s->sender->metaObject()->className(),
                   s->sender->objectName().isEmpty() ? "unnamed" : qPrintable(s->sender->objectName()),
                   slot.signature());
        }
    } else {
        qDebug("        <None>");
    }
#endif
}

#ifndef QT_NO_USERDATA
/*!\internal
 */
uint QObject::registerUserData()
{
    static int user_data_registration = 0;
    return user_data_registration++;
}

/*!\internal
 */
QObjectUserData::~QObjectUserData()
{
}

/*!\internal
 */
void QObject::setUserData(uint id, QObjectUserData* data)
{
    Q_D(QObject);
    if (!d->extraData)
        d->extraData = new QObjectPrivate::ExtraData;

    if (d->extraData->userData.size() <= (int) id)
        d->extraData->userData.resize((int) id + 1);
    d->extraData->userData[id] = data;
}

/*!\internal
 */
QObjectUserData* QObject::userData(uint id) const
{
    Q_D(const QObject);
    if (!d->extraData)
        return 0;
    if ((int)id < d->extraData->userData.size())
        return d->extraData->userData.at(id);
    return 0;
}

#endif // QT_NO_USERDATA


#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QObject *o) {
#ifndef Q_BROKEN_DEBUG_STREAM
    if (!o)
        return dbg << "QObject(0x0) ";
    dbg.nospace() << o->metaObject()->className() << '(' << (void *)o;
    if (!o->objectName().isEmpty())
        dbg << ", name = " << o->objectName();
    dbg << ')';
    return dbg.space();
#else
    qWarning("This compiler doesn't support streaming QObject to QDebug");
    return dbg;
    Q_UNUSED(o);
#endif
}
#endif

/*!
  \fn void QObject::insertChild(QObject *object)

  Use setParent() instead, i.e., call object->setParent(this).
*/

/*!
  \fn void QObject::removeChild(QObject *object)

  Use setParent() instead, i.e., call object->setParent(0).
*/

/*!
  \fn bool QObject::isA(const char *className) const

  Compare \a className with the object's metaObject()->className() instead.
*/

/*!
  \fn const char *QObject::className() const

  Use metaObject()->className() instead.
*/

/*!
  \fn const char *QObject::name() const

  Use objectName() instead.
*/

/*!
  \fn const char *QObject::name(const char *defaultName) const

  Use objectName() instead.
*/

/*!
  \fn void QObject::setName(const char *name)

  Use setObjectName() instead.
*/

/*!
  \fn bool QObject::checkConnectArgs(const char *signal, const
  QObject *object, const char *method)

  Use QMetaObject::checkConnectArgs() instead.
*/

/*!
  \fn QByteArray QObject::normalizeSignalSlot(const char *signalSlot)

  Use QMetaObject::normalizedSignature() instead.
*/

/*!
  \fn const char *QMetaObject::superClassName() const

  \internal
*/

/*!
    \macro Q_CLASSINFO(Name, Value)
    \relates QObject

    This macro associates extra information to the class, which is
    available using QObject::metaObject(). Except for the ActiveQt
    extension, Qt doesn't use this information.

    The extra information takes the form of a \a Name string and a \a
    Value literal string.

    Example:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 35

    \sa QMetaObject::classInfo()
*/

/*!
    \macro Q_INTERFACES(...)
    \relates QObject

    This macro tells Qt which interfaces the class implements. This
    is used when implementing plugins.

    Example:

    \snippet examples/tools/plugandpaintplugins/basictools/basictoolsplugin.h 1
    \dots
    \snippet examples/tools/plugandpaintplugins/basictools/basictoolsplugin.h 3

    See the \l{tools/plugandpaintplugins/basictools}{Plug & Paint
    Basic Tools} example for details.

    \sa Q_DECLARE_INTERFACE(), Q_EXPORT_PLUGIN2(), {How to Create Qt Plugins}
*/

/*!
    \macro Q_PROPERTY(...)
    \relates QObject

    This macro is used for declaring properties in classes that
    inherit QObject. Properties behave like class data members, but
    they have additional features accessible through the \l
    {Meta-Object System}.

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 36

    The property name and type and the \c READ function are required.
    The type can be any type supported by QVariant, or it can be a
    user-defined type.  The other items are optional, but a \c WRITE
    function is common.  The attributes default to true except \c USER,
    which defaults to false.

    For example:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 37

    For more details about how to use this macro, and a more detailed
    example of its use, see the discussion on \l {Qt's Property System}.

    \sa {Qt's Property System}
*/

/*!
    \macro Q_ENUMS(...)
    \relates QObject

    This macro registers one or several enum types to the meta-object
    system.

    For example:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 38

    If you want to register an enum that is declared in another class,
    the enum must be fully qualified with the name of the class
    defining it. In addition, the class \e defining the enum has to
    inherit QObject as well as declare the enum using Q_ENUMS().

    \sa {Qt's Property System}
*/

/*!
    \macro Q_FLAGS(...)
    \relates QObject

    This macro registers one or several \l{QFlags}{flags types} to the
    meta-object system. It is typically used in a class definition to declare
    that values of a given enum can be used as flags and combined using the
    bitwise OR operator.

    For example, in QLibrary, the \l{QLibrary::LoadHints}{LoadHints} flag is
    declared in the following way:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 39a

    The declaration of the flags themselves is performed in the public section
    of the QLibrary class itself, using the \l Q_DECLARE_FLAGS() macro:

    \snippet doc/src/snippets/code/src_corelib_kernel_qobject.cpp 39b

    \note This macro takes care of registering individual flag values
    with the meta-object system, so it is unnecessary to use Q_ENUMS()
    in addition to this macro.

    \sa {Qt's Property System}
*/

/*!
    \macro Q_OBJECT
    \relates QObject

    The Q_OBJECT macro must appear in the private section of a class
    definition that declares its own signals and slots or that uses
    other services provided by Qt's meta-object system.

    For example:

    \snippet doc/src/snippets/signalsandslots/signalsandslots.h 1
    \codeline
    \snippet doc/src/snippets/signalsandslots/signalsandslots.h 2
    \snippet doc/src/snippets/signalsandslots/signalsandslots.h 3

    \note This macro requires the class to be a subclass of QObject. Use
    Q_GADGET instead of Q_OBJECT to enable the meta object system's support
    for enums in a class that is not a QObject subclass. Q_GADGET makes a
    class member, \c{staticMetaObject}, available.
    \c{staticMetaObject} is of type QMetaObject and provides access to the
    enums declared with Q_ENUMS.
    Q_GADGET is provided only for C++.

    \sa {Meta-Object System}, {Signals and Slots}, {Qt's Property System}
*/

/*!
    \macro Q_SIGNALS
    \relates QObject

    Use this macro to replace the \c signals keyword in class
    declarations, when you want to use Qt Signals and Slots with a
    \l{3rd Party Signals and Slots} {3rd party signal/slot mechanism}.

    The macro is normally used when \c no_keywords is specified with
    the \c CONFIG variable in the \c .pro file, but it can be used
    even when \c no_keywords is \e not specified.
*/

/*!
    \macro Q_SIGNAL
    \relates QObject

    This is an additional macro that allows you to mark a single
    function as a signal. It can be quite useful, especially when you
    use a 3rd-party source code parser which doesn't understand a \c
    signals or \c Q_SIGNALS groups.

    Use this macro to replace the \c signals keyword in class
    declarations, when you want to use Qt Signals and Slots with a
    \l{3rd Party Signals and Slots} {3rd party signal/slot mechanism}.

    The macro is normally used when \c no_keywords is specified with
    the \c CONFIG variable in the \c .pro file, but it can be used
    even when \c no_keywords is \e not specified.
*/

/*!
    \macro Q_SLOTS
    \relates QObject

    Use this macro to replace the \c slots keyword in class
    declarations, when you want to use Qt Signals and Slots with a
    \l{3rd Party Signals and Slots} {3rd party signal/slot mechanism}.

    The macro is normally used when \c no_keywords is specified with
    the \c CONFIG variable in the \c .pro file, but it can be used
    even when \c no_keywords is \e not specified.
*/

/*!
    \macro Q_SLOT
    \relates QObject

    This is an additional macro that allows you to mark a single
    function as a slot. It can be quite useful, especially when you
    use a 3rd-party source code parser which doesn't understand a \c
    slots or \c Q_SLOTS groups.

    Use this macro to replace the \c slots keyword in class
    declarations, when you want to use Qt Signals and Slots with a
    \l{3rd Party Signals and Slots} {3rd party signal/slot mechanism}.

    The macro is normally used when \c no_keywords is specified with
    the \c CONFIG variable in the \c .pro file, but it can be used
    even when \c no_keywords is \e not specified.
*/

/*!
    \macro Q_EMIT
    \relates QObject

    Use this macro to replace the \c emit keyword for emitting
    signals, when you want to use Qt Signals and Slots with a
    \l{3rd Party Signals and Slots} {3rd party signal/slot mechanism}.

    The macro is normally used when \c no_keywords is specified with
    the \c CONFIG variable in the \c .pro file, but it can be used
    even when \c no_keywords is \e not specified.
*/

/*!
    \macro Q_INVOKABLE
    \relates QObject

    Apply this macro to definitions of member functions to allow them to
    be invoked via the meta-object system. The macro is written before
    the return type, as shown in the following example:

    \snippet snippets/qmetaobject-invokable/window.h Window class with invokable method

    The \c invokableMethod() function is marked up using Q_INVOKABLE, causing
    it to be registered with the meta-object system and enabling it to be
    invoked using QMetaObject::invokeMethod().
    Since \c normalMethod() function is not registered in this way, it cannot
    be invoked using QMetaObject::invokeMethod().
*/

/*!
    \typedef QObjectList
    \relates QObject

    Synonym for QList<QObject *>.
*/

void qDeleteInEventHandler(QObject *o)
{
#ifdef QT_JAMBI_BUILD
    if (!o)
        return;
    QObjectPrivate::get(o)->inEventHandler = false;
#endif
    delete o;
}


QT_END_NAMESPACE

#include "moc_qobject.cpp"
