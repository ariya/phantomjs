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

#include "codegenerator.h"

#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QTextStream>

static const QString extensionRegistryFileName = QStringLiteral("qopengl-extension-registry.ini");
static const QString extensionIdGroupName = QStringLiteral("ExtensionIds");

CodeGenerator::CodeGenerator()
    : m_parser(0)
{
}

void CodeGenerator::generateCoreClasses(const QString &baseFileName) const
{
    // Output header and implementation files for the backend and base class
    writeCoreHelperClasses(baseFileName + QStringLiteral(".h"), Declaration);
    writeCoreHelperClasses(baseFileName + QStringLiteral(".cpp"), Definition);

    // Output the per-version and profile public classes
    writeCoreClasses(baseFileName);

    // We also need to generate a factory class that can be used by
    // QOpenGLContext to actually create version function objects
    writeCoreFactoryHeader(baseFileName + QStringLiteral("factory_p.h"));
    writeCoreFactoryImplementation(baseFileName + QStringLiteral("factory.cpp"));
}

void CodeGenerator::generateExtensionClasses(const QString &baseFileName) const
{
    writeExtensionHeader(baseFileName + QStringLiteral(".h"));
    writeExtensionImplementation(baseFileName + QStringLiteral(".cpp"));
}

bool CodeGenerator::isLegacyVersion(Version v) const
{
    return (v.major < 3 || (v.major == 3 && v.minor == 0));
}

bool CodeGenerator::versionHasProfiles(Version v) const
{
    VersionProfile vp;
    vp.version = v;
    return vp.hasProfiles();
}

void CodeGenerator::writeCoreHelperClasses(const QString &fileName, ClassComponent component) const
{
    if (!m_parser)
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream stream(&file);

    // Write the preamble
    writePreamble(fileName, stream);

    // Iterate over each OpenGL version. For each version output a private class for
    // core functions and a private class for deprecated functions.
    const QString privateRootClass = QStringLiteral("QOpenGLVersionFunctionsBackend");
    Q_FOREACH (const VersionProfile &versionProfile, m_parser->versionProfiles()) {
        switch (component) {
        case Declaration:
            writeBackendClassDeclaration(stream, versionProfile, privateRootClass);
            break;

        case Definition:
            writeBackendClassImplementation(stream, versionProfile, privateRootClass);
            break;
        }
    }

    // Write the postamble
    writePostamble(fileName, stream);
}

void CodeGenerator::writeCoreClasses(const QString &baseFileName) const
{
    // Iterate over each OpenGL version. For each version output a public class (for legacy
    // versions or two public classes (for modern versions with profiles). Each public class
    // is given pointers to private classes containing the actual entry points. For example,
    // the class for OpenGL 1.1 will have pointers to the private classes for 1.0 core, 1.1
    // core, 1.0 deprecated and 1.1 deprecated. Whereas the class for OpenGL 3.2 Core profile
    // will have pointers to the private classes for 1.0 core, 1.1 core, ..., 3.2 core but
    // not to any of the deprecated private classes
    QList<ClassComponent> components = (QList<ClassComponent>() << Declaration << Definition);
    Q_FOREACH (const ClassComponent &component, components) {
        const QString rootClass = QStringLiteral("QAbstractOpenGLFunctions");
        Q_FOREACH (const Version &classVersion, m_parser->versions()) {
            VersionProfile v;
            v.version = classVersion;
            v.profile = VersionProfile::CompatibilityProfile;

            if (isLegacyVersion(classVersion)) {
                switch (component) {
                case Declaration:
                    writePublicClassDeclaration(baseFileName, v, rootClass);
                    break;

                case Definition:
                    writePublicClassImplementation(baseFileName, v, rootClass);
                    break;
                }
            } else {
                switch (component) {
                case Declaration:
                    writePublicClassDeclaration(baseFileName, v, rootClass);
                    v.profile = VersionProfile::CoreProfile;
                    writePublicClassDeclaration(baseFileName, v, rootClass);
                    break;

                case Definition:
                    writePublicClassImplementation(baseFileName, v, rootClass);
                    v.profile = VersionProfile::CoreProfile;
                    writePublicClassImplementation(baseFileName, v, rootClass);
                    break;
                }
            }
        }
    }
}

void CodeGenerator::writeCoreFactoryHeader(const QString &fileName) const
{
    if (!m_parser)
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream stream(&file);

    // Write the preamble
    writePreamble(fileName, stream);

    // Write the postamble
    writePostamble(fileName, stream);
}

void CodeGenerator::writeCoreFactoryImplementation(const QString &fileName) const
{
    if (!m_parser)
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream stream(&file);

    // Write the preamble
    writePreamble(fileName, stream);

    // Get the set of version functions classes we need to create
    QList<Version> versions = m_parser->versions();
    qSort(versions.begin(), versions.end(), qGreater<Version>());

    // Outout the #include statements
    stream << QStringLiteral("#if !defined(QT_OPENGL_ES_2)") << endl;
    Q_FOREACH (const Version &classVersion, versions) {
        if (!versionHasProfiles(classVersion)) {
            stream << QString(QStringLiteral("#include \"qopenglfunctions_%1_%2.h\""))
                      .arg(classVersion.major)
                      .arg(classVersion.minor) << endl;
        } else {
            const QList<VersionProfile::OpenGLProfile> profiles = (QList<VersionProfile::OpenGLProfile>()
                << VersionProfile::CoreProfile << VersionProfile::CompatibilityProfile);

            Q_FOREACH (const VersionProfile::OpenGLProfile profile, profiles) {
                const QString profileSuffix = profile == VersionProfile::CoreProfile
                                            ? QStringLiteral("core")
                                            : QStringLiteral("compatibility");
                stream << QString(QStringLiteral("#include \"qopenglfunctions_%1_%2_%3.h\""))
                          .arg(classVersion.major)
                          .arg(classVersion.minor)
                          .arg(profileSuffix) << endl;
            }
        }
    }
    stream << QStringLiteral("#else") << endl;
    stream << QStringLiteral("#include \"qopenglfunctions_es2.h\"") << endl;
    stream << QStringLiteral("#endif") << endl;

    stream << endl;

    stream << QStringLiteral("QT_BEGIN_NAMESPACE") << endl << endl;
    stream << QStringLiteral("QAbstractOpenGLFunctions *QOpenGLVersionFunctionsFactory::create(const QOpenGLVersionProfile &versionProfile)") << endl;
    stream << QStringLiteral("{") << endl;
    stream << QStringLiteral("#if !defined(QT_OPENGL_ES_2)") << endl;
    stream << QStringLiteral("    const int major = versionProfile.version().first;") << endl;
    stream << QStringLiteral("    const int minor = versionProfile.version().second;") << endl << endl;

    // Iterate over classes with profiles
    stream << QStringLiteral("    if (versionProfile.hasProfiles()) {") << endl;
    stream << QStringLiteral("        switch (versionProfile.profile()) {") << endl;
    const QList<VersionProfile::OpenGLProfile> profiles = (QList<VersionProfile::OpenGLProfile>()
        << VersionProfile::CoreProfile << VersionProfile::CompatibilityProfile);
    Q_FOREACH (const VersionProfile::OpenGLProfile profile, profiles) {
        const QString caseLabel = profile == VersionProfile::CoreProfile
                                ? QStringLiteral("QSurfaceFormat::CoreProfile")
                                : QStringLiteral("QSurfaceFormat::CompatibilityProfile");
        stream << QString(QStringLiteral("        case %1:")).arg(caseLabel) << endl;

        int i = 0;
        Q_FOREACH (const Version &classVersion, versions) {
            if (!versionHasProfiles(classVersion))
                continue;

            const QString ifString = (i++ == 0) ? QStringLiteral("if") : QStringLiteral("else if");
            stream << QString(QStringLiteral("            %1 (major == %2 && minor == %3)"))
                      .arg(ifString)
                      .arg(classVersion.major)
                      .arg(classVersion.minor) << endl;

            VersionProfile v;
            v.version = classVersion;
            v.profile = profile;
            stream << QString(QStringLiteral("                return new %1;"))
                      .arg(generateClassName(v)) << endl;
        }

        stream << QStringLiteral("            break;") << endl << endl;
    }

    stream << QStringLiteral("        case QSurfaceFormat::NoProfile:") << endl;
    stream << QStringLiteral("        default:") << endl;
    stream << QStringLiteral("            break;") << endl;
    stream << QStringLiteral("        };") << endl;
    stream << QStringLiteral("    } else {") << endl;

    // Iterate over the legacy classes (no profiles)
    int i = 0;
    Q_FOREACH (const Version &classVersion, versions) {
        if (versionHasProfiles(classVersion))
            continue;

        const QString ifString = (i++ == 0) ? QStringLiteral("if") : QStringLiteral("else if");
        stream << QString(QStringLiteral("        %1 (major == %2 && minor == %3)"))
                  .arg(ifString)
                  .arg(classVersion.major)
                  .arg(classVersion.minor) << endl;

        VersionProfile v;
        v.version = classVersion;
        stream << QString(QStringLiteral("            return new %1;"))
                  .arg(generateClassName(v)) << endl;
    }

    stream << QStringLiteral("    }") << endl;
    stream << QStringLiteral("    return 0;") << endl;

    stream << QStringLiteral("#else") << endl;
    stream << QStringLiteral("    Q_UNUSED(versionProfile);") << endl;
    stream << QStringLiteral("    return new QOpenGLFunctions_ES2;") << endl;
    stream << QStringLiteral("#endif") << endl;
    stream << QStringLiteral("}") << endl;

    // Write the postamble
    writePostamble(fileName, stream);
}

/**
  \returns all functions to be included in the class defined by \a classVersionProfile
 */
FunctionCollection CodeGenerator::functionCollection( const VersionProfile& classVersionProfile ) const
{
    const Version classVersion = classVersionProfile.version;
    FunctionCollection functionSet;
    QList<Version> versions = m_parser->versions();

    // Populate these based upon the class version and profile
    Version minVersion;
    minVersion.major = 1;
    minVersion.minor = 0;
    Version maxVersion = classVersion;
    QList<VersionProfile::OpenGLProfile> profiles;
    profiles << VersionProfile::CoreProfile; // Always need core functions

    if (isLegacyVersion(classVersion)
        || (classVersionProfile.hasProfiles()
         && classVersionProfile.profile == VersionProfile::CompatibilityProfile)) {
        // For versions < 3.1 and Compatibility profile we include both core and deprecated functions
        profiles << VersionProfile::CompatibilityProfile;
    }

    Q_FOREACH (const Version &v, versions) {
        // Only include functions from versions in the range
        if (v < minVersion)
            continue;
        if (v > maxVersion)
            break;

        Q_FOREACH (VersionProfile::OpenGLProfile profile, profiles) {
            // Combine version and profile for this subset of functions
            VersionProfile version;
            version.version = v;
            version.profile = profile;

            // Fetch the functions and add to collection for this class
            QList<Function> functions = m_parser->functionsForVersion(version);
            functionSet.insert(version, functions);
        }
    }

    return functionSet;
}

void CodeGenerator::writePreamble(const QString &baseFileName, QTextStream &stream, const QString replacement) const
{
    const QString fileName = baseFileName + QStringLiteral(".header");
    if (!QFile::exists(fileName))
        return;

    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream preambleStream(&file);
        QString preamble = preambleStream.readAll();
        if (!replacement.isEmpty())
            preamble.replace(QStringLiteral("__VERSION__"), replacement, Qt::CaseSensitive);
        stream << preamble;
    }
}

void CodeGenerator::writePostamble(const QString &baseFileName, QTextStream &stream) const
{
    const QString fileName = baseFileName + QStringLiteral(".footer");
    if (!QFile::exists(fileName))
        return;

    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream postambleStream(&file);
        QString postamble = postambleStream.readAll();
        stream << postamble;
    }
}

QString CodeGenerator::passByType(const Argument &arg) const
{
    QString passBy;
    switch (arg.mode) {
    case Argument::Reference:
    case Argument::Array:
        passBy = QStringLiteral("*");
        break;

    default:
    case Argument::Value:
        passBy = QString();
    }
    return passBy;
}

QString CodeGenerator::safeArgumentName(const QString& arg) const
{
    if (arg == QStringLiteral("near")) // MS Windows defines near and far
        return QStringLiteral("nearVal");
    else if (arg == QStringLiteral("far"))
        return QStringLiteral("farVal");
    else if (arg == QStringLiteral("d"))
        return QStringLiteral("dd"); // Don't shadow d pointer
    else
        return arg;
}

QString CodeGenerator::generateClassName(const VersionProfile &classVersion, ClassVisibility visibility) const
{
    QString className;
    switch ( visibility ) {
    case Public: {
        // Class name and base class
        QString profileSuffix;
        if (classVersion.hasProfiles())
            profileSuffix = (classVersion.profile == VersionProfile::CoreProfile ? QStringLiteral("_Core") : QStringLiteral("_Compatibility"));

        className = QString(QStringLiteral("QOpenGLFunctions_%1_%2%3"))
                    .arg(classVersion.version.major)
                    .arg(classVersion.version.minor)
                    .arg(profileSuffix);
        break;
    }
    case Private: {
        QString statusSuffix = (classVersion.profile == VersionProfile::CoreProfile ? QStringLiteral("_Core") : QStringLiteral("_Deprecated"));

        className = QString(QStringLiteral("QOpenGLFunctions_%1_%2%3Private"))
                    .arg(classVersion.version.major)
                    .arg(classVersion.version.minor)
                    .arg(statusSuffix);
        break;
        }
    }

    return className;
}

void CodeGenerator::writeBackendClassDeclaration(QTextStream &stream,
                                                 const VersionProfile &versionProfile,
                                                 const QString &baseClass) const
{
    const QString className = backendClassName(versionProfile);
    stream << QString(QStringLiteral("class %1 : public %2"))
              .arg(className)
              .arg(baseClass)
           << endl;
    stream << QStringLiteral("{") << endl;
    stream << QStringLiteral("public:") << endl;
    stream << QString( QStringLiteral("    %1(QOpenGLContext *context);") ).arg(className) << endl << endl;

    // Output function used for generating key used in QOpenGLContextPrivate
    stream << QStringLiteral("    static QOpenGLVersionStatus versionStatus();") << endl << endl;

    // Get the functions needed for this class
    FunctionList functions = m_parser->functionsForVersion(versionProfile);
    FunctionCollection functionSet;
    functionSet.insert(versionProfile, functions);

    // Declare the functions
    writeClassFunctionDeclarations(stream, functionSet, Private);

    stream << QStringLiteral("};") << endl;
    stream << endl;
}

void CodeGenerator::writeBackendClassImplementation(QTextStream &stream,
                                                    const VersionProfile &versionProfile,
                                                    const QString &baseClass) const
{
    const QString className = backendClassName(versionProfile);
    stream << QString(QStringLiteral("%1::%1(QOpenGLContext *context)")).arg(className) << endl;
    stream << QString(QStringLiteral("    : %1(context)")).arg(baseClass) << endl
           << QStringLiteral("{") << endl;

    // Resolve the entry points for this set of functions
    // Get the functions needed for this class
    FunctionList functions = m_parser->functionsForVersion(versionProfile);
    FunctionCollection functionSet;
    functionSet.insert(versionProfile, functions);
    writeEntryPointResolutionCode(stream, functionSet);

    stream << QStringLiteral("}") << endl << endl;

    stream << QString(QStringLiteral("QOpenGLVersionStatus %1::versionStatus()")).arg(className) << endl;
    stream << QStringLiteral("{") << endl;
    const QString status = versionProfile.profile == VersionProfile::CoreProfile
                         ? QStringLiteral("QOpenGLVersionStatus::CoreStatus")
                         : QStringLiteral("QOpenGLVersionStatus::DeprecatedStatus");
    stream << QString(QStringLiteral("    return QOpenGLVersionStatus(%1, %2, %3);"))
              .arg(versionProfile.version.major)
              .arg(versionProfile.version.minor)
              .arg(status) << endl;
    stream << QStringLiteral("}") << endl << endl;
}

QString CodeGenerator::coreClassFileName(const VersionProfile &versionProfile,
                                         const QString& fileExtension) const
{
    QString profileSuffix;
    if (versionProfile.hasProfiles())
        profileSuffix = (versionProfile.profile == VersionProfile::CoreProfile ? QStringLiteral("_core") : QStringLiteral("_compatibility"));

    const QString fileName = QString(QStringLiteral("qopenglfunctions_%1_%2%3.%4"))
                             .arg(versionProfile.version.major)
                             .arg(versionProfile.version.minor)
                             .arg(profileSuffix)
                             .arg(fileExtension);
    return fileName;
}

void CodeGenerator::writePublicClassDeclaration(const QString &baseFileName,
                                                const VersionProfile &versionProfile,
                                                const QString &baseClass) const
{
    const QString fileName = coreClassFileName(versionProfile, QStringLiteral("h"));
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream stream(&file);

    // Write the preamble
    const QString templateFileName = QString(QStringLiteral("%1__VERSION__.h"))
                                     .arg(baseFileName);
    QString profileSuffix;
    if (versionProfile.hasProfiles())
        profileSuffix = (versionProfile.profile == VersionProfile::CoreProfile ? QStringLiteral("_CORE") : QStringLiteral("_COMPATIBILITY"));

    const QString versionProfileString = QString(QStringLiteral("_%1_%2%3"))
                                         .arg(versionProfile.version.major)
                                         .arg(versionProfile.version.minor)
                                         .arg(profileSuffix);
    writePreamble(templateFileName, stream, versionProfileString);

    // Ctor, dtor, and initialize function;
    const QString className = generateClassName(versionProfile, Public);
    stream << QString(QStringLiteral("class Q_GUI_EXPORT %1 : public %2"))
              .arg(className)
              .arg(baseClass)
           << endl;
    stream << QStringLiteral("{") << endl;
    stream << QStringLiteral("public:") << endl;
    stream << QString(QStringLiteral("    %1();")).arg(className) << endl;
    stream << QString(QStringLiteral("    ~%1();")).arg(className) << endl << endl;
    stream << QStringLiteral("    bool initializeOpenGLFunctions() Q_DECL_OVERRIDE;") << endl << endl;

    // Get the functions needed for this class and declare them
    FunctionCollection functionSet = functionCollection(versionProfile);
    writeClassFunctionDeclarations(stream, functionSet, Public);

    // isCompatible function and backend variables
    stream << QStringLiteral("private:") << endl;
    stream << QStringLiteral("    friend class QOpenGLContext;") << endl << endl;
    stream << QStringLiteral("    static bool isContextCompatible(QOpenGLContext *context);") << endl;
    stream << QStringLiteral("    static QOpenGLVersionProfile versionProfile();") << endl << endl;
    writeBackendVariableDeclarations(stream, backendsForFunctionCollection(functionSet));

    stream << QStringLiteral("};") << endl << endl;

    // Output the inline functions that forward OpenGL calls to the backends' entry points
    writeClassInlineFunctions(stream, className, functionSet);

    // Write the postamble
    writePostamble(templateFileName, stream);
}

void CodeGenerator::writePublicClassImplementation(const QString &baseFileName,
                                                   const VersionProfile &versionProfile,
                                                   const QString& baseClass) const
{
    const QString fileName = coreClassFileName(versionProfile, QStringLiteral("cpp"));
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream stream(&file);

    // Write the preamble
    const QString templateFileName = QString(QStringLiteral("%1__VERSION__.cpp"))
                                     .arg(baseFileName);
    QString profileSuffix;
    if (versionProfile.hasProfiles())
        profileSuffix = (versionProfile.profile == VersionProfile::CoreProfile ? QStringLiteral("_core") : QStringLiteral("_compatibility"));

    const QString versionProfileString = QString(QStringLiteral("_%1_%2%3"))
                                         .arg(versionProfile.version.major)
                                         .arg(versionProfile.version.minor)
                                         .arg(profileSuffix);
    writePreamble(templateFileName, stream, versionProfileString);

    const QString className = generateClassName(versionProfile, Public);
    stream << QStringLiteral("/*!") << endl
           << QStringLiteral("    \\class ") << className << endl
           << QStringLiteral("    \\inmodule QtGui") << endl
           << QStringLiteral("    \\since 5.1") << endl
           << QStringLiteral("    \\wrapper") << endl
           << QStringLiteral("    \\brief The ") << className
           << QString(QStringLiteral(" class provides all functions for OpenGL %1.%2 "))
                .arg(versionProfile.version.major)
                .arg(versionProfile.version.minor);

    if (!profileSuffix.isEmpty()) {
        profileSuffix.remove(0, 1);
        profileSuffix.append(QStringLiteral(" profile"));
    } else {
        profileSuffix = "specification";
    }

    stream << profileSuffix << QStringLiteral(".") << endl << endl
           << QStringLiteral("    This class is a wrapper for functions from ")
           << QString(QStringLiteral("OpenGL %1.%2 "))
                .arg(versionProfile.version.major)
                .arg(versionProfile.version.minor)
           << profileSuffix << QStringLiteral(".") << endl
           << QStringLiteral("    See reference pages on \\l {http://www.opengl.org/sdk/docs/}{opengl.org}") << endl
           << QStringLiteral("    for function documentation.") << endl << endl
           << QStringLiteral("    \\sa QAbstractOpenGLFunctions") << endl
           << QStringLiteral("*/") << endl << endl;

    // Get the data we'll need for this class implementation
    FunctionCollection functionSet = functionCollection(versionProfile);
    QList<VersionProfile> backends = backendsForFunctionCollection(functionSet);

    // Output default constructor
    stream << className << QStringLiteral("::") << className << QStringLiteral("()") << endl;
    stream << QStringLiteral(" : ") << baseClass << QStringLiteral("()");
    Q_FOREACH (const VersionProfile &v, backends)
        stream << endl << QString(QStringLiteral(" , %1(0)")).arg(backendVariableName(v));
    stream << endl << QStringLiteral("{") << endl << QStringLiteral("}") << endl << endl;

    // Output the destructor
    stream << className << QStringLiteral("::~") << className << QStringLiteral("()") << endl;
    stream << QStringLiteral("{") << endl;
    Q_FOREACH (const VersionProfile &v, backends) {
        const QString backendVar = backendVariableName(v);
        const QString backendClass = backendClassName(v);
        stream << QString(QStringLiteral("    if (%1 && !%1->refs.deref()) {")).arg(backendVar) << endl;
        stream << QString(QStringLiteral("        QAbstractOpenGLFunctionsPrivate::removeFunctionsBackend(%1->context, %2::versionStatus());"))
                  .arg(backendVar)
                  .arg(backendClass) << endl;
        stream << QString(QStringLiteral("        delete %1;")).arg(backendVar) << endl;
        stream << QStringLiteral("    }") << endl;
    }
    stream << QStringLiteral("}") << endl << endl;

    // Output the initialize function that creates the backend objects
    stream << QString(QStringLiteral("bool %1::initializeOpenGLFunctions()")).arg(className) << endl;
    stream << QStringLiteral("{") << endl;

    stream << QStringLiteral("    if ( isInitialized() )") << endl;
    stream << QStringLiteral("        return true;") << endl << endl;
    stream << QStringLiteral("    QOpenGLContext* context = QOpenGLContext::currentContext();") << endl << endl;
    stream << QStringLiteral("    // If owned by a context object make sure it is current.") << endl;
    stream << QStringLiteral("    // Also check that current context is capable of resolving all needed functions") << endl;
    stream << QStringLiteral("    if (((owningContext() && owningContext() == context) || !owningContext())") << endl;
    stream << QString(QStringLiteral("        && %1::isContextCompatible(context))")).arg(className) << endl;
    stream << QStringLiteral("    {") << endl;
    stream << QStringLiteral("        // Associate with private implementation, creating if necessary") << endl;
    stream << QStringLiteral("        // Function pointers in the backends are resolved at creation time") << endl;
    stream << QStringLiteral("        QOpenGLVersionFunctionsBackend* d = 0;") << endl;

    Q_FOREACH (const VersionProfile &v, backends) {
        const QString backendClass = backendClassName(v);
        const QString backendVar = backendVariableName(v);
        stream << QString(QStringLiteral("        d = QAbstractOpenGLFunctionsPrivate::functionsBackend(context, %1::versionStatus());"))
                  .arg(backendClass) << endl;
        stream << QStringLiteral("        if (!d) {") << endl;
        stream << QString(QStringLiteral("            d = new %1(context);")).arg(backendClass) << endl;
        stream << QString(QStringLiteral("            QAbstractOpenGLFunctionsPrivate::insertFunctionsBackend(context, %1::versionStatus(), d);"))
                  .arg(backendClass) << endl;
        stream << QStringLiteral("        }") << endl;
        stream << QString(QStringLiteral("        %1 = static_cast<%2*>(d);")).arg(backendVar).arg(backendClass) << endl;
        stream << QStringLiteral("        d->refs.ref();") << endl << endl;
    }

    stream << QStringLiteral("        QAbstractOpenGLFunctions::initializeOpenGLFunctions();") << endl;
    stream << QStringLiteral("    }") << endl;

    stream << QStringLiteral("    return isInitialized();") << endl;
    stream << QStringLiteral("}") << endl << endl;

    // Output the context compatibility check function
    stream << QString(QStringLiteral("bool %1::isContextCompatible(QOpenGLContext *context)")).arg(className) << endl;
    stream << QStringLiteral("{") << endl;
    stream << QStringLiteral("    Q_ASSERT(context);") << endl;
    stream << QStringLiteral("    QSurfaceFormat f = context->format();") << endl;
    stream << QStringLiteral("    const QPair<int, int> v = qMakePair(f.majorVersion(), f.minorVersion());") << endl;
    stream << QString(QStringLiteral("    if (v < qMakePair(%1, %2))"))
              .arg(versionProfile.version.major)
              .arg(versionProfile.version.minor) << endl;
    stream << QStringLiteral("        return false;") << endl << endl;

    // If generating a legacy or compatibility profile class we need to ensure that
    // the context does not expose only core functions
    if (versionProfile.profile != VersionProfile::CoreProfile) {
        stream << QStringLiteral("    if (f.profile() == QSurfaceFormat::CoreProfile)") << endl;
        stream << QStringLiteral("        return false;") << endl << endl;
    }

    stream << QStringLiteral("    return true;") << endl;
    stream << QStringLiteral("}") << endl << endl;

    // Output static function used as helper in template versionFunctions() function
    // in QOpenGLContext
    stream << QString(QStringLiteral("QOpenGLVersionProfile %1::versionProfile()")).arg(className) << endl;
    stream << QStringLiteral("{") << endl;
    stream << QStringLiteral("    QOpenGLVersionProfile v;") << endl;
    stream << QString(QStringLiteral("    v.setVersion(%1, %2);"))
              .arg(versionProfile.version.major)
              .arg(versionProfile.version.minor) << endl;
    if (versionProfile.hasProfiles()) {
        const QString profileName = versionProfile.profile == VersionProfile::CoreProfile
                                    ? QStringLiteral("QSurfaceFormat::CoreProfile")
                                    : QStringLiteral("QSurfaceFormat::CompatibilityProfile");
        stream << QString(QStringLiteral("    v.setProfile(%1);")).arg(profileName) << endl;
    }
    stream << QStringLiteral("    return v;") << endl;
    stream << QStringLiteral("}") << endl;

    // Write the postamble
    writePostamble(templateFileName, stream);
}

void CodeGenerator::writeClassFunctionDeclarations(QTextStream &stream,
                                                   const FunctionCollection &functionSet,
                                                   ClassVisibility visibility) const
{
    Q_FOREACH (const VersionProfile &version, functionSet.keys()) {
        // Add a comment to the header
        stream << QString(QStringLiteral("    // OpenGL %1.%2 %3 functions"))
                  .arg(version.version.major)
                  .arg(version.version.minor)
                  .arg((version.profile == VersionProfile::CoreProfile) ? QStringLiteral("core") : QStringLiteral("deprecated"))
               << endl;

        // Output function declarations
        FunctionList functions = functionSet.value(version);
        Q_FOREACH (const Function &f, functions)
            writeFunctionDeclaration(stream, f, visibility);
        stream << endl;
    } // version and profile
}

void CodeGenerator::writeFunctionDeclaration(QTextStream &stream, const Function &f, ClassVisibility visibility) const
{
    QStringList argList;
    Q_FOREACH (const Argument &arg, f.arguments) {
        QString a = QString(QStringLiteral("%1%2 %3%4"))
                    .arg((arg.direction == Argument::In && arg.mode != Argument::Value) ? QStringLiteral("const ") : QString())
                    .arg(arg.type)
                    .arg(passByType(arg))
                    .arg(safeArgumentName(arg.name));
        argList.append(a);
    }
    QString args = argList.join(QStringLiteral(", "));

    QString signature;
    switch (visibility) {
    case Public:
        signature = QString(QStringLiteral("    %1 gl%2(%3);")).arg(f.returnType).arg(f.name).arg(args);
        break;

    case Private:
    default:
        signature = QString(QStringLiteral("    %1 (QOPENGLF_APIENTRYP %2)(%3);")).arg(f.returnType).arg(f.name).arg(args);
    }
    stream << signature << endl;
}

void CodeGenerator::writeClassInlineFunctions(QTextStream &stream,
                                              const QString &className,
                                              const FunctionCollection &functionSet) const
{
    Q_FOREACH (const VersionProfile &version, functionSet.keys()) {

        // Add a comment to the header
        stream << QString(QStringLiteral("// OpenGL %1.%2 %3 functions"))
                  .arg(version.version.major)
                  .arg(version.version.minor)
                  .arg((version.profile == VersionProfile::CoreProfile) ? QStringLiteral("core") : QStringLiteral("deprecated"))
               << endl;

        // Output function declarations
        const QString backendVar = backendVariableName(version);
        FunctionList functions = functionSet.value(version);
        Q_FOREACH (const Function &f, functions)
            writeInlineFunction(stream, className, backendVar, f);

        stream << endl;

    } // version and profile
}

void CodeGenerator::writeInlineFunction(QTextStream &stream, const QString &className,
                                        const QString &backendVar, const Function &f) const
{
    QStringList argList;
    Q_FOREACH (const Argument &arg, f.arguments) {
        QString a = QString(QStringLiteral("%1%2 %3%4"))
                    .arg((arg.direction == Argument::In && arg.mode != Argument::Value) ? QStringLiteral("const ") : QString())
                    .arg(arg.type)
                    .arg(passByType(arg))
                    .arg(safeArgumentName(arg.name));
        argList.append(a);
    }
    QString args = argList.join(", ");


    QString signature = QString(QStringLiteral("inline %1 %2::gl%3(%4)"))
                        .arg(f.returnType)
                        .arg(className)
                        .arg(f.name)
                        .arg(args);
    stream << signature << endl << QStringLiteral("{") << endl;

    QStringList argumentNames;
    Q_FOREACH (const Argument &arg, f.arguments)
        argumentNames.append(safeArgumentName(arg.name));
    QString argNames = argumentNames.join(", ");

    if (f.returnType == QStringLiteral("void"))
        stream << QString(QStringLiteral("    %1->%2(%3);")).arg(backendVar).arg(f.name).arg(argNames) << endl;
    else
        stream << QString(QStringLiteral("    return %1->%2(%3);")).arg(backendVar).arg(f.name).arg(argNames) << endl;
    stream << QStringLiteral("}") << endl << endl;
}

void CodeGenerator::writeEntryPointResolutionCode(QTextStream &stream,
                                                  const FunctionCollection &functionSet) const
{
    bool hasModuleHandle = false;
    Q_FOREACH (const VersionProfile &version, functionSet.keys()) {

        // Add a comment to the header
        stream << QString(QStringLiteral("    // OpenGL %1.%2 %3 functions"))
                  .arg(version.version.major)
                  .arg(version.version.minor)
                  .arg((version.profile == VersionProfile::CoreProfile) ? QStringLiteral("core") : QStringLiteral("deprecated"))
               << endl;

        // Output function declarations
        FunctionList functions = functionSet.value(version);

        bool useGetProcAddress = (version.version.major == 1 && (version.version.minor == 0 || version.version.minor == 1));
        if (useGetProcAddress) {
            stream << "#if defined(Q_OS_WIN)" << endl;
            if (!hasModuleHandle) {
                stream << "    HMODULE handle = GetModuleHandleA(\"opengl32.dll\");" << endl;
                hasModuleHandle = true;
            }

            Q_FOREACH (const Function &f, functions)
                writeEntryPointResolutionStatement(stream, f, QString(), useGetProcAddress);

            stream << "#else" << endl;
        }

        Q_FOREACH (const Function &f, functions)
            writeEntryPointResolutionStatement(stream, f);

        if (useGetProcAddress)
            stream << "#endif" << endl;

        stream << endl;

    } // version and profile
}

void CodeGenerator::writeEntryPointResolutionStatement(QTextStream &stream, const Function &f,
                                                       const QString &prefix, bool useGetProcAddress) const
{
    QStringList argList;
    Q_FOREACH (const Argument &arg, f.arguments) {
        QString a = QString("%1%2 %3")
                    .arg((arg.direction == Argument::In && arg.mode != Argument::Value) ? QStringLiteral("const ") : QString())
                    .arg(arg.type)
                    .arg(passByType(arg));
        argList.append(a);
    }
    QString args = argList.join(QStringLiteral(", "));

    QString signature;
    if (!useGetProcAddress) {
        signature = QString(QStringLiteral("    %4%3 = reinterpret_cast<%1 (QOPENGLF_APIENTRYP)(%2)>(context->getProcAddress(\"gl%3\"));"))
                    .arg(f.returnType)
                    .arg(args)
                    .arg(f.name)
                    .arg(prefix);
    } else {
        signature = QString(QStringLiteral("    %4%3 = reinterpret_cast<%1 (QOPENGLF_APIENTRYP)(%2)>(GetProcAddress(handle, \"gl%3\"));"))
                    .arg(f.returnType)
                    .arg(args)
                    .arg(f.name)
                    .arg(prefix);
    }
    stream << signature << endl;
}

QList<VersionProfile> CodeGenerator::backendsForFunctionCollection(const FunctionCollection &functionSet) const
{
    QList<VersionProfile> backends;
    Q_FOREACH (const VersionProfile &versionProfile, functionSet.keys()) {
        if (m_parser->versionProfiles().contains(versionProfile))
            backends.append(versionProfile);
    }
    return backends;
}

QString CodeGenerator::backendClassName(const VersionProfile &v) const
{
    QString statusSuffix = v.profile == VersionProfile::CoreProfile
                         ? QStringLiteral("_Core")
                         : QStringLiteral("_Deprecated");
    const QString className = QString(QStringLiteral("QOpenGLFunctions_%1_%2%3Backend"))
                              .arg(v.version.major)
                              .arg(v.version.minor)
                              .arg(statusSuffix);
    return className;
}

QString CodeGenerator::backendVariableName(const VersionProfile &v) const
{
    const QString status = (v.profile == VersionProfile::CoreProfile)
                         ? QStringLiteral("Core")
                         : QStringLiteral("Deprecated");
    const QString varName = QString(QStringLiteral("d_%1_%2_%3"))
                            .arg(v.version.major)
                            .arg(v.version.minor)
                            .arg(status);
    return varName;
}

void CodeGenerator::writeBackendVariableDeclarations(QTextStream &stream, const QList<VersionProfile> &backends) const
{
    // We need a private class for each version and profile (status: core or deprecated)
    Q_FOREACH (const VersionProfile &v, backends) {
        const QString className = backendClassName(v);
        const QString varName = backendVariableName(v);
        stream << QString(QStringLiteral("    %1* %2;")).arg(className).arg(varName) << endl;
    }
}

void CodeGenerator::writeExtensionHeader(const QString &fileName) const
{
    if (!m_parser)
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream stream(&file);

    // Write the preamble
    writePreamble(fileName, stream);

    // Iterate through the list of extensions and create one class per extension
    QStringList extensions = m_parser->extensions();
    Q_FOREACH (const QString &extension, extensions) {
        writeExtensionClassDeclaration(stream, extension, Private);
        writeExtensionClassDeclaration(stream, extension, Public);
    }

    // Write the postamble
    writePostamble(fileName, stream);
}

void CodeGenerator::writeExtensionImplementation(const QString &fileName) const
{
    if (!m_parser)
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream stream(&file);

    // Write the preamble
    writePreamble(fileName, stream);

    // Iterate through the list of extensions and create one class per extension
    QStringList extensions = m_parser->extensions();
    Q_FOREACH (const QString &extension, extensions)
        writeExtensionClassImplementation(stream, extension);

    // Write the postamble
    writePostamble(fileName, stream);
}

void CodeGenerator::writeExtensionClassDeclaration(QTextStream &stream, const QString &extension, ClassVisibility visibility) const
{
    const QString className = generateExtensionClassName(extension, visibility);

    QString baseClass = (visibility == Public) ? QStringLiteral("QAbstractOpenGLExtension") : QStringLiteral("QAbstractOpenGLExtensionPrivate");

    stream << QString(QStringLiteral("class %2 : public %3"))
              .arg(className)
              .arg(baseClass)
           << endl << "{" << endl << "public:" << endl;

    if (visibility == Public) {
        // Default constructor
        stream << QStringLiteral("    ") << className << QStringLiteral("();") << endl << endl;

        // Base class virtual function(s)
        QString resolveFunction = QStringLiteral("    bool initializeOpenGLFunctions() Q_DECL_FINAL;");
        stream << resolveFunction << endl << endl;
    }

    // Output the functions provided by this extension
    QList<Function> functions = m_parser->functionsForExtension(extension);
    Q_FOREACH (const Function &f, functions)
        writeFunctionDeclaration(stream, f, visibility);

    if (visibility == Public) {
        // Write out the protected ctor
        stream << endl << QStringLiteral("protected:") << endl;
        stream << QStringLiteral("    Q_DECLARE_PRIVATE(") << className << QStringLiteral(")") << endl;
    }

    // End the class declaration
    stream << QStringLiteral("};") << endl << endl;

    // Output the inline functions for public class
    if (visibility == Public) {
        Q_FOREACH (const Function &f, functions)
            writeExtensionInlineFunction(stream, className, f);
    }
}

void CodeGenerator::writeExtensionInlineFunction(QTextStream &stream, const QString &className, const Function &f) const
{
    QStringList argList;
    Q_FOREACH (const Argument &arg, f.arguments) {
        QString a = QString(QStringLiteral("%1%2 %3%4"))
                    .arg((arg.direction == Argument::In && arg.mode != Argument::Value) ? QStringLiteral("const ") : QString())
                    .arg(arg.type)
                    .arg(passByType(arg))
                    .arg(safeArgumentName(arg.name));
        argList.append(a);
    }
    QString args = argList.join(", ");


    QString signature = QString(QStringLiteral("inline %1 %2::gl%3(%4)"))
                        .arg(f.returnType)
                        .arg(className)
                        .arg(f.name)
                        .arg(args);
    stream << signature << endl << QStringLiteral("{") << endl;

    stream << QString(QStringLiteral("    Q_D(%1);")).arg(className) << endl;

    QStringList argumentNames;
    Q_FOREACH (const Argument &arg, f.arguments)
        argumentNames.append(safeArgumentName(arg.name));
    QString argNames = argumentNames.join(", ");

    if (f.returnType == QStringLiteral("void"))
        stream << QString(QStringLiteral("    d->%1(%2);")).arg(f.name).arg(argNames) << endl;
    else
        stream << QString(QStringLiteral("    return d->%1(%2);")).arg(f.name).arg(argNames) << endl;
    stream << QStringLiteral("}") << endl << endl;
}

void CodeGenerator::writeExtensionClassImplementation(QTextStream &stream, const QString &extension) const
{
    const QString className = generateExtensionClassName(extension);
    const QString privateClassName = generateExtensionClassName(extension, Private);

    // Output default constructor
    stream << className << QStringLiteral("::") << className << QStringLiteral("()") << endl;
    stream << QStringLiteral(" : QAbstractOpenGLExtension(*(new ") << privateClassName << QStringLiteral("))") << endl;
    stream << QStringLiteral("{") << endl << QStringLiteral("}") << endl << endl;


    // Output function to initialize this class
    stream << QStringLiteral("bool ") << className
           << QStringLiteral("::initializeOpenGLFunctions()") << endl
           << QStringLiteral("{") << endl;

    stream << QStringLiteral("    if (isInitialized())") << endl;
    stream << QStringLiteral("        return true;") << endl << endl;

    stream << QStringLiteral("    QOpenGLContext *context = QOpenGLContext::currentContext();") << endl;
    stream << QStringLiteral("    if (!context) {") << endl;
    stream << QStringLiteral("        qWarning(\"A current OpenGL context is required to resolve OpenGL extension functions\");")
           << endl;
    stream << QStringLiteral("        return false;") << endl;
    stream << QStringLiteral("    }") << endl << endl;

    // Output code to resolve entry points for this class
    stream << QStringLiteral("    // Resolve the functions") << endl;
    stream << QStringLiteral("    Q_D(") << className << QStringLiteral(");") << endl;
    stream << endl;

    // Output function declarations
    QList<Function> functions = m_parser->functionsForExtension(extension);
    Q_FOREACH (const Function &f, functions)
        writeEntryPointResolutionStatement(stream, f, QStringLiteral("d->"));

    // Call the base class implementation
    stream << QStringLiteral("    QAbstractOpenGLExtension::initializeOpenGLFunctions();") << endl;

    // Finish off
    stream << QStringLiteral("    return true;") << endl;
    stream << QStringLiteral("}") << endl << endl;
}

QString CodeGenerator::generateExtensionClassName(const QString &extension, ClassVisibility visibility) const
{
    QString visibilitySuffix;
    if (visibility == Private)
        visibilitySuffix = QStringLiteral("Private");

    return QString(QStringLiteral("QOpenGLExtension_%1%2"))
            .arg(extension)
            .arg(visibilitySuffix);
}
