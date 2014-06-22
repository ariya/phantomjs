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

#include "mingw_make.h"
#include "option.h"
#include "meta.h"

#include <proitems.h>

#include <qregexp.h>
#include <qdir.h>
#include <stdlib.h>
#include <time.h>

QT_BEGIN_NAMESPACE

MingwMakefileGenerator::MingwMakefileGenerator() : Win32MakefileGenerator(), init_flag(false)
{
    if (isWindowsShell())
        quote = "\"";
    else
        quote = "'";
}

QString MingwMakefileGenerator::escapeDependencyPath(const QString &path) const
{
    QString ret = path;
    ret.remove('\"');
    ret.replace('\\', "/");
    ret.replace(' ', "\\ ");
    return ret;
}

QString MingwMakefileGenerator::getLibTarget()
{
    return QString("lib" + project->first("TARGET") + project->first("TARGET_VERSION_EXT") + ".a");
}

bool MingwMakefileGenerator::findLibraries()
{
    QList<QMakeLocalFileName> dirs;
  static const char * const lflags[] = { "QMAKE_LIBS", "QMAKE_LIBS_PRIVATE", 0 };
  for (int i = 0; lflags[i]; i++) {
    ProStringList &l = project->values(lflags[i]);
    ProStringList::Iterator it = l.begin();
    while (it != l.end()) {
        if ((*it).startsWith("-l")) {
            QString steam = (*it).mid(2).toQString();
            ProString out;
            QString suffix = project->first(ProKey("QMAKE_" + steam.toUpper() + "_SUFFIX")).toQString();
            for (QList<QMakeLocalFileName>::Iterator dir_it = dirs.begin(); dir_it != dirs.end(); ++dir_it) {
                QString extension;
                int ver = findHighestVersion((*dir_it).local(), steam, "dll.a|a");
                if (ver > 0)
                    extension += QString::number(ver);
                extension += suffix;
                if(QMakeMetaInfo::libExists((*dir_it).local() + Option::dir_sep + steam) ||
                    exists((*dir_it).local() + Option::dir_sep + steam + extension + ".a") ||
                    exists((*dir_it).local() + Option::dir_sep + steam + extension + ".dll.a")) {
                        out = *it + extension;
                        break;
                }
            }
            if (!out.isEmpty()) // We assume if it never finds it that its correct
                (*it) = out;
        } else if ((*it).startsWith("-L")) {
            dirs.append(QMakeLocalFileName((*it).mid(2).toQString()));
        }

        ++it;
    }
  }
    return true;
}

bool MingwMakefileGenerator::writeMakefile(QTextStream &t)
{
    writeHeader(t);
    if (writeDummyMakefile(t))
        return true;

    if(project->first("TEMPLATE") == "app" ||
       project->first("TEMPLATE") == "lib" ||
       project->first("TEMPLATE") == "aux") {
        if(project->isActiveConfig("create_pc") && project->first("TEMPLATE") == "lib")
            writePkgConfigFile();

        if(Option::mkfile::do_stub_makefile) {
            t << "QMAKE    = " << var("QMAKE_QMAKE") << endl;
            const ProStringList &qut = project->values("QMAKE_EXTRA_TARGETS");
            for (ProStringList::ConstIterator it = qut.begin(); it != qut.end(); ++it)
                t << *it << " ";
            t << "first all clean install distclean uninstall: qmake\n"
              << "qmake_all:\n";
            writeMakeQmake(t);
            t << "FORCE:\n\n";
            return true;
        }
        writeMingwParts(t);
        return MakefileGenerator::writeMakefile(t);
    }
    else if(project->first("TEMPLATE") == "subdirs") {
        writeSubDirs(t);
        return true;
    }
    return false;
 }

void createLdObjectScriptFile(const QString &fileName, const ProStringList &objList)
{
    QString filePath = Option::output_dir + QDir::separator() + fileName;
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream t(&file);
        t << "INPUT(\n";
        for (ProStringList::ConstIterator it = objList.constBegin(); it != objList.constEnd(); ++it) {
            QString path = (*it).toQString();
            if (QDir::isRelativePath(path))
                t << "./" << path << endl;
            else
                t << path << endl;
        }
        t << ");\n";
        t.flush();
        file.close();
    }
}

void createArObjectScriptFile(const QString &fileName, const QString &target, const ProStringList &objList)
{
    QString filePath = Option::output_dir + QDir::separator() + fileName;
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream t(&file);
        t << "CREATE " << target << endl;
        for (ProStringList::ConstIterator it = objList.constBegin(); it != objList.constEnd(); ++it) {
            t << "ADDMOD " << *it << endl;
        }
        t << "SAVE\n";
        t.flush();
        file.close();
    }
}

void createRvctObjectScriptFile(const QString &fileName, const ProStringList &objList)
{
    QString filePath = Option::output_dir + QDir::separator() + fileName;
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream t(&file);
        for (ProStringList::ConstIterator it = objList.constBegin(); it != objList.constEnd(); ++it) {
            QString path = (*it).toQString();
            if (QDir::isRelativePath(path))
                t << "./" << path << endl;
            else
                t << path << endl;
        }
        t.flush();
        file.close();
    }
}

void MingwMakefileGenerator::writeMingwParts(QTextStream &t)
{
    writeStandardParts(t);

    if (!preCompHeaderOut.isEmpty()) {
        QString header = project->first("PRECOMPILED_HEADER").toQString();
        QString cHeader = preCompHeaderOut + Option::dir_sep + "c";
        t << escapeDependencyPath(cHeader) << ": " << escapeDependencyPath(header) << " "
          << escapeDependencyPaths(findDependencies(header)).join(" \\\n\t\t")
          << "\n\t" << mkdir_p_asstring(preCompHeaderOut)
          << "\n\t$(CC) -x c-header -c $(CFLAGS) $(INCPATH) -o " << cHeader << " " << header
          << endl << endl;
        QString cppHeader = preCompHeaderOut + Option::dir_sep + "c++";
        t << escapeDependencyPath(cppHeader) << ": " << escapeDependencyPath(header) << " "
          << escapeDependencyPaths(findDependencies(header)).join(" \\\n\t\t")
          << "\n\t" << mkdir_p_asstring(preCompHeaderOut)
          << "\n\t$(CXX) -x c++-header -c $(CXXFLAGS) $(INCPATH) -o " << cppHeader << " " << header
          << endl << endl;
    }
}

void MingwMakefileGenerator::init()
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
        if(project->values("MAKEFILE").isEmpty())
            project->values("MAKEFILE").append("Makefile");
        return;
    }

    project->values("TARGET_PRL").append(project->first("TARGET"));

    project->values("QMAKE_L_FLAG") << "-L";

    processVars();

    if (!project->values("RES_FILE").isEmpty()) {
        project->values("QMAKE_LIBS") += escapeFilePaths(project->values("RES_FILE"));
    }

    ProStringList &configs = project->values("CONFIG");

    if(project->isActiveConfig("qt_dll"))
        if(configs.indexOf("qt") == -1)
            configs.append("qt");

    if (project->isActiveConfig("dll")) {
        QString destDir = "";
        if(!project->first("DESTDIR").isEmpty())
            destDir = Option::fixPathToTargetOS(project->first("DESTDIR") + Option::dir_sep, false, false);
        project->values("MINGW_IMPORT_LIB").prepend(destDir + "lib" + project->first("TARGET")
                                                         + project->first("TARGET_VERSION_EXT") + ".a");
        project->values("QMAKE_LFLAGS").append(QString("-Wl,--out-implib,") + project->first("MINGW_IMPORT_LIB"));
    }

    if (!project->values("DEF_FILE").isEmpty()) {
        QString defFileName = fileFixify(project->first("DEF_FILE").toQString());
        project->values("QMAKE_LFLAGS").append(QString("-Wl,") + escapeFilePath(defFileName));
    }

    if (project->isActiveConfig("staticlib") && project->first("TEMPLATE") == "lib")
        project->values("QMAKE_LFLAGS").append("-static");

    MakefileGenerator::init();

    // precomp
    if (!project->first("PRECOMPILED_HEADER").isEmpty()
        && project->isActiveConfig("precompile_header")) {
        QString preCompHeader = var("PRECOMPILED_DIR")
                    + QFileInfo(project->first("PRECOMPILED_HEADER").toQString()).fileName();
        preCompHeaderOut = preCompHeader + ".gch";
        project->values("QMAKE_CLEAN").append(preCompHeaderOut + Option::dir_sep + "c");
        project->values("QMAKE_CLEAN").append(preCompHeaderOut + Option::dir_sep + "c++");

        project->values("QMAKE_RUN_CC").clear();
        project->values("QMAKE_RUN_CC").append("$(CC) -c -include " + preCompHeader +
                                                    " $(CFLAGS) $(INCPATH) " + var("QMAKE_CC_O_FLAG") + "$obj $src");
        project->values("QMAKE_RUN_CC_IMP").clear();
        project->values("QMAKE_RUN_CC_IMP").append("$(CC)  -c -include " + preCompHeader +
                                                        " $(CFLAGS) $(INCPATH) " + var("QMAKE_CC_O_FLAG") + "$@ $<");
        project->values("QMAKE_RUN_CXX").clear();
        project->values("QMAKE_RUN_CXX").append("$(CXX) -c -include " + preCompHeader +
                                                     " $(CXXFLAGS) $(INCPATH) " + var("QMAKE_CC_O_FLAG") + "$obj $src");
        project->values("QMAKE_RUN_CXX_IMP").clear();
        project->values("QMAKE_RUN_CXX_IMP").append("$(CXX) -c -include " + preCompHeader +
                                                         " $(CXXFLAGS) $(INCPATH) " + var("QMAKE_CC_O_FLAG") + "$@ $<");
    }

    if(project->isActiveConfig("dll")) {
        project->values("QMAKE_CLEAN").append(project->first("MINGW_IMPORT_LIB"));
    }
}

void MingwMakefileGenerator::writeIncPart(QTextStream &t)
{
    t << "INCPATH       = ";

    if (!project->isActiveConfig("no_include_pwd")) {
        QString pwd = escapeFilePath(fileFixify(qmake_getpwd()));
        if (pwd.isEmpty())
            pwd = ".";
        t << "-I" << pwd << " ";
    }

    QString isystem = var("QMAKE_CFLAGS_ISYSTEM");
    const ProStringList &incs = project->values("INCLUDEPATH");
    for (ProStringList::ConstIterator incit = incs.begin(); incit != incs.end(); ++incit) {
        QString inc = (*incit).toQString();
        inc.replace(QRegExp("\\\\$"), "");
        inc.replace(QRegExp("\""), "");

        if (!isystem.isEmpty() && isSystemInclude(inc))
            t << isystem << ' ';
        else
            t << "-I";
        t << quote << inc << quote << " ";
    }
    t << "-I" << quote << specdir() << quote
      << endl;
}

void MingwMakefileGenerator::writeLibsPart(QTextStream &t)
{
    if(project->isActiveConfig("staticlib") && project->first("TEMPLATE") == "lib") {
        t << "LIB        =        " << var("QMAKE_LIB") << endl;
    } else {
        t << "LINKER      =        " << var("QMAKE_LINK") << endl;
        t << "LFLAGS        =        " << var("QMAKE_LFLAGS") << endl;
        t << "LIBS        =        "
          << var("QMAKE_LIBS").replace(QRegExp("(\\slib|^lib)")," -l") << ' '
          << var("QMAKE_LIBS_PRIVATE").replace(QRegExp("(\\slib|^lib)")," -l") << endl;
    }
}

void MingwMakefileGenerator::writeObjectsPart(QTextStream &t)
{
    if (project->values("OBJECTS").count() < var("QMAKE_LINK_OBJECT_MAX").toInt()) {
        objectsLinkLine = "$(OBJECTS)";
    } else if (project->isActiveConfig("staticlib") && project->first("TEMPLATE") == "lib") {
        QString ar_script_file = var("QMAKE_LINK_OBJECT_SCRIPT") + "." + var("TARGET");
        if (!var("BUILD_NAME").isEmpty()) {
            ar_script_file += "." + var("BUILD_NAME");
        }
        // QMAKE_LIB is used for win32, including mingw, whereas QMAKE_AR is used on Unix.
        if (project->isActiveConfig("rvct_linker")) {
            createRvctObjectScriptFile(ar_script_file, project->values("OBJECTS"));
            QString ar_cmd = project->values("QMAKE_LIB").join(' ');
            if (ar_cmd.isEmpty())
                ar_cmd = "armar --create";
            objectsLinkLine = ar_cmd + " " + var("DEST_TARGET") + " --via " + escapeFilePath(ar_script_file);
        } else {
            // Strip off any options since the ar commands will be read from file.
            QString ar_cmd = var("QMAKE_LIB").section(" ", 0, 0);;
            if (ar_cmd.isEmpty())
                ar_cmd = "ar";
            createArObjectScriptFile(ar_script_file, var("DEST_TARGET"), project->values("OBJECTS"));
            objectsLinkLine = ar_cmd + " -M < " + escapeFilePath(ar_script_file);
        }
    } else {
        QString ld_script_file = var("QMAKE_LINK_OBJECT_SCRIPT") + "." + var("TARGET");
        if (!var("BUILD_NAME").isEmpty()) {
            ld_script_file += "." + var("BUILD_NAME");
        }
        if (project->isActiveConfig("rvct_linker")) {
            createRvctObjectScriptFile(ld_script_file, project->values("OBJECTS"));
            objectsLinkLine = QString::fromLatin1("--via ") + escapeFilePath(ld_script_file);
        } else {
            createLdObjectScriptFile(ld_script_file, project->values("OBJECTS"));
            objectsLinkLine = escapeFilePath(ld_script_file);
        }
    }
    Win32MakefileGenerator::writeObjectsPart(t);
}

void MingwMakefileGenerator::writeBuildRulesPart(QTextStream &t)
{
    t << "first: all\n";
    t << "all: " << escapeDependencyPath(fileFixify(Option::output.fileName())) << " " << valGlue(escapeDependencyPaths(project->values("ALL_DEPS"))," "," "," ") << " $(DESTDIR_TARGET)\n\n";
    t << "$(DESTDIR_TARGET): " << var("PRE_TARGETDEPS") << " $(OBJECTS) " << var("POST_TARGETDEPS");
    if(!project->isEmpty("QMAKE_PRE_LINK"))
        t << "\n\t" <<var("QMAKE_PRE_LINK");
    if(project->isActiveConfig("staticlib") && project->first("TEMPLATE") == "lib") {
        if (project->values("OBJECTS").count() < var("QMAKE_LINK_OBJECT_MAX").toInt()) {
            t << "\n\t$(LIB) $(DESTDIR_TARGET) " << objectsLinkLine << " " ;
        } else {
            t << "\n\t" << objectsLinkLine << " " ;
        }
    } else if (project->first("TEMPLATE") != "aux") {
        t << "\n\t$(LINKER) $(LFLAGS) " << var("QMAKE_LINK_O_FLAG") << "$(DESTDIR_TARGET) " << objectsLinkLine << "  $(LIBS)";
    }
    if(!project->isEmpty("QMAKE_POST_LINK"))
        t << "\n\t" <<var("QMAKE_POST_LINK");
    t << endl;
}

void MingwMakefileGenerator::writeRcFilePart(QTextStream &t)
{
    const QString rc_file = fileFixify(project->first("RC_FILE").toQString());

    ProStringList rcIncPaths = project->values("RC_INCLUDEPATH");
    rcIncPaths.prepend(fileInfo(rc_file).path());
    QString incPathStr;
    for (int i = 0; i < rcIncPaths.count(); ++i) {
        const ProString &path = rcIncPaths.at(i);
        if (path.isEmpty())
            continue;
        incPathStr += QStringLiteral(" --include-dir=");
        if (path != "." && QDir::isRelativePath(path.toQString()))
            incPathStr += "./";
        incPathStr += escapeFilePath(path);
    }

    if (!rc_file.isEmpty()) {
        t << escapeDependencyPath(var("RES_FILE")) << ": " << rc_file << "\n\t"
          << var("QMAKE_RC") << " -i " << rc_file << " -o " << var("RES_FILE")
          << incPathStr << " $(DEFINES)\n\n";
    }
}

QStringList &MingwMakefileGenerator::findDependencies(const QString &file)
{
    QStringList &aList = MakefileGenerator::findDependencies(file);
    // Note: The QMAKE_IMAGE_COLLECTION file have all images
    // as dependency, so don't add precompiled header then
    if (file == project->first("QMAKE_IMAGE_COLLECTION")
        || preCompHeaderOut.isEmpty())
        return aList;
    for (QStringList::Iterator it = Option::c_ext.begin(); it != Option::c_ext.end(); ++it) {
        if (file.endsWith(*it)) {
            QString cHeader = preCompHeaderOut + Option::dir_sep + "c";
            if (!aList.contains(cHeader))
                aList += cHeader;
            break;
        }
    }
    for (QStringList::Iterator it = Option::cpp_ext.begin(); it != Option::cpp_ext.end(); ++it) {
        if (file.endsWith(*it)) {
            QString cppHeader = preCompHeaderOut + Option::dir_sep + "c++";
            if (!aList.contains(cppHeader))
                aList += cppHeader;
            break;
        }
    }
    return aList;
}

QT_END_NAMESPACE
