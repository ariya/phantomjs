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

#include "qdbusmetaobject_p.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qhash.h>
#include <QtCore/qstring.h>
#include <QtCore/qvarlengtharray.h>

#include "qdbusutil_p.h"
#include "qdbuserror.h"
#include "qdbusmetatype.h"
#include "qdbusargument.h"
#include "qdbusintrospection_p.h"
#include "qdbusabstractinterface_p.h"

#include <private/qmetaobject_p.h>
#include <private/qmetaobjectbuilder_p.h>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class QDBusMetaObjectGenerator
{
public:
    QDBusMetaObjectGenerator(const QString &interface,
                             const QDBusIntrospection::Interface *parsedData);
    void write(QDBusMetaObject *obj);
    void writeWithoutXml(QDBusMetaObject *obj);

private:
    struct Method {
        QList<QByteArray> parameterNames;
        QByteArray tag;
        QByteArray name;
        QVarLengthArray<int, 4> inputTypes;
        QVarLengthArray<int, 4> outputTypes;
        QByteArray rawReturnType;
        int flags;
    };

    struct Property {
        QByteArray typeName;
        QByteArray signature;
        int type;
        int flags;
    };
    struct Type {
        int id;
        QByteArray name;
    };

    QMap<QByteArray, Method> signals_;
    QMap<QByteArray, Method> methods;
    QMap<QByteArray, Property> properties;

    const QDBusIntrospection::Interface *data;
    QString interface;

    Type findType(const QByteArray &signature,
                  const QDBusIntrospection::Annotations &annotations,
                  const char *direction = "Out", int id = -1);

    void parseMethods();
    void parseSignals();
    void parseProperties();

    static int aggregateParameterCount(const QMap<QByteArray, Method> &map);
};

static const int intsPerProperty = 2;
static const int intsPerMethod = 2;

struct QDBusMetaObjectPrivate : public QMetaObjectPrivate
{
    int propertyDBusData;
    int methodDBusData;
};

QDBusMetaObjectGenerator::QDBusMetaObjectGenerator(const QString &interfaceName,
                                                   const QDBusIntrospection::Interface *parsedData)
    : data(parsedData), interface(interfaceName)
{
    if (data) {
        parseProperties();
        parseSignals();             // call parseSignals first so that slots override signals
        parseMethods();
    }
}

static int registerComplexDBusType(const char *typeName)
{
    struct QDBusRawTypeHandler {
        static void destroy(void *)
        {
            qFatal("Cannot destroy placeholder type QDBusRawType");
        }

        static void *create(const void *)
        {
            qFatal("Cannot create placeholder type QDBusRawType");
            return 0;
        }

        static void destruct(void *)
        {
            qFatal("Cannot destruct placeholder type QDBusRawType");
        }

        static void *construct(void *, const void *)
        {
            qFatal("Cannot construct placeholder type QDBusRawType");
            return 0;
        }
    };

    return QMetaType::registerNormalizedType(typeName, QDBusRawTypeHandler::destroy,
                                             QDBusRawTypeHandler::create,
                                             QDBusRawTypeHandler::destruct,
                                             QDBusRawTypeHandler::construct,
                                             sizeof(void *),
                                             QMetaType::MovableType,
                                             0);
}

Q_DBUS_EXPORT bool qt_dbus_metaobject_skip_annotations = false;

QDBusMetaObjectGenerator::Type
QDBusMetaObjectGenerator::findType(const QByteArray &signature,
                                   const QDBusIntrospection::Annotations &annotations,
                                   const char *direction, int id)
{
    Type result;
    result.id = QVariant::Invalid;

    int type = QDBusMetaType::signatureToType(signature);
    if (type == QVariant::Invalid && !qt_dbus_metaobject_skip_annotations) {
        // it's not a type normally handled by our meta type system
        // it must contain an annotation
        QString annotationName = QString::fromLatin1("org.qtproject.QtDBus.QtTypeName");
        if (id >= 0)
            annotationName += QString::fromLatin1(".%1%2")
                              .arg(QLatin1String(direction))
                              .arg(id);

        // extract from annotations:
        QByteArray typeName = annotations.value(annotationName).toLatin1();

        // verify that it's a valid one
        if (typeName.isEmpty()) {
            // try the old annotation from Qt 4
            annotationName = QString::fromLatin1("com.trolltech.QtDBus.QtTypeName");
            if (id >= 0)
                annotationName += QString::fromLatin1(".%1%2")
                                  .arg(QLatin1String(direction))
                                  .arg(id);
            typeName = annotations.value(annotationName).toLatin1();
        }

        if (!typeName.isEmpty()) {
            // type name found
            type = QMetaType::type(typeName);
        }

        if (type == QVariant::Invalid || signature != QDBusMetaType::typeToSignature(type)) {
            // type is still unknown or doesn't match back to the signature that it
            // was expected to, so synthesize a fake type
            typeName = "QDBusRawType<0x" + signature.toHex() + ">*";
            type = registerComplexDBusType(typeName);
        }

        result.name = typeName;
    } else if (type == QVariant::Invalid) {
        // this case is used only by the qdbus command-line tool
        // invalid, let's create an impossible type that contains the signature

        if (signature == "av") {
            result.name = "QVariantList";
            type = QVariant::List;
        } else if (signature == "a{sv}") {
            result.name = "QVariantMap";
            type = QVariant::Map;
        } else if (signature == "a{ss}") {
            result.name = "QMap<QString,QString>";
            type = qMetaTypeId<QMap<QString, QString> >();
        } else {
            result.name = "{D-Bus type \"" + signature + "\"}";
            type = registerComplexDBusType(result.name);
        }
    } else {
        result.name = QMetaType::typeName(type);
    }

    result.id = type;
    return result;              // success
}

void QDBusMetaObjectGenerator::parseMethods()
{
    //
    // TODO:
    //  Add cloned methods when the remote object has return types
    //

    QDBusIntrospection::Methods::ConstIterator method_it = data->methods.constBegin();
    QDBusIntrospection::Methods::ConstIterator method_end = data->methods.constEnd();
    for ( ; method_it != method_end; ++method_it) {
        const QDBusIntrospection::Method &m = *method_it;
        Method mm;

        mm.name = m.name.toLatin1();
        QByteArray prototype = mm.name;
        prototype += '(';

        bool ok = true;

        // build the input argument list
        for (int i = 0; i < m.inputArgs.count(); ++i) {
            const QDBusIntrospection::Argument &arg = m.inputArgs.at(i);

            Type type = findType(arg.type.toLatin1(), m.annotations, "In", i);
            if (type.id == QVariant::Invalid) {
                ok = false;
                break;
            }

            mm.inputTypes.append(type.id);

            mm.parameterNames.append(arg.name.toLatin1());

            prototype.append(type.name);
            prototype.append(',');
        }
        if (!ok) continue;

        // build the output argument list:
        for (int i = 0; i < m.outputArgs.count(); ++i) {
            const QDBusIntrospection::Argument &arg = m.outputArgs.at(i);

            Type type = findType(arg.type.toLatin1(), m.annotations, "Out", i);
            if (type.id == QVariant::Invalid) {
                ok = false;
                break;
            }

            mm.outputTypes.append(type.id);

            if (i == 0 && type.id == -1) {
                mm.rawReturnType = type.name;
            }
            if (i != 0) {
                // non-const ref parameter
                mm.parameterNames.append(arg.name.toLatin1());

                prototype.append(type.name);
                prototype.append("&,");
            }
        }
        if (!ok) continue;

        // convert the last commas:
        if (!mm.parameterNames.isEmpty())
            prototype[prototype.length() - 1] = ')';
        else
            prototype.append(')');

        // check the async tag
        if (m.annotations.value(QLatin1String(ANNOTATION_NO_WAIT)) == QLatin1String("true"))
            mm.tag = "Q_NOREPLY";

        // meta method flags
        mm.flags = AccessPublic | MethodSlot | MethodScriptable;

        // add
        methods.insert(QMetaObject::normalizedSignature(prototype), mm);
    }
}

void QDBusMetaObjectGenerator::parseSignals()
{
    QDBusIntrospection::Signals::ConstIterator signal_it = data->signals_.constBegin();
    QDBusIntrospection::Signals::ConstIterator signal_end = data->signals_.constEnd();
    for ( ; signal_it != signal_end; ++signal_it) {
        const QDBusIntrospection::Signal &s = *signal_it;
        Method mm;

        mm.name = s.name.toLatin1();
        QByteArray prototype = mm.name;
        prototype += '(';

        bool ok = true;

        // build the output argument list
        for (int i = 0; i < s.outputArgs.count(); ++i) {
            const QDBusIntrospection::Argument &arg = s.outputArgs.at(i);

            Type type = findType(arg.type.toLatin1(), s.annotations, "Out", i);
            if (type.id == QVariant::Invalid) {
                ok = false;
                break;
            }

            mm.inputTypes.append(type.id);

            mm.parameterNames.append(arg.name.toLatin1());

            prototype.append(type.name);
            prototype.append(',');
        }
        if (!ok) continue;

        // convert the last commas:
        if (!mm.parameterNames.isEmpty())
            prototype[prototype.length() - 1] = ')';
        else
            prototype.append(')');

        // meta method flags
        mm.flags = AccessPublic | MethodSignal | MethodScriptable;

        // add
        signals_.insert(QMetaObject::normalizedSignature(prototype), mm);
    }
}

void QDBusMetaObjectGenerator::parseProperties()
{
    QDBusIntrospection::Properties::ConstIterator prop_it = data->properties.constBegin();
    QDBusIntrospection::Properties::ConstIterator prop_end = data->properties.constEnd();
    for ( ; prop_it != prop_end; ++prop_it) {
        const QDBusIntrospection::Property &p = *prop_it;
        Property mp;
        Type type = findType(p.type.toLatin1(), p.annotations);
        if (type.id == QVariant::Invalid)
            continue;

        QByteArray name = p.name.toLatin1();
        mp.signature = p.type.toLatin1();
        mp.type = type.id;
        mp.typeName = type.name;

        // build the flags:
        mp.flags = StdCppSet | Scriptable | Stored | Designable;
        if (p.access != QDBusIntrospection::Property::Write)
            mp.flags |= Readable;
        if (p.access != QDBusIntrospection::Property::Read)
            mp.flags |= Writable;

        // add the property:
        properties.insert(name, mp);
    }
}

// Returns the sum of all parameters (including return type) for the given
// \a map of methods. This is needed for calculating the size of the methods'
// parameter type/name meta-data.
int QDBusMetaObjectGenerator::aggregateParameterCount(const QMap<QByteArray, Method> &map)
{
    int sum = 0;
    QMap<QByteArray, Method>::const_iterator it;
    for (it = map.constBegin(); it != map.constEnd(); ++it) {
        const Method &m = it.value();
        sum += m.inputTypes.size() + qMax(1, m.outputTypes.size());
    }
    return sum;
}

void QDBusMetaObjectGenerator::write(QDBusMetaObject *obj)
{
    // this code here is mostly copied from qaxbase.cpp
    // with a few modifications to make it cleaner

    QString className = interface;
    className.replace(QLatin1Char('.'), QLatin1String("::"));
    if (className.isEmpty())
        className = QLatin1String("QDBusInterface");

    QVarLengthArray<int> idata;
    idata.resize(sizeof(QDBusMetaObjectPrivate) / sizeof(int));

    int methodParametersDataSize =
            ((aggregateParameterCount(signals_)
             + aggregateParameterCount(methods)) * 2) // types and parameter names
            - signals_.count() // return "parameters" don't have names
            - methods.count(); // ditto

    QDBusMetaObjectPrivate *header = reinterpret_cast<QDBusMetaObjectPrivate *>(idata.data());
    Q_STATIC_ASSERT_X(QMetaObjectPrivate::OutputRevision == 7, "QtDBus meta-object generator should generate the same version as moc");
    header->revision = QMetaObjectPrivate::OutputRevision;
    header->className = 0;
    header->classInfoCount = 0;
    header->classInfoData = 0;
    header->methodCount = signals_.count() + methods.count();
    header->methodData = idata.size();
    header->propertyCount = properties.count();
    header->propertyData = header->methodData + header->methodCount * 5 + methodParametersDataSize;
    header->enumeratorCount = 0;
    header->enumeratorData = 0;
    header->constructorCount = 0;
    header->constructorData = 0;
    header->flags = RequiresVariantMetaObject;
    header->signalCount = signals_.count();
    // These are specific to QDBusMetaObject:
    header->propertyDBusData = header->propertyData + header->propertyCount * 3;
    header->methodDBusData = header->propertyDBusData + header->propertyCount * intsPerProperty;

    int data_size = idata.size() +
                    (header->methodCount * (5+intsPerMethod)) + methodParametersDataSize +
                    (header->propertyCount * (3+intsPerProperty));
    foreach (const Method &mm, signals_)
        data_size += 2 + mm.inputTypes.count() + mm.outputTypes.count();
    foreach (const Method &mm, methods)
        data_size += 2 + mm.inputTypes.count() + mm.outputTypes.count();
    idata.resize(data_size + 1);

    QMetaStringTable strings(className.toLatin1());

    int offset = header->methodData;
    int parametersOffset = offset + header->methodCount * 5;
    int signatureOffset = header->methodDBusData;
    int typeidOffset = header->methodDBusData + header->methodCount * intsPerMethod;
    idata[typeidOffset++] = 0;                           // eod

    // add each method:
    for (int x = 0; x < 2; ++x) {
        // Signals must be added before other methods, to match moc.
        QMap<QByteArray, Method> &map = (x == 0) ? signals_ : methods;
        for (QMap<QByteArray, Method>::ConstIterator it = map.constBegin();
             it != map.constEnd(); ++it) {
            const Method &mm = it.value();

            int argc = mm.inputTypes.size() + qMax(0, mm.outputTypes.size() - 1);

            idata[offset++] = strings.enter(mm.name);
            idata[offset++] = argc;
            idata[offset++] = parametersOffset;
            idata[offset++] = strings.enter(mm.tag);
            idata[offset++] = mm.flags;

            // Parameter types
            for (int i = -1; i < argc; ++i) {
                int type;
                QByteArray typeName;
                if (i < 0) { // Return type
                    if (!mm.outputTypes.isEmpty()) {
                        type = mm.outputTypes.first();
                        if (type == -1) {
                            type = IsUnresolvedType | strings.enter(mm.rawReturnType);
                        }
                    } else {
                        type = QMetaType::Void;
                    }
                } else if (i < mm.inputTypes.size()) {
                    type = mm.inputTypes.at(i);
                } else {
                    Q_ASSERT(mm.outputTypes.size() > 1);
                    type = mm.outputTypes.at(i - mm.inputTypes.size() + 1);
                    // Output parameters are references; type id not available
                    typeName = QMetaType::typeName(type);
                    typeName.append('&');
                }
                Q_ASSERT(type != QMetaType::UnknownType);
                int typeInfo;
                if (!typeName.isEmpty())
                    typeInfo = IsUnresolvedType | strings.enter(typeName);
                else
                    typeInfo = type;
                idata[parametersOffset++] = typeInfo;
            }
            // Parameter names
            for (int i = 0; i < argc; ++i)
                idata[parametersOffset++] = strings.enter(mm.parameterNames.at(i));

            idata[signatureOffset++] = typeidOffset;
            idata[typeidOffset++] = mm.inputTypes.count();
            memcpy(idata.data() + typeidOffset, mm.inputTypes.data(), mm.inputTypes.count() * sizeof(int));
            typeidOffset += mm.inputTypes.count();

            idata[signatureOffset++] = typeidOffset;
            idata[typeidOffset++] = mm.outputTypes.count();
            memcpy(idata.data() + typeidOffset, mm.outputTypes.data(), mm.outputTypes.count() * sizeof(int));
            typeidOffset += mm.outputTypes.count();
        }
    }

    Q_ASSERT(offset == header->methodData + header->methodCount * 5);
    Q_ASSERT(parametersOffset = header->propertyData);
    Q_ASSERT(signatureOffset == header->methodDBusData + header->methodCount * intsPerMethod);
    Q_ASSERT(typeidOffset == idata.size());
    offset += methodParametersDataSize;
    Q_ASSERT(offset == header->propertyData);

    // add each property
    signatureOffset = header->propertyDBusData;
    for (QMap<QByteArray, Property>::ConstIterator it = properties.constBegin();
         it != properties.constEnd(); ++it) {
        const Property &mp = it.value();

        // form is name, typeinfo, flags
        idata[offset++] = strings.enter(it.key()); // name
        Q_ASSERT(mp.type != QMetaType::UnknownType);
        idata[offset++] = mp.type;
        idata[offset++] = mp.flags;

        idata[signatureOffset++] = strings.enter(mp.signature);
        idata[signatureOffset++] = mp.type;
    }

    Q_ASSERT(offset == header->propertyDBusData);
    Q_ASSERT(signatureOffset == header->methodDBusData);

    char *string_data = new char[strings.blobSize()];
    strings.writeBlob(string_data);

    uint *uint_data = new uint[idata.size()];
    memcpy(uint_data, idata.data(), idata.size() * sizeof(int));

    // put the metaobject together
    obj->d.data = uint_data;
    obj->d.relatedMetaObjects = 0;
    obj->d.static_metacall = 0;
    obj->d.extradata = 0;
    obj->d.stringdata = reinterpret_cast<const QByteArrayData *>(string_data);
    obj->d.superdata = &QDBusAbstractInterface::staticMetaObject;
}

#if 0
void QDBusMetaObjectGenerator::writeWithoutXml(const QString &interface)
{
    // no XML definition
    QString tmp(interface);
    tmp.replace(QLatin1Char('.'), QLatin1String("::"));
    QByteArray name(tmp.toLatin1());

    QDBusMetaObjectPrivate *header = new QDBusMetaObjectPrivate;
    memset(header, 0, sizeof *header);
    header->revision = 1;
    // leave the rest with 0

    char *stringdata = new char[name.length() + 1];
    stringdata[name.length()] = '\0';

    d.data = reinterpret_cast<uint*>(header);
    d.relatedMetaObjects = 0;
    d.static_metacall = 0;
    d.extradata = 0;
    d.stringdata = stringdata;
    d.superdata = &QDBusAbstractInterface::staticMetaObject;
    cached = false;
}
#endif

/////////
// class QDBusMetaObject

QDBusMetaObject *QDBusMetaObject::createMetaObject(const QString &interface, const QString &xml,
                                                   QHash<QString, QDBusMetaObject *> &cache,
                                                   QDBusError &error)
{
    error = QDBusError();
    QDBusIntrospection::Interfaces parsed = QDBusIntrospection::parseInterfaces(xml);

    QDBusMetaObject *we = 0;
    QDBusIntrospection::Interfaces::ConstIterator it = parsed.constBegin();
    QDBusIntrospection::Interfaces::ConstIterator end = parsed.constEnd();
    for ( ; it != end; ++it) {
        // check if it's in the cache
        bool us = it.key() == interface;

        QDBusMetaObject *obj = cache.value(it.key(), 0);
        if ( !obj && ( us || !interface.startsWith( QLatin1String("local.") ) ) ) {
            // not in cache; create
            obj = new QDBusMetaObject;
            QDBusMetaObjectGenerator generator(it.key(), it.value().constData());
            generator.write(obj);

            if ( (obj->cached = !it.key().startsWith( QLatin1String("local.") )) )
                // cache it
                cache.insert(it.key(), obj);
            else if (!us)
                delete obj;

        }

        if (us)
            // it's us
            we = obj;
    }

    if (we)
        return we;
    // still nothing?

    if (parsed.isEmpty()) {
        // object didn't return introspection
        we = new QDBusMetaObject;
        QDBusMetaObjectGenerator generator(interface, 0);
        generator.write(we);
        we->cached = false;
        return we;
    } else if (interface.isEmpty()) {
        // merge all interfaces
        it = parsed.constBegin();
        QDBusIntrospection::Interface merged = *it.value().constData();

        for (++it; it != end; ++it) {
            merged.annotations.unite(it.value()->annotations);
            merged.methods.unite(it.value()->methods);
            merged.signals_.unite(it.value()->signals_);
            merged.properties.unite(it.value()->properties);
        }

        merged.name = QLatin1String("local.Merged");
        merged.introspection.clear();

        we = new QDBusMetaObject;
        QDBusMetaObjectGenerator generator(merged.name, &merged);
        generator.write(we);
        we->cached = false;
        return we;
    }

    // mark as an error
    error = QDBusError(QDBusError::UnknownInterface,
        QString::fromLatin1("Interface '%1' was not found")
                       .arg(interface));
    return 0;
}

QDBusMetaObject::QDBusMetaObject()
{
}

static inline const QDBusMetaObjectPrivate *priv(const uint* data)
{
    return reinterpret_cast<const QDBusMetaObjectPrivate *>(data);
}

const int *QDBusMetaObject::inputTypesForMethod(int id) const
{
    //id -= methodOffset();
    if (id >= 0 && id < priv(d.data)->methodCount) {
        int handle = priv(d.data)->methodDBusData + id*intsPerMethod;
        return reinterpret_cast<const int*>(d.data + d.data[handle]);
    }
    return 0;
}

const int *QDBusMetaObject::outputTypesForMethod(int id) const
{
    //id -= methodOffset();
    if (id >= 0 && id < priv(d.data)->methodCount) {
        int handle = priv(d.data)->methodDBusData + id*intsPerMethod;
        return reinterpret_cast<const int*>(d.data + d.data[handle + 1]);
    }
    return 0;
}

int QDBusMetaObject::propertyMetaType(int id) const
{
    //id -= propertyOffset();
    if (id >= 0 && id < priv(d.data)->propertyCount) {
        int handle = priv(d.data)->propertyDBusData + id*intsPerProperty;
        return d.data[handle + 1];
    }
    return 0;
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
