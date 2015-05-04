/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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


#ifndef Q_SPI_STRUCT_MARSHALLERS_H
#define Q_SPI_STRUCT_MARSHALLERS_H

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

#include <QtCore/qlist.h>
#include <QtCore/qpair.h>
#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusObjectPath>

QT_BEGIN_NAMESPACE

typedef QList <int> QSpiIntList;
typedef QList <uint> QSpiUIntList;

// FIXME: make this copy on write
struct QSpiObjectReference
{
    QString service;
    QDBusObjectPath path;

    QSpiObjectReference();
    QSpiObjectReference(const QDBusConnection& connection, const QDBusObjectPath& path)
        : service(connection.baseService()), path(path) {}
};

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiObjectReference &address);
const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiObjectReference &address);

typedef QList <QSpiObjectReference> QSpiObjectReferenceArray;

struct QSpiAccessibleCacheItem
{
    QSpiObjectReference         path;
    QSpiObjectReference         application;
    QSpiObjectReference         parent;
    QList <QSpiObjectReference> children;
    QStringList                 supportedInterfaces;
    QString                     name;
    uint                        role;
    QString                     description;
    QSpiUIntList                state;
};

typedef QList <QSpiAccessibleCacheItem> QSpiAccessibleCacheArray;

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiAccessibleCacheItem &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiAccessibleCacheItem &item);

struct QSpiAction
{
    QString name;
    QString description;
    QString keyBinding;
};

typedef QList <QSpiAction> QSpiActionArray;

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiAction &action);
const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiAction &action);

struct QSpiEventListener
{
    QString listenerAddress;
    QString eventName;
};

typedef QList <QSpiEventListener> QSpiEventListenerArray;

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiEventListener &action);
const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiEventListener &action);

typedef QPair < unsigned int, QList < QSpiObjectReference > > QSpiRelationArrayEntry;
typedef QList< QSpiRelationArrayEntry > QSpiRelationArray;

//a(iisv)
struct QSpiTextRange {
    int startOffset;
    int endOffset;
    QString contents;
    QVariant v;
};
typedef QList <QSpiTextRange> QSpiTextRangeList;
typedef QMap <QString, QString> QSpiAttributeSet;

enum QSpiAppUpdateType {
    QSPI_APP_UPDATE_ADDED = 0,
    QSPI_APP_UPDATE_REMOVED = 1
};

struct QSpiAppUpdate {
    int type; /* Is an application added or removed */
    QString address; /* D-Bus address of application added or removed */
};

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiAppUpdate &update);
const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiAppUpdate &update);

struct QSpiDeviceEvent {
    unsigned int type;
    int id;
    int hardwareCode;
    int modifiers;
    int timestamp;
    QString text;
    bool isText;
};

QDBusArgument &operator<<(QDBusArgument &argument, const QSpiDeviceEvent &event);
const QDBusArgument &operator>>(const QDBusArgument &argument, QSpiDeviceEvent &event);

void qSpiInitializeStructTypes();

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QSpiIntList)
Q_DECLARE_METATYPE(QSpiUIntList)
Q_DECLARE_METATYPE(QSpiObjectReference)
Q_DECLARE_METATYPE(QSpiObjectReferenceArray)
Q_DECLARE_METATYPE(QSpiAccessibleCacheItem)
Q_DECLARE_METATYPE(QSpiAccessibleCacheArray)
Q_DECLARE_METATYPE(QSpiAction)
Q_DECLARE_METATYPE(QSpiActionArray)
Q_DECLARE_METATYPE(QSpiEventListener)
Q_DECLARE_METATYPE(QSpiEventListenerArray)
Q_DECLARE_METATYPE(QSpiRelationArrayEntry)
Q_DECLARE_METATYPE(QSpiRelationArray)
Q_DECLARE_METATYPE(QSpiTextRange)
Q_DECLARE_METATYPE(QSpiTextRangeList)
Q_DECLARE_METATYPE(QSpiAttributeSet)
Q_DECLARE_METATYPE(QSpiAppUpdate)
Q_DECLARE_METATYPE(QSpiDeviceEvent)

#endif /* Q_SPI_STRUCT_MARSHALLERS_H */
