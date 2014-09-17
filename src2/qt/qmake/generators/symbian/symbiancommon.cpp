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

#include "symbiancommon.h"
#include <qdebug.h>
#include <qxmlstream.h>

// Included from tools/shared
#include <symbian/epocroot_p.h>

#define RESOURCE_DIRECTORY_RESOURCE "\\\\resource\\\\apps\\\\"

#define RSS_RULES "RSS_RULES"
#define RSS_RULES_BASE "RSS_RULES."
#define RSS_TAG_NBROFICONS "number_of_icons"
#define RSS_TAG_ICONFILE "icon_file"
#define RSS_TAG_HEADER "header"
#define RSS_TAG_SERVICE_LIST "service_list"
#define RSS_TAG_FILE_OWNERSHIP_LIST "file_ownership_list"
#define RSS_TAG_DATATYPE_LIST "datatype_list"
#define RSS_TAG_FOOTER "footer"
#define RSS_TAG_DEFAULT "default_rules" // Same as just giving rules without tag

#define PLUGIN_COMMON_DEF_FILE_ACTUAL "plugin_commonu.def"

#define MANUFACTURER_NOTE_FILE "manufacturer_note.txt"
#define DEFAULT_MANUFACTURER_NOTE \
    "The package is not supported for devices from this manufacturer. Please try the selfsigned " \
    "version of the package instead."

SymbianCommonGenerator::SymbianCommonGenerator(MakefileGenerator *generator)
    : generator(generator)
{
}

void SymbianCommonGenerator::init()
{
    QMakeProject *project = generator->project;
    fixedTarget = project->first("QMAKE_ORIG_TARGET");
    if (fixedTarget.isEmpty())
        fixedTarget = project->first("TARGET");
    fixedTarget = generator->unescapeFilePath(fixedTarget);
    fixedTarget = removePathSeparators(fixedTarget);
    removeSpecialCharacters(fixedTarget);

    // This should not be empty since the mkspecs are supposed to set it if missing.
    uid3 = project->first("TARGET.UID3").trimmed();

    if ((project->values("TEMPLATE")).contains("app"))
        targetType = TypeExe;
    else if ((project->values("TEMPLATE")).contains("lib")) {
        // Check CONFIG to see if we are to build staticlib or dll
        if (project->isActiveConfig("staticlib") || project->isActiveConfig("static"))
            targetType = TypeLib;
        else if (project->isActiveConfig("plugin"))
            targetType = TypePlugin;
        else
            targetType = TypeDll;
    } else {
        targetType = TypeSubdirs;
    }

    // UID is valid as either hex or decimal, so just convert it to number and back to hex
    // to get proper string for private dir
    bool conversionOk = false;
    uint uidNum = uid3.toUInt(&conversionOk, 0);

    if (!conversionOk) {
        fprintf(stderr, "Error: Invalid UID \"%s\".\n", uid3.toUtf8().constData());
    } else {
        privateDirUid.setNum(uidNum, 16);
        while (privateDirUid.length() < 8)
            privateDirUid.insert(0, QLatin1Char('0'));
    }
}

bool SymbianCommonGenerator::containsStartWithItem(const QChar &c, const QStringList& src)
{
    bool result = false;
    foreach(QString str, src) {
        if (str.startsWith(c)) {
            result =  true;
            break;
        }
    }
    return result;
}

void SymbianCommonGenerator::removeSpecialCharacters(QString& str)
{
    // When modifying this method check also symbianRemoveSpecialCharacters in symbian.conf
    QString underscore = QLatin1String("_");
    str.replace(QLatin1String("/"), underscore);
    str.replace(QLatin1String("\\"), underscore);
    str.replace(QLatin1String(" "), underscore);
    str.replace(QLatin1String(":"), underscore);
}

QString romPath(const QString& path)
{
    if(path.length() > 2 && path[1] == ':')
        return QLatin1String("z:") + path.mid(2);
    return QLatin1String("z:") + path;
}

void SymbianCommonGenerator::generatePkgFile(const QString &iconFile,
                                             bool epocBuild,
                                             const SymbianLocalizationList &symbianLocalizationList)
{
    QMakeProject *project = generator->project;
    QString pkgFilename = Option::output_dir + QLatin1Char('/') +
                          QString("%1_template.pkg").arg(fixedTarget);

    QFile pkgFile(pkgFilename);
    if (!pkgFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        PRINT_FILE_CREATE_ERROR(pkgFilename);
        return;
    }

    QString stubPkgFileName = Option::output_dir + QLatin1Char('/') +
                          QString("%1_stub.pkg").arg(fixedTarget);

    QFile stubPkgFile(stubPkgFileName);
    if (!stubPkgFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        PRINT_FILE_CREATE_ERROR(stubPkgFileName);
        return;
    }

    generatedFiles << pkgFile.fileName();
    QTextStream t(&pkgFile);
    generatedFiles << stubPkgFile.fileName();
    QTextStream ts(&stubPkgFile);

    QString installerSisHeader = project->values("DEPLOYMENT.installer_header").join("\n");
    if (installerSisHeader.isEmpty()) {
        // Use correct protected UID for publishing if application UID is in protected range,
        // otherwise use self-signable test UID.
        QRegExp protUidMatcher("0[xX][0-7].*");
        if (protUidMatcher.exactMatch(uid3))
            installerSisHeader = QLatin1String("0x2002CCCF");
        else
            installerSisHeader = QLatin1String("0xA000D7CE"); // Use default self-signable UID
    }

    QString wrapperStreamBuffer;
    QTextStream tw(&wrapperStreamBuffer);

    QString dateStr = QDateTime::currentDateTime().toString(Qt::ISODate);

    // Header info
    QString wrapperPkgFilename = Option::output_dir + QLatin1Char('/') + QString("%1_installer.%2")
                                 .arg(fixedTarget).arg("pkg");

    QString headerComment = "; %1 generated by qmake at %2\n"
        "; This file is generated by qmake and should not be modified by the user\n"
        ";\n\n";
    t << headerComment.arg(pkgFilename).arg(dateStr);
    tw << headerComment.arg(wrapperPkgFilename).arg(dateStr);
    ts << headerComment.arg(stubPkgFileName).arg(dateStr);

    QStringList commonRawPreRules;
    QStringList mainRawPreRules;
    QStringList instRawPreRules;
    QStringList stubRawPreRules;

    // Though there can't be more than one language or header line, use stringlists
    // in case user wants comments to go with the rules.
    // Note that it makes no sense to have file specific language or header rules,
    // except what is provided for installer header via "DEPLOYMENT.installer_header" variable,
    // because stub and main headers should always match. Vendor rules are similarly limited to
    // make code cleaner as it is unlikely anyone will want different vendor in different files.
    QStringList languageRules;
    QStringList headerRules;
    QStringList vendorRules;

    QStringList commonRawPostRules;
    QStringList mainRawPostRules;
    QStringList instRawPostRules;
    QStringList stubRawPostRules;

    QStringList failList; // Used for detecting incorrect usage

    QString emptySuffix;
    QString mainSuffix(".main");
    QString instSuffix(".installer");
    QString stubSuffix(".stub");

    foreach(QString item, project->values("DEPLOYMENT")) {
        parsePreRules(item, emptySuffix, &commonRawPreRules, &languageRules, &headerRules, &vendorRules);
        parsePreRules(item, mainSuffix, &mainRawPreRules, &failList, &failList, &failList);
        parsePreRules(item, instSuffix, &instRawPreRules, &failList, &failList, &failList);
        parsePreRules(item, stubSuffix, &stubRawPreRules, &failList, &failList, &failList);

        parsePostRules(item, emptySuffix, &commonRawPostRules);
        parsePostRules(item, mainSuffix, &mainRawPostRules);
        parsePostRules(item, instSuffix, &instRawPostRules);
        parsePostRules(item, stubSuffix, &stubRawPostRules);
    }

    if (!failList.isEmpty()) {
        fprintf(stderr, "Warning: Custom language, header, or vendor definitions are not "
                "supported by file specific pkg_prerules.* variables.\n"
                "Use plain pkg_prerules and/or DEPLOYMENT.installer_header for customizing "
                "these items.\n");
    }

    foreach(QString item, commonRawPreRules) {
        if (item.startsWith("(")) {
            // Only regular pkg file should have package dependencies
            mainRawPreRules << item;
        } else if (item.startsWith("[")) {
            // stub pkg file should not have platform dependencies
            mainRawPreRules << item;
            instRawPreRules << item;
        } else {
            mainRawPreRules << item;
            instRawPreRules << item;
            stubRawPreRules << item;
        }
    }

    // Currently common postrules only go to main
    mainRawPostRules << commonRawPostRules;

    // Apply some defaults if specific data does not exist in PKG pre-rules
    if (languageRules.isEmpty()) {
        if (symbianLocalizationList.isEmpty()) {
            languageRules << "; Language\n&EN\n\n";
        } else {
            QStringList langCodes;
            SymbianLocalizationListIterator iter(symbianLocalizationList);
            while (iter.hasNext()) {
                const SymbianLocalization &loc = iter.next();
                langCodes << loc.symbianLanguageCode;
            }
            languageRules << QString("; Languages\n&%1\n\n").arg(langCodes.join(","));
        }
    } else if (headerRules.isEmpty()) {
        // In case user defines langs, he must take care also about SIS header
        fprintf(stderr, "Warning: If language is defined with DEPLOYMENT pkg_prerules, also the SIS header must be defined\n");
    }

    t << languageRules.join("\n") << endl;
    tw << languageRules.join("\n") << endl;
    ts << languageRules.join("\n") << endl;

    // Determine application version. If version has missing component values,
    // those will default to zero.
    // If VERSION is missing altogether or is invalid, use "1,0,0"
    QStringList verNumList = project->first("VERSION").split('.');
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

    QString applicationVersion("1,0,0");
    if (success)
        applicationVersion = QString("%1,%2,%3").arg(major).arg(minor).arg(patch);

    // Append package build version number if it is set
    QString pkgBuildVersion = project->first("DEPLOYMENT.pkg_build_version");
    if (!pkgBuildVersion.isEmpty()) {
        success = false;
        uint build = pkgBuildVersion.toUInt(&success);
        if (success && build < 100) {
            if (pkgBuildVersion.size() == 1)
                pkgBuildVersion.prepend(QLatin1Char('0'));
            applicationVersion.append(pkgBuildVersion);
        } else {
            fprintf(stderr, "Warning: Invalid DEPLOYMENT.pkg_build_version (%s), must be a number between 0 - 99\n", qPrintable(pkgBuildVersion));
        }
    }

    // Package header
    QString sisHeader = "; SIS header: name, uid, version\n#{\"%1\"},(%2),%3\n\n";

    QString defaultVisualTarget = project->values("DEPLOYMENT.display_name").join(" ");
    if (defaultVisualTarget.isEmpty())
        defaultVisualTarget = generator->escapeFilePath(project->first("TARGET"));
    defaultVisualTarget = removePathSeparators(defaultVisualTarget);

    QString visualTarget = generatePkgNameForHeader(symbianLocalizationList, defaultVisualTarget, false);
    QString wrapperTarget = generatePkgNameForHeader(symbianLocalizationList, defaultVisualTarget, true);

    if (installerSisHeader.startsWith("0x", Qt::CaseInsensitive)) {
        tw << sisHeader.arg(wrapperTarget).arg(installerSisHeader).arg(applicationVersion);
    } else {
        tw << installerSisHeader << endl;
    }

    if (headerRules.isEmpty()) {
        t << sisHeader.arg(visualTarget).arg(uid3).arg(applicationVersion);
        ts << sisHeader.arg(visualTarget).arg(uid3).arg(applicationVersion);
    }
    else {
        t << headerRules.join("\n") << endl;
        ts << headerRules.join("\n") << endl;
    }

    // Vendor name
    if (!containsStartWithItem('%', vendorRules)) {
        QString vendorStr = QLatin1String("\"Vendor\",");
        QString locVendors = vendorStr;
        for (int i = 1; i < symbianLocalizationList.size(); i++) {
            locVendors.append(vendorStr);
        }
        locVendors.chop(1);
        vendorRules << QString("; Default localized vendor name\n%{%1}\n\n").arg(locVendors);
    }
    if (!containsStartWithItem(':', vendorRules)) {
        vendorRules << "; Default unique vendor name\n:\"Vendor\"\n\n";
    }

    t << vendorRules.join("\n") << endl;
    tw << vendorRules.join("\n") << endl;
    ts << vendorRules.join("\n") << endl;

    // PKG pre-rules - these are added before actual file installations i.e. SIS package body
    QString comment = "\n; Manual PKG pre-rules from PRO files\n";

    if (mainRawPreRules.size()) {
        t << comment;
        t << mainRawPreRules.join("\n") << endl;
    }
    if (instRawPreRules.size()) {
        tw << comment;
        tw << instRawPreRules.join("\n") << endl;
    }
    if (stubRawPreRules.size()) {
        ts << comment;
        ts << stubRawPreRules.join("\n") << endl;
    }

    t << endl;
    tw << endl;
    ts << endl;

    // Begin Manufacturer block
    if (!project->values("DEPLOYMENT.manufacturers").isEmpty()) {
        QString manufacturerStr("IF ");
        foreach(QString manufacturer, project->values("DEPLOYMENT.manufacturers")) {
            manufacturerStr.append(QString("(MANUFACTURER)=(%1) OR \n   ").arg(manufacturer));
        }
        // Remove the final OR
        manufacturerStr.chop(8);
        t << manufacturerStr << endl;
    }

    if (symbianLocalizationList.size()) {
        // Add localized resources to DEPLOYMENT if default resource deployment is done
        addLocalizedResourcesToDeployment("default_resource_deployment.files", symbianLocalizationList);
    }

    // deploy files specified by DEPLOYMENT variable
    QString remoteTestPath;
    QString zDir;
    remoteTestPath = QString("!:\\private\\%1").arg(privateDirUid);
    if (epocBuild)
        zDir = qt_epocRoot() + QLatin1String("epoc32/data/z");

    DeploymentList depList;
    initProjectDeploySymbian(project, depList, remoteTestPath, true, epocBuild, "$(PLATFORM)", "$(TARGET)", generatedDirs, generatedFiles);
    if (depList.size())
        t << "; DEPLOYMENT" << endl;
    for (int i = 0; i < depList.size(); ++i)  {
        QString from = depList.at(i).from;
        QString to = depList.at(i).to;
        QString flags;
        bool showOnlyFile = false;
        foreach(QString flag, depList.at(i).flags) {
            if (flag == QLatin1String("FT")
                || flag == QLatin1String("FILETEXT")) {
                showOnlyFile = true;
            }
            flags.append(QLatin1Char(',')).append(flag);
        }

        if (epocBuild) {
            // Deploy anything not already deployed from under epoc32 instead from under
            // \epoc32\data\z\ to enable using pkg file without rebuilding
            // the project, which can be useful for some binary only distributions.
            if (!from.contains(QLatin1String("epoc32"), Qt::CaseInsensitive)) {
                from = to;
                if (from.size() > 1 && from.at(1) == QLatin1Char(':'))
                    from = from.mid(2);
                from.prepend(zDir);
            }
        }

        // Files with "FILETEXT"/"FT" flag are meant for showing only at installation time
        // and therefore do not belong to the stub package and will not install the file into phone.
        if (showOnlyFile)
            to.clear();
        else
            ts << QString("\"\" - \"%1\"").arg(romPath(to)) << endl;

        t << QString("\"%1\" - \"%2\"%3").arg(from.replace('\\','/')).arg(to).arg(flags) << endl;

    }
    t << endl;
    ts << endl;

    // PKG post-rules - these are added after actual file installations i.e. SIS package body
    comment = "; Manual PKG post-rules from PRO files\n";

    if (mainRawPostRules.size()) {
        t << comment;
        t << mainRawPostRules.join("\n") << endl;
    }
    if (instRawPostRules.size()) {
        tw << comment;
        tw << instRawPostRules.join("\n") << endl;
    }
    if (stubRawPostRules.size()) {
        ts << comment;
        ts << stubRawPostRules.join("\n") << endl;
    }

    // Close Manufacturer block
    if (!project->values("DEPLOYMENT.manufacturers").isEmpty()) {
        QString manufacturerFailNoteFile;
        if (project->values("DEPLOYMENT.manufacturers.fail_note").isEmpty()) {
            manufacturerFailNoteFile = QString("%1_" MANUFACTURER_NOTE_FILE).arg(uid3);
            QFile ft(manufacturerFailNoteFile);
            if (ft.open(QIODevice::WriteOnly)) {
                generatedFiles << ft.fileName();
                QTextStream t2(&ft);

                t2 << QString(DEFAULT_MANUFACTURER_NOTE) << endl;
            } else {
                PRINT_FILE_CREATE_ERROR(manufacturerFailNoteFile)
            }
        } else {
            manufacturerFailNoteFile = project->values("DEPLOYMENT.manufacturers.fail_note").join("");
        }

        t << "ELSEIF NOT(0) ; MANUFACTURER" << endl
          << "\"" << generator->fileInfo(manufacturerFailNoteFile).absoluteFilePath() << "\""
          << " - \"\", FILETEXT, TEXTEXIT" << endl
          << "ENDIF ; MANUFACTURER" << endl;
    }

    // Write wrapper pkg
    if (!installerSisHeader.isEmpty()) {
        QFile wrapperPkgFile(wrapperPkgFilename);
        if (!wrapperPkgFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            PRINT_FILE_CREATE_ERROR(wrapperPkgFilename);
            return;
        }

        generatedFiles << wrapperPkgFile.fileName();
        QTextStream twf(&wrapperPkgFile);

        twf << wrapperStreamBuffer << endl;

        // Wrapped files deployment
        QString currentPath = Option::output_dir;
        if (!currentPath.endsWith(QLatin1Char('/')))
            currentPath += QLatin1Char('/');
        QString sisName = QString("%1.sis").arg(fixedTarget);
        twf << "\"" << currentPath << sisName << "\" - \"!:\\private\\2002CCCE\\import\\" << sisName << "\"" << endl;

        QString bootStrapPath = QLibraryInfo::location(QLibraryInfo::PrefixPath);
        bootStrapPath.append("/smartinstaller.sis");
        QFileInfo fi(generator->fileInfo(bootStrapPath));
        twf << "@\"" << fi.absoluteFilePath() << "\",(0x2002CCCD)" << endl;
    }
}

QString SymbianCommonGenerator::removePathSeparators(QString &file)
{
    QString ret = file;

    if (QDir::separator().unicode() != '/')
        ret.replace(QDir::separator(), QLatin1Char('/'));

    if (ret.indexOf(QLatin1Char('/')) >= 0)
        ret.remove(0, ret.lastIndexOf(QLatin1Char('/')) + 1);

    return ret;
}

void SymbianCommonGenerator::writeRegRssFile(QMap<QString, QStringList> &userItems)
{
    QString filename(fixedTarget);
    filename.append("_reg.rss");
    if (!Option::output_dir.isEmpty())
        filename = Option::output_dir + '/' + filename;
    QFile ft(filename);
    if (ft.open(QIODevice::WriteOnly)) {
        generatedFiles << ft.fileName();
        QTextStream t(&ft);
        t << "// ============================================================================" << endl;
        t << "// * Generated by qmake (" << qmake_version() << ") (Qt " QT_VERSION_STR ") on: ";
        t << QDateTime::currentDateTime().toString(Qt::ISODate) << endl;
        t << "// * This file is generated by qmake and should not be modified by the" << endl;
        t << "// * user." << endl;
        t << "// ============================================================================" << endl;
        t << endl;
        t << "#include <" << fixedTarget << ".rsg>" << endl;
        t << "#include <appinfo.rh>" << endl;
        foreach(QString item, userItems[RSS_TAG_HEADER])
            t << item << endl;
        t << endl;
        t << "UID2 KUidAppRegistrationResourceFile" << endl;
        t << "UID3 " << uid3 << endl << endl;
        t << "RESOURCE APP_REGISTRATION_INFO" << endl;
        t << "\t{" << endl;
        t << "\tapp_file=\"" << fixedTarget << "\";" << endl;
        t << "\tlocalisable_resource_file=\"" RESOURCE_DIRECTORY_RESOURCE << fixedTarget << "\";" << endl;

        writeRegRssList(t, userItems[RSS_TAG_SERVICE_LIST],
                        QLatin1String(RSS_TAG_SERVICE_LIST),
                        QLatin1String("SERVICE_INFO"));
        writeRegRssList(t, userItems[RSS_TAG_FILE_OWNERSHIP_LIST],
                        QLatin1String(RSS_TAG_FILE_OWNERSHIP_LIST),
                        QLatin1String("FILE_OWNERSHIP_INFO"));
        writeRegRssList(t, userItems[RSS_TAG_DATATYPE_LIST],
                        QLatin1String(RSS_TAG_DATATYPE_LIST),
                        QLatin1String("DATATYPE"));
        t << endl;

        foreach(QString item, userItems[RSS_TAG_DEFAULT])
            t << "\t" << item.replace("\n","\n\t") << endl;
        t << "\t}" << endl;

        foreach(QString item, userItems[RSS_TAG_FOOTER])
            t << item << endl;
    } else {
        PRINT_FILE_CREATE_ERROR(filename)
    }
}

void SymbianCommonGenerator::writeRegRssList(QTextStream &t,
                                               QStringList &userList,
                                               const QString &listTag,
                                               const QString &listItem)
{
    int itemCount = userList.count();
    if (itemCount) {
        t << "\t" << listTag << " ="<< endl;
        t << "\t\t{" << endl;
        foreach(QString item, userList) {
            t << "\t\t" << listItem << endl;
            t << "\t\t\t{" << endl;
            t << "\t\t\t" << item.replace("\n","\n\t\t\t") << endl;
            t << "\t\t\t}";
            if (--itemCount)
                t << ",";
            t << endl;
        }
        t << "\t\t}; "<< endl;
    }
}

void SymbianCommonGenerator::writeRssFile(QString &numberOfIcons, QString &iconFile)
{
    QString filename(fixedTarget);
    if (!Option::output_dir.isEmpty())
        filename = Option::output_dir + '/' + filename;
    filename.append(".rss");
    QFile ft(filename);
    if (ft.open(QIODevice::WriteOnly)) {
        generatedFiles << ft.fileName();
        QTextStream t(&ft);
        t << "// ============================================================================" << endl;
        t << "// * Generated by qmake (" << qmake_version() << ") (Qt " QT_VERSION_STR ") on: ";
        t << QDateTime::currentDateTime().toString(Qt::ISODate) << endl;
        t << "// * This file is generated by qmake and should not be modified by the" << endl;
        t << "// * user." << endl;
        t << "// ============================================================================" << endl;
        t << endl;
        t << "CHARACTER_SET UTF8" << endl;
        t << "#include <appinfo.rh>" << endl;
        t << "#include \"" << fixedTarget << ".loc\"" << endl;
        t << endl;
        t << "RESOURCE LOCALISABLE_APP_INFO r_localisable_app_info" << endl;
        t << "\t{" << endl;
        t << "\tshort_caption = STRING_r_short_caption;" << endl;
        t << "\tcaption_and_icon =" << endl;
        t << "\tCAPTION_AND_ICON_INFO" << endl;
        t << "\t\t{" << endl;
        t << "\t\tcaption = STRING_r_caption;" << endl;

        QString rssIconFile = iconFile;
        rssIconFile = rssIconFile.replace("/", "\\\\");

        if (numberOfIcons.isEmpty() || rssIconFile.isEmpty()) {
            // There can be maximum one item in this tag, validated when parsed
            t << "\t\tnumber_of_icons = 0;" << endl;
            t << "\t\ticon_file = \"\";" << endl;
        } else {
            // There can be maximum one item in this tag, validated when parsed
            t << "\t\tnumber_of_icons = " << numberOfIcons << ";" << endl;
            t << "\t\ticon_file = \"" << rssIconFile << "\";" << endl;
        }
        t << "\t\t};" << endl;
        t << "\t}" << endl;
        t << endl;
    } else {
        PRINT_FILE_CREATE_ERROR(filename);
    }
}

void SymbianCommonGenerator::writeLocFile(const SymbianLocalizationList &symbianLocalizationList)
{
    QString filename = generateLocFileName();
    QFile ft(filename);
    if (ft.open(QIODevice::WriteOnly)) {
        generatedFiles << ft.fileName();
        QTextStream t(&ft);

        QString displayName = generator->project->values("DEPLOYMENT.display_name").join(" ");
        if (displayName.isEmpty())
            displayName = generator->escapeFilePath(generator->project->first("TARGET"));

        t << "// ============================================================================" << endl;
        t << "// * Generated by qmake (" << qmake_version() << ") (Qt " QT_VERSION_STR ") on: ";
        t << QDateTime::currentDateTime().toString(Qt::ISODate) << endl;
        t << "// * This file is generated by qmake and should not be modified by the" << endl;
        t << "// * user." << endl;
        t << "// ============================================================================" << endl;
        t << endl;
        t << "#ifdef LANGUAGE_SC" << endl;
        t << "#define STRING_r_short_caption \"" << displayName  << "\"" << endl;
        t << "#define STRING_r_caption \"" << displayName  << "\"" << endl;

        SymbianLocalizationListIterator iter(symbianLocalizationList);
        while (iter.hasNext()) {
            const SymbianLocalization &loc = iter.next();
            QString shortCaption = loc.shortCaption;
            QString longCaption = loc.longCaption;
            if (shortCaption.isEmpty())
                shortCaption = displayName;
            if (longCaption.isEmpty())
                longCaption = displayName;

            t << "#elif defined LANGUAGE_" << loc.symbianLanguageCode << endl;
            t << "#define STRING_r_short_caption \"" << shortCaption << "\"" << endl;
            t << "#define STRING_r_caption \"" << longCaption << "\"" << endl;
        }

        t << "#else" << endl;
        t << "#define STRING_r_short_caption \"" << displayName  << "\"" << endl;
        t << "#define STRING_r_caption \"" << displayName  << "\"" << endl;
        t << "#endif" << endl;
    } else {
        PRINT_FILE_CREATE_ERROR(filename);
    }
}

void SymbianCommonGenerator::readRssRules(QString &numberOfIcons,
                                            QString &iconFile, QMap<QString,
                                            QStringList> &userRssRules)
{
    QMakeProject *project = generator->project;
    for (QMap<QString, QStringList>::iterator it = project->variables().begin(); it != project->variables().end(); ++it) {
        if (it.key().startsWith(RSS_RULES_BASE)) {
            QString newKey = it.key().mid(sizeof(RSS_RULES_BASE) - 1);
            if (newKey.isEmpty()) {
                fprintf(stderr, "Warning: Empty RSS_RULES_BASE key encountered\n");
                continue;
            }
            QStringList newValues;
            QStringList values = it.value();
            foreach(QString item, values) {
                // If there is no stringlist defined for a rule, use rule value directly
                // This is convenience for defining single line statements
                if (project->values(item).isEmpty()) {
                    newValues << item;
                } else {
                    QStringList itemList;
                    foreach(QString itemRow, project->values(item)) {
                        itemList << itemRow;
                    }
                    newValues << itemList.join("\n");
                }
            }
            // Verify that there is exactly one value in RSS_TAG_NBROFICONS
            if (newKey == RSS_TAG_NBROFICONS) {
                if (newValues.count() == 1) {
                    numberOfIcons = newValues[0];
                } else {
                    fprintf(stderr, "Warning: There must be exactly one value in '%s%s'\n",
                            RSS_RULES_BASE, RSS_TAG_NBROFICONS);
                    continue;
                }
            // Verify that there is exactly one value in RSS_TAG_ICONFILE
            } else if (newKey == RSS_TAG_ICONFILE) {
                if (newValues.count() == 1) {
                    iconFile = newValues[0];
                } else {
                    fprintf(stderr, "Warning: There must be exactly one value in '%s%s'\n",
                            RSS_RULES_BASE, RSS_TAG_ICONFILE);
                    continue;
                }
            } else if (newKey == RSS_TAG_HEADER
                       || newKey == RSS_TAG_SERVICE_LIST
                       || newKey == RSS_TAG_FILE_OWNERSHIP_LIST
                       || newKey == RSS_TAG_DATATYPE_LIST
                       || newKey == RSS_TAG_FOOTER
                       || newKey == RSS_TAG_DEFAULT) {
                userRssRules[newKey] = newValues;
                continue;
            } else {
                fprintf(stderr, "Warning: Unsupported key:'%s%s'\n",
                        RSS_RULES_BASE, newKey.toLatin1().constData());
                continue;
            }
        }
    }

    QStringList newValues;
    foreach(QString item, project->values(RSS_RULES)) {
        // If there is no stringlist defined for a rule, use rule value directly
        // This is convenience for defining single line statements
        if (project->values(item).isEmpty()) {
            newValues << item;
        } else {
            newValues << project->values(item);
        }
    }
    userRssRules[RSS_TAG_DEFAULT] << newValues;

    // Validate that either both RSS_TAG_NBROFICONS and RSS_TAG_ICONFILE keys exist
    // or neither of them exist
    if (!((numberOfIcons.isEmpty() && iconFile.isEmpty()) ||
            (!numberOfIcons.isEmpty() && !iconFile.isEmpty()))) {
        numberOfIcons.clear();
        iconFile.clear();
        fprintf(stderr, "Warning: Both or neither of '%s%s' and '%s%s' keys must exist.\n",
                RSS_RULES_BASE, RSS_TAG_NBROFICONS, RSS_RULES_BASE, RSS_TAG_ICONFILE);
    }

    // Validate that RSS_TAG_NBROFICONS contains only numbers
    if (!numberOfIcons.isEmpty()) {
        bool ok;
        numberOfIcons = numberOfIcons.simplified();
        numberOfIcons.toInt(&ok);
        if (!ok) {
            numberOfIcons.clear();
            iconFile.clear();
            fprintf(stderr, "Warning: '%s%s' must be integer in decimal format.\n",
                    RSS_RULES_BASE, RSS_TAG_NBROFICONS);
        }
    }
}

void SymbianCommonGenerator::writeCustomDefFile()
{
    if (targetType == TypePlugin && !generator->project->isActiveConfig("stdbinary")) {
        // Create custom def file for plugin
        QFile ft(Option::output_dir + QLatin1Char('/') + QLatin1String(PLUGIN_COMMON_DEF_FILE_ACTUAL));

        if (ft.open(QIODevice::WriteOnly)) {
            generatedFiles << ft.fileName();
            QTextStream t(&ft);

            t << "; ==============================================================================" << endl;
            t << "; Generated by qmake (" << qmake_version() << ") (Qt " QT_VERSION_STR ") on: ";
            t << QDateTime::currentDateTime().toString(Qt::ISODate) << endl;
            t << "; This file is generated by qmake and should not be modified by the" << endl;
            t << "; user." << endl;
            t << ";  Name        : " PLUGIN_COMMON_DEF_FILE_ACTUAL << endl;
            t << ";  Part of     : " << generator->project->values("TARGET").join(" ") << endl;
            t << ";  Description : Fixes common plugin symbols to known ordinals" << endl;
            t << ";  Version     : " << endl;
            t << ";" << endl;
            t << "; ==============================================================================" << "\n" << endl;

            t << endl;

            t << "EXPORTS" << endl;
            t << "\tqt_plugin_query_verification_data @ 1 NONAME" << endl;
            t << "\tqt_plugin_instance @ 2 NONAME" << endl;
            t << endl;
        } else {
            PRINT_FILE_CREATE_ERROR(QString(PLUGIN_COMMON_DEF_FILE_ACTUAL))
        }
    }
}

void SymbianCommonGenerator::parseTsFiles(SymbianLocalizationList *symbianLocalizationList)
{
    if (!generator->project->isActiveConfig("localize_deployment")) {
        return;
    }

    QStringList symbianTsFiles;

    symbianTsFiles << generator->project->values("SYMBIAN_MATCHED_TRANSLATIONS");

    if (!symbianTsFiles.isEmpty()) {
        fillQt2SymbianLocalizationList(symbianLocalizationList);

        QMutableListIterator<SymbianLocalization> iter(*symbianLocalizationList);
        while (iter.hasNext()) {
            SymbianLocalization &loc = iter.next();
            static QString matchStrTemplate = QLatin1String(".*_%1\\.ts");
            QString matchStr = matchStrTemplate.arg(loc.qtLanguageCode);

            foreach (QString file, symbianTsFiles) {
                QRegExp matcher(matchStr);
                matcher.setCaseSensitivity(Qt::CaseInsensitive);
                if (matcher.exactMatch(file) && parseTsContent(file, &loc))
                    break;
            }
        }
    }
}

void SymbianCommonGenerator::fillQt2SymbianLocalizationList(SymbianLocalizationList *symbianLocalizationList)
{
    static QString symbianCodePrefix = QLatin1String("SYMBIAN_LANG.");

    QStringList symbianLanguages = generator->project->values("SYMBIAN_MATCHED_LANGUAGES");

    foreach (QString qtCode, symbianLanguages) {
        QString symbianCodeVariable = symbianCodePrefix + qtCode;
        QStringList symbianCodes = generator->project->values(symbianCodeVariable);
		// Some languages have more than one Symbian code, so they get more than one
		// entry in symbianLocalizationList.
        foreach (QString symbianCode, symbianCodes) {
            SymbianLocalization newLoc;
            newLoc.symbianLanguageCode = symbianCode;
            if (!newLoc.symbianLanguageCode.isEmpty()) {
                newLoc.qtLanguageCode = qtCode;
                symbianLocalizationList->append(newLoc);
            }
        }
    }
}

void SymbianCommonGenerator::parsePreRules(const QString &deploymentVariable,
                                           const QString &variableSuffix,
                                           QStringList *rawRuleList,
                                           QStringList *languageRuleList,
                                           QStringList *headerRuleList,
                                           QStringList *vendorRuleList)
{
    QMakeProject *project = generator->project;
    foreach(QString pkgrulesItem, project->values(deploymentVariable + ".pkg_prerules" + variableSuffix)) {
        QStringList pkgrulesValue = project->values(pkgrulesItem);
        // If there is no stringlist defined for a rule, use rule name directly
        // This is convenience for defining single line statements
        if (pkgrulesValue.isEmpty()) {
            if (pkgrulesItem.startsWith("&"))
                *languageRuleList << pkgrulesItem;
            else if (pkgrulesItem.startsWith("#"))
                *headerRuleList << pkgrulesItem;
            else if (pkgrulesItem.startsWith("%") || pkgrulesItem.startsWith(":"))
                *vendorRuleList << pkgrulesItem;
            else
                *rawRuleList << pkgrulesItem;
        } else {
            if (containsStartWithItem('&', pkgrulesValue)) {
                foreach(QString pkgrule, pkgrulesValue) {
                    *languageRuleList << pkgrule;
                }
            } else if (containsStartWithItem('#', pkgrulesValue)) {
                foreach(QString pkgrule, pkgrulesValue) {
                    *headerRuleList << pkgrule;
                }
            } else if (containsStartWithItem('%', pkgrulesValue)
                       || containsStartWithItem(':', pkgrulesValue)) {
                foreach(QString pkgrule, pkgrulesValue) {
                    *vendorRuleList << pkgrule;
                }
            } else {
                foreach(QString pkgrule, pkgrulesValue) {
                    *rawRuleList << pkgrule;
                }
            }
        }
    }
}

void SymbianCommonGenerator::parsePostRules(const QString &deploymentVariable,
                                            const QString &variableSuffix,
                                            QStringList *rawRuleList)
{
    QMakeProject *project = generator->project;
    foreach(QString pkgrulesItem, project->values(deploymentVariable + ".pkg_postrules" + variableSuffix)) {
        QStringList pkgrulesValue = project->values(pkgrulesItem);
        // If there is no stringlist defined for a rule, use rule name directly
        // This is convenience for defining single line statements
        if (pkgrulesValue.isEmpty()) {
            *rawRuleList << pkgrulesItem;
        } else {
            foreach(QString pkgrule, pkgrulesValue) {
                *rawRuleList << pkgrule;
            }
        }
    }
}

bool SymbianCommonGenerator::parseTsContent(const QString &tsFilename, SymbianLocalization *loc)
{
    bool retval = true;
    QMakeProject *project = generator->project;
    QFile tsFile(tsFilename);

    if (tsFile.exists()) {
        if (tsFile.open(QIODevice::ReadOnly)) {
            static QString applicationCaptionsContext = QLatin1String("QtApplicationCaptions");
            static QString pkgNameContext = QLatin1String("QtPackageNames");
            static QString tsElement = QLatin1String("TS");
            static QString contextElement = QLatin1String("context");
            static QString nameElement = QLatin1String("name");
            static QString messageElement = QLatin1String("message");
            static QString sourceElement = QLatin1String("source");
            static QString translationElement = QLatin1String("translation");
            static QString idAttribute = QLatin1String("id");
            static QString shortCaptionId = QLatin1String("qtn_short_caption_");
            static QString longCaptionId = QLatin1String("qtn_long_caption_");
            static QString pkgDisplayNameId = QLatin1String("qtn_package_name_");
            static QString installerPkgDisplayNameId = QLatin1String("qtn_smart_installer_package_name_");
            static QString shortCaptionSource = QLatin1String("Application short caption");
            static QString longCaptionSource = QLatin1String("Application long caption");
            static QString pkgDisplayNameSource = QLatin1String("Package name");
            static QString installerPkgDisplayNameSource = QLatin1String("Smart installer package name");

            enum CurrentContext {
                ContextUnknown,
                ContextUninteresting,
                ContextInteresting
            };

            QXmlStreamReader xml(&tsFile);

            while (!xml.atEnd() && xml.name() != tsElement)
                xml.readNextStartElement();

            while (xml.readNextStartElement()) {
                if (xml.name() == contextElement) {
                    CurrentContext currentContext = ContextUnknown;
                    while (xml.readNextStartElement()) {
                        if (currentContext == ContextUnknown) {
                            // Expect name element before message elements
                            if (xml.name() == nameElement) {
                                QString nameText = xml.readElementText();
                                if (nameText == applicationCaptionsContext || nameText == pkgNameContext) {
                                    currentContext = ContextInteresting;
                                } else {
                                    currentContext = ContextUninteresting;
                                }
                            } else {
                                xml.skipCurrentElement();
                            }
                        } else if (currentContext == ContextInteresting) {
                            if (xml.name() == messageElement) {
                                QString source;
                                QString translation;
                                QString id = xml.attributes().value(idAttribute).toString();
                                while (xml.readNextStartElement()) {
                                    if (xml.name() == sourceElement) {
                                        source = xml.readElementText();
                                    } else if (xml.name() == translationElement) {
                                        // Technically translation element can have child elements
                                        // i.e. numerusform and lengthvariant. We don't support
                                        // these for actual caption/pkgname translations, but since
                                        // they may be present on other unrelated message elements,
                                        // we need to explicitly skip them to avoid parsing errors.
                                        translation = xml.readElementText(QXmlStreamReader::SkipChildElements);
                                    } else {
                                        xml.skipCurrentElement();
                                    }
                                }
                                // Interesting translations can be identified either by id attribute
                                // of the message or by the source text.
                                // Allow translations with correct id to override translations
                                // detected by source text, as the source text can accidentally
                                // be the same in another string if there are non-interesting
                                // translations added to same context.
                                if (id.startsWith(shortCaptionId)
                                    || (loc->shortCaption.isEmpty() && source == shortCaptionSource)) {
                                    loc->shortCaption = translation;
                                } else if (id.startsWith(longCaptionId)
                                    || (loc->longCaption.isEmpty() && source == longCaptionSource)) {
                                    loc->longCaption = translation;
                                } else if (id.startsWith(pkgDisplayNameId)
                                    || (loc->pkgDisplayName.isEmpty() && source == pkgDisplayNameSource)) {
                                    loc->pkgDisplayName = translation;
                                } else if (id.startsWith(installerPkgDisplayNameId)
                                    || (loc->installerPkgDisplayName.isEmpty() && source == installerPkgDisplayNameSource)) {
                                    loc->installerPkgDisplayName = translation;
                                }
                            } else {
                                xml.skipCurrentElement();
                            }
                        } else {
                            xml.skipCurrentElement();
                        }
                    }
                } else {
                    xml.skipCurrentElement();
                }
            }
            if (xml.hasError()) {
                retval = false;
                fprintf(stderr, "ERROR: Encountered error \"%s\" when parsing ts file (%s).\n",
                        qPrintable(xml.errorString()), qPrintable(tsFilename));
            }
        } else {
            retval = false;
            fprintf(stderr, "Warning: Could not open ts file (%s).\n", qPrintable(tsFilename));
        }
    } else {
        retval = false;
        fprintf(stderr, "Warning: ts file does not exist: (%s), unable to parse it.\n",
                qPrintable(tsFilename));
    }

    return retval;
}

QString SymbianCommonGenerator::generatePkgNameForHeader(const SymbianLocalizationList &symbianLocalizationList,
                                                         const QString &defaultName,
                                                         bool isForSmartInstaller)
{
    QStringList allNames;
    QString noTranslation = defaultName;

    if (isForSmartInstaller)
        noTranslation += QLatin1String(" installer");

    SymbianLocalizationListIterator iter(symbianLocalizationList);
    while (iter.hasNext()) {
        const SymbianLocalization &loc = iter.next();
        QString currentName;
        if (isForSmartInstaller) {
            currentName = loc.installerPkgDisplayName;
        } else {
            currentName = loc.pkgDisplayName;
        }

        if (currentName.isEmpty())
            currentName = noTranslation;

        allNames << currentName;
    }

    if (!allNames.size())
        allNames << noTranslation;

    return allNames.join("\",\"");

}

void SymbianCommonGenerator::addLocalizedResourcesToDeployment(const QString &deploymentFilesVar,
                                                               const SymbianLocalizationList &symbianLocalizationList)
{
    QStringList locResources;
    foreach (QString defaultResource, generator->project->values(deploymentFilesVar)) {
        if (defaultResource.endsWith(".rsc")) {
            defaultResource.chop(2);
            SymbianLocalizationListIterator iter(symbianLocalizationList);
            while (iter.hasNext()) {
                const SymbianLocalization &loc = iter.next();
                locResources << QString(defaultResource + loc.symbianLanguageCode);
            }
        }
    }
    generator->project->values(deploymentFilesVar) << locResources;
}

QString SymbianCommonGenerator::generateLocFileName()
{
    QString fileName(fixedTarget);
    if (!Option::output_dir.isEmpty())
        fileName = Option::output_dir + QLatin1Char('/') + fileName;
    fileName.append(".loc");
    return fileName;
}
