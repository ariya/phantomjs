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

#include "winmakefile.h"
#include "option.h"
#include "project.h"
#include "meta.h"
#include <qtextstream.h>
#include <qstring.h>
#include <qhash.h>
#include <qregexp.h>
#include <qstringlist.h>
#include <qdir.h>
#include <stdlib.h>

QT_BEGIN_NAMESPACE

Win32MakefileGenerator::Win32MakefileGenerator() : MakefileGenerator()
{
}

int
Win32MakefileGenerator::findHighestVersion(const QString &d, const QString &stem, const QString &ext)
{
    QString bd = Option::fixPathToLocalOS(d, true);
    if(!exists(bd))
        return -1;

    QMakeMetaInfo libinfo(project);
    bool libInfoRead = libinfo.readLib(bd + Option::dir_sep + stem);

    // If the library, for which we're trying to find the highest version
    // number, is a static library
    if (libInfoRead && libinfo.values("QMAKE_PRL_CONFIG").contains("staticlib"))
        return -1;

    const ProStringList &vover = project->values(ProKey("QMAKE_" + stem.toUpper() + "_VERSION_OVERRIDE"));
    if (!vover.isEmpty())
        return vover.first().toInt();

    int biggest=-1;
    if (project->isActiveConfig("link_highest_lib_version")) {
        static QHash<QString, QStringList> dirEntryListCache;
        QStringList entries = dirEntryListCache.value(bd);
        if (entries.isEmpty()) {
            QDir dir(bd);
            entries = dir.entryList();
            dirEntryListCache.insert(bd, entries);
        }

        QRegExp regx(QString("((lib)?%1([0-9]*)).(%2|prl)$").arg(stem).arg(ext), Qt::CaseInsensitive);
        for(QStringList::Iterator it = entries.begin(); it != entries.end(); ++it) {
            if(regx.exactMatch((*it))) {
                if (!regx.cap(3).isEmpty()) {
                    bool ok = true;
                    int num = regx.cap(3).toInt(&ok);
                    biggest = qMax(biggest, (!ok ? -1 : num));
                }
            }
        }
    }
    if(libInfoRead
       && !libinfo.values("QMAKE_PRL_CONFIG").contains("staticlib")
       && !libinfo.isEmpty("QMAKE_PRL_VERSION"))
       biggest = qMax(biggest, libinfo.first("QMAKE_PRL_VERSION").toQString().replace(".", "").toInt());
    return biggest;
}

bool
Win32MakefileGenerator::findLibraries()
{
    QList<QMakeLocalFileName> dirs;
  static const char * const lflags[] = { "QMAKE_LIBS", "QMAKE_LIBS_PRIVATE", 0 };
  for (int i = 0; lflags[i]; i++) {
    ProStringList &l = project->values(lflags[i]);
    for (ProStringList::Iterator it = l.begin(); it != l.end();) {
        QChar quote;
        bool modified_opt = false, remove = false;
        QString opt = (*it).trimmed().toQString();
        if((opt[0] == '\'' || opt[0] == '"') && opt[(int)opt.length()-1] == opt[0]) {
            quote = opt[0];
            opt = opt.mid(1, opt.length()-2);
        }
        if(opt.startsWith("/LIBPATH:")) {
            dirs.append(QMakeLocalFileName(opt.mid(9)));
        } else if(opt.startsWith("-L") || opt.startsWith("/L")) {
            QString libpath = Option::fixPathToTargetOS(opt.mid(2), false, false);
            QMakeLocalFileName l(libpath);
            if(!dirs.contains(l)) {
                dirs.append(l);
                modified_opt = true;
                if (!quote.isNull()) {
                    libpath = quote + libpath + quote;
                    quote = QChar();
                }
                (*it) = "/LIBPATH:" + libpath;
            } else {
                remove = true;
            }
        } else if(opt.startsWith("-l") || opt.startsWith("/l")) {
            QString lib = opt.right(opt.length() - 2), out;
            if(!lib.isEmpty()) {
                ProString suffix = project->first(ProKey("QMAKE_" + lib.toUpper() + "_SUFFIX"));
                for(QList<QMakeLocalFileName>::Iterator it = dirs.begin();
                    it != dirs.end(); ++it) {
                    QString extension;
                    int ver = findHighestVersion((*it).local(), lib);
                    if(ver > 0)
                        extension += QString::number(ver);
                    extension += suffix;
                    extension += ".lib";
                    if(QMakeMetaInfo::libExists((*it).local() + Option::dir_sep + lib) ||
                       exists((*it).local() + Option::dir_sep + lib + extension)) {
                        out = (*it).real() + Option::dir_sep + lib + extension;
                        if (out.contains(QLatin1Char(' '))) {
                            out.prepend(QLatin1Char('\"'));
                            out.append(QLatin1Char('\"'));
                        }
                        break;
                    }
                }
            }
            if(out.isEmpty())
                out = lib + ".lib";
            modified_opt = true;
            (*it) = out;
        } else if(!exists(Option::fixPathToLocalOS(opt))) {
            QList<QMakeLocalFileName> lib_dirs;
            QString file = opt;
            int slsh = file.lastIndexOf(Option::dir_sep);
            if(slsh != -1) {
                lib_dirs.append(QMakeLocalFileName(file.left(slsh+1)));
                file = file.right(file.length() - slsh - 1);
            } else {
                lib_dirs = dirs;
            }
            if(file.endsWith(".lib")) {
                file = file.left(file.length() - 4);
                if(!file.at(file.length()-1).isNumber()) {
                    ProString suffix = project->first(ProKey("QMAKE_" + file.section(Option::dir_sep, -1).toUpper() + "_SUFFIX"));
                    for(QList<QMakeLocalFileName>::Iterator dep_it = lib_dirs.begin(); dep_it != lib_dirs.end(); ++dep_it) {
                        QString lib_tmpl(file + "%1" + suffix + ".lib");
                        int ver = findHighestVersion((*dep_it).local(), file);
                        if(ver != -1) {
                            if(ver)
                                lib_tmpl = lib_tmpl.arg(ver);
                            else
                                lib_tmpl = lib_tmpl.arg("");
                            if(slsh != -1) {
                                QString dir = (*dep_it).real();
                                if(!dir.endsWith(Option::dir_sep))
                                    dir += Option::dir_sep;
                                lib_tmpl.prepend(dir);
                            }
                            modified_opt = true;
                            (*it) = lib_tmpl;
                            break;
                        }
                    }
                }
            }
        }
        if(remove) {
            it = l.erase(it);
        } else {
            if(!quote.isNull() && modified_opt)
                (*it) = quote + (*it) + quote;
            ++it;
        }
    }
  }
    return true;
}

void
Win32MakefileGenerator::processPrlFiles()
{
    const QString libArg = project->first("QMAKE_L_FLAG").toQString();
    QList<QMakeLocalFileName> libdirs;
    static const char * const lflags[] = { "QMAKE_LIBS", "QMAKE_LIBS_PRIVATE", 0 };
    for (int i = 0; lflags[i]; i++) {
        ProStringList &l = project->values(lflags[i]);
        for (int lit = 0; lit < l.size(); ++lit) {
            QString opt = l.at(lit).trimmed().toQString();
            if((opt[0] == '\'' || opt[0] == '"') && opt[(int)opt.length()-1] == opt[0])
                opt = opt.mid(1, opt.length()-2);
            if (opt.startsWith(libArg)) {
                QMakeLocalFileName l(opt.mid(libArg.length()));
                if (!libdirs.contains(l))
                    libdirs.append(l);
            } else if (!opt.startsWith("/")) {
                if (!processPrlFile(opt) && (QDir::isRelativePath(opt) || opt.startsWith("-l"))) {
                    QString tmp;
                    if (opt.startsWith("-l"))
                        tmp = opt.mid(2);
                    else
                        tmp = opt;
                    for(QList<QMakeLocalFileName>::Iterator it = libdirs.begin(); it != libdirs.end(); ++it) {
                        QString prl = (*it).local() + Option::dir_sep + tmp;
                        if (processPrlFile(prl))
                            break;
                    }
                }
            }
            ProStringList &prl_libs = project->values("QMAKE_CURRENT_PRL_LIBS");
            for (int prl = 0; prl < prl_libs.size(); ++prl) {
                ProString arg = prl_libs.at(prl);
                if (arg.startsWith(libArg))
                    arg = arg.left(libArg.length()) + escapeFilePath(arg.mid(libArg.length()).toQString());
                else if (!arg.startsWith('/'))
                    arg = escapeFilePath(arg.toQString());
                l.insert(lit + prl + 1, arg);
            }
            prl_libs.clear();
        }

        // Merge them into a logical order
        if (!project->isActiveConfig("no_smart_library_merge") && !project->isActiveConfig("no_lflags_merge")) {
            ProStringList lflags;
            for (int lit = 0; lit < l.size(); ++lit) {
                ProString opt = l.at(lit).trimmed();
                if (opt.startsWith(libArg)) {
                    if (!lflags.contains(opt))
                        lflags.append(opt);
                } else {
                    // Make sure we keep the dependency-order of libraries
                    lflags.removeAll(opt);
                    lflags.append(opt);
                }
            }
            l = lflags;
        }
    }
}


void Win32MakefileGenerator::processVars()
{
    project->values("QMAKE_ORIG_TARGET") = project->values("TARGET");
    if (project->isEmpty("QMAKE_PROJECT_NAME"))
        project->values("QMAKE_PROJECT_NAME") = project->values("QMAKE_ORIG_TARGET");
    else if (project->first("TEMPLATE").startsWith("vc"))
        project->values("MAKEFILE") = project->values("QMAKE_PROJECT_NAME");

    if (!project->values("QMAKE_INCDIR").isEmpty())
        project->values("INCLUDEPATH") += project->values("QMAKE_INCDIR");

    if (!project->values("VERSION").isEmpty()) {
        QStringList l = project->first("VERSION").toQString().split('.');
        if (l.size() > 0)
            project->values("VER_MAJ").append(l[0]);
        if (l.size() > 1)
            project->values("VER_MIN").append(l[1]);
    }

    // TARGET_VERSION_EXT will be used to add a version number onto the target name
    if (!project->isActiveConfig("skip_target_version_ext")
        && project->values("TARGET_VERSION_EXT").isEmpty()
        && !project->values("VER_MAJ").isEmpty())
        project->values("TARGET_VERSION_EXT").append(project->values("VER_MAJ").first());

    if(project->isEmpty("QMAKE_COPY_FILE"))
        project->values("QMAKE_COPY_FILE").append("$(COPY)");
    if(project->isEmpty("QMAKE_COPY_DIR"))
        project->values("QMAKE_COPY_DIR").append("xcopy /s /q /y /i");
    if (project->isEmpty("QMAKE_STREAM_EDITOR"))
        project->values("QMAKE_STREAM_EDITOR").append("$(QMAKE) -install sed");
    if(project->isEmpty("QMAKE_INSTALL_FILE"))
        project->values("QMAKE_INSTALL_FILE").append("$(COPY_FILE)");
    if(project->isEmpty("QMAKE_INSTALL_PROGRAM"))
        project->values("QMAKE_INSTALL_PROGRAM").append("$(COPY_FILE)");
    if(project->isEmpty("QMAKE_INSTALL_DIR"))
        project->values("QMAKE_INSTALL_DIR").append("$(COPY_DIR)");

    fixTargetExt();
    processRcFileVar();

    ProStringList &incDir = project->values("INCLUDEPATH");
    for (ProStringList::Iterator incDir_it = incDir.begin(); incDir_it != incDir.end(); ++incDir_it)
        if (!(*incDir_it).isEmpty())
            (*incDir_it) = Option::fixPathToTargetOS((*incDir_it).toQString(), false, false);

    ProString libArg = project->first("QMAKE_L_FLAG");
    ProStringList libs;
    ProStringList &libDir = project->values("QMAKE_LIBDIR");
    for (ProStringList::Iterator libDir_it = libDir.begin(); libDir_it != libDir.end(); ++libDir_it) {
        QString lib = (*libDir_it).toQString();
        if (!lib.isEmpty()) {
            lib.remove('"');
            if (lib.endsWith('\\'))
                lib.chop(1);
            libs << libArg + escapeFilePath(Option::fixPathToTargetOS(lib, false, false));
        }
    }
    project->values("QMAKE_LIBS") += libs + escapeFilePaths(project->values("LIBS"));
    project->values("QMAKE_LIBS_PRIVATE") += escapeFilePaths(project->values("LIBS_PRIVATE"));

    if (project->values("TEMPLATE").contains("app")) {
        project->values("QMAKE_CFLAGS") += project->values("QMAKE_CFLAGS_APP");
        project->values("QMAKE_CXXFLAGS") += project->values("QMAKE_CXXFLAGS_APP");
        project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_APP");
    } else if (project->values("TEMPLATE").contains("lib") && project->isActiveConfig("dll")) {
        if(!project->isActiveConfig("plugin") || !project->isActiveConfig("plugin_no_share_shlib_cflags")) {
            project->values("QMAKE_CFLAGS") += project->values("QMAKE_CFLAGS_SHLIB");
            project->values("QMAKE_CXXFLAGS") += project->values("QMAKE_CXXFLAGS_SHLIB");
        }
        if (project->isActiveConfig("plugin")) {
            project->values("QMAKE_CFLAGS") += project->values("QMAKE_CFLAGS_PLUGIN");
            project->values("QMAKE_CXXFLAGS") += project->values("QMAKE_CXXFLAGS_PLUGIN");
            project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_PLUGIN");
        } else {
            project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_SHLIB");
        }
    }
}

void Win32MakefileGenerator::fixTargetExt()
{
    if (project->isEmpty("QMAKE_EXTENSION_STATICLIB"))
        project->values("QMAKE_EXTENSION_STATICLIB").append("lib");
    if (project->isEmpty("QMAKE_EXTENSION_SHLIB"))
        project->values("QMAKE_EXTENSION_SHLIB").append("dll");

    if (!project->values("QMAKE_APP_FLAG").isEmpty()) {
        project->values("TARGET_EXT").append(".exe");
    } else if (project->isActiveConfig("shared")) {
        project->values("TARGET_EXT").append(project->first("TARGET_VERSION_EXT") + "."
                + project->first("QMAKE_EXTENSION_SHLIB"));
        project->values("TARGET").first() = project->first("QMAKE_PREFIX_SHLIB") + project->first("TARGET");
    } else {
        project->values("TARGET_EXT").append("." + project->first("QMAKE_EXTENSION_STATICLIB"));
        project->values("TARGET").first() = project->first("QMAKE_PREFIX_STATICLIB") + project->first("TARGET");
    }
}

void Win32MakefileGenerator::processRcFileVar()
{
    if (Option::qmake_mode == Option::QMAKE_GENERATE_NOTHING)
        return;

    if (((!project->values("VERSION").isEmpty() || !project->values("RC_ICONS").isEmpty())
        && project->values("RC_FILE").isEmpty()
        && project->values("RES_FILE").isEmpty()
        && !project->isActiveConfig("no_generated_target_info")
        && (project->isActiveConfig("shared") || !project->values("QMAKE_APP_FLAG").isEmpty()))
        || !project->values("QMAKE_WRITE_DEFAULT_RC").isEmpty()){

        QByteArray rcString;
        QTextStream ts(&rcString, QFile::WriteOnly);

        QStringList vers = project->first("VERSION").toQString().split(".", QString::SkipEmptyParts);
        for (int i = vers.size(); i < 4; i++)
            vers += "0";
        QString versionString = vers.join('.');

        QStringList rcIcons;
        foreach (const ProString &icon, project->values("RC_ICONS"))
            rcIcons.append(fileFixify(icon.toQString(), FileFixifyAbsolute));

        QString companyName;
        if (!project->values("QMAKE_TARGET_COMPANY").isEmpty())
            companyName = project->values("QMAKE_TARGET_COMPANY").join(' ');

        QString description;
        if (!project->values("QMAKE_TARGET_DESCRIPTION").isEmpty())
            description = project->values("QMAKE_TARGET_DESCRIPTION").join(' ');

        QString copyright;
        if (!project->values("QMAKE_TARGET_COPYRIGHT").isEmpty())
            copyright = project->values("QMAKE_TARGET_COPYRIGHT").join(' ');

        QString productName;
        if (!project->values("QMAKE_TARGET_PRODUCT").isEmpty())
            productName = project->values("QMAKE_TARGET_PRODUCT").join(' ');
        else
            productName = project->first("TARGET").toQString();

        QString originalName = project->values("TARGET").first() + project->values("TARGET_EXT").first();
        int rcLang = project->intValue("RC_LANG", 1033);            // default: English(USA)
        int rcCodePage = project->intValue("RC_CODEPAGE", 1200);    // default: Unicode

        ts << "# if defined(UNDER_CE)\n";
        ts << "#  include <winbase.h>\n";
        ts << "# else\n";
        ts << "#  include <winver.h>\n";
        ts << "# endif\n";
        ts << endl;
        if (!rcIcons.isEmpty()) {
            for (int i = 0; i < rcIcons.size(); ++i)
                ts << QString("IDI_ICON%1\tICON\tDISCARDABLE\t%2").arg(i + 1).arg(cQuoted(rcIcons[i])) << endl;
            ts << endl;
        }
        ts << "VS_VERSION_INFO VERSIONINFO\n";
        ts << "\tFILEVERSION " << QString(versionString).replace(".", ",") << endl;
        ts << "\tPRODUCTVERSION " << QString(versionString).replace(".", ",") << endl;
        ts << "\tFILEFLAGSMASK 0x3fL\n";
        ts << "#ifdef _DEBUG\n";
        ts << "\tFILEFLAGS VS_FF_DEBUG\n";
        ts << "#else\n";
        ts << "\tFILEFLAGS 0x0L\n";
        ts << "#endif\n";
        ts << "\tFILEOS VOS__WINDOWS32\n";
        if (project->isActiveConfig("shared"))
            ts << "\tFILETYPE VFT_DLL\n";
        else
            ts << "\tFILETYPE VFT_APP\n";
        ts << "\tFILESUBTYPE 0x0L\n";
        ts << "\tBEGIN\n";
        ts << "\t\tBLOCK \"StringFileInfo\"\n";
        ts << "\t\tBEGIN\n";
        ts << "\t\t\tBLOCK \""
           << QString("%1%2").arg(rcLang, 4, 16, QLatin1Char('0')).arg(rcCodePage, 4, 16, QLatin1Char('0'))
           << "\"\n";
        ts << "\t\t\tBEGIN\n";
        ts << "\t\t\t\tVALUE \"CompanyName\", \"" << companyName << "\\0\"\n";
        ts << "\t\t\t\tVALUE \"FileDescription\", \"" <<  description << "\\0\"\n";
        ts << "\t\t\t\tVALUE \"FileVersion\", \"" << versionString << "\\0\"\n";
        ts << "\t\t\t\tVALUE \"LegalCopyright\", \"" << copyright << "\\0\"\n";
        ts << "\t\t\t\tVALUE \"OriginalFilename\", \"" << originalName << "\\0\"\n";
        ts << "\t\t\t\tVALUE \"ProductName\", \"" << productName << "\\0\"\n";
        ts << "\t\t\t\tVALUE \"ProductVersion\", \"" << versionString << "\\0\"\n";
        ts << "\t\t\tEND\n";
        ts << "\t\tEND\n";
        ts << "\t\tBLOCK \"VarFileInfo\"\n";
        ts << "\t\tBEGIN\n";
        ts << "\t\t\tVALUE \"Translation\", "
           << QString("0x%1").arg(rcLang, 4, 16, QLatin1Char('0'))
           << ", " << QString("%1").arg(rcCodePage, 4) << endl;
        ts << "\t\tEND\n";
        ts << "\tEND\n";
        ts << "/* End of Version info */\n";
        ts << endl;

        ts.flush();


        QString rcFilename = project->values("OUT_PWD").first()
                           + "/"
                           + project->values("TARGET").first()
                           + "_resource"
                           + ".rc";
        QFile rcFile(QDir::cleanPath(rcFilename));

        bool writeRcFile = true;
        if (rcFile.exists() && rcFile.open(QFile::ReadOnly)) {
            writeRcFile = rcFile.readAll() != rcString;
            rcFile.close();
        }
        if (writeRcFile) {
            bool ok;
            ok = rcFile.open(QFile::WriteOnly);
            if (!ok) {
                // The file can't be opened... try creating the containing
                // directory first (needed for clean shadow builds)
                QDir().mkpath(QFileInfo(rcFile).path());
                ok = rcFile.open(QFile::WriteOnly);
            }
            if (!ok) {
                ::fprintf(stderr, "Cannot open for writing: %s", rcFile.fileName().toLatin1().constData());
                ::exit(1);
            }
            rcFile.write(rcString);
            rcFile.close();
        }
        if (project->values("QMAKE_WRITE_DEFAULT_RC").isEmpty())
            project->values("RC_FILE").insert(0, rcFile.fileName());
    }
    if (!project->values("RC_FILE").isEmpty()) {
        if (!project->values("RES_FILE").isEmpty()) {
            fprintf(stderr, "Both rc and res file specified.\n");
            fprintf(stderr, "Please specify one of them, not both.");
            exit(1);
        }
        QString resFile = project->first("RC_FILE").toQString();

        // if this is a shadow build then use the absolute path of the rc file
        if (Option::output_dir != qmake_getpwd()) {
            QFileInfo fi(resFile);
            project->values("RC_FILE").first() = fi.absoluteFilePath();
        }

        resFile.replace(".rc", Option::res_ext);
        project->values("RES_FILE").prepend(fileInfo(resFile).fileName());
        if (!project->values("OBJECTS_DIR").isEmpty()) {
            QString resDestDir;
            if (project->isActiveConfig("staticlib"))
                resDestDir = fileInfo(project->first("DESTDIR").toQString()).absoluteFilePath();
            else
                resDestDir = project->first("OBJECTS_DIR").toQString();
            resDestDir.append(Option::dir_sep);
            project->values("RES_FILE").first().prepend(resDestDir);
        }
        project->values("RES_FILE").first() = Option::fixPathToTargetOS(
                    project->values("RES_FILE").first().toQString(), false, false);
        project->values("POST_TARGETDEPS") += project->values("RES_FILE");
        project->values("CLEAN_FILES") += project->values("RES_FILE");
    }
}

void Win32MakefileGenerator::writeCleanParts(QTextStream &t)
{
    t << "clean: compiler_clean " << var("CLEAN_DEPS");
    {
        const char *clean_targets[] = { "OBJECTS", "QMAKE_CLEAN", "CLEAN_FILES", 0 };
        for(int i = 0; clean_targets[i]; ++i) {
            const ProStringList &list = project->values(clean_targets[i]);
            const QString del_statement("-$(DEL_FILE)");
            if(project->isActiveConfig("no_delete_multiple_files")) {
                for (ProStringList::ConstIterator it = list.begin(); it != list.end(); ++it)
                    t << "\n\t" << del_statement << " " << escapeFilePath((*it));
            } else {
                QString files, file;
                const int commandlineLimit = 2047; // NT limit, expanded
                for (ProStringList::ConstIterator it = list.begin(); it != list.end(); ++it) {
                    file = " " + escapeFilePath((*it));
                    if(del_statement.length() + files.length() +
                       qMax(fixEnvVariables(file).length(), file.length()) > commandlineLimit) {
                        t << "\n\t" << del_statement << files;
                        files.clear();
                    }
                    files += file;
                }
                if(!files.isEmpty())
                    t << "\n\t" << del_statement << files;
            }
        }
    }
    t << endl << endl;

    t << "distclean: clean " << var("DISTCLEAN_DEPS");
    {
        const char *clean_targets[] = { "QMAKE_DISTCLEAN", 0 };
        for(int i = 0; clean_targets[i]; ++i) {
            const ProStringList &list = project->values(clean_targets[i]);
            const QString del_statement("-$(DEL_FILE)");
            if(project->isActiveConfig("no_delete_multiple_files")) {
                for (ProStringList::ConstIterator it = list.begin(); it != list.end(); ++it)
                    t << "\n\t" << del_statement << " "
                      << escapeFilePath(Option::fixPathToTargetOS((*it).toQString()));
            } else {
                QString files, file;
                const int commandlineLimit = 2047; // NT limit, expanded
                for (ProStringList::ConstIterator it = list.begin(); it != list.end(); ++it) {
                    file = " " + escapeFilePath(Option::fixPathToTargetOS((*it).toQString()));
                    if(del_statement.length() + files.length() +
                       qMax(fixEnvVariables(file).length(), file.length()) > commandlineLimit) {
                        t << "\n\t" << del_statement << files;
                        files.clear();
                    }
                    files += file;
                }
                if(!files.isEmpty())
                    t << "\n\t" << del_statement << files;
            }
        }
    }
    t << "\n\t-$(DEL_FILE) $(DESTDIR_TARGET)\n";
    {
        QString ofile = Option::fixPathToTargetOS(fileFixify(Option::output.fileName()));
        if(!ofile.isEmpty())
            t << "\t-$(DEL_FILE) " << ofile << endl;
    }
    t << endl;
}

void Win32MakefileGenerator::writeIncPart(QTextStream &t)
{
    t << "INCPATH       = ";

    const ProStringList &incs = project->values("INCLUDEPATH");
    for(int i = 0; i < incs.size(); ++i) {
        QString inc = incs.at(i).toQString();
        inc.replace(QRegExp("\\\\$"), "");
        inc.replace(QRegExp("\""), "");
        if(!inc.isEmpty())
            t << "-I\"" << inc << "\" ";
    }
    t << "-I\"" << specdir() << "\""
      << endl;
}

void Win32MakefileGenerator::writeStandardParts(QTextStream &t)
{
    t << "####### Compiler, tools and options\n\n";
    t << "CC            = " << var("QMAKE_CC") << endl;
    t << "CXX           = " << var("QMAKE_CXX") << endl;
    t << "DEFINES       = "
      << varGlue("PRL_EXPORT_DEFINES","-D"," -D"," ")
      << varGlue("DEFINES","-D"," -D","") << endl;
    t << "CFLAGS        = " << var("QMAKE_CFLAGS") << " $(DEFINES)\n";
    t << "CXXFLAGS      = " << var("QMAKE_CXXFLAGS") << " $(DEFINES)\n";

    writeIncPart(t);
    writeLibsPart(t);

    t << "QMAKE         = " << var("QMAKE_QMAKE") << endl;
    t << "IDC           = " << (project->isEmpty("QMAKE_IDC") ? QString("idc") :
                              Option::fixPathToTargetOS(var("QMAKE_IDC"), false)) << endl;
    t << "IDL           = " << (project->isEmpty("QMAKE_IDL") ? QString("midl") :
                              Option::fixPathToTargetOS(var("QMAKE_IDL"), false)) << endl;
    t << "ZIP           = " << var("QMAKE_ZIP") << endl;
    t << "DEF_FILE      = " << varList("DEF_FILE") << endl;
    t << "RES_FILE      = " << varList("RES_FILE") << endl; // Not on mingw, can't see why not though...
    t << "COPY          = " << var("QMAKE_COPY") << endl;
    t << "SED           = " << var("QMAKE_STREAM_EDITOR") << endl;
    t << "COPY_FILE     = " << var("QMAKE_COPY_FILE") << endl;
    t << "COPY_DIR      = " << var("QMAKE_COPY_DIR") << endl;
    t << "DEL_FILE      = " << var("QMAKE_DEL_FILE") << endl;
    t << "DEL_DIR       = " << var("QMAKE_DEL_DIR") << endl;
    t << "MOVE          = " << var("QMAKE_MOVE") << endl;
    t << "CHK_DIR_EXISTS= " << var("QMAKE_CHK_DIR_EXISTS") << endl;
    t << "MKDIR         = " << var("QMAKE_MKDIR") << endl;
    t << "INSTALL_FILE    = " << var("QMAKE_INSTALL_FILE") << endl;
    t << "INSTALL_PROGRAM = " << var("QMAKE_INSTALL_PROGRAM") << endl;
    t << "INSTALL_DIR     = " << var("QMAKE_INSTALL_DIR") << endl;
    t << endl;

    t << "####### Output directory\n\n";
    if(!project->values("OBJECTS_DIR").isEmpty())
        t << "OBJECTS_DIR   = " << var("OBJECTS_DIR").replace(QRegExp("\\\\$"),"") << endl;
    else
        t << "OBJECTS_DIR   = . \n";
    t << endl;

    t << "####### Files\n\n";
    t << "SOURCES       = " << valList(escapeFilePaths(project->values("SOURCES")))
      << " " << valList(escapeFilePaths(project->values("GENERATED_SOURCES"))) << endl;

    // do this here so we can set DEST_TARGET to be the complete path to the final target if it is needed.
    QString orgDestDir = var("DESTDIR");
    QString destDir = Option::fixPathToTargetOS(orgDestDir, false);
    if (!destDir.isEmpty() && (orgDestDir.endsWith('/') || orgDestDir.endsWith(Option::dir_sep)))
        destDir += Option::dir_sep;
    QString target = QString(project->first("TARGET")+project->first("TARGET_EXT"));
    target.remove("\"");
    project->values("DEST_TARGET").prepend(destDir + target);

    writeObjectsPart(t);

    writeExtraCompilerVariables(t);
    writeExtraVariables(t);

    t << "DIST          = " << varList("DISTFILES") << endl;
    t << "QMAKE_TARGET  = " << var("QMAKE_ORIG_TARGET") << endl;
    // The comment is important to maintain variable compatibility with Unix
    // Makefiles, while not interpreting a trailing-slash as a linebreak
    t << "DESTDIR        = " << escapeFilePath(destDir) << " #avoid trailing-slash linebreak\n";
    t << "TARGET         = " << escapeFilePath(target) << endl;
    t << "DESTDIR_TARGET = " << escapeFilePath(var("DEST_TARGET")) << endl;
    t << endl;

    t << "####### Implicit rules\n\n";
    writeImplicitRulesPart(t);

    t << "####### Build rules\n\n";
    writeBuildRulesPart(t);

    if(project->isActiveConfig("shared") && !project->values("DLLDESTDIR").isEmpty()) {
        const ProStringList &dlldirs = project->values("DLLDESTDIR");
        for (ProStringList::ConstIterator dlldir = dlldirs.begin(); dlldir != dlldirs.end(); ++dlldir) {
            t << "\t-$(COPY_FILE) \"$(DESTDIR_TARGET)\" "
              << Option::fixPathToTargetOS((*dlldir).toQString(), false) << endl;
        }
    }
    t << endl;

    writeRcFilePart(t);

    writeMakeQmake(t);

    QStringList dist_files = fileFixify(Option::mkfile::project_files);
    if(!project->isEmpty("QMAKE_INTERNAL_INCLUDED_FILES"))
        dist_files += project->values("QMAKE_INTERNAL_INCLUDED_FILES").toQStringList();
    if(!project->isEmpty("TRANSLATIONS"))
        dist_files << var("TRANSLATIONS");
    if(!project->isEmpty("FORMS")) {
        const ProStringList &forms = project->values("FORMS");
        for (ProStringList::ConstIterator formit = forms.begin(); formit != forms.end(); ++formit) {
            QString ui_h = fileFixify((*formit) + Option::h_ext.first());
            if(exists(ui_h))
                dist_files << ui_h;
        }
    }
    t << "dist:\n\t"
      << "$(ZIP) " << var("QMAKE_ORIG_TARGET") << ".zip $(SOURCES) $(DIST) "
      << dist_files.join(' ') << " " << var("TRANSLATIONS") << " ";
    if(!project->isEmpty("QMAKE_EXTRA_COMPILERS")) {
        const ProStringList &quc = project->values("QMAKE_EXTRA_COMPILERS");
        for (ProStringList::ConstIterator it = quc.begin(); it != quc.end(); ++it) {
            const ProStringList &inputs = project->values(ProKey(*it + ".input"));
            for (ProStringList::ConstIterator input = inputs.begin(); input != inputs.end(); ++input)
                t << (*input) << " ";
        }
    }
    t << endl << endl;

    writeCleanParts(t);
    writeExtraTargets(t);
    writeExtraCompilerTargets(t);
    t << endl << endl;
}

void Win32MakefileGenerator::writeLibsPart(QTextStream &t)
{
    if(project->isActiveConfig("staticlib") && project->first("TEMPLATE") == "lib") {
        t << "LIBAPP        = " << var("QMAKE_LIB") << endl;
        t << "LIBFLAGS      = " << var("QMAKE_LIBFLAGS") << endl;
    } else {
        t << "LINKER        = " << var("QMAKE_LINK") << endl;
        t << "LFLAGS        = " << var("QMAKE_LFLAGS") << endl;
        t << "LIBS          = " << var("QMAKE_LIBS") << " " << var("QMAKE_LIBS_PRIVATE") << endl;
    }
}

void Win32MakefileGenerator::writeObjectsPart(QTextStream &t)
{
    t << "OBJECTS       = " << valList(escapeDependencyPaths(project->values("OBJECTS"))) << endl;
}

void Win32MakefileGenerator::writeImplicitRulesPart(QTextStream &t)
{
    t << ".SUFFIXES:";
    for(QStringList::Iterator cppit = Option::cpp_ext.begin(); cppit != Option::cpp_ext.end(); ++cppit)
        t << " " << (*cppit);
    for(QStringList::Iterator cit = Option::c_ext.begin(); cit != Option::c_ext.end(); ++cit)
        t << " " << (*cit);
    t << endl << endl;
    for(QStringList::Iterator cppit = Option::cpp_ext.begin(); cppit != Option::cpp_ext.end(); ++cppit)
        t << (*cppit) << Option::obj_ext << ":\n\t" << var("QMAKE_RUN_CXX_IMP") << endl << endl;
    for(QStringList::Iterator cit = Option::c_ext.begin(); cit != Option::c_ext.end(); ++cit)
        t << (*cit) << Option::obj_ext << ":\n\t" << var("QMAKE_RUN_CC_IMP") << endl << endl;
}

void Win32MakefileGenerator::writeBuildRulesPart(QTextStream &)
{
}

void Win32MakefileGenerator::writeRcFilePart(QTextStream &t)
{
    if(!project->values("RC_FILE").isEmpty()) {
        const ProString res_file = project->first("RES_FILE");
        const QString rc_file = fileFixify(project->first("RC_FILE").toQString());
        // The resource tool needs to have the same defines passed in as the compiler, since you may
        // use these defines in the .rc file itself. Also, we need to add the _DEBUG define manually
        // since the compiler defines this symbol by itself, and we use it in the automatically
        // created rc file when VERSION is define the .pro file.

        const ProStringList rcIncPaths = project->values("RC_INCLUDEPATH");
        QString incPathStr;
        for (int i = 0; i < rcIncPaths.count(); ++i) {
            const ProString &path = rcIncPaths.at(i);
            if (path.isEmpty())
                continue;
            incPathStr += QStringLiteral(" /i ");
            incPathStr += escapeFilePath(path);
        }

        t << res_file << ": " << rc_file << "\n\t"
          << var("QMAKE_RC") << (project->isActiveConfig("debug") ? " -D_DEBUG" : "")
          << " $(DEFINES)" << incPathStr << " -fo " << res_file << " " << rc_file;
        t << endl << endl;
    }
}

QString Win32MakefileGenerator::getLibTarget()
{
    return QString(project->first("TARGET") + project->first("TARGET_VERSION_EXT") + ".lib");
}

QString Win32MakefileGenerator::defaultInstall(const QString &t)
{
    if((t != "target" && t != "dlltarget") ||
       (t == "dlltarget" && (project->first("TEMPLATE") != "lib" || !project->isActiveConfig("shared"))) ||
        project->first("TEMPLATE") == "subdirs")
       return QString();

    const QString root = "$(INSTALL_ROOT)";
    ProStringList &uninst = project->values(ProKey(t + ".uninstall"));
    QString ret;
    QString targetdir = Option::fixPathToTargetOS(project->first(ProKey(t + ".path")).toQString(), false);
    targetdir = fileFixify(targetdir, FileFixifyAbsolute);
    if(targetdir.right(1) != Option::dir_sep)
        targetdir += Option::dir_sep;

    if(t == "target" && project->first("TEMPLATE") == "lib") {
        if(project->isActiveConfig("create_prl") && !project->isActiveConfig("no_install_prl") &&
           !project->isEmpty("QMAKE_INTERNAL_PRL_FILE")) {
            QString dst_prl = Option::fixPathToTargetOS(project->first("QMAKE_INTERNAL_PRL_FILE").toQString());
            int slsh = dst_prl.lastIndexOf(Option::dir_sep);
            if(slsh != -1)
                dst_prl = dst_prl.right(dst_prl.length() - slsh - 1);
            dst_prl = filePrefixRoot(root, targetdir + dst_prl);
            ret += installMetaFile(ProKey("QMAKE_PRL_INSTALL_REPLACE"), project->first("QMAKE_INTERNAL_PRL_FILE").toQString(), dst_prl);
            if(!uninst.isEmpty())
                uninst.append("\n\t");
            uninst.append("-$(DEL_FILE) \"" + dst_prl + "\"");
        }
        if(project->isActiveConfig("create_pc")) {
            QString dst_pc = pkgConfigFileName(false);
            if (!dst_pc.isEmpty()) {
                dst_pc = filePrefixRoot(root, targetdir + dst_pc);
                const QString dst_pc_dir = Option::fixPathToTargetOS(fileInfo(dst_pc).path(), false);
                if (!dst_pc_dir.isEmpty()) {
                    if (!ret.isEmpty())
                        ret += "\n\t";
                    ret += mkdir_p_asstring(dst_pc_dir, true);
                }
                if(!ret.isEmpty())
                    ret += "\n\t";
                ret += installMetaFile(ProKey("QMAKE_PKGCONFIG_INSTALL_REPLACE"), pkgConfigFileName(true), dst_pc);
                if(!uninst.isEmpty())
                    uninst.append("\n\t");
                uninst.append("-$(DEL_FILE) \"" + dst_pc + "\"");
            }
        }
        if(project->isActiveConfig("shared") && !project->isActiveConfig("plugin")) {
            QString lib_target = getLibTarget();
            lib_target.remove('"');
            QString src_targ = (project->isEmpty("DESTDIR") ? QString("$(DESTDIR)") : project->first("DESTDIR")) + lib_target;
            QString dst_targ = filePrefixRoot(root, fileFixify(targetdir + lib_target, FileFixifyAbsolute));
            if(!ret.isEmpty())
                ret += "\n\t";
            ret += QString("-$(INSTALL_FILE)") + " \"" + src_targ + "\" \"" + dst_targ + "\"";
            if(!uninst.isEmpty())
                uninst.append("\n\t");
            uninst.append("-$(DEL_FILE) \"" + dst_targ + "\"");
        }
    }

    if (t == "dlltarget" || project->values(ProKey(t + ".CONFIG")).indexOf("no_dll") == -1) {
        QString src_targ = "$(DESTDIR_TARGET)";
        QString dst_targ = filePrefixRoot(root, fileFixify(targetdir + "$(TARGET)", FileFixifyAbsolute));
        if(!ret.isEmpty())
            ret += "\n\t";
        ret += QString("-$(INSTALL_FILE)") + " \"" + src_targ + "\" \"" + dst_targ + "\"";
        if(!uninst.isEmpty())
            uninst.append("\n\t");
        uninst.append("-$(DEL_FILE) \"" + dst_targ + "\"");
    }
    return ret;
}

QString Win32MakefileGenerator::escapeFilePath(const QString &path) const
{
    QString ret = path;
    if(!ret.isEmpty()) {
        ret = unescapeFilePath(ret);
        if (ret.contains(' ') || ret.contains('\t'))
            ret = "\"" + ret + "\"";
        debug_msg(2, "EscapeFilePath: %s -> %s", path.toLatin1().constData(), ret.toLatin1().constData());
    }
    return ret;
}

QString Win32MakefileGenerator::cQuoted(const QString &str)
{
    QString ret = str;
    ret.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    ret.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    ret.prepend(QLatin1Char('"'));
    ret.append(QLatin1Char('"'));
    return ret;
}

QT_END_NAMESPACE
