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

#include "msvc_nmake.h"
#include "option.h"
#include <qregexp.h>
#include <qhash.h>
#include <qdir.h>
#include <time.h>

QT_BEGIN_NAMESPACE

NmakeMakefileGenerator::NmakeMakefileGenerator() : Win32MakefileGenerator(), init_flag(false)
{

}

bool
NmakeMakefileGenerator::writeMakefile(QTextStream &t)
{
    writeHeader(t);
    if(!project->values("QMAKE_FAILED_REQUIREMENTS").isEmpty()) {
        QStringList &qut = project->values("QMAKE_EXTRA_TARGETS");
        for(QStringList::ConstIterator it = qut.begin(); it != qut.end(); ++it)
            t << *it << " ";
        t << "all first clean:" << "\n\t"
          << "@echo \"Some of the required modules ("
          << var("QMAKE_FAILED_REQUIREMENTS") << ") are not available.\"" << "\n\t"
          << "@echo \"Skipped.\"" << endl << endl;
        writeMakeQmake(t);
        return true;
    }

    if(project->first("TEMPLATE") == "app" ||
       project->first("TEMPLATE") == "lib" ||
       project->first("TEMPLATE") == "aux") {
#if 0
        if(Option::mkfile::do_stub_makefile)
            return MakefileGenerator::writeStubMakefile(t);
#endif
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
                                              const QString &makeArguments, const QString &callPostfix)
{
    // Pass MAKEFLAGS as environment variable to sub-make calls.
    // Unlike other make tools nmake doesn't do this automatically.
    t << "\n\t@set MAKEFLAGS=$(MAKEFLAGS)";
    Win32MakefileGenerator::writeSubMakeCall(t, callPrefix, makeArguments, callPostfix);
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
    QStringList &uninst = project->values(t + ".uninstall");
    QString targetdir = Option::fixPathToTargetOS(project->first(t + ".path"), false);
    targetdir = fileFixify(targetdir, FileFixifyAbsolute);
    if(targetdir.right(1) != Option::dir_sep)
        targetdir += Option::dir_sep;

    if(t == "target" && project->first("TEMPLATE") == "lib") {
        if(project->isActiveConfig("shared") && project->isActiveConfig("debug")) {
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
        t << precompObj << ": " << precompH << " " << findDependencies(precompH).join(" \\\n\t\t")
          << "\n\t" << "$(CXX) " + precompRule +" $(CXXFLAGS) $(INCPATH) -TP " << precompH << endl << endl;
    }
}

QString NmakeMakefileGenerator::var(const QString &value)
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

    project->values("QMAKE_LIBS") += escapeFilePaths(project->values("LIBS"));
    project->values("QMAKE_LIBS_PRIVATE") += escapeFilePaths(project->values("LIBS_PRIVATE"));
    processVars();

    if (!project->values("RES_FILE").isEmpty()) {
        project->values("QMAKE_LIBS") += escapeFilePaths(project->values("RES_FILE"));
    }

    if (!project->values("DEF_FILE").isEmpty()) {
        QString defFileName = fileFixify(project->values("DEF_FILE")).first();
        project->values("QMAKE_LFLAGS").append(QString("/DEF:") + escapeFilePath(defFileName));
    }

    if(!project->values("VERSION").isEmpty()) {
        QString version = project->values("VERSION")[0];
        int firstDot = version.indexOf(".");
        QString major = version.left(firstDot);
        QString minor = version.right(version.length() - firstDot - 1);
        minor.replace(".", "");
        project->values("QMAKE_LFLAGS").append("/VERSION:" + major + "." + minor);
    }

    // Base class init!
    MakefileGenerator::init();

    // Setup PCH variables
    precompH = project->first("PRECOMPILED_HEADER");
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
        project->values("PRECOMPILED_OBJECT") = QStringList(precompObj);
        project->values("PRECOMPILED_PCH")    = QStringList(precompPch);
    }

    QString version = project->first("TARGET_VERSION_EXT");
    if(project->isActiveConfig("shared")) {
        project->values("QMAKE_CLEAN").append(project->first("DESTDIR") + project->first("TARGET") + version + ".exp");
    }
    if(project->isActiveConfig("debug")) {
        project->values("QMAKE_DISTCLEAN").append(project->first("DESTDIR") + project->first("TARGET") + version + ".pdb");
        project->values("QMAKE_CLEAN").append(project->first("DESTDIR") + project->first("TARGET") + version + ".ilk");
        project->values("QMAKE_CLEAN").append("vc*.pdb");
        project->values("QMAKE_CLEAN").append("vc*.idb");
    }
}

void NmakeMakefileGenerator::writeLibDirPart(QTextStream &t)
{
    QStringList libDirs = project->values("QMAKE_LIBDIR");
    for (int i = 0; i < libDirs.size(); ++i)
        libDirs[i].remove("\"");
    t << valGlue(libDirs,"/LIBPATH:\"","\" /LIBPATH:\"","\"") << " ";
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
        QString directories[] = { QString("UI_SOURCES_DIR"), QString("UI_DIR"), QString() };
        for(int y = 0; !directories[y].isNull(); y++) {
            QString dirTemp = project->first(directories[y]);
            if (dirTemp.endsWith("\\"))
                dirTemp.truncate(dirTemp.length()-1);
            if(!dirTemp.isEmpty())
                source_directories.insert(dirTemp, (void*)1);
        }
        QString srcs[] = { QString("SOURCES"), QString("GENERATED_SOURCES"), QString() };
        for(int x = 0; !srcs[x].isNull(); x++) {
            QStringList &l = project->values(srcs[x]);
            for(QStringList::Iterator sit = l.begin(); sit != l.end(); ++sit) {
                QString sep = "\\";
                if((*sit).indexOf(sep) == -1)
                    sep = "/";
                QString dir = (*sit).section(sep, 0, -2);
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
                  << var("QMAKE_RUN_CXX_IMP_BATCH").replace(QRegExp("\\$@"), var("OBJECTS_DIR")) << endl << "\t$<" << endl << "<<" << endl << endl;
            for(QStringList::Iterator cit = Option::c_ext.begin(); cit != Option::c_ext.end(); ++cit)
                t << "{" << it.key() << "}" << (*cit) << "{" << objDir << "}" << Option::obj_ext << "::\n\t"
                  << var("QMAKE_RUN_CC_IMP_BATCH").replace(QRegExp("\\$@"), var("OBJECTS_DIR")) << endl << "\t$<" << endl << "<<" << endl << endl;
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
    if (project->first("TEMPLATE") == "aux") {
        t << "first:" << endl;
        return;
    }

    t << "first: all" << endl;
    t << "all: " << fileFixify(Option::output.fileName()) << " " << varGlue("ALL_DEPS"," "," "," ") << "$(DESTDIR_TARGET)" << endl << endl;
    t << "$(DESTDIR_TARGET): " << var("PRE_TARGETDEPS") << " $(OBJECTS) " << var("POST_TARGETDEPS");

    if(!project->isEmpty("QMAKE_PRE_LINK"))
        t << "\n\t" <<var("QMAKE_PRE_LINK");
    if(project->isActiveConfig("staticlib")) {
        t << "\n\t" << "$(LIBAPP) $(LIBFLAGS) /OUT:$(DESTDIR_TARGET) @<<" << "\n\t  "
          << "$(OBJECTS)";
    } else {
        t << "\n\t" << "$(LINK) $(LFLAGS) /OUT:$(DESTDIR_TARGET) @<< " << "\n\t  "
          << "$(OBJECTS) $(LIBS)";
    }
    t << endl << "<<";
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

QT_END_NAMESPACE
