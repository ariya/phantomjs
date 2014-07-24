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

#include <QtCore/qmetaobject.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qdebug.h>

#include "qdbusinterface_p.h"   // for ANNOTATION_NO_WAIT
#include "qdbusabstractadaptor_p.h" // for QCLASSINFO_DBUS_*
#include "qdbusconnection_p.h"  // for the flags
#include "qdbusmetatype_p.h"
#include "qdbusmetatype.h"
#include "qdbusutil_p.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

extern Q_DBUS_EXPORT QString qDBusGenerateMetaObjectXml(QString interface, const QMetaObject *mo,
                                                       const QMetaObject *base, int flags);

static inline QString typeNameToXml(const char *typeName)
{
    // ### copied from qtextdocument.cpp
    // ### move this into Qt Core at some point
    QString plain = QLatin1String(typeName);
    QString rich;
    rich.reserve(int(plain.length() * 1.1));
    for (int i = 0; i < plain.length(); ++i) {
        if (plain.at(i) == QLatin1Char('<'))
            rich += QLatin1String("&lt;");
        else if (plain.at(i) == QLatin1Char('>'))
            rich += QLatin1String("&gt;");
        else if (plain.at(i) == QLatin1Char('&'))
            rich += QLatin1String("&amp;");
        else
            rich += plain.at(i);
    }
    return rich;
}

static inline QLatin1String accessAsString(bool read, bool write)
{
    if (read)
        return write ? QLatin1String("readwrite") : QLatin1String("read") ;
    else
        return write ? QLatin1String("write") : QLatin1String("") ;
}

// implement the D-Bus org.freedesktop.DBus.Introspectable interface
// we do that by analysing the metaObject of all the adaptor interfaces

static QString generateInterfaceXml(const QMetaObject *mo, int flags, int methodOffset, int propOffset)
{
    QString retval;

    // start with properties:
    if (flags & (QDBusConnection::ExportScriptableProperties |
                 QDBusConnection::ExportNonScriptableProperties)) {
        for (int i = propOffset; i < mo->propertyCount(); ++i) {

            QMetaProperty mp = mo->property(i);

            if (!((mp.isScriptable() && (flags & QDBusConnection::ExportScriptableProperties)) ||
                  (!mp.isScriptable() && (flags & QDBusConnection::ExportNonScriptableProperties))))
                continue;

            int typeId = mp.userType();
            if (!typeId)
                continue;
            const char *signature = QDBusMetaType::typeToSignature(typeId);
            if (!signature)
                continue;

            retval += QString::fromLatin1("    <property name=\"%1\" type=\"%2\" access=\"%3\"")
                      .arg(QLatin1String(mp.name()),
                           QLatin1String(signature),
                           accessAsString(mp.isReadable(), mp.isWritable()));

            if (QDBusMetaType::signatureToType(signature) == QVariant::Invalid) {
                const char *typeName = QMetaType::typeName(typeId);
                retval += QString::fromLatin1(">\n      <annotation name=\"org.qtproject.QtDBus.QtTypeName\" value=\"%3\"/>\n    </property>\n")
                          .arg(typeNameToXml(typeName));
            } else {
                retval += QLatin1String("/>\n");
            }
        }
    }

    // now add methods:
    for (int i = methodOffset; i < mo->methodCount(); ++i) {
        QMetaMethod mm = mo->method(i);

        bool isSignal;
        if (mm.methodType() == QMetaMethod::Signal)
            // adding a signal
            isSignal = true;
        else if (mm.access() == QMetaMethod::Public && (mm.methodType() == QMetaMethod::Slot || mm.methodType() == QMetaMethod::Method))
            isSignal = false;
        else
            continue;           // neither signal nor public slot

        if (isSignal && !(flags & (QDBusConnection::ExportScriptableSignals |
                                   QDBusConnection::ExportNonScriptableSignals)))
            continue;           // we're not exporting any signals
        if (!isSignal && (!(flags & (QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportNonScriptableSlots)) &&
                          !(flags & (QDBusConnection::ExportScriptableInvokables | QDBusConnection::ExportNonScriptableInvokables))))
            continue;           // we're not exporting any slots or invokables

        QString xml = QString::fromLatin1("    <%1 name=\"%2\">\n")
                      .arg(isSignal ? QLatin1String("signal") : QLatin1String("method"))
                      .arg(QString::fromLatin1(mm.name()));

        // check the return type first
        int typeId = mm.returnType();
        if (typeId != QMetaType::UnknownType && typeId != QMetaType::Void) {
            const char *typeName = QDBusMetaType::typeToSignature(typeId);
            if (typeName) {
                xml += QString::fromLatin1("      <arg type=\"%1\" direction=\"out\"/>\n")
                       .arg(typeNameToXml(typeName));

                // do we need to describe this argument?
                if (QDBusMetaType::signatureToType(typeName) == QVariant::Invalid)
                    xml += QString::fromLatin1("      <annotation name=\"org.qtproject.QtDBus.QtTypeName.Out0\" value=\"%1\"/>\n")
                        .arg(typeNameToXml(QMetaType::typeName(typeId)));
            } else {
                qWarning() << "Unsupported return type" << typeId << QMetaType::typeName(typeId) << "in method" << mm.name();
                continue;
            }
        }
        else if (typeId == QMetaType::UnknownType) {
            qWarning() << "Invalid return type in method" << mm.name();
            continue;           // wasn't a valid type
        }

        QList<QByteArray> names = mm.parameterNames();
        QVector<int> types;
        QString errorMsg;
        int inputCount = qDBusParametersForMethod(mm, types, errorMsg);
        if (inputCount == -1) {
            qWarning() << "Skipped method" << mm.name() << ":" << qPrintable(errorMsg);
            continue;           // invalid form
        }
        if (isSignal && inputCount + 1 != types.count())
            continue;           // signal with output arguments?
        if (isSignal && types.at(inputCount) == QDBusMetaTypeId::message())
            continue;           // signal with QDBusMessage argument?
        if (isSignal && mm.attributes() & QMetaMethod::Cloned)
            continue;           // cloned signal?

        int j;
        bool isScriptable = mm.attributes() & QMetaMethod::Scriptable;
        for (j = 1; j < types.count(); ++j) {
            // input parameter for a slot or output for a signal
            if (types.at(j) == QDBusMetaTypeId::message()) {
                isScriptable = true;
                continue;
            }

            QString name;
            if (!names.at(j - 1).isEmpty())
                name = QString::fromLatin1("name=\"%1\" ").arg(QLatin1String(names.at(j - 1)));

            bool isOutput = isSignal || j > inputCount;

            const char *signature = QDBusMetaType::typeToSignature(types.at(j));
            xml += QString::fromLatin1("      <arg %1type=\"%2\" direction=\"%3\"/>\n")
                   .arg(name)
                   .arg(QLatin1String(signature))
                   .arg(isOutput ? QLatin1String("out") : QLatin1String("in"));

            // do we need to describe this argument?
            if (QDBusMetaType::signatureToType(signature) == QVariant::Invalid) {
                const char *typeName = QMetaType::typeName(types.at(j));
                xml += QString::fromLatin1("      <annotation name=\"org.qtproject.QtDBus.QtTypeName.%1%2\" value=\"%3\"/>\n")
                       .arg(isOutput ? QLatin1String("Out") : QLatin1String("In"))
                       .arg(isOutput && !isSignal ? j - inputCount : j - 1)
                       .arg(typeNameToXml(typeName));
            }
        }

        int wantedMask;
        if (isScriptable)
            wantedMask = isSignal ? QDBusConnection::ExportScriptableSignals
                                  : QDBusConnection::ExportScriptableSlots;
        else
            wantedMask = isSignal ? QDBusConnection::ExportNonScriptableSignals
                                  : QDBusConnection::ExportNonScriptableSlots;
        if ((flags & wantedMask) != wantedMask)
            continue;

        if (qDBusCheckAsyncTag(mm.tag()))
            // add the no-reply annotation
            xml += QLatin1String("      <annotation name=\"" ANNOTATION_NO_WAIT "\""
                                 " value=\"true\"/>\n");

        retval += xml;
        retval += QString::fromLatin1("    </%1>\n")
                  .arg(isSignal ? QLatin1String("signal") : QLatin1String("method"));
    }

    return retval;
}

QString qDBusGenerateMetaObjectXml(QString interface, const QMetaObject *mo,
                                   const QMetaObject *base, int flags)
{
    if (interface.isEmpty())
        // generate the interface name from the meta object
        interface = qDBusInterfaceFromMetaObject(mo);

    QString xml;
    int idx = mo->indexOfClassInfo(QCLASSINFO_DBUS_INTROSPECTION);
    if (idx >= mo->classInfoOffset())
        return QString::fromUtf8(mo->classInfo(idx).value());
    else
        xml = generateInterfaceXml(mo, flags, base->methodCount(), base->propertyCount());

    if (xml.isEmpty())
        return QString();       // don't add an empty interface
    return QString::fromLatin1("  <interface name=\"%1\">\n%2  </interface>\n")
        .arg(interface, xml);
}
#if 0
QString qDBusGenerateMetaObjectXml(QString interface, const QMetaObject *mo, const QMetaObject *base,
                                   int flags)
{
    if (interface.isEmpty()) {
        // generate the interface name from the meta object
        int idx = mo->indexOfClassInfo(QCLASSINFO_DBUS_INTERFACE);
        if (idx >= mo->classInfoOffset()) {
            interface = QLatin1String(mo->classInfo(idx).value());
        } else {
            interface = QLatin1String(mo->className());
            interface.replace(QLatin1String("::"), QLatin1String("."));

            if (interface.startsWith(QLatin1String("QDBus"))) {
                interface.prepend(QLatin1String("org.qtproject.QtDBus."));
            } else if (interface.startsWith(QLatin1Char('Q')) &&
                       interface.length() >= 2 && interface.at(1).isUpper()) {
                // assume it's Qt
                interface.prepend(QLatin1String("org.qtproject.Qt."));
            } else if (!QCoreApplication::instance()||
                       QCoreApplication::instance()->applicationName().isEmpty()) {
                interface.prepend(QLatin1String("local."));
            } else {
                interface.prepend(QLatin1Char('.')).prepend(QCoreApplication::instance()->applicationName());
                QStringList domainName =
                    QCoreApplication::instance()->organizationDomain().split(QLatin1Char('.'),
                                                                             QString::SkipEmptyParts);
                if (domainName.isEmpty())
                    interface.prepend(QLatin1String("local."));
                else
                    for (int i = 0; i < domainName.count(); ++i)
                        interface.prepend(QLatin1Char('.')).prepend(domainName.at(i));
            }
        }
    }

    QString xml;
    int idx = mo->indexOfClassInfo(QCLASSINFO_DBUS_INTROSPECTION);
    if (idx >= mo->classInfoOffset())
        return QString::fromUtf8(mo->classInfo(idx).value());
    else
        xml = generateInterfaceXml(mo, flags, base->methodCount(), base->propertyCount());

    if (xml.isEmpty())
        return QString();       // don't add an empty interface
    return QString::fromLatin1("  <interface name=\"%1\">\n%2  </interface>\n")
        .arg(interface, xml);
}

#endif

QT_END_NAMESPACE

#endif // QT_NO_DBUS
