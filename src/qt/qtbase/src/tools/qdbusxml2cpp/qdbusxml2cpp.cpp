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
#include <qdatetime.h>
#include <qdebug.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtextstream.h>
#include <qset.h>

#include <qdbusmetatype.h>
#include <private/qdbusintrospection_p.h>

#include <stdio.h>
#include <stdlib.h>

#define PROGRAMNAME     "qdbusxml2cpp"
#define PROGRAMVERSION  "0.8"
#define PROGRAMCOPYRIGHT "Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies)."

#define ANNOTATION_NO_WAIT      "org.freedesktop.DBus.Method.NoReply"

static QString globalClassName;
static QString parentClassName;
static QString proxyFile;
static QString adaptorFile;
static QString inputFile;
static QDateTime classCreationTime;
static bool skipNamespaces;
static bool verbose;
static bool includeMocs;
static QString commandLine;
static QStringList includes;
static QStringList wantedInterfaces;

static const char help[] =
    "Usage: " PROGRAMNAME " [options...] [xml-or-xml-file] [interfaces...]\n"
    "Produces the C++ code to implement the interfaces defined in the input file.\n"
    "\n"
    "Options:\n"
    "  -a <filename>    Write the adaptor code to <filename>\n"
    "  -c <classname>   Use <classname> as the class name for the generated classes\n"
    "  -h               Show this information\n"
    "  -i <filename>    Add #include to the output\n"
    "  -l <classname>   When generating an adaptor, use <classname> as the parent class\n"
    "  -m               Generate #include \"filename.moc\" statements in the .cpp files\n"
    "  -N               Don't use namespaces\n"
    "  -p <filename>    Write the proxy code to <filename>\n"
    "  -v               Be verbose.\n"
    "  -V               Show the program version and quit.\n"
    "\n"
    "If the file name given to the options -a and -p does not end in .cpp or .h, the\n"
    "program will automatically append the suffixes and produce both files.\n"
    "You can also use a colon (:) to separate the header name from the source file\n"
    "name, as in '-a filename_p.h:filename.cpp'.\n"
    "\n"
    "If you pass a dash (-) as the argument to either -p or -a, the output is written\n"
    "to the standard output\n";

static const char includeList[] =
    "#include <QtCore/QByteArray>\n"
    "#include <QtCore/QList>\n"
    "#include <QtCore/QMap>\n"
    "#include <QtCore/QString>\n"
    "#include <QtCore/QStringList>\n"
    "#include <QtCore/QVariant>\n";

static const char forwardDeclarations[] =
    "QT_BEGIN_NAMESPACE\n"
    "class QByteArray;\n"
    "template<class T> class QList;\n"
    "template<class Key, class Value> class QMap;\n"
    "class QString;\n"
    "class QStringList;\n"
    "class QVariant;\n"
    "QT_END_NAMESPACE\n";

static void showHelp()
{
    printf("%s", help);
    exit(0);
}

static void showVersion()
{
    printf("%s version %s\n", PROGRAMNAME, PROGRAMVERSION);
    printf("D-Bus binding tool for Qt\n");
    exit(0);
}

static QString nextArg(QStringList &args, int i, char opt)
{
    QString arg = args.value(i);
    if (arg.isEmpty()) {
        printf("-%c needs at least one argument\n", opt);
        exit(1);
    }
    return args.takeAt(i);
}

static void parseCmdLine(QStringList args)
{
    args.takeFirst();

    commandLine = QLatin1String(PROGRAMNAME " ");
    commandLine += args.join(QLatin1Char(' '));

    int i = 0;
    while (i < args.count()) {

        if (!args.at(i).startsWith(QLatin1Char('-'))) {
            ++i;
            continue;
        }
        QString arg = args.takeAt(i);

        char c = '\0';
        if (arg.length() == 2)
            c = arg.at(1).toLatin1();
        else if (arg == QLatin1String("--help"))
            c = 'h';

        switch (c) {
        case 'a':
            adaptorFile = nextArg(args, i, 'a');
            break;

        case 'c':
            globalClassName = nextArg(args, i, 'c');
            break;

        case 'v':
            verbose = true;
            break;

        case 'i':
            includes << nextArg(args, i, 'i');
            break;

        case 'l':
            parentClassName = nextArg(args, i, 'l');
            break;

        case 'm':
            includeMocs = true;
            break;

        case 'N':
            skipNamespaces = true;
            break;

        case '?':
        case 'h':
            showHelp();
            break;

        case 'V':
            showVersion();
            break;

        case 'p':
            proxyFile = nextArg(args, i, 'p');
            break;

        default:
            printf("unknown option: '%s'\n", qPrintable(arg));
            exit(1);
        }
    }

    if (!args.isEmpty())
        inputFile = args.takeFirst();

    wantedInterfaces << args;
}

static QDBusIntrospection::Interfaces readInput()
{
    QFile input(inputFile);
    if (inputFile.isEmpty() || inputFile == QLatin1String("-")) {
        input.open(stdin, QIODevice::ReadOnly);
        classCreationTime = QDateTime::currentDateTime();
    } else {
        input.open(QIODevice::ReadOnly);
        classCreationTime = QFileInfo(input).lastModified();
    }

    QByteArray data = input.readAll();

    // check if the input is already XML
    data = data.trimmed();
    if (data.startsWith("<!DOCTYPE ") || data.startsWith("<?xml") ||
        data.startsWith("<node") || data.startsWith("<interface"))
        // already XML
        return QDBusIntrospection::parseInterfaces(QString::fromUtf8(data));

    fprintf(stderr, "Cannot process input: '%s'. Stop.\n", qPrintable(inputFile));
    exit(1);
}

static void cleanInterfaces(QDBusIntrospection::Interfaces &interfaces)
{
    if (!wantedInterfaces.isEmpty()) {
        QDBusIntrospection::Interfaces::Iterator it = interfaces.begin();
        while (it != interfaces.end())
            if (!wantedInterfaces.contains(it.key()))
                it = interfaces.erase(it);
            else
                ++it;
    }
}

// produce a header name from the file name
static QString header(const QString &name)
{
    QStringList parts = name.split(QLatin1Char(':'));
    QString retval = parts.first();

    if (retval.isEmpty() || retval == QLatin1String("-"))
        return retval;

    if (!retval.endsWith(QLatin1String(".h")) && !retval.endsWith(QLatin1String(".cpp")) &&
        !retval.endsWith(QLatin1String(".cc")))
        retval.append(QLatin1String(".h"));

    return retval;
}

// produce a cpp name from the file name
static QString cpp(const QString &name)
{
    QStringList parts = name.split(QLatin1Char(':'));
    QString retval = parts.last();

    if (retval.isEmpty() || retval == QLatin1String("-"))
        return retval;

    if (!retval.endsWith(QLatin1String(".h")) && !retval.endsWith(QLatin1String(".cpp")) &&
        !retval.endsWith(QLatin1String(".cc")))
        retval.append(QLatin1String(".cpp"));

    return retval;
}

// produce a moc name from the file name
static QString moc(const QString &name)
{
    QString retval = header(name);
    if (retval.isEmpty())
        return retval;

    retval.truncate(retval.length() - 1); // drop the h in .h
    retval += QLatin1String("moc");
    return retval;
}

static QTextStream &writeHeader(QTextStream &ts, bool changesWillBeLost)
{
    ts << "/*" << endl
       << " * This file was generated by " PROGRAMNAME " version " PROGRAMVERSION << endl
       << " * Command line was: " << commandLine << endl
       << " *" << endl
       << " * " PROGRAMNAME " is " PROGRAMCOPYRIGHT << endl
       << " *" << endl
       << " * This is an auto-generated file." << endl;

    if (changesWillBeLost)
        ts << " * Do not edit! All changes made to it will be lost." << endl;
    else
        ts << " * This file may have been hand-edited. Look for HAND-EDIT comments" << endl
           << " * before re-generating it." << endl;

    ts << " */" << endl
       << endl;

    return ts;
}

enum ClassType { Proxy, Adaptor };
static QString classNameForInterface(const QString &interface, ClassType classType)
{
    if (!globalClassName.isEmpty())
        return globalClassName;

    QStringList parts = interface.split(QLatin1Char('.'));

    QString retval;
    if (classType == Proxy)
        foreach (QString part, parts) {
            part[0] = part[0].toUpper();
            retval += part;
        }
    else {
        retval = parts.last();
        retval[0] = retval[0].toUpper();
    }

    if (classType == Proxy)
        retval += QLatin1String("Interface");
    else
        retval += QLatin1String("Adaptor");

    return retval;
}

static QByteArray qtTypeName(const QString &signature, const QDBusIntrospection::Annotations &annotations, int paramId = -1, const char *direction = "Out")
{
    int type = QDBusMetaType::signatureToType(signature.toLatin1());
    if (type == QVariant::Invalid) {
        QString annotationName = QString::fromLatin1("org.qtproject.QtDBus.QtTypeName");
        if (paramId >= 0)
            annotationName += QString::fromLatin1(".%1%2").arg(QLatin1String(direction)).arg(paramId);
        QString qttype = annotations.value(annotationName);
        if (!qttype.isEmpty())
            return qttype.toLatin1();

        QString oldAnnotationName = QString::fromLatin1("com.trolltech.QtDBus.QtTypeName");
        if (paramId >= 0)
            oldAnnotationName += QString::fromLatin1(".%1%2").arg(QLatin1String(direction)).arg(paramId);
        qttype = annotations.value(oldAnnotationName);

        if (qttype.isEmpty()) {
            fprintf(stderr, "Got unknown type `%s'\n", qPrintable(signature));
            fprintf(stderr, "You should add <annotation name=\"%s\" value=\"<type>\"/> to the XML description\n",
                    qPrintable(annotationName));
            exit(1);
        }

        fprintf(stderr, "Warning: deprecated annotation '%s' found; suggest updating to '%s'\n",
                qPrintable(oldAnnotationName), qPrintable(annotationName));
        return qttype.toLatin1();
    }

    return QVariant::typeToName(QVariant::Type(type));
}

static QString nonConstRefArg(const QByteArray &arg)
{
    return QLatin1String(arg + " &");
}

static QString templateArg(const QByteArray &arg)
{
    if (!arg.endsWith('>'))
        return QLatin1String(arg);

    return QLatin1String(arg + ' ');
}

static QString constRefArg(const QByteArray &arg)
{
    if (!arg.startsWith('Q'))
        return QLatin1String(arg + ' ');
    else
        return QString( QLatin1String("const %1 &") ).arg( QLatin1String(arg) );
}

static QStringList makeArgNames(const QDBusIntrospection::Arguments &inputArgs,
                                const QDBusIntrospection::Arguments &outputArgs =
                                QDBusIntrospection::Arguments())
{
    QStringList retval;
    for (int i = 0; i < inputArgs.count(); ++i) {
        const QDBusIntrospection::Argument &arg = inputArgs.at(i);
        QString name = arg.name;
        if (name.isEmpty())
            name = QString( QLatin1String("in%1") ).arg(i);
        else
            name.replace(QLatin1Char('-'), QLatin1Char('_'));
        while (retval.contains(name))
            name += QLatin1String("_");
        retval << name;
    }
    for (int i = 0; i < outputArgs.count(); ++i) {
        const QDBusIntrospection::Argument &arg = outputArgs.at(i);
        QString name = arg.name;
        if (name.isEmpty())
            name = QString( QLatin1String("out%1") ).arg(i);
        else
            name.replace(QLatin1Char('-'), QLatin1Char('_'));
        while (retval.contains(name))
            name += QLatin1String("_");
        retval << name;
    }
    return retval;
}

static void writeArgList(QTextStream &ts, const QStringList &argNames,
                         const QDBusIntrospection::Annotations &annotations,
                         const QDBusIntrospection::Arguments &inputArgs,
                         const QDBusIntrospection::Arguments &outputArgs = QDBusIntrospection::Arguments())
{
    // input args:
    bool first = true;
    int argPos = 0;
    for (int i = 0; i < inputArgs.count(); ++i) {
        const QDBusIntrospection::Argument &arg = inputArgs.at(i);
        QString type = constRefArg(qtTypeName(arg.type, annotations, i, "In"));

        if (!first)
            ts << ", ";
        ts << type << argNames.at(argPos++);
        first = false;
    }

    argPos++;

    // output args
    // yes, starting from 1
    for (int i = 1; i < outputArgs.count(); ++i) {
        const QDBusIntrospection::Argument &arg = outputArgs.at(i);
        QString name = arg.name;

        if (!first)
            ts << ", ";
        ts << nonConstRefArg(qtTypeName(arg.type, annotations, i, "Out"))
           << argNames.at(argPos++);
        first = false;
    }
}

static QString propertyGetter(const QDBusIntrospection::Property &property)
{
    QString getter = property.annotations.value(QLatin1String("org.qtproject.QtDBus.PropertyGetter"));
    if (!getter.isEmpty())
        return getter;

    getter = property.annotations.value(QLatin1String("com.trolltech.QtDBus.propertyGetter"));
    if (!getter.isEmpty()) {
        fprintf(stderr, "Warning: deprecated annotation 'com.trolltech.QtDBus.propertyGetter' found;"
                " suggest updating to 'org.qtproject.QtDBus.PropertyGetter'\n");
        return getter;
    }

    getter =  property.name;
    getter[0] = getter[0].toLower();
    return getter;
}

static QString propertySetter(const QDBusIntrospection::Property &property)
{
    QString setter = property.annotations.value(QLatin1String("org.qtproject.QtDBus.PropertySetter"));
    if (!setter.isEmpty())
        return setter;

    setter = property.annotations.value(QLatin1String("com.trolltech.QtDBus.propertySetter"));
    if (!setter.isEmpty()) {
        fprintf(stderr, "Warning: deprecated annotation 'com.trolltech.QtDBus.propertySetter' found;"
                " suggest updating to 'org.qtproject.QtDBus.PropertySetter'\n");
        return setter;
    }

    setter = QLatin1String("set") + property.name;
    setter[3] = setter[3].toUpper();
    return setter;
}

static QString stringify(const QString &data)
{
    QString retval;
    int i;
    for (i = 0; i < data.length(); ++i) {
        retval += QLatin1Char('\"');
        for ( ; i < data.length() && data[i] != QLatin1Char('\n') && data[i] != QLatin1Char('\r'); ++i)
            if (data[i] == QLatin1Char('\"'))
                retval += QLatin1String("\\\"");
            else
                retval += data[i];
        if (i+1 < data.length() && data[i] == QLatin1Char('\r') && data[i+1] == QLatin1Char('\n'))
            i++;
        retval += QLatin1String("\\n\"\n");
    }
    return retval;
}

static bool openFile(const QString &fileName, QFile &file)
{
    if (fileName.isEmpty())
        return false;

    bool isOk = false;
    if (fileName == QLatin1String("-")) {
        isOk = file.open(stdout, QIODevice::WriteOnly | QIODevice::Text);
    } else {
        file.setFileName(fileName);
        isOk = file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    }

    if (!isOk)
        fprintf(stderr, "Unable to open '%s': %s\n", qPrintable(fileName),
                qPrintable(file.errorString()));
    return isOk;
}

static void writeProxy(const QString &filename, const QDBusIntrospection::Interfaces &interfaces)
{
    // open the file
    QString headerName = header(filename);
    QByteArray headerData;
    QTextStream hs(&headerData);

    QString cppName = cpp(filename);
    QByteArray cppData;
    QTextStream cs(&cppData);

    // write the header:
    writeHeader(hs, true);
    if (cppName != headerName)
        writeHeader(cs, false);

    // include guards:
    QString includeGuard;
    if (!headerName.isEmpty() && headerName != QLatin1String("-")) {
        includeGuard = headerName.toUpper().replace(QLatin1Char('.'), QLatin1Char('_'));
        int pos = includeGuard.lastIndexOf(QLatin1Char('/'));
        if (pos != -1)
            includeGuard = includeGuard.mid(pos + 1);
    } else {
        includeGuard = QLatin1String("QDBUSXML2CPP_PROXY");
    }
    includeGuard = QString(QLatin1String("%1_%2"))
                   .arg(includeGuard)
                   .arg(classCreationTime.toTime_t());
    hs << "#ifndef " << includeGuard << endl
       << "#define " << includeGuard << endl
       << endl;

    // include our stuff:
    hs << "#include <QtCore/QObject>" << endl
       << includeList
       << "#include <QtDBus/QtDBus>" << endl;

    foreach (const QString &include, includes) {
        hs << "#include \"" << include << "\"" << endl;
        if (headerName.isEmpty())
            cs << "#include \"" << include << "\"" << endl;
    }

    hs << endl;

    if (cppName != headerName) {
        if (!headerName.isEmpty() && headerName != QLatin1String("-"))
            cs << "#include \"" << headerName << "\"" << endl << endl;
    }

    foreach (const QDBusIntrospection::Interface *interface, interfaces) {
        QString className = classNameForInterface(interface->name, Proxy);

        // comment:
        hs << "/*" << endl
           << " * Proxy class for interface " << interface->name << endl
           << " */" << endl;
        cs << "/*" << endl
           << " * Implementation of interface class " << className << endl
           << " */" << endl
           << endl;

        // class header:
        hs << "class " << className << ": public QDBusAbstractInterface" << endl
           << "{" << endl
           << "    Q_OBJECT" << endl;

        // the interface name
        hs << "public:" << endl
           << "    static inline const char *staticInterfaceName()" << endl
           << "    { return \"" << interface->name << "\"; }" << endl
           << endl;

        // constructors/destructors:
        hs << "public:" << endl
           << "    " << className << "(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);" << endl
           << endl
           << "    ~" << className << "();" << endl
           << endl;
        cs << className << "::" << className << "(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent)" << endl
           << "    : QDBusAbstractInterface(service, path, staticInterfaceName(), connection, parent)" << endl
           << "{" << endl
           << "}" << endl
           << endl
           << className << "::~" << className << "()" << endl
           << "{" << endl
           << "}" << endl
           << endl;

        // properties:
        foreach (const QDBusIntrospection::Property &property, interface->properties) {
            QByteArray type = qtTypeName(property.type, property.annotations);
            QString templateType = templateArg(type);
            QString constRefType = constRefArg(type);
            QString getter = propertyGetter(property);
            QString setter = propertySetter(property);

            hs << "    Q_PROPERTY(" << type << " " << property.name;

            // getter:
            if (property.access != QDBusIntrospection::Property::Write)
                // it's readble
                hs << " READ " << getter;

            // setter
            if (property.access != QDBusIntrospection::Property::Read)
                // it's writeable
                hs << " WRITE " << setter;

            hs << ")" << endl;

            // getter:
            if (property.access != QDBusIntrospection::Property::Write) {
                hs << "    inline " << type << " " << getter << "() const" << endl
                    << "    { return qvariant_cast< " << type << " >(property(\""
                    << property.name << "\")); }" << endl;
            }

            // setter:
            if (property.access != QDBusIntrospection::Property::Read) {
                hs << "    inline void " << setter << "(" << constRefArg(type) << "value)" << endl
                   << "    { setProperty(\"" << property.name
                   << "\", QVariant::fromValue(value)); }" << endl;
            }

            hs << endl;
        }

        // methods:
        hs << "public Q_SLOTS: // METHODS" << endl;
        foreach (const QDBusIntrospection::Method &method, interface->methods) {
            bool isDeprecated = method.annotations.value(QLatin1String("org.freedesktop.DBus.Deprecated")) == QLatin1String("true");
            bool isNoReply =
                method.annotations.value(QLatin1String(ANNOTATION_NO_WAIT)) == QLatin1String("true");
            if (isNoReply && !method.outputArgs.isEmpty()) {
                fprintf(stderr, "warning: method %s in interface %s is marked 'no-reply' but has output arguments.\n",
                        qPrintable(method.name), qPrintable(interface->name));
                continue;
            }

            hs << "    inline "
               << (isDeprecated ? "Q_DECL_DEPRECATED " : "");

            if (isNoReply) {
                hs << "Q_NOREPLY void ";
            } else {
                hs << "QDBusPendingReply<";
                for (int i = 0; i < method.outputArgs.count(); ++i)
                    hs << (i > 0 ? ", " : "")
                       << templateArg(qtTypeName(method.outputArgs.at(i).type, method.annotations, i, "Out"));
                hs << "> ";
            }

            hs << method.name << "(";

            QStringList argNames = makeArgNames(method.inputArgs);
            writeArgList(hs, argNames, method.annotations, method.inputArgs);

            hs << ")" << endl
               << "    {" << endl
               << "        QList<QVariant> argumentList;" << endl;

            if (!method.inputArgs.isEmpty()) {
                hs << "        argumentList";
                for (int argPos = 0; argPos < method.inputArgs.count(); ++argPos)
                    hs << " << QVariant::fromValue(" << argNames.at(argPos) << ')';
                hs << ";" << endl;
            }

            if (isNoReply)
                hs << "        callWithArgumentList(QDBus::NoBlock, "
                   <<  "QLatin1String(\"" << method.name << "\"), argumentList);" << endl;
            else
                hs << "        return asyncCallWithArgumentList(QLatin1String(\""
                   << method.name << "\"), argumentList);" << endl;

            // close the function:
            hs << "    }" << endl;

            if (method.outputArgs.count() > 1) {
                // generate the old-form QDBusReply methods with multiple incoming parameters
                hs << "    inline "
                   << (isDeprecated ? "Q_DECL_DEPRECATED " : "")
                   << "QDBusReply<"
                   << templateArg(qtTypeName(method.outputArgs.first().type, method.annotations, 0, "Out")) << "> ";
                hs << method.name << "(";

                QStringList argNames = makeArgNames(method.inputArgs, method.outputArgs);
                writeArgList(hs, argNames, method.annotations, method.inputArgs, method.outputArgs);

                hs << ")" << endl
                   << "    {" << endl
                   << "        QList<QVariant> argumentList;" << endl;

                int argPos = 0;
                if (!method.inputArgs.isEmpty()) {
                    hs << "        argumentList";
                    for (argPos = 0; argPos < method.inputArgs.count(); ++argPos)
                        hs << " << QVariant::fromValue(" << argNames.at(argPos) << ')';
                    hs << ";" << endl;
                }

                hs << "        QDBusMessage reply = callWithArgumentList(QDBus::Block, "
                   <<  "QLatin1String(\"" << method.name << "\"), argumentList);" << endl;

                argPos++;
                hs << "        if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().count() == "
                   << method.outputArgs.count() << ") {" << endl;

                // yes, starting from 1
                for (int i = 1; i < method.outputArgs.count(); ++i)
                    hs << "            " << argNames.at(argPos++) << " = qdbus_cast<"
                       << templateArg(qtTypeName(method.outputArgs.at(i).type, method.annotations, i, "Out"))
                       << ">(reply.arguments().at(" << i << "));" << endl;
                hs << "        }" << endl
                   << "        return reply;" << endl
                   << "    }" << endl;
            }

            hs << endl;
        }

        hs << "Q_SIGNALS: // SIGNALS" << endl;
        foreach (const QDBusIntrospection::Signal &signal, interface->signals_) {
            hs << "    ";
            if (signal.annotations.value(QLatin1String("org.freedesktop.DBus.Deprecated")) ==
                QLatin1String("true"))
                hs << "Q_DECL_DEPRECATED ";

            hs << "void " << signal.name << "(";

            QStringList argNames = makeArgNames(signal.outputArgs);
            writeArgList(hs, argNames, signal.annotations, signal.outputArgs);

            hs << ");" << endl; // finished for header
        }

        // close the class:
        hs << "};" << endl
           << endl;
    }

    if (!skipNamespaces) {
        QStringList last;
        QDBusIntrospection::Interfaces::ConstIterator it = interfaces.constBegin();
        do
        {
            QStringList current;
            QString name;
            if (it != interfaces.constEnd()) {
                current = it->constData()->name.split(QLatin1Char('.'));
                name = current.takeLast();
            }

            int i = 0;
            while (i < current.count() && i < last.count() && current.at(i) == last.at(i))
                ++i;

            // i parts matched
            // close last.arguments().count() - i namespaces:
            for (int j = i; j < last.count(); ++j)
                hs << QString((last.count() - j - 1 + i) * 2, QLatin1Char(' ')) << "}" << endl;

            // open current.arguments().count() - i namespaces
            for (int j = i; j < current.count(); ++j)
                hs << QString(j * 2, QLatin1Char(' ')) << "namespace " << current.at(j) << " {" << endl;

            // add this class:
            if (!name.isEmpty()) {
                hs << QString(current.count() * 2, QLatin1Char(' '))
                   << "typedef ::" << classNameForInterface(it->constData()->name, Proxy)
                   << " " << name << ";" << endl;
            }

            if (it == interfaces.constEnd())
                break;
            ++it;
            last = current;
        } while (true);
    }

    // close the include guard
    hs << "#endif" << endl;

    QString mocName = moc(filename);
    if (includeMocs && !mocName.isEmpty())
        cs << endl
           << "#include \"" << mocName << "\"" << endl;

    cs.flush();
    hs.flush();

    QFile file;
    const bool headerOpen = openFile(headerName, file);
    if (headerOpen)
        file.write(headerData);

    if (headerName == cppName) {
        if (headerOpen)
            file.write(cppData);
    } else {
        QFile cppFile;
        if (openFile(cppName, cppFile))
            cppFile.write(cppData);
    }
}

static void writeAdaptor(const QString &filename, const QDBusIntrospection::Interfaces &interfaces)
{
    // open the file
    QString headerName = header(filename);
    QByteArray headerData;
    QTextStream hs(&headerData);

    QString cppName = cpp(filename);
    QByteArray cppData;
    QTextStream cs(&cppData);

    // write the headers
    writeHeader(hs, false);
    if (cppName != headerName)
        writeHeader(cs, true);

    // include guards:
    QString includeGuard;
    if (!headerName.isEmpty() && headerName != QLatin1String("-")) {
        includeGuard = headerName.toUpper().replace(QLatin1Char('.'), QLatin1Char('_'));
        int pos = includeGuard.lastIndexOf(QLatin1Char('/'));
        if (pos != -1)
            includeGuard = includeGuard.mid(pos + 1);
    } else {
        includeGuard = QLatin1String("QDBUSXML2CPP_ADAPTOR");
    }
    includeGuard = QString(QLatin1String("%1_%2"))
                   .arg(includeGuard)
                   .arg(QDateTime::currentDateTime().toTime_t());
    hs << "#ifndef " << includeGuard << endl
       << "#define " << includeGuard << endl
       << endl;

    // include our stuff:
    hs << "#include <QtCore/QObject>" << endl;
    if (cppName == headerName)
        hs << "#include <QtCore/QMetaObject>" << endl
           << "#include <QtCore/QVariant>" << endl;
    hs << "#include <QtDBus/QtDBus>" << endl;

    foreach (const QString &include, includes) {
        hs << "#include \"" << include << "\"" << endl;
        if (headerName.isEmpty())
            cs << "#include \"" << include << "\"" << endl;
    }

    if (cppName != headerName) {
        if (!headerName.isEmpty() && headerName != QLatin1String("-"))
            cs << "#include \"" << headerName << "\"" << endl;

        cs << "#include <QtCore/QMetaObject>" << endl
           << includeList
           << endl;
        hs << forwardDeclarations;
    } else {
        hs << includeList;
    }

    hs << endl;

    QString parent = parentClassName;
    if (parentClassName.isEmpty())
        parent = QLatin1String("QObject");

    foreach (const QDBusIntrospection::Interface *interface, interfaces) {
        QString className = classNameForInterface(interface->name, Adaptor);

        // comment:
        hs << "/*" << endl
           << " * Adaptor class for interface " << interface->name << endl
           << " */" << endl;
        cs << "/*" << endl
           << " * Implementation of adaptor class " << className << endl
           << " */" << endl
           << endl;

        // class header:
        hs << "class " << className << ": public QDBusAbstractAdaptor" << endl
           << "{" << endl
           << "    Q_OBJECT" << endl
           << "    Q_CLASSINFO(\"D-Bus Interface\", \"" << interface->name << "\")" << endl
           << "    Q_CLASSINFO(\"D-Bus Introspection\", \"\"" << endl
           << stringify(interface->introspection)
           << "        \"\")" << endl
           << "public:" << endl
           << "    " << className << "(" << parent << " *parent);" << endl
           << "    virtual ~" << className << "();" << endl
           << endl;

        if (!parentClassName.isEmpty())
            hs << "    inline " << parent << " *parent() const" << endl
               << "    { return static_cast<" << parent << " *>(QObject::parent()); }" << endl
               << endl;

        // constructor/destructor
        cs << className << "::" << className << "(" << parent << " *parent)" << endl
           << "    : QDBusAbstractAdaptor(parent)" << endl
           << "{" << endl
           << "    // constructor" << endl
           << "    setAutoRelaySignals(true);" << endl
           << "}" << endl
           << endl
           << className << "::~" << className << "()" << endl
           << "{" << endl
           << "    // destructor" << endl
           << "}" << endl
           << endl;

        hs << "public: // PROPERTIES" << endl;
        foreach (const QDBusIntrospection::Property &property, interface->properties) {
            QByteArray type = qtTypeName(property.type, property.annotations);
            QString constRefType = constRefArg(type);
            QString getter = propertyGetter(property);
            QString setter = propertySetter(property);

            hs << "    Q_PROPERTY(" << type << " " << property.name;
            if (property.access != QDBusIntrospection::Property::Write)
                hs << " READ " << getter;
            if (property.access != QDBusIntrospection::Property::Read)
                hs << " WRITE " << setter;
            hs << ")" << endl;

            // getter:
            if (property.access != QDBusIntrospection::Property::Write) {
                hs << "    " << type << " " << getter << "() const;" << endl;
                cs << type << " "
                   << className << "::" << getter << "() const" << endl
                   << "{" << endl
                   << "    // get the value of property " << property.name << endl
                   << "    return qvariant_cast< " << type <<" >(parent()->property(\"" << property.name << "\"));" << endl
                   << "}" << endl
                   << endl;
            }

            // setter
            if (property.access != QDBusIntrospection::Property::Read) {
                hs << "    void " << setter << "(" << constRefType << "value);" << endl;
                cs << "void " << className << "::" << setter << "(" << constRefType << "value)" << endl
                   << "{" << endl
                   << "    // set the value of property " << property.name << endl
                   << "    parent()->setProperty(\"" << property.name << "\", QVariant::fromValue(value";
                if (constRefType.contains(QLatin1String("QDBusVariant")))
                    cs << ".variant()";
                cs << "));" << endl
                   << "}" << endl
                   << endl;
            }

            hs << endl;
        }

        hs << "public Q_SLOTS: // METHODS" << endl;
        foreach (const QDBusIntrospection::Method &method, interface->methods) {
            bool isNoReply =
                method.annotations.value(QLatin1String(ANNOTATION_NO_WAIT)) == QLatin1String("true");
            if (isNoReply && !method.outputArgs.isEmpty()) {
                fprintf(stderr, "warning: method %s in interface %s is marked 'no-reply' but has output arguments.\n",
                        qPrintable(method.name), qPrintable(interface->name));
                continue;
            }

            hs << "    ";
            if (method.annotations.value(QLatin1String("org.freedesktop.DBus.Deprecated")) ==
                QLatin1String("true"))
                hs << "Q_DECL_DEPRECATED ";

            QByteArray returnType;
            if (isNoReply) {
                hs << "Q_NOREPLY void ";
                cs << "void ";
            } else if (method.outputArgs.isEmpty()) {
                hs << "void ";
                cs << "void ";
            } else {
                returnType = qtTypeName(method.outputArgs.first().type, method.annotations, 0, "Out");
                hs << returnType << " ";
                cs << returnType << " ";
            }

            QString name = method.name;
            hs << name << "(";
            cs << className << "::" << name << "(";

            QStringList argNames = makeArgNames(method.inputArgs, method.outputArgs);
            writeArgList(hs, argNames, method.annotations, method.inputArgs, method.outputArgs);
            writeArgList(cs, argNames, method.annotations, method.inputArgs, method.outputArgs);

            hs << ");" << endl; // finished for header
            cs << ")" << endl
               << "{" << endl
               << "    // handle method call " << interface->name << "." << method.name << endl;

            // make the call
            bool usingInvokeMethod = false;
            if (parentClassName.isEmpty() && method.inputArgs.count() <= 10
                && method.outputArgs.count() <= 1)
                usingInvokeMethod = true;

            if (usingInvokeMethod) {
                // we are using QMetaObject::invokeMethod
                if (!returnType.isEmpty())
                    cs << "    " << returnType << " " << argNames.at(method.inputArgs.count())
                       << ";" << endl;

                static const char invoke[] = "    QMetaObject::invokeMethod(parent(), \"";
                cs << invoke << name << "\"";

                if (!method.outputArgs.isEmpty())
                    cs << ", Q_RETURN_ARG("
                       << qtTypeName(method.outputArgs.at(0).type, method.annotations,
                                     0, "Out")
                       << ", "
                       << argNames.at(method.inputArgs.count())
                       << ")";

                for (int i = 0; i < method.inputArgs.count(); ++i)
                    cs << ", Q_ARG("
                       << qtTypeName(method.inputArgs.at(i).type, method.annotations,
                                     i, "In")
                       << ", "
                       << argNames.at(i)
                       << ")";

                cs << ");" << endl;

                if (!returnType.isEmpty())
                    cs << "    return " << argNames.at(method.inputArgs.count()) << ";" << endl;
            } else {
                if (parentClassName.isEmpty())
                    cs << "    //";
                else
                    cs << "    ";

                if (!method.outputArgs.isEmpty())
                    cs << "return ";

                if (parentClassName.isEmpty())
                    cs << "static_cast<YourObjectType *>(parent())->";
                else
                    cs << "parent()->";
                cs << name << "(";

                int argPos = 0;
                bool first = true;
                for (int i = 0; i < method.inputArgs.count(); ++i) {
                    cs << (first ? "" : ", ") << argNames.at(argPos++);
                    first = false;
                }
                ++argPos;           // skip retval, if any
                for (int i = 1; i < method.outputArgs.count(); ++i) {
                    cs << (first ? "" : ", ") << argNames.at(argPos++);
                    first = false;
                }

                cs << ");" << endl;
            }
            cs << "}" << endl
               << endl;
        }

        hs << "Q_SIGNALS: // SIGNALS" << endl;
        foreach (const QDBusIntrospection::Signal &signal, interface->signals_) {
            hs << "    ";
            if (signal.annotations.value(QLatin1String("org.freedesktop.DBus.Deprecated")) ==
                QLatin1String("true"))
                hs << "Q_DECL_DEPRECATED ";

            hs << "void " << signal.name << "(";

            QStringList argNames = makeArgNames(signal.outputArgs);
            writeArgList(hs, argNames, signal.annotations, signal.outputArgs);

            hs << ");" << endl; // finished for header
        }

        // close the class:
        hs << "};" << endl
           << endl;
    }

    // close the include guard
    hs << "#endif" << endl;

    QString mocName = moc(filename);
    if (includeMocs && !mocName.isEmpty())
        cs << endl
           << "#include \"" << mocName << "\"" << endl;

    cs.flush();
    hs.flush();

    QFile file;
    const bool headerOpen = openFile(headerName, file);
    if (headerOpen)
        file.write(headerData);

    if (headerName == cppName) {
        if (headerOpen)
            file.write(cppData);
    } else {
        QFile cppFile;
        if (openFile(cppName, cppFile))
            cppFile.write(cppData);
    }
}

int main(int argc, char **argv)
{
    QStringList arguments;

    for (int i = 0; i < argc; ++i) {
        arguments.append(QString::fromLocal8Bit(argv[i]));
    }

    parseCmdLine(arguments);

    QDBusIntrospection::Interfaces interfaces = readInput();
    cleanInterfaces(interfaces);

    if (!proxyFile.isEmpty() || adaptorFile.isEmpty())
        writeProxy(proxyFile, interfaces);

    if (!adaptorFile.isEmpty())
        writeAdaptor(adaptorFile, interfaces);

    return 0;
}

