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

#include "qdbusconnection_p.h"

#include "qdbus_symbols_p.h"
#include <QtCore/qcoreapplication.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qthread.h>

#include "qdbusabstractadaptor.h"
#include "qdbusabstractadaptor_p.h"
#include "qdbusconnection.h"
#include "qdbusextratypes.h"
#include "qdbusmessage.h"
#include "qdbusmetatype.h"
#include "qdbusmetatype_p.h"
#include "qdbusmessage_p.h"
#include "qdbusutil_p.h"
#include "qdbusvirtualobject.h"

#include <algorithm>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

// defined in qdbusxmlgenerator.cpp
extern QString qDBusGenerateMetaObjectXml(QString interface, const QMetaObject *mo,
                                          const QMetaObject *base, int flags);

static const char introspectableInterfaceXml[] =
    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
    "    <method name=\"Introspect\">\n"
    "      <arg name=\"xml_data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "  </interface>\n";

static const char propertiesInterfaceXml[] =
    "  <interface name=\"org.freedesktop.DBus.Properties\">\n"
    "    <method name=\"Get\">\n"
    "      <arg name=\"interface_name\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"property_name\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"value\" type=\"v\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"Set\">\n"
    "      <arg name=\"interface_name\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"property_name\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"value\" type=\"v\" direction=\"in\"/>\n"
    "    </method>\n"
    "    <method name=\"GetAll\">\n"
    "      <arg name=\"interface_name\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"values\" type=\"a{sv}\" direction=\"out\"/>\n"
    "      <annotation name=\"org.qtproject.QtDBus.QtTypeName.Out0\" value=\"QVariantMap\"/>\n"
    "    </method>\n"
    "    <signal name=\"PropertiesChanged\">\n"
    "      <arg name=\"interface_name\" type=\"s\" direction=\"out\"/>\n"
    "      <arg name=\"changed_properties\" type=\"a{sv}\" direction=\"out\"/>\n"
    "      <annotation name=\"org.qtproject.QtDBus.QtTypeName.Out1\" value=\"QVariantMap\"/>\n"
    "      <arg name=\"invalidated_properties\" type=\"as\" direction=\"out\"/>\n"
    "    </signal>\n"
    "  </interface>\n";

static const char peerInterfaceXml[] =
    "  <interface name=\"org.freedesktop.DBus.Peer\">\n"
    "    <method name=\"Ping\"/>\n"
    "    <method name=\"GetMachineId\">\n"
    "      <arg name=\"machine_uuid\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "  </interface>\n";

static QString generateSubObjectXml(QObject *object)
{
    QString retval;
    const QObjectList &objs = object->children();
    QObjectList::ConstIterator it = objs.constBegin();
    QObjectList::ConstIterator end = objs.constEnd();
    for ( ; it != end; ++it) {
        QString name = (*it)->objectName();
        if (!name.isEmpty() && QDBusUtil::isValidPartOfObjectPath(name))
            retval += QString::fromLatin1("  <node name=\"%1\"/>\n")
                      .arg(name);
    }
    return retval;
}

// declared as extern in qdbusconnection_p.h

QString qDBusIntrospectObject(const QDBusConnectionPrivate::ObjectTreeNode &node, const QString &path)
{
    // object may be null

    QString xml_data(QLatin1String(DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE));
    xml_data += QLatin1String("<node>\n");

    if (node.obj) {
        Q_ASSERT_X(QThread::currentThread() == node.obj->thread(),
                   "QDBusConnection: internal threading error",
                   "function called for an object that is in another thread!!");

        if (node.flags & (QDBusConnection::ExportScriptableContents
                           | QDBusConnection::ExportNonScriptableContents)) {
            // create XML for the object itself
            const QMetaObject *mo = node.obj->metaObject();
            for ( ; mo != &QObject::staticMetaObject; mo = mo->superClass())
                xml_data += qDBusGenerateMetaObjectXml(QString(), mo, mo->superClass(),
                                                       node.flags);
        }

        // does this object have adaptors?
        QDBusAdaptorConnector *connector;
        if (node.flags & QDBusConnection::ExportAdaptors &&
            (connector = qDBusFindAdaptorConnector(node.obj))) {

            // trasverse every adaptor in this object
            QDBusAdaptorConnector::AdaptorMap::ConstIterator it = connector->adaptors.constBegin();
            QDBusAdaptorConnector::AdaptorMap::ConstIterator end = connector->adaptors.constEnd();
            for ( ; it != end; ++it) {
                // add the interface:
                QString ifaceXml = QDBusAbstractAdaptorPrivate::retrieveIntrospectionXml(it->adaptor);
                if (ifaceXml.isEmpty()) {
                    // add the interface's contents:
                    ifaceXml += qDBusGenerateMetaObjectXml(QString::fromLatin1(it->interface),
                                                           it->adaptor->metaObject(),
                                                           &QDBusAbstractAdaptor::staticMetaObject,
                                                           QDBusConnection::ExportScriptableContents
                                                           | QDBusConnection::ExportNonScriptableContents);

                    QDBusAbstractAdaptorPrivate::saveIntrospectionXml(it->adaptor, ifaceXml);
                }

                xml_data += ifaceXml;
            }
        }

        // is it a virtual node that handles introspection itself?
        if (node.flags & QDBusConnectionPrivate::VirtualObject) {
            xml_data += node.treeNode->introspect(path);
        }

        xml_data += QLatin1String( propertiesInterfaceXml );
    }

    xml_data += QLatin1String( introspectableInterfaceXml );
    xml_data += QLatin1String( peerInterfaceXml );

    if (node.flags & QDBusConnection::ExportChildObjects) {
        xml_data += generateSubObjectXml(node.obj);
    } else {
        // generate from the object tree
        QDBusConnectionPrivate::ObjectTreeNode::DataList::ConstIterator it =
            node.children.constBegin();
        QDBusConnectionPrivate::ObjectTreeNode::DataList::ConstIterator end =
            node.children.constEnd();
        for ( ; it != end; ++it)
            if (it->obj || !it->children.isEmpty())
                xml_data += QString::fromLatin1("  <node name=\"%1\"/>\n")
                            .arg(it->name);
    }

    xml_data += QLatin1String("</node>\n");
    return xml_data;
}

// implement the D-Bus interface org.freedesktop.DBus.Properties

static inline QDBusMessage interfaceNotFoundError(const QDBusMessage &msg, const QString &interface_name)
{
    return msg.createErrorReply(QDBusError::UnknownInterface,
                                QString::fromLatin1("Interface %1 was not found in object %2")
                                .arg(interface_name)
                                .arg(msg.path()));
}

static inline QDBusMessage
propertyNotFoundError(const QDBusMessage &msg, const QString &interface_name, const QByteArray &property_name)
{
    return msg.createErrorReply(QDBusError::UnknownProperty,
                                QString::fromLatin1("Property %1%2%3 was not found in object %4")
                                .arg(interface_name,
                                     QString::fromLatin1(interface_name.isEmpty() ? "" : "."),
                                     QString::fromLatin1(property_name),
                                     msg.path()));
}

QDBusMessage qDBusPropertyGet(const QDBusConnectionPrivate::ObjectTreeNode &node,
                              const QDBusMessage &msg)
{
    Q_ASSERT(msg.arguments().count() == 2);
    Q_ASSERT_X(!node.obj || QThread::currentThread() == node.obj->thread(),
               "QDBusConnection: internal threading error",
               "function called for an object that is in another thread!!");

    QString interface_name = msg.arguments().at(0).toString();
    QByteArray property_name = msg.arguments().at(1).toString().toUtf8();

    QDBusAdaptorConnector *connector;
    QVariant value;
    bool interfaceFound = false;
    if (node.flags & QDBusConnection::ExportAdaptors &&
        (connector = qDBusFindAdaptorConnector(node.obj))) {

        // find the class that implements interface_name or try until we've found the property
        // in case of an empty interface
        if (interface_name.isEmpty()) {
            for (QDBusAdaptorConnector::AdaptorMap::ConstIterator it = connector->adaptors.constBegin(),
                 end = connector->adaptors.constEnd(); it != end; ++it) {
                const QMetaObject *mo = it->adaptor->metaObject();
                int pidx = mo->indexOfProperty(property_name);
                if (pidx != -1) {
                    value = mo->property(pidx).read(it->adaptor);
                    break;
                }
            }
        } else {
            QDBusAdaptorConnector::AdaptorMap::ConstIterator it;
            it = std::lower_bound(connector->adaptors.constBegin(), connector->adaptors.constEnd(),
                                  interface_name);
            if (it != connector->adaptors.constEnd() && interface_name == QLatin1String(it->interface)) {
                interfaceFound = true;
                value = it->adaptor->property(property_name);
            }
        }
    }

    if (!interfaceFound && !value.isValid()
        && node.flags & (QDBusConnection::ExportAllProperties |
                         QDBusConnection::ExportNonScriptableProperties)) {
        // try the object itself
        if (!interface_name.isEmpty())
            interfaceFound = qDBusInterfaceInObject(node.obj, interface_name);

        if (interfaceFound) {
            int pidx = node.obj->metaObject()->indexOfProperty(property_name);
            if (pidx != -1) {
                QMetaProperty mp = node.obj->metaObject()->property(pidx);
                if ((mp.isScriptable() && (node.flags & QDBusConnection::ExportScriptableProperties)) ||
                    (!mp.isScriptable() && (node.flags & QDBusConnection::ExportNonScriptableProperties)))
                    value = mp.read(node.obj);
            }
        }
    }

    if (!value.isValid()) {
        // the property was not found
        if (!interfaceFound)
            return interfaceNotFoundError(msg, interface_name);
        return propertyNotFoundError(msg, interface_name, property_name);
    }

    return msg.createReply(QVariant::fromValue(QDBusVariant(value)));
}

enum PropertyWriteResult {
    PropertyWriteSuccess = 0,
    PropertyNotFound,
    PropertyTypeMismatch,
    PropertyReadOnly,
    PropertyWriteFailed
};

static QDBusMessage propertyWriteReply(const QDBusMessage &msg, const QString &interface_name,
                                       const QByteArray &property_name, int status)
{
    switch (status) {
    case PropertyNotFound:
        return propertyNotFoundError(msg, interface_name, property_name);
    case PropertyTypeMismatch:
        return msg.createErrorReply(QDBusError::InvalidArgs,
                                    QString::fromLatin1("Invalid arguments for writing to property %1%2%3")
                                    .arg(interface_name,
                                         QString::fromLatin1(interface_name.isEmpty() ? "" : "."),
                                         QString::fromLatin1(property_name)));
    case PropertyReadOnly:
        return msg.createErrorReply(QDBusError::PropertyReadOnly,
                                    QString::fromLatin1("Property %1%2%3 is read-only")
                                    .arg(interface_name,
                                         QString::fromLatin1(interface_name.isEmpty() ? "" : "."),
                                         QString::fromLatin1(property_name)));
    case PropertyWriteFailed:
        return msg.createErrorReply(QDBusError::InternalError,
                                    QString::fromLatin1("Internal error"));

    case PropertyWriteSuccess:
        return msg.createReply();
    }
    Q_ASSERT_X(false, "", "Should not be reached");
    return QDBusMessage();
}

static int writeProperty(QObject *obj, const QByteArray &property_name, QVariant value,
                         int propFlags = QDBusConnection::ExportAllProperties)
{
    const QMetaObject *mo = obj->metaObject();
    int pidx = mo->indexOfProperty(property_name);
    if (pidx == -1) {
        // this object has no property by that name
        return PropertyNotFound;
    }

    QMetaProperty mp = mo->property(pidx);

    // check if this property is writable
    if (!mp.isWritable())
        return PropertyReadOnly;

    // check if this property is exported
    bool isScriptable = mp.isScriptable();
    if (!(propFlags & QDBusConnection::ExportScriptableProperties) && isScriptable)
        return PropertyNotFound;
    if (!(propFlags & QDBusConnection::ExportNonScriptableProperties) && !isScriptable)
        return PropertyNotFound;

    // we found our property
    // do we have the right type?
    int id = mp.userType();
    if (!id){
        // type not registered or invalid / void?
        qWarning("QDBusConnection: Unable to handle unregistered datatype '%s' for property '%s::%s'",
                 mp.typeName(), mo->className(), property_name.constData());
        return PropertyWriteFailed;
    }

    if (id != QMetaType::QVariant && value.userType() == QDBusMetaTypeId::argument()) {
        // we have to demarshall before writing
        void *null = 0;
        QVariant other(id, null);
        if (!QDBusMetaType::demarshall(qvariant_cast<QDBusArgument>(value), id, other.data())) {
            qWarning("QDBusConnection: type `%s' (%d) is not registered with QtDBus. "
                     "Use qDBusRegisterMetaType to register it",
                     mp.typeName(), id);
            return PropertyWriteFailed;
        }

        value = other;
    }

    // the property type here should match
    return mp.write(obj, value) ? PropertyWriteSuccess : PropertyWriteFailed;
}

QDBusMessage qDBusPropertySet(const QDBusConnectionPrivate::ObjectTreeNode &node,
                              const QDBusMessage &msg)
{
    Q_ASSERT(msg.arguments().count() == 3);
    Q_ASSERT_X(!node.obj || QThread::currentThread() == node.obj->thread(),
               "QDBusConnection: internal threading error",
               "function called for an object that is in another thread!!");

    QString interface_name = msg.arguments().at(0).toString();
    QByteArray property_name = msg.arguments().at(1).toString().toUtf8();
    QVariant value = qvariant_cast<QDBusVariant>(msg.arguments().at(2)).variant();

    QDBusAdaptorConnector *connector;
    if (node.flags & QDBusConnection::ExportAdaptors &&
        (connector = qDBusFindAdaptorConnector(node.obj))) {

        // find the class that implements interface_name or try until we've found the property
        // in case of an empty interface
        if (interface_name.isEmpty()) {
            for (QDBusAdaptorConnector::AdaptorMap::ConstIterator it = connector->adaptors.constBegin(),
                 end = connector->adaptors.constEnd(); it != end; ++it) {
                int status = writeProperty(it->adaptor, property_name, value);
                if (status == PropertyNotFound)
                    continue;
                return propertyWriteReply(msg, interface_name, property_name, status);
            }
        } else {
            QDBusAdaptorConnector::AdaptorMap::ConstIterator it;
            it = std::lower_bound(connector->adaptors.constBegin(), connector->adaptors.constEnd(),
                                  interface_name);
            if (it != connector->adaptors.end() && interface_name == QLatin1String(it->interface)) {
                return propertyWriteReply(msg, interface_name, property_name,
                                          writeProperty(it->adaptor, property_name, value));
            }
        }
    }

    if (node.flags & (QDBusConnection::ExportScriptableProperties |
                      QDBusConnection::ExportNonScriptableProperties)) {
        // try the object itself
        bool interfaceFound = true;
        if (!interface_name.isEmpty())
            interfaceFound = qDBusInterfaceInObject(node.obj, interface_name);

        if (interfaceFound) {
            return propertyWriteReply(msg, interface_name, property_name,
                                      writeProperty(node.obj, property_name, value, node.flags));
        }
    }

    // the property was not found
    if (!interface_name.isEmpty())
        return interfaceNotFoundError(msg, interface_name);
    return propertyWriteReply(msg, interface_name, property_name, PropertyNotFound);
}

// unite two QVariantMaps, but don't generate duplicate keys
static QVariantMap &operator+=(QVariantMap &lhs, const QVariantMap &rhs)
{
    QVariantMap::ConstIterator it = rhs.constBegin(),
                              end = rhs.constEnd();
    for ( ; it != end; ++it)
        lhs.insert(it.key(), it.value());
    return lhs;
}

static QVariantMap readAllProperties(QObject *object, int flags)
{
    QVariantMap result;
    const QMetaObject *mo = object->metaObject();

    // QObject has properties, so don't start from 0
    for (int i = QObject::staticMetaObject.propertyCount(); i < mo->propertyCount(); ++i) {
        QMetaProperty mp = mo->property(i);

        // is it readable?
        if (!mp.isReadable())
            continue;

        // is it a registered property?
        int typeId = mp.userType();
        if (!typeId)
            continue;
        const char *signature = QDBusMetaType::typeToSignature(typeId);
        if (!signature)
            continue;

        // is this property visible from the outside?
        if ((mp.isScriptable() && flags & QDBusConnection::ExportScriptableProperties) ||
            (!mp.isScriptable() && flags & QDBusConnection::ExportNonScriptableProperties)) {
            // yes, it's visible
            QVariant value = mp.read(object);
            if (value.isValid())
                result.insert(QString::fromLatin1(mp.name()), value);
        }
    }

    return result;
}

QDBusMessage qDBusPropertyGetAll(const QDBusConnectionPrivate::ObjectTreeNode &node,
                                 const QDBusMessage &msg)
{
    Q_ASSERT(msg.arguments().count() == 1);
    Q_ASSERT_X(!node.obj || QThread::currentThread() == node.obj->thread(),
               "QDBusConnection: internal threading error",
               "function called for an object that is in another thread!!");

    QString interface_name = msg.arguments().at(0).toString();

    bool interfaceFound = false;
    QVariantMap result;

    QDBusAdaptorConnector *connector;
    if (node.flags & QDBusConnection::ExportAdaptors &&
        (connector = qDBusFindAdaptorConnector(node.obj))) {

        if (interface_name.isEmpty()) {
            // iterate over all interfaces
            for (QDBusAdaptorConnector::AdaptorMap::ConstIterator it = connector->adaptors.constBegin(),
                 end = connector->adaptors.constEnd(); it != end; ++it) {
                result += readAllProperties(it->adaptor, QDBusConnection::ExportAllProperties);
            }
        } else {
            // find the class that implements interface_name
            QDBusAdaptorConnector::AdaptorMap::ConstIterator it;
            it = std::lower_bound(connector->adaptors.constBegin(), connector->adaptors.constEnd(),
                                  interface_name);
            if (it != connector->adaptors.constEnd() && interface_name == QLatin1String(it->interface)) {
                interfaceFound = true;
                result = readAllProperties(it->adaptor, QDBusConnection::ExportAllProperties);
            }
        }
    }

    if (node.flags & QDBusConnection::ExportAllProperties &&
        (!interfaceFound || interface_name.isEmpty())) {
        // try the object itself
        result += readAllProperties(node.obj, node.flags);
        interfaceFound = true;
    }

    if (!interfaceFound && !interface_name.isEmpty()) {
        // the interface was not found
        return interfaceNotFoundError(msg, interface_name);
    }

    return msg.createReply(QVariant::fromValue(result));
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
