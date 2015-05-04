/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake application of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "unixmake.h"
#include "option.h"
#include "meta.h"
#include <qregexp.h>
#include <qbytearray.h>
#include <qfile.h>
#include <qdir.h>
#include <qdatetime.h>
#include <qdebug.h>
#include <time.h>

QT_BEGIN_NAMESPACE

UnixMakefileGenerator::UnixMakefileGenerator() : MakefileGenerator(), init_flag(false), include_deps(false)
{

}

void
UnixMakefileGenerator::writePrlFile(QTextStream &t)
{
    MakefileGenerator::writePrlFile(t);
    // libtool support

    if(project->isActiveConfig("create_libtool") && project->first("TEMPLATE") == "lib") { //write .la
        if(project->isActiveConfig("compile_libtool"))
            warn_msg(WarnLogic, "create_libtool specified with compile_libtool can lead to conflicting .la\n"
                     "formats, create_libtool has been disabled\n");
        else
            writeLibtoolFile();
    }
    // pkg-config support
    if(project->isActiveConfig("create_pc") && project->first("TEMPLATE") == "lib")
        writePkgConfigFile();
}

bool
UnixMakefileGenerator::writeMakefile(QTextStream &t)
{

    writeHeader(t);
    if (writeDummyMakefile(t))
        return true;

    if (project->values("TEMPLATE").first() == "app" ||
        project->values("TEMPLATE").first() == "lib" ||
        project->values("TEMPLATE").first() == "aux") {
        if(Option::mkfile::do_stub_makefile && MakefileGenerator::writeStubMakefile(t))
            return true;
        writeMakeParts(t);
        return MakefileGenerator::writeMakefile(t);
    } else if(project->values("TEMPLATE").first() == "subdirs") {
        MakefileGenerator::writeSubDirs(t);
        return true;
    }
    return false;
}

void
UnixMakefileGenerator::writeDefaultVariables(QTextStream &t)
{
    MakefileGenerator::writeDefaultVariables(t);
    t << "TAR           = " << var("QMAKE_TAR") << endl;
    t << "COMPRESS      = " << var("QMAKE_GZIP") << endl;

    if (project->isEmpty("QMAKE_DISTNAME")) {
        ProString distname = project->first("QMAKE_ORIG_TARGET");
        if (!project->isActiveConfig("no_dist_version"))
            distname += project->first("VERSION");
        project->values("QMAKE_DISTNAME") = distname;
    }
    t << "DISTNAME      = " << var("QMAKE_DISTNAME") << endl;

    if (project->isEmpty("QMAKE_DISTDIR"))
        project->values("QMAKE_DISTDIR") = project->first("QMAKE_DISTNAME");
    t << "DISTDIR = " << escapeFilePath(fileFixify(
            (project->isEmpty("OBJECTS_DIR") ? ProString(".tmp/") : project->first("OBJECTS_DIR")) + project->first("QMAKE_DISTDIR"),
            Option::output_dir, Option::output_dir, FileFixifyAbsolute)) << endl;
}

void
UnixMakefileGenerator::writeSubTargets(QTextStream &t, QList<MakefileGenerator::SubTarget*> targets, int flags)
{
    MakefileGenerator::writeSubTargets(t, targets, flags);

    t << "dist: distdir FORCE" << endl;
    t << "\t(cd `dirname $(DISTDIR)` && $(TAR) $(DISTNAME).tar $(DISTNAME) && $(COMPRESS) $(DISTNAME).tar)"
         " && $(MOVE) `dirname $(DISTDIR)`/$(DISTNAME).tar.gz . && $(DEL_FILE) -r $(DISTDIR)";
    t << endl << endl;

    t << "distdir:";
    for (int target = 0; target < targets.size(); ++target) {
        SubTarget *subtarget = targets.at(target);
        t << " " << subtarget->target << "-distdir";
    }
    t << " FORCE\n\t"
      << mkdir_p_asstring("$(DISTDIR)", false) << "\n\t"
      << "$(COPY_FILE) --parents " << var("DISTFILES") << " $(DISTDIR)" << Option::dir_sep << endl << endl;

    const QString abs_source_path = project->first("QMAKE_ABSOLUTE_SOURCE_PATH").toQString();
    for (int target = 0; target < targets.size(); ++target) {
        SubTarget *subtarget = targets.at(target);
        QString in_directory = subtarget->in_directory;
        if (!in_directory.isEmpty() && !in_directory.endsWith(Option::dir_sep))
            in_directory += Option::dir_sep;
        QString out_directory = subtarget->out_directory;
        if (!out_directory.isEmpty() && !out_directory.endsWith(Option::dir_sep))
            out_directory += Option::dir_sep;
        if (!abs_source_path.isEmpty() && out_directory.startsWith(abs_source_path))
            out_directory = Option::output_dir + out_directory.mid(abs_source_path.length());

        QString dist_directory = out_directory;
        if (dist_directory.endsWith(Option::dir_sep))
            dist_directory.chop(Option::dir_sep.length());
        if (!dist_directory.startsWith(Option::dir_sep))
            dist_directory.prepend(Option::dir_sep);

        QString out_directory_cdin = out_directory.isEmpty() ? "\n\t"
                                                             : "\n\tcd " + out_directory + " && ";
        QString makefilein = " -e -f " + subtarget->makefile + " distdir DISTDIR=$(DISTDIR)" + dist_directory;

        QString out = subtarget->makefile;
        QString in = escapeFilePath(fileFixify(in_directory + subtarget->profile, FileFixifyAbsolute));
        if (out.startsWith(in_directory))
            out.remove(0, in_directory.length());

        t << subtarget->target << "-distdir: FORCE";
        writeSubTargetCall(t, in_directory, in, out_directory, out,
                           out_directory_cdin, makefilein);
        t << endl;
    }
}

void
UnixMakefileGenerator::writeMakeParts(QTextStream &t)
{
    QString deps = fileFixify(Option::output.fileName()), target_deps, prl;
    bool do_incremental = (project->isActiveConfig("incremental") &&
                           !project->values("QMAKE_INCREMENTAL").isEmpty() &&
                           (!project->values("QMAKE_APP_FLAG").isEmpty() ||
                            (!project->isActiveConfig("staticlib")))),
         src_incremental=false;

    ProStringList &bundledFiles = project->values("QMAKE_BUNDLED_FILES");

    t << "####### Compiler, tools and options\n\n";
    t << "CC            = " << var("QMAKE_CC") << endl;
    t << "CXX           = " << var("QMAKE_CXX") << endl;
    t << "DEFINES       = "
      << varGlue("PRL_EXPORT_DEFINES","-D"," -D"," ")
      << varGlue("DEFINES","-D"," -D","") << endl;
    t << "CFLAGS        = " << var("QMAKE_CFLAGS") << " $(DEFINES)\n";
    t << "CXXFLAGS      = " << var("QMAKE_CXXFLAGS") << " $(DEFINES)\n";
    t << "INCPATH       =";
    {
        QString isystem = var("QMAKE_CFLAGS_ISYSTEM");
        const ProStringList &incs = project->values("INCLUDEPATH");
        for(int i = 0; i < incs.size(); ++i) {
            ProString inc = escapeFilePath(incs.at(i));
            if (inc.isEmpty())
                continue;

            if (!isystem.isEmpty() && isSystemInclude(inc.toQString()))
                t << ' ' << isystem << ' ' << inc;
            else
                t << " -I" << inc;
        }
    }
    if(!project->isEmpty("QMAKE_FRAMEWORKPATH_FLAGS"))
       t << " " << var("QMAKE_FRAMEWORKPATH_FLAGS");
    t << endl;

    writeDefaultVariables(t);

    if(!project->isActiveConfig("staticlib")) {
        t << "LINK          = " << var("QMAKE_LINK") << endl;
        t << "LFLAGS        = " << var("QMAKE_LFLAGS") << endl;
        t << "LIBS          = $(SUBLIBS) " << var("QMAKE_LIBS") << " " << var("QMAKE_LIBS_PRIVATE") << endl;
    }

    t << "AR            = " << var("QMAKE_AR") << endl;
    t << "RANLIB        = " << var("QMAKE_RANLIB") << endl;
    if(project->isActiveConfig("compile_libtool"))
        t << "LIBTOOL       = " << var("QMAKE_LIBTOOL") << endl;
    t << "SED           = " << var("QMAKE_STREAM_EDITOR") << endl;
    t << "STRIP         = " << var("QMAKE_STRIP") << endl;

    t << endl;

    t << "####### Output directory\n\n";
    if (! project->values("OBJECTS_DIR").isEmpty())
        t << "OBJECTS_DIR   = " << var("OBJECTS_DIR") << endl;
    else
        t << "OBJECTS_DIR   = ./\n";
    t << endl;

    /* files */
    t << "####### Files\n\n";
    t << "SOURCES       = " << valList(escapeFilePaths(project->values("SOURCES"))) << " "
      << valList(escapeFilePaths(project->values("GENERATED_SOURCES"))) << endl;
    if(do_incremental) {
        const ProStringList &objs = project->values("OBJECTS");
        const ProStringList &incrs = project->values("QMAKE_INCREMENTAL");
        ProStringList incrs_out;
        t << "OBJECTS       = ";
        for (ProStringList::ConstIterator objit = objs.begin(); objit != objs.end(); ++objit) {
            bool increment = false;
            for (ProStringList::ConstIterator incrit = incrs.begin(); incrit != incrs.end(); ++incrit) {
                if ((*objit).toQString().indexOf(QRegExp((*incrit).toQString(), Qt::CaseSensitive,
                                                 QRegExp::Wildcard)) != -1) {
                    increment = true;
                    incrs_out.append((*objit));
                    break;
                }
            }
            if(!increment)
                t << "\\\n\t\t" << (*objit);
        }
        if(incrs_out.count() == objs.count()) { //we just switched places, no real incrementals to be done!
            t << escapeFilePaths(incrs_out).join(" \\\n\t\t") << endl;
        } else if(!incrs_out.count()) {
            t << endl;
        } else {
            src_incremental = true;
            t << endl;
            t << "INCREMENTAL_OBJECTS = " << escapeFilePaths(incrs_out).join(" \\\n\t\t") << endl;
        }
    } else {
        t << "OBJECTS       = " << valList(escapeFilePaths(project->values("OBJECTS"))) << endl;
    }
    if(do_incremental && !src_incremental)
        do_incremental = false;
    t << "DIST          = " << valList(fileFixify(project->values("DISTFILES").toQStringList())) << " "
                            << valList(escapeFilePaths(project->values("HEADERS"))) << " "
                            << valList(escapeFilePaths(project->values("SOURCES"))) << endl;
    t << "QMAKE_TARGET  = " << var("QMAKE_ORIG_TARGET") << endl;
    // The comment is important for mingw32-make.exe on Windows as otherwise trailing slashes
    // would be interpreted as line continuation. The lack of spacing between the value and the
    // comment is also important as otherwise quoted use of "$(DESTDIR)" would include this
    // spacing.
    t << "DESTDIR       = " << var("DESTDIR") << "#avoid trailing-slash linebreak\n";
    if(project->isActiveConfig("compile_libtool"))
        t << "TARGETL       = " << var("TARGET_la") << endl;
    t << "TARGET        = " << escapeFilePath(var("TARGET")) << endl;
    if(project->isActiveConfig("plugin")) {
        t << "TARGETD       = " << escapeFilePath(var("TARGET")) << endl;
    } else if(!project->isActiveConfig("staticlib") && project->values("QMAKE_APP_FLAG").isEmpty()) {
        t << "TARGETA       = " << escapeFilePath(var("TARGETA")) << endl;
        if(!project->isEmpty("QMAKE_BUNDLE")) {
            t << "TARGETD       = " << escapeFilePath(var("TARGET_x.y")) << endl;
            t << "TARGET0       = " << escapeFilePath(var("TARGET_")) << endl;
        } else if (!project->isActiveConfig("unversioned_libname")) {
            t << "TARGET0       = " << escapeFilePath(var("TARGET_")) << endl;
            if (project->isEmpty("QMAKE_HPUX_SHLIB")) {
                t << "TARGETD       = " << escapeFilePath(var("TARGET_x.y.z")) << endl;
                t << "TARGET1       = " << escapeFilePath(var("TARGET_x")) << endl;
                t << "TARGET2       = " << escapeFilePath(var("TARGET_x.y")) << endl;
            } else {
                t << "TARGETD       = " << escapeFilePath(var("TARGET_x")) << endl;
            }
        }
    }
    writeExtraCompilerVariables(t);
    writeExtraVariables(t);
    t << endl;

    // blasted includes
    const ProStringList &qeui = project->values("QMAKE_EXTRA_INCLUDES");
    ProStringList::ConstIterator it;
    for(it = qeui.begin(); it != qeui.end(); ++it)
        t << "include " << (*it) << endl;

    /* rules */
    t << "first: all\n";
    t << "####### Implicit rules\n\n";
    t << ".SUFFIXES: " << Option::obj_ext;
    for(QStringList::Iterator cit = Option::c_ext.begin(); cit != Option::c_ext.end(); ++cit)
        t << " " << (*cit);
    for(QStringList::Iterator cppit = Option::cpp_ext.begin(); cppit != Option::cpp_ext.end(); ++cppit)
        t << " " << (*cppit);
    t << endl << endl;
    for(QStringList::Iterator cppit = Option::cpp_ext.begin(); cppit != Option::cpp_ext.end(); ++cppit)
        t << (*cppit) << Option::obj_ext << ":\n\t" << var("QMAKE_RUN_CXX_IMP") << endl << endl;
    for(QStringList::Iterator cit = Option::c_ext.begin(); cit != Option::c_ext.end(); ++cit)
        t << (*cit) << Option::obj_ext << ":\n\t" << var("QMAKE_RUN_CC_IMP") << endl << endl;

    if(include_deps) {
        if (project->isActiveConfig("gcc_MD_depends")) {
            ProStringList objects = project->values("OBJECTS");
            for (ProStringList::Iterator it = objects.begin(); it != objects.end(); ++it) {
                QString d_file = (*it).toQString().replace(QRegExp(Option::obj_ext + "$"), ".d");
                t << "-include " << d_file << endl;
                project->values("QMAKE_DISTCLEAN") << d_file;
            }
        } else {
            QString cmd=var("QMAKE_CFLAGS_DEPS") + " ";
            cmd += varGlue("DEFINES","-D"," -D","") + varGlue("PRL_EXPORT_DEFINES"," -D"," -D","");
            if(!project->isEmpty("QMAKE_ABSOLUTE_SOURCE_PATH"))
                cmd += " -I" + project->first("QMAKE_ABSOLUTE_SOURCE_PATH") + " ";
            cmd += " $(INCPATH) " + varGlue("DEPENDPATH", "-I", " -I", "");
            ProString odir;
            if(!project->values("OBJECTS_DIR").isEmpty())
                odir = project->first("OBJECTS_DIR");

            QString pwd = escapeFilePath(fileFixify(qmake_getpwd()));

            t << "###### Dependencies\n\n";
            t << odir << ".deps/%.d: " << pwd << "/%.cpp\n\t";
            if(project->isActiveConfig("echo_depend_creation"))
                t << "@echo Creating depend for $<\n\t";
            t << mkdir_p_asstring("$(@D)", false) << "\n\t"
              << "@$(CXX) " << cmd << " $< | sed \"s,^\\($(*F).o\\):," << odir << "\\1:,g\" >$@\n\n";

            t << odir << ".deps/%.d: " << pwd << "/%.c\n\t";
            if(project->isActiveConfig("echo_depend_creation"))
                t << "@echo Creating depend for $<\n\t";
            t << mkdir_p_asstring("$(@D)", false) << "\n\t"
              << "@$(CC) " << cmd << " $< | sed \"s,^\\($(*F).o\\):," << odir << "\\1:,g\" >$@\n\n";

            static const char * const src[] = { "SOURCES", "GENERATED_SOURCES", 0 };
            for (int x = 0; src[x]; x++) {
                const ProStringList &l = project->values(src[x]);
                for (ProStringList::ConstIterator it = l.begin(); it != l.end(); ++it) {
                    if(!(*it).isEmpty()) {
                        QString d_file;
                        for(QStringList::Iterator cit = Option::c_ext.begin();
                            cit != Option::c_ext.end(); ++cit) {
                            if((*it).endsWith((*cit))) {
                                d_file = (*it).left((*it).length() - (*cit).length()).toQString();
                                break;
                            }
                        }
                        if(d_file.isEmpty()) {
                            for(QStringList::Iterator cppit = Option::cpp_ext.begin();
                                cppit != Option::cpp_ext.end(); ++cppit) {
                                if((*it).endsWith((*cppit))) {
                                    d_file = (*it).left((*it).length() - (*cppit).length()).toQString();
                                    break;
                                }
                            }
                        }

                        if(!d_file.isEmpty()) {
                            d_file = odir + ".deps/" + fileFixify(d_file, pwd, Option::output_dir) + ".d";
                            QStringList deps = findDependencies((*it).toQString()).filter(QRegExp(
                                        "((^|/)" + Option::h_moc_mod + "|" + Option::cpp_moc_ext + "$)"));
                            if(!deps.isEmpty())
                                t << d_file << ": " << deps.join(' ') << endl;
                            t << "-include " << d_file << endl;
                            project->values("QMAKE_DISTCLEAN") += d_file;
                        }
                    }
                }
            }
        }
    }

    t << "####### Build rules\n\n";
    if(!project->values("SUBLIBS").isEmpty()) {
        ProString libdir = "tmp/";
        if(!project->isEmpty("SUBLIBS_DIR"))
            libdir = project->first("SUBLIBS_DIR");
        t << "SUBLIBS       = ";
        const ProStringList &l = project->values("SUBLIBS");
        for (ProStringList::ConstIterator it = l.begin(); it != l.end(); ++it)
            t << libdir << project->first("QMAKE_PREFIX_STATICLIB") << (*it) << "."
              << project->first("QMAKE_EXTENSION_STATICLIB") << " ";
        t << endl << endl;
    }
    if ((project->isActiveConfig("depend_prl") || project->isActiveConfig("fast_depend_prl"))
        && !project->isEmpty("QMAKE_PRL_INTERNAL_FILES")) {
        const ProStringList &l = project->values("QMAKE_PRL_INTERNAL_FILES");
        ProStringList::ConstIterator it;
        for(it = l.begin(); it != l.end(); ++it) {
            QMakeMetaInfo libinfo(project);
            if (libinfo.readLib((*it).toQString()) && !libinfo.isEmpty("QMAKE_PRL_BUILD_DIR")) {
                ProString dir;
                int slsh = (*it).lastIndexOf(Option::dir_sep);
                if(slsh != -1)
                    dir = (*it).left(slsh + 1);
                QString targ = dir + libinfo.first("QMAKE_PRL_TARGET");
                target_deps += " " + targ;
                t << targ;
                if (project->isActiveConfig("fast_depend_prl"))
                    t << ":\n\t@echo \"Creating '";
                else
                    t << ": FORCE\n\t@echo \"Creating/updating '";
                t << targ << "'\"\n\t"
                  << "(cd " << libinfo.first("QMAKE_PRL_BUILD_DIR") << ";"
                  << "$(MAKE))\n";
            }
        }
    }
    QString allDeps;
    if (!project->values("QMAKE_APP_FLAG").isEmpty() || project->first("TEMPLATE") == "aux") {
        QString destdir = project->first("DESTDIR").toQString();
        if(!project->isEmpty("QMAKE_BUNDLE")) {
            QString bundle_loc = project->first("QMAKE_BUNDLE_LOCATION").toQString();
            if(!bundle_loc.isEmpty() && !bundle_loc.startsWith("/"))
                bundle_loc.prepend("/");
            if(!bundle_loc.endsWith("/"))
                bundle_loc += "/";
            destdir += project->first("QMAKE_BUNDLE") + bundle_loc;
        }
        if(do_incremental) {
            //incremental target
            QString incr_target = var("TARGET") + "_incremental";
            if(incr_target.indexOf(Option::dir_sep) != -1)
                incr_target = incr_target.right(incr_target.length() -
                                                (incr_target.lastIndexOf(Option::dir_sep) + 1));
            QString incr_deps, incr_objs;
            if(project->first("QMAKE_INCREMENTAL_STYLE") == "ld") {
                QString incr_target_dir = var("OBJECTS_DIR") + incr_target + Option::obj_ext;
                //actual target
                t << incr_target_dir << ": $(OBJECTS)\n\t"
                  << "ld -r  -o "<< incr_target_dir << " $(OBJECTS)\n";
                //communicated below
                deps.prepend(incr_target_dir + " ");
                incr_deps = "$(INCREMENTAL_OBJECTS)";
                if(!incr_objs.isEmpty())
                    incr_objs += " ";
                incr_objs += incr_target_dir;
            } else {
                //actual target
                QString incr_target_dir = var("DESTDIR") + "lib" + incr_target + "." +
                                          project->values("QMAKE_EXTENSION_SHLIB").first();
                QString incr_lflags = var("QMAKE_LFLAGS_SHLIB") + " ";
                if(project->isActiveConfig("debug"))
                    incr_lflags += var("QMAKE_LFLAGS_DEBUG");
                else if (project->isActiveConfig("debug_info"))
                    incr_lflags += var("QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO");
                else
                    incr_lflags += var("QMAKE_LFLAGS_RELEASE");
                t << incr_target_dir << ": $(INCREMENTAL_OBJECTS)\n\t";
                if(!destdir.isEmpty())
                    t << "\n\t" << mkdir_p_asstring(destdir) << "\n\t";
                t << "$(LINK) " << incr_lflags << " -o "<< incr_target_dir <<
                    " $(INCREMENTAL_OBJECTS)\n";
                //communicated below
                if(!destdir.isEmpty()) {
                    if(!incr_objs.isEmpty())
                        incr_objs += " ";
                    incr_objs += "-L" + destdir;
                } else {
                    if(!incr_objs.isEmpty())
                        incr_objs += " ";
                    incr_objs += "-L" + qmake_getpwd();
                }
                if(!incr_objs.isEmpty())
                    incr_objs += " ";
                incr_objs += " -l" + incr_target;
                deps.prepend(incr_target_dir + " ");
                incr_deps = "$(OBJECTS)";
            }

            //real target
            t << var("TARGET") << ": " << var("PRE_TARGETDEPS") << " " << incr_deps << " " << target_deps
              << " " << var("POST_TARGETDEPS") << "\n\t";
            if(!destdir.isEmpty())
                t << "\n\t" << mkdir_p_asstring(destdir) << "\n\t";
            if(!project->isEmpty("QMAKE_PRE_LINK"))
                t << var("QMAKE_PRE_LINK") << "\n\t";
            t << "$(LINK) $(LFLAGS) " << var("QMAKE_LINK_O_FLAG") << "$(TARGET) " << incr_deps << " " << incr_objs << " $(OBJCOMP) $(LIBS)";
            if(!project->isEmpty("QMAKE_POST_LINK"))
                t << "\n\t" << var("QMAKE_POST_LINK");
            t << endl << endl;
        } else {
            t << "$(TARGET): " << var("PRE_TARGETDEPS") << " $(OBJECTS) "
              << target_deps << " " << var("POST_TARGETDEPS") << "\n\t";
            if (project->first("TEMPLATE") != "aux") {
                if (!destdir.isEmpty())
                    t << mkdir_p_asstring(destdir) << "\n\t";
                if (!project->isEmpty("QMAKE_PRE_LINK"))
                    t << var("QMAKE_PRE_LINK") << "\n\t";
                t << "$(LINK) $(LFLAGS) " << var("QMAKE_LINK_O_FLAG") << "$(TARGET) $(OBJECTS) $(OBJCOMP) $(LIBS)";
                if (!project->isEmpty("QMAKE_POST_LINK"))
                    t << "\n\t" << var("QMAKE_POST_LINK");
            }
            t << endl << endl;
        }
        allDeps = " $(TARGET)";
    } else if(!project->isActiveConfig("staticlib")) {
        QString destdir = unescapeFilePath(project->first("DESTDIR").toQString()), incr_deps;
        if(!project->isEmpty("QMAKE_BUNDLE")) {
            QString bundle_loc = project->first("QMAKE_BUNDLE_LOCATION").toQString();
            if(!bundle_loc.isEmpty() && !bundle_loc.startsWith("/"))
                bundle_loc.prepend("/");
            if(!bundle_loc.endsWith("/"))
                bundle_loc += "/";
            destdir += project->first("QMAKE_BUNDLE") + bundle_loc;
        }
        destdir = escapeFilePath(destdir);

        if(do_incremental) {
            ProString s_ext = project->first("QMAKE_EXTENSION_SHLIB");
            QString incr_target = var("QMAKE_ORIG_TARGET").replace(
                QRegExp("\\." + s_ext), "").replace(QRegExp("^lib"), "") + "_incremental";
            if(incr_target.indexOf(Option::dir_sep) != -1)
                incr_target = incr_target.right(incr_target.length() -
                                                (incr_target.lastIndexOf(Option::dir_sep) + 1));
            incr_target = escapeFilePath(incr_target);

            if(project->first("QMAKE_INCREMENTAL_STYLE") == "ld") {
                QString incr_target_dir = escapeFilePath(var("OBJECTS_DIR") + incr_target + Option::obj_ext);
                //actual target
                const QString link_deps = "$(OBJECTS) ";
                t << incr_target_dir << ": " << link_deps << "\n\t"
                  << "ld -r  -o " << incr_target_dir << " " << link_deps << endl;
                //communicated below
                ProStringList &cmd = project->values("QMAKE_LINK_SHLIB_CMD");
                cmd[0] = cmd.at(0).toQString().replace("$(OBJECTS) ", "$(INCREMENTAL_OBJECTS)"); //ick
                cmd.append(incr_target_dir);
                deps.prepend(incr_target_dir + " ");
                incr_deps = "$(INCREMENTAL_OBJECTS)";
            } else {
                //actual target
                QString incr_target_dir = escapeFilePath(destdir + "lib" + incr_target + "." + s_ext);
                QString incr_lflags = var("QMAKE_LFLAGS_SHLIB") + " ";
                if(!project->isEmpty("QMAKE_LFLAGS_INCREMENTAL"))
                    incr_lflags += var("QMAKE_LFLAGS_INCREMENTAL") + " ";
                if(project->isActiveConfig("debug"))
                    incr_lflags += var("QMAKE_LFLAGS_DEBUG");
                else if (project->isActiveConfig("debug_info"))
                    incr_lflags += var("QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO");
                else
                    incr_lflags += var("QMAKE_LFLAGS_RELEASE");
                t << incr_target_dir << ": $(INCREMENTAL_OBJECTS)\n\t";
                if(!destdir.isEmpty())
                    t << mkdir_p_asstring(destdir, false) << "\n\t";
                t << "$(LINK) " << incr_lflags << " " << var("QMAKE_LINK_O_FLAG") << incr_target_dir <<
                    " $(INCREMENTAL_OBJECTS)\n";
                //communicated below
                ProStringList &cmd = project->values("QMAKE_LINK_SHLIB_CMD");
                if(!destdir.isEmpty())
                    cmd.append(" -L" + destdir);
                cmd.append(" -l" + incr_target);
                deps.prepend(incr_target_dir + " ");
                incr_deps = "$(OBJECTS)";
            }

            //real target
            t << destdir << "$(TARGET): " << var("PRE_TARGETDEPS") << " "
              << incr_deps << " $(SUBLIBS) " << target_deps << " " << var("POST_TARGETDEPS");
        } else {
            t << destdir << "$(TARGET): " << var("PRE_TARGETDEPS")
              << " $(OBJECTS) $(SUBLIBS) $(OBJCOMP) " << target_deps
              << " " << var("POST_TARGETDEPS");
        }
        allDeps = ' ' + destdir + "$(TARGET)";
        if(!destdir.isEmpty())
            t << "\n\t" << mkdir_p_asstring(destdir, false);
        if(!project->isEmpty("QMAKE_PRE_LINK"))
            t << "\n\t" << var("QMAKE_PRE_LINK");

        if(project->isActiveConfig("compile_libtool")) {
            t << "\n\t"
              << var("QMAKE_LINK_SHLIB_CMD");
        } else if(project->isActiveConfig("plugin")) {
            t << "\n\t"
              << "-$(DEL_FILE) $(TARGET)\n\t"
              << var("QMAKE_LINK_SHLIB_CMD");
            if(!destdir.isEmpty())
                t << "\n\t"
                  << "-$(MOVE) $(TARGET) " << destdir << " ";
            if(!project->isEmpty("QMAKE_POST_LINK"))
                t << "\n\t" << var("QMAKE_POST_LINK");
            t << endl << endl;
        } else if(!project->isEmpty("QMAKE_BUNDLE")) {
            QString currentLink = destdir + "Versions/Current";
            bundledFiles << currentLink << destdir + "$(TARGET)";
            t << "\n\t"
              << "-$(DEL_FILE) $(TARGET) $(TARGET0) $(DESTDIR)$(TARGET0)\n\t"
              << var("QMAKE_LINK_SHLIB_CMD") << "\n\t"
              << mkdir_p_asstring("\"`dirname $(DESTDIR)$(TARGETD)`\"", false) << "\n\t"
              << "-$(MOVE) $(TARGET) $(DESTDIR)$(TARGETD)\n\t"
              << mkdir_p_asstring("\"`dirname $(DESTDIR)$(TARGET0)`\"", false) << "\n\t"
              << varGlue("QMAKE_LN_SHLIB", "-", " ",
                         " Versions/Current/$(TARGET) $(DESTDIR)$(TARGET0)") << "\n\t"
              << "-$(DEL_FILE) " << currentLink << "\n\t"
              << varGlue("QMAKE_LN_SHLIB","-"," ", " " + project->first("QMAKE_FRAMEWORK_VERSION") +
                         " " + currentLink) << "\n\t";
            if(!project->isEmpty("QMAKE_POST_LINK"))
                t << "\n\t" << var("QMAKE_POST_LINK");
            t << endl << endl;
        } else if(project->isEmpty("QMAKE_HPUX_SHLIB")) {
            t << "\n\t";

            if (!project->isActiveConfig("unversioned_libname"))
                t << "-$(DEL_FILE) $(TARGET) $(TARGET0) $(TARGET1) $(TARGET2)";
            else
                t << "-$(DEL_FILE) $(TARGET)";

            t << "\n\t" << var("QMAKE_LINK_SHLIB_CMD");

            if (!project->isActiveConfig("unversioned_libname")) {
                t << "\n\t"
                  << varGlue("QMAKE_LN_SHLIB","-"," "," $(TARGET) $(TARGET0)") << "\n\t"
                  << varGlue("QMAKE_LN_SHLIB","-"," "," $(TARGET) $(TARGET1)") << "\n\t"
                  << varGlue("QMAKE_LN_SHLIB","-"," "," $(TARGET) $(TARGET2)");
            }
            if (!destdir.isEmpty()) {
                t << "\n\t"
                  << "-$(DEL_FILE) " << destdir << "$(TARGET)\n\t"
                  << "-$(MOVE) $(TARGET)  " << destdir << " ";

                if (!project->isActiveConfig("unversioned_libname")) {
                    t << "\n\t"
                      << "-$(DEL_FILE) " << destdir << "$(TARGET0)\n\t"
                      << "-$(DEL_FILE) " << destdir << "$(TARGET1)\n\t"
                      << "-$(DEL_FILE) " << destdir << "$(TARGET2)\n\t"
                      << "-$(MOVE) $(TARGET0) " << destdir << " \n\t"
                      << "-$(MOVE) $(TARGET1) " << destdir << " \n\t"
                      << "-$(MOVE) $(TARGET2) " << destdir << " ";
                }
            }
            if(!project->isEmpty("QMAKE_POST_LINK"))
                t << "\n\t" << var("QMAKE_POST_LINK");
            t << endl << endl;
        } else {
            t << "\n\t"
              << "-$(DEL_FILE) $(TARGET) $(TARGET0)\n\t"
              << var("QMAKE_LINK_SHLIB_CMD") << "\n\t";
            t << varGlue("QMAKE_LN_SHLIB",""," "," $(TARGET) $(TARGET0)");
            if(!destdir.isEmpty())
                t  << "\n\t"
                   << "-$(DEL_FILE) " << destdir << "$(TARGET)\n\t"
                   << "-$(DEL_FILE) " << destdir << "$(TARGET0)\n\t"
                   << "-$(MOVE) $(TARGET)  " << destdir << " \n\t"
                   << "-$(MOVE) $(TARGET0) " << destdir << " \n\t";
            if(!project->isEmpty("QMAKE_POST_LINK"))
                t << "\n\t" << var("QMAKE_POST_LINK");
            t << endl << endl;
        }
        t << endl << endl;

        if (! project->isActiveConfig("plugin")) {
            t << "staticlib: $(TARGETA)\n\n";
            t << "$(TARGETA): " << var("PRE_TARGETDEPS") << " $(OBJECTS) $(OBJCOMP)";
            if(do_incremental)
                t << " $(INCREMENTAL_OBJECTS)";
            t << " " << var("POST_TARGETDEPS") << "\n\t"
              << "-$(DEL_FILE) $(TARGETA) \n\t"
              << var("QMAKE_AR_CMD");
            if(do_incremental)
                t << " $(INCREMENTAL_OBJECTS)";
            if(!project->isEmpty("QMAKE_RANLIB"))
                t << "\n\t$(RANLIB) $(TARGETA)";
            t << endl << endl;
        }
    } else {
        QString destdir = project->first("DESTDIR").toQString();
        allDeps = ' ' + destdir + "$(TARGET)"
                  + varGlue("QMAKE_AR_SUBLIBS", ' ' + destdir, ' ' + destdir, "");
        t << "staticlib: " << destdir << "$(TARGET)\n\n";
        if(project->isEmpty("QMAKE_AR_SUBLIBS")) {
            t << destdir << "$(TARGET): " << var("PRE_TARGETDEPS")
              << " $(OBJECTS) $(OBJCOMP) " << var("POST_TARGETDEPS") << "\n\t";
            if(!destdir.isEmpty())
                t << mkdir_p_asstring(destdir) << "\n\t";
            t << "-$(DEL_FILE) $(TARGET)\n\t"
              << var("QMAKE_AR_CMD") << "\n";
            if(!project->isEmpty("QMAKE_POST_LINK"))
                t << "\t" << var("QMAKE_POST_LINK") << "\n";
            if(!project->isEmpty("QMAKE_RANLIB"))
                t << "\t$(RANLIB) $(TARGET)\n";
            if(!destdir.isEmpty())
                t << "\t-$(DEL_FILE) " << destdir << "$(TARGET)\n"
                  << "\t-$(MOVE) $(TARGET) " << destdir << " \n";
        } else {
            int max_files = project->first("QMAKE_MAX_FILES_PER_AR").toInt();
            ProStringList objs = project->values("OBJECTS") + project->values("OBJCOMP"),
                        libs = project->values("QMAKE_AR_SUBLIBS");
            libs.prepend("$(TARGET)");
            for (ProStringList::Iterator libit = libs.begin(), objit = objs.begin();
                 libit != libs.end(); ++libit) {
                ProStringList build;
                for(int cnt = 0; cnt < max_files && objit != objs.end(); ++objit, cnt++)
                    build << (*objit);
                QString ar;
                if((*libit) == "$(TARGET)") {
                    t << destdir << "$(TARGET): " << var("PRE_TARGETDEPS")
                      << " " << var("POST_TARGETDEPS") << valList(build) << "\n\t";
                    ar = project->first("QMAKE_AR_CMD").toQString();
                    ar = ar.replace("$(OBJECTS)", build.join(' '));
                } else {
                    t << (*libit) << ": " << valList(build) << "\n\t";
                    ar = "$(AR) " + (*libit) + " " + build.join(' ');
                }
                if(!destdir.isEmpty())
                    t << mkdir_p_asstring(destdir) << "\n\t";
                t << "-$(DEL_FILE) " << (*libit) << "\n\t"
                  << ar << "\n";
                if(!project->isEmpty("QMAKE_POST_LINK"))
                    t << "\t" << var("QMAKE_POST_LINK") << "\n";
                if(!project->isEmpty("QMAKE_RANLIB"))
                    t << "\t$(RANLIB) " << (*libit) << "\n";
                if(!destdir.isEmpty())
                    t << "\t-$(DEL_FILE) " << destdir << (*libit) << "\n"
                      << "\t-$(MOVE) " << (*libit) << " " << destdir << " \n";
            }
        }
        t << endl << endl;
    }

    writeMakeQmake(t);
    if(project->isEmpty("QMAKE_FAILED_REQUIREMENTS") && !project->isActiveConfig("no_autoqmake")) {
        QStringList meta_files;
        if(project->isActiveConfig("create_libtool") && project->first("TEMPLATE") == "lib" &&
           !project->isActiveConfig("compile_libtool")) { //libtool
            meta_files += libtoolFileName();
        }
        if(project->isActiveConfig("create_pc") && project->first("TEMPLATE") == "lib") { //pkg-config
            meta_files += pkgConfigFileName();
        }
        if(!meta_files.isEmpty())
            t << escapeDependencyPaths(meta_files).join(" ") << ": \n\t"
              << "@$(QMAKE) -prl " << buildArgs() << " " << project->projectFile() << endl;
    }

    if (!project->isEmpty("QMAKE_BUNDLE")) {
        QHash<QString, QString> symlinks;
        ProStringList &alldeps = project->values("ALL_DEPS");
        QString bundle_dir = project->first("DESTDIR") + project->first("QMAKE_BUNDLE") + "/";
        if (!project->first("QMAKE_PKGINFO").isEmpty()) {
            ProString pkginfo = escapeFilePath(project->first("QMAKE_PKGINFO"));
            bundledFiles << pkginfo;
            alldeps << pkginfo;
            QString destdir = bundle_dir + "Contents";
            t << pkginfo << ": \n\t";
            if (!destdir.isEmpty())
                t << mkdir_p_asstring(destdir) << "\n\t";
            t << "@$(DEL_FILE) " << pkginfo << "\n\t"
              << "@echo \"APPL"
              << (project->isEmpty("QMAKE_PKGINFO_TYPEINFO")
                  ? QString::fromLatin1("????") : project->first("QMAKE_PKGINFO_TYPEINFO").left(4))
              << "\" >" << pkginfo << endl;
        }
        if (!project->first("QMAKE_BUNDLE_RESOURCE_FILE").isEmpty()) {
            ProString resources = escapeFilePath(project->first("QMAKE_BUNDLE_RESOURCE_FILE"));
            bundledFiles << resources;
            alldeps << resources;
            QString destdir = bundle_dir + "Contents/Resources";
            t << resources << ": \n\t";
            t << mkdir_p_asstring(destdir) << "\n\t";
            t << "@touch " << resources << "\n\t\n";
        }
        //copy the plist
        while (!project->isActiveConfig("no_plist")) {  // 'while' just to be able to 'break'
            QString info_plist = escapeFilePath(fileFixify(project->first("QMAKE_INFO_PLIST").toQString()));
            if (info_plist.isEmpty())
                info_plist = specdir() + QDir::separator() + "Info.plist." + project->first("TEMPLATE");
            if (!exists(Option::fixPathToLocalOS(info_plist))) {
                warn_msg(WarnLogic, "Could not resolve Info.plist: '%s'. Check if QMAKE_INFO_PLIST points to a valid file.",
                         info_plist.toLatin1().constData());
                break;
            }
            bool isApp = (project->first("TEMPLATE") == "app");
            QString info_plist_out = escapeFilePath(
                    bundle_dir + (isApp ? "Contents/Info.plist"
                                        : "Versions/" + project->first("QMAKE_FRAMEWORK_VERSION")
                                          + "/Resources/Info.plist"));
            bundledFiles << info_plist_out;
            alldeps << info_plist_out;
            QString destdir = info_plist_out.section(Option::dir_sep, 0, -2);
            t << info_plist_out << ": \n\t";
            if (!destdir.isEmpty())
                t << mkdir_p_asstring(destdir, false) << "\n\t";
            ProStringList commonSedArgs;
            if (!project->values("VERSION").isEmpty()) {
                commonSedArgs << "-e \"s,@SHORT_VERSION@," << project->first("VER_MAJ") << "."
                                                           << project->first("VER_MIN") << ",g\" ";
                commonSedArgs << "-e \"s,@FULL_VERSION@," << project->first("VER_MAJ") << "."
                                                          << project->first("VER_MIN") << "."
                                                          << project->first("VER_PAT") << ",g\" ";
            }
            commonSedArgs << "-e \"s,@TYPEINFO@,"<< (project->isEmpty("QMAKE_PKGINFO_TYPEINFO") ?
                       QString::fromLatin1("????") : project->first("QMAKE_PKGINFO_TYPEINFO").left(4)) << ",g\" ";

            QString bundlePrefix = project->first("QMAKE_TARGET_BUNDLE_PREFIX").toQString();
            if (bundlePrefix.isEmpty())
                bundlePrefix = "com.yourcompany";
            if (bundlePrefix.endsWith("."))
                bundlePrefix.chop(1);
            QString bundleIdentifier =  bundlePrefix + "." + var("QMAKE_BUNDLE");
            if (bundleIdentifier.endsWith(".app"))
                bundleIdentifier.chop(4);
            if (bundleIdentifier.endsWith(".framework"))
                bundleIdentifier.chop(10);
            commonSedArgs << "-e \"s,@BUNDLEIDENTIFIER@," << bundleIdentifier << ",g\" ";

            if (isApp) {
                QString icon = fileFixify(var("ICON"));
                t << "@$(DEL_FILE) " << info_plist_out << "\n\t"
                  << "@sed ";
                foreach (const ProString &arg, commonSedArgs)
                    t << arg;
                t << "-e \"s,@ICON@," << icon.section(Option::dir_sep, -1) << ",g\" "
                  << "-e \"s,@EXECUTABLE@," << var("QMAKE_ORIG_TARGET") << ",g\" "
                  << "-e \"s,@TYPEINFO@,"<< (project->isEmpty("QMAKE_PKGINFO_TYPEINFO") ?
                             QString::fromLatin1("????") : project->first("QMAKE_PKGINFO_TYPEINFO").left(4)) << ",g\" "
                  << "" << info_plist << " >" << info_plist_out << endl;
                //copy the icon
                if (!project->isEmpty("ICON")) {
                    QString dir = bundle_dir + "Contents/Resources/";
                    const QString icon_path = escapeFilePath(dir + icon.section(Option::dir_sep, -1));
                    bundledFiles << icon_path;
                    alldeps << icon_path;
                    t << icon_path << ": " << icon << "\n\t"
                      << mkdir_p_asstring(dir) << "\n\t"
                      << "@$(DEL_FILE) " << icon_path << "\n\t"
                      << "@$(COPY_FILE) " << escapeFilePath(icon) << " " << icon_path << endl;
                }
            } else {
                symlinks[bundle_dir + "Resources"] = "Versions/Current/Resources";
                t << "@$(DEL_FILE) " << info_plist_out << "\n\t"
                  << "@sed ";
                foreach (const ProString &arg, commonSedArgs)
                    t << arg;
                t << "-e \"s,@LIBRARY@," << var("QMAKE_ORIG_TARGET") << ",g\" "
                  << "-e \"s,@TYPEINFO@,"
                  << (project->isEmpty("QMAKE_PKGINFO_TYPEINFO") ?
                      QString::fromLatin1("????") : project->first("QMAKE_PKGINFO_TYPEINFO").left(4)) << ",g\" "
                  << "" << info_plist << " >" << info_plist_out << endl;
            }
            break;
        } // project->isActiveConfig("no_plist")
        //copy other data
        if(!project->isEmpty("QMAKE_BUNDLE_DATA")) {
            const ProStringList &bundle_data = project->values("QMAKE_BUNDLE_DATA");
            for(int i = 0; i < bundle_data.count(); i++) {
                const ProStringList &files = project->values(ProKey(bundle_data[i] + ".files"));
                QString path = bundle_dir;
                const ProKey vkey(bundle_data[i] + ".version");
                const ProKey pkey(bundle_data[i] + ".path");
                if (!project->isEmpty(vkey)) {
                    QString version = project->first(vkey) + "/" +
                                      project->first("QMAKE_FRAMEWORK_VERSION") + "/";
                    ProString name = project->first(pkey);
                    int pos = name.indexOf('/');
                    if (pos > 0)
                        name = name.mid(0, pos);
                    symlinks[Option::fixPathToLocalOS(path + name)] =
                            project->first(vkey) + "/Current/" + name;
                    path += version;
                }
                path += project->first(pkey).toQString();
                path = Option::fixPathToLocalOS(path);
                for(int file = 0; file < files.count(); file++) {
                    QString fn = files.at(file).toQString();
                    QString src = fileFixify(fn, FileFixifyAbsolute);
                    if (!QFile::exists(src))
                        src = fn;
                    src = escapeFilePath(src);
                    const QString dst = escapeFilePath(path + Option::dir_sep + fileInfo(fn).fileName());
                    bundledFiles << dst;
                    alldeps << dst;
                    t << dst << ": " << src << "\n\t"
                      << mkdir_p_asstring(path) << "\n\t";
                    QFileInfo fi(fileInfo(fn));
                    if(fi.isDir())
                        t << "@$(DEL_FILE) -r " << dst << "\n\t"
                          << "@$(COPY_DIR) " << src << " " << dst << endl;
                    else
                        t << "@$(DEL_FILE) " << dst << "\n\t"
                          << "@$(COPY_FILE) " << src << " " << dst << endl;
                }
            }
        }
        QHash<QString, QString>::ConstIterator symIt = symlinks.constBegin(),
                                               symEnd = symlinks.constEnd();
        for (; symIt != symEnd; ++symIt) {
            bundledFiles << symIt.key();
            alldeps << symIt.key();
            t << symIt.key() << ":\n\t"
              << mkdir_p_asstring(bundle_dir) << "\n\t"
              << "@$(SYMLINK) " << symIt.value() << " " << bundle_dir << endl;
        }
    }

    t << endl << "all: " << escapeDependencyPath(deps)
      << valGlue(escapeDependencyPaths(project->values("ALL_DEPS")), " \\\n\t\t", " \\\n\t\t", "")
      << allDeps << endl << endl;

    t << "dist: distdir FORCE\n\t";
    t << "(cd `dirname $(DISTDIR)` && $(TAR) $(DISTNAME).tar $(DISTNAME) && $(COMPRESS) $(DISTNAME).tar)"
         " && $(MOVE) `dirname $(DISTDIR)`" << Option::dir_sep << "$(DISTNAME).tar.gz ."
         " && $(DEL_FILE) -r $(DISTDIR)";
    t << endl << endl;

    t << "distdir: FORCE\n\t"
      << mkdir_p_asstring("$(DISTDIR)", false) << "\n\t"
      << "$(COPY_FILE) --parents $(DIST) $(DISTDIR)" << Option::dir_sep << endl;
    if(!project->isEmpty("QMAKE_EXTRA_COMPILERS")) {
        const ProStringList &quc = project->values("QMAKE_EXTRA_COMPILERS");
        for (ProStringList::ConstIterator it = quc.begin(); it != quc.end(); ++it) {
            const ProStringList &var = project->values(ProKey(*it + ".input"));
            for (ProStringList::ConstIterator var_it = var.begin(); var_it != var.end(); ++var_it) {
                const ProStringList &val = project->values((*var_it).toKey());
                if(val.isEmpty())
                    continue;
                t << "\t$(COPY_FILE) --parents " << val.join(' ') << " $(DISTDIR)" << Option::dir_sep << endl;
            }
        }
    }
    if(!project->isEmpty("TRANSLATIONS"))
        t << "\t$(COPY_FILE) --parents " << var("TRANSLATIONS") << " $(DISTDIR)" << Option::dir_sep << endl;
    t << endl << endl;

    QString clean_targets = "compiler_clean " + var("CLEAN_DEPS");
    if(do_incremental) {
        t << "incrclean:\n";
        if(src_incremental)
            t << "\t-$(DEL_FILE) $(INCREMENTAL_OBJECTS)\n";
        t << endl;
    }

    t << "clean:" << clean_targets << "\n\t";
    if(!project->isEmpty("OBJECTS")) {
        if(project->isActiveConfig("compile_libtool"))
            t << "-$(LIBTOOL) --mode=clean $(DEL_FILE) $(OBJECTS)\n\t";
        else
            t << "-$(DEL_FILE) $(OBJECTS)\n\t";
    }
    if(doPrecompiledHeaders() && !project->isEmpty("PRECOMPILED_HEADER")) {
        ProStringList precomp_files;
        ProString precomph_out_dir;

        if(!project->isEmpty("PRECOMPILED_DIR"))
            precomph_out_dir = project->first("PRECOMPILED_DIR");
        precomph_out_dir += project->first("QMAKE_ORIG_TARGET");
        if (!project->isActiveConfig("clang_pch_style"))
            precomph_out_dir += project->first("QMAKE_PCH_OUTPUT_EXT");

        if (project->isActiveConfig("icc_pch_style")) {
            // icc style
            ProString pchBaseName = project->first("QMAKE_ORIG_TARGET");
            ProString pchOutput;
            if(!project->isEmpty("PRECOMPILED_DIR"))
                pchOutput = project->first("PRECOMPILED_DIR");
            pchOutput += pchBaseName + project->first("QMAKE_PCH_OUTPUT_EXT");
            ProString sourceFile = pchOutput + Option::cpp_ext.first();
            ProString objectFile = createObjectList(ProStringList(sourceFile)).first();

            precomp_files << precomph_out_dir << sourceFile << objectFile;
        } else {
            // gcc style (including clang_pch_style)
            precomph_out_dir += Option::dir_sep;

            ProString header_prefix = project->first("QMAKE_PRECOMP_PREFIX");
            ProString header_suffix = project->isActiveConfig("clang_pch_style")
                               ? project->first("QMAKE_PCH_OUTPUT_EXT") : "";

            if(!project->isEmpty("QMAKE_CFLAGS_PRECOMPILE"))
                precomp_files += precomph_out_dir + header_prefix + "c" + header_suffix;
            if(!project->isEmpty("QMAKE_CXXFLAGS_PRECOMPILE"))
                precomp_files += precomph_out_dir + header_prefix + "c++" + header_suffix;
            if(project->isActiveConfig("objective_c")) {
                if(!project->isEmpty("QMAKE_OBJCFLAGS_PRECOMPILE"))
                    precomp_files += precomph_out_dir + header_prefix + "objective-c" + header_suffix;
                if(!project->isEmpty("QMAKE_OBJCXXFLAGS_PRECOMPILE"))
                    precomp_files += precomph_out_dir + header_prefix + "objective-c++" + header_suffix;
            }
        }
        t << "-$(DEL_FILE) " << precomp_files.join(' ') << "\n\t";
    }
    if(!project->isEmpty("IMAGES"))
        t << varGlue("QMAKE_IMAGE_COLLECTION", "\t-$(DEL_FILE) ", " ", "") << "\n\t";
    if(src_incremental)
        t << "-$(DEL_FILE) $(INCREMENTAL_OBJECTS)\n\t";
    t << varGlue("QMAKE_CLEAN","-$(DEL_FILE) "," ","\n\t")
      << "-$(DEL_FILE) *~ core *.core\n"
      << varGlue("CLEAN_FILES","\t-$(DEL_FILE) "," ","") << endl << endl;

    ProString destdir = project->first("DESTDIR");
    if (!destdir.isEmpty() && !destdir.endsWith(Option::dir_sep))
        destdir += Option::dir_sep;
    t << "distclean: clean " << var("DISTCLEAN_DEPS") << '\n';
    if(!project->isEmpty("QMAKE_BUNDLE")) {
        QString bundlePath = escapeFilePath(destdir + project->first("QMAKE_BUNDLE"));
        t << "\t-$(DEL_FILE) -r " << bundlePath << endl;
    } else if(project->isActiveConfig("compile_libtool")) {
        t << "\t-$(LIBTOOL) --mode=clean $(DEL_FILE) $(TARGET)\n";
    } else if (project->isActiveConfig("staticlib")) {
        t << "\t-$(DEL_FILE) " << destdir << "$(TARGET) \n";
    } else if (project->values("QMAKE_APP_FLAG").isEmpty() && !project->isActiveConfig("plugin")) {
        t << "\t-$(DEL_FILE) " << destdir << "$(TARGET) \n";
        if (!project->isActiveConfig("unversioned_libname")) {
            t << "\t-$(DEL_FILE) " << destdir << "$(TARGET0) " << destdir << "$(TARGET1) "
              << destdir << "$(TARGET2) $(TARGETA)\n";
        } else {
            t << "\t-$(DEL_FILE) $(TARGETA)\n";
        }
    } else {
        t << "\t-$(DEL_FILE) $(TARGET) \n";
    }
    t << varGlue("QMAKE_DISTCLEAN","\t-$(DEL_FILE) "," ","\n");
    {
        QString ofile = Option::fixPathToTargetOS(fileFixify(Option::output.fileName()));
        if(!ofile.isEmpty())
            t << "\t-$(DEL_FILE) " << ofile << endl;
    }
    t << endl << endl;

    t << "####### Sub-libraries\n\n";
    if (!project->values("SUBLIBS").isEmpty()) {
        ProString libdir = "tmp/";
        if (!project->isEmpty("SUBLIBS_DIR"))
            libdir = project->first("SUBLIBS_DIR");
        const ProStringList &l = project->values("SUBLIBS");
        for (it = l.begin(); it != l.end(); ++it)
            t << libdir << project->first("QMAKE_PREFIX_STATICLIB") << (*it) << "."
              << project->first("QMAKE_EXTENSION_STATICLIB") << ":\n\t"
              << var(ProKey("MAKELIB" + *it)) << endl << endl;
    }

    if(doPrecompiledHeaders() && !project->isEmpty("PRECOMPILED_HEADER")) {
        QString pchInput = project->first("PRECOMPILED_HEADER").toQString();
        t << "###### Precompiled headers\n";
        QString comps[] = { "C", "CXX", "OBJC", "OBJCXX", QString() };
        for(int i = 0; !comps[i].isNull(); i++) {
            QString pchFlags = var(ProKey("QMAKE_" + comps[i] + "FLAGS_PRECOMPILE"));
            if(pchFlags.isEmpty())
                continue;

            QString cflags;
            if(comps[i] == "OBJC" || comps[i] == "OBJCXX")
                cflags += " $(CFLAGS)";
            else
                cflags += " $(" + comps[i] + "FLAGS)";

            ProString pchBaseName = project->first("QMAKE_ORIG_TARGET");
            ProString pchOutput;
            if(!project->isEmpty("PRECOMPILED_DIR"))
                pchOutput = project->first("PRECOMPILED_DIR");
            pchOutput += pchBaseName;
            if (!project->isActiveConfig("clang_pch_style"))
                pchOutput += project->first("QMAKE_PCH_OUTPUT_EXT");

            if (project->isActiveConfig("icc_pch_style")) {
                // icc style
                QString sourceFile = pchOutput + Option::cpp_ext.first();
                QString objectFile = createObjectList(ProStringList(sourceFile)).first().toQString();
                t << pchOutput << ": " << pchInput << " " << findDependencies(pchInput).join(" \\\n\t\t")
                  << "\n\techo \"// Automatically generated, do not modify\" > " << sourceFile
                  << "\n\trm -f " << pchOutput;

                pchFlags = pchFlags.replace("${QMAKE_PCH_TEMP_SOURCE}", sourceFile)
                           .replace("${QMAKE_PCH_TEMP_OBJECT}", objectFile);
            } else {
                // gcc style (including clang_pch_style)
                ProString header_prefix = project->first("QMAKE_PRECOMP_PREFIX");
                ProString header_suffix = project->isActiveConfig("clang_pch_style")
                                  ? project->first("QMAKE_PCH_OUTPUT_EXT") : "";
                pchOutput += Option::dir_sep;
                QString pchOutputDir = pchOutput.toQString(), pchOutputFile;

                if(comps[i] == "C") {
                    pchOutputFile = "c";
                } else if(comps[i] == "CXX") {
                    pchOutputFile = "c++";
                } else if(project->isActiveConfig("objective_c")) {
                    if(comps[i] == "OBJC")
                        pchOutputFile = "objective-c";
                    else if(comps[i] == "OBJCXX")
                        pchOutputFile = "objective-c++";
                }
                if(pchOutputFile.isEmpty())
                    continue;
                pchOutput += header_prefix + pchOutputFile + header_suffix;

                t << pchOutput << ": " << pchInput << " " << findDependencies(pchInput).join(" \\\n\t\t")
                  << "\n\t" << mkdir_p_asstring(pchOutputDir);
            }
            pchFlags = pchFlags.replace("${QMAKE_PCH_INPUT}", pchInput)
                       .replace("${QMAKE_PCH_OUTPUT_BASE}", pchBaseName.toQString())
                       .replace("${QMAKE_PCH_OUTPUT}", pchOutput.toQString());

            QString compiler;
            if(comps[i] == "C" || comps[i] == "OBJC" || comps[i] == "OBJCXX")
                compiler = "$(CC)";
            else
                compiler = "$(CXX)";

            // compile command
            t << "\n\t" << compiler << cflags << " $(INCPATH) " << pchFlags << endl << endl;
        }
    }

    writeExtraTargets(t);
    writeExtraCompilerTargets(t);
}

void UnixMakefileGenerator::init2()
{
    if(project->isEmpty("QMAKE_FRAMEWORK_VERSION"))
        project->values("QMAKE_FRAMEWORK_VERSION").append(project->values("VER_MAJ").first());

    if (project->values("TEMPLATE").first() == "aux")
        return;

    if (!project->values("QMAKE_APP_FLAG").isEmpty()) {
        if(!project->isEmpty("QMAKE_BUNDLE")) {
            ProString bundle_loc = project->first("QMAKE_BUNDLE_LOCATION");
            if(!bundle_loc.isEmpty() && !bundle_loc.startsWith("/"))
                bundle_loc.prepend("/");
            if(!bundle_loc.endsWith("/"))
                bundle_loc += "/";
            project->values("TARGET").first().prepend(project->first("QMAKE_BUNDLE") + bundle_loc);
        }
        if(!project->isEmpty("TARGET"))
            project->values("TARGET").first().prepend(project->first("DESTDIR"));
       if (!project->values("QMAKE_CYGWIN_EXE").isEmpty())
            project->values("TARGET_EXT").append(".exe");
    } else if (project->isActiveConfig("staticlib")) {
        project->values("TARGET").first().prepend(project->first("QMAKE_PREFIX_STATICLIB"));
        project->values("TARGET").first() += "." + project->first("QMAKE_EXTENSION_STATICLIB");
        if(project->values("QMAKE_AR_CMD").isEmpty())
            project->values("QMAKE_AR_CMD").append("$(AR) $(TARGET) $(OBJECTS)");
    } else {
        project->values("TARGETA").append(project->first("DESTDIR") + project->first("QMAKE_PREFIX_STATICLIB")
                + project->first("TARGET") + "." + project->first("QMAKE_EXTENSION_STATICLIB"));
        if(project->isActiveConfig("compile_libtool"))
            project->values("TARGET_la") = ProStringList(project->first("DESTDIR") + "lib" + project->first("TARGET") + Option::libtool_ext);

        ProStringList &ar_cmd = project->values("QMAKE_AR_CMD");
        if (!ar_cmd.isEmpty())
            ar_cmd[0] = ar_cmd.at(0).toQString().replace("(TARGET)","(TARGETA)");
        else
            ar_cmd.append("$(AR) $(TARGETA) $(OBJECTS)");
        if(project->isActiveConfig("compile_libtool")) {
            project->values("TARGET") = project->values("TARGET_la");
        } else if(!project->isEmpty("QMAKE_BUNDLE")) {
            ProString bundle_loc = project->first("QMAKE_BUNDLE_LOCATION");
            if(!bundle_loc.isEmpty() && !bundle_loc.startsWith("/"))
                bundle_loc.prepend("/");
            if(!bundle_loc.endsWith("/"))
                bundle_loc += "/";
            project->values("TARGET_").append(project->first("QMAKE_BUNDLE") +
                                                   bundle_loc + unescapeFilePath(project->first("TARGET")));
            project->values("TARGET_x.y").append(project->first("QMAKE_BUNDLE") +
                                                      "/Versions/" +
                                                      project->first("QMAKE_FRAMEWORK_VERSION") +
                                                      bundle_loc + unescapeFilePath(project->first("TARGET")));
        } else if(project->isActiveConfig("plugin")) {
            QString prefix;
            if(!project->isActiveConfig("no_plugin_name_prefix"))
                prefix = "lib";
            project->values("TARGET_x.y.z").append(prefix +
                                                        project->first("TARGET") + "." +
                                                        project->first("QMAKE_EXTENSION_PLUGIN"));
            if(project->isActiveConfig("lib_version_first"))
                project->values("TARGET_x").append(prefix + project->first("TARGET") + "." +
                                                        project->first("VER_MAJ") + "." +
                                                        project->first("QMAKE_EXTENSION_PLUGIN"));
            else
                project->values("TARGET_x").append(prefix + project->first("TARGET") + "." +
                                                        project->first("QMAKE_EXTENSION_PLUGIN") +
                                                        "." + project->first("VER_MAJ"));
            project->values("TARGET") = project->values("TARGET_x.y.z");
        } else if (!project->isEmpty("QMAKE_HPUX_SHLIB")) {
            project->values("TARGET_").append("lib" + project->first("TARGET") + ".sl");
            if(project->isActiveConfig("lib_version_first"))
                project->values("TARGET_x").append("lib" + project->first("VER_MAJ") + "." +
                                                        project->first("TARGET"));
            else
                project->values("TARGET_x").append("lib" + project->first("TARGET") + "." +
                                                        project->first("VER_MAJ"));
            project->values("TARGET") = project->values("TARGET_x");
        } else if (!project->isEmpty("QMAKE_AIX_SHLIB")) {
            project->values("TARGET_").append(project->first("QMAKE_PREFIX_STATICLIB") + project->first("TARGET")
                    + "." + project->first("QMAKE_EXTENSION_STATICLIB"));
            if(project->isActiveConfig("lib_version_first")) {
                project->values("TARGET_x").append("lib" + project->first("TARGET") + "." +
                                                        project->first("VER_MAJ") + "." +
                                                        project->first("QMAKE_EXTENSION_SHLIB"));
                project->values("TARGET_x.y").append("lib" + project->first("TARGET") + "." +
                                                          project->first("VER_MAJ") +
                                                          "." + project->first("VER_MIN") + "." +
                                                          project->first("QMAKE_EXTENSION_SHLIB"));
                project->values("TARGET_x.y.z").append("lib" + project->first("TARGET") + "." +
                                                            project->first("VER_MAJ") + "." +
                                                            project->first("VER_MIN") + "." +
                                                            project->first("VER_PAT") + "." +
                                                            project->first("QMAKE_EXTENSION_SHLIB"));
            } else {
                project->values("TARGET_x").append("lib" + project->first("TARGET") + "." +
                                                        project->first("QMAKE_EXTENSION_SHLIB") +
                                                        "." + project->first("VER_MAJ"));
                project->values("TARGET_x.y").append("lib" + project->first("TARGET") + "." +
                                                          project->first("QMAKE_EXTENSION_SHLIB") +
                                                          "." + project->first("VER_MAJ") +
                                                          "." + project->first("VER_MIN"));
                project->values("TARGET_x.y.z").append("lib" + project->first("TARGET") + "." +
                                                            project->first("QMAKE_EXTENSION_SHLIB") + "." +
                                                            project->first("VER_MAJ") + "." +
                                                            project->first("VER_MIN") + "." +
                                                            project->first("VER_PAT"));
            }
            project->values("TARGET") = project->values("TARGET_x.y.z");
        } else {
            project->values("TARGET_").append("lib" + project->first("TARGET") + "." +
                                                   project->first("QMAKE_EXTENSION_SHLIB"));
            if(project->isActiveConfig("lib_version_first")) {
                project->values("TARGET_x").append("lib" + project->first("TARGET") + "." +
                                                        project->first("VER_MAJ") + "." +
                                                        project->first("QMAKE_EXTENSION_SHLIB"));
                project->values("TARGET_x.y").append("lib" + project->first("TARGET") + "." +
                                                          project->first("VER_MAJ") +
                                                          "." + project->first("VER_MIN") + "." +
                                                          project->first("QMAKE_EXTENSION_SHLIB"));
                project->values("TARGET_x.y.z").append("lib" + project->first("TARGET") + "." +
                                                            project->first("VER_MAJ") + "." +
                                                            project->first("VER_MIN") +  "." +
                                                            project->first("VER_PAT") + "." +
                                                            project->values("QMAKE_EXTENSION_SHLIB").first());
            } else {
                project->values("TARGET_x").append("lib" + project->first("TARGET") + "." +
                                                        project->first("QMAKE_EXTENSION_SHLIB") +
                                                        "." + project->first("VER_MAJ"));
                project->values("TARGET_x.y").append("lib" + project->first("TARGET") + "." +
                                                      project->first("QMAKE_EXTENSION_SHLIB")
                                                      + "." + project->first("VER_MAJ") +
                                                      "." + project->first("VER_MIN"));
                project->values("TARGET_x.y.z").append("lib" + project->first("TARGET") +
                                                            "." +
                                                            project->values(
                                                                "QMAKE_EXTENSION_SHLIB").first() + "." +
                                                            project->first("VER_MAJ") + "." +
                                                            project->first("VER_MIN") +  "." +
                                                            project->first("VER_PAT"));
            }
            if (project->isActiveConfig("unversioned_libname"))
                project->values("TARGET") = project->values("TARGET_");
            else
                project->values("TARGET") = project->values("TARGET_x.y.z");
        }
        if(project->isEmpty("QMAKE_LN_SHLIB"))
            project->values("QMAKE_LN_SHLIB").append("ln -s");
        if (!project->values("QMAKE_LFLAGS_SONAME").isEmpty()) {
            ProString soname;
            if(project->isActiveConfig("plugin")) {
                if(!project->values("TARGET").isEmpty())
                    soname += project->first("TARGET");
            } else if(!project->isEmpty("QMAKE_BUNDLE")) {
                soname += project->first("TARGET_x.y");
            } else if(project->isActiveConfig("unversioned_soname")) {
                soname = "lib" + project->first("QMAKE_ORIG_TARGET")
                    + "." + project->first("QMAKE_EXTENSION_SHLIB");
            } else if(!project->values("TARGET_x").isEmpty()) {
                soname += project->first("TARGET_x");
            }
            if(!soname.isEmpty()) {
                if(project->isActiveConfig("absolute_library_soname") &&
                   project->values("INSTALLS").indexOf("target") != -1 &&
                   !project->isEmpty("target.path")) {
                    QString instpath = Option::fixPathToTargetOS(project->first("target.path").toQString());
                    if(!instpath.endsWith(Option::dir_sep))
                        instpath += Option::dir_sep;
                    soname.prepend(instpath);
                } else if (!project->isEmpty("QMAKE_SONAME_PREFIX")) {
                    QString sonameprefix = project->first("QMAKE_SONAME_PREFIX").toQString();
                    if (!sonameprefix.startsWith('@') && !sonameprefix.startsWith('$'))
                        sonameprefix = Option::fixPathToTargetOS(sonameprefix, false);
                    if (!sonameprefix.endsWith(Option::dir_sep))
                        sonameprefix += Option::dir_sep;
                    soname.prepend(sonameprefix);
                }
                project->values("QMAKE_LFLAGS_SONAME").first() += escapeFilePath(soname);
            }
        }
        if (project->values("QMAKE_LINK_SHLIB_CMD").isEmpty())
            project->values("QMAKE_LINK_SHLIB_CMD").append(
                "$(LINK) $(LFLAGS) " + project->first("QMAKE_LINK_O_FLAG") + "$(TARGET) $(OBJECTS) $(LIBS) $(OBJCOMP)");
    }
    if (!project->values("QMAKE_APP_FLAG").isEmpty()) {
        project->values("QMAKE_CFLAGS") += project->values("QMAKE_CFLAGS_APP");
        project->values("QMAKE_CXXFLAGS") += project->values("QMAKE_CXXFLAGS_APP");
        project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_APP");
    } else if (project->isActiveConfig("dll")) {
        if(!project->isActiveConfig("plugin") || !project->isActiveConfig("plugin_no_share_shlib_cflags")) {
            project->values("QMAKE_CFLAGS") += project->values("QMAKE_CFLAGS_SHLIB");
            project->values("QMAKE_CXXFLAGS") += project->values("QMAKE_CXXFLAGS_SHLIB");
        }
        if (project->isActiveConfig("plugin")) {
            project->values("QMAKE_CFLAGS") += project->values("QMAKE_CFLAGS_PLUGIN");
            project->values("QMAKE_CXXFLAGS") += project->values("QMAKE_CXXFLAGS_PLUGIN");
            project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_PLUGIN");
            if(project->isActiveConfig("plugin_with_soname") && !project->isActiveConfig("compile_libtool"))
                project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_SONAME");
        } else {
            project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_SHLIB");
            if(!project->isEmpty("QMAKE_LFLAGS_COMPAT_VERSION")) {
                if(project->isEmpty("COMPAT_VERSION"))
                    project->values("QMAKE_LFLAGS") += QString(project->first("QMAKE_LFLAGS_COMPAT_VERSION") +
                                                                    project->first("VER_MAJ") + "." +
                                                                    project->first("VER_MIN"));
                else
                    project->values("QMAKE_LFLAGS") += QString(project->first("QMAKE_LFLAGS_COMPAT_VERSION") +
                                                                    project->first("COMPATIBILITY_VERSION"));
            }
            if(!project->isEmpty("QMAKE_LFLAGS_VERSION")) {
                project->values("QMAKE_LFLAGS") += QString(project->first("QMAKE_LFLAGS_VERSION") +
                                                                project->first("VER_MAJ") + "." +
                                                                project->first("VER_MIN") + "." +
                                                                project->first("VER_PAT"));
            }
            if(!project->isActiveConfig("compile_libtool"))
                project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_SONAME");
        }
    }

    if (include_deps && project->isActiveConfig("gcc_MD_depends")) {
        // use -MMD if we know about -isystem too
        ProString MD_flag(project->values("QMAKE_CFLAGS_ISYSTEM").isEmpty() ? "-MD" : "-MMD");
        project->values("QMAKE_CFLAGS") += MD_flag;
        project->values("QMAKE_CXXFLAGS") += MD_flag;
    }
}

QString
UnixMakefileGenerator::libtoolFileName(bool fixify)
{
    QString ret = var("TARGET");
    int slsh = ret.lastIndexOf(Option::dir_sep);
    if(slsh != -1)
        ret = ret.right(ret.length() - slsh - 1);
    int dot = ret.indexOf('.');
    if(dot != -1)
        ret = ret.left(dot);
    ret += Option::libtool_ext;
    if(!project->isEmpty("QMAKE_LIBTOOL_DESTDIR"))
        ret.prepend(project->first("QMAKE_LIBTOOL_DESTDIR") + Option::dir_sep);
    if(fixify) {
        if(QDir::isRelativePath(ret) && !project->isEmpty("DESTDIR"))
            ret.prepend(project->first("DESTDIR").toQString());
        ret = Option::fixPathToLocalOS(fileFixify(ret, qmake_getpwd(), Option::output_dir));
    }
    return ret;
}

void
UnixMakefileGenerator::writeLibtoolFile()
{
    QString fname = libtoolFileName(), lname = fname;
    mkdir(fileInfo(fname).path());
    int slsh = lname.lastIndexOf(Option::dir_sep);
    if(slsh != -1)
        lname = lname.right(lname.length() - slsh - 1);
    QFile ft(fname);
    if(!ft.open(QIODevice::WriteOnly))
        return;
    project->values("ALL_DEPS").append(fileFixify(fname));

    QTextStream t(&ft);
    t << "# " << lname << " - a libtool library file\n";
    t << "# Generated by qmake/libtool (" QMAKE_VERSION_STR ") (Qt "
      << QT_VERSION_STR << ") on: " << QDateTime::currentDateTime().toString();
    t << "\n";

    t << "# The name that we can dlopen(3).\n"
      << "dlname='" << var(project->isActiveConfig("plugin") ? "TARGET" : "TARGET_x")
      << "'\n\n";

    t << "# Names of this library.\n";
    t << "library_names='";
    if(project->isActiveConfig("plugin")) {
        t << var("TARGET");
    } else {
        if (project->isEmpty("QMAKE_HPUX_SHLIB"))
            t << var("TARGET_x.y.z") << " ";
        t << var("TARGET_x") << " " << var("TARGET_");
    }
    t << "'\n\n";

    t << "# The name of the static archive.\n"
      << "old_library='" << lname.left(lname.length()-Option::libtool_ext.length()) << ".a'\n\n";

    t << "# Libraries that this one depends upon.\n";
    ProStringList libs;
    if(!project->isEmpty("QMAKE_INTERNAL_PRL_LIBS"))
        libs = project->values("QMAKE_INTERNAL_PRL_LIBS");
    else
        libs << "QMAKE_LIBS"; //obvious one
    t << "dependency_libs='";
    for (ProStringList::ConstIterator it = libs.begin(); it != libs.end(); ++it)
        t << project->values((*it).toKey()).join(' ') << " ";
    t << "'\n\n";

    t << "# Version information for " << lname << "\n";
    int maj = project->first("VER_MAJ").toInt();
    int min = project->first("VER_MIN").toInt();
    int pat = project->first("VER_PAT").toInt();
    t << "current=" << (10*maj + min) << "\n" // best I can think of
      << "age=0\n"
      << "revision=" << pat << "\n\n";

    t << "# Is this an already installed library.\n"
        "installed=yes\n\n"; // ###

    t << "# Files to dlopen/dlpreopen.\n"
        "dlopen=''\n"
        "dlpreopen=''\n\n";

    ProString install_dir = project->first("QMAKE_LIBTOOL_LIBDIR");
    if(install_dir.isEmpty())
        install_dir = project->first("target.path");
    if(install_dir.isEmpty())
        install_dir = project->first("DESTDIR");
    t << "# Directory that this library needs to be installed in:\n"
        "libdir='" << Option::fixPathToTargetOS(install_dir.toQString(), false) << "'\n";
}

QT_END_NAMESPACE
