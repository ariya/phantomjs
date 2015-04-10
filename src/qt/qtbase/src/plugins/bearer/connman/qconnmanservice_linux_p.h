/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QCONNMANSERVICE_H
#define QCONNMANSERVICE_H

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

#include <QtDBus/QtDBus>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusArgument>

#include <QtDBus/QDBusPendingCallWatcher>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusContext>
#include <QMap>

#ifndef QT_NO_BEARERMANAGEMENT
#ifndef QT_NO_DBUS

#ifndef __CONNMAN_DBUS_H

#define CONNMAN_SERVICE     "net.connman"
#define CONNMAN_PATH        "/net/connman"
#define CONNMAN_MANAGER_INTERFACE   CONNMAN_SERVICE ".Manager"
#define CONNMAN_MANAGER_PATH        "/"
#define CONNMAN_SERVICE_INTERFACE   CONNMAN_SERVICE ".Service"
#define CONNMAN_TECHNOLOGY_INTERFACE    CONNMAN_SERVICE ".Technology"
#endif

QT_BEGIN_NAMESPACE

struct ConnmanMap {
    QDBusObjectPath objectPath;
    QVariantMap propertyMap;
};

typedef QList< ConnmanMap > ConnmanMapList;

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(ConnmanMap))
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(ConnmanMapList))

QT_BEGIN_NAMESPACE

QDBusArgument &operator<<(QDBusArgument &argument, const ConnmanMap &obj);
const QDBusArgument &operator>>(const QDBusArgument &argument, ConnmanMap &obj);

class QConnmanTechnologyInterface;
class QConnmanServiceInterface;

class QConnmanManagerInterface : public  QDBusAbstractInterface
{
    Q_OBJECT

public:

    QConnmanManagerInterface( QObject *parent = 0);
    ~QConnmanManagerInterface();

    QDBusObjectPath path() const;
    QVariantMap getProperties();

    QString getState();
    bool getOfflineMode();
    QStringList getTechnologies();
    QStringList getServices();
    void requestScan(const QString &type);

    QHash<QString, QConnmanTechnologyInterface *> technologiesMap;

Q_SIGNALS:
    void propertyChanged(const QString &, const QDBusVariant &value);
    void stateChanged(const QString &);
    void propertyChangedContext(const QString &,const QString &,const QDBusVariant &);
    void servicesChanged(const ConnmanMapList&, const QList<QDBusObjectPath> &);

    void servicesReady(const QStringList &);
    void scanFinished();

protected:
    void connectNotify(const QMetaMethod &signal);
    QVariant getProperty(const QString &);

private:
     QVariantMap propertiesCacheMap;
     QStringList servicesList;
     QStringList technologiesList;

private slots:
    void onServicesChanged(const ConnmanMapList&, const QList<QDBusObjectPath> &);
    void changedProperty(const QString &, const QDBusVariant &value);

    void propertiesReply(QDBusPendingCallWatcher *call);
    void servicesReply(QDBusPendingCallWatcher *call);

    void technologyAdded(const QDBusObjectPath &technology, const QVariantMap &properties);
    void technologyRemoved(const QDBusObjectPath &technology);

};

class QConnmanServiceInterface : public QDBusAbstractInterface
{
    Q_OBJECT

public:

    explicit QConnmanServiceInterface(const QString &dbusPathName,QObject *parent = 0);
    ~QConnmanServiceInterface();

    QVariantMap getProperties();
      // clearProperty
    void connect();
    void disconnect();
    void remove();

// properties
    QString state();
    QString lastError();
    QString name();
    QString type();
    QString security();
    bool favorite();
    bool autoConnect();
    bool roaming();
    QVariantMap ethernet();
    QString serviceInterface();

    bool isOfflineMode();
    QStringList services();

Q_SIGNALS:
    void propertyChanged(const QString &, const QDBusVariant &value);
    void propertyChangedContext(const QString &,const QString &,const QDBusVariant &);
    void propertiesReady();
    void stateChanged(const QString &state);

protected:
    void connectNotify(const QMetaMethod &signal);
    QVariant getProperty(const QString &);
private:
    QVariantMap propertiesCacheMap;
private slots:
    void propertiesReply(QDBusPendingCallWatcher *call);
    void changedProperty(const QString &, const QDBusVariant &value);

};

class QConnmanTechnologyInterface : public QDBusAbstractInterface
{
    Q_OBJECT

public:

    explicit QConnmanTechnologyInterface(const QString &dbusPathName,QObject *parent = 0);
    ~QConnmanTechnologyInterface();

    QString type();

    void scan();
Q_SIGNALS:
    void propertyChanged(const QString &, const QDBusVariant &value);
    void propertyChangedContext(const QString &,const QString &,const QDBusVariant &);
    void scanFinished();
protected:
    void connectNotify(const QMetaMethod &signal);
    QVariant getProperty(const QString &);
private:
    QVariantMap properties();
    QVariantMap propertiesMap;
    void scanReply(QDBusPendingCallWatcher *call);

};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif // QT_NO_BEARERMANAGEMENT

#endif //QCONNMANSERVICE_H
