/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake application of the Qt Toolkit.
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

#include "symmake_sbsv2.h"
#include "initprojectdeploy_symbian.h"

#include <qstring.h>
#include <qstringlist.h>
#include <qdir.h>
#include <qdatetime.h>
#include <qdebug.h>

// Included from tools/shared
#include <symbian/epocroot_p.h>

SymbianSbsv2MakefileGenerator::SymbianSbsv2MakefileGenerator() : SymbianMakefileGenerator() { }
SymbianSbsv2MakefileGenerator::~SymbianSbsv2MakefileGenerator() { }

#define FLM_DEST_DIR "epoc32/tools/makefile_templates/qt"
#define FLM_SOURCE_DIR "/mkspecs/symbian-sbsv2/flm/qt"
#define PLATFORM_GCCE "gcce"
#define PLATFORM_WINSCW "winscw"
#define PLATFORM_ARM_PREFIX "arm"
#define BUILD_DEBUG "udeb"
#define BUILD_RELEASE "urel"
#define SBS_RVCT_PREFIX "rvct"

static QString winscwPlatform;
static QString armPlatformPrefix;
static QString gccePlatform;
static QString sbsRvctPrefix;

#if defined(Q_OS_UNIX)
    extern char **environ;
#endif

static void fixFlmCmd(QString *cmdLine, const QMap<QString, QString> &commandsToReplace)
{
    // If commandItem starts with any $$QMAKE_* commands, do a replace for SBS equivalent.
    // Command replacement is done only for the start of the command or right after
    // concatenation operators (&& and ||), as otherwise unwanted replacements might occur.
    static QString cmdFind(QLatin1String("(^|&&\\s*|\\|\\|\\s*)%1"));
    static QString cmdReplace(QLatin1String("\\1%1"));

    // $$escape_expand(\\n\\t) doesn't work for bld.inf files, but is often used as command
    // separator, so replace it with "&&" command concatenator.
    cmdLine->replace("\n\t", "&&");

    // Strip output suppression, as sbsv2 can't handle it in FLMs. Cannot be done by simply
    // adding "@" to commandsToReplace, as it'd get handled last due to alphabetical ordering,
    // potentially masking other commands that need replacing.
    if (cmdLine->contains("@"))
        cmdLine->replace(QRegExp(cmdFind.arg("@")), cmdReplace.arg(""));

    // Iterate command replacements in reverse alphabetical order of keys so
    // that keys which are starts of other longer keys are iterated after longer keys.
    QMapIterator<QString, QString> cmdIter(commandsToReplace);
    cmdIter.toBack();
    while (cmdIter.hasPrevious()) {
        cmdIter.previous();
        if (cmdLine->contains(cmdIter.key()))
            cmdLine->replace(QRegExp(cmdFind.arg(cmdIter.key())), cmdReplace.arg(cmdIter.value()));
    }

    // Sbsv2 toolchain strips all backslashes (even double ones) from option parameters, so just
    // assume all backslashes are directory separators and replace them with slashes.
    // Problem: If some command actually needs backslashes for something else than dir separator,
    // we are out of luck.
    cmdLine->replace("\\", "/");
}

// Copies Qt FLMs to correct location under epocroot.
// This is not done by configure as it is possible to change epocroot after configure.
void SymbianSbsv2MakefileGenerator::exportFlm()
{
    static bool flmExportDone = false;

    if (!flmExportDone) {
        QDir sourceDir = QDir(QLibraryInfo::location(QLibraryInfo::PrefixPath) + FLM_SOURCE_DIR);
        QFileInfoList sourceInfos = sourceDir.entryInfoList(QDir::Files);

        QDir destDir(qt_epocRoot() + FLM_DEST_DIR);
        if (!destDir.exists()) {
            if (destDir.mkpath(destDir.absolutePath()))
                generatedDirs << destDir.absolutePath();
        }

        foreach(QFileInfo item, sourceInfos) {
            QFileInfo destInfo = QFileInfo(destDir.absolutePath() + "/" + item.fileName());
            if (!destInfo.exists() || destInfo.lastModified() != item.lastModified()) {
                if (destInfo.exists())
                    QFile::remove(destInfo.absoluteFilePath());
                if (QFile::copy(item.absoluteFilePath(), destInfo.absoluteFilePath()))
                    generatedFiles << destInfo.absoluteFilePath();
                else
                    fprintf(stderr, "Error: Could not copy '%s' -> '%s'\n",
                            qPrintable(item.absoluteFilePath()),
                            qPrintable(destInfo.absoluteFilePath()));
            }
        }
        flmExportDone = true;
    }
}

void SymbianSbsv2MakefileGenerator::findInstalledCompilerVersions(const QString &matchExpression,
                                                                  const QString &versionPrefix,
                                                                  QStringList *versionList)
{
    // No need to be able to find env variables on other operating systems,
    // as only linux and windows have support for symbian-sbsv2 toolchain
#if defined(Q_OS_UNIX) || defined(Q_OS_WIN)
    char *entry = 0;
    int count = 0;
    QRegExp matcher(matchExpression);
    while ((entry = environ[count++])) {
        if (matcher.exactMatch(QString::fromLocal8Bit(entry))
            && fileInfo(matcher.cap(matcher.captureCount())).exists()) {
            // First capture (index 0) is the whole match, which is skipped.
            // Next n captures are version numbers, which are interesting.
            // Final capture is the env var value, which we already used, so that is skipped, too.
            int capture = 1;
            int finalCapture = matcher.captureCount() - 1;
            QString version = versionPrefix;
            while (capture <= finalCapture) {
                version.append(matcher.cap(capture));
                if (capture != finalCapture)
                    version.append(QLatin1Char('.'));
                capture++;
            }
            *versionList << version;
        }
    }
#endif
}

void SymbianSbsv2MakefileGenerator::findGcceVersions(QStringList *gcceVersionList,
                                                     QString *defaultVersion)
{
    QString matchStr = QLatin1String("SBS_GCCE(\\d)(\\d)(\\d)BIN=(.*)");
    findInstalledCompilerVersions(matchStr, gccePlatform, gcceVersionList);

    QString qtGcceVersion = QString::fromLocal8Bit(qgetenv("QT_GCCE_VERSION"));

    if (!qtGcceVersion.isEmpty()) {
        if (QRegExp("\\d+\\.\\d+\\.\\d+").exactMatch(qtGcceVersion)) {
            *defaultVersion = gccePlatform + qtGcceVersion;
        } else {
            fprintf(stderr, "Warning: Variable QT_GCCE_VERSION ('%s') is in incorrect "
                    "format, expected format is: 'x.y.z'. Attempting to autodetect GCCE version.\n",
                    qPrintable(qtGcceVersion));
        }
    }

    if (defaultVersion->isEmpty() && gcceVersionList->size()) {
        gcceVersionList->sort();
        *defaultVersion = gcceVersionList->last();
    }
}

void SymbianSbsv2MakefileGenerator::findRvctVersions(QStringList *rvctVersionList,
                                                     QString *defaultVersion)
{
    QString matchStr = QLatin1String("RVCT(\\d)(\\d)BIN=(.*)");
    findInstalledCompilerVersions(matchStr, sbsRvctPrefix, rvctVersionList);

    QString qtRvctVersion = QString::fromLocal8Bit(qgetenv("QT_RVCT_VERSION"));

    if (!qtRvctVersion.isEmpty()) {
        if (QRegExp("\\d+\\.\\d+").exactMatch(qtRvctVersion)) {
            *defaultVersion = sbsRvctPrefix + qtRvctVersion;
        } else {
            fprintf(stderr, "Warning: Variable QT_RVCT_VERSION ('%s') is in incorrect "
                    "format, expected format is: 'x.y'.\n",
                    qPrintable(qtRvctVersion));
        }
    }
}

QString SymbianSbsv2MakefileGenerator::configClause(const QString &platform,
                                                    const QString &build,
                                                    const QString &compilerVersion,
                                                    const QString &clauseTemplate)
{
    QString retval;
    if (QString::compare(platform, winscwPlatform) == 0) {
        retval = clauseTemplate.arg(build);
    } else if (platform.startsWith(armPlatformPrefix)) {
        QString fixedCompilerVersion = compilerVersion;
        fixedCompilerVersion.replace(".","_");
        retval = clauseTemplate.arg(platform.mid(sizeof(PLATFORM_ARM_PREFIX)-1))
                                  .arg(build)
                                  .arg(fixedCompilerVersion);
    } // else - Unsupported platform for makefile target, return empty clause
    return retval;
}

void SymbianSbsv2MakefileGenerator::writeSbsDeploymentList(const DeploymentList& depList, QTextStream& t)
{
    for (int i = 0; i < depList.size(); ++i) {
        t << "START EXTENSION qt/qmake_emulator_deployment" << endl;
        QString fromItem = depList.at(i).from;
        QString toItem = depList.at(i).to;
        fromItem.replace("\\", "/");
        toItem.replace("\\", "/");
#if defined(Q_OS_WIN)
        // add drive if it doesn't have one yet
        if (toItem.size() > 1 && toItem[1] != QLatin1Char(':'))
            toItem.prepend(QDir::current().absolutePath().left(2));
#endif
        t << "OPTION DEPLOY_SOURCE " << fromItem << endl;
        t << "OPTION DEPLOY_TARGET " << toItem << endl;
        t << "END" << endl;
    }
}

void SymbianSbsv2MakefileGenerator::writeMkFile(const QString& wrapperFileName, bool deploymentOnly)
{
    // Can't use extension makefile with sbsv2
    Q_UNUSED(wrapperFileName);
    Q_UNUSED(deploymentOnly);
}

void SymbianSbsv2MakefileGenerator::writeWrapperMakefile(QFile& wrapperFile, bool isPrimaryMakefile)
{
    static QString debugBuild;
    static QString releaseBuild;
    static QString defaultGcceCompilerVersion;
    static QString defaultRvctCompilerVersion;
    static QStringList rvctVersions;
    static QStringList gcceVersions;
    static QStringList allArmCompilerVersions;

    // Initialize static variables used in makefile creation
    if (debugBuild.isEmpty()) {
        debugBuild.append(QLatin1String(BUILD_DEBUG));
        releaseBuild.append(QLatin1String(BUILD_RELEASE));
        winscwPlatform.append(QLatin1String(PLATFORM_WINSCW));
        gccePlatform.append(QLatin1String(PLATFORM_GCCE));
        armPlatformPrefix.append(QLatin1String(PLATFORM_ARM_PREFIX));
        sbsRvctPrefix.append(QLatin1String(SBS_RVCT_PREFIX));

        findGcceVersions(&gcceVersions, &defaultGcceCompilerVersion);
        findRvctVersions(&rvctVersions, &defaultRvctCompilerVersion);

        allArmCompilerVersions << rvctVersions << gcceVersions;

        if (!allArmCompilerVersions.size()) {
            fprintf(stderr, "Warning: No HW compilers detected. "
                    "Please install either GCCE or RVCT compiler to enable release builds.\n");
        }
    }

    QStringList allPlatforms;
    foreach(QString platform, project->values("SYMBIAN_PLATFORMS")) {
        allPlatforms << platform.toLower();
    }

    if (!gcceVersions.size())
        allPlatforms.removeAll(gccePlatform);

    QString testClause;
    if (project->isActiveConfig(SYMBIAN_TEST_CONFIG))
        testClause = QLatin1String(".test");
    else
        testClause = QLatin1String("");

    // Note: armClause is used for gcce, too, which has a side effect
    //       of requiring armv* platform(s) in SYMBIAN_PLATFORMS in order
    //       to get any compiler version specific targets.
    QString armClause = " -c " PLATFORM_ARM_PREFIX ".%1.%2.%3" + testClause;
    QString genericArmClause;
    if (defaultRvctCompilerVersion.isEmpty()) {
        // Note: Argument %3 needs to be empty string in this version of clause
        genericArmClause = " -c " PLATFORM_ARM_PREFIX "%1_%2%3" + testClause;
    } else {
        // If defaultRvctCompilerVersion is defined, use specific sbs clause for "generic" clause
        genericArmClause = armClause;
    }
    QString winscwClause = " -c " PLATFORM_WINSCW "_%1.mwccinc" + testClause;;

    QStringList armPlatforms = allPlatforms.filter(QRegExp("^" PLATFORM_ARM_PREFIX));

    if (!allArmCompilerVersions.size()) {
        foreach (QString item, armPlatforms) {
            allPlatforms.removeAll(item);
        }
        armPlatforms.clear();
    }

    QStringList allClauses;
    QStringList debugClauses;
    QStringList releaseClauses;

    // Only winscw and arm platforms are supported
    QStringList debugPlatforms = allPlatforms;
    QStringList releasePlatforms = allPlatforms;
    releasePlatforms.removeAll(winscwPlatform); // No release for emulator

    if (!releasePlatforms.size()) {
        fprintf(stderr, "Warning: No valid release platforms in SYMBIAN_PLATFORMS (%s)\n"
                "Most likely required compiler(s) are not properly installed.\n",
                qPrintable(project->values("SYMBIAN_PLATFORMS").join(" ")));
    }

    if (debugPlatforms.contains(winscwPlatform))
        debugClauses << configClause(winscwPlatform, debugBuild, QString(), winscwClause);

    foreach(QString item, armPlatforms) {
        // Only use single clause per arm platform even if multiple compiler versions were found,
        // otherwise we get makefile target collisions from sbsv2 toolchain.
        if (rvctVersions.size()) {
            debugClauses << configClause(item, debugBuild, defaultRvctCompilerVersion, genericArmClause);
            releaseClauses << configClause(item, releaseBuild, defaultRvctCompilerVersion, genericArmClause);
        } else {
            debugClauses << configClause(item, debugBuild, defaultGcceCompilerVersion, armClause);
            releaseClauses << configClause(item, releaseBuild, defaultGcceCompilerVersion, armClause);
        }
    }

    allClauses << debugClauses << releaseClauses;

    QTextStream t(&wrapperFile);

    MakefileGenerator::writeHeader(t);

    t << "MAKEFILE          = " << fileInfo(wrapperFile.fileName()).fileName() << endl;
    t << "QMAKE             = " << var("QMAKE_QMAKE") << endl;
    t << "DEL_FILE          = " << var("QMAKE_DEL_FILE") << endl;
    t << "DEL_DIR           = " << var("QMAKE_DEL_DIR") << endl;
    t << "CHK_DIR_EXISTS    = " << var("QMAKE_CHK_DIR_EXISTS") << endl;
    t << "MKDIR             = " << var("QMAKE_MKDIR") << endl;
    t << "MOVE              = " << var("QMAKE_MOVE") << endl;
    t << "DEBUG_PLATFORMS   = " << debugPlatforms.join(" ") << endl;
    t << "RELEASE_PLATFORMS = " << releasePlatforms.join(" ") << endl;
    t << "MAKE              = make" << endl;
    t << "SBS               = sbs" << endl;
    t << endl;
    t << "DEFINES" << '\t' << " = "
        << varGlue("PRL_EXPORT_DEFINES","-D"," -D"," ")
        << varGlue("QMAKE_COMPILER_DEFINES", "-D", "-D", " ")
        << varGlue("DEFINES","-D"," -D","") << endl;

    t << "INCPATH" << '\t' << " = ";

    for (QMap<QString, QStringList>::iterator it = systeminclude.begin(); it != systeminclude.end(); ++it) {
        QStringList values = it.value();
        for (int i = 0; i < values.size(); ++i) {
            t << " -I\"" << values.at(i) << "\" ";
        }
    }

    t << endl;
    t << "first: default" << endl << endl;
    if (!isPrimaryMakefile) {
        t << "all:" << endl << endl;
        t << "default: all" << endl << endl;
    } else {
        t << "all: debug release" << endl << endl;
        if (debugPlatforms.contains(winscwPlatform))
            t << "default: debug-winscw";
        else if (debugPlatforms.size())
            t << "default: debug-" << debugPlatforms.first();
        else
            t << "default: all";
        t << endl;

        QString qmakeCmd = "\t$(QMAKE) \"" + project->projectFile() + "\" " + buildArgs();

        t << "qmake:" << endl;
        t << qmakeCmd << endl;
        t << endl;

        t << BLD_INF_FILENAME ": " << project->projectFile() << endl;
        t << qmakeCmd << endl;
        t << endl;

        QString locFileDep = generateLocFileTarget(t, qmakeCmd);

        t << "debug: " << locFileDep << BLD_INF_FILENAME << endl;
        t << "\t$(SBS)";
        foreach(QString clause, debugClauses) {
            t << clause;
        }
        t << endl;
        t << "clean-debug: " << BLD_INF_FILENAME << endl;
        t << "\t$(SBS) reallyclean --toolcheck=off";
        foreach(QString clause, debugClauses) {
            t << clause;
        }
        t << endl;

        t << "freeze-debug: " << BLD_INF_FILENAME << endl;
        t << "\t$(SBS) freeze";
        foreach(QString clause, debugClauses) {
            t << clause;
        }
        t << endl;

        t << "release: " << locFileDep << BLD_INF_FILENAME << endl;
        t << "\t$(SBS)";
        foreach(QString clause, releaseClauses) {
            t << clause;
        }
        t << endl;
        t << "clean-release: " << BLD_INF_FILENAME << endl;
        t << "\t$(SBS) reallyclean --toolcheck=off";
        foreach(QString clause, releaseClauses) {
            t << clause;
        }
        t << endl;

        t << "freeze-release: " << BLD_INF_FILENAME << endl;
        t << "\t$(SBS) freeze";
        foreach(QString clause, releaseClauses) {
            t << clause;
        }
        t << endl << endl;

        QString defaultGcceArmVersion;
        if (armPlatforms.size()) {
            defaultGcceArmVersion = armPlatforms.first();
        } else {
            defaultGcceArmVersion = QLatin1String("armv5");
        }

        // For more specific builds, targets are in this form:
        // release-armv5 - generic target, compiler version determined by toolchain or autodetection
        // release-armv5-rvct4.0 - compiler version specific target
        foreach(QString item, debugPlatforms) {
            QString clause;
            if (item.compare(winscwPlatform) == 0)
                clause = configClause(item, debugBuild, QString(), winscwClause);
            else if (item.compare(gccePlatform) == 0 )
                clause = configClause(defaultGcceArmVersion, debugBuild, defaultGcceCompilerVersion, armClause);
            else // use generic arm clause
                clause = configClause(item, debugBuild, defaultRvctCompilerVersion, genericArmClause);

            t << "debug-" << item << ": " << locFileDep << BLD_INF_FILENAME << endl;
            t << "\t$(SBS)" << clause << endl;
            t << "clean-debug-" << item << ": " << BLD_INF_FILENAME << endl;
            t << "\t$(SBS) reallyclean" << clause << endl;
            t << "freeze-debug-" << item << ": " << BLD_INF_FILENAME << endl;
            t << "\t$(SBS) freeze" << clause << endl;
        }

        foreach(QString item, releasePlatforms) {
            QString clause;
            if (item.compare(gccePlatform) == 0 )
                clause = configClause(defaultGcceArmVersion, releaseBuild, defaultGcceCompilerVersion, armClause);
            else // use generic arm clause
                clause = configClause(item, releaseBuild, defaultRvctCompilerVersion, genericArmClause);

            t << "release-" << item << ": " << locFileDep << BLD_INF_FILENAME << endl;
            t << "\t$(SBS)" << clause << endl;
            t << "clean-release-" << item << ": " << BLD_INF_FILENAME << endl;
            t << "\t$(SBS) reallyclean" << clause << endl;
            t << "freeze-release-" << item << ": " << BLD_INF_FILENAME << endl;
            t << "\t$(SBS) freeze" << clause << endl;
        }

        foreach(QString item, armPlatforms) {
            foreach(QString compilerVersion, allArmCompilerVersions) {
                QString debugClause = configClause(item, debugBuild, compilerVersion, armClause);
                QString releaseClause = configClause(item, releaseBuild, compilerVersion, armClause);
                t << "debug-" << item << "-" << compilerVersion << ": " << locFileDep << BLD_INF_FILENAME << endl;
                t << "\t$(SBS)" << debugClause << endl;
                t << "clean-debug-" << item << "-" << compilerVersion << ": " << BLD_INF_FILENAME << endl;
                t << "\t$(SBS) reallyclean" << debugClause << endl;
                t << "freeze-debug-" << item << "-" << compilerVersion << ": " << BLD_INF_FILENAME << endl;
                t << "\t$(SBS) freeze" << debugClause << endl;
                t << "release-" << item << "-" << compilerVersion << ": " << locFileDep << BLD_INF_FILENAME << endl;
                t << "\t$(SBS)" << releaseClause << endl;
                t << "clean-release-" << item << "-" << compilerVersion << ": " << BLD_INF_FILENAME << endl;
                t << "\t$(SBS) reallyclean" << releaseClause << endl;
                t << "freeze-release-" << item << "-" << compilerVersion << ": " << BLD_INF_FILENAME << endl;
                t << "\t$(SBS) freeze" << releaseClause << endl;
            }
        }

        t << endl;
        t << "export: " << BLD_INF_FILENAME << endl;
        t << "\t$(SBS) export";
        foreach(QString clause, allClauses) {
            t << clause;
        }
        t << endl << endl;

        t << "cleanexport: " << BLD_INF_FILENAME << endl;
        t << "\t$(SBS) cleanexport";
        foreach(QString clause, allClauses) {
            t << clause;
        }
        t << endl << endl;

        // Typically one wants to freeze release binaries, so make plain freeze target equal to
        // freeze-release. If freezing of debug binaries is needed for some reason, then
        // freeze-debug target should be used. There is no point to try freezing both with one
        // target as both produce the same def file.
        t << "freeze: freeze-release" << endl << endl;
    }

    // Add all extra targets including extra compiler targets also to wrapper makefile,
    // even though many of them may have already been added to bld.inf as FLMs.
    // This is to enable use of targets like 'mocables', which call targets generated by extra compilers.
    if (targetType != TypeSubdirs) {
        t << extraTargetsCache;
        t << extraCompilersCache;
    } else {
        QList<MakefileGenerator::SubTarget*> subtargets = findSubDirsSubTargets();
        writeSubTargets(t, subtargets, SubTargetSkipDefaultVariables|SubTargetSkipDefaultTargets);
        qDeleteAll(subtargets);
    }

    generateDistcleanTargets(t);

    // Do not check for tools when doing generic clean, as most tools are not actually needed for
    // cleaning. Mainly this is relevant for environments that do not have winscw compiler.
    t << "clean: " << BLD_INF_FILENAME << endl;
    t << "\t-$(SBS) reallyclean --toolcheck=off";
    foreach(QString clause, allClauses) {
        t << clause;
    }
    t << endl << endl;

    t << endl;
}

void SymbianSbsv2MakefileGenerator::writeBldInfExtensionRulesPart(QTextStream& t, const QString &iconTargetFile)
{
    // Makes sure we have needed FLMs in place.
    exportFlm();

    // Parse extra compilers data
    QStringList rawDefines;
    QStringList defines;
    QStringList incPath;

    rawDefines << project->values("PRL_EXPORT_DEFINES")
               << project->values("QMAKE_COMPILER_DEFINES")
               << project->values("DEFINES");

    // Remove defines containing doubly-escaped characters (e.g. escaped double-quotation mark
    // inside a string define) as bld.inf parsing done by sbsv2 toolchain breaks if they are
    // present.
    static QString backslashes = QLatin1String("\\\\");
    QMutableStringListIterator i(rawDefines);
    while (i.hasNext()) {
        QString val = i.next();
        if (val.indexOf(backslashes) != -1)
            i.remove();
    }

    defines << valGlue(rawDefines,"-D"," -D","");

    for (QMap<QString, QStringList>::iterator it = systeminclude.begin(); it != systeminclude.end(); ++it) {
        QStringList values = it.value();
        for (int i = 0; i < values.size(); ++i) {
            incPath << QLatin1String(" -I\"") + values.at(i) + "\"";
        }
    }

    QMap<QString, QString> commandsToReplace;
    commandsToReplace.insert(project->values("QMAKE_COPY").join(" "),
                             project->values("QMAKE_SBSV2_COPY").join(" "));
    commandsToReplace.insert(project->values("QMAKE_COPY_DIR").join(" "),
                             project->values("QMAKE_SBSV2_COPY_DIR").join(" "));
    commandsToReplace.insert(project->values("QMAKE_MOVE").join(" "),
                             project->values("QMAKE_SBSV2_MOVE").join(" "));
    commandsToReplace.insert(project->values("QMAKE_DEL_FILE").join(" "),
                             project->values("QMAKE_SBSV2_DEL_FILE").join(" "));
    commandsToReplace.insert(project->values("QMAKE_MKDIR").join(" "),
                             project->values("QMAKE_SBSV2_MKDIR").join(" "));
    commandsToReplace.insert(project->values("QMAKE_DEL_DIR").join(" "),
                             project->values("QMAKE_SBSV2_DEL_DIR").join(" "));
    commandsToReplace.insert(project->values("QMAKE_DEL_TREE").join(" "),
                             project->values("QMAKE_SBSV2_DEL_TREE").join(" "));

    // Write extra compilers and targets to initialize QMAKE_ET_* variables
    // Cache results to avoid duplicate calls when creating wrapper makefile
    QTextStream extraCompilerStream(&extraCompilersCache);
    QTextStream extraTargetStream(&extraTargetsCache);
    writeExtraCompilerTargets(extraCompilerStream);
    writeExtraTargets(extraTargetStream);

    // Figure out everything the target depends on as we don't want to run extra targets that
    // are not necessary.
    QStringList allPreDeps;
    foreach(QString item, project->values("PRE_TARGETDEPS")) {
        allPreDeps.append(QDir::cleanPath(outputDir.absoluteFilePath(item)));
    }

    foreach (QString item, project->values("GENERATED_SOURCES")) {
        allPreDeps.append(QDir::cleanPath(outputDir.absoluteFilePath(item)));
    }

    for (QMap<QString, QStringList>::iterator it = sources.begin(); it != sources.end(); ++it) {
        QString currentSourcePath = it.key();
        QStringList values = it.value();
        for (int i = 0; i < values.size(); ++i) {
            QString sourceFile = currentSourcePath + "/" + values.at(i);
            QStringList deps = findDependencies(QDir::toNativeSeparators(sourceFile));
            foreach(QString depItem, deps) {
                appendIfnotExist(allPreDeps, QDir::cleanPath(outputDir.absoluteFilePath(depItem)));
            }
        }
    }

    // Write FLM rules for all extra targets and compilers that we depend on to build the target.
    QStringList extraTargets;
    extraTargets << project->values("QMAKE_EXTRA_TARGETS") <<  project->values("QMAKE_EXTRA_COMPILERS");
    foreach(QString item, extraTargets) {
        foreach(QString targetItem, project->values(QLatin1String("QMAKE_INTERNAL_ET_PARSED_TARGETS.") + item)) {
            // Make sure targetpath is absolute
            QString absoluteTarget = QDir::cleanPath(outputDir.absoluteFilePath(targetItem));
#if defined(Q_OS_WIN)
            if (allPreDeps.contains(absoluteTarget, Qt::CaseInsensitive)) {
#else
            if (allPreDeps.contains(absoluteTarget)) {
#endif
                QStringList deps = project->values(QLatin1String("QMAKE_INTERNAL_ET_PARSED_DEPS.") + item + targetItem);
                QString commandItem =  project->values(QLatin1String("QMAKE_INTERNAL_ET_PARSED_CMD.") + item + targetItem).join(" ");

                // Make sure all deps paths are absolute
                QString absoluteDeps;
                foreach (QString depItem, deps) {
                    if (!depItem.isEmpty()) {
                        absoluteDeps.append(QDir::cleanPath(outputDir.absoluteFilePath(depItem)));
                        absoluteDeps.append(" ");
                    }
                }

                t << "START EXTENSION qt/qmake_extra_pre_targetdep.export" << endl;
                t << "OPTION PREDEP_TARGET " << absoluteTarget << endl;
                t << "OPTION DEPS " << absoluteDeps << endl;

                if (commandItem.indexOf("$(INCPATH)") != -1)
                    commandItem.replace("$(INCPATH)", incPath.join(" "));
                if (commandItem.indexOf("$(DEFINES)") != -1)
                    commandItem.replace("$(DEFINES)", defines.join(" "));

                fixFlmCmd(&commandItem, commandsToReplace);

                t << "OPTION COMMAND " << commandItem << endl;
                t << "END" << endl;
            }
        }
    }

    t << endl;

    // Write deployment rules
    QString remoteTestPath = qt_epocRoot()
        + QLatin1String("epoc32/release/winscw/udeb/z/private/") + privateDirUid;
    DeploymentList depList;

    //write emulator deployment
    // There are deployment targets for both uded and urel emulators.
    t << "#if defined(WINSCW)" << endl;
    initProjectDeploySymbian(project, depList, remoteTestPath, false, true,
        QLatin1String(EMULATOR_DEPLOYMENT_PLATFORM), QLatin1String(BUILD_DEBUG),
        generatedDirs, generatedFiles);
    writeSbsDeploymentList(depList, t);

    depList.clear();
    remoteTestPath = qt_epocRoot()
        + QLatin1String("epoc32/release/winscw/urel/z/private/") + privateDirUid;
    initProjectDeploySymbian(project, depList, remoteTestPath, false, true,
        QLatin1String(EMULATOR_DEPLOYMENT_PLATFORM), QLatin1String(BUILD_RELEASE),
        generatedDirs, generatedFiles);
    writeSbsDeploymentList(depList, t);
    t << "#endif" << endl;

    //write ROM deployment
    remoteTestPath = qt_epocRoot() + QLatin1String("epoc32/data/z/private/") + privateDirUid;
    depList.clear();
    initProjectDeploySymbian(project, depList, remoteTestPath, false, true,
        QLatin1String(ROM_DEPLOYMENT_PLATFORM), QString(), generatedDirs, generatedFiles);
    writeSbsDeploymentList(depList, t);
    t << endl;

    // Write post link rules
    if (!project->isEmpty("QMAKE_POST_LINK")) {
        QString postLinkCmd = var("QMAKE_POST_LINK");
        fixFlmCmd(&postLinkCmd, commandsToReplace);
        t << "START EXTENSION qt/qmake_post_link" << endl;
        t << "OPTION POST_LINK_CMD " << postLinkCmd << endl;
        t << "OPTION LINK_TARGET " << fixedTarget << QLatin1String(".") << getTargetExtension() << endl;
        t << "END" << endl;
        t << endl;
    }

    // Application icon generation
    QStringList icons = project->values("ICON");
    if (icons.size()) {
        QString icon = icons.first();
        if (icons.size() > 1)
            fprintf(stderr, "Warning: Only first icon specified in ICON variable is used: '%s'.", qPrintable(icon));

        t << "START EXTENSION s60/mifconv" << endl;

        QFileInfo iconInfo = fileInfo(icon);

        QString iconPath = outputDir.relativeFilePath(iconInfo.absolutePath());
        QString iconFile = iconInfo.baseName();

        QFileInfo iconTargetInfo = fileInfo(iconTargetFile);
        QString iconTarget = iconTargetInfo.fileName();

        t << "OPTION SOURCES -c32 " << iconFile << endl;
        t << "OPTION SOURCEDIR " << iconPath << endl;
        t << "OPTION TARGETFILE " << iconTarget << endl;
        t << "OPTION SVGENCODINGVERSION 3" << endl; // Compatibility with S60 3.1 devices and up
        t << "END" << endl;
    }

    t << "START EXTENSION qt/qmake_store_build" << endl;
    t << "END" << endl;
    t << endl;

    // Handle QMAKE_CLEAN
    QStringList cleanFiles = project->values("QMAKE_CLEAN");
    if (!cleanFiles.isEmpty()) {
        QStringList absoluteCleanFiles;
        foreach (QString cleanFile, cleanFiles) {
            QString fileName = QLatin1String("\"");
            fileName.append(QDir::cleanPath(outputDir.absoluteFilePath(cleanFile)));
            fileName.append(QLatin1String("\""));
            absoluteCleanFiles << fileName;
        }
        t << "START EXTENSION qt/qmake_clean" << endl;
        t << "OPTION CLEAN_FILES " << absoluteCleanFiles.join(" ") << endl;
        t << "END" << endl;
    }
    t << endl;
}

void SymbianSbsv2MakefileGenerator::writeBldInfMkFilePart(QTextStream& t, bool addDeploymentExtension)
{
    // We don't generate extension makefile in sbsb2
    Q_UNUSED(t);
    Q_UNUSED(addDeploymentExtension);
}

void SymbianSbsv2MakefileGenerator::appendAbldTempDirs(QStringList& sysincspaths, QString includepath)
{
    //Do nothing
    Q_UNUSED(sysincspaths);
    Q_UNUSED(includepath);
}
