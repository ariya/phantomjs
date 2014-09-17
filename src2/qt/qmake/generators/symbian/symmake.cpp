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

#include "symmake.h"

#include <qstring.h>
#include <qhash.h>
#include <qstringlist.h>
#include <qdir.h>
#include <qdatetime.h>
#include <stdlib.h>
#include <qdebug.h>

// Included from tools/shared
#include <symbian/epocroot_p.h>

#define RESOURCE_DIRECTORY_MMP "/resource/apps"
#define REGISTRATION_RESOURCE_DIRECTORY_HW "/private/10003a3f/import/apps"
#define PLUGIN_COMMON_DEF_FILE_FOR_MMP "./plugin_common.def"
#define BLD_INF_FILENAME_LEN (sizeof(BLD_INF_FILENAME) - 1)

#define BLD_INF_RULES_BASE "BLD_INF_RULES."
#define BLD_INF_TAG_PLATFORMS "prj_platforms"
#define BLD_INF_TAG_MMPFILES "prj_mmpfiles"
#define BLD_INF_TAG_TESTMMPFILES "prj_testmmpfiles"
#define BLD_INF_TAG_EXTENSIONS "prj_extensions"
#define BLD_INF_TAG_TESTEXTENSIONS "prj_testextensions"

#define MMP_TARGET "TARGET"
#define MMP_TARGETTYPE "TARGETTYPE"
#define MMP_SECUREID "SECUREID"
#define MMP_OPTION "OPTION"
#define MMP_LINKEROPTION "LINKEROPTION"
#define MMP_CAPABILITY "CAPABILITY"
#define MMP_EPOCALLOWDLLDATA "EPOCALLOWDLLDATA"
#define MMP_EPOCHEAPSIZE "EPOCHEAPSIZE"
#define MMP_EPOCSTACKSIZE "EPOCSTACKSIZE"
#define MMP_UID "UID"
#define MMP_VENDORID "VENDORID"
#define MMP_VERSION "VERSION"
#define MMP_START_RESOURCE "START RESOURCE"
#define MMP_END_RESOURCE "END"

#define VAR_CXXFLAGS "QMAKE_CXXFLAGS"
#define VAR_CFLAGS "QMAKE_CFLAGS"
#define VAR_LFLAGS "QMAKE_LFLAGS"

#define DEFINE_REPLACE_REGEXP "[^A-Z0-9_]"

QString SymbianMakefileGenerator::fixPathForMmp(const QString& origPath, const QDir& parentDir)
{
    static QString epocRootStr;
    if (epocRootStr.isEmpty()) {
        epocRootStr = qt_epocRoot();
        QFileInfo efi(epocRootStr);
        if (!efi.exists() || epocRootStr.isEmpty()) {
            fprintf(stderr, "Unable to resolve epocRoot '%s' to real dir on current drive, defaulting to '/' for mmp paths\n", qPrintable(qt_epocRoot()));
            epocRootStr = "/";
        } else {
            epocRootStr = efi.absoluteFilePath();
        }
        if (!epocRootStr.endsWith("/"))
            epocRootStr += "/";

        epocRootStr += "epoc32/";
    }

    QString resultPath = origPath;

    // Make it relative, unless it starts with "%epocroot%/epoc32/"
    if (resultPath.startsWith(epocRootStr, Qt::CaseInsensitive)) {
        resultPath.replace(epocRootStr, "/epoc32/", Qt::CaseInsensitive);
    } else {
        resultPath = parentDir.relativeFilePath(resultPath);
    }
    resultPath = QDir::cleanPath(resultPath);

    if (resultPath.isEmpty())
        resultPath = ".";

    return resultPath;
}

QString SymbianMakefileGenerator::absolutizePath(const QString& origPath)
{
    // Prepend epocroot to any paths beginning with "/epoc32/"
    QString resultPath = QDir::fromNativeSeparators(origPath);
    if (resultPath.startsWith("/epoc32/", Qt::CaseInsensitive))
        resultPath = QDir::fromNativeSeparators(qt_epocRoot()) + resultPath.mid(1);

    QFileInfo fi(outputDir, resultPath);
    // Since origPath can be something given in HEADERS, we need to check if we are dealing
    // with a file or a directory. In case the origPath doesn't yet exist, isFile() returns
    // false and we default to assuming it is a dir.
    if (fi.isFile()) {
        resultPath = fi.absolutePath();
    } else {
        resultPath = fi.absoluteFilePath();
    }

    resultPath = QDir::cleanPath(resultPath);

    return resultPath;
}

SymbianMakefileGenerator::SymbianMakefileGenerator() : MakefileGenerator(), SymbianCommonGenerator(this) { }
SymbianMakefileGenerator::~SymbianMakefileGenerator() { }

void SymbianMakefileGenerator::writeHeader(QTextStream &t)
{
    t << "// ============================================================================" << endl;
    t << "// * Makefile for building: " << escapeFilePath(var("TARGET")) << endl;
    t << "// * Generated by qmake (" << qmake_version() << ") (Qt " QT_VERSION_STR ") on: ";
    t << QDateTime::currentDateTime().toString(Qt::ISODate) << endl;
    t << "// * This file is generated by qmake and should not be modified by the" << endl;
    t << "// * user." << endl;
    t << "// * Project:  " << fileFixify(project->projectFile()) << endl;
    t << "// * Template: " << var("TEMPLATE") << endl;
    t << "// ============================================================================" << endl;
    t << endl;

    // Defining define for bld.inf

    QString shortProFilename = project->projectFile();
    shortProFilename.replace(0, shortProFilename.lastIndexOf("/") + 1, QString(""));
    shortProFilename.replace(Option::pro_ext, QString(""));

    QString bldinfDefine = shortProFilename;
    bldinfDefine.append("_");
    bldinfDefine.append(generate_uid(project->projectFile()));
    bldinfDefine = bldinfDefine.toUpper();

    // replace anything not alphanumeric with underscore
    QRegExp replacementMask(DEFINE_REPLACE_REGEXP);
    bldinfDefine.replace(replacementMask, QLatin1String("_"));

    bldinfDefine.prepend("BLD_INF_");

    t << "#define " << bldinfDefine << endl << endl;
}

bool SymbianMakefileGenerator::writeMakefile(QTextStream &t)
{
    if(!project->values("QMAKE_FAILED_REQUIREMENTS").isEmpty()) {
        fprintf(stderr, "Project files not generated because all requirements are not met:\n\t%s\n",
                qPrintable(var("QMAKE_FAILED_REQUIREMENTS")));
        return false;
    }

    writeHeader(t);

    QString numberOfIcons;
    QString iconFile;
    QMap<QString, QStringList> userRssRules;
    readRssRules(numberOfIcons, iconFile, userRssRules);

    SymbianLocalizationList symbianLocalizationList;
    parseTsFiles(&symbianLocalizationList);

    // Generate pkg files if there are any actual files to deploy
    bool generatePkg = false;

    if (targetType == TypeExe) {
        generatePkg = true;
    } else {
        foreach(QString item, project->values("DEPLOYMENT")) {
            // ### Qt 5: remove .sources, inconsistent with INSTALLS
            if (!project->values(item + ".sources").isEmpty() ||
                !project->values(item + ".files").isEmpty()) {
                generatePkg = true;
                break;
            }
        }
    }

    if (generatePkg) {
        generatePkgFile(iconFile, true, symbianLocalizationList);
    }

    writeBldInfContent(t, generatePkg, iconFile);

    // Generate empty wrapper makefile here, because wrapper makefile must exist before writeMkFile,
    // but all required data is not yet available.
    bool isPrimaryMakefile = true;
    QString wrapperFileName = Option::output_dir + QLatin1Char('/') + QLatin1String("Makefile");
    QString outputFileName = fileInfo(Option::output.fileName()).fileName();
    if (outputFileName != BLD_INF_FILENAME) {
        wrapperFileName.append(".").append(outputFileName.startsWith(BLD_INF_FILENAME)
                                           ? outputFileName.mid(sizeof(BLD_INF_FILENAME))
                                           : outputFileName);
        isPrimaryMakefile = false;
    }

    QFile wrapperMakefile(wrapperFileName);
    if (wrapperMakefile.open(QIODevice::WriteOnly)) {
        generatedFiles << wrapperFileName;
    } else {
        PRINT_FILE_CREATE_ERROR(wrapperFileName);
        return false;
    }

    if (targetType == TypeSubdirs) {
        // If we have something to deploy, generate extension makefile for just that, since
        // normal extension makefile is not getting generated and we need emulator deployment to be done.
        if (generatePkg)
            writeMkFile(wrapperFileName, true);
        writeWrapperMakefile(wrapperMakefile, isPrimaryMakefile);
        return true;
    }

    writeMkFile(wrapperFileName, false);

    QString absoluteMmpFileName = Option::output_dir + QLatin1Char('/') + mmpFileName;
    writeMmpFile(absoluteMmpFileName, symbianLocalizationList);

    if (targetType == TypeExe) {
        if (!project->isActiveConfig("no_icon")) {
            writeRegRssFile(userRssRules);
            writeRssFile(numberOfIcons, iconFile);
            writeLocFile(symbianLocalizationList);
        }
    }

    writeCustomDefFile();
    writeWrapperMakefile(wrapperMakefile, isPrimaryMakefile);

    return true;
}

void SymbianMakefileGenerator::init()
{
    MakefileGenerator::init();
    SymbianCommonGenerator::init();

    outputDir = QDir(Option::output_dir);

    if (0 != project->values("QMAKE_PLATFORM").size())
        platform = varGlue("QMAKE_PLATFORM", "", " ", "");

    if (0 == project->values("QMAKESPEC").size())
        project->values("QMAKESPEC").append(qgetenv("QMAKESPEC"));

    project->values("QMAKE_LIBS") += escapeFilePaths(project->values("LIBS"));
    project->values("QMAKE_LIBS_PRIVATE") += escapeFilePaths(project->values("LIBS_PRIVATE"));

    // Disallow renaming of bld.inf.
    project->values("MAKEFILE").clear();
    project->values("MAKEFILE") += BLD_INF_FILENAME;

    // .mmp
    mmpFileName = fixedTarget;
    if (targetType == TypeExe)
        mmpFileName.append("_exe");
    else if (targetType == TypeDll || targetType == TypePlugin)
        mmpFileName.append("_dll");
    else if (targetType == TypeLib)
        mmpFileName.append("_lib");
    mmpFileName.append(Option::mmp_ext);

    initMmpVariables();

    uid2 = project->first("TARGET.UID2");

    uid2 = uid2.trimmed();
}

QString SymbianMakefileGenerator::getTargetExtension()
{
    QString ret;
    if (targetType == TypeExe) {
        ret.append("exe");
    } else if (targetType == TypeLib) {
        ret.append("lib");
    } else if (targetType == TypeDll || targetType == TypePlugin) {
        ret.append("dll");
    } else if (targetType == TypeSubdirs) {
        // Not actually usable, so return empty
    } else {
        // If nothing else set, default to exe
        ret.append("exe");
    }

    return ret;
}

QString SymbianMakefileGenerator::generateUID3()
{
    QString target = project->first("TARGET");
    QString currPath = qmake_getpwd();
    target.prepend("/").prepend(currPath);
    return generate_test_uid(target);
}

void SymbianMakefileGenerator::initMmpVariables()
{
    QStringList sysincspaths;
    QStringList srcincpaths;
    QStringList srcpaths;

    srcpaths << project->values("SOURCES") << project->values("GENERATED_SOURCES");
    srcpaths << project->values("UNUSED_SOURCES") << project->values("UI_SOURCES_DIR");
    srcpaths << project->values("UI_DIR");

    QString absolutizedCurrent = absolutizePath(".");

    for (int j = 0; j < srcpaths.size(); ++j) {
        QFileInfo fi(fileInfo(srcpaths.at(j)));
        // Sometimes sources have other than *.c* files (e.g. *.moc); prune them.
        if (fi.suffix().startsWith("c")) {
            if (fi.filePath().length() > fi.fileName().length()) {
                appendIfnotExist(srcincpaths, fi.path());
                sources[absolutizePath(fi.path())] += fi.fileName();
            } else {
                sources[absolutizedCurrent] += fi.fileName();
                appendIfnotExist(srcincpaths, absolutizedCurrent);
            }
        }
    }

    QStringList incpaths;
    incpaths << project->values("INCLUDEPATH");
    incpaths << QLibraryInfo::location(QLibraryInfo::HeadersPath);
    incpaths << project->values("HEADERS");
    incpaths << srcincpaths;
    incpaths << project->values("UI_HEADERS_DIR");
    incpaths << project->values("UI_DIR");

    for (int j = 0; j < incpaths.size(); ++j) {
        QString includepath = absolutizePath(incpaths.at(j));
        appendIfnotExist(sysincspaths, includepath);
        appendAbldTempDirs(sysincspaths, includepath);
    }

    // Remove duplicate include path entries
    QStringList temporary;
    for (int i = 0; i < sysincspaths.size(); ++i) {
        QString origPath = sysincspaths.at(i);
        QFileInfo origPathInfo(outputDir, origPath);
        bool bFound = false;

        for (int j = 0; j < temporary.size(); ++j) {
            QString tmpPath = temporary.at(j);
            QFileInfo tmpPathInfo(outputDir, tmpPath);

            if (origPathInfo.absoluteFilePath() == tmpPathInfo.absoluteFilePath()) {
                bFound = true;
                if (!tmpPathInfo.isRelative() && origPathInfo.isRelative()) {
                    // We keep the relative notation
                    temporary.removeOne(tmpPath);
                    temporary << origPath;
                }
            }
        }

        if (!bFound)
            temporary << origPath;

    }

    sysincspaths.clear();
    sysincspaths << temporary;

    systeminclude.insert("SYSTEMINCLUDE", sysincspaths);

    // Check MMP_RULES for singleton keywords that are overridden
    QStringList overridableMmpKeywords;
    QStringList restrictableMmpKeywords;
    QStringList restrictedMmpKeywords;
    bool inResourceBlock = false;

    overridableMmpKeywords << QLatin1String(MMP_TARGETTYPE) << QLatin1String(MMP_EPOCHEAPSIZE);
    restrictableMmpKeywords << QLatin1String(MMP_TARGET) << QLatin1String(MMP_SECUREID)
       << QLatin1String(MMP_OPTION) << QLatin1String(MMP_LINKEROPTION)
       << QLatin1String(MMP_CAPABILITY) << QLatin1String(MMP_EPOCALLOWDLLDATA)
       << QLatin1String(MMP_EPOCSTACKSIZE) << QLatin1String(MMP_UID)
       << QLatin1String(MMP_VENDORID) << QLatin1String(MMP_VERSION);

    foreach (QString item, project->values("MMP_RULES")) {
        if (project->values(item).isEmpty()) {
            handleMmpRulesOverrides(item, inResourceBlock, restrictedMmpKeywords,
                                    restrictableMmpKeywords, overridableMmpKeywords);
        } else {
            foreach (QString itemRow, project->values(item)) {
                handleMmpRulesOverrides(itemRow, inResourceBlock, restrictedMmpKeywords,
                                        restrictableMmpKeywords, overridableMmpKeywords);
            }
        }
    }

    if (restrictedMmpKeywords.size()) {
        fprintf(stderr, "Warning: Restricted statements detected in MMP_RULES:\n"
                "         (%s)\n"
                "         Use corresponding qmake variable(s) instead.\n",
                qPrintable(restrictedMmpKeywords.join(", ")));
        }
}

void SymbianMakefileGenerator::handleMmpRulesOverrides(QString &checkString,
                                                       bool &inResourceBlock,
                                                       QStringList &restrictedMmpKeywords,
                                                       const QStringList &restrictableMmpKeywords,
                                                       const QStringList &overridableMmpKeywords)
{
    QString simplifiedString = checkString.simplified();

    if (!inResourceBlock && simplifiedString.startsWith(MMP_START_RESOURCE, Qt::CaseInsensitive))
        inResourceBlock = true;
    else if (inResourceBlock && simplifiedString.startsWith(MMP_END_RESOURCE, Qt::CaseInsensitive))
        inResourceBlock = false;

    // Allow restricted and overridable items in RESOURCE blocks as those do not actually
    // override anything.
    if (!inResourceBlock) {
        appendKeywordIfMatchFound(overriddenMmpKeywords, overridableMmpKeywords, simplifiedString);
        appendKeywordIfMatchFound(restrictedMmpKeywords, restrictableMmpKeywords, simplifiedString);
    }
}

void SymbianMakefileGenerator::appendKeywordIfMatchFound(QStringList &list,
                                                         const QStringList &keywordList,
                                                         QString &checkString)
{
    // Check if checkString starts with any supplied keyword and
    // add the found keyword to list if it does.
    foreach (QString item, keywordList) {
        if (checkString.startsWith(QString(item).append(" "), Qt::CaseInsensitive)
            || checkString.compare(item, Qt::CaseInsensitive) == 0) {
            appendIfnotExist(list, item);
        }
    }
}


bool SymbianMakefileGenerator::removeDuplicatedStrings(QStringList &stringList)
{
    QStringList tmpStringList;

    for (int i = 0; i < stringList.size(); ++i) {
        QString string = stringList.at(i);
        if (tmpStringList.contains(string))
            continue;
        else
            tmpStringList.append(string);
    }

    stringList.clear();
    stringList = tmpStringList;
    return true;
}

void SymbianMakefileGenerator::writeMmpFileHeader(QTextStream &t)
{
    t << "// ==============================================================================" << endl;
    t << "// Generated by qmake (" << qmake_version() << ") (Qt " QT_VERSION_STR ") on: ";
    t << QDateTime::currentDateTime().toString(Qt::ISODate) << endl;
    t << "// This file is generated by qmake and should not be modified by the" << endl;
    t << "// user." << endl;
    t << "//  Name        : " << mmpFileName << endl;
    t << "// ==============================================================================" << endl << endl;
}

void SymbianMakefileGenerator::writeMmpFile(QString &filename, const SymbianLocalizationList &symbianLocalizationList)
{
    QFile ft(filename);
    if (ft.open(QIODevice::WriteOnly)) {
        generatedFiles << ft.fileName();

        QTextStream t(&ft);

        writeMmpFileHeader(t);

        writeMmpFileTargetPart(t);

        writeMmpFileResourcePart(t, symbianLocalizationList);

        writeMmpFileMacrosPart(t);

        writeMmpFileIncludePart(t);

        for (QMap<QString, QStringList>::iterator it = sources.begin(); it != sources.end(); ++it) {
            QStringList values = it.value();
            QString currentSourcePath = it.key();

            if (values.size())
                t << "SOURCEPATH \t" <<  fixPathForMmp(currentSourcePath, Option::output_dir) << endl;

            for (int i = 0; i < values.size(); ++i) {
                QString sourceFileName = values.at(i);
                t << "SOURCE\t\t" << sourceFileName << endl;
            }
            t << endl;
        }
        t << endl;

        if (!project->isActiveConfig("static") && !project->isActiveConfig("staticlib")) {
            writeMmpFileLibraryPart(t);
        }

        writeMmpFileCapabilityPart(t);

        writeMmpFileCompilerOptionPart(t);

        writeMmpFileBinaryVersionPart(t);

        writeMmpFileRulesPart(t);
    } else {
        PRINT_FILE_CREATE_ERROR(filename)
    }
}

void SymbianMakefileGenerator::writeMmpFileMacrosPart(QTextStream& t)
{
    t << endl;

    QStringList &defines = project->values("DEFINES");
    if (defines.size())
        t << "// Qt Macros" << endl;
    for (int i = 0; i < defines.size(); ++i) {
        QString def = defines.at(i);
        addMacro(t, def);
    }

    // These are required in order that all methods will be correctly exported e.g from qtestlib
    QStringList &exp_defines = project->values("PRL_EXPORT_DEFINES");
    if (exp_defines.size())
        t << endl << "// Qt Export Defines" << endl;
    for (int i = 0; i < exp_defines.size(); ++i) {
        QString def = exp_defines.at(i);
        addMacro(t, def);
    }

    t << endl;
}

void SymbianMakefileGenerator::addMacro(QTextStream& t, const QString& value)
{
    // String macros for Makefile based platforms are defined like this in pro files:
    //
    //   DEFINES += VERSION_STRING=\\\"1.2.3\\\"
    //
    // This will not work in *.mmp files, which don't need double escaping, and
    // will therefore result in a VERSION_STRING value of \"1.2.3\" instead of "1.2.3".
    // Improve cross platform support by removing one level of escaping from all
    // DEFINES values.
    static QChar backslash = QLatin1Char('\\');
    QString fixedValue;
    fixedValue.reserve(value.size());
    int pos = 0;
    int prevPos = 0;
    while (pos < value.size()) {
        if (value.at(pos) == backslash) {
            fixedValue += value.mid(prevPos, pos - prevPos);
            pos++;
            prevPos = pos;
        }
        pos++;
    }
    fixedValue += value.mid(prevPos);

    t << "MACRO\t\t" << fixedValue << endl;
}


void SymbianMakefileGenerator::writeMmpFileTargetPart(QTextStream& t)
{
    bool skipTargetType = overriddenMmpKeywords.contains(MMP_TARGETTYPE);
    bool skipEpocHeapSize = overriddenMmpKeywords.contains(MMP_EPOCHEAPSIZE);

    if (targetType == TypeExe) {
        t << MMP_TARGET "\t\t" << fixedTarget << ".exe" << endl;
        if (!skipTargetType) {
            if (project->isActiveConfig("stdbinary"))
                t << MMP_TARGETTYPE "\t\tSTDEXE" << endl;
            else
                t << MMP_TARGETTYPE "\t\tEXE" << endl;
        }
    } else if (targetType == TypeDll || targetType == TypePlugin) {
        t << MMP_TARGET "\t\t" << fixedTarget << ".dll" << endl;
        if (!skipTargetType) {
            if (project->isActiveConfig("stdbinary"))
                t << MMP_TARGETTYPE "\t\tSTDDLL" << endl;
            else
                t << MMP_TARGETTYPE "\t\tDLL" << endl;
        }
    } else if (targetType == TypeLib) {
        t << MMP_TARGET "\t\t" << fixedTarget << ".lib" << endl;
        if (!skipTargetType) {
            if (project->isActiveConfig("stdbinary"))
                t << MMP_TARGETTYPE "\t\tSTDLIB" << endl;
            else
                t << MMP_TARGETTYPE "\t\tLIB" << endl;
        }
    } else {
        fprintf(stderr, "Error: Unexpected targettype (%d) in SymbianMakefileGenerator::writeMmpFileTargetPart\n", targetType);
    }

    t << endl;

    t << MMP_UID "\t\t" << uid2 << " " << uid3 << endl;

    if (0 != project->values("TARGET.SID").size()) {
        t << MMP_SECUREID "\t\t" << project->values("TARGET.SID").join(" ") << endl;
    } else {
        if (0 == uid3.size())
            t << MMP_SECUREID "\t\t0" << endl;
        else
            t << MMP_SECUREID "\t\t" << uid3 << endl;
    }

    // default value used from mkspecs is 0
    if (0 != project->values("TARGET.VID").size()) {
        t << MMP_VENDORID "\t\t" << project->values("TARGET.VID").join(" ") << endl;
    }

    t << endl;

    if (0 != project->first("TARGET.EPOCSTACKSIZE").size())
        t << MMP_EPOCSTACKSIZE "\t\t" << project->first("TARGET.EPOCSTACKSIZE") << endl;
    if (!skipEpocHeapSize && 0 != project->values("TARGET.EPOCHEAPSIZE").size())
        t << MMP_EPOCHEAPSIZE "\t\t" << project->values("TARGET.EPOCHEAPSIZE").join(" ") << endl;
    if (0 != project->values("TARGET.EPOCALLOWDLLDATA").size())
        t << MMP_EPOCALLOWDLLDATA << endl;

    if (targetType == TypePlugin && !project->isActiveConfig("stdbinary")) {
        // Use custom def file for Qt plugins
        t << "DEFFILE " PLUGIN_COMMON_DEF_FILE_FOR_MMP << endl;
    }

    t << endl;
}


/*
    Application registration resource files should be installed to the
    \private\10003a3f\import\apps directory.
*/
void SymbianMakefileGenerator::writeMmpFileResourcePart(QTextStream& t, const SymbianLocalizationList &symbianLocalizationList)
{
    if ((targetType == TypeExe) &&
            !project->isActiveConfig("no_icon")) {

        QString locTarget = fixedTarget;
        locTarget.append(".rss");

        t << "SOURCEPATH\t\t\t. " << endl;
        t << MMP_START_RESOURCE "\t\t" << locTarget << endl;
        t << "LANG SC ";    // no endl
        SymbianLocalizationListIterator iter(symbianLocalizationList);
        while (iter.hasNext()) {
            const SymbianLocalization &loc = iter.next();
            t << loc.symbianLanguageCode << " "; // no endl
        }
        t << endl;
        t << "HEADER" << endl;
        t << "TARGETPATH\t\t\t" RESOURCE_DIRECTORY_MMP << endl;
        t << MMP_END_RESOURCE << endl << endl;

        QString regTarget = fixedTarget;
        regTarget.append("_reg.rss");

        t << "SOURCEPATH\t\t\t." << endl;
        t << MMP_START_RESOURCE "\t\t" << regTarget << endl;
        if (isForSymbianSbsv2())
            t << "DEPENDS " << fixedTarget << ".rsg" << endl;
        t << "TARGETPATH\t\t" REGISTRATION_RESOURCE_DIRECTORY_HW << endl;
        t << MMP_END_RESOURCE << endl << endl;
    }
}

void SymbianMakefileGenerator::writeMmpFileSystemIncludePart(QTextStream& t)
{
    for (QMap<QString, QStringList>::iterator it = systeminclude.begin(); it != systeminclude.end(); ++it) {
        QStringList values = it.value();
        for (int i = 0; i < values.size(); ++i) {
            QString handledPath = values.at(i);
            t << "SYSTEMINCLUDE\t\t" << fixPathForMmp(handledPath, Option::output_dir) << endl;
        }
    }

    t << endl;
}

void SymbianMakefileGenerator::writeMmpFileIncludePart(QTextStream& t)
{
    writeMmpFileSystemIncludePart(t);
}

void SymbianMakefileGenerator::writeMmpFileLibraryPart(QTextStream& t)
{
    QStringList &libs = project->values("LIBS");
    libs << project->values("QMAKE_LIBS") << project->values("QMAKE_LIBS_PRIVATE");

    removeDuplicatedStrings(libs);

    for (int i = 0; i < libs.size(); ++i) {
        QString lib = libs.at(i);
        // The -L flag is uninteresting, since all symbian libraries exist in the same directory.
        if (lib.startsWith("-l")) {
            lib.remove(0, 2);
            QString mmpStatement;
            if (lib.endsWith(".lib")) {
                lib.chop(4);
                mmpStatement = "STATICLIBRARY\t";
            } else {
                if (lib.endsWith(".dll"))
                    lib.chop(4);
                mmpStatement = "LIBRARY\t\t";
            }
            t << mmpStatement <<  lib << ".lib" << endl;
        }
    }

    t << endl;
}

void SymbianMakefileGenerator::writeMmpFileCapabilityPart(QTextStream& t)
{
    if (0 != project->first("TARGET.CAPABILITY").size()) {
        QStringList &capabilities = project->values("TARGET.CAPABILITY");
        t << MMP_CAPABILITY "\t\t";

        for (int i = 0; i < capabilities.size(); ++i) {
            QString cap = capabilities.at(i);
            t << cap << " ";
        }
    } else {
        t << MMP_CAPABILITY "\t\tNone";
    }
    t << endl << endl;
}

void SymbianMakefileGenerator::writeMmpFileConditionalOptions(QTextStream& t,
                                                              const QString &optionType,
                                                              const QString &optionTag,
                                                              const QString &variableBase)
{
    foreach(QString compilerVersion, project->values("VERSION_FLAGS." + optionTag)) {
        QStringList currentValues = project->values(variableBase + "." + compilerVersion);
        if (currentValues.size()) {
            t << "#if defined(" << compilerVersion << ")" << endl;
            t << optionType << " " << optionTag << " " << currentValues.join(" ") <<  endl;
            t << "#endif" << endl;
        }
    }
}

void SymbianMakefileGenerator::writeMmpFileSimpleOption(QTextStream& t,
                                                        const QString &optionType,
                                                        const QString &optionTag,
                                                        const QString &options)
{
    QString trimmedOptions = options.trimmed();
    if (!trimmedOptions.isEmpty())
        t << optionType << " " << optionTag << " " << trimmedOptions << endl;
}

void SymbianMakefileGenerator::appendMmpFileOptions(QString &options, const QStringList &list)
{
    if (list.size()) {
        options.append(list.join(" "));
        options.append(" ");
    }
}

void SymbianMakefileGenerator::writeMmpFileCompilerOptionPart(QTextStream& t)
{
    QStringList keywords =  project->values("MMP_OPTION_KEYWORDS");
    QStringList commonCxxFlags = project->values(VAR_CXXFLAGS);
    QStringList commonCFlags = project->values(VAR_CFLAGS);
    QStringList commonLFlags = project->values(VAR_LFLAGS);

    foreach(QString item, keywords) {
        QString compilerOption;
        QString linkerOption;

        appendMmpFileOptions(compilerOption, project->values(VAR_CXXFLAGS "." + item));
        appendMmpFileOptions(compilerOption, project->values(VAR_CFLAGS "." + item));
        appendMmpFileOptions(compilerOption, commonCxxFlags);
        appendMmpFileOptions(compilerOption, commonCFlags);

        appendMmpFileOptions(linkerOption, project->values(VAR_LFLAGS "."  + item));
        appendMmpFileOptions(linkerOption, commonLFlags);

        writeMmpFileSimpleOption(t, MMP_OPTION, item, compilerOption);
        writeMmpFileSimpleOption(t, MMP_LINKEROPTION, item, linkerOption);

        writeMmpFileConditionalOptions(t, MMP_OPTION, item, VAR_CXXFLAGS);
        writeMmpFileConditionalOptions(t, MMP_LINKEROPTION, item, VAR_LFLAGS);
    }

    t << endl;
}

void SymbianMakefileGenerator::writeMmpFileBinaryVersionPart(QTextStream& t)
{
    QString applicationVersion = project->first("VERSION");
    QStringList verNumList = applicationVersion.split('.');
    uint major = 0;
    uint minor = 0;
    uint patch = 0;
    bool success = false;

    if (verNumList.size() > 0) {
        major = verNumList[0].toUInt(&success);
        if (success && verNumList.size() > 1) {
            minor = verNumList[1].toUInt(&success);
            if (success && verNumList.size() > 2) {
                patch = verNumList[2].toUInt(&success);
            }
        }
    }

    QString mmpVersion;
    if (success && major <= 0xFFFF && minor <= 0xFF && patch <= 0xFF) {
        // Symbian binary version only has major and minor components, so compress
        // Qt's minor and patch values into the minor component. Since Symbian's minor
        // component is a 16 bit value, only allow 8 bits for each to avoid overflow.
        mmpVersion.append(QString::number(major))
            .append('.')
            .append(QString::number((minor << 8) + patch));
    } else {
        if (!applicationVersion.isEmpty())
            fprintf(stderr, "Invalid VERSION string: %s\n", qPrintable(applicationVersion));
        mmpVersion = "10.0"; // Default binary version for symbian is 10.0
    }

    t << MMP_VERSION " " << mmpVersion  << endl;
}

void SymbianMakefileGenerator::writeMmpFileRulesPart(QTextStream& t)
{
    foreach(QString item, project->values("MMP_RULES")) {
        t << endl;
        // If there is no stringlist defined for a rule, use rule name directly
        // This is convenience for defining single line mmp statements
        if (project->values(item).isEmpty()) {
            t << item << endl;
        } else {
            foreach(QString itemRow, project->values(item)) {
                t << itemRow << endl;
            }
        }
    }
}

void SymbianMakefileGenerator::writeBldInfContent(QTextStream &t, bool addDeploymentExtension, const QString &iconFile)
{
    // Read user defined bld inf rules

    QMap<QString, QStringList> userBldInfRules;
    for (QMap<QString, QStringList>::iterator it = project->variables().begin(); it != project->variables().end(); ++it) {
        if (it.key().startsWith(BLD_INF_RULES_BASE)) {
            QString newKey = it.key().mid(sizeof(BLD_INF_RULES_BASE) - 1);
            if (newKey.isEmpty()) {
                fprintf(stderr, "Warning: Empty BLD_INF_RULES key encountered\n");
                continue;
            }
            QStringList newValues;
            QStringList values = it.value();
            foreach(QString item, values) {
                // If there is no stringlist defined for a rule, use rule name directly
                // This is convenience for defining single line statements
                if (project->values(item).isEmpty()) {
                    newValues << item;
                } else {
                    foreach(QString itemRow, project->values(item)) {
                        newValues << itemRow;
                    }
                }
            }
            userBldInfRules.insert(newKey, newValues);
        }
    }

    // Add includes of subdirs bld.inf files

    QString currentPath = qmake_getpwd();
    QDir directory(currentPath);

    const QStringList &subdirs = project->values("SUBDIRS");
    foreach(QString item, subdirs) {
        bool fromFile = false;
        QString fixedItem;
        if (!project->isEmpty(item + ".file")) {
            fixedItem = project->first(item + ".file");
            fromFile = true;
        } else if (!project->isEmpty(item + ".subdir")) {
            fixedItem = project->first(item + ".subdir");
            fromFile = false;
        } else {
            fixedItem = item;
            fromFile = item.endsWith(Option::pro_ext);
        }

        QString condition;
        if (!project->isEmpty(item + ".condition"))
            condition = project->first(item + ".condition");

        QFileInfo subdir(fileInfo(fixedItem));
        QString relativePath = directory.relativeFilePath(fixedItem);
        QString fullProName = subdir.absoluteFilePath();
        QString bldinfFilename;
        QString subdirFileName;

        if (fromFile) {
            subdirFileName = subdir.completeBaseName();
        } else {
            subdirFileName = subdir.fileName();
        }

        if (subdir.isDir()) {
            // Subdir is a regular project
            bldinfFilename = relativePath + QString("/") + QString(BLD_INF_FILENAME);
            fullProName += QString("/") + subdirFileName + Option::pro_ext;
        } else {
            // Subdir is actually a .pro file
            if (relativePath.contains("/")) {
                // .pro not in same directory as parent .pro
                relativePath.remove(relativePath.lastIndexOf("/") + 1, relativePath.length());
                bldinfFilename = relativePath;
            } else {
                // .pro and parent .pro in same directory
                bldinfFilename = QString("./");
            }
            bldinfFilename += QString(BLD_INF_FILENAME ".") + subdirFileName;
        }

        QString uid = generate_uid(fullProName);
        QString bldinfDefine = QString("BLD_INF_") + subdirFileName + QString("_") + uid;
        bldinfDefine = bldinfDefine.toUpper();

        // replace anything not alphanumeric with underscore
        QRegExp replacementMask(DEFINE_REPLACE_REGEXP);
        bldinfDefine.replace(replacementMask, QLatin1String("_"));

        if (!condition.isEmpty())
            t << "#if defined(" << condition << ")" << endl;

        t << "#ifndef " << bldinfDefine << endl;
        t << "\t#include \"" << bldinfFilename << "\"" << endl;
        t << "#endif" << endl;

        if (!condition.isEmpty())
            t << "#endif" << endl;

    }

    // Add supported project platforms

    t << endl << BLD_INF_TAG_PLATFORMS << endl << endl;
    if (0 != project->values("SYMBIAN_PLATFORMS").size())
        t << project->values("SYMBIAN_PLATFORMS").join(" ") << endl;

    QStringList userItems = userBldInfRules.value(BLD_INF_TAG_PLATFORMS);
    foreach(QString item, userItems)
        t << item << endl;
    userBldInfRules.remove(BLD_INF_TAG_PLATFORMS);
    t << endl;

    // Add project mmps and old style extension makefiles

    QString mmpTag;
    if (project->isActiveConfig(SYMBIAN_TEST_CONFIG))
        mmpTag = QLatin1String(BLD_INF_TAG_TESTMMPFILES);
    else
        mmpTag = QLatin1String(BLD_INF_TAG_MMPFILES);

    t << endl << mmpTag << endl << endl;

    writeBldInfMkFilePart(t, addDeploymentExtension);
    if (targetType != TypeSubdirs)
        t << mmpFileName << endl;

    userItems = userBldInfRules.value(mmpTag);
    foreach(QString item, userItems)
        t << item << endl;
    userBldInfRules.remove(mmpTag);

    QString extensionTag;
    if (project->isActiveConfig(SYMBIAN_TEST_CONFIG))
        extensionTag = QLatin1String(BLD_INF_TAG_TESTEXTENSIONS);
    else
        extensionTag = QLatin1String(BLD_INF_TAG_EXTENSIONS);

    t << endl << extensionTag << endl << endl;

    // Generate extension rules

    writeBldInfExtensionRulesPart(t, iconFile);

    userItems = userBldInfRules.value(extensionTag);
    foreach(QString item, userItems)
        t << item << endl;
    userBldInfRules.remove(extensionTag);

    // Add rest of the user defined content

    for (QMap<QString, QStringList>::iterator it = userBldInfRules.begin(); it != userBldInfRules.end(); ++it) {
        t << endl << endl << it.key() << endl << endl;
        userItems = it.value();
        foreach(QString item, userItems)
            t << item << endl;
    }
}

void SymbianMakefileGenerator::appendIfnotExist(QStringList &list, QString value)
{
    if (!list.contains(value))
        list += value;
}

void SymbianMakefileGenerator::appendIfnotExist(QStringList &list, QStringList values)
{
    foreach(QString item, values)
        appendIfnotExist(list, item);
}


QString SymbianMakefileGenerator::removeTrailingPathSeparators(QString &file)
{
    QString ret = file;
    if (ret.endsWith(QDir::separator())) {
        ret.remove(ret.length() - 1, 1);
    }

    return ret;
}

void SymbianMakefileGenerator::generateCleanCommands(QTextStream& t,
        const QStringList& toClean,
        const QString& cmd,
        const QString& cmdOptions,
        const QString& itemPrefix,
        const QString& itemSuffix)
{
    for (int i = 0; i < toClean.size(); ++i) {
        QString item = toClean.at(i);
        item.prepend(itemPrefix).append(itemSuffix);
#if defined(Q_OS_WIN)
        t << "\t-@ if EXIST \"" << QDir::toNativeSeparators(item) << "\" ";
        t << cmd << " " << cmdOptions << " \"" << QDir::toNativeSeparators(item) << "\"" << endl;
#else
        t << "\t-if test -e " << QDir::toNativeSeparators(item) << "; then ";
        t << cmd << " " << cmdOptions << " " << QDir::toNativeSeparators(item) << "; fi" << endl;
#endif
    }
}

void SymbianMakefileGenerator::generateDistcleanTargets(QTextStream& t)
{
    t << "dodistclean:" << endl;
    const QStringList &subdirs = project->values("SUBDIRS");
    foreach(QString item, subdirs) {
        bool fromFile = false;
        QString fixedItem;
        if (!project->isEmpty(item + ".file")) {
            fixedItem = project->first(item + ".file");
            fromFile = true;
        } else if (!project->isEmpty(item + ".subdir")) {
            fixedItem = project->first(item + ".subdir");
            fromFile = false;
        } else {
            fromFile = item.endsWith(Option::pro_ext);
            fixedItem = item;
        }
        QFileInfo fi(outputDir, fixedItem);
        if (!fromFile) {
            t << "\t-$(MAKE) -f \"" << Option::fixPathToTargetOS(fi.absoluteFilePath() + "/Makefile") << "\" dodistclean" << endl;
        } else {
            QString itemName = fi.fileName();
            int extIndex = itemName.lastIndexOf(Option::pro_ext);
            if (extIndex)
                fixedItem = fi.absolutePath() + "/" + QString("Makefile.") + itemName.mid(0, extIndex);
            t << "\t-$(MAKE) -f \"" << Option::fixPathToTargetOS(fixedItem) << "\" dodistclean" << endl;
        }

    }

    generatedFiles << Option::output.fileName(); // bld.inf
    generatedFiles << project->values("QMAKE_INTERNAL_PRL_FILE"); // Add generated prl files for cleanup
    generatedFiles << project->values("QMAKE_DISTCLEAN"); // Add any additional files marked for distclean
    QStringList fixedFiles;
    QStringList fixedDirs;
    foreach(QString item, generatedFiles) {
        QString fixedItem = Option::fixPathToTargetOS(outputDir.absoluteFilePath(item));
        if (!fixedFiles.contains(fixedItem)) {
            fixedFiles << fixedItem;
        }
    }
    foreach(QString item, generatedDirs) {
        QString fixedItem = Option::fixPathToTargetOS(outputDir.absoluteFilePath(item));
        if (!fixedDirs.contains(fixedItem)) {
            fixedDirs << fixedItem;
        }
    }
    generateCleanCommands(t, fixedFiles, "$(DEL_FILE)", "", "", "");
    generateCleanCommands(t, fixedDirs, "$(DEL_DIR)", "", "", "");
    t << endl;

    t << "distclean: clean dodistclean" << endl;
    t << endl;
}

// Returns a string that can be used as a dependency to loc file on other targets
QString SymbianMakefileGenerator::generateLocFileTarget(QTextStream& t, const QString& locCmd)
{
    QString locFile;
    if (targetType == TypeExe && !project->isActiveConfig("no_icon")) {
        locFile = Option::fixPathToLocalOS(generateLocFileName());
        t << locFile << QLatin1String(": ") << project->values("SYMBIAN_MATCHED_TRANSLATIONS").join(" ") << endl;
        t << locCmd << endl;
        t << endl;
        locFile += QLatin1Char(' ');
    }

    return locFile;
}
