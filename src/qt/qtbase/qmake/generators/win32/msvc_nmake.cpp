/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "msvc_nmake.h"
#include "option.h"
#include "cesdkhandler.h"

#include <qregexp.h>
#include <qhash.h>
#include <qdir.h>

#include <windows/registry_p.h>

#include <time.h>

QT_BEGIN_NAMESPACE

static QString nmakePathList(const QStringList &list)
{
    QStringList pathList;
    foreach (const QString &path, list)
        pathList.append(QDir::cleanPath(path));

    return QDir::toNativeSeparators(pathList.join(QLatin1Char(';')))
            .replace('#', QStringLiteral("^#")).replace('$', QStringLiteral("$$"));
}

NmakeMakefileGenerator::NmakeMakefileGenerator() : Win32MakefileGenerator(), init_flag(false), usePCH(false)
{

}

bool
NmakeMakefileGenerator::writeMakefile(QTextStream &t)
{
    writeHeader(t);
    if (writeDummyMakefile(t))
        return true;

    if(project->first("TEMPLATE") == "app" ||
       project->first("TEMPLATE") == "lib" ||
       project->first("TEMPLATE") == "aux") {
#if 0
        if(Option::mkfile::do_stub_makefile)
            return MakefileGenerator::writeStubMakefile(t);
#endif
        if (!project->isHostBuild()) {
            const ProValueMap &variables = project->variables();
            if (project->isActiveConfig("wince")) {
                CeSdkHandler sdkhandler;
                sdkhandler.parse();
                const QString sdkName = variables["CE_SDK"].join(' ')
                                        + " (" + variables["CE_ARCH"].join(' ') + ")";
                const QList<CeSdkInfo> sdkList = sdkhandler.listAll();
                CeSdkInfo sdk;
                foreach (const CeSdkInfo &info, sdkList) {
                    if (info.name().compare(sdkName, Qt::CaseInsensitive ) == 0) {
                        sdk = info;
                        break;
                    }
                }
                if (sdk.isValid()) {
                    t << "\nINCLUDE = " << sdk.includePath();
                    t << "\nLIB = " << sdk.libPath();
                    t << "\nPATH = " << sdk.binPath() << "\n";
                } else {
                    QStringList sdkStringList;
                    foreach (const CeSdkInfo &info, sdkList)
                        sdkStringList << info.name();

                    fprintf(stderr, "Failed to find Windows CE SDK matching %s, found: %s\n"
                                    "SDK needs to be specified in mkspec (using: %s/qmake.conf)\n"
                                    "SDK name needs to match the following format: CE_SDK (CE_ARCH)\n",
                            qPrintable(sdkName), qPrintable(sdkStringList.join(", ")),
                            qPrintable(variables["QMAKESPEC"].first().toQString()));
                    return false;
                }
            } else if (project->isActiveConfig(QStringLiteral("winrt"))) {
                QString arch = project->first("VCPROJ_ARCH").toQString().toLower();
                QString compiler;
                QString compilerArch;
                if (arch == QStringLiteral("arm")) {
                    compiler = QStringLiteral("x86_arm");
                    compilerArch = QStringLiteral("arm");
                } else if (arch == QStringLiteral("x64")) {
                    const ProStringList hostArch = project->values("QMAKE_TARGET.arch");
                    if (hostArch.contains("x86_64"))
                        compiler = QStringLiteral("amd64");
                    else
                        compiler = QStringLiteral("x86_amd64");
                    compilerArch = QStringLiteral("amd64");
                } else {
                    arch = QStringLiteral("x86");
                }

                const QString msvcVer = project->first("MSVC_VER").toQString();
                if (msvcVer.isEmpty()) {
                    fprintf(stderr, "Mkspec does not specify MSVC_VER. Cannot continue.\n");
                    return false;
                }
                const QString winsdkVer = project->first("WINSDK_VER").toQString();
                if (winsdkVer.isEmpty()) {
                    fprintf(stderr, "Mkspec does not specify WINSDK_VER. Cannot continue.\n");
                    return false;
                }
                const QString targetVer = project->first("WINTARGET_VER").toQString();
                if (targetVer.isEmpty()) {
                    fprintf(stderr, "Mkspec does not specify WINTARGET_VER. Cannot continue.\n");
                    return false;
                }

                const bool isPhone = project->isActiveConfig(QStringLiteral("winphone"));
#ifdef Q_OS_WIN
                QString regKeyPrefix;
#if !defined(Q_OS_WIN64) && _WIN32_WINNT >= 0x0501
                BOOL isWow64;
                IsWow64Process(GetCurrentProcess(), &isWow64);
                if (!isWow64)
                    regKeyPrefix = QStringLiteral("Software\\");
                else
#endif
                    regKeyPrefix = QStringLiteral("Software\\Wow6432Node\\");

                QString regKey = regKeyPrefix + QStringLiteral("Microsoft\\VisualStudio\\") + msvcVer + ("\\Setup\\VC\\ProductDir");
                const QString vcInstallDir = qt_readRegistryKey(HKEY_LOCAL_MACHINE, regKey);
                if (vcInstallDir.isEmpty()) {
                    fprintf(stderr, "Failed to find the Visual Studio installation directory.\n");
                    return false;
                }

                regKey = regKeyPrefix
                        + (isPhone ? QStringLiteral("Microsoft\\Microsoft SDKs\\WindowsPhone\\v")
                                   : QStringLiteral("Microsoft\\Microsoft SDKs\\Windows\\v"))
                        + winsdkVer + QStringLiteral("\\InstallationFolder");
                const QString kitDir = qt_readRegistryKey(HKEY_LOCAL_MACHINE, regKey);
                if (kitDir.isEmpty()) {
                    fprintf(stderr, "Failed to find the Windows Kit installation directory.\n");
                    return false;
                }
#else
                const QString vcInstallDir = "/fake/vc_install_dir";
                const QString kitDir = "/fake/sdk_install_dir";
#endif // Q_OS_WIN

                QStringList incDirs;
                QStringList libDirs;
                QStringList binDirs;
                if (isPhone) {
                    QString sdkDir = vcInstallDir + QStringLiteral("/WPSDK/") + targetVer;
                    if (!QDir(sdkDir).exists()) {
                        fprintf(stderr, "Failed to find the Windows Phone SDK in %s.\n"
                                        "Check that it is properly installed.\n",
                                qPrintable(QDir::toNativeSeparators(sdkDir)));
                        return false;
                    }
                    incDirs << sdkDir + QStringLiteral("/include");
                    libDirs << sdkDir + QStringLiteral("/lib/") + compilerArch;
                    binDirs << sdkDir + QStringLiteral("/bin/") + compiler;
                    libDirs << kitDir + QStringLiteral("/lib/") + arch;
                    incDirs << kitDir + QStringLiteral("/include")
                            << kitDir + QStringLiteral("/include/abi")
                            << kitDir + QStringLiteral("/include/mincore")
                            << kitDir + QStringLiteral("/include/minwin");
                } else {
                    incDirs << vcInstallDir + QStringLiteral("/include");
                    libDirs << vcInstallDir + QStringLiteral("/lib/store/") + compilerArch
                            << vcInstallDir + QStringLiteral("/lib/") + compilerArch;
                    binDirs << vcInstallDir + QStringLiteral("/bin/") + compiler
                            << vcInstallDir + QStringLiteral("/../Common7/IDE");
                    libDirs << kitDir + QStringLiteral("/Lib/") + targetVer + ("/um/") + arch;
                    incDirs << kitDir + QStringLiteral("/include/um")
                            << kitDir + QStringLiteral("/include/shared")
                            << kitDir + QStringLiteral("/include/winrt");
                }

                // Inherit PATH
                binDirs << QString::fromLocal8Bit(qgetenv("PATH")).split(QLatin1Char(';'));

                t << "\nINCLUDE = " << nmakePathList(incDirs);
                t << "\nLIB = " << nmakePathList(libDirs);
                t << "\nPATH = " << nmakePathList(binDirs) << '\n';
            }
        }
        writeNmakeParts(t);
        return MakefileGenerator::writeMakefile(t);
    }
    else if(project->first("TEMPLATE") == "subdirs") {
        writeSubDirs(t);
        return true;
    }
    return false;
}

void NmakeMakefileGenerator::writeSubMakeCall(QTextStream &t, const QString &callPrefix,
                                              const QString &makeArguments)
{
    // Pass MAKEFLAGS as environment variable to sub-make calls.
    // Unlike other make tools nmake doesn't do this automatically.
    t << "\n\t@set MAKEFLAGS=$(MAKEFLAGS)";
    Win32MakefileGenerator::writeSubMakeCall(t, callPrefix, makeArguments);
}

QString NmakeMakefileGenerator::getPdbTarget()
{
    return QString(project->first("TARGET") + project->first("TARGET_VERSION_EXT") + ".pdb");
}

QString NmakeMakefileGenerator::defaultInstall(const QString &t)
{
    if((t != "target" && t != "dlltarget") ||
       (t == "dlltarget" && (project->first("TEMPLATE") != "lib" || !project->isActiveConfig("shared"))) ||
        project->first("TEMPLATE") == "subdirs")
       return QString();

    QString ret = Win32MakefileGenerator::defaultInstall(t);

    const QString root = "$(INSTALL_ROOT)";
    ProStringList &uninst = project->values(ProKey(t + ".uninstall"));
    QString targetdir = Option::fixPathToTargetOS(project->first(ProKey(t + ".path")).toQString(), false);
    targetdir = fileFixify(targetdir, FileFixifyAbsolute);
    if(targetdir.right(1) != Option::dir_sep)
        targetdir += Option::dir_sep;

    if (project->isActiveConfig("debug_info")) {
        if (t == "dlltarget"
            || project->first("TEMPLATE") != "lib"
            || (project->isActiveConfig("shared")
                && project->values(ProKey(t + ".CONFIG")).indexOf("no_dll") == -1)) {
            QString pdb_target = getPdbTarget();
            pdb_target.remove('"');
            QString src_targ = (project->isEmpty("DESTDIR") ? QString("$(DESTDIR)") : project->first("DESTDIR")) + pdb_target;
            QString dst_targ = filePrefixRoot(root, fileFixify(targetdir + pdb_target, FileFixifyAbsolute));
            if(!ret.isEmpty())
                ret += "\n\t";
            ret += QString("-$(INSTALL_FILE)") + " \"" + src_targ + "\" \"" + dst_targ + "\"";
            if(!uninst.isEmpty())
                uninst.append("\n\t");
            uninst.append("-$(DEL_FILE) \"" + dst_targ + "\"");
        }
    }

    return ret;
}

QStringList &NmakeMakefileGenerator::findDependencies(const QString &file)
{
    QStringList &aList = MakefileGenerator::findDependencies(file);
    // Note: The QMAKE_IMAGE_COLLECTION file have all images
    // as dependency, so don't add precompiled header then
    if (file == project->first("QMAKE_IMAGE_COLLECTION"))
        return aList;
    for(QStringList::Iterator it = Option::cpp_ext.begin(); it != Option::cpp_ext.end(); ++it) {
        if(file.endsWith(*it)) {
            if(!precompObj.isEmpty() && !aList.contains(precompObj))
                aList += precompObj;
            break;
        }
    }
    return aList;
}

void NmakeMakefileGenerator::writeNmakeParts(QTextStream &t)
{
    writeStandardParts(t);

    // precompiled header
    if(usePCH) {
        QString precompRule = QString("-c -Yc -Fp%1 -Fo%2").arg(precompPch).arg(precompObj);
        t << precompObj << ": " << precompH << " " << escapeDependencyPaths(findDependencies(precompH)).join(" \\\n\t\t")
          << "\n\t$(CXX) " + precompRule +" $(CXXFLAGS) $(INCPATH) -TP " << precompH << endl << endl;
    }
}

QString NmakeMakefileGenerator::var(const ProKey &value) const
{
    if (usePCH) {
        if ((value == "QMAKE_RUN_CXX_IMP_BATCH"
            || value == "QMAKE_RUN_CXX_IMP"
            || value == "QMAKE_RUN_CXX")) {
            QFileInfo precompHInfo(fileInfo(precompH));
            QString precompRule = QString("-c -FI%1 -Yu%2 -Fp%3")
                .arg(precompHInfo.fileName())
                .arg(precompHInfo.fileName())
                .arg(precompPch);
            QString p = MakefileGenerator::var(value);
            p.replace("-c", precompRule);
            // Cannot use -Gm with -FI & -Yu, as this gives an
            // internal compiler error, on the newer compilers
            // ### work-around for a VS 2003 bug. Move to some prf file or remove completely.
            p.remove("-Gm");
            return p;
        } else if (value == "QMAKE_CXXFLAGS") {
            // Remove internal compiler error option
            // ### work-around for a VS 2003 bug. Move to some prf file or remove completely.
            return MakefileGenerator::var(value).remove("-Gm");
        }
    }

    // Normal val
    return MakefileGenerator::var(value);
}

void NmakeMakefileGenerator::init()
{
    if(init_flag)
        return;
    init_flag = true;

    /* this should probably not be here, but I'm using it to wrap the .t files */
    if(project->first("TEMPLATE") == "app")
        project->values("QMAKE_APP_FLAG").append("1");
    else if(project->first("TEMPLATE") == "lib")
        project->values("QMAKE_LIB_FLAG").append("1");
    else if(project->first("TEMPLATE") == "subdirs") {
        MakefileGenerator::init();
        if(project->values("MAKEFILE").isEmpty())
            project->values("MAKEFILE").append("Makefile");
        if(project->isEmpty("QMAKE_COPY_FILE"))
            project->values("QMAKE_COPY_FILE").append("$(COPY)");
        if(project->isEmpty("QMAKE_COPY_DIR"))
            project->values("QMAKE_COPY_DIR").append("xcopy /s /q /y /i");
        if(project->isEmpty("QMAKE_INSTALL_FILE"))
            project->values("QMAKE_INSTALL_FILE").append("$(COPY_FILE)");
        if(project->isEmpty("QMAKE_INSTALL_PROGRAM"))
            project->values("QMAKE_INSTALL_PROGRAM").append("$(COPY_FILE)");
        if(project->isEmpty("QMAKE_INSTALL_DIR"))
            project->values("QMAKE_INSTALL_DIR").append("$(COPY_DIR)");
        return;
    }

    project->values("QMAKE_L_FLAG") << "/LIBPATH:";

    processVars();

    if (!project->values("RES_FILE").isEmpty()) {
        project->values("QMAKE_LIBS") += escapeFilePaths(project->values("RES_FILE"));
    }

    if (!project->values("DEF_FILE").isEmpty()) {
        QString defFileName = fileFixify(project->first("DEF_FILE").toQString());
        project->values("QMAKE_LFLAGS").append(QString("/DEF:") + escapeFilePath(defFileName));
    }

    if(!project->values("VERSION").isEmpty()) {
        ProString version = project->values("VERSION")[0];
        int firstDot = version.indexOf(".");
        QString major = version.left(firstDot).toQString();
        QString minor = version.right(version.length() - firstDot - 1).toQString();
        minor.replace(".", "");
        project->values("QMAKE_LFLAGS").append("/VERSION:" + major + "." + minor);
    }

    if (project->isEmpty("QMAKE_LINK_O_FLAG"))
        project->values("QMAKE_LINK_O_FLAG").append("/OUT:");

    // Base class init!
    MakefileGenerator::init();

    // Setup PCH variables
    precompH = project->first("PRECOMPILED_HEADER").toQString();
    usePCH = !precompH.isEmpty() && project->isActiveConfig("precompile_header");
    if (usePCH) {
        // Created files
        precompObj = var("PRECOMPILED_DIR") + project->first("TARGET") + "_pch" + Option::obj_ext;
        precompPch = var("PRECOMPILED_DIR") + project->first("TARGET") + "_pch.pch";
        // Add linking of precompObj (required for whole precompiled classes)
        project->values("OBJECTS")                  += precompObj;
        // Add pch file to cleanup
        project->values("QMAKE_CLEAN")          += precompPch;
        // Return to variable pool
        project->values("PRECOMPILED_OBJECT") = ProStringList(precompObj);
        project->values("PRECOMPILED_PCH")    = ProStringList(precompPch);
    }

    ProString version = project->first("TARGET_VERSION_EXT");
    if(project->isActiveConfig("shared")) {
        project->values("QMAKE_CLEAN").append(project->first("DESTDIR") + project->first("TARGET") + version + ".exp");
    }
    if (project->isActiveConfig("debug_info")) {
        QString pdbfile = project->first("DESTDIR") + project->first("TARGET") + version + ".pdb";
        QString escapedPdbFile = escapeFilePath(pdbfile);
        project->values("QMAKE_CFLAGS").append("/Fd" + escapedPdbFile);
        project->values("QMAKE_CXXFLAGS").append("/Fd" + escapedPdbFile);
        project->values("QMAKE_DISTCLEAN").append(pdbfile);
    }
    if (project->isActiveConfig("debug")) {
        project->values("QMAKE_CLEAN").append(project->first("DESTDIR") + project->first("TARGET") + version + ".ilk");
        project->values("QMAKE_CLEAN").append(project->first("DESTDIR") + project->first("TARGET") + version + ".idb");
    } else {
        ProStringList &defines = project->values("DEFINES");
        if (!defines.contains("NDEBUG"))
            defines.append("NDEBUG");
    }
}

void NmakeMakefileGenerator::writeImplicitRulesPart(QTextStream &t)
{
    t << ".SUFFIXES:";
    for(QStringList::Iterator cit = Option::c_ext.begin(); cit != Option::c_ext.end(); ++cit)
        t << " " << (*cit);
    for(QStringList::Iterator cppit = Option::cpp_ext.begin(); cppit != Option::cpp_ext.end(); ++cppit)
        t << " " << (*cppit);
    t << endl << endl;

    if(!project->isActiveConfig("no_batch")) {
        // Batchmode doesn't use the non implicit rules QMAKE_RUN_CXX & QMAKE_RUN_CC
        project->variables().remove("QMAKE_RUN_CXX");
        project->variables().remove("QMAKE_RUN_CC");

        QHash<QString, void*> source_directories;
        source_directories.insert(".", (void*)1);
        static const char * const directories[] = { "UI_SOURCES_DIR", "UI_DIR", 0 };
        for (int y = 0; directories[y]; y++) {
            QString dirTemp = project->first(directories[y]).toQString();
            if (dirTemp.endsWith("\\"))
                dirTemp.truncate(dirTemp.length()-1);
            if(!dirTemp.isEmpty())
                source_directories.insert(dirTemp, (void*)1);
        }
        static const char * const srcs[] = { "SOURCES", "GENERATED_SOURCES", 0 };
        for (int x = 0; srcs[x]; x++) {
            const ProStringList &l = project->values(srcs[x]);
            for (ProStringList::ConstIterator sit = l.begin(); sit != l.end(); ++sit) {
                QString sep = "\\";
                if((*sit).indexOf(sep) == -1)
                    sep = "/";
                QString dir = (*sit).toQString().section(sep, 0, -2);
                if(!dir.isEmpty() && !source_directories[dir])
                    source_directories.insert(dir, (void*)1);
            }
        }

        for(QHash<QString, void*>::Iterator it(source_directories.begin()); it != source_directories.end(); ++it) {
            if(it.key().isEmpty())
                continue;
            QString objDir = var("OBJECTS_DIR");
            if (objDir == ".\\")
                objDir = "";
            for(QStringList::Iterator cppit = Option::cpp_ext.begin(); cppit != Option::cpp_ext.end(); ++cppit)
                t << "{" << it.key() << "}" << (*cppit) << "{" << objDir << "}" << Option::obj_ext << "::\n\t"
                  << var("QMAKE_RUN_CXX_IMP_BATCH").replace(QRegExp("\\$@"), var("OBJECTS_DIR")) << endl << "\t$<\n<<\n\n";
            for(QStringList::Iterator cit = Option::c_ext.begin(); cit != Option::c_ext.end(); ++cit)
                t << "{" << it.key() << "}" << (*cit) << "{" << objDir << "}" << Option::obj_ext << "::\n\t"
                  << var("QMAKE_RUN_CC_IMP_BATCH").replace(QRegExp("\\$@"), var("OBJECTS_DIR")) << endl << "\t$<\n<<\n\n";
        }
    } else {
        for(QStringList::Iterator cppit = Option::cpp_ext.begin(); cppit != Option::cpp_ext.end(); ++cppit)
            t << (*cppit) << Option::obj_ext << ":\n\t" << var("QMAKE_RUN_CXX_IMP") << endl << endl;
        for(QStringList::Iterator cit = Option::c_ext.begin(); cit != Option::c_ext.end(); ++cit)
            t << (*cit) << Option::obj_ext << ":\n\t" << var("QMAKE_RUN_CC_IMP") << endl << endl;
    }

}

void NmakeMakefileGenerator::writeBuildRulesPart(QTextStream &t)
{
    const ProString templateName = project->first("TEMPLATE");

    t << "first: all\n";
    t << "all: " << fileFixify(Option::output.fileName()) << " " << varGlue("ALL_DEPS"," "," "," ") << "$(DESTDIR_TARGET)\n\n";
    t << "$(DESTDIR_TARGET): " << var("PRE_TARGETDEPS") << " $(OBJECTS) " << var("POST_TARGETDEPS");

    if(!project->isEmpty("QMAKE_PRE_LINK"))
        t << "\n\t" <<var("QMAKE_PRE_LINK");
    if(project->isActiveConfig("staticlib")) {
        t << "\n\t$(LIBAPP) $(LIBFLAGS) " << var("QMAKE_LINK_O_FLAG") << "$(DESTDIR_TARGET) @<<\n\t  "
          << "$(OBJECTS)"
          << "\n<<";
    } else if (templateName != "aux") {
        const bool embedManifest = ((templateName == "app" && project->isActiveConfig("embed_manifest_exe"))
                                    || (templateName == "lib" && project->isActiveConfig("embed_manifest_dll")
                                        && !(project->isActiveConfig("plugin") && project->isActiveConfig("no_plugin_manifest"))
                                        ));
        if (embedManifest) {
            bool generateManifest = false;
            const QString target = var("DEST_TARGET");
            QString manifest = project->first("QMAKE_MANIFEST").toQString();
            QString extraLFlags;
            if (manifest.isEmpty()) {
                generateManifest = true;
                manifest = escapeFilePath(target + ".embed.manifest");
                extraLFlags = "/MANIFEST /MANIFESTFILE:" + manifest;
                project->values("QMAKE_CLEAN") << manifest;
            } else {
                manifest = escapeFilePath(fileFixify(manifest));
            }

            const QString resourceId = (templateName == "app") ? "1" : "2";
            const bool incrementalLinking = project->values("QMAKE_LFLAGS").toQStringList().filter(QRegExp("(/|-)INCREMENTAL:NO")).isEmpty();
            if (incrementalLinking) {
                // Link a resource that contains the manifest without modifying the exe/dll after linking.

                QString manifest_rc = escapeFilePath(target +  "_manifest.rc");
                QString manifest_res = escapeFilePath(target +  "_manifest.res");
                QString manifest_bak = escapeFilePath(target +  "_manifest.bak");
                project->values("QMAKE_CLEAN") << manifest_rc << manifest_res;

                t << "\n\techo " << resourceId
                  << " /* CREATEPROCESS_MANIFEST_RESOURCE_ID */ 24 /* RT_MANIFEST */ "
                  << cQuoted(unescapeFilePath(manifest)) << ">" << manifest_rc;

                if (generateManifest) {
                    t << "\n\tif not exist $(DESTDIR_TARGET) if exist " << manifest
                      << " del " << manifest;
                    t << "\n\tif exist " << manifest << " copy /Y " << manifest << ' ' << manifest_bak;
                    const QString extraInlineFileContent = "\n!IF EXIST(" + manifest_res + ")\n" + manifest_res + "\n!ENDIF";
                    t << "\n\t";
                    writeLinkCommand(t, extraLFlags, extraInlineFileContent);
                    t << "\n\tif exist " << manifest_bak << " fc /b " << manifest << ' ' << manifest_bak << " >NUL || del " << manifest_bak;
                    t << "\n\tif not exist " << manifest_bak << " rc.exe /fo" << manifest_res << ' ' << manifest_rc;
                    t << "\n\tif not exist " << manifest_bak << ' ';
                    writeLinkCommand(t, extraLFlags, manifest_res);
                    t << "\n\tif exist " << manifest_bak << " del " << manifest_bak;
                } else {
                    t << "\n\trc.exe /fo" << manifest_res << " " << manifest_rc;
                    t << "\n\t";
                    writeLinkCommand(t, extraLFlags, manifest_res);
                }
            } else {
                // directly embed the manifest in the executable after linking
                t << "\n\t";
                writeLinkCommand(t, extraLFlags);
                t << "\n\tmt.exe /nologo /manifest " << manifest
                  << " /outputresource:$(DESTDIR_TARGET);" << resourceId;
            }
        }  else {
            t << "\n\t";
            writeLinkCommand(t);
        }
    }
    QString signature = !project->isEmpty("SIGNATURE_FILE") ? var("SIGNATURE_FILE") : var("DEFAULT_SIGNATURE");
    bool useSignature = !signature.isEmpty() && !project->isActiveConfig("staticlib") &&
                        !project->isEmpty("CE_SDK") && !project->isEmpty("CE_ARCH");
    if(useSignature) {
        t << "\n\tsigntool sign /F " << signature << " $(DESTDIR_TARGET)";
    }
    if(!project->isEmpty("QMAKE_POST_LINK")) {
        t << "\n\t" << var("QMAKE_POST_LINK");
    }
    t << endl;
}

void NmakeMakefileGenerator::writeLinkCommand(QTextStream &t, const QString &extraFlags, const QString &extraInlineFileContent)
{
    t << "$(LINKER) $(LFLAGS)";
    if (!extraFlags.isEmpty())
        t << ' ' << extraFlags;
    t << " " << var("QMAKE_LINK_O_FLAG") << "$(DESTDIR_TARGET) @<<\n"
      << "$(OBJECTS) $(LIBS)";
    if (!extraInlineFileContent.isEmpty())
        t << ' ' << extraInlineFileContent;
    t << "\n<<";
}

QT_END_NAMESPACE
