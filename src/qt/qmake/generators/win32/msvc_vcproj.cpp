/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the qmake application of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "msvc_vcproj.h"
#include "option.h"
#include "xmloutput.h"
#include <qdir.h>
#include <qdiriterator.h>
#include <qcryptographichash.h>
#include <qregexp.h>
#include <qhash.h>
#include <quuid.h>
#include <stdlib.h>

//#define DEBUG_SOLUTION_GEN

QT_BEGIN_NAMESPACE
// Filter GUIDs (Do NOT change these!) ------------------------------
const char _GUIDSourceFiles[]          = "{4FC737F1-C7A5-4376-A066-2A32D752A2FF}";
const char _GUIDHeaderFiles[]          = "{93995380-89BD-4b04-88EB-625FBE52EBFB}";
const char _GUIDGeneratedFiles[]       = "{71ED8ED8-ACB9-4CE9-BBE1-E00B30144E11}";
const char _GUIDResourceFiles[]        = "{D9D6E242-F8AF-46E4-B9FD-80ECBC20BA3E}";
const char _GUIDLexYaccFiles[]         = "{E12AE0D2-192F-4d59-BD23-7D3FA58D3183}";
const char _GUIDTranslationFiles[]     = "{639EADAA-A684-42e4-A9AD-28FC9BCB8F7C}";
const char _GUIDFormFiles[]            = "{99349809-55BA-4b9d-BF79-8FDBB0286EB3}";
const char _GUIDExtraCompilerFiles[]   = "{E0D8C965-CC5F-43d7-AD63-FAEF0BBC0F85}";
QT_END_NAMESPACE

#ifdef Q_OS_WIN32
#include <qt_windows.h>
#include <windows/registry_p.h>

QT_BEGIN_NAMESPACE

struct {
    DotNET version;
    const char *versionStr;
    const char *regKey;
} dotNetCombo[] = {
#ifdef Q_OS_WIN64
    {NET2010, "MSVC.NET 2010 (10.0)", "Software\\Wow6432Node\\Microsoft\\VisualStudio\\10.0\\Setup\\VC\\ProductDir"},
    {NET2010, "MSVC.NET 2010 Express Edition (10.0)", "Software\\Wow6432Node\\Microsoft\\VCExpress\\10.0\\Setup\\VC\\ProductDir"},
    {NET2008, "MSVC.NET 2008 (9.0)", "Software\\Wow6432Node\\Microsoft\\VisualStudio\\9.0\\Setup\\VC\\ProductDir"},
    {NET2008, "MSVC.NET 2008 Express Edition (9.0)", "Software\\Wow6432Node\\Microsoft\\VCExpress\\9.0\\Setup\\VC\\ProductDir"},
    {NET2005, "MSVC.NET 2005 (8.0)", "Software\\Wow6432Node\\Microsoft\\VisualStudio\\8.0\\Setup\\VC\\ProductDir"},
    {NET2005, "MSVC.NET 2005 Express Edition (8.0)", "Software\\Wow6432Node\\Microsoft\\VCExpress\\8.0\\Setup\\VC\\ProductDir"},
    {NET2003, "MSVC.NET 2003 (7.1)", "Software\\Wow6432Node\\Microsoft\\VisualStudio\\7.1\\Setup\\VC\\ProductDir"},
    {NET2002, "MSVC.NET 2002 (7.0)", "Software\\Wow6432Node\\Microsoft\\VisualStudio\\7.0\\Setup\\VC\\ProductDir"},
#else
    {NET2010, "MSVC.NET 2010 (10.0)", "Software\\Microsoft\\VisualStudio\\10.0\\Setup\\VC\\ProductDir"},
    {NET2010, "MSVC.NET 2010 Express Edition (10.0)", "Software\\Microsoft\\VCExpress\\10.0\\Setup\\VC\\ProductDir"},
    {NET2008, "MSVC.NET 2008 (9.0)", "Software\\Microsoft\\VisualStudio\\9.0\\Setup\\VC\\ProductDir"},
    {NET2008, "MSVC.NET 2008 Express Edition (9.0)", "Software\\Microsoft\\VCExpress\\9.0\\Setup\\VC\\ProductDir"},
    {NET2005, "MSVC.NET 2005 (8.0)", "Software\\Microsoft\\VisualStudio\\8.0\\Setup\\VC\\ProductDir"},
    {NET2005, "MSVC.NET 2005 Express Edition (8.0)", "Software\\Microsoft\\VCExpress\\8.0\\Setup\\VC\\ProductDir"},
    {NET2003, "MSVC.NET 2003 (7.1)", "Software\\Microsoft\\VisualStudio\\7.1\\Setup\\VC\\ProductDir"},
    {NET2002, "MSVC.NET 2002 (7.0)", "Software\\Microsoft\\VisualStudio\\7.0\\Setup\\VC\\ProductDir"},
#endif
    {NETUnknown, "", ""},
};

QT_END_NAMESPACE
#endif

QT_BEGIN_NAMESPACE
DotNET which_dotnet_version()
{
#ifndef Q_OS_WIN32
    return NET2002; // Always generate 7.0 versions on other platforms
#else
    // Only search for the version once
    static DotNET current_version = NETUnknown;
    if(current_version != NETUnknown)
        return current_version;

    // Fallback to .NET 2002
    current_version = NET2002;

    QStringList warnPath;
    QHash<DotNET, QString> installPaths;
    int installed = 0;
    int i = 0;
    for(; dotNetCombo[i].version; ++i) {
        QString path = qt_readRegistryKey(HKEY_LOCAL_MACHINE, dotNetCombo[i].regKey);
        if (!path.isEmpty() && installPaths.value(dotNetCombo[i].version) != path) {
            installPaths.insert(dotNetCombo[i].version, path);
            ++installed;
            current_version = dotNetCombo[i].version;
                        warnPath += QString("%1").arg(dotNetCombo[i].versionStr);
        }
    }

    if (installed < 2)
        return current_version;

    // More than one version installed, search directory path
    QString paths = qgetenv("PATH");
    QStringList pathlist = paths.toLower().split(";");

    i = installed = 0;
    for(; dotNetCombo[i].version; ++i) {
        QString productPath = qt_readRegistryKey(HKEY_LOCAL_MACHINE, dotNetCombo[i].regKey).toLower();
                if (productPath.isEmpty())
                        continue;
        QStringList::iterator it;
        for(it = pathlist.begin(); it != pathlist.end(); ++it) {
            if((*it).contains(productPath)) {
                ++installed;
                current_version = dotNetCombo[i].version;
                warnPath += QString("%1 in path").arg(dotNetCombo[i].versionStr);
                                break;
            }
        }
    }
        switch(installed) {
        case 1:
                break;
        case 0:
                warn_msg(WarnLogic, "Generator: MSVC.NET: Found more than one version of Visual Studio, but"
                                 " none in your path! Fallback to lowest version (%s)", warnPath.join(", ").toLatin1().data());
                break;
        default:
                warn_msg(WarnLogic, "Generator: MSVC.NET: Found more than one version of Visual Studio in"
                                 " your path! Fallback to lowest version (%s)", warnPath.join(", ").toLatin1().data());
                break;
        }

    return current_version;
#endif
};

// Flatfile Tags ----------------------------------------------------
const char _slnHeader70[]       = "Microsoft Visual Studio Solution File, Format Version 7.00";
const char _slnHeader71[]       = "Microsoft Visual Studio Solution File, Format Version 8.00";
const char _slnHeader80[]       = "Microsoft Visual Studio Solution File, Format Version 9.00"
                                  "\n# Visual Studio 2005";
const char _slnHeader90[]       = "Microsoft Visual Studio Solution File, Format Version 10.00"
                                  "\n# Visual Studio 2008";
const char _slnHeader100[]      = "Microsoft Visual Studio Solution File, Format Version 11.00"
                                  "\n# Visual Studio 2010";
                                  // The following UUID _may_ change for later servicepacks...
                                  // If so we need to search through the registry at
                                  // HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\7.0\Projects
                                  // to find the subkey that contains a "PossibleProjectExtension"
                                  // containing "vcproj"...
                                  // Use the hardcoded value for now so projects generated on other
                                  // platforms are actually usable.
const char _slnMSVCvcprojGUID[] = "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}";
const char _slnProjectBeg[]     = "\nProject(\"";
const char _slnProjectMid[]     = "\") = ";
const char _slnProjectEnd[]     = "\nEndProject";
const char _slnGlobalBeg[]      = "\nGlobal";
const char _slnGlobalEnd[]      = "\nEndGlobal";
const char _slnSolutionConf[]   = "\n\tGlobalSection(SolutionConfiguration) = preSolution"
                                  "\n\t\tConfigName.0 = Debug|Win32"
                                  "\n\t\tConfigName.1 = Release|Win32"
                                  "\n\tEndGlobalSection";
const char _slnProjDepBeg[]     = "\n\tGlobalSection(ProjectDependencies) = postSolution";
const char _slnProjDepEnd[]     = "\n\tEndGlobalSection";
const char _slnProjConfBeg[]    = "\n\tGlobalSection(ProjectConfiguration) = postSolution";
const char _slnProjRelConfTag1[]= ".Release|%1.ActiveCfg = Release|";
const char _slnProjRelConfTag2[]= ".Release|%1.Build.0 = Release|";
const char _slnProjDbgConfTag1[]= ".Debug|%1.ActiveCfg = Debug|";
const char _slnProjDbgConfTag2[]= ".Debug|%1.Build.0 = Debug|";
const char _slnProjConfEnd[]    = "\n\tEndGlobalSection";
const char _slnExtSections[]    = "\n\tGlobalSection(ExtensibilityGlobals) = postSolution"
                                  "\n\tEndGlobalSection"
                                  "\n\tGlobalSection(ExtensibilityAddIns) = postSolution"
                                  "\n\tEndGlobalSection";
// ------------------------------------------------------------------

VcprojGenerator::VcprojGenerator()
    : Win32MakefileGenerator(),
      init_flag(false),
      is64Bit(false),
      projectWriter(0)
{
}

VcprojGenerator::~VcprojGenerator()
{
    delete projectWriter;
}

bool VcprojGenerator::writeMakefile(QTextStream &t)
{
    initProject(); // Fills the whole project with proper data

    // Generate solution file
    if(project->first("TEMPLATE") == "vcsubdirs") {
        if (!project->isActiveConfig("build_pass")) {
            debug_msg(1, "Generator: MSVC.NET: Writing solution file");
            writeSubDirs(t);
        } else {
            debug_msg(1, "Generator: MSVC.NET: Not writing solution file for build_pass configs");
        }
        return true;
    } else
    // Generate single configuration project file
    if (project->first("TEMPLATE") == "vcapp" ||
        project->first("TEMPLATE") == "vclib") {
        if(!project->isActiveConfig("build_pass")) {
            debug_msg(1, "Generator: MSVC.NET: Writing single configuration project file");
            XmlOutput xmlOut(t);
            projectWriter->write(xmlOut, vcProject);
        }
        return true;
    }
    return project->isActiveConfig("build_pass");
}

bool VcprojGenerator::writeProjectMakefile()
{
    usePlatformDir();
    QTextStream t(&Option::output);

    // Check if all requirements are fulfilled
    if(!project->values("QMAKE_FAILED_REQUIREMENTS").isEmpty()) {
        fprintf(stderr, "Project file not generated because all requirements not met:\n\t%s\n",
                var("QMAKE_FAILED_REQUIREMENTS").toLatin1().constData());
        return true;
    }

    // Generate project file
    if(project->first("TEMPLATE") == "vcapp" ||
       project->first("TEMPLATE") == "vclib") {
        if (!mergedProjects.count()) {
            warn_msg(WarnLogic, "Generator: MSVC.NET: no single configuration created, cannot output project!");
            return false;
        }

        debug_msg(1, "Generator: MSVC.NET: Writing project file");
        VCProject mergedProject;
        for (int i = 0; i < mergedProjects.count(); ++i) {
            VCProjectSingleConfig *singleProject = &(mergedProjects.at(i)->vcProject);
            mergedProject.SingleProjects += *singleProject;
            for (int j = 0; j < singleProject->ExtraCompilersFiles.count(); ++j) {
                const QString &compilerName = singleProject->ExtraCompilersFiles.at(j).Name;
                if (!mergedProject.ExtraCompilers.contains(compilerName))
                    mergedProject.ExtraCompilers += compilerName;
            }
        }

        if(mergedProjects.count() > 1 &&
           mergedProjects.at(0)->vcProject.Name ==
           mergedProjects.at(1)->vcProject.Name)
            mergedProjects.at(0)->writePrlFile();
        mergedProject.Name = project->first("QMAKE_PROJECT_NAME");
        mergedProject.Version = mergedProjects.at(0)->vcProject.Version;
        mergedProject.ProjectGUID = project->isEmpty("QMAKE_UUID") ? getProjectUUID().toString().toUpper() : project->first("QMAKE_UUID");
        mergedProject.Keyword = project->first("VCPROJ_KEYWORD");
        mergedProject.SccProjectName = mergedProjects.at(0)->vcProject.SccProjectName;
        mergedProject.SccLocalPath = mergedProjects.at(0)->vcProject.SccLocalPath;
        mergedProject.PlatformName = mergedProjects.at(0)->vcProject.PlatformName;

        XmlOutput xmlOut(t);
        projectWriter->write(xmlOut, mergedProject);
        return true;
    } else if(project->first("TEMPLATE") == "vcsubdirs") {
        return writeMakefile(t);
    }
    return false;
}

struct VcsolutionDepend {
    QString uuid;
    QString vcprojFile, orig_target, target;
    Target targetType;
    QStringList dependencies;
};

QUuid VcprojGenerator::getProjectUUID(const QString &filename)
{
    bool validUUID = true;

    // Read GUID from variable-space
    QUuid uuid = project->first("GUID");

    // If none, create one based on the MD5 of absolute project path
    if(uuid.isNull() || !filename.isEmpty()) {
        QString abspath = Option::fixPathToLocalOS(filename.isEmpty()?project->first("QMAKE_MAKEFILE"):filename);
        QByteArray digest = QCryptographicHash::hash(abspath.toUtf8(), QCryptographicHash::Md5);
        memcpy((unsigned char*)(&uuid), digest.constData(), sizeof(QUuid));
        validUUID = !uuid.isNull();
        uuid.data4[0] = (uuid.data4[0] & 0x3F) | 0x80; // UV_DCE variant
        uuid.data3 = (uuid.data3 & 0x0FFF) | (QUuid::Name<<12);
    }

    // If still not valid, generate new one, and suggest adding to .pro
    if(uuid.isNull() || !validUUID) {
        uuid = QUuid::createUuid();
        fprintf(stderr,
                "qmake couldn't create a GUID based on filepath, and we couldn't\nfind a valid GUID in the .pro file (Consider adding\n'GUID = %s'  to the .pro file)\n",
                uuid.toString().toUpper().toLatin1().constData());
    }

    // Store GUID in variable-space
    project->values("GUID") = QStringList(uuid.toString().toUpper());
    return uuid;
}

QUuid VcprojGenerator::increaseUUID(const QUuid &id)
{
    QUuid result(id);
    qint64 dataFirst = (result.data4[0] << 24) +
                       (result.data4[1] << 16) +
                       (result.data4[2] << 8) +
                        result.data4[3];
    qint64 dataLast =  (result.data4[4] << 24) +
                       (result.data4[5] << 16) +
                       (result.data4[6] <<  8) +
                        result.data4[7];

    if(!(dataLast++))
        dataFirst++;

    result.data4[0] = uchar((dataFirst >> 24) & 0xff);
    result.data4[1] = uchar((dataFirst >> 16) & 0xff);
    result.data4[2] = uchar((dataFirst >>  8) & 0xff);
    result.data4[3] = uchar(dataFirst         & 0xff);
    result.data4[4] = uchar((dataLast  >> 24) & 0xff);
    result.data4[5] = uchar((dataLast  >> 16) & 0xff);
    result.data4[6] = uchar((dataLast  >>  8) & 0xff);
    result.data4[7] = uchar(dataLast          & 0xff);
    return result;
}

QStringList VcprojGenerator::collectSubDirs(QMakeProject *proj)
{
    QStringList subdirs;
    QStringList tmp_proj_subdirs = proj->variables()["SUBDIRS"];
    for(int x = 0; x < tmp_proj_subdirs.size(); ++x) {
        QString tmpdir = tmp_proj_subdirs.at(x);
        if(!proj->isEmpty(tmpdir + ".file")) {
            if(!proj->isEmpty(tmpdir + ".subdir"))
                warn_msg(WarnLogic, "Cannot assign both file and subdir for subdir %s",
                         tmpdir.toLatin1().constData());
            tmpdir = proj->first(tmpdir + ".file");
        } else if(!proj->isEmpty(tmpdir + ".subdir")) {
            tmpdir = proj->first(tmpdir + ".subdir");
        }
        subdirs += tmpdir;
    }
    return subdirs;
}

void VcprojGenerator::writeSubDirs(QTextStream &t)
{
    // Check if all requirements are fulfilled
    if(!project->values("QMAKE_FAILED_REQUIREMENTS").isEmpty()) {
        fprintf(stderr, "Project file not generated because all requirements not met:\n\t%s\n",
                var("QMAKE_FAILED_REQUIREMENTS").toLatin1().constData());
        return;
    }

    switch(which_dotnet_version()) {
    case NET2010:
        t << _slnHeader100;
        break;
    case NET2008:
        t << _slnHeader90;
        break;
    case NET2005:
        t << _slnHeader80;
        break;
    case NET2003:
        t << _slnHeader71;
        break;
    case NET2002:
        t << _slnHeader70;
        break;
    default:
        t << _slnHeader70;
        warn_msg(WarnLogic, "Generator: MSVC.NET: Unknown version (%d) of MSVC detected for .sln", which_dotnet_version());
        break;
    }

    QHash<QString, VcsolutionDepend*> solution_depends;
    QList<VcsolutionDepend*> solution_cleanup;

    QString oldpwd = qmake_getpwd();

    // Make sure that all temp projects are configured
    // for release so that the depends are created
    // without the debug <lib>dxxx.lib name mangling
    QStringList old_after_vars = Option::after_user_vars;
    Option::after_user_vars.append("CONFIG+=release");

    QStringList subdirs = collectSubDirs(project);
    for(int i = 0; i < subdirs.size(); ++i) {
        QString tmp = subdirs.at(i);
        QFileInfo fi(fileInfo(Option::fixPathToLocalOS(tmp, true)));
        if(fi.exists()) {
            if(fi.isDir()) {
                QString profile = tmp;
                if(!profile.endsWith(Option::dir_sep))
                    profile += Option::dir_sep;
                profile += fi.baseName() + Option::pro_ext;
                subdirs.append(profile);
            } else {
                QMakeProject tmp_proj;
                QString dir = fi.path(), fn = fi.fileName();
                if(!dir.isEmpty()) {
                    if(!qmake_setpwd(dir))
                        fprintf(stderr, "Cannot find directory: %s\n", dir.toLatin1().constData());
                }
                if(tmp_proj.read(fn)) {
                    // Check if all requirements are fulfilled
                    if(!tmp_proj.variables()["QMAKE_FAILED_REQUIREMENTS"].isEmpty()) {
                        fprintf(stderr, "Project file(%s) not added to Solution because all requirements not met:\n\t%s\n",
                                fn.toLatin1().constData(), tmp_proj.values("QMAKE_FAILED_REQUIREMENTS").join(" ").toLatin1().constData());
                        continue;
                    }
                    if(tmp_proj.first("TEMPLATE") == "vcsubdirs") {
                        foreach(const QString &tmpdir, collectSubDirs(&tmp_proj))
                            subdirs += fileFixify(tmpdir);
                    } else if(tmp_proj.first("TEMPLATE") == "vcapp" || tmp_proj.first("TEMPLATE") == "vclib") {
                        // Initialize a 'fake' project to get the correct variables
                        // and to be able to extract all the dependencies
                        Option::QMAKE_MODE old_mode = Option::qmake_mode;
                        Option::qmake_mode = Option::QMAKE_GENERATE_NOTHING;
                        QString old_output_dir = Option::output_dir;
                        Option::output_dir = QFileInfo(fileFixify(dir, qmake_getpwd(), Option::output_dir)).canonicalFilePath();
                        VcprojGenerator tmp_vcproj;
                        tmp_vcproj.setNoIO(true);
                        tmp_vcproj.setProjectFile(&tmp_proj);
                        Option::qmake_mode = old_mode;
                        Option::output_dir = old_output_dir;
                        if(Option::debug_level) {
                            debug_msg(1, "Dumping all variables:");
                            QMap<QString, QStringList> &vars = tmp_proj.variables();
                            for(QMap<QString, QStringList>::Iterator it = vars.begin();
                                it != vars.end(); ++it) {
                                if(it.key().left(1) != "." && !it.value().isEmpty())
                                    debug_msg(1, "%s: %s === %s", fn.toLatin1().constData(), it.key().toLatin1().constData(),
                                                it.value().join(" :: ").toLatin1().constData());
                            }
                        }

                        // We assume project filename is [QMAKE_PROJECT_NAME].vcproj
                        QString vcproj = unescapeFilePath(tmp_vcproj.project->first("QMAKE_PROJECT_NAME") + project->first("VCPROJ_EXTENSION"));
                        QString vcprojDir = qmake_getpwd();

                        // If file doesn't exsist, then maybe the users configuration
                        // doesn't allow it to be created. Skip to next...
                        if(!exists(vcprojDir + Option::dir_sep + vcproj)) {

                            // Try to find the directory which fits relative
                            // to the output path, which represents the shadow
                            // path in case we are shadow building
                            QStringList list = fi.path().split(QLatin1Char('/'));
                            QString tmpDir = QFileInfo(Option::output).path() + Option::dir_sep;
                            bool found = false;
                            for (int i = list.size() - 1; i >= 0; --i) {
                                QString curr;
                                for (int j = i; j < list.size(); ++j)
                                    curr += list.at(j) + Option::dir_sep;
                                if (exists(tmpDir + curr + vcproj)) {
                                    vcprojDir = QDir::cleanPath(tmpDir + curr);
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) {
                                warn_msg(WarnLogic, "Ignored (not found) '%s'", QString(vcprojDir + Option::dir_sep + vcproj).toLatin1().constData());
                                goto nextfile; // # Dirty!
                            }
                        }

                        VcsolutionDepend *newDep = new VcsolutionDepend;
                        newDep->vcprojFile = vcprojDir + Option::dir_sep + vcproj;
                        newDep->orig_target = unescapeFilePath(tmp_proj.first("QMAKE_ORIG_TARGET"));
                        newDep->target = tmp_proj.first("MSVCPROJ_TARGET").section(Option::dir_sep, -1);
                        newDep->targetType = tmp_vcproj.projectTarget;
                        newDep->uuid = tmp_proj.isEmpty("QMAKE_UUID") ? getProjectUUID(Option::fixPathToLocalOS(vcprojDir + QDir::separator() + vcproj)).toString().toUpper(): tmp_proj.first("QMAKE_UUID");

                        // We want to store it as the .lib name.
                        if(newDep->target.endsWith(".dll"))
                            newDep->target = newDep->target.left(newDep->target.length()-3) + "lib";

                        // All ActiveQt Server projects are dependent on idc.exe
                        if(tmp_proj.variables()["CONFIG"].contains("qaxserver"))
                            newDep->dependencies << "idc.exe";

                        // All extra compilers which has valid input are considered dependencies
                        const QStringList &quc = tmp_proj.variables()["QMAKE_EXTRA_COMPILERS"];
                        for(QStringList::ConstIterator it = quc.constBegin(); it != quc.constEnd(); ++it) {
                            const QStringList &invar = tmp_proj.variables().value((*it) + ".input");
                            for(QStringList::ConstIterator iit = invar.constBegin(); iit != invar.constEnd(); ++iit) {
                                const QStringList fileList = tmp_proj.variables().value(*iit);
                                if (!fileList.isEmpty()) {
                                    const QStringList &cmdsParts = tmp_proj.variables().value((*it) + ".commands");
                                    bool startOfLine = true;
                                    foreach(QString cmd, cmdsParts) {
                                        if (!startOfLine) {
                                            if (cmd.contains("\r"))
                                                startOfLine = true;
                                            continue;
                                        }
                                        if (cmd.isEmpty())
                                            continue;

                                        startOfLine = false;
                                        // Extra compiler commands might be defined in variables, so
                                        // expand them (don't care about the in/out files)
                                        cmd = tmp_vcproj.replaceExtraCompilerVariables(cmd, QStringList(), QStringList());
                                        // Pull out command based on spaces and quoting, if the
                                        // command starts with that
                                        cmd = cmd.left(cmd.indexOf(cmd.at(0) == '"' ? '"' : ' ', 1));
                                        QString dep = cmd.section('/', -1).section('\\', -1);
                                        if (!newDep->dependencies.contains(dep))
                                            newDep->dependencies << dep;
                                    }
                                }
                            }
                        }

                        // Add all unknown libs to the deps
                        QStringList where = QStringList() << "QMAKE_LIBS" << "QMAKE_LIBS_PRIVATE";
                        if(!tmp_proj.isEmpty("QMAKE_INTERNAL_PRL_LIBS"))
                            where = tmp_proj.variables()["QMAKE_INTERNAL_PRL_LIBS"];
                        for(QStringList::iterator wit = where.begin();
                            wit != where.end(); ++wit) {
                            QStringList &l = tmp_proj.variables()[(*wit)];
                            for(QStringList::Iterator it = l.begin(); it != l.end(); ++it) {
                                QString opt = (*it);
                                if(!opt.startsWith("/") &&   // Not a switch
                                    opt != newDep->target && // Not self
                                    opt != "opengl32.lib" && // We don't care about these libs
                                    opt != "glu32.lib" &&    // to make depgen alittle faster
                                    opt != "kernel32.lib" &&
                                    opt != "user32.lib" &&
                                    opt != "gdi32.lib" &&
                                    opt != "comdlg32.lib" &&
                                    opt != "advapi32.lib" &&
                                    opt != "shell32.lib" &&
                                    opt != "ole32.lib" &&
                                    opt != "oleaut32.lib" &&
                                    opt != "uuid.lib" &&
                                    opt != "imm32.lib" &&
                                    opt != "winmm.lib" &&
                                    opt != "wsock32.lib" &&
                                    opt != "ws2_32.lib" &&
                                    opt != "winspool.lib" &&
                                    opt != "delayimp.lib")
                                {
                                    newDep->dependencies << opt.section(Option::dir_sep, -1);
                                }
                            }
                        }
#ifdef DEBUG_SOLUTION_GEN
                        qDebug("Deps for %20s: [%s]", newDep->target.toLatin1().constData(), newDep->dependencies.join(" :: ").toLatin1().constData());
#endif
                        solution_cleanup.append(newDep);
                        solution_depends.insert(newDep->target, newDep);
                        t << _slnProjectBeg << _slnMSVCvcprojGUID << _slnProjectMid
                            << "\"" << newDep->orig_target << "\", \"" << newDep->vcprojFile
                            << "\", \"" << newDep->uuid << "\"";
                        t << _slnProjectEnd;
                    }
                }
nextfile:
                qmake_setpwd(oldpwd);
            }
        }
    }
    t << _slnGlobalBeg;

    QString slnConf = _slnSolutionConf;
    if (!project->isEmpty("CE_SDK") && !project->isEmpty("CE_ARCH")) {
        QString slnPlatform = QString("|") + project->values("CE_SDK").join(" ") + " (" + project->first("CE_ARCH") + ")";
        slnConf.replace(QString("|Win32"), slnPlatform);
    } else if (is64Bit) {
        slnConf.replace(QString("|Win32"), "|x64");
    }
    t << slnConf;

    t << _slnProjDepBeg;

    // Restore previous after_user_var options
    Option::after_user_vars = old_after_vars;

    // Figure out dependencies
    for(QList<VcsolutionDepend*>::Iterator it = solution_cleanup.begin(); it != solution_cleanup.end(); ++it) {
        int cnt = 0;
        for(QStringList::iterator dit = (*it)->dependencies.begin();  dit != (*it)->dependencies.end(); ++dit) {
            if(VcsolutionDepend *vc = solution_depends[*dit])
                t << "\n\t\t" << (*it)->uuid << "." << cnt++ << " = " << vc->uuid;
        }
    }
    t << _slnProjDepEnd;
    t << _slnProjConfBeg;
    for(QList<VcsolutionDepend*>::Iterator it = solution_cleanup.begin(); it != solution_cleanup.end(); ++it) {
        QString platform = is64Bit ? "x64" : "Win32";
        if (!project->isEmpty("CE_SDK") && !project->isEmpty("CE_ARCH"))
            platform = project->values("CE_SDK").join(" ") + " (" + project->first("CE_ARCH") + ")";
        t << "\n\t\t" << (*it)->uuid << QString(_slnProjDbgConfTag1).arg(platform) << platform;
        t << "\n\t\t" << (*it)->uuid << QString(_slnProjDbgConfTag2).arg(platform) << platform;
        t << "\n\t\t" << (*it)->uuid << QString(_slnProjRelConfTag1).arg(platform) << platform;
        t << "\n\t\t" << (*it)->uuid << QString(_slnProjRelConfTag2).arg(platform) << platform;
    }
    t << _slnProjConfEnd;
    t << _slnExtSections;
    t << _slnGlobalEnd;


    while (!solution_cleanup.isEmpty())
        delete solution_cleanup.takeFirst();
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

bool VcprojGenerator::hasBuiltinCompiler(const QString &file)
{
    // Source files
    for (int i = 0; i < Option::cpp_ext.count(); ++i)
        if (file.endsWith(Option::cpp_ext.at(i)))
            return true;
    for (int i = 0; i < Option::c_ext.count(); ++i)
        if (file.endsWith(Option::c_ext.at(i)))
            return true;
    if (file.endsWith(".rc")
        || file.endsWith(".idl"))
        return true;
    return false;
}

void VcprojGenerator::init()
{
    if (init_flag)
        return;
    init_flag = true;
    is64Bit = (project->first("QMAKE_TARGET.arch") == "x86_64");
    projectWriter = createProjectWriter();

    if(project->first("TEMPLATE") == "vcsubdirs") //too much work for subdirs
        return;

    debug_msg(1, "Generator: MSVC.NET: Initializing variables");

    // this should probably not be here, but I'm using it to wrap the .t files
    if (project->first("TEMPLATE") == "vcapp")
        project->values("QMAKE_APP_FLAG").append("1");
    else if (project->first("TEMPLATE") == "vclib")
        project->values("QMAKE_LIB_FLAG").append("1");
    if (project->values("QMAKESPEC").isEmpty())
        project->values("QMAKESPEC").append(qgetenv("QMAKESPEC"));

    processVars();

    project->values("QMAKE_LIBS") += escapeFilePaths(project->values("LIBS"));
    project->values("QMAKE_LIBS_PRIVATE") += escapeFilePaths(project->values("LIBS_PRIVATE"));

    if(!project->values("VERSION").isEmpty()) {
        QString version = project->values("VERSION")[0];
        int firstDot = version.indexOf(".");
        QString major = version.left(firstDot);
        QString minor = version.right(version.length() - firstDot - 1);
        minor.replace(QRegExp("\\."), "");
        project->values("QMAKE_LFLAGS").append("/VERSION:" + major + "." + minor);
    }

    MakefileGenerator::init();
    initOld();           // Currently calling old DSP code to set variables. CLEAN UP!

    // Figure out what we're trying to build
    if(project->first("TEMPLATE") == "vcapp") {
        projectTarget = Application;
    } else if(project->first("TEMPLATE") == "vclib") {
        if(project->isActiveConfig("staticlib")) {
            if (!project->values("RES_FILE").isEmpty())
                project->values("MSVCPROJ_LIBS") += escapeFilePaths(project->values("RES_FILE"));
            projectTarget = StaticLib;
        } else
            projectTarget = SharedLib;
    }

    // Setup PCH variables
    precompH = project->first("PRECOMPILED_HEADER");
    precompCPP = project->first("PRECOMPILED_SOURCE");
    usePCH = !precompH.isEmpty() && project->isActiveConfig("precompile_header");
    if (usePCH) {
        precompHFilename = fileInfo(precompH).fileName();
        // Created files
        QString origTarget = unescapeFilePath(project->first("QMAKE_ORIG_TARGET"));
        precompObj = origTarget + Option::obj_ext;
        precompPch = origTarget + ".pch";
        // Add PRECOMPILED_HEADER to HEADERS
        if (!project->values("HEADERS").contains(precompH))
            project->values("HEADERS") += precompH;
        // Return to variable pool
        project->values("PRECOMPILED_OBJECT") = QStringList(precompObj);
        project->values("PRECOMPILED_PCH")    = QStringList(precompPch);

        autogenPrecompCPP = precompCPP.isEmpty() && project->isActiveConfig("autogen_precompile_source");
        if (autogenPrecompCPP) {
            precompCPP = precompH
                + (Option::cpp_ext.count() ? Option::cpp_ext.at(0) : QLatin1String(".cpp"));
            project->values("GENERATED_SOURCES") += precompCPP;
        } else if (!precompCPP.isEmpty()) {
            project->values("SOURCES") += precompCPP;
        }
    }

    // Add all input files for a custom compiler into a map for uniqueness,
    // unless the compiler is configure as a combined stage, then use the first one
    const QStringList &quc = project->values("QMAKE_EXTRA_COMPILERS");
    for(QStringList::ConstIterator it = quc.constBegin(); it != quc.constEnd(); ++it) {
        const QStringList &invar = project->variables().value((*it) + ".input");
        const QString compiler_out = project->first((*it) + ".output");
        for(QStringList::ConstIterator iit = invar.constBegin(); iit != invar.constEnd(); ++iit) {
            QStringList fileList = project->variables().value(*iit);
            if (!fileList.isEmpty()) {
                if (project->values((*it) + ".CONFIG").indexOf("combine") != -1)
                    fileList = QStringList(fileList.first());
                for(QStringList::ConstIterator fit = fileList.constBegin(); fit != fileList.constEnd(); ++fit) {
                    QString file = (*fit);
                    if (verifyExtraCompiler((*it), file)) {
                        if (!hasBuiltinCompiler(file)) {
                            extraCompilerSources[file] += *it;
                        } else {
                            QString out = Option::fixPathToTargetOS(replaceExtraCompilerVariables(
                                            compiler_out, file, QString()), false);
                            extraCompilerSources[out] += *it;
                            extraCompilerOutputs[out] = QStringList(file); // Can only have one
                        }
                    }
                }
            }
        }
    }

#if 0 // Debugging
    Q_FOREACH(QString aKey, extraCompilerSources.keys()) {
        qDebug("Extracompilers for %s are (%s)", aKey.toLatin1().constData(), extraCompilerSources.value(aKey).join(", ").toLatin1().constData());
    }
    Q_FOREACH(QString aKey, extraCompilerOutputs.keys()) {
        qDebug("Object mapping for %s is (%s)", aKey.toLatin1().constData(), extraCompilerOutputs.value(aKey).join(", ").toLatin1().constData());
    }
    qDebug("");
#endif
}

bool VcprojGenerator::mergeBuildProject(MakefileGenerator *other)
{
    if (!other || !other->projectFile()) {
        warn_msg(WarnLogic, "VcprojGenerator: Cannot merge null project.");
        return false;
    }
    if (other->projectFile()->first("MAKEFILE_GENERATOR") != project->first("MAKEFILE_GENERATOR")) {
        warn_msg(WarnLogic, "VcprojGenerator: Cannot merge other types of projects! (ignored)");
        return false;
    }

    VcprojGenerator *otherVC = static_cast<VcprojGenerator*>(other);
    mergedProjects += otherVC;
    return true;
}

void VcprojGenerator::initProject()
{
    // Initialize XML sub elements
    // - Do this first since project elements may need
    // - to know of certain configuration options
    initConfiguration();
    initRootFiles();
    initSourceFiles();
    initHeaderFiles();
    initGeneratedFiles();
    initLexYaccFiles();
    initTranslationFiles();
    initFormFiles();
    initResourceFiles();
    initExtraCompilerOutputs();

    // Own elements -----------------------------
    vcProject.Name = unescapeFilePath(project->first("QMAKE_ORIG_TARGET"));
    switch(which_dotnet_version()) {
    case NET2010:
        vcProject.Version = "10.00";
        break;
    case NET2008:
        vcProject.Version = "9,00";
        break;
    case NET2005:
                //### using ',' because of a bug in 2005 B2
                //### VS uses '.' or ',' depending on the regional settings! Using ',' always works.
        vcProject.Version = "8,00";
        break;
    case NET2003:
        vcProject.Version = "7.10";
        break;
    case NET2002:
        vcProject.Version = "7.00";
        break;
    default:
        vcProject.Version = "7.00";
        warn_msg(WarnLogic, "Generator: MSVC.NET: Unknown version (%d) of MSVC detected for .vcproj", which_dotnet_version());
        break;
    }

    vcProject.Keyword = project->first("VCPROJ_KEYWORD");
    if (project->isEmpty("CE_SDK") || project->isEmpty("CE_ARCH")) {
        vcProject.PlatformName = (is64Bit ? "x64" : "Win32");
    } else {
        vcProject.PlatformName = project->values("CE_SDK").join(" ") + " (" + project->first("CE_ARCH") + ")";
    }
    // These are not used by Qt, but may be used by customers
    vcProject.SccProjectName = project->first("SCCPROJECTNAME");
    vcProject.SccLocalPath = project->first("SCCLOCALPATH");
    vcProject.flat_files = project->isActiveConfig("flat");
}

void VcprojGenerator::initConfiguration()
{
    // Initialize XML sub elements
    // - Do this first since main configuration elements may need
    // - to know of certain compiler/linker options
    VCConfiguration &conf = vcProject.Configuration;
    conf.CompilerVersion = which_dotnet_version();

    initCompilerTool();

    // Only on configuration per build
    bool isDebug = project->isActiveConfig("debug");

    if(projectTarget == StaticLib)
        initLibrarianTool();
    else {
        conf.linker.GenerateDebugInformation = isDebug ? _True : _False;
        initLinkerTool();
    }
    initResourceTool();
    initIDLTool();

    // Own elements -----------------------------
    QString temp = project->first("BuildBrowserInformation");
    switch (projectTarget) {
    case SharedLib:
        conf.ConfigurationType = typeDynamicLibrary;
        break;
    case StaticLib:
        conf.ConfigurationType = typeStaticLibrary;
        break;
    case Application:
    default:
        conf.ConfigurationType = typeApplication;
        break;
    }

    conf.OutputDirectory = project->first("DESTDIR");
    if (conf.OutputDirectory.isEmpty())
        conf.OutputDirectory = ".\\";
    if (!conf.OutputDirectory.endsWith("\\"))
        conf.OutputDirectory += '\\';
    if (conf.CompilerVersion >= NET2010) {
        // The target name could have been changed.
        conf.PrimaryOutput = project->first("TARGET");
        if ( !conf.PrimaryOutput.isEmpty() && !project->first("TARGET_VERSION_EXT").isEmpty() && project->isActiveConfig("shared"))
            conf.PrimaryOutput.append(project->first("TARGET_VERSION_EXT"));
    }

    conf.Name = project->values("BUILD_NAME").join(" ");
    if (conf.Name.isEmpty())
        conf.Name = isDebug ? "Debug" : "Release";
    conf.ConfigurationName = conf.Name;
    if (project->isEmpty("CE_SDK") || project->isEmpty("CE_ARCH")) {
        conf.Name += (is64Bit ? "|x64" : "|Win32");
    } else {
        conf.Name += "|" + project->values("CE_SDK").join(" ") + " (" + project->first("CE_ARCH") + ")";
    }
    conf.ATLMinimizesCRunTimeLibraryUsage = (project->first("ATLMinimizesCRunTimeLibraryUsage").isEmpty() ? _False : _True);
    conf.BuildBrowserInformation = triState(temp.isEmpty() ? (short)unset : temp.toShort());
    temp = project->first("CharacterSet");
    conf.CharacterSet = charSet(temp.isEmpty() ? (short)charSetNotSet : temp.toShort());
    conf.DeleteExtensionsOnClean = project->first("DeleteExtensionsOnClean");
    conf.ImportLibrary = conf.linker.ImportLibrary;
    conf.IntermediateDirectory = project->first("OBJECTS_DIR");
    conf.WholeProgramOptimization = conf.compiler.WholeProgramOptimization;
    temp = project->first("UseOfATL");
    if(!temp.isEmpty())
        conf.UseOfATL = useOfATL(temp.toShort());
    temp = project->first("UseOfMfc");
    if(!temp.isEmpty())
        conf.UseOfMfc = useOfMfc(temp.toShort());

    // Configuration does not need parameters from
    // these sub XML items;
    initCustomBuildTool();
    initPreBuildEventTools();
    initPostBuildEventTools();
    // Only deploy for CE projects
    if (!project->isEmpty("CE_SDK") && !project->isEmpty("CE_ARCH"))
        initDeploymentTool();
    initPreLinkEventTools();

    // Set definite values in both configurations
    if (isDebug) {
        conf.compiler.PreprocessorDefinitions.removeAll("NDEBUG");
    } else {
        conf.compiler.PreprocessorDefinitions += "NDEBUG";
    }
}

void VcprojGenerator::initCompilerTool()
{
    QString placement = project->first("OBJECTS_DIR");
    if(placement.isEmpty())
        placement = ".\\";

    VCConfiguration &conf = vcProject.Configuration;
    if (conf.CompilerVersion >= NET2010) {
        // adjust compiler tool defaults for VS 2010 and above
        conf.compiler.Optimization = optimizeDisabled;
    }
    conf.compiler.AssemblerListingLocation = placement ;
    conf.compiler.ProgramDataBaseFileName = ".\\" ;
    conf.compiler.ObjectFile = placement ;
    conf.compiler.ExceptionHandling = ehNone;
    // PCH
    if (usePCH) {
        conf.compiler.UsePrecompiledHeader     = pchUseUsingSpecific;
        conf.compiler.PrecompiledHeaderFile    = "$(IntDir)\\" + precompPch;
        conf.compiler.PrecompiledHeaderThrough = project->first("PRECOMPILED_HEADER");
        conf.compiler.ForcedIncludeFiles       = project->values("PRECOMPILED_HEADER");

        if (conf.CompilerVersion <= NET2003) {
            // Minimal build option triggers an Internal Compiler Error
            // when used in conjunction with /FI and /Yu, so remove it
            // ### work-around for a VS 2003 bug. Move to some prf file or remove completely.
            project->values("QMAKE_CFLAGS_DEBUG").removeAll("-Gm");
            project->values("QMAKE_CFLAGS_DEBUG").removeAll("/Gm");
            project->values("QMAKE_CXXFLAGS_DEBUG").removeAll("-Gm");
            project->values("QMAKE_CXXFLAGS_DEBUG").removeAll("/Gm");
        }
    }

    conf.compiler.parseOptions(project->values("QMAKE_CXXFLAGS"));

    if (project->isActiveConfig("windows"))
        conf.compiler.PreprocessorDefinitions += "_WINDOWS";
    else if (project->isActiveConfig("console"))
        conf.compiler.PreprocessorDefinitions += "_CONSOLE";

    conf.compiler.PreprocessorDefinitions += project->values("DEFINES");
    conf.compiler.PreprocessorDefinitions += project->values("PRL_EXPORT_DEFINES");
    conf.compiler.parseOptions(project->values("MSVCPROJ_INCPATH"));
}

void VcprojGenerator::initLibrarianTool()
{
    VCConfiguration &conf = vcProject.Configuration;
    conf.librarian.OutputFile = "$(OutDir)\\";
    conf.librarian.OutputFile += project->first("MSVCPROJ_TARGET");
    conf.librarian.AdditionalOptions += project->values("QMAKE_LIBFLAGS");
}

void VcprojGenerator::initLinkerTool()
{
    findLibraries(); // Need to add the highest version of the libs
    VCConfiguration &conf = vcProject.Configuration;
    conf.linker.parseOptions(project->values("QMAKE_LFLAGS"));

    foreach (const QString &libDir, project->values("QMAKE_LIBDIR")) {
        if (libDir.startsWith("/LIBPATH:"))
            conf.linker.AdditionalLibraryDirectories += libDir.mid(9);
        else
            conf.linker.AdditionalLibraryDirectories += libDir;
    }

    if (!project->values("DEF_FILE").isEmpty())
        conf.linker.ModuleDefinitionFile = project->first("DEF_FILE");

    foreach(QString libs, project->values("MSVCPROJ_LIBS")) {
        if (libs.left(9).toUpper() == "/LIBPATH:") {
            QStringList l = QStringList(libs);
            conf.linker.parseOptions(l);
        } else {
            conf.linker.AdditionalDependencies += libs;
        }
    }

    conf.linker.OutputFile = "$(OutDir)\\";
    conf.linker.OutputFile += project->first("MSVCPROJ_TARGET");

    if(project->isActiveConfig("dll")){
        conf.linker.parseOptions(project->values("QMAKE_LFLAGS_QT_DLL"));
    }
}

void VcprojGenerator::initResourceTool()
{
    VCConfiguration &conf = vcProject.Configuration;
    conf.resource.PreprocessorDefinitions = conf.compiler.PreprocessorDefinitions;

    // We need to add _DEBUG for the debug version of the project, since the normal compiler defines
    // do not contain it. (The compiler defines this symbol automatically, which is wy we don't need
    // to add it for the compiler) However, the resource tool does not do this.
    if(project->isActiveConfig("debug"))
        conf.resource.PreprocessorDefinitions += "_DEBUG";
    if(project->isActiveConfig("staticlib"))
        conf.resource.ResourceOutputFileName = "$(OutDir)\\$(InputName).res";
}

void VcprojGenerator::initIDLTool()
{
}

void VcprojGenerator::initCustomBuildTool()
{
}

void VcprojGenerator::initPreBuildEventTools()
{
}

void VcprojGenerator::initPostBuildEventTools()
{
    VCConfiguration &conf = vcProject.Configuration;
    if (!project->values("QMAKE_POST_LINK").isEmpty()) {
        QStringList cmdline = VCToolBase::fixCommandLine(var("QMAKE_POST_LINK"));
        conf.postBuild.CommandLine = cmdline;
        conf.postBuild.Description = cmdline.join(QLatin1String("\r\n"));
        conf.postBuild.ExcludedFromBuild = _False;
    }

    QString signature = !project->isEmpty("SIGNATURE_FILE") ? var("SIGNATURE_FILE") : var("DEFAULT_SIGNATURE");
    bool useSignature = !signature.isEmpty() && !project->isActiveConfig("staticlib") &&
                        !project->isEmpty("CE_SDK") && !project->isEmpty("CE_ARCH");
    if (useSignature) {
        conf.postBuild.CommandLine.prepend(
                QLatin1String("signtool sign /F ") + signature + QLatin1String(" \"$(TargetPath)\""));
        conf.postBuild.ExcludedFromBuild = _False;
    }

    if (!project->values("MSVCPROJ_COPY_DLL").isEmpty()) {
        conf.postBuild.Description += var("MSVCPROJ_COPY_DLL_DESC");
        conf.postBuild.CommandLine += var("MSVCPROJ_COPY_DLL");
        conf.postBuild.ExcludedFromBuild = _False;
    }
}

void VcprojGenerator::initDeploymentTool()
{
    VCConfiguration &conf = vcProject.Configuration;
    QString targetPath = project->values("deploy.path").join(" ");
    if (targetPath.isEmpty())
        targetPath = QString("%CSIDL_PROGRAM_FILES%\\") + project->first("TARGET");
    if (targetPath.endsWith("/") || targetPath.endsWith("\\"))
        targetPath.chop(1);

    // Only deploy Qt libs for shared build
    if (!project->values("QMAKE_QT_DLL").isEmpty()) {
        QStringList& arg = project->values("MSVCPROJ_LIBS");
        for (QStringList::ConstIterator it = arg.constBegin(); it != arg.constEnd(); ++it) {
            if (it->contains(project->first("QMAKE_LIBDIR"))) {
                QString dllName = *it;

                if (dllName.contains(QLatin1String("QAxContainer"))
                    || dllName.contains(QLatin1String("qtmain"))
                    || dllName.contains(QLatin1String("QtUiTools")))
                    continue;
                dllName.replace(QLatin1String(".lib") , QLatin1String(".dll"));
                QFileInfo info(dllName);
                conf.deployment.AdditionalFiles += info.fileName()
                                                + "|" + QDir::toNativeSeparators(info.absolutePath())
                                                + "|" + targetPath
                                                + "|0;";
            }
        }
    }

    // C-runtime deployment
    QString runtime = project->values("QT_CE_C_RUNTIME").join(QLatin1String(" "));
    if (!runtime.isEmpty() && (runtime != QLatin1String("no"))) {
        QString runtimeVersion = QLatin1String("msvcr");
        QString mkspec = project->first("QMAKESPEC");
        // If no .qmake.cache has been found, we fallback to the original mkspec
        if (mkspec.isEmpty())
            mkspec = project->first("QMAKESPEC_ORIGINAL");

        if (!mkspec.isEmpty()) {
            if (mkspec.endsWith("2008"))
                runtimeVersion.append("90");
            else
                runtimeVersion.append("80");
            if (project->isActiveConfig("debug"))
                runtimeVersion.append("d");
            runtimeVersion.append(".dll");

            if (runtime == "yes") {
                // Auto-find C-runtime
                QString vcInstallDir = qgetenv("VCINSTALLDIR");
                if (!vcInstallDir.isEmpty()) {
                    vcInstallDir += "\\ce\\dll\\";
                    vcInstallDir += project->values("CE_ARCH").join(QLatin1String(" "));
                    if (!QFileInfo(vcInstallDir + QDir::separator() + runtimeVersion).exists())
                        runtime.clear();
                    else
                        runtime = vcInstallDir;
                }
            }
        }

        if (!runtime.isEmpty() && runtime != QLatin1String("yes")) {
            conf.deployment.AdditionalFiles += runtimeVersion
                                            + "|" + QDir::toNativeSeparators(runtime)
                                            + "|" + targetPath
                                            + "|0;";
        }
    }

    // foreach item in DEPLOYMENT
    foreach(QString item, project->values("DEPLOYMENT")) {
        // get item.path
        QString devicePath = project->first(item + ".path");
        if (devicePath.isEmpty())
            devicePath = targetPath;
        // check if item.path is relative (! either /,\ or %)
        if (!(devicePath.at(0) == QLatin1Char('/')
            || devicePath.at(0) == QLatin1Char('\\')
            || devicePath.at(0) == QLatin1Char('%'))) {
            // create output path
            devicePath = Option::fixPathToLocalOS(QDir::cleanPath(targetPath + QLatin1Char('\\') + devicePath));
        }
        // foreach d in item.sources
        // ### Qt 5: remove .sources, inconsistent with INSTALLS
        foreach(QString source, project->values(item + ".sources") + project->values(item + ".files")) {
            QString itemDevicePath = devicePath;
            source = Option::fixPathToLocalOS(source);
            QString nameFilter;
            QFileInfo info(source);
            QString searchPath;
            if (info.isDir()) {
                nameFilter = QLatin1String("*");
                itemDevicePath += "\\" + info.fileName();
                searchPath = info.absoluteFilePath();
            } else {
                nameFilter = source.split('\\').last();
                searchPath = info.absolutePath();
            }

            int pathSize = searchPath.size();
            QDirIterator iterator(searchPath, QStringList() << nameFilter
                                  , QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks
                                  , QDirIterator::Subdirectories);
            // foreach dirIterator-entry in d
            while(iterator.hasNext()) {
                iterator.next();
                QString absoluteItemPath = Option::fixPathToLocalOS(QFileInfo(iterator.filePath()).absolutePath());
                // Identify if it is just another subdir
                int diffSize = absoluteItemPath.size() - pathSize;
                // write out rules
                conf.deployment.AdditionalFiles += iterator.fileName()
                    + "|" + absoluteItemPath
                    + "|" + itemDevicePath + (diffSize ? (absoluteItemPath.right(diffSize)) : QLatin1String(""))
                    + "|0;";
            }
        }
    }
}

void VcprojGenerator::initPreLinkEventTools()
{
    VCConfiguration &conf = vcProject.Configuration;
    if(!project->values("QMAKE_PRE_LINK").isEmpty()) {
        QStringList cmdline = VCToolBase::fixCommandLine(var("QMAKE_PRE_LINK"));
        conf.preLink.CommandLine = cmdline;
        conf.preLink.Description = cmdline.join(QLatin1String("\r\n"));
        conf.preLink.ExcludedFromBuild = _False;
    }
}

void VcprojGenerator::initRootFiles()
{
    // Note: Root files do _not_ have any filter name, filter nor GUID!
    vcProject.RootFiles.addFiles(project->values("RC_FILE"));

    vcProject.RootFiles.Project = this;
    vcProject.RootFiles.Config = &(vcProject.Configuration);
    vcProject.RootFiles.CustomBuild = none;
}

void VcprojGenerator::initSourceFiles()
{
    vcProject.SourceFiles.Name = "Source Files";
    vcProject.SourceFiles.Filter = "cpp;c;cxx;def;odl;idl;hpj;bat;asm;asmx";
    vcProject.SourceFiles.Guid = _GUIDSourceFiles;

    vcProject.SourceFiles.addFiles(project->values("SOURCES"));

    vcProject.SourceFiles.Project = this;
    vcProject.SourceFiles.Config = &(vcProject.Configuration);
    vcProject.SourceFiles.CustomBuild = none;
}

void VcprojGenerator::initHeaderFiles()
{
    vcProject.HeaderFiles.Name = "Header Files";
    vcProject.HeaderFiles.Filter = "h;hpp;hxx;hm;inl;inc;xsd";
    vcProject.HeaderFiles.Guid = _GUIDHeaderFiles;

    vcProject.HeaderFiles.addFiles(project->values("HEADERS"));
    if (usePCH) // Generated PCH cpp file
        vcProject.HeaderFiles.addFile(precompH);

    vcProject.HeaderFiles.Project = this;
    vcProject.HeaderFiles.Config = &(vcProject.Configuration);
//    vcProject.HeaderFiles.CustomBuild = mocHdr;
//    addMocArguments(vcProject.HeaderFiles);
}

void VcprojGenerator::initGeneratedFiles()
{
    vcProject.GeneratedFiles.Name = "Generated Files";
    vcProject.GeneratedFiles.Filter = "cpp;c;cxx;moc;h;def;odl;idl;res;";
    vcProject.GeneratedFiles.Guid = _GUIDGeneratedFiles;

    // ### These cannot have CustomBuild (mocSrc)!!
    vcProject.GeneratedFiles.addFiles(project->values("GENERATED_SOURCES"));
    vcProject.GeneratedFiles.addFiles(project->values("GENERATED_FILES"));
    vcProject.GeneratedFiles.addFiles(project->values("IDLSOURCES"));
    vcProject.GeneratedFiles.addFiles(project->values("RES_FILE"));
    vcProject.GeneratedFiles.addFiles(project->values("QMAKE_IMAGE_COLLECTION"));   // compat
    if(!extraCompilerOutputs.isEmpty())
        vcProject.GeneratedFiles.addFiles(extraCompilerOutputs.keys());

    vcProject.GeneratedFiles.Project = this;
    vcProject.GeneratedFiles.Config = &(vcProject.Configuration);
//    vcProject.GeneratedFiles.CustomBuild = mocSrc;
}

void VcprojGenerator::initLexYaccFiles()
{
    vcProject.LexYaccFiles.Name = "Lex / Yacc Files";
    vcProject.LexYaccFiles.ParseFiles = _False;
    vcProject.LexYaccFiles.Filter = "l;y";
    vcProject.LexYaccFiles.Guid = _GUIDLexYaccFiles;

    vcProject.LexYaccFiles.addFiles(project->values("LEXSOURCES"));
    vcProject.LexYaccFiles.addFiles(project->values("YACCSOURCES"));

    vcProject.LexYaccFiles.Project = this;
    vcProject.LexYaccFiles.Config = &(vcProject.Configuration);
    vcProject.LexYaccFiles.CustomBuild = lexyacc;
}

void VcprojGenerator::initTranslationFiles()
{
    vcProject.TranslationFiles.Name = "Translation Files";
    vcProject.TranslationFiles.ParseFiles = _False;
    vcProject.TranslationFiles.Filter = "ts;xlf";
    vcProject.TranslationFiles.Guid = _GUIDTranslationFiles;

    vcProject.TranslationFiles.addFiles(project->values("TRANSLATIONS"));

    vcProject.TranslationFiles.Project = this;
    vcProject.TranslationFiles.Config = &(vcProject.Configuration);
    vcProject.TranslationFiles.CustomBuild = none;
}

void VcprojGenerator::initFormFiles()
{
    vcProject.FormFiles.Name = "Form Files";
    vcProject.FormFiles.ParseFiles = _False;
    vcProject.FormFiles.Filter = "ui";
    vcProject.FormFiles.Guid = _GUIDFormFiles;

    vcProject.FormFiles.addFiles(project->values("FORMS"));
    vcProject.FormFiles.addFiles(project->values("FORMS3"));

    vcProject.FormFiles.Project = this;
    vcProject.FormFiles.Config = &(vcProject.Configuration);
    vcProject.FormFiles.CustomBuild = none;
}

void VcprojGenerator::initResourceFiles()
{
    vcProject.ResourceFiles.Name = "Resource Files";
    vcProject.ResourceFiles.ParseFiles = _False;
    vcProject.ResourceFiles.Filter = "qrc;*"; //"rc;ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe;resx;ts;xlf;qrc";
    vcProject.ResourceFiles.Guid = _GUIDResourceFiles;

    // Bad hack, please look away -------------------------------------
    QString rcc_dep_cmd = project->values("rcc.depend_command").join(" ");
    if(!rcc_dep_cmd.isEmpty()) {
        QStringList qrc_files = project->values("RESOURCES");
        QStringList deps;
        if(!qrc_files.isEmpty()) {
            for (int i = 0; i < qrc_files.count(); ++i) {
                char buff[256];
                QString dep_cmd = replaceExtraCompilerVariables(rcc_dep_cmd, qrc_files.at(i),"");

                dep_cmd = Option::fixPathToLocalOS(dep_cmd, true, false);
                if(canExecute(dep_cmd)) {
                    dep_cmd.prepend(QLatin1String("cd ")
                                    + escapeFilePath(Option::fixPathToLocalOS(Option::output_dir, false))
                                    + QLatin1String(" && "));
                    if(FILE *proc = QT_POPEN(dep_cmd.toLatin1().constData(), "r")) {
                        QString indeps;
                        while(!feof(proc)) {
                            int read_in = (int)fread(buff, 1, 255, proc);
                            if(!read_in)
                                break;
                            indeps += QByteArray(buff, read_in);
                        }
                        QT_PCLOSE(proc);
                        if(!indeps.isEmpty())
                            deps += fileFixify(indeps.replace('\n', ' ').simplified().split(' '),
                                               QString(), Option::output_dir);
                    }
                }
            }
            vcProject.ResourceFiles.addFiles(deps);
        }
    }
    // You may look again --------------------------------------------

    vcProject.ResourceFiles.addFiles(project->values("RESOURCES"));
    vcProject.ResourceFiles.addFiles(project->values("IMAGES"));

    vcProject.ResourceFiles.Project = this;
    vcProject.ResourceFiles.Config = &(vcProject.Configuration);
    vcProject.ResourceFiles.CustomBuild = none;
}

void VcprojGenerator::initExtraCompilerOutputs()
{
    QStringList otherFilters;
    otherFilters << "FORMS"
                 << "FORMS3"
                 << "GENERATED_FILES"
                 << "GENERATED_SOURCES"
                 << "HEADERS"
                 << "IDLSOURCES"
                 << "IMAGES"
                 << "LEXSOURCES"
                 << "QMAKE_IMAGE_COLLECTION"
                 << "RC_FILE"
                 << "RESOURCES"
                 << "RES_FILE"
                 << "SOURCES"
                 << "TRANSLATIONS"
                 << "YACCSOURCES";
    const QStringList &quc = project->values("QMAKE_EXTRA_COMPILERS");
    for(QStringList::ConstIterator it = quc.begin(); it != quc.end(); ++it) {
        QString extracompilerName = project->first((*it) + ".name");
        if (extracompilerName.isEmpty())
            extracompilerName = (*it);

        // Create an extra compiler filter and add the files
        VCFilter extraCompile;
        extraCompile.Name = extracompilerName;
        extraCompile.ParseFiles = _False;
        extraCompile.Filter = "";
        extraCompile.Guid = QString(_GUIDExtraCompilerFiles) + "-" + (*it);


        // If the extra compiler has a variable_out set the output file
        // is added to an other file list, and does not need its own..
        bool addOnInput = hasBuiltinCompiler(project->first((*it) + ".output"));
        QString tmp_other_out = project->first((*it) + ".variable_out");
        if (!tmp_other_out.isEmpty() && !addOnInput)
            continue;

        if (!addOnInput) {
            QString tmp_out = project->first((*it) + ".output");
            if (project->values((*it) + ".CONFIG").indexOf("combine") != -1) {
                // Combined output, only one file result
                extraCompile.addFile(
                    Option::fixPathToTargetOS(replaceExtraCompilerVariables(tmp_out, QString(), QString()), false));
            } else {
                // One output file per input
                QStringList tmp_in = project->values(project->first((*it) + ".input"));
                for (int i = 0; i < tmp_in.count(); ++i) {
                    const QString &filename = tmp_in.at(i);
                    if (extraCompilerSources.contains(filename))
                        extraCompile.addFile(
                            Option::fixPathToTargetOS(replaceExtraCompilerVariables(filename, tmp_out, QString()), false));
                }
            }
        } else {
            // In this case we the outputs have a built-in compiler, so we cannot add the custom
            // build steps there. So, we turn it around and add it to the input files instead,
            // provided that the input file variable is not handled already (those in otherFilters
            // are handled, so we avoid them).
            QStringList inputVars = project->values((*it) + ".input");
            foreach(QString inputVar, inputVars) {
                if (!otherFilters.contains(inputVar)) {
                    QStringList tmp_in = project->values(inputVar);
                    for (int i = 0; i < tmp_in.count(); ++i) {
                        const QString &filename = tmp_in.at(i);
                        if (extraCompilerSources.contains(filename))
                            extraCompile.addFile(
                                Option::fixPathToTargetOS(replaceExtraCompilerVariables(filename, QString(), QString()), false));
                    }
                }
            }
        }
        extraCompile.Project = this;
        extraCompile.Config = &(vcProject.Configuration);
        extraCompile.CustomBuild = none;

        vcProject.ExtraCompilersFiles.append(extraCompile);
    }
}

void VcprojGenerator::initOld()
{
    // $$QMAKE.. -> $$MSVCPROJ.. -------------------------------------
    project->values("MSVCPROJ_LIBS") += project->values("QMAKE_LIBS");
    project->values("MSVCPROJ_LIBS") += project->values("QMAKE_LIBS_PRIVATE");
    QStringList &incs = project->values("INCLUDEPATH");
    for(QStringList::Iterator incit = incs.begin(); incit != incs.end(); ++incit) {
        QString inc = (*incit);
        if (!inc.startsWith('"') && !inc.endsWith('"'))
            inc = QString("\"%1\"").arg(inc); // Quote all paths if not quoted already
        project->values("MSVCPROJ_INCPATH").append("-I" + inc);
    }
    project->values("MSVCPROJ_INCPATH").append("-I" + specdir());

    QString dest;
    project->values("MSVCPROJ_TARGET") = QStringList(project->first("TARGET"));
    Option::fixPathToTargetOS(project->first("TARGET"));
    dest = project->first("TARGET") + project->first("TARGET_EXT");
    project->values("MSVCPROJ_TARGET") = QStringList(dest);

    // DLL COPY ------------------------------------------------------
    if(project->isActiveConfig("dll") && !project->values("DLLDESTDIR").isEmpty()) {
        QStringList dlldirs = project->values("DLLDESTDIR");
        QString copydll("");
        QStringList::Iterator dlldir;
        for(dlldir = dlldirs.begin(); dlldir != dlldirs.end(); ++dlldir) {
            if(!copydll.isEmpty())
                copydll += " && ";
            copydll += "copy  \"$(TargetPath)\" \"" + *dlldir + "\"";
        }

        QString deststr("Copy " + dest + " to ");
        for(dlldir = dlldirs.begin(); dlldir != dlldirs.end();) {
            deststr += *dlldir;
            ++dlldir;
            if(dlldir != dlldirs.end())
                deststr += ", ";
        }

        project->values("MSVCPROJ_COPY_DLL").append(copydll);
        project->values("MSVCPROJ_COPY_DLL_DESC").append(deststr);
    }

    project->values("QMAKE_INTERNAL_PRL_LIBS") << "MSVCPROJ_LIBS";

    // Verbose output if "-d -d"...
    outputVariables();
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

VCProjectWriter *VcprojGenerator::createProjectWriter()
{
    return new VCProjectWriter;
}

QString VcprojGenerator::replaceExtraCompilerVariables(const QString &var, const QStringList &in, const QStringList &out)
{
    QString ret = MakefileGenerator::replaceExtraCompilerVariables(var, in, out);

    QStringList &defines = project->values("VCPROJ_MAKEFILE_DEFINES");
    if(defines.isEmpty())
        defines.append(varGlue("PRL_EXPORT_DEFINES"," -D"," -D","") +
                       varGlue("DEFINES"," -D"," -D",""));
    ret.replace("$(DEFINES)", defines.first());

    QStringList &incpath = project->values("VCPROJ_MAKEFILE_INCPATH");
    if(incpath.isEmpty() && !this->var("MSVCPROJ_INCPATH").isEmpty())
        incpath.append(this->var("MSVCPROJ_INCPATH"));
    ret.replace("$(INCPATH)", incpath.join(" "));

    return ret;
}

bool VcprojGenerator::openOutput(QFile &file, const QString &/*build*/) const
{
    QString outdir;
    if(!file.fileName().isEmpty()) {
        QFileInfo fi(fileInfo(file.fileName()));
        if(fi.isDir())
            outdir = file.fileName() + QDir::separator();
    }
    if(!outdir.isEmpty() || file.fileName().isEmpty()) {
        QString ext = project->first("VCPROJ_EXTENSION");
        if(project->first("TEMPLATE") == "vcsubdirs")
            ext = project->first("VCSOLUTION_EXTENSION");
        QString outputName = unescapeFilePath(project->first("TARGET"));
        if (!project->first("MAKEFILE").isEmpty())
            outputName = project->first("MAKEFILE");
        file.setFileName(outdir + outputName + ext);
    }
    return Win32MakefileGenerator::openOutput(file, QString());
}

QString VcprojGenerator::fixFilename(QString ofile) const
{
    ofile = Option::fixPathToLocalOS(ofile);
    int slashfind = ofile.lastIndexOf(Option::dir_sep);
    if(slashfind == -1) {
        ofile = ofile.replace('-', '_');
    } else {
        int hyphenfind = ofile.indexOf('-', slashfind);
        while (hyphenfind != -1 && slashfind < hyphenfind) {
            ofile = ofile.replace(hyphenfind, 1, '_');
            hyphenfind = ofile.indexOf('-', hyphenfind + 1);
        }
    }
    return ofile;
}

QString VcprojGenerator::findTemplate(QString file)
{
    QString ret;
    if(!exists((ret = file)) &&
       !exists((ret = QString(Option::mkfile::qmakespec + "/" + file))) &&
       !exists((ret = QString(QLibraryInfo::location(QLibraryInfo::DataPath) + "/win32-msvc.net/" + file))) &&
       !exists((ret = QString(QLibraryInfo::location(QLibraryInfo::DataPath) + "/win32-msvc2002/" + file))) &&
       !exists((ret = QString(QLibraryInfo::location(QLibraryInfo::DataPath) + "/win32-msvc2003/" + file))) &&
       !exists((ret = QString(QLibraryInfo::location(QLibraryInfo::DataPath) + "/win32-msvc2005/" + file))) &&
       !exists((ret = QString(QLibraryInfo::location(QLibraryInfo::DataPath) + "/win32-msvc2008/" + file))))
        return "";
    debug_msg(1, "Generator: MSVC.NET: Found template \'%s\'", ret.toLatin1().constData());
    return ret;
}

void VcprojGenerator::outputVariables()
{
#if 0
    qDebug("Generator: MSVC.NET: List of current variables:");
    for(QMap<QString, QStringList>::ConstIterator it = project->variables().begin(); it != project->variables().end(); ++it)
        qDebug("Generator: MSVC.NET: %s => %s", qPrintable(it.key()), qPrintable(it.value().join(" | ")));
#endif
}

QT_END_NAMESPACE
