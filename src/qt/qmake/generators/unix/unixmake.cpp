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

#include "unixmake.h"
#include "option.h"
#include <qregexp.h>
#include <qfile.h>
#include <qhash.h>
#include <qdir.h>
#include <time.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

void
UnixMakefileGenerator::init()
{
    if(init_flag)
        return;
    init_flag = true;

    if(project->isEmpty("QMAKE_EXTENSION_SHLIB")) {
        if(project->isEmpty("QMAKE_CYGWIN_SHLIB")) {
            project->values("QMAKE_EXTENSION_SHLIB").append("so");
        } else {
            project->values("QMAKE_EXTENSION_SHLIB").append("dll");
        }
    }

    if (project->isEmpty("QMAKE_PREFIX_SHLIB"))
        // Prevent crash when using the empty variable.
        project->values("QMAKE_PREFIX_SHLIB").append("");

    if(!project->isEmpty("QMAKE_FAILED_REQUIREMENTS")) /* no point */
        return;

    QStringList &configs = project->values("CONFIG");
    if(project->isEmpty("ICON") && !project->isEmpty("RC_FILE"))
        project->values("ICON") = project->values("RC_FILE");
    if(project->isEmpty("QMAKE_EXTENSION_PLUGIN"))
        project->values("QMAKE_EXTENSION_PLUGIN").append(project->first("QMAKE_EXTENSION_SHLIB"));
    if(project->isEmpty("QMAKE_COPY_FILE"))
        project->values("QMAKE_COPY_FILE").append("$(COPY)");
    if(project->isEmpty("QMAKE_STREAM_EDITOR"))
        project->values("QMAKE_STREAM_EDITOR").append("sed");
    if(project->isEmpty("QMAKE_COPY_DIR"))
        project->values("QMAKE_COPY_DIR").append("$(COPY) -R");
    if(project->isEmpty("QMAKE_INSTALL_FILE"))
        project->values("QMAKE_INSTALL_FILE").append("$(COPY_FILE)");
    if(project->isEmpty("QMAKE_INSTALL_DIR"))
        project->values("QMAKE_INSTALL_DIR").append("$(COPY_DIR)");
    if(project->isEmpty("QMAKE_INSTALL_PROGRAM"))
        project->values("QMAKE_INSTALL_PROGRAM").append("$(COPY_FILE)");
    if(project->isEmpty("QMAKE_LIBTOOL"))
        project->values("QMAKE_LIBTOOL").append("libtool --silent");
    if(project->isEmpty("QMAKE_SYMBOLIC_LINK"))
        project->values("QMAKE_SYMBOLIC_LINK").append("ln -f -s");

    /* this should probably not be here, but I'm using it to wrap the .t files */
    if(project->first("TEMPLATE") == "app")
        project->values("QMAKE_APP_FLAG").append("1");
    else if(project->first("TEMPLATE") == "lib")
        project->values("QMAKE_LIB_FLAG").append("1");
    else if(project->first("TEMPLATE") == "subdirs") {
        MakefileGenerator::init();
        if(project->isEmpty("MAKEFILE"))
            project->values("MAKEFILE").append("Makefile");
        if(project->values("QMAKE_INTERNAL_QMAKE_DEPS").indexOf("qmake_all") == -1)
            project->values("QMAKE_INTERNAL_QMAKE_DEPS").append("qmake_all");
        return; /* subdirs is done */
    }

    //If the TARGET looks like a path split it into DESTDIR and the resulting TARGET
    if(!project->isEmpty("TARGET")) {
        project->values("TARGET") = escapeFilePaths(project->values("TARGET"));
        QString targ = unescapeFilePath(project->first("TARGET"));
        int slsh = qMax(targ.lastIndexOf('/'), targ.lastIndexOf(Option::dir_sep));
        if(slsh != -1) {
            if(project->isEmpty("DESTDIR"))
                project->values("DESTDIR").append("");
            else if(project->first("DESTDIR").right(1) != Option::dir_sep)
                project->values("DESTDIR") = QStringList(project->first("DESTDIR") + Option::dir_sep);
            project->values("DESTDIR") = QStringList(project->first("DESTDIR") + targ.left(slsh+1));
            project->values("TARGET") = QStringList(targ.mid(slsh+1));
        }
    }

    project->values("QMAKE_ORIG_TARGET") = project->values("TARGET");
    project->values("QMAKE_ORIG_DESTDIR") = project->values("DESTDIR");
    project->values("QMAKE_LIBS") += escapeFilePaths(project->values("LIBS"));
    project->values("QMAKE_LIBS_PRIVATE") += escapeFilePaths(project->values("LIBS_PRIVATE"));
    if((!project->isEmpty("QMAKE_LIB_FLAG") && !project->isActiveConfig("staticlib")) ||
       (project->isActiveConfig("qt") &&  project->isActiveConfig("plugin"))) {
        if(configs.indexOf("dll") == -1) configs.append("dll");
    } else if(!project->isEmpty("QMAKE_APP_FLAG") || project->isActiveConfig("dll")) {
        configs.removeAll("staticlib");
    }
    if(!project->isEmpty("QMAKE_INCREMENTAL"))
        project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_INCREMENTAL");
    else if(!project->isEmpty("QMAKE_LFLAGS_PREBIND") &&
            !project->values("QMAKE_LIB_FLAG").isEmpty() &&
            project->isActiveConfig("dll"))
        project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_PREBIND");
    if(!project->isEmpty("QMAKE_INCDIR"))
        project->values("INCLUDEPATH") += project->values("QMAKE_INCDIR");
    if(!project->isEmpty("QMAKE_LIBDIR")) {
        const QStringList &libdirs = project->values("QMAKE_LIBDIR");
        for(int i = 0; i < libdirs.size(); ++i) {
            if(!project->isEmpty("QMAKE_LFLAGS_RPATH") && project->isActiveConfig("rpath_libdirs"))
                project->values("QMAKE_LFLAGS") += var("QMAKE_LFLAGS_RPATH") + libdirs[i];
            if (project->isActiveConfig("rvct_linker")) {
                project->values("QMAKE_LIBDIR_FLAGS") += "--userlibpath " + escapeFilePath(libdirs[i]);
            } else if (project->isActiveConfig("armcc_linker")) {
                project->values("QMAKE_LIBDIR_FLAGS") += "-L--userlibpath=" + escapeFilePath(libdirs[i]);
            } else {
                project->values("QMAKE_LIBDIR_FLAGS") += "-L" + escapeFilePath(libdirs[i]);
            }
        }
    }
    if(project->isActiveConfig("macx") && !project->isEmpty("QMAKE_FRAMEWORKPATH")) {
        const QStringList &fwdirs = project->values("QMAKE_FRAMEWORKPATH");
        for(int i = 0; i < fwdirs.size(); ++i) {
            project->values("QMAKE_FRAMEWORKPATH_FLAGS") += "-F" + escapeFilePath(fwdirs[i]);
        }
    }
    if(!project->isEmpty("QMAKE_RPATHDIR")) {
        const QStringList &rpathdirs = project->values("QMAKE_RPATHDIR");
        for(int i = 0; i < rpathdirs.size(); ++i) {
            if(!project->isEmpty("QMAKE_LFLAGS_RPATH"))
                project->values("QMAKE_LFLAGS") += var("QMAKE_LFLAGS_RPATH") + escapeFilePath(QFileInfo(rpathdirs[i]).absoluteFilePath());
        }
    }

    project->values("QMAKE_FILETAGS") << "SOURCES" << "GENERATED_SOURCES" << "TARGET" << "DESTDIR";
    if(!project->isEmpty("QMAKE_EXTRA_COMPILERS")) {
        const QStringList &quc = project->values("QMAKE_EXTRA_COMPILERS");
        for(int i = 0; i < quc.size(); ++i)
            project->values("QMAKE_FILETAGS") += project->values(quc[i]+".input");
    }

    if(project->isActiveConfig("GNUmake") && !project->isEmpty("QMAKE_CFLAGS_DEPS"))
        include_deps = true; //do not generate deps
    if(project->isActiveConfig("compile_libtool"))
        Option::obj_ext = ".lo"; //override the .o

    MakefileGenerator::init();

    QString comps[] = { "C", "CXX", "OBJC", "OBJCXX", QString() };
    for(int i = 0; !comps[i].isNull(); i++) {
        QString compile_flag = var("QMAKE_COMPILE_FLAG");
        if(compile_flag.isEmpty())
            compile_flag = "-c";

        if(doPrecompiledHeaders() && !project->isEmpty("PRECOMPILED_HEADER")) {
            QString pchFlags = var("QMAKE_" + comps[i] + "FLAGS_USE_PRECOMPILE");

            QString pchBaseName;
            if(!project->isEmpty("PRECOMPILED_DIR")) {
                pchBaseName = Option::fixPathToTargetOS(project->first("PRECOMPILED_DIR"));
                if(!pchBaseName.endsWith(Option::dir_sep))
                    pchBaseName += Option::dir_sep;
            }
            pchBaseName += project->first("QMAKE_ORIG_TARGET");

            // replace place holders
            pchFlags = pchFlags.replace("${QMAKE_PCH_INPUT}",
                                        fileFixify(project->first("PRECOMPILED_HEADER")));
            pchFlags = pchFlags.replace("${QMAKE_PCH_OUTPUT_BASE}", pchBaseName);
            if (project->isActiveConfig("icc_pch_style")) {
                // icc style
                pchFlags = pchFlags.replace("${QMAKE_PCH_OUTPUT}",
                                            pchBaseName + project->first("QMAKE_PCH_OUTPUT_EXT"));
            } else {
                // gcc style (including clang_pch_style)
                QString headerPrefix = project->first("QMAKE_PRECOMP_PREFIX");
                QString headerSuffix;
                if (project->isActiveConfig("clang_pch_style"))
                    headerSuffix = project->first("QMAKE_PCH_OUTPUT_EXT");
                else
                    pchBaseName += project->first("QMAKE_PCH_OUTPUT_EXT");

                pchBaseName += Option::dir_sep;
                QString pchOutputFile;

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

                if(!pchOutputFile.isEmpty()) {
                    pchFlags = pchFlags.replace("${QMAKE_PCH_OUTPUT}",
                            pchBaseName + pchOutputFile + headerSuffix);
                }
            }

            if (!pchFlags.isEmpty())
                compile_flag += " " + pchFlags;
        }

        QString cflags;
        if(comps[i] == "OBJC" || comps[i] == "OBJCXX")
            cflags += " $(CFLAGS)";
        else
            cflags += " $(" + comps[i] + "FLAGS)";
        compile_flag += cflags + " $(INCPATH)";

        QString compiler = comps[i];
        if (compiler == "C")
            compiler = "CC";

        QString runComp = "QMAKE_RUN_" + compiler;
        if(project->isEmpty(runComp))
            project->values(runComp).append("$(" + compiler + ") " + compile_flag + " -o $obj $src");
        QString runCompImp = "QMAKE_RUN_" + compiler + "_IMP";
        if(project->isEmpty(runCompImp))
            project->values(runCompImp).append("$(" + compiler + ") " + compile_flag + " -o \"$@\" \"$<\"");
    }

    if(project->isActiveConfig("macx") && !project->isEmpty("TARGET") && !project->isActiveConfig("compile_libtool") &&
       ((project->isActiveConfig("build_pass") || project->isEmpty("BUILDS")))) {
        QString bundle;
        if(project->isActiveConfig("bundle") && !project->isEmpty("QMAKE_BUNDLE_EXTENSION")) {
            bundle = unescapeFilePath(project->first("TARGET"));
            if(!project->isEmpty("QMAKE_BUNDLE_NAME"))
                bundle = unescapeFilePath(project->first("QMAKE_BUNDLE_NAME"));
            if(!bundle.endsWith(project->first("QMAKE_BUNDLE_EXTENSION")))
                bundle += project->first("QMAKE_BUNDLE_EXTENSION");
        } else if(project->first("TEMPLATE") == "app" && project->isActiveConfig("app_bundle")) {
            bundle = unescapeFilePath(project->first("TARGET"));
            if(!project->isEmpty("QMAKE_APPLICATION_BUNDLE_NAME"))
                bundle = unescapeFilePath(project->first("QMAKE_APPLICATION_BUNDLE_NAME"));
            if(!bundle.endsWith(".app"))
                bundle += ".app";
            if(project->isEmpty("QMAKE_BUNDLE_LOCATION"))
                project->values("QMAKE_BUNDLE_LOCATION").append("Contents/MacOS");
            project->values("QMAKE_PKGINFO").append(project->first("DESTDIR") + bundle + "/Contents/PkgInfo");
            project->values("QMAKE_BUNDLE_RESOURCE_FILE").append(project->first("DESTDIR") + bundle + "/Contents/Resources/empty.lproj");
        } else if(project->first("TEMPLATE") == "lib" && !project->isActiveConfig("staticlib") &&
                  ((!project->isActiveConfig("plugin") && project->isActiveConfig("lib_bundle")) ||
                   (project->isActiveConfig("plugin") && project->isActiveConfig("plugin_bundle")))) {
            bundle = unescapeFilePath(project->first("TARGET"));
            if(project->isActiveConfig("plugin")) {
                if(!project->isEmpty("QMAKE_PLUGIN_BUNDLE_NAME"))
                    bundle = unescapeFilePath(project->first("QMAKE_PLUGIN_BUNDLE_NAME"));
                if(!project->isEmpty("QMAKE_BUNDLE_EXTENSION") && !bundle.endsWith(project->first("QMAKE_BUNDLE_EXTENSION")))
                    bundle += project->first("QMAKE_BUNDLE_EXTENSION");
                else if(!bundle.endsWith(".plugin"))
                    bundle += ".plugin";
                if(project->isEmpty("QMAKE_BUNDLE_LOCATION"))
                    project->values("QMAKE_BUNDLE_LOCATION").append("Contents/MacOS");
            } else {
                if(!project->isEmpty("QMAKE_FRAMEWORK_BUNDLE_NAME"))
                    bundle = unescapeFilePath(project->first("QMAKE_FRAMEWORK_BUNDLE_NAME"));
                if(!project->isEmpty("QMAKE_BUNDLE_EXTENSION") && !bundle.endsWith(project->first("QMAKE_BUNDLE_EXTENSION")))
                    bundle += project->first("QMAKE_BUNDLE_EXTENSION");
                else if(!bundle.endsWith(".framework"))
                    bundle += ".framework";
            }
        }
        if(!bundle.isEmpty()) {
            project->values("QMAKE_BUNDLE") = QStringList(bundle);
            project->values("ALL_DEPS") += project->first("QMAKE_PKGINFO");
            project->values("ALL_DEPS") += project->first("QMAKE_BUNDLE_RESOURCE_FILE");
        } else {
            project->values("QMAKE_BUNDLE").clear();
            project->values("QMAKE_BUNDLE_LOCATION").clear();
        }
    } else { //no bundling here
        project->values("QMAKE_BUNDLE").clear();
        project->values("QMAKE_BUNDLE_LOCATION").clear();
    }

    if(!project->isEmpty("QMAKE_INTERNAL_INCLUDED_FILES"))
        project->values("DISTFILES") += project->values("QMAKE_INTERNAL_INCLUDED_FILES");
    project->values("DISTFILES") += project->projectFile();

    init2();
    project->values("QMAKE_INTERNAL_PRL_LIBS") << "QMAKE_LIBDIR_FLAGS" << "QMAKE_FRAMEWORKPATH_FLAGS" << "QMAKE_LIBS";
    if(!project->isEmpty("QMAKE_MAX_FILES_PER_AR")) {
        bool ok;
        int max_files = project->first("QMAKE_MAX_FILES_PER_AR").toInt(&ok);
        QStringList ar_sublibs, objs = project->values("OBJECTS");
        if(ok && max_files > 5 && max_files < (int)objs.count()) {
            QString lib;
            for(int i = 0, obj_cnt = 0, lib_cnt = 0; i != objs.size(); ++i) {
                if((++obj_cnt) >= max_files) {
                    if(lib_cnt) {
                        lib.sprintf("lib%s-tmp%d.a",
                                    project->first("QMAKE_ORIG_TARGET").toLatin1().constData(), lib_cnt);
                        ar_sublibs << lib;
                        obj_cnt = 0;
                    }
                    lib_cnt++;
                }
            }
        }
        if(!ar_sublibs.isEmpty()) {
            project->values("QMAKE_AR_SUBLIBS") = ar_sublibs;
            project->values("QMAKE_INTERNAL_PRL_LIBS") << "QMAKE_AR_SUBLIBS";
        }
    }

    if(project->isActiveConfig("compile_libtool")) {
        const QString libtoolify[] = { "QMAKE_RUN_CC", "QMAKE_RUN_CC_IMP",
                                       "QMAKE_RUN_CXX", "QMAKE_RUN_CXX_IMP",
                                       "QMAKE_LINK_THREAD", "QMAKE_LINK", "QMAKE_AR_CMD", "QMAKE_LINK_SHLIB_CMD",
                                       QString() };
        for(int i = 0; !libtoolify[i].isNull(); i++) {
            QStringList &l = project->values(libtoolify[i]);
            if(!l.isEmpty()) {
                QString libtool_flags, comp_flags;
                if(libtoolify[i].startsWith("QMAKE_LINK") || libtoolify[i] == "QMAKE_AR_CMD") {
                    libtool_flags += " --mode=link";
                    if(project->isActiveConfig("staticlib")) {
                        libtool_flags += " -static";
                    } else {
                        if(!project->isEmpty("QMAKE_LIB_FLAG")) {
                            int maj = project->first("VER_MAJ").toInt();
                            int min = project->first("VER_MIN").toInt();
                            int pat = project->first("VER_PAT").toInt();
                            comp_flags += " -version-info " + QString::number(10*maj + min) +
                                          ":" + QString::number(pat) + ":0";
                            if(libtoolify[i] != "QMAKE_AR_CMD") {
                                QString rpath = Option::output_dir;
                                if(!project->isEmpty("DESTDIR")) {
                                    rpath = project->first("DESTDIR");
                                    if(QDir::isRelativePath(rpath))
                                        rpath.prepend(Option::output_dir + Option::dir_sep);
                                }
                                comp_flags += " -rpath " + Option::fixPathToTargetOS(rpath, false);
                            }
                        }
                    }
                    if(project->isActiveConfig("plugin"))
                        libtool_flags += " -module";
                } else {
                    libtool_flags += " --mode=compile";
                }
                l.first().prepend("$(LIBTOOL)" + libtool_flags + " ");
                if(!comp_flags.isEmpty())
                    l.first() += comp_flags;
            }
        }
    }
}

void
UnixMakefileGenerator::processPrlVariable(const QString &var, const QStringList &l)
{
    if(var == "QMAKE_PRL_LIBS") {
        project->values("QMAKE_CURRENT_PRL_LIBS") += l;
    } else
        MakefileGenerator::processPrlVariable(var, l);
}

QStringList
&UnixMakefileGenerator::findDependencies(const QString &file)
{
    QStringList &ret = MakefileGenerator::findDependencies(file);
    // Note: The QMAKE_IMAGE_COLLECTION file have all images
    // as dependency, so don't add precompiled header then
    if(doPrecompiledHeaders() && !project->isEmpty("PRECOMPILED_HEADER")
       && file != project->first("QMAKE_IMAGE_COLLECTION")) {
        QString header_prefix;
        if(!project->isEmpty("PRECOMPILED_DIR"))
            header_prefix = project->first("PRECOMPILED_DIR");
        header_prefix += project->first("QMAKE_ORIG_TARGET");
        if (!project->isActiveConfig("clang_pch_style"))
            header_prefix += project->first("QMAKE_PCH_OUTPUT_EXT");
        if (project->isActiveConfig("icc_pch_style")) {
            // icc style
            for(QStringList::Iterator it = Option::cpp_ext.begin(); it != Option::cpp_ext.end(); ++it) {
                if(file.endsWith(*it)) {
                    ret += header_prefix;
                    break;
                }
            }
        } else {
            // gcc style (including clang_pch_style)
            QString header_suffix = project->isActiveConfig("clang_pch_style")
                    ? project->first("QMAKE_PCH_OUTPUT_EXT") : "";
            header_prefix += Option::dir_sep + project->first("QMAKE_PRECOMP_PREFIX");
            for(QStringList::Iterator it = Option::c_ext.begin(); it != Option::c_ext.end(); ++it) {
                if(file.endsWith(*it)) {
                    if(!project->isEmpty("QMAKE_CFLAGS_PRECOMPILE")) {
                        QString precomp_c_h = header_prefix + "c" + header_suffix;
                        if(!ret.contains(precomp_c_h))
                            ret += precomp_c_h;
                    }
                    if(project->isActiveConfig("objective_c")) {
                        if(!project->isEmpty("QMAKE_OBJCFLAGS_PRECOMPILE")) {
                            QString precomp_objc_h = header_prefix + "objective-c" + header_suffix;
                            if(!ret.contains(precomp_objc_h))
                                ret += precomp_objc_h;
                        }
                        if(!project->isEmpty("QMAKE_OBJCXXFLAGS_PRECOMPILE")) {
                            QString precomp_objcpp_h = header_prefix + "objective-c++" + header_suffix;
                            if(!ret.contains(precomp_objcpp_h))
                                ret += precomp_objcpp_h;
                        }
                    }
                    break;
                }
            }
            for(QStringList::Iterator it = Option::cpp_ext.begin(); it != Option::cpp_ext.end(); ++it) {
                if(file.endsWith(*it)) {
                    if(!project->isEmpty("QMAKE_CXXFLAGS_PRECOMPILE")) {
                        QString precomp_cpp_h = header_prefix + "c++" + header_suffix;
                        if(!ret.contains(precomp_cpp_h))
                            ret += precomp_cpp_h;
                    }
                    if(project->isActiveConfig("objective_c")) {
                        if(!project->isEmpty("QMAKE_OBJCXXFLAGS_PRECOMPILE")) {
                            QString precomp_objcpp_h = header_prefix + "objective-c++" + header_suffix;
                            if(!ret.contains(precomp_objcpp_h))
                                ret += precomp_objcpp_h;
                        }
                    }
                    break;
                }
            }
        }
    }
    return ret;
}

bool
UnixMakefileGenerator::findLibraries()
{
    QList<QMakeLocalFileName> libdirs, frameworkdirs;
    frameworkdirs.append(QMakeLocalFileName("/System/Library/Frameworks"));
    frameworkdirs.append(QMakeLocalFileName("/Library/Frameworks"));
    const QString lflags[] = { "QMAKE_LIBDIR_FLAGS", "QMAKE_FRAMEWORKPATH_FLAGS", "QMAKE_LFLAGS", "QMAKE_LIBS", "QMAKE_LIBS_PRIVATE", QString() };
    for(int i = 0; !lflags[i].isNull(); i++) {
        QStringList &l = project->values(lflags[i]);
        for(QStringList::Iterator it = l.begin(); it != l.end(); ++it) {
            bool do_suffix = true;
            QString stub, dir, extn, opt = (*it).trimmed();
            if(opt.startsWith("-")) {
                if(opt.startsWith("-L")) {
                    QMakeLocalFileName f(opt.right(opt.length()-2));
                    if(!libdirs.contains(f))
                        libdirs.append(f);
                } else if(opt.startsWith("-l")) {
                    if (!project->isEmpty("QMAKE_RVCT_LINKSTYLE")) {
                        (*it) = opt.mid(2);
                    } else if (project->isActiveConfig("rvct_linker") || project->isActiveConfig("armcc_linker")) {
                        (*it) = "lib" + opt.mid(2) + ".so";
                    } else {
                        stub = opt.mid(2);
                    }
                } else if(Option::target_mode == Option::TARG_MACX_MODE && opt.startsWith("-F")) {
                    frameworkdirs.append(QMakeLocalFileName(opt.right(opt.length()-2)));
                } else if(Option::target_mode == Option::TARG_MACX_MODE && opt.startsWith("-framework")) {
                    if(opt.length() > 11) {
                        opt = opt.mid(11);
                    } else {
                        ++it;
                        opt = (*it);
                    }
                    do_suffix = false;
                    extn = "";
                    dir = "/System/Library/Frameworks/" + opt + ".framework/";
                    stub = opt;
                }
            } else {
                extn = dir = "";
                stub = opt;
                int slsh = opt.lastIndexOf(Option::dir_sep);
                if(slsh != -1) {
                    dir = opt.left(slsh);
                    stub = opt.mid(slsh+1);
                }
                QRegExp stub_reg("^.*lib(" + stub + "[^./=]*)\\.(.*)$");
                if(stub_reg.exactMatch(stub)) {
                    stub = stub_reg.cap(1);
                    extn = stub_reg.cap(2);
                }
            }
            if(!stub.isEmpty()) {
                if(do_suffix && !project->isEmpty("QMAKE_" + stub.toUpper() + "_SUFFIX"))
                    stub += project->first("QMAKE_" + stub.toUpper() + "_SUFFIX");
                bool found = false;
                QStringList extens;
                if(!extn.isNull())
                    extens << extn;
                else if (!project->isEmpty("QMAKE_SYMBIAN_SHLIB"))
                    // In Symbian you link to the stub .lib file, but run with the .dll file.
                    extens << "lib";
                else
                    extens << project->values("QMAKE_EXTENSION_SHLIB").first() << "a";
                for(QStringList::Iterator extit = extens.begin(); extit != extens.end(); ++extit) {
                    if(dir.isNull()) {
                        for(QList<QMakeLocalFileName>::Iterator dep_it = libdirs.begin(); dep_it != libdirs.end(); ++dep_it) {
                            QString pathToLib = ((*dep_it).local() + Option::dir_sep
                                    + project->values("QMAKE_PREFIX_SHLIB").first()
                                    + stub + "." + (*extit));
                            if(exists(pathToLib)) {
                                if (!project->isEmpty("QMAKE_RVCT_LINKSTYLE"))
                                    (*it) = pathToLib;
                                else
                                    (*it) = "-l" + stub;
                                found = true;
                                break;
                            }
                        }
                    } else {
                        if(exists(project->values("QMAKE_PREFIX_SHLIB").first() + stub + "." + (*extit))) {
                            (*it) = project->values("QMAKE_PREFIX_SHLIB").first() + stub + "." + (*extit);
                            found = true;
                            break;
                        }
                    }
                }
                if(!found && project->isActiveConfig("compile_libtool")) {
                    for(int dep_i = 0; dep_i < libdirs.size(); ++dep_i) {
                        if(exists(libdirs[dep_i].local() + Option::dir_sep + project->values("QMAKE_PREFIX_SHLIB").first() + stub + Option::libtool_ext)) {
                            (*it) = libdirs[dep_i].real() + Option::dir_sep + project->values("QMAKE_PREFIX_SHLIB").first() + stub + Option::libtool_ext;
                            found = true;
                            break;
                        }
                    }
                }
            }
        }
    }
    return false;
}

QString linkLib(const QString &file, const QString &libName) {
    QString ret;
    QRegExp reg("^.*lib(" + QRegExp::escape(libName) + "[^./=]*).*$");
    if(reg.exactMatch(file))
        ret = "-l" + reg.cap(1);
    return ret;
}

void
UnixMakefileGenerator::processPrlFiles()
{
    QList<QMakeLocalFileName> libdirs, frameworkdirs;
    frameworkdirs.append(QMakeLocalFileName("/System/Library/Frameworks"));
    const QString lflags[] = { "QMAKE_LIBDIR_FLAGS", "QMAKE_FRAMEWORKPATH_FLAGS", "QMAKE_LFLAGS", "QMAKE_LIBS", QString() };
    for(int i = 0; !lflags[i].isNull(); i++) {
        QStringList &l = project->values(lflags[i]);
        for(int lit = 0; lit < l.size(); ++lit) {
            QString opt = l.at(lit).trimmed();
            if(opt.startsWith("-")) {
                if(opt.startsWith("-L")) {
                    QMakeLocalFileName l(opt.right(opt.length()-2));
                    if(!libdirs.contains(l))
                       libdirs.append(l);
                } else if(opt.startsWith("-l")) {
                    QString lib = opt.right(opt.length() - 2);
                    for(int dep_i = 0; dep_i < libdirs.size(); ++dep_i) {
                        const QMakeLocalFileName &lfn = libdirs[dep_i];
                        if(!project->isActiveConfig("compile_libtool")) { //give them the .libs..
                            QString la = lfn.local() + Option::dir_sep + project->values("QMAKE_PREFIX_SHLIB").first() + lib + Option::libtool_ext;
                            if(exists(la) && QFile::exists(lfn.local() + Option::dir_sep + ".libs")) {
                                QString dot_libs = lfn.real() + Option::dir_sep + ".libs";
                                l.append("-L" + dot_libs);
                                libdirs.append(QMakeLocalFileName(dot_libs));
                            }
                        }

                        QString prl = lfn.local() + Option::dir_sep + project->values("QMAKE_PREFIX_SHLIB").first() + lib;
                        if(!project->isEmpty("QMAKE_" + lib.toUpper() + "_SUFFIX"))
                            prl += project->first("QMAKE_" + lib.toUpper() + "_SUFFIX");
                        if(processPrlFile(prl)) {
                            if(prl.startsWith(lfn.local()))
                                prl.replace(0, lfn.local().length(), lfn.real());
                            opt = linkLib(prl, lib);
                            break;
                        }
                    }
                } else if(Option::target_mode == Option::TARG_MACX_MODE && opt.startsWith("-F")) {
                    QMakeLocalFileName f(opt.right(opt.length()-2));
                    if(!frameworkdirs.contains(f))
                        frameworkdirs.append(f);
                } else if(Option::target_mode == Option::TARG_MACX_MODE && opt.startsWith("-framework")) {
                    if(opt.length() > 11)
                        opt = opt.mid(11);
                    else
                        opt = l.at(++lit);
                    opt = opt.trimmed();
                    const QList<QMakeLocalFileName> dirs = frameworkdirs + libdirs;
                    for(int dep_i = 0; dep_i < dirs.size(); ++dep_i) {
                        QString prl = dirs[dep_i].local() + "/" + opt + ".framework/" + opt + Option::prl_ext;
                        if(processPrlFile(prl))
                            break;
                    }
                }
            } else if(!opt.isNull()) {
                QString lib = opt;
                processPrlFile(lib);
#if 0
                if(ret)
                    opt = linkLib(lib, "");
#endif
                if(!opt.isEmpty())
                    l.replaceInStrings(lib, opt);
            }

            QStringList &prl_libs = project->values("QMAKE_CURRENT_PRL_LIBS");
            if(!prl_libs.isEmpty()) {
                for(int prl = 0; prl < prl_libs.size(); ++prl)
                    l.insert(lit+prl+1, prl_libs.at(prl));
                prl_libs.clear();
            }
        }

        //merge them into a logical order
        if(!project->isActiveConfig("no_smart_library_merge") && !project->isActiveConfig("no_lflags_merge")) {
            QHash<QString, QStringList> lflags;
            for(int lit = 0; lit < l.size(); ++lit) {
                QString arch("default");
                QString opt = l.at(lit).trimmed();
                if(opt.startsWith("-")) {
                    if (Option::target_mode == Option::TARG_MACX_MODE && opt.startsWith("-Xarch")) {
                        if (opt.length() > 7) {
                            arch = opt.mid(7);
                            opt = l.at(++lit);
                        }
                    }

                    if(opt.startsWith("-L") ||
                       (Option::target_mode == Option::TARG_MACX_MODE && opt.startsWith("-F"))) {
                        if(!lflags[arch].contains(opt))
                            lflags[arch].append(opt);
                    } else if(opt.startsWith("-l") || opt == "-pthread") {
                        // Make sure we keep the dependency-order of libraries
                        if (lflags[arch].contains(opt))
                            lflags[arch].removeAll(opt);
                        lflags[arch].append(opt);
                    } else if(Option::target_mode == Option::TARG_MACX_MODE && opt.startsWith("-framework")) {
                        if(opt.length() > 11)
                            opt = opt.mid(11);
                        else {
                            opt = l.at(++lit);
                            if (Option::target_mode == Option::TARG_MACX_MODE && opt.startsWith("-Xarch"))
                                opt = l.at(++lit); // The user has done the right thing and prefixed each part
                        }
                        bool found = false;
                        for(int x = 0; x < lflags[arch].size(); ++x) {
                            QString xf = lflags[arch].at(x);
                            if(xf.startsWith("-framework")) {
                                QString framework;
                                if(xf.length() > 11)
                                    framework = xf.mid(11);
                                else
                                    framework = lflags[arch].at(++x);
                                if(framework == opt) {
                                    found = true;
                                    break;
                                }
                            }
                        }
                        if(!found) {
                            lflags[arch].append("-framework");
                            lflags[arch].append(opt);
                        }
                    } else {
                        lflags[arch].append(opt);
                    }
                } else if(!opt.isNull()) {
                    if(!lflags[arch].contains(opt))
                        lflags[arch].append(opt);
                }
            }

            l =  lflags.take("default");

            // Process architecture specific options (Xarch)
            QHash<QString, QStringList>::const_iterator archIterator = lflags.constBegin();
            while (archIterator != lflags.constEnd()) {
                const QStringList archOptions = archIterator.value();
                for (int i = 0; i < archOptions.size(); ++i) {
                    l.append(QLatin1String("-Xarch_") + archIterator.key());
                    l.append(archOptions.at(i));
                }
                ++archIterator;
            }
        }
    }
}

QString
UnixMakefileGenerator::defaultInstall(const QString &t)
{
    if(t != "target" || project->first("TEMPLATE") == "subdirs")
        return QString();

    bool bundle = false;
    const QString root = "$(INSTALL_ROOT)";
    QStringList &uninst = project->values(t + ".uninstall");
    QString ret, destdir=project->first("DESTDIR");
    QString targetdir = Option::fixPathToTargetOS(project->first("target.path"), false);
    if(!destdir.isEmpty() && destdir.right(1) != Option::dir_sep)
        destdir += Option::dir_sep;
    targetdir = fileFixify(targetdir, FileFixifyAbsolute);
    if(targetdir.right(1) != Option::dir_sep)
        targetdir += Option::dir_sep;

    QStringList links;
    QString target="$(TARGET)";
    QStringList &targets = project->values(t + ".targets");
    if(!project->isEmpty("QMAKE_BUNDLE")) {
        target = project->first("QMAKE_BUNDLE");
        bundle = true;
    } else if(project->first("TEMPLATE") == "app") {
        target = "$(QMAKE_TARGET)";
    } else if(project->first("TEMPLATE") == "lib") {
        if(project->isEmpty("QMAKE_CYGWIN_SHLIB")) {
            if(!project->isActiveConfig("staticlib") && !project->isActiveConfig("plugin")) {
                if(project->isEmpty("QMAKE_HPUX_SHLIB")) {
                    links << "$(TARGET0)" << "$(TARGET1)" << "$(TARGET2)";
                } else {
                    links << "$(TARGET0)";
                }
            }
        }
    }
    for(int i = 0; i < targets.size(); ++i) {
        QString src = targets.at(i),
                dst = filePrefixRoot(root, targetdir + src.section('/', -1));
        if(!ret.isEmpty())
            ret += "\n\t";
        ret += "-$(INSTALL_FILE) \"" + src + "\" \"" + dst + "\"";
        if(!uninst.isEmpty())
            uninst.append("\n\t");
        uninst.append("-$(DEL_FILE) \"" + dst + "\"");
    }

    if(!bundle && project->isActiveConfig("compile_libtool")) {
        QString src_targ = target;
        if(src_targ == "$(TARGET)")
            src_targ = "$(TARGETL)";
        QString dst_dir = fileFixify(targetdir, FileFixifyAbsolute);
        if(QDir::isRelativePath(dst_dir))
            dst_dir = Option::fixPathToTargetOS(Option::output_dir + Option::dir_sep + dst_dir);
        ret = "-$(LIBTOOL) --mode=install cp \"" + src_targ + "\" \"" + filePrefixRoot(root, dst_dir) + "\"";
        uninst.append("-$(LIBTOOL) --mode=uninstall \"" + src_targ + "\"");
    } else {
        QString src_targ = target;
        if(!destdir.isEmpty())
            src_targ = Option::fixPathToTargetOS(destdir + target, false);
        QString dst_targ = filePrefixRoot(root, fileFixify(targetdir + target, FileFixifyAbsolute));
        if(bundle) {
            if(!ret.isEmpty())
                ret += "\n\t";
            ret += "$(DEL_FILE) -r \"" + dst_targ + "\"\n\t";
        }
        if(!ret.isEmpty())
            ret += "\n\t";

        QString copy_cmd("-");
        if (bundle)
            copy_cmd += "$(INSTALL_DIR)";
        else if (project->first("TEMPLATE") == "lib" && project->isActiveConfig("staticlib"))
            copy_cmd += "$(INSTALL_FILE)";
        else
            copy_cmd += "$(INSTALL_PROGRAM)";
        copy_cmd += " \"" + src_targ + "\" \"" + dst_targ + "\"";
        if(project->first("TEMPLATE") == "lib" && !project->isActiveConfig("staticlib")
           && project->values(t + ".CONFIG").indexOf("fix_rpath") != -1) {
            if(!project->isEmpty("QMAKE_FIX_RPATH")) {
                ret += copy_cmd;
                ret += "\n\t-" + var("QMAKE_FIX_RPATH") + " \"" +
                       dst_targ + "\" \"" + dst_targ + "\"";
            } else if(!project->isEmpty("QMAKE_LFLAGS_RPATH")) {
                ret += "-$(LINK) $(LFLAGS) " + var("QMAKE_LFLAGS_RPATH") + targetdir + " -o \"" +
                       dst_targ + "\" $(OBJECTS) $(LIBS) $(OBJCOMP)";
            } else {
                ret += copy_cmd;
            }
        } else {
            ret += copy_cmd;
        }

        if(project->first("TEMPLATE") == "lib" && project->isActiveConfig("staticlib")) {
            if(!project->isEmpty("QMAKE_RANLIB"))
                ret += QString("\n\t$(RANLIB) \"") + dst_targ + "\"";
        } else if(!project->isActiveConfig("debug") && !project->isActiveConfig("nostrip") && !project->isEmpty("QMAKE_STRIP")) {
            ret += "\n\t-$(STRIP)";
            if(project->first("TEMPLATE") == "lib" && !project->isEmpty("QMAKE_STRIPFLAGS_LIB"))
                ret += " " + var("QMAKE_STRIPFLAGS_LIB");
            else if(project->first("TEMPLATE") == "app" && !project->isEmpty("QMAKE_STRIPFLAGS_APP"))
                ret += " " + var("QMAKE_STRIPFLAGS_APP");
            if(bundle)
                ret = " \"" + dst_targ + "/Contents/MacOS/$(QMAKE_TARGET)\"";
            else
                ret += " \"" + dst_targ + "\"";
        }
        if(!uninst.isEmpty())
            uninst.append("\n\t");
        if(bundle)
            uninst.append("-$(DEL_FILE) -r \"" + dst_targ + "\"");
        else
            uninst.append("-$(DEL_FILE) \"" + dst_targ + "\"");
        if(!links.isEmpty()) {
            for(int i = 0; i < links.size(); ++i) {
                if(Option::target_mode == Option::TARG_UNIX_MODE ||
                   Option::target_mode == Option::TARG_MACX_MODE) {
                    QString link = Option::fixPathToTargetOS(destdir + links[i], false);
                    int lslash = link.lastIndexOf(Option::dir_sep);
                    if(lslash != -1)
                        link = link.right(link.length() - (lslash + 1));
                    QString dst_link = filePrefixRoot(root, fileFixify(targetdir + link, FileFixifyAbsolute));
                    ret += "\n\t-$(SYMLINK) \"$(TARGET)\" \"" + dst_link + "\"";
                    if(!uninst.isEmpty())
                        uninst.append("\n\t");
                    uninst.append("-$(DEL_FILE) \"" + dst_link + "\"");
                }
            }
        }
    }
    if(project->first("TEMPLATE") == "lib") {
        QStringList types;
        types << "prl" << "libtool" << "pkgconfig";
        for(int i = 0; i < types.size(); ++i) {
            const QString type = types.at(i);
            QString meta;
            if(type == "prl" && project->isActiveConfig("create_prl") && !project->isActiveConfig("no_install_prl") &&
               !project->isEmpty("QMAKE_INTERNAL_PRL_FILE"))
                meta = prlFileName(false);
            if(type == "libtool" && project->isActiveConfig("create_libtool") && !project->isActiveConfig("compile_libtool"))
                meta = libtoolFileName(false);
            if(type == "pkgconfig" && project->isActiveConfig("create_pc"))
                meta = pkgConfigFileName(false);
            if(!meta.isEmpty()) {
                QString src_meta = meta;
                if(!destdir.isEmpty())
                    src_meta = Option::fixPathToTargetOS(destdir + meta, false);
                QString dst_meta = filePrefixRoot(root, fileFixify(targetdir + meta, FileFixifyAbsolute));
                if(!uninst.isEmpty())
                    uninst.append("\n\t");
                uninst.append("-$(DEL_FILE) \"" + dst_meta + "\"");
                const QString replace_rule("QMAKE_" + type.toUpper() + "_INSTALL_REPLACE");
                const QString dst_meta_dir = fileInfo(dst_meta).path();
                if(!dst_meta_dir.isEmpty()) {
                    if(!ret.isEmpty())
                        ret += "\n\t";
                    ret += mkdir_p_asstring(dst_meta_dir, true);
                }
                QString install_meta = "$(INSTALL_FILE) \"" + src_meta + "\" \"" + dst_meta + "\"";
                if(project->isEmpty(replace_rule) || project->isActiveConfig("no_sed_meta_install")) {
                    if(!ret.isEmpty())
                        ret += "\n\t";
                    ret += "-" + install_meta;
                } else {
                    if(!ret.isEmpty())
                        ret += "\n\t";
                    ret += "-$(SED)";
                    QStringList replace_rules = project->values(replace_rule);
                    for(int r = 0; r < replace_rules.size(); ++r) {
                        const QString match = project->first(replace_rules.at(r) + ".match"),
                                    replace = project->first(replace_rules.at(r) + ".replace");
                        if(!match.isEmpty() /*&& match != replace*/)
                            ret += " -e \"s," + match + "," + replace + ",g\"";
                    }
                    ret += " \"" + src_meta + "\" >\"" + dst_meta + "\"";
                    //ret += " || " + install_meta;
                }
            }
        }
    }
    return ret;
}

QString
UnixMakefileGenerator::escapeFilePath(const QString &path) const
{
    QString ret = path;
    if(!ret.isEmpty()) {
        ret = unescapeFilePath(ret).replace(QLatin1Char(' '), QLatin1String("\\ "));
        debug_msg(2, "EscapeFilePath: %s -> %s", path.toLatin1().constData(), ret.toLatin1().constData());
    }
    return ret;
}

QT_END_NAMESPACE
