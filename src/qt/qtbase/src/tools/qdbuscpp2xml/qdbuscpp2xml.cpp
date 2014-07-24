/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include <qbytearray.h>
#include <qstring.h>
#include <qvarlengtharray.h>
#include <qfile.h>
#include <qlist.h>
#include <qbuffer.h>
#include <qregexp.h>
#include <qvector.h>
#include <qdebug.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <qdbusconnection.h>    // for the Export* flags
#include <private/qdbusconnection_p.h>    // for the qDBusCheckAsyncTag

// copied from dbus-protocol.h:
static const char docTypeHeader[] =
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
    "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n";

#define ANNOTATION_NO_WAIT      "org.freedesktop.DBus.Method.NoReply"
#define QCLASSINFO_DBUS_INTERFACE       "D-Bus Interface"
#define QCLASSINFO_DBUS_INTROSPECTION   "D-Bus Introspection"

#include <qdbusmetatype.h>
#include <private/qdbusmetatype_p.h>
#include <private/qdbusutil_p.h>

#include "moc.h"
#include "generator.h"
#include "preprocessor.h"

#define PROGRAMNAME     "qdbuscpp2xml"
#define PROGRAMVERSION  "0.2"
#define PROGRAMCOPYRIGHT "Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies)."

static QString outputFile;
static int flags;

static const char help[] =
    "Usage: " PROGRAMNAME " [options...] [files...]\n"
    "Parses the C++ source or header file containing a QObject-derived class and\n"
    "produces the D-Bus Introspection XML."
    "\n"
    "Options:\n"
    "  -p|-s|-m       Only parse scriptable Properties, Signals and Methods (slots)\n"
    "  -P|-S|-M       Parse all Properties, Signals and Methods (slots)\n"
    "  -a             Output all scriptable contents (equivalent to -psm)\n"
    "  -A             Output all contents (equivalent to -PSM)\n"
    "  -o <filename>  Write the output to file <filename>\n"
    "  -h             Show this information\n"
    "  -V             Show the program version and quit.\n"
    "\n";


int qDBusParametersForMethod(const FunctionDef &mm, QVector<int>& metaTypes, QString &errorMsg)
{
    QList<QByteArray> parameterTypes;

    foreach (const ArgumentDef &arg, mm.arguments)
        parameterTypes.append(arg.normalizedType);

    return qDBusParametersForMethod(parameterTypes, metaTypes, errorMsg);
}


static inline QString typeNameToXml(const char *typeName)
{
    QString plain = QLatin1String(typeName);
    return plain.toHtmlEscaped();
}

static QString addFunction(const FunctionDef &mm, bool isSignal = false) {

    QString xml = QString::fromLatin1("    <%1 name=\"%2\">\n")
                  .arg(isSignal ? QLatin1String("signal") : QLatin1String("method"))
                  .arg(QLatin1String(mm.name));

    // check the return type first
    int typeId = QMetaType::type(mm.normalizedType.constData());
    if (typeId != QMetaType::Void) {
        if (typeId) {
            const char *typeName = QDBusMetaType::typeToSignature(typeId);
            if (typeName) {
                xml += QString::fromLatin1("      <arg type=\"%1\" direction=\"out\"/>\n")
                        .arg(typeNameToXml(typeName));

                    // do we need to describe this argument?
                    if (QDBusMetaType::signatureToType(typeName) == QVariant::Invalid)
                        xml += QString::fromLatin1("      <annotation name=\"org.qtproject.QtDBus.QtTypeName.Out0\" value=\"%1\"/>\n")
                            .arg(typeNameToXml(mm.normalizedType.constData()));
            } else {
                return QString();
            }
        } else if (!mm.normalizedType.isEmpty()) {
            return QString();           // wasn't a valid type
        }
    }
    QList<ArgumentDef> names = mm.arguments;
    QVector<int> types;
    QString errorMsg;
    int inputCount = qDBusParametersForMethod(mm, types, errorMsg);
    if (inputCount == -1) {
        qWarning() << qPrintable(errorMsg);
        return QString();           // invalid form
    }
    if (isSignal && inputCount + 1 != types.count())
        return QString();           // signal with output arguments?
    if (isSignal && types.at(inputCount) == QDBusMetaTypeId::message())
        return QString();           // signal with QDBusMessage argument?

    bool isScriptable = mm.isScriptable;
    for (int j = 1; j < types.count(); ++j) {
        // input parameter for a slot or output for a signal
        if (types.at(j) == QDBusMetaTypeId::message()) {
            isScriptable = true;
            continue;
        }

        QString name;
        if (!names.at(j - 1).name.isEmpty())
            name = QString::fromLatin1("name=\"%1\" ").arg(QString::fromLatin1(names.at(j - 1).name));

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
        return QString();

    if (qDBusCheckAsyncTag(mm.tag.constData()))
        // add the no-reply annotation
        xml += QLatin1String("      <annotation name=\"" ANNOTATION_NO_WAIT "\""
                              " value=\"true\"/>\n");

    QString retval = xml;
    retval += QString::fromLatin1("    </%1>\n")
              .arg(isSignal ? QLatin1String("signal") : QLatin1String("method"));

    return retval;
}


static QString generateInterfaceXml(const ClassDef *mo)
{
    QString retval;

    // start with properties:
    if (flags & (QDBusConnection::ExportScriptableProperties |
                 QDBusConnection::ExportNonScriptableProperties)) {
        static const char *accessvalues[] = {0, "read", "write", "readwrite"};
        foreach (const PropertyDef &mp, mo->propertyList) {
            if (!((!mp.scriptable.isEmpty() && (flags & QDBusConnection::ExportScriptableProperties)) ||
                  (!mp.scriptable.isEmpty() && (flags & QDBusConnection::ExportNonScriptableProperties))))
                continue;

            int access = 0;
            if (!mp.read.isEmpty())
                access |= 1;
            if (!mp.write.isEmpty())
                access |= 2;

            int typeId = QMetaType::type(mp.type.constData());
            if (!typeId)
                continue;
            const char *signature = QDBusMetaType::typeToSignature(typeId);
            if (!signature)
                continue;

            retval += QString::fromLatin1("    <property name=\"%1\" type=\"%2\" access=\"%3\"")
                      .arg(QLatin1String(mp.name))
                      .arg(QLatin1String(signature))
                      .arg(QLatin1String(accessvalues[access]));

            if (QDBusMetaType::signatureToType(signature) == QVariant::Invalid) {
                retval += QString::fromLatin1(">\n      <annotation name=\"org.qtproject.QtDBus.QtTypeName\" value=\"%3\"/>\n    </property>\n")
                          .arg(typeNameToXml(mp.type.constData()));
            } else {
                retval += QLatin1String("/>\n");
            }
        }
    }

    // now add methods:

    if (flags & (QDBusConnection::ExportScriptableSignals | QDBusConnection::ExportNonScriptableSignals)) {
        foreach (const FunctionDef &mm, mo->signalList) {
            if (mm.wasCloned)
                continue;

            retval += addFunction(mm, true);
        }
    }

    if (flags & (QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportNonScriptableSlots)) {
        foreach (const FunctionDef &slot, mo->slotList) {
            if (slot.access == FunctionDef::Public)
              retval += addFunction(slot);
        }
        foreach (const FunctionDef &method, mo->methodList) {
            if (method.access == FunctionDef::Public)
              retval += addFunction(method);
        }
    }
    return retval;
}

QString qDBusInterfaceFromClassDef(const ClassDef *mo)
{
    QString interface;

    foreach (const ClassInfoDef &cid, mo->classInfoList) {
        if (cid.name == QCLASSINFO_DBUS_INTERFACE)
            return QString::fromUtf8(cid.value);
    }
    interface = QLatin1String(mo->classname);
    interface.replace(QLatin1String("::"), QLatin1String("."));

    if (interface.startsWith(QLatin1String("QDBus"))) {
        interface.prepend(QLatin1String("org.qtproject.QtDBus."));
    } else if (interface.startsWith(QLatin1Char('Q')) &&
                interface.length() >= 2 && interface.at(1).isUpper()) {
        // assume it's Qt
        interface.prepend(QLatin1String("local.org.qtproject.Qt."));
    } else {
        interface.prepend(QLatin1String("local."));
    }

    return interface;
}


QString qDBusGenerateClassDefXml(const ClassDef *cdef)
{
    foreach (const ClassInfoDef &cid, cdef->classInfoList) {
        if (cid.name == QCLASSINFO_DBUS_INTROSPECTION)
            return QString::fromUtf8(cid.value);
    }

    // generate the interface name from the meta object
    QString interface = qDBusInterfaceFromClassDef(cdef);

    QString xml = generateInterfaceXml(cdef);

    if (xml.isEmpty())
        return QString();       // don't add an empty interface
    return QString::fromLatin1("  <interface name=\"%1\">\n%2  </interface>\n")
        .arg(interface, xml);
}

static void showHelp()
{
    printf("%s", help);
    exit(0);
}

static void showVersion()
{
    printf("%s version %s\n", PROGRAMNAME, PROGRAMVERSION);
    printf("D-Bus QObject-to-XML converter\n");
    exit(0);
}

static void parseCmdLine(QStringList &arguments)
{
    flags = 0;
    for (int i = 0; i < arguments.count(); ++i) {
        const QString arg = arguments.at(i);

        if (arg == QLatin1String("--help"))
            showHelp();

        if (!arg.startsWith(QLatin1Char('-')))
            continue;

        char c = arg.count() == 2 ? arg.at(1).toLatin1() : char(0);
        switch (c) {
        case 'P':
            flags |= QDBusConnection::ExportNonScriptableProperties;
            // fall through
        case 'p':
            flags |= QDBusConnection::ExportScriptableProperties;
            break;

        case 'S':
            flags |= QDBusConnection::ExportNonScriptableSignals;
            // fall through
        case 's':
            flags |= QDBusConnection::ExportScriptableSignals;
            break;

        case 'M':
            flags |= QDBusConnection::ExportNonScriptableSlots;
            // fall through
        case 'm':
            flags |= QDBusConnection::ExportScriptableSlots;
            break;

        case 'A':
            flags |= QDBusConnection::ExportNonScriptableContents;
            // fall through
        case 'a':
            flags |= QDBusConnection::ExportScriptableContents;
            break;

        case 'o':
            if (arguments.count() < i + 2 || arguments.at(i + 1).startsWith(QLatin1Char('-'))) {
                printf("-o expects a filename\n");
                exit(1);
            }
            outputFile = arguments.takeAt(i + 1);
            break;

        case 'h':
        case '?':
            showHelp();
            break;

        case 'V':
            showVersion();
            break;

        default:
            printf("unknown option: \"%s\"\n", qPrintable(arg));
            exit(1);
        }
    }

    if (flags == 0)
        flags = QDBusConnection::ExportScriptableContents
                | QDBusConnection::ExportNonScriptableContents;
}

int main(int argc, char **argv)
{
    QStringList args;
    for (int n = 1; n < argc; ++n)
        args.append(QString::fromLocal8Bit(argv[n]));
    parseCmdLine(args);

    QList<ClassDef> classes;

    for (int i = 0; i < args.count(); ++i) {
        const QString arg = args.at(i);

        if (arg.startsWith(QLatin1Char('-')))
            continue;

        QFile f(arg);
        if (!f.open(QIODevice::ReadOnly|QIODevice::Text)) {
            fprintf(stderr, PROGRAMNAME ": could not open '%s': %s\n",
                    qPrintable(arg), qPrintable(f.errorString()));
            return 1;
        }

        Preprocessor pp;
        Moc moc;
        pp.macros["Q_MOC_RUN"];
        pp.macros["__cplusplus"];

        const QByteArray filename = QFile::decodeName(argv[i]).toLatin1();

        moc.filename = filename;
        moc.currentFilenames.push(filename);

        moc.symbols = pp.preprocessed(moc.filename, &f);
        moc.parse();

        if (moc.classList.isEmpty())
            return 0;
        classes = moc.classList;

        f.close();
    }

    QFile output;
    if (outputFile.isEmpty()) {
        output.open(stdout, QIODevice::WriteOnly);
    } else {
        output.setFileName(outputFile);
        if (!output.open(QIODevice::WriteOnly)) {
            fprintf(stderr, PROGRAMNAME ": could not open output file '%s': %s",
                    qPrintable(outputFile), qPrintable(output.errorString()));
            return 1;
        }
    }

    output.write(docTypeHeader);
    output.write("<node>\n");
    foreach (const ClassDef &cdef, classes) {
        QString xml = qDBusGenerateClassDefXml(&cdef);
        output.write(xml.toLocal8Bit());
    }
    output.write("</node>\n");

    return 0;
}

