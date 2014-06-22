/***************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB)
** Contact: http://www.qt-project.org/legal
**
** This file is part of the utilities of the Qt Toolkit.
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

#include "specparser.h"

#include <QDebug>
#include <QFile>
#include <QRegExp>
#include <QStringList>
#include <QTextStream>

#ifdef SPECPARSER_DEBUG
#define qSpecParserDebug qDebug
#else
#define qSpecParserDebug QT_NO_QDEBUG_MACRO
#endif

SpecParser::SpecParser()
{
}

void SpecParser::parse()
{
    // Get the mapping form generic types to specific types suitable for use in C-headers
    if (!parseTypeMap())
        return;

    // Open up a stream on the actual OpenGL function spec file
    QFile file(m_specFileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open spec file:" << m_specFileName << "Aborting";
        return;
    }

    QTextStream stream(&file);

    // Extract the info that we need
    parseFunctions(stream);
}

bool SpecParser::parseTypeMap()
{
    QFile file(m_typeMapFileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open spec file:" << m_specFileName << "Aborting";
        return false;
    }

    QTextStream stream(&file);

    static QRegExp typeMapRegExp("([^,]+)\\W+([^,]+)");

    while (!stream.atEnd()) {
        QString line = stream.readLine();

        if (line.startsWith(QStringLiteral("#")))
            continue;

        if (typeMapRegExp.indexIn(line) != -1) {
            QString key = typeMapRegExp.cap(1).simplified();
            QString value = typeMapRegExp.cap(2).simplified();

            // Special case for void
            if (value == QStringLiteral("*"))
                value = QStringLiteral("void");

            m_typeMap.insert(key, value);
            qSpecParserDebug() << "Found type mapping from" << key << "=>" << value;
        }
    }

    return true;
}

void SpecParser::parseEnums()
{
}

void SpecParser::parseFunctions(QTextStream &stream)
{
    static QRegExp functionRegExp("^(\\w+)\\(.*\\)");
    static QRegExp returnRegExp("^\\treturn\\s+(\\S+)");
    static QRegExp argumentRegExp("param\\s+(\\S+)\\s+(\\S+) (\\S+) (\\S+)");
    static QRegExp versionRegExp("^\\tversion\\s+(\\S+)");
    static QRegExp deprecatedRegExp("^\\tdeprecated\\s+(\\S+)");
    static QRegExp categoryRegExp("^\\tcategory\\s+(\\S+)");
    static QRegExp categoryVersionRegExp("VERSION_(\\d)_(\\d)");
    static QRegExp extToCoreVersionRegExp("passthru:\\s/\\*\\sOpenGL\\s(\\d)\\.(\\d)\\s.*\\sextensions:");
    static QRegExp extToCoreRegExp("passthru:\\s/\\*\\s(ARB_\\S*)\\s.*\\*/");

    Function currentFunction;
    VersionProfile currentVersionProfile;
    QString currentCategory;
    bool haveVersionInfo = false;
    bool acceptCurrentFunctionInCore = false;
    bool acceptCurrentFunctionInExtension = false;

    QHash<QString, Version> extensionsNowInCore;
    Version extToCoreCurrentVersion;
    int functionCount = 0;

    QSet<Version> versions;

    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (line.startsWith("#"))
            continue;

        if (functionRegExp.indexIn(line) != -1) {

            if (!currentFunction.name.isEmpty()) {

                // NB - Special handling!
                // Versions 4.2 and 4.3 (and probably newer) add functionality by
                // subsuming extensions such as ARB_texture_storage. However, some extensions
                // also include functions to interact with the EXT_direct_state_access
                // extension. These functions should be added to the DSA extension rather
                // than the core functionality. The core will already contain non-DSA
                // versions of these functions.
                if (acceptCurrentFunctionInCore && currentFunction.name.endsWith(QStringLiteral("EXT"))) {
                    acceptCurrentFunctionInCore = false;
                    acceptCurrentFunctionInExtension = true;
                    currentCategory = QStringLiteral("EXT_direct_state_access");
                }

                // Finish off previous function (if any) by inserting it into the core
                // functionality or extension functionality (or both)
                if (acceptCurrentFunctionInCore) {
                    m_functions.insert(currentVersionProfile, currentFunction);
                    versions.insert(currentVersionProfile.version);
                }

                if (acceptCurrentFunctionInExtension)
                    m_extensionFunctions.insert(currentCategory, currentFunction);
            }

            // Start a new function
            ++functionCount;
            haveVersionInfo = false;
            acceptCurrentFunctionInCore = true;
            acceptCurrentFunctionInExtension = false;
            currentCategory = QString();
            currentFunction = Function();

            // We assume a core function unless we find a deprecated flag (see below)
            currentVersionProfile = VersionProfile();
            currentVersionProfile.profile = VersionProfile::CoreProfile;

            // Extract the function name
            QString functionName = functionRegExp.cap(1);
            currentFunction.name = functionName;
            qSpecParserDebug() << "Found function:" << functionName;

        } else if (argumentRegExp.indexIn(line) != -1) {
            // Extract info about this function argument
            Argument arg;
            arg.name = argumentRegExp.cap(1);

            QString type = argumentRegExp.cap(2); // Lookup in type map
            arg.type = m_typeMap.value(type);

            QString direction = argumentRegExp.cap(3);
            if (direction == QStringLiteral("in")) {
                arg.direction = Argument::In;
            } else if (direction == QStringLiteral("out")) {
                arg.direction = Argument::Out;
            } else {
                qWarning() << "Invalid argument direction found:" << direction;
                acceptCurrentFunctionInCore = false;
            }

            QString mode = argumentRegExp.cap(4);
            if (mode == QStringLiteral("value")) {
                arg.mode = Argument::Value;
            } else if (mode == QStringLiteral("array")) {
                arg.mode = Argument::Array;
            } else if (mode == QStringLiteral("reference")) {
                arg.mode = Argument::Reference;
            } else {
                qWarning() << "Invalid argument mode found:" << mode;
                acceptCurrentFunctionInCore = false;
            }

            qSpecParserDebug() << "    argument:" << arg.type << arg.name;
            currentFunction.arguments.append(arg);

        } else if (returnRegExp.indexIn(line) != -1) {
            // Lookup the return type from the typemap
            QString returnTypeKey = returnRegExp.cap(1).simplified();
            if (!m_typeMap.contains(returnTypeKey)) {
                qWarning() << "Unknown return type found:" << returnTypeKey;
                acceptCurrentFunctionInCore = false;
            }
            QString returnType = m_typeMap.value(returnTypeKey);
            qSpecParserDebug() << "    return type:" << returnType;
            currentFunction.returnType = returnType;

        } else if (versionRegExp.indexIn(line) != -1 && !haveVersionInfo) { // Only use version line if no other source
            // Extract the OpenGL version in which this function was introduced
            QString version = versionRegExp.cap(1);
            qSpecParserDebug() << "    version:" << version;
            QStringList parts = version.split(QLatin1Char('.'));
            if (parts.size() != 2) {
                qWarning() << "Found invalid version number";
                continue;
            }
            int majorVersion = parts.first().toInt();
            int minorVersion = parts.last().toInt();
            Version v;
            v.major = majorVersion;
            v.minor = minorVersion;
            currentVersionProfile.version = v;

        } else if (deprecatedRegExp.indexIn(line) != -1) {
            // Extract the OpenGL version in which this function was deprecated.
            // If it is OpenGL 3.1 then it must be a compatibility profile function
            QString deprecatedVersion = deprecatedRegExp.cap(1).simplified();
            if (deprecatedVersion == QStringLiteral("3.1") && !inDeprecationException(currentFunction.name))
                currentVersionProfile.profile = VersionProfile::CompatibilityProfile;

        } else if (categoryRegExp.indexIn(line) != -1) {
            // Extract the category for this function
            QString category = categoryRegExp.cap(1).simplified();
            qSpecParserDebug() << "    category:" << category;

            if (categoryVersionRegExp.indexIn(category) != -1) {
                // Use the version info in the category in preference to the version
                // entry as this is more applicable and consistent
                int majorVersion = categoryVersionRegExp.cap(1).toInt();
                int minorVersion = categoryVersionRegExp.cap(2).toInt();

                Version v;
                v.major = majorVersion;
                v.minor = minorVersion;
                currentVersionProfile.version = v;
                haveVersionInfo = true;

            } else {
                // Make a note of the extension name and tag this function as being part of an extension
                qSpecParserDebug() << "Found category =" << category;
                currentCategory = category;
                acceptCurrentFunctionInExtension = true;

                // See if this category (extension) is in our set of extensions that
                // have now been folded into the core feature set
                if (extensionsNowInCore.contains(category)) {
                    currentVersionProfile.version = extensionsNowInCore.value(category);
                    haveVersionInfo = true;
                } else {
                    acceptCurrentFunctionInCore = false;
                }
            }

        } else if (extToCoreVersionRegExp.indexIn(line) != -1) {
            qSpecParserDebug() << line;
            int majorVersion = extToCoreVersionRegExp.cap(1).toInt();
            int minorVersion = extToCoreVersionRegExp.cap(2).toInt();
            extToCoreCurrentVersion.major = majorVersion;
            extToCoreCurrentVersion.minor = minorVersion;

        } else if (extToCoreRegExp.indexIn(line) != -1) {
            QString extension = extToCoreRegExp.cap(1);
            extensionsNowInCore.insert(extension, extToCoreCurrentVersion);
        }
    }

    m_versions = versions.toList();
    qSort(m_versions);
}

bool SpecParser::inDeprecationException(const QString &functionName) const
{
    return (functionName == QStringLiteral("TexImage3D"));
}
