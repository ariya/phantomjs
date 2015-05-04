/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtDBus module of the Qt Toolkit.
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

#ifndef QDBUSCONNECTION_P_H
#define QDBUSCONNECTION_P_H

#include <qdbuserror.h>
#include <qdbusconnection.h>

#include <QtCore/qatomic.h>
#include <QtCore/qhash.h>
#include <QtCore/qmutex.h>
#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>
#include <QtCore/qreadwritelock.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qvector.h>

#include "qdbus_symbols_p.h"

#include <qdbusmessage.h>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class QDBusMessage;
class QSocketNotifier;
class QTimerEvent;
class QDBusObjectPrivate;
class QDBusCallDeliveryEvent;
class QDBusActivateObjectEvent;
class QMetaMethod;
class QDBusInterfacePrivate;
struct QDBusMetaObject;
class QDBusAbstractInterface;
class QDBusConnectionInterface;
class QDBusPendingCallPrivate;

#ifndef QT_BOOTSTRAPPED

class QDBusErrorInternal
{
    mutable DBusError error;
    Q_DISABLE_COPY(QDBusErrorInternal)
public:
    inline QDBusErrorInternal() { q_dbus_error_init(&error); }
    inline ~QDBusErrorInternal() { q_dbus_error_free(&error); }
    inline bool operator !() const { return !q_dbus_error_is_set(&error); }
    inline operator DBusError *() { q_dbus_error_free(&error); return &error; }
    inline operator QDBusError() const { QDBusError err(&error); q_dbus_error_free(&error); return err; }
};

// QDBusConnectionPrivate holds the DBusConnection and
// can have many QDBusConnection objects referring to it

class QDBusConnectionPrivate: public QObject
{
    Q_OBJECT
public:
    // structs and enums
    enum ConnectionMode { InvalidMode, ServerMode, ClientMode, PeerMode }; // LocalMode

    struct Watcher
    {
        Watcher(): watch(0), read(0), write(0) {}
        DBusWatch *watch;
        QSocketNotifier *read;
        QSocketNotifier *write;
    };

    struct SignalHook
    {
        inline SignalHook() : obj(0), midx(-1) { }
        QString service, path, signature;
        QObject* obj;
        int midx;
        QVector<int> params;
        QStringList argumentMatch;
        QByteArray matchRule;
    };

    enum TreeNodeType {
        Object = 0x0,
        VirtualObject = 0x01000000
    };

    struct ObjectTreeNode
    {
        typedef QVector<ObjectTreeNode> DataList;

        inline ObjectTreeNode() : obj(0), flags(0) { }
        inline ObjectTreeNode(const QString &n) // intentionally implicit
            : name(n), obj(0), flags(0) { }
        inline ~ObjectTreeNode() { }
        inline bool operator<(const QString &other) const
            { return name < other; }
        inline bool operator<(const QStringRef &other) const
            { return QStringRef(&name) < other; }
#if defined(Q_CC_MSVC) && _MSC_VER < 1600
        inline bool operator<(const ObjectTreeNode &other) const
            { return name < other.name; }
        friend inline bool operator<(const QString &str, const ObjectTreeNode &obj)
            { return str < obj.name; }
        friend inline bool operator<(const QStringRef &str, const ObjectTreeNode &obj)
            { return str < QStringRef(&obj.name); }
#endif
        inline bool isActive() const
        { return obj || !children.isEmpty(); }

        QString name;
        union {
            QObject *obj;
            QDBusVirtualObject *treeNode;
        };
        int flags;

        DataList children;
    };

public:
    // typedefs
    typedef QMultiHash<int, Watcher> WatcherHash;
    typedef QHash<int, DBusTimeout *> TimeoutHash;
    typedef QList<QPair<DBusTimeout *, int> > PendingTimeoutList;

    typedef QMultiHash<QString, SignalHook> SignalHookHash;
    typedef QHash<QString, QDBusMetaObject* > MetaObjectHash;
    typedef QHash<QByteArray, int> MatchRefCountHash;
    typedef QList<QDBusPendingCallPrivate*> PendingCallList;

    struct WatchedServiceData {
        WatchedServiceData() : refcount(0) {}
        WatchedServiceData(const QString &owner, int refcount = 0)
            : owner(owner), refcount(refcount)
        {}
        QString owner;
        int refcount;
    };
    typedef QHash<QString, WatchedServiceData> WatchedServicesHash;

public:
    // public methods are entry points from other objects
    explicit QDBusConnectionPrivate(QObject *parent = 0);
    ~QDBusConnectionPrivate();
    void deleteYourself();

    void setBusService(const QDBusConnection &connection);
    void setPeer(DBusConnection *connection, const QDBusErrorInternal &error);
    void setConnection(DBusConnection *connection, const QDBusErrorInternal &error);
    void setServer(DBusServer *server, const QDBusErrorInternal &error);
    void closeConnection();

    QString getNameOwner(const QString &service);

    int send(const QDBusMessage &message);
    QDBusMessage sendWithReply(const QDBusMessage &message, int mode, int timeout = -1);
    QDBusMessage sendWithReplyLocal(const QDBusMessage &message);
    QDBusPendingCallPrivate *sendWithReplyAsync(const QDBusMessage &message, QObject *receiver,
                                                const char *returnMethod, const char *errorMethod,int timeout = -1);
    bool connectSignal(const QString &service, const QString &path, const QString& interface,
                       const QString &name, const QStringList &argumentMatch, const QString &signature,
                       QObject *receiver, const char *slot);
    void connectSignal(const QString &key, const SignalHook &hook);
    SignalHookHash::Iterator disconnectSignal(SignalHookHash::Iterator &it);
    bool disconnectSignal(const QString &service, const QString &path, const QString& interface,
                          const QString &name, const QStringList &argumentMatch, const QString &signature,
                          QObject *receiver, const char *slot);
    void registerObject(const ObjectTreeNode *node);
    void unregisterObject(const QString &path, QDBusConnection::UnregisterMode mode);
    void connectRelay(const QString &service,
                      const QString &path, const QString &interface,
                      QDBusAbstractInterface *receiver, const QMetaMethod &signal);
    void disconnectRelay(const QString &service,
                         const QString &path, const QString &interface,
                         QDBusAbstractInterface *receiver, const QMetaMethod &signal);
    void registerService(const QString &serviceName);
    void unregisterService(const QString &serviceName);

    bool handleMessage(const QDBusMessage &msg);
    void waitForFinished(QDBusPendingCallPrivate *pcall);

    QDBusMetaObject *findMetaObject(const QString &service, const QString &path,
                                    const QString &interface, QDBusError &error);

    void postEventToThread(int action, QObject *target, QEvent *event);

    inline void serverConnection(const QDBusConnection &connection)
        { emit newServerConnection(connection); }

private:
    void checkThread();
    bool handleError(const QDBusErrorInternal &error);

    void handleSignal(const QString &key, const QDBusMessage &msg);
    void handleSignal(const QDBusMessage &msg);
    void handleObjectCall(const QDBusMessage &message);

    void activateSignal(const SignalHook& hook, const QDBusMessage &msg);
    void activateObject(ObjectTreeNode &node, const QDBusMessage &msg, int pathStartPos);
    bool activateInternalFilters(const ObjectTreeNode &node, const QDBusMessage &msg);
    bool activateCall(QObject *object, int flags, const QDBusMessage &msg);

    void sendError(const QDBusMessage &msg, QDBusError::ErrorType code);
    void deliverCall(QObject *object, int flags, const QDBusMessage &msg,
                     const QVector<int> &metaTypes, int slotIdx);

    bool isServiceRegisteredByThread(const QString &serviceName);

    QString getNameOwnerNoCache(const QString &service);

protected:
    void customEvent(QEvent *e);
    void timerEvent(QTimerEvent *e);

public slots:
    // public slots
    void doDispatch();
    void socketRead(int);
    void socketWrite(int);
    void objectDestroyed(QObject *o);
    void relaySignal(QObject *obj, const QMetaObject *, int signalId, const QVariantList &args);

private slots:
    void serviceOwnerChangedNoLock(const QString &name, const QString &oldOwner, const QString &newOwner);
    void registerServiceNoLock(const QString &serviceName);
    void unregisterServiceNoLock(const QString &serviceName);

signals:
    void serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner);
    void callWithCallbackFailed(const QDBusError &error, const QDBusMessage &message);
    void newServerConnection(const QDBusConnection &connection);

public:
    QAtomicInt ref;
    QDBusConnection::ConnectionCapabilities capabilities;
    QString name;               // this connection's name
    QString baseService;        // this connection's base service
    QStringList serverConnectionNames;

    ConnectionMode mode;
    QDBusConnectionInterface *busService;

    // the dispatch lock protects everything related to the DBusConnection or DBusServer
    // including the timeouts and watches
    QMutex dispatchLock;
    DBusConnection *connection;
    DBusServer *server;
    WatcherHash watchers;
    TimeoutHash timeouts;
    PendingTimeoutList timeoutsPendingAdd;

    // the master lock protects our own internal state
    QReadWriteLock lock;
    QDBusError lastError;

    QStringList serviceNames;
    WatchedServicesHash watchedServices;
    SignalHookHash signalHooks;
    MatchRefCountHash matchRefCounts;
    ObjectTreeNode rootNode;
    MetaObjectHash cachedMetaObjects;
    PendingCallList pendingCalls;

    QMutex callDeliveryMutex;
    QDBusCallDeliveryEvent *callDeliveryState; // protected by the callDeliveryMutex mutex

    bool anonymousAuthenticationAllowed;

public:
    // static methods
    static int findSlot(QObject *obj, const QByteArray &normalizedName, QVector<int> &params);
    static bool prepareHook(QDBusConnectionPrivate::SignalHook &hook, QString &key,
                            const QString &service,
                            const QString &path, const QString &interface, const QString &name,
                            const QStringList &argMatch,
                            QObject *receiver, const char *signal, int minMIdx,
                            bool buildSignature);
    static DBusHandlerResult messageFilter(DBusConnection *, DBusMessage *, void *);
    static bool checkReplyForDelivery(QDBusConnectionPrivate *target, QObject *object,
                                      int idx, const QList<int> &metaTypes,
                                      const QDBusMessage &msg);
    static QDBusCallDeliveryEvent *prepareReply(QDBusConnectionPrivate *target, QObject *object,
                                                int idx, const QVector<int> &metaTypes,
                                                const QDBusMessage &msg);
    static void processFinishedCall(QDBusPendingCallPrivate *call);

    static QDBusConnectionPrivate *d(const QDBusConnection& q) { return q.d; }
    static QDBusConnection q(QDBusConnectionPrivate *connection) { return QDBusConnection(connection); }

    static void setSender(const QDBusConnectionPrivate *s);

    friend class QDBusActivateObjectEvent;
    friend class QDBusCallDeliveryEvent;
};

// in qdbusmisc.cpp
extern int qDBusParametersForMethod(const QMetaMethod &mm, QVector<int> &metaTypes, QString &errorMsg);
#endif // QT_BOOTSTRAPPED
extern Q_DBUS_EXPORT int qDBusParametersForMethod(const QList<QByteArray> &parameters, QVector<int>& metaTypes, QString &errorMsg);
extern Q_DBUS_EXPORT bool qDBusCheckAsyncTag(const char *tag);
#ifndef QT_BOOTSTRAPPED
extern bool qDBusInterfaceInObject(QObject *obj, const QString &interface_name);
extern QString qDBusInterfaceFromMetaObject(const QMetaObject *mo);

// in qdbusinternalfilters.cpp
extern QString qDBusIntrospectObject(const QDBusConnectionPrivate::ObjectTreeNode &node, const QString &path);
extern QDBusMessage qDBusPropertyGet(const QDBusConnectionPrivate::ObjectTreeNode &node,
                                     const QDBusMessage &msg);
extern QDBusMessage qDBusPropertySet(const QDBusConnectionPrivate::ObjectTreeNode &node,
                                     const QDBusMessage &msg);
extern QDBusMessage qDBusPropertyGetAll(const QDBusConnectionPrivate::ObjectTreeNode &node,
                                        const QDBusMessage &msg);
#endif // QT_BOOTSTRAPPED

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
